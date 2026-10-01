"""Versioned interactive schedule, independent trace checks and response tables."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import csv
import math
import statistics
import struct

from contracts import descriptor

SCHEMA = "fourier-transitions-v1"
SUITE = "interactive-v1"


def _float(value):
    return struct.unpack("f", struct.pack("f", value))[0]


def validate_transition_config(config, registry=None):
    """Reject unsupported controls rather than silently dropping requests."""
    d = descriptor(config["backend"], registry)
    supported = config["backend"] in ("core-float", "core-double",
                                      "pffft-hybrid-float", "pffft-scheduled-batch-float")
    if not supported:
        raise ValueError("Backend has no transition adapter")
    if (config.get("transition_suite") != SUITE or
            type(config.get("transition_control", False)) is not bool or
            not 128 <= config["n"] <= 2048 or config["n"] & (config["n"]-1) or
            config["hop"] < 8 or
            config["callbacks"]*config["block"] < 26*config["hop"]):
        raise ValueError("Unsupported transition suite or observation horizon")
    required = dict(count=1, voices=1, load=0, cache_mib=0, state="startup",
                    alignment="aligned", warm_hops=0, smooth=0)
    if any(config.get(k) != v for k, v in required.items()) or config.get("callback_offset", 0):
        raise ValueError("Transitions require an unmodified single startup workload")
    if config.get("pass_name", config.get("pass")) != "callback":
        raise ValueError("Transitions retain callback windows only")
    return d


def manifest(config, registry=None):
    """Compute every requested full setting, including no-op and replacement."""
    d = validate_transition_config(config, registry)
    n, h = config["n"], config["hop"]
    initial = dict(n=n, hop=h, window=8, rate=config["rate"], octave=0., alpha=0.)
    settings, events = dict(initial), []

    def add(sample, reason, **changes):
        settings.update(changes)
        events.append(dict(generation=len(events)+1, request_sample=sample, reason=reason,
                           settings=dict(initial if config.get("transition_control", False) else settings)))

    add(2*h, "length-increase-boundary", n=min(16384, 8*n))
    add(4*h+1, "length-decrease-early", n=n)
    add(6*h+h//2, "hop-increase-middle", hop=2*h)
    add(11*h-2, "window-late", window=11)
    add(13*h-1, "smoothing-before-publication", octave=_float(1/3), alpha=_float(.8))
    add(15*h, "no-op-boundary")
    add(17*h+1, "replace-pending-first", window=8)
    add(17*h+2, "replace-pending-last", alpha=0.)
    add(21*h, "hop-decrease-boundary", hop=h//2)
    add(23*h+1, "smoothing-disable", octave=0.)
    add(config["callbacks"]*config["block"]-1, "horizon-no-response", window=0)
    return dict(schema=SCHEMA, suite=SUITE, initial=initial, events=events,
                horizon=config["callbacks"]*config["block"], immediate=d["schedule"] == "immediate")


def _lifecycle(config, registry=None):
    model = manifest(config, registry)
    events = model["events"]
    for event in events:
        event.update(application_sample=-1, first_publication_sample=-1, replaced_by=-1)
    settings = model["initial"]
    endpoint = generation = history_start = next_event = 0
    pending = None
    publications = []
    # Independent host state simulation: accepted boundaries are generated from
    # old hop, then the new hop governs the frame just accepted at that boundary.
    for sample in range(model["horizon"]):
        if next_event < len(events) and events[next_event]["request_sample"] == sample:
            if pending is not None:
                pending["replaced_by"] = events[next_event]["generation"]
            pending = events[next_event]
            next_event += 1
        if sample == endpoint and pending is not None:
            pending["application_sample"] = sample
            if settings["n"] != pending["settings"]["n"]:
                history_start = sample
            settings = pending["settings"]
            generation = pending["generation"]
            pending = None
        if sample == endpoint + (0 if model["immediate"] else settings["hop"]-1):
            publications.append(dict(instance=0, channel=0, generation=generation,
                                     endpoint=endpoint, publication_sample=sample,
                                     history_start=history_start, bins=settings["n"]//2+1,
                                     n=settings["n"], hop=settings["hop"]))
            if generation and events[generation-1]["first_publication_sample"] < 0:
                events[generation-1]["first_publication_sample"] = sample
        if sample == endpoint+settings["hop"]-1:
            endpoint += settings["hop"]
    for event in events:
        event["outcome"] = ("replaced" if event["replaced_by"] >= 0 else
                            "published" if event["first_publication_sample"] >= 0 else
                            "applied-no-publication" if event["application_sample"] >= 0 else
                            "pending-no-response")
    return model, publications


def expected_publications(config, registry=None):
    """Variable schedule used to check raw CSV independently of its JSON trace."""
    return _lifecycle(config, registry)[1]


def validate_transition_trace(trace, config, registry=None):
    """Require complete event identity, generation-local norms and all spectra."""
    model, publications = _lifecycle(config, registry)
    expected_top = dict(schema=SCHEMA, suite=SUITE, backend=config["backend"],
                        control=config.get("transition_control", False), horizon=model["horizon"],
                        block=config["block"], rate=config["rate"], instance=0, channel=0,
                        reference_precision="binary64 FFT; long-double direct DFT at N<=256",
                        reference_mantissa_bits=53,
                        initial=model["initial"], immediate=model["immediate"], events=model["events"])
    for key, value in expected_top.items():
        if trace.get(key) != value:
            raise ValueError("Transition trace differs from independent contract: " + key)
    if type(trace.get("direct_dft_mantissa_bits")) is not int or trace["direct_dft_mantissa_bits"] < 53:
        raise ValueError("Invalid direct DFT reference precision")
    for key in ("time_origin", "retention", "latch", "memory_policy"):
        if not isinstance(trace.get(key), str) or not trace[key]:
            raise ValueError("Missing transition policy: " + key)
    resources = validate_transition_resources(trace.get("resource_contract", {}), config, registry)
    if trace.get("memory_policy") != resources["memory_policy"] or trace.get("provider_instances") != resources["provider_instances"]:
        raise ValueError("Transition timed instance differs from resource contract")
    actual = trace.get("publications")
    if not isinstance(actual, list) or len(actual) != len(publications):
        raise ValueError("Missing transition publications")
    d = descriptor(config["backend"], registry)
    tolerance = 3e-4 if d["precision"] == "float" else 1e-10
    for row, expected in zip(actual, publications):
        for key in ("instance", "channel", "generation", "endpoint", "publication_sample", "history_start", "bins"):
            if type(row.get(key)) is not int or row[key] != expected[key]:
                raise ValueError("Wrong or mixed transition frame identity: " + key)
        for key in ("max_abs_error", "max_reference"):
            value = row.get(key)
            if not isinstance(value, (int, float)) or not math.isfinite(value) or value < 0:
                raise ValueError("Invalid transition absolute numerical error")
        accuracy = row.get("accuracy", {})
        if (accuracy.get("policy") != "spectrum-norms-v1" or accuracy.get("tolerance") != tolerance or
                accuracy.get("vectors") != 1 or accuracy.get("values") != row["bins"]):
            raise ValueError("Incomplete transition numerical audit")
        for key in ("max_relative_l2", "max_relative_linf", "max_legacy_scaled_error"):
            value = accuracy.get(key)
            if not isinstance(value, (int, float)) or not math.isfinite(value) or value < 0:
                raise ValueError("Non-finite transition accuracy")
            if key != "max_legacy_scaled_error" and value > tolerance:
                raise ValueError("Corrupt or mixed transition spectrum")
        for key in ("zero_vectors", "legacy_pointwise_failures"):
            value = accuracy.get(key)
            if type(value) is not int or not 0 <= value <= (1 if key == "zero_vectors" else row["bins"]):
                raise ValueError("Invalid transition diagnostic count")
        worst = accuracy.get("worst_pointwise", {})
        for key in ("actual", "reference"):
            value = worst.get(key)
            if not isinstance(value, (int, float)) or not math.isfinite(value):
                raise ValueError("Non-finite transition diagnostic")
        norm = row["max_abs_error"]/row["max_reference"] if row["max_reference"] else 0
        if (not row["max_reference"] and row["max_abs_error"] or
                not math.isclose(norm, accuracy["max_relative_linf"], rel_tol=1e-12, abs_tol=1e-30)):
            raise ValueError("Transition error norm is inconsistent")
        pointwise = abs(worst["actual"]-worst["reference"])/max(1., abs(worst["reference"]))
        if not math.isclose(pointwise, accuracy["max_legacy_scaled_error"], rel_tol=1e-12, abs_tol=1e-30):
            raise ValueError("Transition pointwise diagnostic is inconsistent")
        if (accuracy["legacy_pointwise_failures"] > 0) != (pointwise > tolerance):
            raise ValueError("Transition pointwise failure count is inconsistent")
        if (type(worst.get("bin")) is not int or not 0 <= worst["bin"] < row["bins"] or
                worst.get("channel") != 0 or worst.get("endpoint") != (row["endpoint"] if pointwise else 0)):
            raise ValueError("Transition pointwise identity is inconsistent")
    return trace


def transition_tables(trace, config, raw_path, registry=None):
    """Return per-process response, error and event-adjacent callback cost rows.

    Window: one baseline hop before request through two maximum hops after it,
    clipped to the observation horizon; callbacks selected by overlapping sample
    intervals. Windows overlap intentionally. These are observed costs, never
    worst-case execution-time bounds or device/UI response measurements.
    """
    validate_transition_trace(trace, config, registry)
    with raw_path.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    callbacks = [r for r in rows if r["kind"] == "callback"]
    timers = [float(r["ns"]) for r in rows if r["kind"] == "timer"]
    if len(callbacks) != config["callbacks"] or len(timers) != 1024:
        raise ValueError("Missing transition callback/timer observations")
    responses, costs = [], []
    for event in trace["events"]:
        row = {k: event[k] for k in ("generation", "reason", "request_sample", "outcome", "replaced_by")}
        for field, origin in (("application", "application_sample"), ("first_publication", "first_publication_sample")):
            latency = event[origin]-event["request_sample"] if event[origin] >= 0 else None
            row[field+"_latency_samples"] = latency
            row[field+"_latency_ms"] = 1000*latency/config["rate"] if latency is not None else None
        responses.append(row)
        start, end = max(0, event["request_sample"]-config["hop"]), min(trace["horizon"], event["request_sample"]+4*config["hop"])
        selected = [r for r in callbacks if int(r["sample"]) < end and int(r["sample"])+int(r["samples"]) > start]
        values = sorted(float(r["ns"]) for r in selected)
        if not values or any(not math.isfinite(v) or v < 0 for v in values+timers):
            raise ValueError("Invalid transition cost window")
        costs.append(dict(generation=event["generation"], window_start=start, window_end=end,
                          callback_indices=[int(r["index"]) for r in selected], observations=len(values),
                          mean_ns=statistics.mean(values), p99_ns=values[math.ceil(.99*len(values))-1],
                          observed_max_ns=max(values), timer_p99_ns=sorted(timers)[math.ceil(.99*len(timers))-1]))
    return dict(responses=responses, costs=costs, publications=trace["publications"])


def validate_transition_resources(resource_part, config, registry=None):
    """Validate exact transition engine policy and both actual native plans.

    Accept an allocation-audit resource record or its provider_info payload.
    Native allocator bytes remain unknown; C++ execution allocations are still
    checked by the ordinary resource checker and no stronger claim is made.
    """
    validate_transition_config(config, registry)
    info = resource_part.get("provider_info", resource_part)
    core = config["backend"].startswith("core-")
    memory_policy = ("production SpectrumAnalysis.configure; preallocated maximum capacities" if core else
                     "benchmark native control; two exact-size plans and maximum buffers prepared before timing; no transition allocation")
    engine_policy = ("production-scalar" if core else "prepared-native-immediate"
                     if config["backend"] == "pffft-scheduled-batch-float" else "prepared-native-balanced")
    lengths = [config["n"], min(16384, 8*config["n"])]
    expected = dict(transition_policy=SUITE, engine_policy=engine_policy, memory_policy=memory_policy,
                    prepared_lengths=lengths, maximum_hop=2*config["hop"])
    if not isinstance(info, dict) or any(info.get(k) != v for k, v in expected.items()):
        raise ValueError("Missing or incorrect transition engine/memory policy")
    instances = info.get("provider_instances")
    if not isinstance(instances, list) or len(instances) != (0 if core else 2):
        raise ValueError("Missing transition native plan bank")
    for native, n in zip(instances, lengths):
        expected_native = dict(provider="rack-pffft", api="rack::dsp::RealFFT::rfft",
                               plan_policy="one exact-size reusable ordered plan", precision="float",
                               native_layout="DC,Nyquist,Re1,Im1,...", inverse_normalization="adapter 1/N",
                               aligned_io_payload_bytes=8*n, native_plan_bytes=None,
                               native_heap_scratch_bytes=0, source_derived_twiddle_payload_bytes=4*n,
                               source_derived_stack_scratch_payload_bytes=4*n)
        if any(native.get(k) != v for k, v in expected_native.items()):
            raise ValueError("Wrong transition native provider or prepared length")
        if (type(native.get("simd_lanes")) is not int or native["simd_lanes"] < 1 or
                not native.get("unknown_reason") or not native.get("native_execution_allocation_policy")):
            raise ValueError("Missing transition native allocation limitations")
    return info

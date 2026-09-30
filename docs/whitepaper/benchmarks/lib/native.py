"""Contracts for new native pipelines; archived scalar-glue backends stay unchanged."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import math


def exact(actual, expected):
    """Do not let JSON booleans or floating counts masquerade as integers."""
    if type(actual) is not type(expected): return False
    if isinstance(expected, dict):
        return actual.keys() == expected.keys() and all(exact(actual[k], v) for k, v in expected.items())
    if isinstance(expected, list):
        return len(actual) == len(expected) and all(exact(a, b) for a, b in zip(actual, expected))
    return actual == expected


def validate_info(info, descriptor, config):
    n, channels = config["n"], descriptor["channels"]
    expected = dict(analysis_pipeline="native-segments-v1", channels=channels, native_batch_channels=channels,
                    natural_frequency_output=True, channel_endpoints="simultaneous per instance; jH",
                    task_units=channels*n+1+2*channels*(n//2+1), retained_input_samples_per_channel=n+config["hop"],
                    publication_delay_samples=0 if descriptor["schedule"] == "immediate" else config["hop"]-1,
                    initial_cache="dirty; rebuild in first frame")
    if any(not exact(info.get(k), v) for k, v in expected.items()) or not info.get("layout_policy") or not info.get("limitations"):
        raise ValueError("Native layout/schedule/storage contract mismatch")
    if descriptor["provider"] == "pffft":
        layout = "unordered then pffft_zreorder" if "unordered" in descriptor["id"] else "ordered packed"
        if info["layout_policy"] != layout:
            raise ValueError("Missing charged PFFFT reorder policy")


def validate_audit(accuracy, config, contract):
    audit = accuracy.get("native_audit", {})
    if audit.get("policy") != "native-all-channels-v1" or len(audit.get("instances", [])) != config["count"]:
        raise ValueError("Missing native per-channel endpoint audit")
    n, hop, total = config["n"], config["hop"], config["callbacks"]*config["block"]
    warm = 0 if config["state"] == "startup" else ((n+hop-1)//hop+config["warm_hops"])*hop
    delay = contract["publication_delay_samples"]
    for index, item in enumerate(audit["instances"]):
        offset = config.get("callback_offset", 0)+(index*hop//config["count"] if config["alignment"] == "staggered" else 0)
        start = warm+offset
        first = start+(delay-start)%hop
        count = max(0, (start+total-1-first)//hop+1)
        expected = dict(instance=index, first_sample=start, samples=total, publications=count,
                        channels=[dict(channel=c, first_endpoint=first-delay if count else 0,
                                       last_endpoint=first-delay+(count-1)*hop if count else 0,
                                       spectra=count, bins=count*(n//2+1)) for c in range(contract["channels"])])
        if not exact(item, expected):
            raise ValueError("Native channel endpoints/coverage mismatch")


def validate_diagnostic(data, registry=None, allow_fixture=False):
    """Structural/numerical check only: diagnostics never qualify as campaign data."""
    from contracts import validate_config, resolve_contract
    from check import validate_provider_info, validate_analysis_accuracy

    if (data.get("schema") != 1 or data.get("kind") != "native-stage-diagnostic-v1"
            or type(data.get("fixture_clock")) is not bool or (data["fixture_clock"] and not allow_fixture)
            or data.get("mode") not in ("trace", "stages", "overhead")):
        raise ValueError("Invalid diagnostic identity or synthetic timing")
    c = data["config"]; descriptor = validate_config(c, registry, measurement=True)
    contract = resolve_contract(c, registry)
    if (descriptor["kind"] != "native-analysis" or data["contract"] != contract
            or c["count"] != 1 or c["alignment"] != "aligned" or c["callback_offset"]
            or c["load"] or c["cache_mib"] or c["pass_name"] != "callback"
            or c.get("execution_regime", "continuous") != "continuous"):
        raise ValueError("Diagnostic workload mismatch")
    total, hop = c["callbacks"]*c["block"], c["hop"]
    warm = 0 if c["state"] == "startup" else ((c["n"]+hop-1)//hop+c["warm_hops"])*hop
    if (total > 65536 or data["samples"] != total or data["warm_samples"] != warm
            or data["stages"] != ["window-pack", "native-transform-reorder", "magnitude-prefix", "smooth-store"]):
        raise ValueError("Diagnostic dimensions/stages mismatch")
    validate_provider_info(data["provider"], descriptor, c)
    validate_audit({"native_audit": data["coverage"]}, c, contract)
    publications = data["coverage"]["instances"][0]["publications"]
    validate_analysis_accuracy(dict(data, analysis=data["accuracy"]), c, publications, contract, "spectrum-norms-v1")
    if data["checked_samples"] != publications*contract["channels"]*(c["n"]//2+1):
        raise ValueError("Missing diagnostic numerical outputs")
    lengths = [contract["channels"]*c["n"], 1]+[contract["channels"]*(c["n"]//2+1)]*2
    work = sum(lengths); cursor = stage = 0; expected = []
    for sample in range(total):
        phase = sample%hop
        if phase == 0: cursor = stage = 0
        quota = (work if phase == 0 else 0) if descriptor["schedule"] == "immediate" else (
            (phase+1)*work//hop-phase*work//hop)
        while quota:
            count = min(quota, lengths[stage]-cursor)
            expected.append(dict(sample=sample, stage=stage, first=cursor, count=count))
            cursor += count; quota -= count
            if cursor == lengths[stage]: cursor = 0; stage += 1
    if len(data["events"]) != len(expected): raise ValueError("Missing diagnostic stage segments")
    for event, coordinates in zip(data["events"], expected):
        if not exact({k: v for k, v in event.items() if k != "ns"}, coordinates):
            raise ValueError("Diagnostic stage placement/units mismatch")
        ns = event.get("ns")
        if data["mode"] == "trace":
            if ns is not None: raise ValueError("Untimed trace contains timing")
        elif type(ns) not in (int, float) or not math.isfinite(ns) or ns < 0:
            raise ValueError("Invalid stage timing")

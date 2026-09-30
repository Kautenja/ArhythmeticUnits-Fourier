"""Read retained observations once for integrity, summaries and report details."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
from collections import Counter
import csv
import math
import statistics

from contracts import resolve_contract


def _rank(values, fraction):
    return values[max(0, math.ceil(len(values)*fraction)-1)]


def quantile(values, fraction):
    """Nearest-rank quantile; retain raw rows rather than treating it as a bound."""
    return _rank(sorted(values), fraction)


def _read(path, config, registry=None, report=False, validate=True):
    groups, counts, publications, previous = {}, Counter(), Counter(), {}
    visible_range = playback_range = None
    transition_rows = None
    if validate and config.get("transition_suite"):
        from transitions import expected_publications
        transition_rows = expected_publications(config, registry)
    mode = config.get("pass_name")
    details_requested = report and mode in ("callback", "throughput", "complete", "incremental")
    if validate:
        contract = resolve_contract(config, registry)
        n, hop, block = config["n"], config["hop"], config["block"]
        delay = contract["publication_delay_samples"]
        center_offset = contract["center_offset_samples"]
        playback_delay = contract["playback_delay_samples"]
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream):
            kind = row["kind"]
            counts[kind] += 1
            # Historical summary-only callers ignored publication timing cells.
            if validate or kind != "publication":
                duration = float(row["ns"])
                if not math.isfinite(duration) or duration < 0:
                    raise ValueError("Invalid timing observation" if validate
                                     else f"Invalid measurements in {path}")
            if kind != "publication":
                if validate and transition_rows is not None and kind == "callback":
                    index = counts[kind]-1
                    expected_callback = dict(index=index, sample=index*config["block"],
                                             samples=config["block"], analyzer=0)
                    if any(int(row[key]) != value for key, value in expected_callback.items()):
                        raise ValueError("Transition callback coordinates mismatch")
                groups.setdefault(kind, []).append(duration)
                continue
            if validate:
                analyzer, sample = int(row["analyzer"]), int(row["sample"])
                offset = config.get("callback_offset", 0) + (analyzer*hop//config["count"] if config["alignment"] == "staggered" else 0)
                if not 0 <= analyzer < config["count"] or not 0 <= sample < config["callbacks"]*block:
                    raise ValueError("Publication index outside workload")
                if transition_rows is not None:
                    index = counts["publication"]-1
                    if index >= len(transition_rows):
                        raise ValueError("Extra transition publication")
                    expected_row = transition_rows[index]
                    if analyzer != 0 or sample != expected_row["publication_sample"]:
                        raise ValueError("Transition publication cadence mismatch")
                    delay = sample-expected_row["endpoint"]
                    center_offset = (expected_row["n"]-1)/2
                elif (sample+offset)%hop != delay or (analyzer in previous and sample-previous[analyzer] != hop):
                    raise ValueError("Publication cadence mismatch")
                ages = (float(row["endpoint_age_samples"]), float(row["center_age_samples"]),
                        float(row["callback_visible_age_samples"]))
                if ages != (delay, delay+center_offset, delay+block-1-sample%block):
                    raise ValueError("Publication age mismatch")
                if contract["boundary"] in ("inverse-job", "chain") and float(row.get("playback_delay_samples", "nan")) != playback_delay:
                    raise ValueError("Playback delay mismatch")
                previous[analyzer] = sample
                publications[analyzer] += 1
            if details_requested:
                visible = ages[2] if validate else float(row["callback_visible_age_samples"])
                playback = float(row["playback_delay_samples"])
                visible_range = ([visible, visible] if visible_range is None
                                 else [min(visible_range[0], visible), max(visible_range[1], visible)])
                playback_range = ([playback, playback] if playback_range is None
                                  else [min(playback_range[0], playback), max(playback_range[1], playback)])
    if validate:
        expected = Counter(timer=1024)
        frames = config["callbacks"]
        if mode in ("callback", "throughput"):
            expected[mode] = frames if mode == "callback" else 1
            if transition_rows is not None:
                expected["publication"] = len(transition_rows)
            elif contract["boundary"] != "control":
                for analyzer in range(config["count"]):
                    offset = config.get("callback_offset", 0) + (analyzer*hop//config["count"] if config["alignment"] == "staggered" else 0)
                    bias = hop-1-delay
                    count = (frames*block+offset+bias)//hop-(offset+bias)//hop
                    if publications[analyzer] != count:
                        raise ValueError("Missing publication records")
                    expected["publication"] += count
        elif mode in ("complete", "incremental"):
            expected[mode] = frames
        else:
            real, inverse = contract["step_model"] == "radix2-real", contract["step_model"] == "radix2-inverse"
            butterflies = n//4*((n//2).bit_length()-1) if real else n//2*(n.bit_length()-1)
            expected["buffer"] = frames
            expected["butterfly_step" if mode == "steps" else "butterflies"] = frames*(butterflies-int(real) if mode == "steps" else 1)
            if real:
                expected["reconstruct_step" if mode == "steps" else "last_butterfly_and_reconstruction"] = frames
            if inverse:
                expected["normalize_step" if mode == "steps" else "normalization"] = frames*(n if mode == "steps" else 1)
        if counts != expected:
            raise ValueError(f"Observation count mismatch: {counts} != {expected}")
    result = {"publication_audit_rows": counts["publication"], "groups": {}}
    for kind, values in groups.items():
        # Preserve historical floating-point accumulation order before sorting.
        total, mean, maximum = sum(values), statistics.mean(values), max(values)
        values.sort()
        summary = dict(observations=len(values), total_ns=total, mean_ns=mean,
                       p50_ns=_rank(values, .5), p95_ns=_rank(values, .95),
                       p99_ns=_rank(values, .99), observed_max_ns=maximum)
        if kind in ("callback", "throughput"):
            samples = config["callbacks"]*config["block"]
            budget_ns = 1e9*config["block"]/config["rate"]
            summary.update(ns_per_engine_sample=total/samples,
                           simulated_compute_utilization=total/(1e9*samples/config["rate"]))
            if kind == "callback":
                summary.update(budget_ns=budget_ns,
                               observed_compute_budget_exceedances=sum(v > budget_ns for v in values))
        result["groups"][kind] = summary
    if "timer" not in groups or len(groups) < 2:
        raise ValueError(f"Missing timing rows in {path}")
    details = None
    if details_requested:
        values = groups[mode]
        # Retain endpoints and the existing deterministic ECDF grid per process.
        indices = sorted({i*(len(values)-1)//min(4095, max(1, len(values)-1))
                          for i in range(min(4096, len(values)))})
        details = dict(observation_count=len(values),
                       ecdf=[[values[i], (i+1)/len(values)] for i in indices],
                       callback_visible_age_range=visible_range,
                       playback_delay_range=playback_range)
    return result, details


def read_observations(path, config, registry=None, report=False):
    """Validate raw counts/cadence/ages and return summary plus optional details."""
    return _read(path, config, registry, report)


def summarize(path, config):
    """Preserve summary-only callers without imposing current registry contracts."""
    return _read(path, config, validate=False)[0]


def validate_rows(path, config, registry=None):
    """Check observation counts and publication age/cadence from raw records."""
    read_observations(path, config, registry)

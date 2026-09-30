"""Coordinate-preserving hop peaks and explicit aggregate-block budget scenarios."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
from collections import defaultdict
import csv
import math
import statistics

POLICY = "hop-peaks-v1"
LIMITS = ("Hop peaks reuse whole callbacks that intersect each analyzer's frame interval. "
          "Shared callbacks and partial edge hops are retained, never split into invented costs. "
          "Analyzer-indexed groups still measure aggregate block work; no per-analyzer attribution. "
          "Callbacks, hops, ports and channels are dependent observations, not process/session replicates. "
          "Budget fractions are allocation scenarios, not device deadlines. Consumer/device metrics are unavailable. "
          "No confidence interval; descriptive within-process summaries and same-host session ranges only.")


def distribution(values):
    if not values:
        return None
    ordered = sorted(values)
    mean = statistics.mean(values)
    return dict(count=len(values), mean_ns=mean, median_ns=statistics.median(values),
                p99_ns=ordered[max(0, math.ceil(.99*len(values))-1)], maximum_ns=max(values),
                peak_over_mean=max(values)/mean if mean else None)


def derive(rows, config, contract, execution=None):
    """Input rows retain original sample coordinates; no time is apportioned."""
    mode, hop, block = config["pass_name"], config["hop"], config["block"]
    selected = [r for r in rows if r["kind"] == mode]
    values = []
    for index, row in enumerate(selected):
        duration = float(row["ns"])
        length = block if mode == "callback" else config["callbacks"]*block
        start = index*block if mode == "callback" else 0
        if (not math.isfinite(duration) or duration < 0 or
                any(int(row.get(k, -1)) != v for k, v in dict(index=index, analyzer=0, sample=start, samples=length).items())):
            raise ValueError("Malformed scheduling observation/timestamp")
        values.append(duration)
    if len(values) != (config["callbacks"] if mode == "callback" else 1):
        raise ValueError("Missing scheduling observations")
    total = config["callbacks"]*block
    result = dict(policy=POLICY, scope="aggregate-block", boundary=contract["boundary"], limitations=LIMITS,
                  samples=total, simulated_duration_ns=1e9*total/config["rate"],
                  timed_compute_ns=sum(values), elapsed_measurement_ns=None,
                  compute=distribution(values), publication_rows=sum(r["kind"] == "publication" for r in rows),
                  consumer=None, device=None, analyzer_attributed_compute=None,
                  budgets=[], analyzers=[], paced=None, throughput_chunks=None)
    if execution is not None:
        from execution import validate_process
        from execution import POLICY as EXECUTION_POLICY
        policy = dict(EXECUTION_POLICY, **{k: execution[k] for k in
            ("regime", "process_settle_ms", "throughput_chunks", "thread_policy", "fpu_policy")})
        validate_process(execution, policy, config, dict(groups={mode: dict(total_ns=sum(values))}))
        intervals = execution["observations"]
        result["elapsed_measurement_ns"] = intervals[-1]["finish_ns"]-intervals[0]["wake_ns"]
        if mode == "throughput":
            result["throughput_chunks"] = dict(intervals=intervals,
                compute=distribution([r["finish_ns"]-r["start_ns"] for r in intervals]))
        elif execution["regime"] == "paced":
            result["paced"] = dict(scope="synthetic-release-schedule; not device deadlines",
                wake_lateness=distribution([r["wake_ns"]-r["release_ns"] for r in intervals]),
                conditioning=distribution([r["start_ns"]-r["wake_ns"] for r in intervals]),
                release_to_finish=distribution([r["finish_ns"]-r["release_ns"] for r in intervals]),
                release_deadline_exceedances=sum(r["finish_ns"] > r["deadline_ns"] for r in intervals),
                intervals=len(intervals))
    if mode != "callback" or config.get("transition_suite") or contract["boundary"] == "control":
        return result
    budget = 1e9*block/config["rate"]
    result["budgets"] = [dict(scope="aggregate compute allocation scenario", fraction=f,
        threshold_ns=budget*f, exceedances=sum(v > budget*f for v in values), observations=len(values))
        for f in (.25, .5, 1.)]
    # Endpoint coordinates come from actual replay where present. Missing edge
    # publications use the declared stationary lattice, explicitly labeled.
    endpoints = defaultdict(set)
    for row in rows:
        if row["kind"] != "publication":
            continue
        endpoint = float(row["sample"])-float(row["endpoint_age_samples"])
        analyzer = int(row["analyzer"])
        if not math.isfinite(endpoint) or endpoint != int(endpoint) or not 0 <= analyzer < config["count"]:
            raise ValueError("Malformed publication endpoint")
        endpoints[analyzer].add(int(endpoint))
    for analyzer in range(config["count"]):
        offset = config.get("callback_offset", 0)+(analyzer*hop//config["count"] if config["alignment"] == "staggered" else 0)
        if any((endpoint+offset)%hop for endpoint in endpoints[analyzer]):
            raise ValueError("Publication endpoint disagrees with stationary frame lattice")
        groups, phases = {}, defaultdict(list)
        memberships = defaultdict(int)
        for index, value in enumerate(values):
            start, end = index*block, (index+1)*block
            phases[(start+offset)%hop].append(value)
            endpoint = (start+offset)//hop*hop-offset
            while endpoint < end:
                group = groups.setdefault(endpoint, dict(endpoint_sample=endpoint, end_sample=endpoint+hop,
                    endpoint_source="publication-replay" if endpoint in endpoints[analyzer] else "contract-edge",
                    partial=endpoint < 0 or endpoint+hop > total, callbacks=[], maximum_ns=0))
                group["callbacks"].append(index)
                group["maximum_ns"] = max(group["maximum_ns"], value)
                memberships[index] += 1
                endpoint += hop
        ordered = [groups[e] for e in sorted(groups)]
        for group in ordered:
            group["shared_callbacks"] = [i for i in group["callbacks"] if memberships[i] > 1]
        phase_rows = [dict(start_phase_samples=phase, **distribution(v)) for phase, v in sorted(phases.items())]
        phase_means = [v["mean_ns"] for v in phase_rows]
        result["analyzers"].append(dict(analyzer=analyzer, offset_samples=offset,
            scope="aggregate callbacks intersecting this analyzer's hops", hops=ordered,
            complete_hop_peaks=distribution([r["maximum_ns"] for r in ordered if not r["partial"]]),
            all_hop_peaks=distribution([r["maximum_ns"] for r in ordered]),
            shared_callback_count=sum(n > 1 for n in memberships.values()),
            phase_profile=phase_rows,
            phase_mean_max_over_min=max(phase_means)/min(phase_means) if min(phase_means) > 0 else None))
    return result


def read(path, config, contract, execution=None):
    with path.open(newline="") as stream:
        return derive(list(csv.DictReader(stream)), config, contract, execution)


def tables(data, output):
    """New tables only when explicit workload contracts request these metrics."""
    records = [r for r in data["records"] if any(p.get("scheduling") for p in r["processes"])]
    if not records:
        return []
    from report import identity
    rows = {name: [] for name in ("processes", "analyzers", "hops", "phases", "budgets", "sessions")}
    for record in records:
        sessions = defaultdict(list)
        for process in record["processes"]:
            metric = process.get("scheduling")
            if not metric:
                raise ValueError("Mixed scheduling metric contracts")
            prefix = dict(config_sha256=identity(record["config"]), stratum=record["stratum"],
                backend=record["config"]["backend"], boundary=metric["boundary"],
                session=process["session"], repeat=process["repeat"])
            compute = metric["compute"]
            sessions[process["session"]].append(process["cost"])
            rows["processes"].append(dict(prefix, scope=metric["scope"], samples=metric["samples"],
                observations=compute["count"], mean_ns=compute["mean_ns"], median_ns=compute["median_ns"],
                p99_ns=compute["p99_ns"], peak_over_mean=compute["peak_over_mean"], maximum_ns=compute["maximum_ns"],
                timed_compute_ns=metric["timed_compute_ns"], elapsed_measurement_ns=metric["elapsed_measurement_ns"],
                simulated_duration_ns=metric["simulated_duration_ns"],
                release_deadline_exceedances=(metric["paced"] or {}).get("release_deadline_exceedances")))
            for scenario in metric["budgets"]:
                rows["budgets"].append(dict(prefix, **scenario))
            for analyzer in metric["analyzers"]:
                peaks = analyzer["complete_hop_peaks"] or dict.fromkeys(
                    ("count", "mean_ns", "median_ns", "p99_ns", "maximum_ns", "peak_over_mean"))
                rows["analyzers"].append(dict(prefix, analyzer=analyzer["analyzer"], scope=analyzer["scope"],
                    offset_samples=analyzer["offset_samples"], partial_hops=sum(h["partial"] for h in analyzer["hops"]),
                    shared_callback_count=analyzer["shared_callback_count"],
                    phase_mean_max_over_min=analyzer["phase_mean_max_over_min"],
                    **{"complete_hop_peak_"+k: v for k, v in peaks.items()}))
                for hop in analyzer["hops"]:
                    rows["hops"].append(dict(prefix, analyzer=analyzer["analyzer"], **dict(hop,
                        callbacks=",".join(map(str, hop["callbacks"])), shared_callbacks=",".join(map(str, hop["shared_callbacks"])))))
                for phase in analyzer["phase_profile"]:
                    rows["phases"].append(dict(prefix, analyzer=analyzer["analyzer"], **phase))
        for session, costs in sorted(sessions.items()):
            rows["sessions"].append(dict(config_sha256=identity(record["config"]), stratum=record["stratum"],
                backend=record["config"]["backend"], boundary=record["contract"]["boundary"], session=session,
                processes=len(costs), mean_cost=statistics.mean(costs), median_cost=statistics.median(costs),
                minimum_cost=min(costs), maximum_cost=max(costs), cost_unit=record["processes"][0]["cost_unit"]))
    names = []
    for name, items in rows.items():
        path = output/("scheduling-"+name+".csv")
        with path.open("w", newline="") as stream:
            columns = list(items[0]) if items else ["config_sha256", "stratum", "backend", "boundary", "session", "repeat"]
            writer = csv.DictWriter(stream, columns); writer.writeheader(); writer.writerows(items)
        names.append(path.name)
    (output/"scheduling.md").write_text("# Scheduling Metrics\n\n"+LIMITS+"\n\n"
        "Policy: hop-peaks-v1. Primary descriptive outcomes are complete-hop peak distributions "
        "per analyzer/process in scheduling-analyzers.csv; p99 and maxima remain secondary. scheduling-hops.csv retains all "
        "coordinates, callback membership and partial edges. scheduling-phases.csv groups whole "
        "callbacks by their start phase, without attributing costs to sub-callback stages. "
        "scheduling-sessions.csv keeps process means and medians within each session. "
        "Publication age comes from untimed replay, not display observation.\n")
    return names

#!/usr/bin/env python3
"""Deterministic evidence tables and scientific figures from checked campaigns."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
from collections import defaultdict
import csv
import hashlib
import json
import math
import os
import platform
from pathlib import Path
import statistics

from check import check
from run import digest
from numerical import coverage_tables
from transitions import transition_tables

LIMITS = ("Costs retain traversal, conversion, required stores and configured background load. "
          "No timer subtraction. Callback quantiles and maxima describe each process, not WCET or device underruns. "
          "Ranges across session means describe observed variation, not confidence intervals. "
          "Session labels are supplied by the operator; labels alone do not prove independence. "
          "C++ live heap includes the object; native allocation and stack may remain unknown. "
          "Different arithmetic, plans, storage and publication ages remain explicit confounds. "
          "Normwise analysis acceptance does not guarantee every weak bin; legacy pointwise violations remain diagnostic. "
          "Independent four-channel banks are compared only with the same input contract. "
          "Double uses common float input/window bytes. Unmeasured overlap-reuse methods are outside the ranking.")


def identity(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def variation(processes):
    sessions = defaultdict(list)
    for row in processes:
        sessions[row["session"]].append(row["cost"])
    means = [statistics.mean(values) for _, values in sorted(sessions.items())]
    return dict(processes=len(processes), sessions=len(means),
                session_means={key: statistics.mean(values) for key, values in sorted(sessions.items())},
                mean_of_session_means=statistics.mean(means), observed_session_min=min(means),
                observed_session_max=max(means), confidence_interval=None,
                uncertainty="observed session range; no inferential interval")


def callback_tails(processes):
    """Summarize process quantiles without pooling callbacks or weighting busy sessions."""
    sessions = defaultdict(list)
    for row in processes:
        sessions[row["session"]].append(row["timing"]["p99_ns"])
    medians = {key: statistics.median(values) for key, values in sorted(sessions.items())}
    return dict(session_median_p99_ns=medians, median_session_p99_ns=statistics.median(medians.values()),
                observed_session_p99_min_ns=min(medians.values()),
                observed_session_p99_max_ns=max(medians.values()),
                observed_max_ns=max(p["timing"]["observed_max_ns"] for p in processes),
                uncertainty="observed range of session medians of process p99; not a confidence interval")


def ages(contract, rate):
    """Keep distinct time origins; a buffered transform has no streaming age."""
    result = {name+"_ms": (contract[name+"_samples"]*1000/rate
                          if contract[name+"_samples"] is not None and contract[name+"_samples"] >= 0 else None)
              for name in ("publication_delay", "center_offset", "playback_delay")}
    result["spectrum_center_age_ms"] = (
        result["publication_delay_ms"]+result["center_offset_ms"]
        if contract["boundary"] == "analysis" and result["publication_delay_ms"] is not None
        and result["center_offset_ms"] is not None else None)
    return result


def collect(directories, phase):
    records, sources, seen = {}, [], set()
    for directory in sorted(directories, key=lambda p: str(p.resolve())):
        report_data = {}
        check(directory, report_data=report_data)
        metadata = json.loads((directory/"metadata.json").read_text())
        if metadata.get("phase") != phase:
            raise ValueError("Report phase must match every campaign; smoke/pilot/confirmation cannot be mixed")
        host, session = metadata.get("host_id"), metadata.get("session_id")
        if not host or not session:
            raise ValueError("Missing host/session identity")
        provenance = {key: metadata.get(key) for key in (
            "source_sha256", "sdk_sha256", "external_dependency_sha256", "compiler", "build_command",
            "build_features", "compile_commands", "platform", "machine", "cpu_model")}
        if "execution_policy" in metadata:
            provenance["execution_policy"] = metadata["execution_policy"]
        # Different hosts, source/dependency bytes or build policies cannot be pooled.
        stratum = identity(dict(host=host, provenance=provenance))[:16]
        sources.append(dict(directory=str(directory.resolve()), metadata_sha256=digest(directory/"metadata.json"),
                            stratum=stratum, host=host, session=session, provenance=provenance,
                            revision=metadata["revision"], binary_sha256=metadata["binary_sha256"],
                            notes=metadata.get("notes", ""), phase=phase, runtime=metadata.get("runtime")))
        inventory = json.loads((directory/"inventory.json").read_text())
        for index, config in enumerate(metadata["configs"]):
            descriptor = inventory[config["backend"]]
            config_key = identity(config)
            unique = (stratum, session, config_key)
            if unique in seen:
                raise ValueError("Duplicate workload within a host/session; do not relabel process repeats as sessions")
            seen.add(unique)
            key = (stratum, config_key)
            resource = json.loads((directory/metadata["resources"][str(index)]).read_text())
            contract = metadata["contracts"][str(index)]
            record = records.setdefault(key, dict(stratum=stratum, host=host, config=config,
                descriptor=descriptor, contract=contract, processes=[], resources=[],
                independent_channels=config["count"]*(config["active_ports"] if config.get("workload_schema") == 3 else contract["channels"]),
                input_contract=contract.get("input_contract", "independent-four-v1" if descriptor["kind"] == "analysis4" else "common-float-v1")))
            record["resources"].append(dict(session=session, measurements=resource))
            for run in sorted((r for r in metadata["runs"] if r["workload"] == index), key=lambda r: r["repeat"]):
                groups = run["summary"]["groups"]
                mode = config["pass_name"]
                if mode not in ("callback", "throughput", "complete", "incremental"):
                    raise ValueError("This matched report expects streaming or complete transform passes")
                timing = groups[mode]
                cost = timing["ns_per_engine_sample"] if mode in ("callback", "throughput") else timing["mean_ns"]
                errors = (directory/run["stderr"]).read_text().strip()
                accuracy = json.loads(errors) if errors.startswith("{") else None
                transition = None
                if config.get("transition_suite"):
                    trace_path = directory/run["transition"]
                    trace = json.loads(trace_path.read_text())
                    transition = dict(trace=trace, path=str(trace_path.resolve()),
                        sha256=metadata["artifact_sha256"][run["transition"]],
                        tables=transition_tables(trace, config, directory/run["raw"], inventory))
                runtime = None
                if "runtime" in run:
                    runtime = dict(profile=json.loads((directory/run["runtime"]).read_text()),
                        path=str((directory/run["runtime"]).resolve()),
                        sha256=metadata["artifact_sha256"][run["runtime"]])
                record["processes"].append(dict(session=session, repeat=run["repeat"], cost=cost,
                    cost_unit="ns/engine-sample" if mode in ("callback", "throughput") else "ns/transform",
                    cost_per_channel=cost/record["independent_channels"], timing=timing, timer=groups["timer"],
                    observation_window_samples=config["callbacks"]*config["block"]
                        if mode in ("callback", "throughput") else None,
                    publication_audit_rows=run["summary"]["publication_audit_rows"],
                    transition=transition, runtime=runtime, accuracy=accuracy,
                    numerical_status=("all-publication transition replay; spectrum-norms-v1" if transition
                        else "per-run numerical report; see reference coverage" if accuracy
                        else "preflight only; no per-run numerical report"),
                    raw=str((directory/run["raw"]).resolve()),
                    raw_sha256=metadata["artifact_sha256"][run["raw"]], **report_data.pop(run["raw"])))
    result = []
    for key, record in sorted(records.items()):
        record["variation"] = variation(record["processes"])
        record["tails"] = callback_tails(record["processes"]) if record["config"]["pass_name"] == "callback" else None
        if phase == "confirmation" and record["variation"]["sessions"] < 3:
            raise ValueError("Confirmation reporting requires at least three labeled sessions per workload/stratum")
        result.append(record)
    if not result:
        raise ValueError("No checked observations")
    return dict(schema=2 if any(r["config"].get("workload_schema") == 3 for r in result) else 1,
                phase=phase, limitations=LIMITS, sources=sources, records=result)


def comparison_key(record, vary_length=False):
    config = {k: v for k, v in record["config"].items() if k != "backend" and (not vary_length or k != "n")}
    contract = record["contract"]
    return identity(dict(stratum=record["stratum"], config=config, boundary=contract["boundary"],
                         operation=contract["operation"], precision=contract["precision"],
                         channels=record["independent_channels"], input=record["input_contract"]))[:16]


def stationary_records(data):
    """A changing length/hop has no single valid stationary publication age."""
    return [record for record in data["records"] if not record["config"].get("transition_suite")]


def tables(data, output):
    columns = ["stratum", "backend", "boundary", "operation", "precision", "n", "hop", "block", "channels",
               "mode", "state", "callback_offset", "cost_unit", "mean_session_cost", "session_min", "session_max",
               "sessions", "processes", "publication_delay_samples", "center_offset_samples", "playback_delay_samples",
               "cpp_live_heap_bytes_min", "cpp_live_heap_bytes_max", "native_memory", "max_abs_error", "max_relative_l2", "max_relative_linf", "legacy_pointwise_failures", "numerical_status", "callback_visible_age_min", "callback_visible_age_max"]
    workload_columns = ["config_sha256", "host", "rate", "count", "alignment", "load", "smooth", "voices",
                        "cache_mib", "warm_hops", "callbacks"]
    if data.get("schema") == 2:
        from workloads import DEFAULTS
        workload_columns += list(DEFAULTS)
    human_columns = ["mean_serial_audio_time_percent", "callback_budget_us", "median_session_p99_us",
                     "session_p99_min_us", "session_p99_max_us", "observed_callback_max_us", "p99_budget_percent",
                     "process_observations_min", "process_observations_max", "observation_window_seconds",
                     "publication_delay_ms", "center_offset_ms", "playback_delay_ms", "spectrum_center_age_ms"]
    columns += workload_columns+human_columns
    lines = ["# External FFT Evidence Report", "", "Evidence phase: **"+data["phase"].upper()+"**.", "",
             "SMOKE data validates tooling only and must not enter manuscript results." if data["phase"] == "smoke"
             else "Pilot informs design; only frozen confirmation campaigns support final results.", "", LIMITS, "",
             "See results.csv for every workload setting and results in raw and human units; process-timings.csv exposes each process's quantiles, timer control, observation count and window. evidence.json retains native plans, raw-data hashes and source identity.", "",
             "Cost is the mean of session means. P99 is the median of session medians of process p99 values; brackets give the observed session-median range. Maxima are observed, not bounds. No callbacks are pooled. Blank CSV fields mean not applicable or unavailable.", "",
             "Serial audio-time % expresses measured synchronous work relative to simulated audio time; it is not a Rack CPU meter or an energy measurement. Budget % uses the entire callback interval, of which an analyzer receives only a share. Ages end at algorithmic publication/delivery, not screen repaint.", "",
             "Transition workloads have separate response, callback-cost and publication-error tables in transitions.md and transitions-*.csv. Their changing configurations are excluded from stationary results.csv and age figures, but retained in process-timings.csv, implementations.csv and evidence.json. accuracy-coverage.csv records stationary numerical coverage; transition coverage is publication-specific.", "",
             "Observation windows are simulated audio spans, not elapsed wall time: the driver has no real-time pacing. timed_total_ns sums measured intervals only. Publication audit counts come from a separate untimed replay; they are not hardware or timed-burst counters.", ""]
    if data.get("schema") == 2:
        lines[-2] = ("v3 execution sidecars distinguish continuous and paced intervals. Simulated audio duration, "
                     "summed compute duration and elapsed measurement span are separate. See scheduling.md for "
                     "hop peaks, shared callback membership, phase profiles and allocation scenarios.")
    with (output/"results.csv").open("w", newline="") as stream:
        writer = csv.writer(stream); writer.writerow(columns)
        for r in stationary_records(data):
            c, contract, v = r["config"], r["contract"], r["variation"]
            heaps = [x["measurements"]["allocation"]["setup"]["live_bytes"] for x in r["resources"]]
            errors = [p["accuracy"]["max_abs_error"] for p in r["processes"] if p["accuracy"]]
            norms = [p["accuracy"]["analysis"] for p in r["processes"] if p["accuracy"] and "analysis" in p["accuracy"]]
            status = sorted({p["numerical_status"] for p in r["processes"]})
            visible = [age for p in r["processes"] for age in (p["callback_visible_age_range"] or [])]
            unit = r["processes"][0]["cost_unit"]
            streaming = c["pass_name"] in ("callback", "throughput")
            tails = r["tails"]
            budget_us = 1e6*c["block"]/c["rate"] if tails else None
            age = ages(contract, c["rate"])
            extra = [identity(c), r["host"]]+[c.get(key) for key in workload_columns[2:]]
            human = [v["mean_of_session_means"]*c["rate"]/1e7 if streaming else None, budget_us,
                     tails["median_session_p99_ns"]/1000 if tails else None,
                     tails["observed_session_p99_min_ns"]/1000 if tails else None,
                     tails["observed_session_p99_max_ns"]/1000 if tails else None,
                     tails["observed_max_ns"]/1000 if tails else None,
                     tails["median_session_p99_ns"]/10/budget_us if tails else None,
                     min(p["observation_count"] for p in r["processes"]),
                     max(p["observation_count"] for p in r["processes"]),
                     c["callbacks"]*c["block"]/c["rate"] if streaming else None,
                     *age.values()]
            writer.writerow([r["stratum"], c["backend"], contract["boundary"], contract["operation"], contract["precision"],
                c["n"], c["hop"], c["block"], r["independent_channels"], c["pass_name"], c["state"], c.get("callback_offset", 0),
                unit, v["mean_of_session_means"], v["observed_session_min"], v["observed_session_max"], v["sessions"], v["processes"],
                contract["publication_delay_samples"], contract["center_offset_samples"], contract["playback_delay_samples"],
                min(heaps), max(heaps), "provider-specific or unknown; see evidence.json", max(errors) if errors else "unavailable",
                max(x["max_relative_l2"] for x in norms) if norms else "unavailable",
                max(x["max_relative_linf"] for x in norms) if norms else "unavailable",
                sum(x["legacy_pointwise_failures"] for x in norms) if norms else "unavailable", "; ".join(status),
                min(visible) if visible else "unavailable", max(visible) if visible else "unavailable"]+extra+human)
    # Group only identical contracts/settings; the header names all experimental factors.
    matched = defaultdict(list)
    for record in stationary_records(data):
        matched[comparison_key(record)].append(record)
    def number(value):
        return "N/A" if value is None else f"{value:.3f}"
    for key, members in sorted(matched.items()):
        r = members[0]
        c, contract = r["config"], r["contract"]
        lines += [f"## Matched Group {key}", "",
                  f"{contract['boundary']} / {contract['operation']}; {contract['precision']}; "
                  f"{r['independent_channels']} channels; {r['input_contract']}; host {r['host']}; stratum {r['stratum']}.", "",
                  "Settings: "+", ".join(f"{k}={v}" for k, v in sorted(c.items()) if k != "backend")+".", "",
                  "| Backend | Mean Cost (ns) / Unit | Session Cost Range | P99 [Range] (us) | Max (us) | Spectrum / Publication / Delivery Age (ms) | Sessions / Processes |",
                  "| --- | ---: | --- | --- | ---: | --- | ---: |"]
        for r in sorted(members, key=lambda x: x["config"]["backend"]):
            v, t = r["variation"], r["tails"]
            tail = (f"{t['median_session_p99_ns']/1000:.3f} [{t['observed_session_p99_min_ns']/1000:.3f}, "
                    f"{t['observed_session_p99_max_ns']/1000:.3f}]" if t else "N/A")
            age = ages(r["contract"], r["config"]["rate"])
            age_text = " / ".join(number(age[k]) for k in ("spectrum_center_age_ms", "publication_delay_ms", "playback_delay_ms"))
            lines.append(f"| {r['config']['backend']} | {v['mean_of_session_means']:.3f} / "
                         f"{r['processes'][0]['cost_unit'].split('/', 1)[1]} | "
                         f"{v['observed_session_min']:.3f}–{v['observed_session_max']:.3f} | {tail} | "
                         f"{number(t['observed_max_ns']/1000 if t else None)} | {age_text} | {v['sessions']} / {v['processes']} |")
        lines.append("")
    process_tables(data, output)
    from metrics import tables as scheduling_tables
    scheduling_tables(data, output)
    coverage_tables(dict(data, records=stationary_records(data)), output)
    transition_report_tables(data, output)
    with (output/"implementations.csv").open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(["stratum", "backend", "config_sha256", "provider", "plan_policy", "descriptor",
                         "source_provenance", "resource_measurements", "numerical_evidence"])
        for r in data["records"]:
            writer.writerow([r["stratum"], r["config"]["backend"], identity(r["config"]), r["descriptor"]["provider"],
                r["contract"]["plan"], json.dumps(r["descriptor"], sort_keys=True),
                json.dumps([dict(directory=s["directory"], session=s["session"], revision=s["revision"],
                    metadata_sha256=s["metadata_sha256"], binary_sha256=s["binary_sha256"],
                    provenance_sha256=identity(s["provenance"]))
                    for s in data["sources"] if s["stratum"] == r["stratum"]], sort_keys=True),
                json.dumps(r["resources"], sort_keys=True),
                json.dumps([dict(session=p["session"], repeat=p["repeat"], accuracy=p["accuracy"], status=p["numerical_status"],
                                transition_sha256=p["transition"]["sha256"] if p.get("transition") else None)
                            for p in r["processes"]], sort_keys=True)])
    (output/"report.md").write_text("\n".join(lines)+"\n")


def process_tables(data, output):
    config_keys = sorted({key for r in data["records"] for key in r["config"]})
    columns = ["stratum", "host", "config_sha256", *config_keys, "session", "repeat", "cost", "cost_unit",
               "observations", "observation_window_samples", "observation_window_seconds", "publication_audit_rows",
               "timed_total_ns", "timed_total_us", "p50_ns", "p95_ns", "p99_ns", "observed_max_ns",
               "p50_us", "p95_us", "p99_us", "observed_max_us",
               "callback_budget_us", "observed_compute_budget_exceedances", "compute_budget_exceedance_fraction",
               "timer_p99_us", "timer_max_us", "publication_delay_ms", "center_offset_ms", "playback_delay_ms",
               "spectrum_center_age_ms", "callback_visible_age_min_ms", "callback_visible_age_max_ms", "raw", "raw_sha256"]
    with (output/"process-timings.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=columns)
        writer.writeheader()
        for r in data["records"]:
            c = r["config"]
            for p in r["processes"]:
                t = p["timing"]
                row = dict(c, stratum=r["stratum"], host=r["host"], config_sha256=identity(c),
                           **{k: p[k] for k in ("session", "repeat", "cost", "cost_unit", "observation_window_samples",
                                               "publication_audit_rows", "raw", "raw_sha256")},
                           observations=p["observation_count"],
                           timed_total_ns=t["total_ns"], timed_total_us=t["total_ns"]/1000,
                           observation_window_seconds=p["observation_window_samples"]/c["rate"]
                               if p["observation_window_samples"] is not None else None,
                           timer_p99_us=p["timer"]["p99_ns"]/1000, timer_max_us=p["timer"]["observed_max_ns"]/1000,
                           **({} if c.get("transition_suite") else ages(r["contract"], c["rate"])))
                for name in ("p50", "p95", "p99", "observed_max"):
                    row[name+"_ns"] = t[name+"_ns"]
                    row[name+"_us"] = t[name+"_ns"]/1000
                if c["pass_name"] == "callback":
                    row.update(callback_budget_us=t["budget_ns"]/1000,
                               observed_compute_budget_exceedances=t["observed_compute_budget_exceedances"],
                               compute_budget_exceedance_fraction=t["observed_compute_budget_exceedances"]/p["observation_count"])
                if not c.get("transition_suite") and p["callback_visible_age_range"]:
                    row.update(callback_visible_age_min_ms=p["callback_visible_age_range"][0]*1000/c["rate"],
                               callback_visible_age_max_ms=p["callback_visible_age_range"][1]*1000/c["rate"])
                writer.writerow(row)



def transition_report_tables(data, output):
    """Keep each changing-setting process and every request/publication auditable."""
    common = ["stratum", "host", "config_sha256", "backend", "precision", "session", "repeat",
              "suite", "control", "horizon_samples", "rate", "block", "raw", "raw_sha256",
              "transition_sha256"]
    response_columns = common + ["generation", "reason", "request_sample", "application_sample",
        "first_publication_sample", "outcome", "replaced_by", "requested_settings",
        "application_latency_samples", "application_latency_ms",
        "first_publication_latency_samples", "first_publication_latency_ms"]
    cost_columns = common + ["generation", "reason", "request_count", "applied_generation_count", "window_start", "window_end",
        "callback_indices", "observations", "mean_ns", "p99_ns", "observed_max_ns", "timer_p99_ns",
        "mean_us", "p99_us", "observed_max_us", "timer_p99_us"]
    publication_columns = common + ["instance", "channel", "generation", "endpoint", "publication_sample",
        "history_start", "bins", "settings", "endpoint_age_samples", "endpoint_age_ms",
        "accuracy_policy", "tolerance", "checked_spectra", "checked_bins", "zero_spectra",
        "max_relative_l2", "max_relative_linf", "max_abs_error", "max_reference", "absolute_error_units",
        "legacy_pointwise_failures", "max_legacy_scaled_error", "worst_pointwise"]
    responses, costs, publications = [], [], []
    lines = ["# Parameter-Transition Evidence", "", "Evidence phase: **"+data["phase"].upper()+"**.", "",
        "Each row retains its process and session. No-change controls receive the same request",
        "schedule with initial settings; their control field is true. Results are not pooled.", "",
        "Response latencies use input-sample time, measured from request before processing to",
        "application or the first complete publication for that generation. They are not UI",
        "or wall-clock response. Replaced, pending and applied-without-publication outcomes",
        "remain explicit; empty latency fields mean no response within the recorded horizon.", "",
        "Callback windows run from one initial hop before each request through four initial",
        "hops after it, clipped to the horizon. Every overlapping callback is retained by index;",
        "windows intentionally overlap. Mean, nearest-rank p99 and maxima describe observed",
        "costs, not WCET bounds or device underruns. Timer p99 is retained without subtraction.", "",
        "Configuration and scheduled cache/output work are charged within timed callbacks.",
        "Prepared plan/buffer policies are reproduced per process below; resource and provider",
        "records remain in evidence.json. Plans prepared before measurement are not measured",
        "as interactive construction. Numerical/lifecycle replay is separate from timing.", "",
        "Publication errors use spectrum-norms-v1; relative errors are dimensionless and",
        "absolute errors use unnormalized FFT magnitude. The publication table retains every",
        "generation, endpoint, history origin, checked-bin count and weak-bin diagnostic.", ""]
    for record in data["records"]:
        config = record["config"]
        if not config.get("transition_suite"):
            continue
        for process in record["processes"]:
            item = process.get("transition")
            if not item:
                raise ValueError("Transition workload missing checked per-process trace")
            trace, tables = item["trace"], item["tables"]
            prefix = dict(stratum=record["stratum"], host=record["host"], config_sha256=identity(config),
                backend=config["backend"], precision=record["contract"]["precision"],
                session=process["session"], repeat=process["repeat"], suite=trace["suite"],
                control=trace["control"], horizon_samples=trace["horizon"], rate=config["rate"],
                block=config["block"], raw=process["raw"], raw_sha256=process["raw_sha256"],
                transition_sha256=item["sha256"])
            events = {event["generation"]: event for event in trace["events"]}
            lines += [f"## {config['backend']} / {process['session']} / Repeat {process['repeat']}", "",
                f"Stratum: {record['stratum']}; configuration: {identity(config)}; "
                f"no-change control: {trace['control']}; horizon: {trace['horizon']} samples.", ""]
            for key in ("time_origin", "latch", "retention", "memory_policy"):
                lines += [key.replace("_", " ").capitalize()+": "+trace[key]+".", ""]
            lines += ["| Generation / Request | Outcome | Application (samples / ms) | First Publication (samples / ms) | Callback Mean / P99 / Max (us) | Observations |",
                "| --- | --- | --- | --- | --- | ---: |"]
            process_costs = {row["generation"]: row for row in tables["costs"]}
            for row in tables["responses"]:
                event = events[row["generation"]]
                responses.append(dict(prefix, **row,
                    application_sample=event["application_sample"],
                    first_publication_sample=event["first_publication_sample"],
                    requested_settings=json.dumps(event["settings"], sort_keys=True)))
                cost = process_costs[row["generation"]]
                def latency(field):
                    samples, ms = row[field+"_latency_samples"], row[field+"_latency_ms"]
                    return "unavailable" if samples is None else f"{samples} / {ms:.6g}"
                lines.append(f"| {row['generation']} / {row['reason']} | {row['outcome']} | "
                    f"{latency('application')} | {latency('first_publication')} | "
                    f"{cost['mean_ns']/1000:.6g} / {cost['p99_ns']/1000:.6g} / "
                    f"{cost['observed_max_ns']/1000:.6g} | {cost['observations']} |")
            lines.append("")
            for row in tables["costs"]:
                costs.append(dict(prefix, **dict(row, callback_indices=json.dumps(row["callback_indices"])),
                    reason=events[row["generation"]]["reason"], request_count=len(events),
                    applied_generation_count=sum(event["application_sample"] >= 0 for event in events.values()),
                    **{name+"_us": row[name+"_ns"]/1000
                       for name in ("mean", "p99", "observed_max", "timer_p99")}))
            for row in tables["publications"]:
                accuracy = row["accuracy"]
                setting = events[row["generation"]]["settings"] if row["generation"] else trace["initial"]
                age = row["publication_sample"]-row["endpoint"]
                publications.append(dict(prefix,
                    **{key: row[key] for key in ("instance", "channel", "generation", "endpoint",
                        "publication_sample", "history_start", "bins")},
                    settings=json.dumps(setting, sort_keys=True), endpoint_age_samples=age,
                    endpoint_age_ms=age*1000/config["rate"], accuracy_policy=accuracy["policy"],
                    tolerance=accuracy["tolerance"], checked_spectra=accuracy["vectors"],
                    checked_bins=accuracy["values"], zero_spectra=accuracy["zero_vectors"],
                    max_relative_l2=accuracy["max_relative_l2"], max_relative_linf=accuracy["max_relative_linf"],
                    max_abs_error=row.get("max_abs_error", accuracy.get("max_abs_error")),
                    max_reference=row.get("max_reference"), absolute_error_units="unnormalized FFT magnitude",
                    legacy_pointwise_failures=accuracy["legacy_pointwise_failures"],
                    max_legacy_scaled_error=accuracy["max_legacy_scaled_error"],
                    worst_pointwise=json.dumps(accuracy["worst_pointwise"], sort_keys=True)))
    for name, columns, rows in (("responses", response_columns, responses),
                                ("callback-costs", cost_columns, costs),
                                ("publications", publication_columns, publications)):
        with (output/("transitions-"+name+".csv")).open("w", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=columns)
            writer.writeheader()
            writer.writerows(rows)
    if not responses:
        lines += ["No transition workloads are present in these checked campaigns.", ""]
    (output/"transitions.md").write_text("\n".join(lines)+"\n")


def figures(data, output):
    os.environ.setdefault("MPLCONFIGDIR", str(output/".matplotlib"))
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import matplotlib.ft2font
    import numpy
    data["plotting_environment"] = dict(python=platform.python_version(), matplotlib=matplotlib.__version__,
                                        numpy=numpy.__version__, freetype=matplotlib.ft2font.__freetype_version__)
    plt.rcParams.update({"svg.hashsalt": "fourier-fr10", "font.size": 8})
    matched, lengths = defaultdict(list), defaultdict(list)
    for record in stationary_records(data):
        matched[comparison_key(record)].append(record)
        lengths[comparison_key(record, True)].append(record)
    catalog = []
    def title(members):
        r = members[0]
        c, contract = r["config"], r["contract"]
        lengths = sorted({m["config"]["n"] for m in members})
        controls = (f"window={c['window']}; octave={c['octave']:g}; "
                    f"time={c['temporal_mode']}:{c['temporal_value']:g}; fixture={c['fixture']}; {c['execution_regime']}"
                    if c.get("workload_schema") == 3 else f"smooth={c['smooth']}")
        return (f"{data['phase'].upper()} · {contract['boundary']} / {contract['operation']}\n"
                f"N={','.join(map(str, lengths))}; H={c['hop']}; B={c['block']}; {contract['precision']}; "
                f"{r['independent_channels']} channels; {c['pass_name']}; {c['state']}; {controls}\n"
                f"offset={c.get('callback_offset', 0)}; {c['alignment']}; load={c['load']}; "
                f"cache={c['cache_mib']} MiB; {c['rate']:g} Hz; host={r['host']}")
    def save(fig, name, kind, members):
        fig.tight_layout()
        fig.savefig(output/(name+".svg"), metadata={"Date": None})
        fig.savefig(output/(name+".png"), dpi=120, metadata={"Software": "Fourier FR-10"})
        plt.close(fig)
        catalog.append(dict(file=name+".svg", preview=name+".png", kind=kind,
                            workloads=[r["config"] for r in members], stratum=members[0]["stratum"]))
    for key, members in sorted(lengths.items()):
        fig, ax = plt.subplots(figsize=(7.5, 4.5))
        backends = defaultdict(list)
        for r in members:
            backends[r["config"]["backend"]].append(r)
        for color_index, (backend, rows) in enumerate(sorted(backends.items())):
            color = plt.get_cmap("tab10")(color_index%10)
            rows.sort(key=lambda r: r["config"]["n"])
            ax.plot([r["config"]["n"] for r in rows], [r["variation"]["mean_of_session_means"] for r in rows], ".-", color=color, label=backend)
            for r in rows:
                v = r["variation"]
                ax.vlines(r["config"]["n"], v["observed_session_min"], v["observed_session_max"], color=color)
        ax.set(xlabel="Transform length N", ylabel=members[0]["processes"][0]["cost_unit"],
               title=title(members))
        ax.legend(fontsize=6); ax.grid(alpha=.2)
        save(fig, "cost-length-"+key, "cost versus length; session ranges", members)
    for key, members in sorted(matched.items()):
        if members[0]["config"]["pass_name"] != "callback":
            continue
        fig, axes = plt.subplots(1, 2, figsize=(10, 4))
        for color_index, r in enumerate(sorted(members, key=lambda r: r["config"]["backend"])):
            color = plt.get_cmap("tab10")(color_index%10)
            name = r["config"]["backend"]
            for i, p in enumerate(r["processes"]):
                axes[0].step([x[0]/1000 for x in p["ecdf"]], [x[1] for x in p["ecdf"]], where="post", color=color, alpha=.6, label=name if i == 0 else None)
            contract = r["contract"]
            age = contract["playback_delay_samples"] if contract["boundary"] == "chain" else contract["publication_delay_samples"]
            if contract["boundary"] == "analysis":
                age += contract["center_offset_samples"]
            age *= 1000/r["config"]["rate"]
            v = r["variation"]
            axes[1].plot(age, v["mean_of_session_means"], ".", color=color, label=name)
            axes[1].vlines(age, v["observed_session_min"], v["observed_session_max"], color=color)
        # Keep rare long observations visible without compressing the bulk of
        # the distribution. Symlog also retains timer-quantized zero durations;
        # the 1 ns linear region is a display scale, not a resolution claim.
        axes[0].set_xscale("symlog", linthresh=.001)
        axes[0].set(xlabel="Callback duration (µs; symlog scale)", ylabel="Empirical CDF per process", ylim=(0, 1.02))
        axes[1].set(xlabel="Delivery delay (ms)" if members[0]["contract"]["boundary"] == "chain" else ("Spectrum-center age (ms)" if members[0]["contract"]["boundary"] == "analysis"
                      else "Publication delay (ms)"), ylabel="ns/engine-sample")
        for ax in axes:
            ax.legend(fontsize=5); ax.grid(alpha=.2)
        fig.suptitle(title(members))
        save(fig, "tail-age-"+key, "per-process callback CDF and cost/age; no pooled callbacks", members)
        fig, ax = plt.subplots(figsize=(8, 5))
        for color_index, r in enumerate(sorted(members, key=lambda r: r["config"]["backend"])):
            color = plt.get_cmap("tab10")(color_index%10)
            v, t = r["variation"], r["tails"]
            x, y = v["mean_of_session_means"], t["median_session_p99_ns"]/1000
            ax.plot(x, y, ".", color=color, label=r["config"]["backend"])
            ax.hlines(y, v["observed_session_min"], v["observed_session_max"], color=color)
            ax.vlines(x, t["observed_session_p99_min_ns"]/1000, t["observed_session_p99_max_ns"]/1000, color=color)
        ax.set(xlabel="Mean cost (ns/engine-sample)", ylabel="Callback p99 (µs; median of session medians)",
               title=title(members))
        ax.text(.01, .01, "Lower left = lower cost and p99; bars = observed session ranges\n"
                "Inspect age and maximum tables too; p99 can miss rare FFT bursts",
                transform=ax.transAxes, fontsize=6, va="bottom")
        ax.legend(fontsize=6, loc="best"); ax.grid(alpha=.2)
        save(fig, "cost-tail-"+key, "mean cost versus callback p99; observed session ranges", members)
    return catalog


def report(directories, output, phase, plots=True):
    for directory in directories:
        if output.resolve() == directory.resolve() or directory.resolve() in output.resolve().parents:
            raise ValueError("Reports must be outside campaign artifacts")
    data = collect(directories, phase)
    data["generator_sha256"] = digest(Path(__file__))
    output.mkdir(parents=True, exist_ok=False)
    tables(data, output)
    data["figures"] = figures(data, output) if plots else []
    (output/"evidence.json").write_text(json.dumps(data, indent=2, sort_keys=True)+"\n")
    artifacts = {p.name: digest(p) for p in sorted(output.iterdir()) if p.is_file()}
    (output/"manifest.json").write_text(json.dumps(artifacts, indent=2, sort_keys=True)+"\n")
    return len(data["records"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("campaigns", type=Path, nargs="+")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--phase", choices=("smoke", "pilot", "confirmation"), required=True)
    parser.add_argument("--no-plots", action="store_true", help="Tables/JSON only; matplotlib unnecessary")
    args = parser.parse_args()
    print(f"Wrote {report(args.campaigns, args.output, args.phase, not args.no_plots)} evidence rows")

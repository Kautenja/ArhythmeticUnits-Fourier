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
from run import digest, quantile

LIMITS = ("Costs retain traversal, conversion, required stores and configured background load. "
          "No timer subtraction. Callback quantiles and maxima describe each process, not WCET or device underruns. "
          "Ranges across session means describe observed variation, not confidence intervals. "
          "Session labels are supplied by the operator; labels alone do not prove independence. "
          "C++ live heap includes the object; native allocation and stack may remain unknown. "
          "Different arithmetic, plans, storage and publication ages remain explicit confounds. "
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


def collect(directories, phase):
    records, sources, seen = {}, [], set()
    for directory in sorted(directories, key=lambda p: str(p.resolve())):
        check(directory)
        metadata = json.loads((directory/"metadata.json").read_text())
        if metadata.get("phase") != phase:
            raise ValueError("Report phase must match every campaign; smoke/pilot/confirmation cannot be mixed")
        host, session = metadata.get("host_id"), metadata.get("session_id")
        if not host or not session:
            raise ValueError("Missing host/session identity")
        provenance = {key: metadata.get(key) for key in (
            "source_sha256", "sdk_sha256", "external_dependency_sha256", "compiler", "build_command",
            "build_features", "platform", "machine", "cpu_model")}
        # Different hosts, source/dependency bytes or build policies cannot be pooled.
        stratum = identity(dict(host=host, provenance=provenance))[:16]
        sources.append(dict(directory=str(directory.resolve()), metadata_sha256=digest(directory/"metadata.json"),
                            stratum=stratum, host=host, session=session, provenance=provenance,
                            revision=metadata["revision"], binary_sha256=metadata["binary_sha256"],
                            notes=metadata.get("notes", ""), phase=phase))
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
                independent_channels=config["count"]*contract["channels"],
                input_contract="independent-four-v1" if descriptor["kind"] == "analysis4" else "common-float-v1"))
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
                values, visible, playback = [], [], []
                with (directory/run["raw"]).open() as stream:
                    for row in csv.DictReader(stream):
                        if row["kind"] == mode:
                            values.append(float(row["ns"]))
                        if row["kind"] == "publication":
                            visible.append(float(row["callback_visible_age_samples"]))
                            playback.append(float(row["playback_delay_samples"]))
                values.sort()
                # Retain endpoints and a deterministic ECDF grid; do not pool processes.
                indices = sorted({i*(len(values)-1)//min(4095, max(1, len(values)-1))
                                  for i in range(min(4096, len(values)))})
                record["processes"].append(dict(session=session, repeat=run["repeat"], cost=cost,
                    cost_unit="ns/engine-sample" if mode in ("callback", "throughput") else "ns/transform",
                    cost_per_channel=cost/record["independent_channels"], timing=timing, timer=groups["timer"],
                    observation_count=len(values), observation_window_samples=config["callbacks"]*config["block"]
                        if mode in ("callback", "throughput") else None,
                    ecdf=[[values[i], (i+1)/len(values)] for i in indices],
                    callback_visible_age_range=[min(visible), max(visible)] if visible else None,
                    playback_delay_range=[min(playback), max(playback)] if playback else None,
                    accuracy=accuracy, numerical_status="per-run numerical report; see reference coverage" if accuracy else "preflight only; no per-run numerical report",
                    raw=str((directory/run["raw"]).resolve()), raw_sha256=digest(directory/run["raw"])))
    result = []
    for key, record in sorted(records.items()):
        record["variation"] = variation(record["processes"])
        if phase == "confirmation" and record["variation"]["sessions"] < 3:
            raise ValueError("Confirmation reporting requires at least three labeled sessions per workload/stratum")
        result.append(record)
    if not result:
        raise ValueError("No checked observations")
    return dict(schema=1, phase=phase, limitations=LIMITS, sources=sources, records=result)


def comparison_key(record, vary_length=False):
    config = {k: v for k, v in record["config"].items() if k != "backend" and (not vary_length or k != "n")}
    contract = record["contract"]
    return identity(dict(stratum=record["stratum"], config=config, boundary=contract["boundary"],
                         operation=contract["operation"], precision=contract["precision"],
                         channels=record["independent_channels"], input=record["input_contract"]))[:16]


def tables(data, output):
    columns = ["stratum", "backend", "boundary", "operation", "precision", "n", "hop", "block", "channels",
               "mode", "state", "callback_offset", "cost_unit", "mean_session_cost", "session_min", "session_max",
               "sessions", "processes", "publication_delay_samples", "center_offset_samples", "playback_delay_samples",
               "cpp_live_heap_bytes_min", "cpp_live_heap_bytes_max", "native_memory", "max_abs_error", "numerical_status", "callback_visible_age_min", "callback_visible_age_max"]
    lines = ["# External FFT Evidence Report", "", "Evidence phase: **"+data["phase"].upper()+"**.", "",
             "SMOKE data validates tooling only and must not enter manuscript results." if data["phase"] == "smoke"
             else "Pilot informs design; only frozen confirmation campaigns support final results.", "", LIMITS, "",
             "See results.csv for matched workload/cost/age/error/storage columns and evidence.json for every process, native plan, raw-data hash and source identity.", "",
             "| Backend | Family / Operation | N / H | Channels | Cost | Session Range | Sessions |",
             "| --- | --- | ---: | ---: | ---: | --- | ---: |"]
    with (output/"results.csv").open("w", newline="") as stream:
        writer = csv.writer(stream); writer.writerow(columns)
        for r in data["records"]:
            c, contract, v = r["config"], r["contract"], r["variation"]
            heaps = [x["measurements"]["allocation"]["setup"]["live_bytes"] for x in r["resources"]]
            errors = [p["accuracy"]["max_abs_error"] for p in r["processes"] if p["accuracy"]]
            status = sorted({p["numerical_status"] for p in r["processes"]})
            visible = [age for p in r["processes"] for age in (p["callback_visible_age_range"] or [])]
            unit = r["processes"][0]["cost_unit"]
            writer.writerow([r["stratum"], c["backend"], contract["boundary"], contract["operation"], contract["precision"],
                c["n"], c["hop"], c["block"], r["independent_channels"], c["pass_name"], c["state"], c.get("callback_offset", 0),
                unit, v["mean_of_session_means"], v["observed_session_min"], v["observed_session_max"], v["sessions"], v["processes"],
                contract["publication_delay_samples"], contract["center_offset_samples"], contract["playback_delay_samples"],
                min(heaps), max(heaps), "provider-specific or unknown; see evidence.json", max(errors) if errors else "unavailable", "; ".join(status),
                min(visible) if visible else "unavailable", max(visible) if visible else "unavailable"])
            lines.append(f"| {c['backend']} | {contract['boundary']} / {contract['operation']} | {c['n']} / {c['hop']} | "
                         f"{r['independent_channels']} | {v['mean_of_session_means']:.3f} {unit} | "
                         f"{v['observed_session_min']:.3f}–{v['observed_session_max']:.3f} | {v['sessions']} |")
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
                json.dumps([dict(session=p["session"], repeat=p["repeat"], accuracy=p["accuracy"], status=p["numerical_status"])
                            for p in r["processes"]], sort_keys=True)])
    (output/"report.md").write_text("\n".join(lines)+"\n")


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
    for record in data["records"]:
        matched[comparison_key(record)].append(record)
        lengths[comparison_key(record, True)].append(record)
    catalog = []
    def title(members):
        r = members[0]
        c, contract = r["config"], r["contract"]
        lengths = sorted({m["config"]["n"] for m in members})
        return (f"{data['phase'].upper()} · {contract['boundary']} / {contract['operation']}\n"
                f"N={','.join(map(str, lengths))}; H={c['hop']}; B={c['block']}; {contract['precision']}; "
                f"{r['independent_channels']} channels; {c['pass_name']}; {c['state']}; smooth={c['smooth']}\n"
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
                axes[0].step([x[0] for x in p["ecdf"]], [x[1] for x in p["ecdf"]], where="post", color=color, alpha=.6, label=name if i == 0 else None)
            contract = r["contract"]
            age = contract["playback_delay_samples"] if contract["boundary"] == "chain" else contract["publication_delay_samples"]
            if contract["boundary"] == "analysis":
                age += contract["center_offset_samples"]
            v = r["variation"]
            axes[1].plot(age, v["mean_of_session_means"], ".", color=color, label=name)
            axes[1].vlines(age, v["observed_session_min"], v["observed_session_max"], color=color)
        axes[0].set(xlabel="Callback duration (ns)", ylabel="Empirical CDF per process", ylim=(0, 1.02))
        axes[1].set(xlabel="Delivery delay (samples)" if members[0]["contract"]["boundary"] == "chain" else ("Spectrum-center age (samples)" if members[0]["contract"]["boundary"] == "analysis"
                      else "Publication delay (samples)"), ylabel="ns/engine-sample")
        for ax in axes:
            ax.legend(fontsize=5); ax.grid(alpha=.2)
        fig.suptitle(title(members))
        save(fig, "tail-age-"+key, "per-process callback CDF and cost/age; no pooled callbacks", members)
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

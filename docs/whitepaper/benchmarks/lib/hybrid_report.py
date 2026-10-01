#!/usr/bin/env python3
"""Produce matched attribution tables from validated FR-6 campaign artifacts."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import json
from pathlib import Path
import statistics

from check import check
from run import digest

BACKENDS = ("pffft-scheduled-batch-float", "pffft-hybrid-float", "pffft-analysis-float",
            "legacy-batch-float", "legacy-incremental-float", "core-float")
LIMITATIONS = (
    "PFFFT scheduled batch/hybrid share arithmetic, caches, storage and task dispatch; "
    "their difference includes quota execution and work placement. The ordinary PFFFT batch "
    "uses a shorter ring and bulk loops. Legacy batch/incremental isolate scheduling within "
    "legacy arithmetic. Production uses a different positive-bin radix-2 pipeline, maximum-size "
    "plans and retained storage: comparisons with it do not isolate scheduling overhead. "
    "Native FFT packing, execution and output conversion remain indivisible. Task counts are "
    "not equal-cost timing bounds. Ratios pair process repetitions, not independent callback "
    "observations; no confidence interval or performance conclusion follows from a smoke run."
)


def assemble(directory, metadata):
    groups = {}
    for index, config in enumerate(metadata["configs"]):
        if config["backend"] not in BACKENDS:
            continue
        workload = {k: v for k, v in config.items() if k != "backend"}
        key = json.dumps(workload, sort_keys=True)
        group = groups.setdefault(key, dict(workload=workload, backends={}))
        resource = json.loads((directory/metadata["resources"][str(index)]).read_text())
        runs = [r for r in metadata["runs"] if r["workload"] == index]
        observations = []
        for run in sorted(runs, key=lambda r: r["repeat"]):
            timing = run["summary"]["groups"][config["pass_name"]]
            divisor = config["block"]*(config["callbacks"] if config["pass_name"] == "throughput" else 1)
            observations.append(dict(repeat=run["repeat"], raw=run["raw"],
                                     ns_per_engine_sample=timing["mean_ns"]/divisor,
                                     timing=timing))
        group["backends"][config["backend"]] = dict(
            contract=metadata["contracts"][str(index)], resources=resource, observations=observations)
    if not groups:
        raise ValueError("No FR-6 workloads")
    for group in groups.values():
        if set(group["backends"]) != set(BACKENDS):
            raise ValueError("Attribution requires all six matched backends for each workload")
        comparisons = []
        for numerator, denominator, interpretation in (
                (BACKENDS[1], BACKENDS[0], "same arithmetic/storage; scheduling and placement"),
                (BACKENDS[4], BACKENDS[3], "legacy arithmetic; scheduling and placement"),
                (BACKENDS[1], BACKENDS[2], "practical external baseline; loops/storage differ"),
                (BACKENDS[1], BACKENDS[5], "practical comparison; arithmetic/layout/plans/storage differ")):
            top = group["backends"][numerator]["observations"]
            bottom = group["backends"][denominator]["observations"]
            if [r["repeat"] for r in top] != [r["repeat"] for r in bottom] or not top:
                raise ValueError("Unmatched process repetitions")
            ratios = [a["ns_per_engine_sample"]/b["ns_per_engine_sample"]
                      if b["ns_per_engine_sample"] > 0 else None for a, b in zip(top, bottom)]
            comparisons.append(dict(numerator=numerator, denominator=denominator,
                                    interpretation=interpretation, process_ratios=ratios,
                                    median_ratio=statistics.median(ratios) if None not in ratios else None))
        group["comparisons"] = comparisons
    return list(groups.values())


def report(directory, output):
    check(directory)
    metadata = json.loads((directory/"metadata.json").read_text())
    groups = assemble(directory, metadata)
    result = dict(schema=1, campaign=str(directory.resolve()),
                  metadata_sha256=digest(directory/"metadata.json"), generator_sha256=digest(Path(__file__)),
                  limitations=LIMITATIONS, groups=groups)
    # Reports must not mutate the evidence directory or replace prior results.
    if output.resolve() == directory.resolve() or directory.resolve() in output.resolve().parents:
        raise ValueError("Write attribution outside the campaign directory")
    output.mkdir(parents=True, exist_ok=False)
    (output/"attribution.json").write_text(json.dumps(result, indent=2)+"\n")
    lines = ["# Hybrid Analysis Attribution", "", LIMITATIONS, "",
             "Per-process costs include all analyzers and background load. No timer subtraction is applied.", ""]
    for index, group in enumerate(groups):
        lines += [f"## Workload {index+1}", "", "```json", json.dumps(group["workload"], sort_keys=True), "```", "",
                  "| Backend | Median Process ns/Engine Sample | Publication Delay (Samples) | C++ Live Heap Bytes (Includes Object) |",
                  "| --- | ---: | ---: | ---: |"]
        for backend in BACKENDS:
            item = group["backends"][backend]
            cost = statistics.median(r["ns_per_engine_sample"] for r in item["observations"])
            memory = item["resources"]["allocation"]
            lines.append(f"| {backend} | {cost:.3f} | {item['contract']['publication_delay_samples']} | "
                         f"{memory['setup']['live_bytes']} |")
        lines += ["", "Native storage and stack are additional; see the retained resource records in attribution.json.", "",
                  "| Ratio (Numerator / Denominator) | Median Process Ratio | Interpretation |",
                  "| --- | ---: | --- |"]
        for pair in group["comparisons"]:
            value = "undefined" if pair["median_ratio"] is None else f"{pair['median_ratio']:.4f}"
            lines.append(f"| {pair['numerator']} / {pair['denominator']} | {value} | {pair['interpretation']} |")
        lines.append("")
    (output/"attribution.md").write_text("\n".join(lines)+"\n")
    return len(groups)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("campaign", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(f"Wrote {report(args.campaign, args.output)} matched attribution groups")

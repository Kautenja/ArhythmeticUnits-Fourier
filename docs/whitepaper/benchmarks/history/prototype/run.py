#!/usr/bin/env python3
"""Run the report's experiments without changing the archived evidence."""

import argparse
import csv
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[5]


def output(command):
    return subprocess.check_output(command, cwd=ROOT, text=True).strip()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT / "docs/whitepaper/.build/evaluation")
    parser.add_argument("--cpu", default="unrecorded")
    parser.add_argument("--memory-gib", type=int)
    args = parser.parse_args()
    directory = args.out.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    binary = directory / "evaluate"
    command = [os.environ.get("CXX", "g++"), "-std=c++11", "-O3", "-DNDEBUG",
               "-Wall", "-Wextra", "-pedantic", "-Isrc",
               "docs/whitepaper/benchmarks/history/prototype/evaluate.cpp", "-o", str(binary)]
    source_paths = [ROOT / "docs/whitepaper/benchmarks/history/prototype/evaluate.cpp",
                    ROOT / "docs/whitepaper/benchmarks/history/prototype/run.py"]
    source_paths += sorted((ROOT / "src/dsp").glob("*.hpp"))
    before = {str(p.relative_to(ROOT)): digest(p) for p in source_paths}
    meta = {
        "started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "git_head": output(["git", "rev-parse", "HEAD"]),
        "source_sha256": before,
        "source_status": output(["git", "status", "--short", "--", "src", "test"]),
        "compiler": output([command[0], "--version"]),
        "compile_command": command,
        "platform": platform.platform(),
        "cpu": args.cpu,
        "memory_gib": args.memory_gib,
        "repetitions": 9,
        "frames_per_pass": 64,
        "warmup_frames": 16,
        "bootstrap_resamples": 10000,
        "bootstrap_seed": 20260928,
        "scope": "Scalar single-channel RFFT; fixed H cadence; no Rack, smoothing, graphics, or optimized external FFT baseline.",
    }
    subprocess.run(command, cwd=ROOT, check=True)
    for mode, filename in [("verify", "verification.csv"), ("measure", "timing.csv")]:
        with (directory / filename).open("w") as stream:
            subprocess.run([str(binary), mode], stdout=stream, cwd=ROOT, check=True)
    meta["clock"] = json.loads(output([str(binary), "clock"]))
    if any(digest(p) != before[str(p.relative_to(ROOT))] for p in source_paths):
        raise RuntimeError("Source changed during the experiment; discard this run")
    rows = list(csv.DictReader((directory / "timing.csv").open()))
    summary = []
    random_source = random.Random(20260928)
    for n, hop in sorted({(int(r["n"]), int(r["hop"])) for r in rows}):
        groups = {}
        for mode in ["complete", "incremental"]:
            selected = [r for r in rows if int(r["n"]) == n and int(r["hop"]) == hop and r["mode"] == mode]
            groups[mode] = sorted(selected, key=lambda r: int(r["repetition"]))
            entry = {"n": n, "hop": hop, "mode": mode}
            for field in ["frame_ns", "call_p99_ns", "call_max_ns", "start_median_ns", "finish_median_ns"]:
                values = [float(r[field]) for r in selected]
                entry[field + "_median"] = statistics.median(values)
                entry[field + "_min"] = min(values)
                entry[field + "_max"] = max(values)
            summary.append(entry)
        for field in ["frame_ns", "call_max_ns"]:
            ratios = [float(i[field]) / float(c[field])
                      for i, c in zip(groups["incremental"], groups["complete"])]
            bootstrap = sorted(statistics.median(random_source.choices(ratios, k=len(ratios)))
                               for _ in range(10000))
            summary[-1][field + "_paired_ratio"] = statistics.median(ratios)
            summary[-1][field + "_paired_ratio_ci95"] = [bootstrap[249], bootstrap[9749]]
    meta["finished_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    meta["data_sha256"] = {name: digest(directory / name)
                           for name in ["verification.csv", "timing.csv"]}
    (directory / "metadata.json").write_text(json.dumps(meta, indent=2) + "\n")
    (directory / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(f"Wrote verified experiment outputs to {directory}")


if __name__ == "__main__":
    main()

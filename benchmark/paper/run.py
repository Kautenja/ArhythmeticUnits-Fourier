#!/usr/bin/env python3
"""Build and retain reproducible, serial paper experiments (Python stdlib only)."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import csv
import datetime as dt
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import random
import shlex
import statistics
import subprocess
import tarfile

from contracts import SYNTHESIS_BACKENDS, synthesis_contract

ROOT = Path(__file__).resolve().parents[2]
BINARY = ROOT / "build/benchmark/rack/paper"
BASE = dict(backend="core-float", pass_name="callback", n=2048, hop=1024,
            block=64, count=1, alignment="aligned", load=0, smooth=0, voices=1,
            rate=48000, state="steady", cache_mib=0)


def workload(**changes):
    return dict(BASE, **changes)


def matrix(profile):
    """Factor sweeps, not an unbounded Cartesian product; custom JSON is supported."""
    rows = synthesis_matrix(profile == "smoke")
    if profile == "synthesis":
        return rows
    cores = ("core-float", "core-double", "core-simd4", "legacy-batch-float",
             "legacy-incremental-float", "legacy-batch-double", "legacy-incremental-double")
    modules = ("fourier", "spectre")
    if profile == "smoke":
        for backend in cores + modules:
            rows += [workload(backend=backend, block=64),
                     workload(backend=backend, pass_name="throughput", smooth=1),
                     workload(backend=backend, count=4, alignment="staggered", load=8),
                     workload(backend=backend, state="startup"),
                     workload(backend=backend, state="live", cache_mib=1)]
    else:
        for backend in cores:
            for n in (128, 2048, 16384):
                for hop in (257, 1024):
                    for smooth in (0, 1):
                        for block in (1, 16, 64, 256):
                            rows.append(workload(backend=backend, n=n, hop=hop, smooth=smooth, block=block))
                        rows.append(workload(backend=backend, n=n, hop=hop, smooth=smooth, pass_name="throughput"))
        # Multiple active analyzers, identical workload under both alignments.
        for backend in cores + modules:
            for count in (1, 4, 16):
                for alignment in ("aligned", "staggered"):
                    for load in (0, 64):
                        for pass_name in ("callback", "throughput"):
                            rows.append(workload(backend=backend, count=count, alignment=alignment,
                                                 load=load, pass_name=pass_name))
            for state in ("startup", "live"):
                rows.append(workload(backend=backend, state=state))
            rows.append(workload(backend=backend, cache_mib=32))
        for backend in modules:
            for n in ((128, 2048, 16384) if backend == "fourier" else (2048,)):
                for rate in (48000, 96000, 192000):
                    for voices in (1, 16):
                        for smooth in (0, 1):
                            for block in (16, 64, 256):
                                rows.append(workload(backend=backend, n=n, rate=rate, voices=voices,
                                                     smooth=smooth, block=block))
    for block in ((64,) if profile == "smoke" else (1, 16, 64, 256)):
        for count in (1, 4, 16):
            for load in (0, 64):
                for pass_name in ("callback", "throughput"):
                    rows.append(workload(backend="driver", block=block, count=count, load=load, pass_name=pass_name))
    for name in ("fft", "rfft", "ifft"):
        for precision in ("float", "double"):
            for n in ((128,) if profile == "smoke" else (128, 2048, 16384)):
                for mode in ("complete", "incremental", "phases", "steps"):
                    rows.append(workload(backend=f"{name}-{precision}", n=n, pass_name=mode, hop=257))
    # Duplicate base rows from independent factor sweeps need only one identity.
    return list({json.dumps(row, sort_keys=True): row for row in rows}.values())


def synthesis_matrix(smoke):
    """Matched inverse and full filtering controls; independent of Rack modules."""
    rows = []
    for backend in sorted(SYNTHESIS_BACKENDS):
        base = workload(backend=backend, n=128 if smoke else 2048, hop=32 if smoke else 1024)
        for n in ((128,) if smoke else (128, 2048, 16384)):
            for mode in ("callback", "throughput"):
                for block in ((64,) if smoke or mode == "throughput" else (16, 64, 256)):
                    rows.append(dict(base, n=n, hop=32 if n == 128 else 1024, pass_name=mode, block=block))
        rows += [dict(base, state="startup"),
                 dict(base, count=4, alignment="staggered", load=8, cache_mib=1)]
        if not smoke:
            rows += [dict(base, hop=257), dict(base, rate=96000),
                     dict(base, count=16, load=64),
                     dict(base, count=16, alignment="staggered", load=64)]
    return list({json.dumps(row, sort_keys=True): row for row in rows}.values())


def command(config):
    keys = ("backend", "pass_name", "n", "hop", "block", "count", "alignment", "load",
            "smooth", "voices", "callbacks", "warm_hops", "rate", "state", "cache_mib")
    return [str(BINARY)] + [str(config[key]) for key in keys] + ["v1"]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture(args, cwd=ROOT):
    return subprocess.check_output(args, cwd=cwd, text=True, stderr=subprocess.STDOUT).strip()


def quantile(values, fraction):
    """Nearest-rank quantile; retain raw rows rather than treating it as a bound."""
    return sorted(values)[max(0, math.ceil(len(values)*fraction)-1)]


def summarize(path, config):
    groups = {}
    publications = 0
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream):
            if row["kind"] == "publication":
                publications += 1
                continue
            groups.setdefault(row["kind"], []).append(float(row["ns"]))
    result = {"publication_audit_rows": publications, "groups": {}}
    for kind, values in groups.items():
        if not values or not all(math.isfinite(v) and v >= 0 for v in values):
            raise ValueError(f"Invalid measurements in {path}")
        summary = dict(observations=len(values), total_ns=sum(values), mean_ns=statistics.mean(values),
                       p50_ns=quantile(values, .5), p95_ns=quantile(values, .95),
                       p99_ns=quantile(values, .99), observed_max_ns=max(values))
        if kind in ("callback", "throughput"):
            samples = config["callbacks"]*config["block"]
            budget_ns = 1e9*config["block"]/config["rate"]
            summary.update(ns_per_engine_sample=sum(values)/samples,
                           simulated_compute_utilization=sum(values)/(1e9*samples/config["rate"]))
            if kind == "callback":
                summary.update(budget_ns=budget_ns,
                               observed_compute_budget_exceedances=sum(v > budget_ns for v in values))
        result["groups"][kind] = summary
    if "timer" not in groups or len(groups) < 2:
        raise ValueError(f"Missing timing rows in {path}")
    return result


def save(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="New campaign directory; never overwritten")
    parser.add_argument("--profile", choices=("smoke", "paper", "synthesis"), default="smoke")
    parser.add_argument("--config", type=Path, help="JSON array of complete or partial workload objects")
    parser.add_argument("--repeats", type=int, default=7)
    parser.add_argument("--hops", type=int, default=64, help="Minimum measured hops per streaming pass")
    parser.add_argument("--frames", type=int, default=64, help="Transform frames per pass")
    parser.add_argument("--step-frames", type=int, default=2, help="Frames retaining every transform step")
    parser.add_argument("--warm-hops", type=int, default=64)
    parser.add_argument("--seed", type=int, default=20260929)
    parser.add_argument("--rack-dir", type=Path, default=ROOT / "../..")
    parser.add_argument("--cxx", default="c++")
    parser.add_argument("--notes", default="", help="Power mode, affinity, host activity, session context")
    parser.add_argument("--list", action="store_true", help="Print resolved workloads without building/running")
    args = parser.parse_args()
    if min(args.repeats, args.hops, args.frames, args.step_frames) < 1 or args.warm_hops < 0:
        parser.error("Counts must be positive and warmup nonnegative")
    if args.hops < 2:
        parser.error("At least two measured hops are required")
    configs = matrix(args.profile)
    if args.config:
        configs = [workload(**item) for item in json.loads(args.config.read_text())]
    for config in configs:
        if config.keys() - set(BASE):
            parser.error("Unknown workload keys: " + str(config.keys() - set(BASE)))
        if config["backend"].startswith(("inverse-stream-", "ols-")):
            try:
                synthesis_contract(config)
            except ValueError as error:
                parser.error(str(error))
        config["warm_hops"] = args.warm_hops
        config["callbacks"] = (math.ceil(args.hops*config["hop"]/config["block"])
                               if config["pass_name"] in ("callback", "throughput") else
                               args.step_frames if config["pass_name"] == "steps" else args.frames)
    if not configs:
        parser.error("Empty workload matrix")
    if args.list:
        print(json.dumps(configs, indent=2))
        return
    output = args.output.resolve()
    if any(base == output or base in output.parents for base in (ROOT/"src", ROOT/"benchmark", ROOT/".git")):
        parser.error("Write campaigns outside source and Git metadata directories")
    output.mkdir(parents=True, exist_ok=False)
    rack = args.rack_dir.resolve()
    build = ["make", "-B", "benchmark-paper-build", f"RACK_DIR={rack}", f"CXX={args.cxx}"]
    metadata = dict(schema=1, status="incomplete", started_utc=dt.datetime.now(dt.timezone.utc).isoformat(),
                    revision=capture(["git", "rev-parse", "HEAD"]),
                    git_status=capture(["git", "status", "--porcelain"]),
                    platform=platform.platform(), machine=platform.machine(), processor=platform.processor(),
                    python=platform.python_version(), cpu_count=os.cpu_count(),
                    compiler=capture(shlex.split(args.cxx)+["--version"]), build_command=build,
                    rack_dir=str(rack), seed=args.seed, repeats=args.repeats, notes=args.notes,
                    protocol="v1", configs=configs, runs=[])
    metadata["synthesis_contracts"] = {
        str(index): synthesis_contract(config) for index, config in enumerate(configs)
        if config["backend"] in SYNTHESIS_BACKENDS}
    if platform.system() == "Darwin":
        try:
            metadata["cpu_model"] = capture(["sysctl", "-n", "machdep.cpu.brand_string"])
        except subprocess.CalledProcessError as error:
            metadata["cpu_model"] = "unavailable: " + error.output.strip()
    elif Path("/proc/cpuinfo").exists():
        metadata["cpuinfo"] = Path("/proc/cpuinfo").read_text()
    save(output/"metadata.json", metadata)
    with (output/"build.log").open("w") as log:
        subprocess.run(build, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
    metadata["binary_sha256"] = digest(BINARY)
    # Archive working sources, including uncommitted benchmark development.
    sources = sorted({p for base in (ROOT/"src", ROOT/"benchmark") for p in base.rglob("*")
                      if p.is_file() and "__pycache__" not in p.parts} |
                     {ROOT/"Makefile", ROOT/"SConstruct", ROOT/"plugin.json"})
    metadata["source_sha256"] = {str(p.relative_to(ROOT)): digest(p) for p in sources}
    with tarfile.open(output/"source.tar.gz", "w:gz") as archive:
        for path in sources:
            archive.add(path, arcname=str(path.relative_to(ROOT)))
    # SDK headers/build rules and linked library affect generated code/behavior.
    sdk = sorted({p for base in (rack/"include", rack/"dep/include") for p in base.rglob("*") if p.is_file()} |
                 {p for p in rack.glob("*.mk")} | {p for p in rack.glob("libRack.*") if p.is_file()})
    metadata["sdk_sha256"] = {str(p.relative_to(rack)): digest(p) for p in sdk}
    try:
        metadata["rack_revision"] = capture(["git", "rev-parse", "HEAD"], rack)
        metadata["rack_status"] = capture(["git", "status", "--porcelain", "--untracked-files=no"], rack)
    except subprocess.CalledProcessError:
        metadata["rack_revision"] = "SDK without Git metadata"
    env = dict(os.environ, DYLD_LIBRARY_PATH=str(rack), LD_LIBRARY_PATH=str(rack))
    with (output/"verification.txt").open("w") as verification:
        subprocess.run([str(BINARY), "--verify"], cwd=ROOT, env=env,
                       stdout=verification, stderr=subprocess.STDOUT, check=True)
    jobs = [(repeat, index) for repeat in range(args.repeats) for index in range(len(configs))]
    random.Random(args.seed).shuffle(jobs)
    save(output/"metadata.json", metadata)
    for ordinal, (repeat, index) in enumerate(jobs):
        config = configs[index]
        stem = f"workload-{index:04d}-repeat-{repeat:02d}"
        raw, errors = output/(stem+".csv"), output/(stem+".stderr")
        invocation = command(config)
        print(f"[{ordinal+1}/{len(jobs)}] {stem} {config['backend']} {config['pass_name']}", flush=True)
        started = dt.datetime.now(dt.timezone.utc).isoformat()
        with raw.open("w") as stdout, errors.open("w") as stderr:
            subprocess.run(invocation, cwd=ROOT, env=env, stdout=stdout, stderr=stderr, check=True)
        metadata["runs"].append(dict(workload=index, repeat=repeat, command=invocation,
                                     started_utc=started, raw=raw.name, stderr=errors.name,
                                     summary=summarize(raw, config)))
        save(output/"metadata.json", metadata)
    metadata["status"] = "complete"
    metadata["finished_utc"] = dt.datetime.now(dt.timezone.utc).isoformat()
    metadata["artifact_sha256"] = {p.name: digest(p) for p in output.iterdir()
                                  if p.is_file() and p.name != "metadata.json"}
    save(output/"metadata.json", metadata)


if __name__ == "__main__":
    main()

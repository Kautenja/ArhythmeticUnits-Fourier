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
import shutil
import subprocess
import tarfile

from contracts import SYNTHESIS_BACKENDS, load_registry, resolve_contract, validate_config
from dependencies import fftw_inputs
from campaigns import resolve as resolve_campaign, inventory as campaign_inventory

ROOT = Path(__file__).resolve().parents[2]
BINARY = ROOT / (".build/benchmark/rack/paper.exe" if os.name == "nt" else
                 ".build/benchmark/rack/paper")
BASE = dict(backend="core-float", pass_name="callback", n=2048, hop=1024,
            block=64, count=1, alignment="aligned", load=0, smooth=0, voices=1,
            rate=48000, state="steady", cache_mib=0, callback_offset=0)


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
    return [str(BINARY)] + [str(config[key]) for key in keys] + [str(config.get("callback_offset", 0)), "v2"]


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
    parser.add_argument("output", type=Path, nargs="?", help="New campaign directory; never overwritten")
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
    parser.add_argument("--inventory", action="store_true", help="List capabilities, including unavailable adapters")
    parser.add_argument("--fftw-prefix", type=Path, help="Enable optional serial float/double FFTW static libraries")
    parser.add_argument("--enable-vdsp", action="store_true", help="Enable macOS Accelerate/vDSP research adapters")
    parser.add_argument("--variant", help="Explicit host variant for a campaign manifest: rack, portable, macos")
    parser.add_argument("--describe-matrix", action="store_true", help="Resolved counts, channel contracts and configurations")
    parser.add_argument("--phase", choices=("smoke", "pilot", "confirmation"), help="Evidence classification; defaults to smoke")
    parser.add_argument("--session-id", default="", help="Independent measurement session label; required outside smoke")
    parser.add_argument("--host-id", default="", help="Physical measurement host label; required outside smoke")
    args = parser.parse_args()
    features, external_inputs = [], {}
    if args.fftw_prefix:
        args.fftw_prefix = args.fftw_prefix.resolve()
        try:
            external_inputs.update(fftw_inputs(args.fftw_prefix))
        except ValueError as error:
            parser.error(str(error))
        features.append("fftw")
    if args.enable_vdsp:
        if platform.system() != "Darwin":
            parser.error("vDSP is unavailable: --enable-vdsp requires macOS")
        features.append("vdsp")
    registry = load_registry(features=features)
    if args.inventory:
        print(json.dumps(registry, indent=2, sort_keys=True))
        return
    if args.output is None and not (args.list or args.describe_matrix):
        parser.error("An output directory is required")
    if min(args.repeats, args.hops, args.frames, args.step_frames) < 1 or args.warm_hops < 0:
        parser.error("Counts must be positive and warmup nonnegative")
    if args.hops < 2:
        parser.error("At least two measured hops are required")
    configs = matrix(args.profile)
    manifest = None
    if args.config:
        document = json.loads(args.config.read_text())
        if isinstance(document, dict):
            try:
                configs, manifest = resolve_campaign(document, args.variant, registry, platform.system(), BASE)
            except (ValueError, KeyError) as error:
                parser.error(str(error))
        else:
            if args.variant:
                parser.error("--variant requires a campaign manifest")
            configs = [workload(**item) for item in document]
    phase = args.phase or ("pilot" if manifest and manifest["phase"] != "smoke" else "smoke")
    if manifest and manifest["phase"] == "smoke" and phase != "smoke":
        parser.error("Smoke configurations cannot become publication evidence")
    if not (args.list or args.describe_matrix) and phase != "smoke" and not (args.session_id and args.host_id):
        parser.error("Pilot/confirmation requires explicit --host-id and --session-id")
    for config in configs:
        if config.keys() - set(BASE):
            parser.error("Unknown workload keys: " + str(config.keys() - set(BASE)))
        try:
            validate_config(config, registry)
        except ValueError as error:
            parser.error(str(error))
        config["warm_hops"] = args.warm_hops
        config["callbacks"] = (math.ceil(args.hops*config["hop"]/config["block"])
                               if config["pass_name"] in ("callback", "throughput") else
                               args.step_frames if config["pass_name"] == "steps" else args.frames)
        try:
            validate_config(config, registry, measurement=True)
        except ValueError as error:
            parser.error(str(error))
    if len({json.dumps(c, sort_keys=True) for c in configs}) != len(configs):
        parser.error("Duplicate workload configuration")
    if not configs:
        parser.error("Empty workload matrix")
    if args.list or args.describe_matrix:
        result = dict(campaign=manifest, phase=phase, inventory=campaign_inventory(configs, registry), configs=configs) if args.describe_matrix else configs
        print(json.dumps(result, indent=2))
        return
    output = args.output.resolve()
    if any(base == output or base in output.parents for base in (ROOT/"src", ROOT/"benchmark", ROOT/".git")):
        parser.error("Write campaigns outside source and Git metadata directories")
    output.mkdir(parents=True, exist_ok=False)
    rack = args.rack_dir.resolve()
    build = ["make", "-B", "benchmark-paper-build", f"RACK_DIR={rack}", f"CXX={args.cxx}"]
    # The recorded feature set must override ambient Make environment settings.
    build.append("PAPER_FFTW_PREFIX="+(str(args.fftw_prefix) if args.fftw_prefix else ""))
    build.append("PAPER_VDSP="+str(int(args.enable_vdsp)))
    metadata = dict(schema=2, status="incomplete", started_utc=dt.datetime.now(dt.timezone.utc).isoformat(),
                    revision=capture(["git", "rev-parse", "HEAD"]),
                    git_status=capture(["git", "status", "--porcelain"]),
                    platform=platform.platform(), machine=platform.machine(), processor=platform.processor(),
                    python=platform.python_version(), cpu_count=os.cpu_count(),
                    compiler=capture(shlex.split(args.cxx)+["--version"]), build_command=build,
                    rack_dir=str(rack), seed=args.seed, repeats=args.repeats, notes=args.notes,
                    protocol="v2", configs=configs, runs=[], build_features=features,
                    external_dependency_sha256={name: digest(path) for name, path in external_inputs.items()})
    metadata["phase"] = phase
    metadata["session_id"] = args.session_id or "smoke"
    metadata["host_id"] = args.host_id or "unlabeled-smoke-host"
    metadata["campaign_manifest"] = manifest
    metadata["matrix_inventory"] = campaign_inventory(configs, registry)
    metadata["config_file_sha256"] = digest(args.config) if args.config else None
    metadata["contracts"] = {str(i): resolve_contract(c, registry) for i, c in enumerate(configs)}
    metadata["resources"] = {}
    if args.enable_vdsp:
        metadata["platform_framework"] = dict(name="Apple Accelerate/vDSP", binary_hash=None,
            limitation="System framework/dyld cache; exact per-instance OS and compile SDK identity in provider_info",
            SDKROOT=os.environ.get("SDKROOT"), default_xcrun_sdk={})
        for key, option in (("version", "--show-sdk-version"), ("build", "--show-sdk-build-version")):
            try:
                metadata["platform_framework"]["default_xcrun_sdk"][key] = capture(["xcrun", "--sdk", "macosx", option])
            except (OSError, subprocess.CalledProcessError):
                metadata["platform_framework"]["default_xcrun_sdk"][key] = "unavailable"
    if platform.system() == "Darwin":
        try:
            metadata["cpu_model"] = capture(["sysctl", "-n", "machdep.cpu.brand_string"])
        except subprocess.CalledProcessError as error:
            metadata["cpu_model"] = "unavailable: " + error.output.strip()
    elif Path("/proc/cpuinfo").exists():
        metadata["cpuinfo"] = Path("/proc/cpuinfo").read_text()
    save(output/"metadata.json", metadata)
    # Archive working sources, including uncommitted benchmark development.
    sources = sorted({p for base in (ROOT/"src", ROOT/"benchmark") for p in base.rglob("*")
                      if p.is_file() and "__pycache__" not in p.parts} |
                     {ROOT/"Makefile", ROOT/"plugin.json", *ROOT.glob("mk/*.mk")})
    # SDK headers/build rules and linked library affect generated code/behavior.
    sdk = sorted({p for base in (rack/"include", rack/"dep/include") for p in base.rglob("*") if p.is_file()} |
                 {p for p in rack.glob("*.mk")} | {p for p in rack.glob("libRack.*") if p.is_file()} |
                 {p for p in (rack/"dep/pffft").glob("pffft.[ch]") if p.is_file()})
    build_inputs = {p: digest(p) for p in sources+sdk+list(external_inputs.values())}
    with (output/"build.log").open("w") as log:
        subprocess.run(build, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
    if any(digest(p) != expected for p, expected in build_inputs.items()):
        raise ValueError("Source or dependency changed during build")
    audit_binary = BINARY.with_name("paper-audit" + BINARY.suffix)
    shutil.copy2(BINARY, output/"paper.bin")
    shutil.copy2(audit_binary, output/"paper-audit.bin")
    metadata["binary_sha256"] = digest(BINARY)
    metadata["audit_binary_sha256"] = digest(audit_binary)
    metadata["source_sha256"] = {str(p.relative_to(ROOT)): digest(p) for p in sources}
    with tarfile.open(output/"source.tar.gz", "w:gz") as archive:
        for path in sources:
            archive.add(path, arcname=str(path.relative_to(ROOT)))
    metadata["sdk_sha256"] = {str(p.relative_to(rack)): digest(p) for p in sdk}
    with tarfile.open(output/"dependencies.tar.gz", "w:gz", dereference=True) as archive:
        for path in sdk:
            archive.add(path, arcname=str(path.relative_to(rack)), recursive=False)
    if external_inputs:
        with tarfile.open(output/"external-dependencies.tar.gz", "w:gz", dereference=True) as archive:
            for name, path in external_inputs.items():
                archive.add(path, arcname=name, recursive=False)
    metadata["optional_build_provenance"] = "retained" if "fftw/provenance.json" in external_inputs else "not supplied"
    metadata["dependency_scope"] = "Rack headers, build rules and libRack bytes; system libraries identified by loader output and OS version"
    loader = (["otool", "-L", str(BINARY)] if platform.system() == "Darwin" else
              ["objdump", "-p", str(BINARY)] if os.name == "nt" else ["ldd", str(BINARY)])
    try:
        (output/"linked-libraries.txt").write_text(capture(loader)+"\n")
    except (OSError, subprocess.CalledProcessError) as error:
        raise RuntimeError("Cannot identify linked implementations") from error
    try:
        metadata["rack_revision"] = capture(["git", "rev-parse", "HEAD"], rack)
        metadata["pffft_revision"] = capture(["git", "rev-parse", "HEAD"], rack/"dep/pffft") if (rack/"dep/pffft/.git").exists() else "source revision unavailable in SDK"
        metadata["rack_status"] = capture(["git", "status", "--porcelain", "--untracked-files=no"], rack)
    except subprocess.CalledProcessError:
        metadata["rack_revision"] = "SDK without Git metadata"
    env = dict(os.environ, DYLD_LIBRARY_PATH=str(rack), LD_LIBRARY_PATH=str(rack))
    with (output/"verification.txt").open("w") as verification:
        subprocess.run([str(BINARY), "--verify"], cwd=ROOT, env=env,
                       stdout=verification, stderr=subprocess.STDOUT, check=True)
    compiled_registry = json.loads(subprocess.check_output([str(BINARY), "--inventory"], env=env, text=True))
    if compiled_registry != registry:
        raise ValueError("Compiled backend registry differs from runner")
    save(output/"inventory.json", compiled_registry)
    for index, config in enumerate(configs):
        arguments = command(config)[1:]
        contract = json.loads(subprocess.check_output([str(BINARY), "--describe"]+arguments, env=env, text=True))
        if contract != metadata["contracts"][str(index)]:
            raise ValueError("C++ and Python evidence contracts differ")
        resource = {}
        for label, executable in (("timing", BINARY), ("allocation", audit_binary)):
            resource[label] = json.loads(subprocess.check_output(
                [str(executable), "--resources"]+arguments, env=env, text=True))
        filename = f"resources-{index:04d}.json"
        save(output/filename, resource)
        metadata["resources"][str(index)] = filename
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
    if (digest(BINARY) != metadata["binary_sha256"]
            or digest(audit_binary) != metadata["audit_binary_sha256"]
            or any(digest(p) != metadata["source_sha256"][str(p.relative_to(ROOT))] for p in sources)
            or any(digest(p) != metadata["sdk_sha256"][str(p.relative_to(rack))] for p in sdk)
            or any(digest(path) != metadata["external_dependency_sha256"][name] for name, path in external_inputs.items())):
        raise ValueError("Source, executable or dependencies changed during campaign")
    metadata["status"] = "complete"
    metadata["finished_utc"] = dt.datetime.now(dt.timezone.utc).isoformat()
    metadata["artifact_sha256"] = {p.name: digest(p) for p in output.iterdir()
                                  if p.is_file() and p.name != "metadata.json"}
    save(output/"metadata.json", metadata)
    from check import check
    try:
        check(output)
    except Exception:
        metadata["status"] = "invalid"
        save(output/"metadata.json", metadata)
        raise


if __name__ == "__main__":
    main()

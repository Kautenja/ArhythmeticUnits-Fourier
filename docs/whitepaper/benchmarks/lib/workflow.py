"""User commands; timing stays in the existing C++ protocol and serial runner."""

# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import importlib.util
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import time
from paths import ROOT, BENCHMARKS
from contracts import load_registry
from campaigns import VARIANTS, inventory
from profiles import load, measured
from study import write_new, read_freeze, freeze, evidence
from run import digest


def providers(args):
    return VARIANTS[args.variant]["features"]


def preflight(args, build=False):
    rack = args.rack_dir.resolve()
    required = [rack / "include/rack.hpp", rack / "plugin.mk"]
    if not all(p.exists() for p in required):
        raise ValueError("Missing Rack SDK: set --rack-dir or RACK_DIR")
    if not shutil.which(args.cxx) or not shutil.which("make"):
        raise ValueError("A C++ compiler and Make are required")
    if args.variant == "macos" and platform.system() != "Darwin":
        raise ValueError("The macos variant requires Accelerate")
    if "fftw" in providers(args):
        from dependencies import fftw_inputs

        fftw_inputs(args.fftw_prefix.resolve())
    print(
        "Ready:",
        args.variant,
        "on",
        platform.system(),
        platform.machine(),
        "; SDK:",
        rack,
        flush=True,
    )
    print(
        "Optional tools:",
        {k: bool(shutil.which(k)) for k in ("latexmk", "pdflatex")},
        "; matplotlib:",
        bool(importlib.util.find_spec("matplotlib")),
        flush=True,
    )
    if build:
        command = [
            "make",
            "benchmark-paper-build",
            f"RACK_DIR={rack}",
            f"CXX={args.cxx}",
            "PAPER_VDSP=" + str(int("vdsp" in providers(args))),
            "PAPER_FFTW_PREFIX="
            + (str(args.fftw_prefix.resolve()) if "fftw" in providers(args) else ""),
        ]
        subprocess.run(command, cwd=ROOT, check=True)
        binary = (
            ROOT
            / ".build/benchmark/rack"
            / ("paper.exe" if os.name == "nt" else "paper")
        )
        subprocess.run(
            [str(binary), "--verify"],
            cwd=ROOT,
            check=True,
            env=dict(
                os.environ, DYLD_LIBRARY_PATH=str(rack), LD_LIBRARY_PATH=str(rack)
            ),
        )


def selection(args):
    profile, configs, manifest, options = load(
        args.profile, args.variant, platform.system()
    )
    for key in options:
        override = getattr(args, key, None)
        if override is not None:
            options[key] = override
    if (
        min(options["repeats"], options["frames"]) < 1
        or options["hops"] < 2
        or options["warm_hops"] < 0
    ):
        raise ValueError("Invalid repetition/duration/warmup counts")
    registry = load_registry(features=providers(args))
    return measured(configs, options, registry), manifest, options, registry


def run_study(args):
    frozen = read_freeze(args.freeze) if args.freeze else None
    if frozen:
        if any(
            getattr(args, k, None) is not None
            for k in ("repeats", "hops", "frames", "warm_hops", "seed")
        ):
            raise ValueError(
                "Frozen options cannot be overridden; create a new pilot-backed freeze"
            )
        if frozen["variant"] != args.variant:
            raise ValueError("Variant differs from freeze")
        from study import seed_for_session
        if frozen.get('fixture'): raise ValueError('Cannot launch a synthetic freeze')
        configs, options, phase = frozen['configs'], dict(frozen['options'], seed=seed_for_session(frozen, args.session)), 'confirmation'
    else:
        configs, manifest, options, _ = selection(args)
        phase = manifest["phase"]
    if phase != "smoke" and (not args.host or not args.session or not args.notes):
        raise ValueError(
            "Pilot/confirmation require --host, --session, and actual --notes"
        )
    if args.output.exists():
        raise ValueError("Evidence is immutable; restart in a new output directory")
    destination = args.output.resolve()
    if any(
        destination == p or p in destination.parents
        for p in (
            ROOT / "src",
            ROOT / "benchmark",
            ROOT / "docs/whitepaper",
            ROOT / ".git",
        )
    ):
        raise ValueError(
            "Keep campaign output outside source, manuscript and Git directories"
        )
    preflight(args)
    # Retained launch input sits beside the campaign; it never changes during a run.
    config_path = args.output.resolve().with_name(args.output.name + ".input.json")
    write_new(
        config_path,
        [
            {k: v for k, v in c.items() if k not in ("callbacks", "warm_hops")}
            for c in configs
        ],
    )
    command = [
        sys.executable,
        str(BENCHMARKS / "lib/run.py"),
        str(args.output),
        "--config",
        str(config_path),
        "--phase",
        phase,
        "--rack-dir",
        str(args.rack_dir),
        "--cxx",
        args.cxx,
        "--host-id",
        args.host or "smoke-host",
        "--session-id",
        args.session or "smoke",
        "--notes",
        args.notes,
    ]
    for k, v in options.items():
        command += ["--" + k.replace("_", "-"), str(v)]
    for key in ("execution_regime", "session_settle_seconds", "process_settle_ms", "throughput_chunks",
                "thread_policy", "fpu_policy"):
        if hasattr(args, key):
            command += ["--" + key.replace("_", "-"), str(getattr(args, key))]
    if frozen:
        command += ["--freeze", str(args.freeze)]
    if "vdsp" in providers(args):
        command += ["--enable-vdsp"]
    if "fftw" in providers(args):
        command += ["--fftw-prefix", str(args.fftw_prefix)]
    print(
        "Launching",
        len(configs) * options["repeats"],
        "serial processes;",
        phase.upper(),
        flush=True,
    )
    subprocess.run(command, cwd=ROOT, check=True)


def status(path, logs=False):
    m = json.loads((path / "metadata.json").read_text())
    total = len(m.get("configs", [])) * m.get("repeats", 0)
    print(
        json.dumps(
            dict(
                status=m.get("status"),
                phase=m.get("phase"),
                host=m.get("host_id"),
                session=m.get("session_id"),
                completed=len(m.get("runs", [])),
                total=total,
                failed=int(m.get("status") == "invalid"),
                active=m.get("active_job"),
                failure=m.get("failure"),
            ),
            indent=2,
        )
    )
    if logs:
        candidates = [path / "build.log", path / "verification.txt"]
        if m.get("active_job"):
            candidates.append(path / m["active_job"]["stderr"])
        for p in candidates:
            if p.exists():
                print(
                    str(p)
                    + ":\n"
                    + "\n".join(p.read_text(errors="replace").splitlines()[-8:])
                )
    return m.get("status")


def estimate(paths):
    for path in paths:
        m = evidence(path)
        raw = sum((path / r["raw"]).stat().st_size for r in m["runs"])
        count = len(m["runs"])
        elapsed = m.get("runtime", {}).get("phases_ns", {})
        windows = [
            g
            for r in m["runs"]
            for k, g in r["summary"]["groups"].items()
            if k != "timer"
        ]
        print(
            json.dumps(
                dict(
                    campaign=str(path),
                    processes=count,
                    raw_bytes=raw,
                    bytes_per_process=raw / count,
                    process_wall_seconds=elapsed.get("benchmark_process", 0) / 1e9,
                    fixed_build_seconds=elapsed.get("build", 0) / 1e9,
                    observations_min=min(g["observations"] for g in windows),
                    caution="Scale process and byte rates only for matching workloads/durations. FFTW planning, replay and tails may scale nonlinearly; reserve extra disk.",
                ),
                indent=2,
            )
        )


def main(argv=None):
    arguments = sys.argv[1:] if argv is None else argv
    if arguments and arguments[0] in ('study-prepare', 'study-run', 'study-check'):
        from offline import main as offline_main
        try:
            offline_main([arguments[0][6:]]+arguments[1:])
            return 0
        except KeyboardInterrupt:
            print('Interrupted; partial results are retained.', file=sys.stderr)
            return 130
        except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as error:
            print('Error:', error, file=sys.stderr)
            return 1
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)

    def host(p):
        p.add_argument("--variant", choices=VARIANTS, default="rack")
        p.add_argument(
            "--rack-dir",
            type=Path,
            default=Path(os.environ.get("RACK_DIR", ROOT / "../..")),
        )
        p.add_argument("--fftw-prefix", type=Path, default=ROOT / ".build/deps/fftw")
        p.add_argument("--cxx", default=os.environ.get("CXX", "c++"))

    def profile(p):
        host(p)
        p.add_argument(
            "--profile",
            default="smoke",
            help="smoke, pilot, extensions, or a version-2 JSON profile",
        )
        for key in ("repeats", "hops", "frames", "warm_hops", "seed"):
            p.add_argument("--" + key.replace("_", "-"), type=int)

    p = commands.add_parser(
        "setup", help="Check prerequisites; optionally build and verify"
    )
    host(p)
    p.add_argument("--build", action="store_true")
    p = commands.add_parser(
        "inventory", help="List all backend capabilities and unavailable providers"
    )
    host(p)
    p = commands.add_parser(
        "dependency", help="Build the pinned optional FFTW dependency"
    )
    p.add_argument("name", choices=["fftw"])
    p.add_argument("--prefix", type=Path, default=ROOT / ".build/deps/fftw")
    p.add_argument("--jobs", type=int, default=2)
    p = commands.add_parser(
        "plan", help="Resolve a profile without building or measuring"
    )
    profile(p)
    p.add_argument("--output", type=Path)
    p = commands.add_parser("run", help="Launch one serial smoke/pilot/frozen session")
    profile(p)
    from execution import add_arguments
    add_arguments(p)
    p.add_argument("--freeze", type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--host", default="")
    p.add_argument("--session", default="")
    p.add_argument("--notes", default="")
    p = commands.add_parser(
        "freeze", help="Freeze pilot-backed workloads and recording policy"
    )
    p.add_argument("--session-seeds", type=Path, help="JSON mapping of predeclared confirmation session labels to distinct seeds")
    profile(p)
    p.add_argument("pilots", nargs="+", type=Path)
    p.add_argument("--rationale", required=True)
    p.add_argument("--output", required=True, type=Path)
    p = commands.add_parser(
        "status", help="Show progress/failure and retained log tails"
    )
    p.add_argument("campaign", type=Path)
    p.add_argument("--logs", action="store_true")
    p.add_argument("--follow", action="store_true")
    for name in ("check", "estimate"):
        p = commands.add_parser(name)
        p.add_argument("campaigns", nargs="+", type=Path)
    p = commands.add_parser("runtime")
    p.add_argument("campaign", type=Path)
    p = commands.add_parser("report", help="Regenerate checked tables and figures")
    p.add_argument("campaigns", nargs="+", type=Path)
    p.add_argument("--phase", required=True, choices=["smoke", "pilot", "confirmation"])
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--no-plots", action="store_true")
    p = commands.add_parser(
        "select", help="Write a versioned selection for reviewed report rows"
    )
    p.add_argument("report", type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--fixture", action="store_true")
    p = commands.add_parser(
        "export", help="Generate checked LaTeX includes and figures"
    )
    p.add_argument("selection", type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--fixture", action="store_true")
    p = commands.add_parser("check-export")
    p.add_argument("output", type=Path)
    p = commands.add_parser(
        "bundle", help="Package portable audit evidence, omitting SDK/native binaries"
    )
    p.add_argument("campaigns", nargs="+", type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--selection", type=Path)
    p = commands.add_parser("unpack")
    p.add_argument("bundle", type=Path)
    p.add_argument("--output", required=True, type=Path)
    p = commands.add_parser(
        "retire-plan", help="Inventory exact candidates without deleting"
    )
    p.add_argument("candidates", nargs="+", type=Path)
    p.add_argument("--output", required=True, type=Path)
    p = commands.add_parser(
        "retire", help="Preview guarded retirement; --execute applies an eligible plan"
    )
    p.add_argument("plan", type=Path)
    p.add_argument("--replacement", required=True, type=Path)
    p.add_argument("--publication", required=True, type=Path)
    p.add_argument("--receipt", required=True, type=Path)
    p.add_argument("--execute", action="store_true")
    args = parser.parse_args(argv)
    try:
        c = args.command
        if c == "setup":
            preflight(args, args.build)
        elif c == "inventory":
            print(
                json.dumps(
                    load_registry(features=providers(args)), indent=2, sort_keys=True
                )
            )
        elif c == "dependency":
            subprocess.run(
                [
                    sys.executable,
                    str(BENCHMARKS / "lib/build_fftw.py"),
                    "--prefix",
                    str(args.prefix),
                    "--jobs",
                    str(args.jobs),
                ],
                check=True,
            )
        elif c == "plan":
            configs, manifest, options, registry = selection(args)
            value = dict(
                schema=1,
                phase=manifest["phase"],
                variant=args.variant,
                options=options,
                omitted_by_provider=manifest["omitted_by_provider"],
                inventory=inventory(configs, registry),
                configs=configs,
            )
            if args.output:
                write_new(args.output, value)
            print(
                json.dumps(
                    dict(
                        workloads=len(configs),
                        processes=len(configs) * options["repeats"],
                        phase=manifest["phase"],
                        by_backend=value["inventory"]["by_backend"],
                        output=str(args.output) if args.output else None,
                    ),
                    indent=2,
                )
            )
        elif c == "run":
            run_study(args)
        elif c == "freeze":
            configs, _, options, _ = selection(args)
            value = freeze(
                args.pilots, configs, options, args.variant, args.rationale, args.output,
                session_seed_policy=(dict(schema=1, kind='predeclared-session-seeds-v1',
                                          sessions=json.loads(args.session_seeds.read_text())) if args.session_seeds else None)
            )
            print(
                "Frozen:",
                value["freeze_id"],
                "; collect at least three actual independent sessions",
            )
        elif c == "status":
            while True:
                state = status(args.campaign, args.logs)
                if not args.follow or state in ("complete", "invalid"):
                    break
                time.sleep(2)
        elif c == "check":
            from check import check

            for path in args.campaigns:
                print("Verified", check(path), "runs:", path)
        elif c == "estimate":
            estimate(args.campaigns)
        elif c == "runtime":
            from runtime import format_runtime

            print(format_runtime(args.campaign))
        elif c == "report":
            from reporting import derive

            print(
                "Wrote",
                derive(args.campaigns, args.output, args.phase, not args.no_plots),
                "checked workloads",
            )
        elif c == "select":
            from publication import selection_manifest

            selection_manifest(args.report, args.output, args.fixture)
            print("Selection written:", args.output)
        elif c == "export":
            from publication import export

            export(args.selection, args.output, args.fixture)
            print("Export verified:", args.output)
        elif c == "check-export":
            from publication import verify_export

            verify_export(args.output)
            print("Fresh publication assets:", args.output)
        elif c == "bundle":
            from bundles import pack

            pack(args.campaigns, args.output, args.selection)
            print("Portable audit bundle:", args.output)
        elif c == "unpack":
            from bundles import unpack

            unpack(args.bundle, args.output)
            print("Extracted and verified:", args.output)
        elif c == "retire-plan":
            from retirement import inventory_plan

            inventory_plan(args.candidates, args.output)
            print("Preview only:", args.output)
        elif c == "retire":
            from retirement import retire

            print(
                json.dumps(
                    retire(
                        args.plan,
                        args.replacement,
                        args.publication,
                        args.receipt,
                        args.execute,
                    ),
                    indent=2,
                )
            )
        return 0
    except KeyboardInterrupt:
        print(
            "Interrupted; preserve the partial directory and restart under a new name.",
            file=sys.stderr,
        )
        return 130
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as error:
        print("Error:", error, file=sys.stderr)
        return 1

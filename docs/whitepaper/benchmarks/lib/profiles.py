"""Small versioned study profiles; expanded configurations are generated artifacts."""

# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
from paths import BENCHMARKS, HISTORY
from campaigns import VARIANTS, document, names, resolve
from contracts import load_registry, validate_config
from run import BASE


def load(profile, variant, system):
    path = (
        Path(profile)
        if str(profile).endswith(".json")
        else BENCHMARKS / "profiles" / (profile + ".json")
    )
    value = json.loads(path.read_text())
    if value.get("schema") not in (2, 3) or set(value) - {
        "schema",
        "preset",
        "transitions",
        "seed",
        "repeats",
        "hops",
        "frames",
        "warm_hops",
        "workloads",
    }:
        raise ValueError("Expected a version-2 or version-3 study profile; see profiles/README.md")
    if type(value.get("transitions", True)) is not bool or not isinstance(
        value.get("workloads", []), list
    ):
        raise ValueError("transitions must be boolean and workloads must be a list")
    preset = value.get("preset", "explicit" if value["schema"] == 3 else None)
    if preset not in (("smoke", "pilot", "extensions", "explicit") if value["schema"] == 3 else ("smoke", "pilot", "extensions")) or variant not in VARIANTS:
        raise ValueError("Unknown preset or provider variant")
    features = VARIANTS[variant]["features"]
    registry = load_registry(features=features)
    if preset == "explicit":
        if value.get("transitions", False) or not value.get("workloads"):
            raise ValueError("Explicit profiles require workloads and cannot append transitions")
        rows, phase = [], "pilot"
    elif preset == "smoke":
        rows = []
        for precision in ("float", "double"):
            for family in ("core", "legacy-batch", "legacy-incremental"):
                backend = family + "-" + precision
                rows += [
                    dict(backend=backend, n=128, hop=37, block=16, state="startup"),
                    dict(
                        backend=backend,
                        n=128,
                        hop=37,
                        block=16,
                        state="live",
                        smooth=1,
                        count=3,
                        alignment="staggered",
                        callback_offset=5,
                        pass_name="throughput",
                    ),
                ]
        for backend in (
            "pffft-analysis-float",
            "pffft-hybrid-float",
            "pffft-scheduled-batch-float",
            "core-independent4-float",
            "core-independent4-simd",
            "pffft-analysis4-float",
            "fftw-analysis-float",
            "fftw-analysis-double",
            "vdsp-analysis-float",
            "vdsp-analysis-double",
        ):
            rows.append(dict(backend=backend, n=128, hop=37, block=16, smooth=1))
        # Exercise each inverse, complete-chain and transform provider once.
        # The pilot covers duration and scheduling factors; smoke checks wiring.
        for precision in ("float", "double"):
            _, streams, transforms = names(precision)
            for backend in streams:
                rows.append(dict(backend=backend, n=128, hop=37, block=16))
            for backend in transforms:
                rows.append(dict(backend=backend, n=128, hop=37, pass_name="complete"))
        if value.get("transitions", True):
            for backend in (
                "core-float",
                "core-double",
                "pffft-hybrid-float",
                "pffft-scheduled-batch-float",
            ):
                for control in (False, True):
                    rows.append(
                        dict(
                            backend=backend,
                            n=128,
                            hop=37,
                            block=16,
                            state="startup",
                            transition_suite="interactive-v1",
                            transition_control=control,
                        )
                    )
            for backend in ("core-float", "pffft-hybrid-float"):
                rows.append(
                    dict(
                        backend=backend,
                        n=2048,
                        hop=1024,
                        block=64,
                        state="startup",
                        transition_suite="interactive-v1",
                        transition_control=False,
                    )
                )
        phase = "smoke"
    else:
        rows = document(preset)["workloads"]
        if value.get("transitions", True):
            # Same interactive request matrix, separate from stationary factors.
            for n, h in ((128, 37), (2048, 1024)):
                for backend in (
                    "core-float",
                    "core-double",
                    "pffft-hybrid-float",
                    "pffft-scheduled-batch-float",
                ):
                    for control in (False, True):
                        rows.append(
                            dict(
                                backend=backend,
                                n=n,
                                hop=h,
                                block=64,
                                state="startup",
                                transition_suite="interactive-v1",
                                transition_control=control,
                            )
                        )
        phase = "pilot"
    rows += value.get("workloads", [])
    # Historical schema-2 presets excluded Rack modules. Explicit v3 profiles
    # can select these already-linked workloads without changing old matrices.
    variants = ({name: dict(v, providers=v["providers"]+["rack-module"]) for name, v in VARIANTS.items()}
                if value["schema"] == 3 else VARIANTS)
    configs, manifest = resolve(
        dict(schema=1, phase=phase, variants=variants, workloads=rows),
        variant,
        registry,
        system,
        BASE,
    )
    options = {k: value[k] for k in ("seed", "repeats", "hops", "frames", "warm_hops")}
    if (
        any(
            type(v) is not int or v < (0 if k in ("seed", "warm_hops") else 1)
            for k, v in options.items()
        )
        or options["hops"] < 2
    ):
        raise ValueError("Invalid profile counts")
    return value, configs, manifest, options


def measured(configs, options, registry):
    import math

    result = []
    for c in configs:
        c = dict(c)
        c["warm_hops"] = 0 if c.get("transition_suite") else options["warm_hops"]
        c["callbacks"] = (
            math.ceil(
                max(options["hops"], 26 if c.get("transition_suite") else 0)
                * c["hop"]
                / c["block"]
            )
            if c["pass_name"] in ("callback", "throughput")
            else options["frames"]
        )
        validate_config(c, registry, measurement=True)
        result.append(c)
    if len({json.dumps(c, sort_keys=True) for c in result}) != len(result):
        raise ValueError("Duplicate profile workload")
    return result

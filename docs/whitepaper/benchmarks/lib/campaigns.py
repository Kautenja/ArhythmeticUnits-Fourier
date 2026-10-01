#!/usr/bin/env python3
"""Deterministic, explicit host variants for the external comparison campaigns."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
from paths import ROOT, BENCHMARKS, HISTORY

from collections import Counter
import json
from pathlib import Path

from workloads import FIELDS, expand
from contracts import validate_config, resolve_contract

VARIANTS = {
    "rack": dict(systems=["Darwin", "Linux", "Windows"], features=[], providers=["fourier", "pffft"]),
    "portable": dict(systems=["Darwin", "Linux", "Windows"], features=["fftw"], providers=["fourier", "pffft", "fftw"]),
    "macos": dict(systems=["Darwin"], features=["fftw", "vdsp"], providers=["fourier", "pffft", "fftw", "vdsp"]),
}


def names(precision):
    native = ["pffft", "fftw", "vdsp"] if precision == "float" else ["fftw", "vdsp"]
    analysis = [f"core-{precision}", f"legacy-batch-{precision}", f"legacy-incremental-{precision}"]
    analysis += [f"{p}-analysis-{precision}" for p in native]
    if precision == "float":
        analysis += ["pffft-scheduled-batch-float", "pffft-hybrid-float"]
    streams = []
    for family, suffix in (("inverse-stream", "inverse"), ("ols-identity", "ols-identity"), ("ols-fir", "ols-fir")):
        streams += [f"{family}-{mode}-{precision}" for mode in ("batch", "incremental")]
        streams += [f"{p}-{suffix}-{precision}" for p in native]
    transforms = [f"{op}-{precision}" for op in ("fft", "rfft", "ifft")]
    transforms += [f"{p}-{op}-{precision}" for p in native for op in ("fft", "rfft", "ifft")]
    return analysis, streams, transforms


CHANNELS = ["core-independent4-float", "core-independent4-simd", "pffft-analysis4-float",
            "fftw-analysis4-float", "vdsp-analysis4-float"]


def document(phase):
    rows = []
    def add(backends, **config):
        rows.extend(dict(config, backend=name) for name in backends)
    if phase == "smoke":
        for precision in ("float", "double"):
            analysis, streams, transforms = names(precision)
            for mode in ("callback", "throughput"):
                add(analysis+streams+(CHANNELS if precision == "float" else []), n=128, hop=37, pass_name=mode)
            add(transforms, n=128, hop=37, pass_name="complete")
        analysis, streams, _ = names("float")
        for changes in (dict(callback_offset=17), dict(state="startup"),
                        dict(count=4, alignment="staggered", load=8, cache_mib=1, callback_offset=9)):
            add(analysis+streams+CHANNELS, n=128, hop=37, **changes)
        add(analysis+CHANNELS, n=128, hop=37, state="live", smooth=1)
        add(analysis+CHANNELS, n=2048, hop=257, smooth=1)
        add(analysis+streams, n=2048, hop=37)
        add(names("float")[2], n=2048, hop=37, pass_name="complete")
    elif phase == "pilot":
        analysis, streams, transforms = names("float")
        for n in (2048, 4096, 16384):
            for smooth in (0, 1):
                for block in (16, 64, 256):
                    add(analysis, n=n, hop=1024, block=block, smooth=smooth)
                add(analysis, n=n, hop=1024, smooth=smooth, pass_name="throughput")
            for block in (16, 64, 256):
                add(streams, n=n, hop=1024, block=block)
            add(streams, n=n, hop=1024, pass_name="throughput")
            add(transforms, n=n, hop=1024, pass_name="complete")
    elif phase == "extensions":
        analysis, streams, _ = names("float")
        changes = [dict(n=128, hop=37), dict(hop=257), dict(rate=96000), dict(load=64),
                   dict(state="startup"), dict(cache_mib=32), dict(block=1)]
        changes += [dict(count=count, alignment=alignment) for count in (4, 16) for alignment in ("aligned", "staggered")]
        for change in changes:
            for mode in ("callback", "throughput"):
                add(analysis+streams, **dict(dict(n=2048, hop=1024, pass_name=mode), **change))
        for offset in (1, 15, 63):
            add(analysis+streams, n=2048, hop=1024, callback_offset=offset)
        add(analysis, n=2048, hop=257, state="live", smooth=1)
        for n in (128, 2048, 4096, 16384):
            hop = 37 if n == 128 else 1024
            for smooth in (0, 1):
                for mode in ("callback", "throughput"):
                    add(CHANNELS, n=n, hop=hop, smooth=smooth, pass_name=mode)
            double_analysis, double_streams, transforms = names("double")
            for mode in ("callback", "throughput"):
                add(double_analysis+double_streams, n=n, hop=hop, pass_name=mode)
                add(double_analysis, n=n, hop=hop, smooth=1, pass_name=mode)
            add(transforms, n=n, hop=hop, pass_name="complete")
    else:
        raise ValueError("Unknown campaign phase")
    return dict(schema=1, phase=phase, variants=VARIANTS,
                workloads=list({json.dumps(r, sort_keys=True): r for r in rows}.values()))


def resolve(document, variant, registry, system, base):
    if document.get("schema") != 1 or variant not in document["variants"]:
        raise ValueError("A campaign manifest requires an explicit supported --variant")
    selected = document["variants"][variant]
    if system not in selected["systems"]:
        raise ValueError("Campaign variant is unavailable on this operating system")
    for feature in selected["features"]:
        if not any(d["build_feature"] == feature and d["available"] for d in registry.values()):
            raise ValueError("Campaign requires optional feature: "+feature)
    rows, omitted = [], Counter()
    for partial in document["workloads"]:
        if partial.keys()-(base.keys() | FIELDS | {"transition_suite", "transition_control"}):
            raise ValueError("Unknown campaign workload field")
        config = expand(dict(base, **partial))
        descriptor = registry[config["backend"]]
        if descriptor["provider"] not in selected["providers"]:
            omitted[descriptor["provider"]] += 1
            continue
        validate_config(config, registry)
        rows.append(config)
    return rows, dict(phase=document["phase"], variant=variant, omitted_by_provider=dict(omitted))


def inventory(configs, registry):
    return dict(workloads=len(configs), by_backend=dict(Counter(c["backend"] for c in configs)),
                by_boundary=dict(Counter(registry[c["backend"]]["boundary"] for c in configs)),
                by_precision=dict(Counter(registry[c["backend"]]["precision"] for c in configs)),
                by_independent_channels=dict(Counter(str((c["active_ports"] if c.get("workload_schema") == 3 else registry[c["backend"]]["channels"])*c["count"]) for c in configs)),
                contracts=[resolve_contract(c, registry) for c in configs])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    for phase in ("smoke", "pilot", "extensions"):
        path = (HISTORY/"configs")/("external-"+phase+".json")
        expected = json.dumps(document(phase), indent=2)+"\n"
        if args.check:
            if path.read_text() != expected:
                raise SystemExit("Stale campaign manifest: "+str(path))
        else:
            path.write_text(expected)

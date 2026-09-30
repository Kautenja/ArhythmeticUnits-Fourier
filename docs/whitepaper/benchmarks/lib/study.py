"""Freeze and verify a study without treating smoke timings as paper evidence."""

# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
from run import digest, save
from report import identity
from check import check

POLICIES = dict(
    analysis_accuracy_policy="spectrum-norms-v1",
    scalar_analysis_audit_policy="all-publications-v1",
    transition_policy="fourier-transitions-v1",
)
IDENTITY = (
    "source_sha256",
    "sdk_sha256",
    "external_dependency_sha256",
    "compiler",
    "compile_commands",
    "build_features",
    "platform",
    "machine",
    "cpu_model",
    "platform_framework",
)


def evidence(path, phase=None):
    check(path)
    m = json.loads((path / "metadata.json").read_text())
    if phase and m.get("phase") != phase:
        raise ValueError("Evidence phase differs from requested phase")
    for k, v in POLICIES.items():
        if k == "transition_policy" and not any(
            c.get("transition_suite") for c in m["configs"]
        ):
            continue
        if m.get(k) != v:
            raise ValueError(
                "Replacement evidence lacks current numerical/transition coverage"
            )
    return m


def freeze(pilots, configs, options, variant, rationale, output):
    if not rationale.strip():
        raise ValueError(
            "Freeze requires a pilot interpretation and inclusion rationale"
        )
    metadata = [evidence(p, "pilot") for p in pilots]
    identities = [{k: m.get(k) for k in IDENTITY} for m in metadata]
    if any(i != identities[0] for i in identities):
        raise ValueError("Pilot source/dependency/compiler identities differ")
    if len({(m["host_id"], m["session_id"]) for m in metadata}) != len(metadata):
        raise ValueError("Duplicate pilot session label")

    # Durations may change; arbitrary new workloads require a new pilot.
    def key(c):
        return identity(
            {k: v for k, v in c.items() if k not in ("callbacks", "warm_hops")}
        )

    available = {key(c) for m in metadata for c in m["configs"]}
    if any(key(c) not in available for c in configs):
        raise ValueError("Confirmation workload has no pilot coverage")
    value = dict(
        schema=1,
        kind="fourier-freeze-v1",
        variant=variant,
        configs=configs,
        options=options,
        provenance=identities[0],
        policies=POLICIES,
        minimum_sessions=3,
        rationale=rationale,
        pilots=[
            dict(
                metadata_sha256=digest(p / "metadata.json"),
                host=m["host_id"],
                session=m["session_id"],
            )
            for p, m in zip(pilots, metadata)
        ],
    )
    value["freeze_id"] = identity(value)
    write_new(output, value)
    return value


def read_freeze(path):
    f = json.loads(path.read_text())
    unsigned = {k: v for k, v in f.items() if k != "freeze_id"}
    if (
        f.get("kind") != "fourier-freeze-v1"
        or f.get("schema") != 1
        or f.get("freeze_id") != identity(unsigned)
    ):
        raise ValueError("Altered or unsupported freeze manifest")
    if (
        f.get("policies") != POLICIES
        or f.get("minimum_sessions", 0) < 3
        or not f.get("rationale")
    ):
        raise ValueError("Missing freeze policy/session floor/rationale")
    return f


def enforce(f, metadata):
    if (
        metadata["configs"] != f["configs"]
        or metadata["repeats"] != f["options"]["repeats"]
        or metadata["seed"] != f["options"]["seed"]
    ):
        raise ValueError("Run differs from frozen workload/order/repetitions")
    if {k: metadata.get(k) for k in IDENTITY} != f["provenance"]:
        raise ValueError(
            "Source, dependency, compiler or provider identity changed since pilot freeze"
        )


def confirmations(paths):
    records = [evidence(p, "confirmation") for p in paths]
    freezes = []
    for m in records:
        f = m.get("study_freeze")
        if not f:
            raise ValueError("Confirmation has no verified study freeze")
        unsigned = {k: v for k, v in f.items() if k != "freeze_id"}
        if f.get("freeze_id") != identity(unsigned) or f.get("policies") != POLICIES:
            raise ValueError("Invalid embedded freeze")
        enforce(f, m)
        freezes.append(f["freeze_id"])
    if len(set(freezes)) != 1:
        raise ValueError(
            "Do not pool different frozen studies in one publication selection"
        )
    if len({m["host_id"] for m in records}) != 1:
        raise ValueError("Select one host/stratum at a time")
    labels = [m["session_id"] for m in records]
    if len(labels) != len(set(labels)) or len(labels) < max(
        3, records[0]["study_freeze"]["minimum_sessions"]
    ):
        raise ValueError(
            "Confirmation requires at least three separately labeled actual sessions"
        )
    return records


def write_new(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x") as stream:
        stream.write(json.dumps(value, indent=2, sort_keys=True) + "\n")

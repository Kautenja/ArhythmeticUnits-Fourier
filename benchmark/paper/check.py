#!/usr/bin/env python3
"""Verify a completed paper campaign without trusting its stored summaries."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import csv
import math
from collections import Counter
import hashlib
import json
from pathlib import Path
import tarfile

from run import digest, summarize
from contracts import normalize_registry, resolve_contract, validate_config


def validate_rows(path, config, registry=None):
    """Check observation counts and publication age/cadence from raw records."""
    counts = Counter()
    publications = Counter()
    previous = {}
    mode = config["pass_name"]
    contract = resolve_contract(config, registry)
    n, hop, block = config["n"], config["hop"], config["block"]
    delay = contract["publication_delay_samples"]
    center_offset = contract["center_offset_samples"]
    playback_delay = contract["playback_delay_samples"]
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream):
            if not math.isfinite(float(row["ns"])) or float(row["ns"]) < 0:
                raise ValueError("Invalid timing observation")
            counts[row["kind"]] += 1
            if row["kind"] != "publication":
                continue
            analyzer, sample = int(row["analyzer"]), int(row["sample"])
            offset = config.get("callback_offset", 0) + (analyzer*hop//config["count"] if config["alignment"] == "staggered" else 0)
            if not 0 <= analyzer < config["count"] or not 0 <= sample < config["callbacks"]*block:
                raise ValueError("Publication index outside workload")
            if (sample+offset)%hop != delay or (analyzer in previous and sample-previous[analyzer] != hop):
                raise ValueError("Publication cadence mismatch")
            ages = (float(row["endpoint_age_samples"]), float(row["center_age_samples"]),
                    float(row["callback_visible_age_samples"]))
            if ages != (delay, delay+center_offset, delay+block-1-sample%block):
                raise ValueError("Publication age mismatch")
            if contract["boundary"] in ("inverse-job", "chain") and float(row.get("playback_delay_samples", "nan")) != playback_delay:
                raise ValueError("Playback delay mismatch")
            previous[analyzer] = sample
            publications[analyzer] += 1
    expected = Counter(timer=1024)
    frames = config["callbacks"]
    if mode in ("callback", "throughput"):
        expected[mode] = frames if mode == "callback" else 1
        if contract["boundary"] != "control":
            for analyzer in range(config["count"]):
                offset = config.get("callback_offset", 0) + (analyzer*hop//config["count"] if config["alignment"] == "staggered" else 0)
                bias = hop-1-delay
                count = (frames*block+offset+bias)//hop-(offset+bias)//hop
                if publications[analyzer] != count:
                    raise ValueError("Missing publication records")
                expected["publication"] += count
    elif mode in ("complete", "incremental"):
        expected[mode] = frames
    else:
        real, inverse = contract["step_model"] == "radix2-real", contract["step_model"] == "radix2-inverse"
        butterflies = n//4*((n//2).bit_length()-1) if real else n//2*(n.bit_length()-1)
        expected["buffer"] = frames
        expected["butterfly_step" if mode == "steps" else "butterflies"] = frames*(butterflies-int(real) if mode == "steps" else 1)
        if real:
            expected["reconstruct_step" if mode == "steps" else "last_butterfly_and_reconstruction"] = frames
        if inverse:
            expected["normalize_step" if mode == "steps" else "normalization"] = frames*(n if mode == "steps" else 1)
    if counts != expected:
        raise ValueError(f"Observation count mismatch: {counts} != {expected}")


def validate_synthesis_accuracy(accuracy, config, publications, registry=None):
    """Reject missing, truncated, or numerically invalid full-output audits."""
    contract = resolve_contract(config, registry)
    playback = (config["callbacks"]*config["block"]*config["count"]
                if contract["boundary"] == "chain" else 0)
    expected = playback + publications*contract["outputs_per_channel"]*contract["channels"]
    tolerance = (3e-4 if contract["boundary"] == "analysis" else 2e-5) if contract["precision"] == "float" else 1e-10
    error, scale = accuracy["max_abs_error"], accuracy["max_reference"]
    if (not all(math.isfinite(v) and v >= 0 for v in (error, scale))
            or error > tolerance*max(1, scale)
            or accuracy["checked_samples"] != expected
            or accuracy["playback_checked_samples"] != playback
            or accuracy["publications"] != publications or not publications):
        raise ValueError("Invalid synthesis numerical report")


def validate_resources(resource):
    """Allocation observations are from a separate executable, not timed evidence."""
    for label, instrumented in (("timing", False), ("allocation", True)):
        item = resource[label]
        if item["schema"] != 1 or item["instrumented"] != instrumented or not item["unknown_reason"]:
            raise ValueError("Invalid resource audit identity")
        if item["native_allocation_bytes"] is not None or item["stack_scratch_bytes"] is not None:
            raise ValueError("Unsupported native storage claim")
        if item["object_bytes"] <= 0 or item["operations"] <= 0:
            raise ValueError("Empty resource audit")
        for name in ("setup", "execution", "destruction"):
            phase = item[name]
            if not math.isfinite(phase["ns"]) or phase["ns"] < 0:
                raise ValueError("Invalid resource timing")
            for key in ("allocations", "allocated_bytes", "live_bytes", "peak_bytes"):
                value = phase[key]
                if (instrumented and (type(value) is not int or value < 0)) or (not instrumented and value is not None):
                    raise ValueError("Invalid allocation observation")
            if instrumented and phase["peak_bytes"] < phase["live_bytes"]:
                raise ValueError("Invalid peak storage")
    if resource["timing"]["object_bytes"] != resource["allocation"]["object_bytes"]:
        raise ValueError("Resource adapter mismatch")


def validate_provider_info(info, descriptor):
    if descriptor["kind"] == "analysis4":
        simd = descriptor["id"] == "core-independent4-simd"
        native = info.get("native_instances", [])
        if (info.get("input_contract") != "independent-four-v1"
                or info.get("scalar_instances") != (0 if simd else 4) or len(native) != (0 if simd else 4)):
            raise ValueError("Missing independent four-channel evidence")
        if descriptor["provider"] != "fourier":
            for child in native:
                validate_provider_info(child, dict(descriptor, kind="external", channels=1))
        elif any(child is not None for child in native):
            raise ValueError("Unexpected first-party native provider")
        return
    expected = {"pffft": "rack-pffft", "fftw": "fftw", "vdsp": "Apple Accelerate/vDSP"}
    if not isinstance(info, dict) or info.get("provider") != expected[descriptor["provider"]]:
        raise ValueError("Missing or wrong native provider identity")
    if info.get("precision") != descriptor["precision"]:
        raise ValueError("Native provider precision mismatch")
    if descriptor["provider"] == "pffft" and (not info.get("plan_policy") or "native_plan_bytes" not in info):
        raise ValueError("Missing native setup/storage evidence")
    if descriptor["provider"] == "fftw":
        if (info.get("threads") != 1 or info.get("plan_policy") != "FFTW_MEASURE"
                or info.get("imported_wisdom") is not False or not info.get("version")
                or not info.get("exported_wisdom")):
            raise ValueError("Invalid FFTW plan policy/provenance")
        operation = descriptor["operation"]
        plans = (["real_plan"] if operation in ("analysis", "rfft") else ["forward_plan"]
                 if operation == "fft" else ["inverse_plan"] if operation in ("ifft", "inverse")
                 else ["forward_plan", "inverse_plan"])
        if any(not info.get(plan) for plan in plans):
            raise ValueError("Missing measured FFTW plan")

    if descriptor["provider"] == "vdsp":
        if (info.get("setup_count") != 1 or not info.get("plan") or not info.get("framework_image")
                or not info.get("os_build") or not info.get("sdk_version_max_allowed")
                or "setup_bytes" not in info or not info.get("limitations")):
            raise ValueError("Missing vDSP platform/setup evidence")


def validate_hybrid_info(info, config):
    """Task counts describe dependency order, never native butterfly timing."""
    n, hop = config["n"], config["hop"]
    work = n+1+2*(n//2+1)
    hybrid = config["backend"] == "pffft-hybrid-float"
    expected = dict(mode="hybrid" if hybrid else "batch", task_units=work,
                    prepare_units=n, native_calls_per_frame=1, magnitude_units=n//2+1,
                    output_units=n//2+1, retained_input_samples=n+hop,
                    fft_sample_offset=((n+1)*hop+work-1)//work-1 if hybrid else 0,
                    publication_delay_samples=hop-1 if hybrid else 0,
                    cost_model="unequal tasks; native FFT including conversion is indivisible")
    if info.get("analysis_schedule") != expected:
        raise ValueError("Hybrid schedule/storage evidence mismatch")


def check(directory):
    metadata = json.loads((directory / "metadata.json").read_text())
    if metadata["schema"] == 1:
        from legacy_check import check as legacy_check
        return legacy_check(directory)
    if metadata["schema"] != 2 or metadata["status"] != "complete":
        raise ValueError("Unsupported or incomplete campaign")
    artifacts = metadata["artifact_sha256"]
    required = {"source.tar.gz", "dependencies.tar.gz", "inventory.json", "build.log",
                "verification.txt", "linked-libraries.txt", "paper.bin", "paper-audit.bin"}
    if metadata.get("external_dependency_sha256"):
        required.add("external-dependencies.tar.gz")
    required.update(metadata["resources"].values())
    required.update(run[key] for run in metadata["runs"] for key in ("raw", "stderr"))
    if not required <= artifacts.keys():
        raise ValueError("Missing artifact checksums")
    for filename, expected in artifacts.items():
        if Path(filename).name != filename or digest(directory/filename) != expected:
            raise ValueError(f"Artifact checksum mismatch: {filename}")
    if (artifacts["paper.bin"] != metadata["binary_sha256"]
            or artifacts["paper-audit.bin"] != metadata["audit_binary_sha256"]):
        raise ValueError("Executable identity mismatch")
    archives = [("source.tar.gz", "source_sha256"), ("dependencies.tar.gz", "sdk_sha256")]
    if "fftw" in metadata.get("build_features", ()):
        dependencies = metadata.get("external_dependency_sha256", {})
        if not {"fftw/include/fftw3.h", "fftw/lib/libfftw3.a", "fftw/lib/libfftw3f.a"} <= dependencies.keys():
            raise ValueError("Missing optional FFTW dependency evidence")
    if metadata.get("external_dependency_sha256"):
        archives.append(("external-dependencies.tar.gz", "external_dependency_sha256"))
    for archive_name, field in archives:
        with tarfile.open(directory/archive_name) as archive:
            for filename, expected in metadata[field].items():
                content = archive.extractfile(filename)
                if content is None or hashlib.sha256(content.read()).hexdigest() != expected:
                    raise ValueError(f"Archived source/dependency checksum mismatch: {filename}")
            if field == "source_sha256":
                document = json.load(archive.extractfile("benchmark/paper/backends.json"))
                registry = normalize_registry(document, metadata.get("build_features", ()))
    if registry != json.loads((directory/"inventory.json").read_text()):
        raise ValueError("Compiled registry differs from archived source")
    if "matrix_inventory" in metadata:
        from campaigns import inventory
        if inventory(metadata["configs"], registry) != metadata["matrix_inventory"]:
            raise ValueError("Resolved matrix inventory mismatch")
        if metadata.get("phase") not in ("smoke", "pilot", "confirmation"):
            raise ValueError("Missing evidence phase")
        if not metadata.get("host_id") or not metadata.get("session_id"):
            raise ValueError("Missing measurement host/session")
        manifest = metadata.get("campaign_manifest")
        if manifest and manifest["phase"] == "smoke" and metadata["phase"] != "smoke":
            raise ValueError("Smoke matrix mislabeled as publication evidence")
    keys = {str(i) for i in range(len(metadata["configs"]))}
    if set(metadata["contracts"]) != keys or set(metadata["resources"]) != keys:
        raise ValueError("Missing workload contracts/resources")
    if "vdsp" in metadata.get("build_features", ()):
        platform_info = metadata.get("platform_framework", {})
        if platform_info.get("name") != "Apple Accelerate/vDSP" or not platform_info.get("limitation"):
            raise ValueError("Missing platform framework identity")
    config_ids = set()
    for index, config in enumerate(metadata["configs"]):
        validate_config(config, registry, measurement=True)
        identity = json.dumps(config, sort_keys=True)
        if identity in config_ids:
            raise ValueError("Duplicate workload")
        config_ids.add(identity)
        if metadata["contracts"][str(index)] != resolve_contract(config, registry):
            raise ValueError("Evidence contract mismatch")
        resource = json.loads((directory/metadata["resources"][str(index)]).read_text())
        validate_resources(resource)
        if registry[config["backend"]]["kind"] in ("external", "scheduled-analysis", "analysis4"):
            for label in ("timing", "allocation"):
                validate_provider_info(resource[label]["provider_info"], registry[config["backend"]])
                if registry[config["backend"]]["kind"] == "scheduled-analysis":
                    validate_hybrid_info(resource[label]["provider_info"], config)
    identities = set()
    for run in metadata["runs"]:
        identity = (run["workload"], run["repeat"])
        if identity in identities:
            raise ValueError(f"Duplicate run: {identity}")
        identities.add(identity)
        config = metadata["configs"][run["workload"]]
        contract = resolve_contract(config, registry)
        validate_rows(directory/run["raw"], config, registry)
        if summarize(directory/run["raw"], config) != run["summary"]:
            raise ValueError(f"Summary mismatch: {identity}")
        external = registry[config["backend"]]["kind"] in ("external", "scheduled-analysis", "analysis4")
        if external:
            report = json.loads((directory/run["stderr"]).read_text())
            if len(report["provider_instances"]) != config["count"]:
                raise ValueError("Missing measured provider instances")
            for instance in report["provider_instances"]:
                validate_provider_info(instance, registry[config["backend"]])
                if registry[config["backend"]]["kind"] == "scheduled-analysis":
                    validate_hybrid_info(instance, config)
            if contract["boundary"] == "transform" and report["checked_bins"] != config["n"]:
                raise ValueError("Missing external transform bins")
        if contract["boundary"] in ("inverse-job", "chain") or (external and contract["boundary"] == "analysis"):
            accuracy = json.loads((directory/run["stderr"]).read_text())
            validate_synthesis_accuracy(accuracy, config, run["summary"]["publication_audit_rows"], registry)
        if contract["boundary"] == "transform":
            accuracy = json.loads((directory/run["stderr"]).read_text())
            validate_transform_accuracy(accuracy, contract)
    expected = {(index, repeat) for index in range(len(metadata["configs"]))
                for repeat in range(metadata["repeats"])}
    if identities != expected:
        raise ValueError("Missing or unexpected workload/repetition")
    return len(identities)


def validate_transform_accuracy(accuracy, contract):
    values = [accuracy[key] for key in ("max_abs_error", "max_reference", "roundtrip_max_abs_error")]
    tolerance = 2e-5 if contract["precision"] == "float" else 1e-10
    if (not all(math.isfinite(value) and value >= 0 for value in values)
            or values[0]/max(1, values[1]) >= tolerance or values[2] >= tolerance):
        raise ValueError("Invalid transform numerical report")
    if contract["step_count"] is not None and accuracy["steps"] != contract["step_count"]:
        raise ValueError("Transform work count mismatch")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    print(f"Verified {check(args.directory)} runs, source archive, artifact hashes and summaries")

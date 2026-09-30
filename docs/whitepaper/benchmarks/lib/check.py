#!/usr/bin/env python3
"""Verify a completed paper campaign without trusting its stored summaries."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import math
import hashlib
import json
from pathlib import Path
import tarfile

from run import digest
from observations import read_observations, validate_rows
from contracts import normalize_registry, resolve_contract, validate_config
from runtime import validate_profile
from numerical import validate_scalar_audit
from transitions import validate_transition_trace, validate_transition_resources
from execution import validate_campaign, validate_process



def validate_analysis_accuracy(accuracy, config, publications, contract, required_policy=None):
    data = accuracy.get("analysis")
    if data is None:
        if required_policy:
            raise ValueError("Missing required analysis numerical policy")
        return  # Historical campaigns keep their original policy.
    if data.get("policy") != "spectrum-norms-v1" or required_policy not in (None, "spectrum-norms-v1"):
        raise ValueError("Unknown analysis numerical policy")
    tolerance = 3e-4 if contract["precision"] == "float" else 1e-10
    old_limit = 3e-4 if contract["precision"] == "float" else 1e-10
    vectors = publications*contract["channels"]
    if (data.get("tolerance") != tolerance or data.get("vectors") != vectors
            or data.get("values") != accuracy["checked_samples"]):
        raise ValueError("Analysis policy/count mismatch")
    for key, maximum in (("vectors", vectors), ("values", accuracy["checked_samples"]),
                         ("zero_vectors", vectors), ("legacy_pointwise_failures", accuracy["checked_samples"])):
        if type(data[key]) is not int or not 0 <= data[key] <= maximum:
            raise ValueError("Invalid analysis diagnostic count")
    for key in ("max_relative_l2", "max_relative_linf", "max_legacy_scaled_error"):
        if not math.isfinite(data[key]) or data[key] < 0:
            raise ValueError("Invalid analysis norm/pointwise metric")
    if max(data["max_relative_l2"], data["max_relative_linf"]) > tolerance:
        raise ValueError("Analysis spectrum norm tolerance exceeded")
    if bool(data["legacy_pointwise_failures"]) != (data["max_legacy_scaled_error"] > old_limit):
        raise ValueError("Missing pointwise diagnostic failures")
    error, scale = accuracy["max_abs_error"], accuracy["max_reference"]
    if ((scale == 0 and error != 0) or error > data["max_relative_linf"]*scale*(1+1e-12)
            or (error == 0) != (data["max_relative_l2"] == 0)
            or (data["zero_vectors"] == vectors) != (scale == 0)):
        raise ValueError("Inconsistent analysis norm/absolute error")
    worst = data["worst_pointwise"]
    for key, high in (("bin", config["n"]//2), ("channel", contract["channels"]-1), ("endpoint", None)):
        if type(worst[key]) is not int or worst[key] < 0 or (high is not None and worst[key] > high):
            raise ValueError("Invalid pointwise diagnostic location")
    if not all(math.isfinite(worst[k]) for k in ("actual", "reference")):
        raise ValueError("Non-finite pointwise diagnostic")
    measured = abs(worst["actual"]-worst["reference"])/max(1, abs(worst["reference"]))
    if not math.isclose(measured, data["max_legacy_scaled_error"], rel_tol=1e-12, abs_tol=0):
        raise ValueError("Pointwise diagnostic mismatch")


def validate_synthesis_accuracy(accuracy, config, publications, registry=None, required_policy=None):
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
    if contract["boundary"] == "analysis":
        validate_analysis_accuracy(accuracy, config, publications, contract, required_policy)


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


def validate_provider_info(info, descriptor, config=None):
    if descriptor["kind"] == "analysis4":
        simd = descriptor["id"] == "core-independent4-simd"
        native = info.get("native_instances", [])
        if (info.get("input_contract") != ("explicit-four-v3" if config and config.get("workload_schema") == 3 else "independent-four-v1")
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


def runtime_artifacts(metadata, required):
    """Require distinct, authenticated sidecars only for declared telemetry."""
    if "runtime_profile" not in metadata:
        if "runtime" in metadata or any("runtime" in run for run in metadata["runs"]):
            raise ValueError("Runtime telemetry requires a declared profile")
        return []
    if metadata["runtime_profile"] != "coarse-wall-v1":
        raise ValueError("Unsupported runtime profile policy")
    names = []
    occupied = required | {"metadata.json"}
    for run in metadata["runs"]:
        name = run.get("runtime")
        if (not isinstance(name, str) or not name or Path(name).name != name
                or name in (".", "..") or name in occupied):
            raise ValueError("Missing, invalid or aliased runtime artifact")
        occupied.add(name)
        names.append(name)
    return names


def transition_artifacts(metadata, required):
    """Require an authenticated, distinct trace for each interactive process."""
    declared = metadata.get("transition_policy")
    if declared not in (None, "fourier-transitions-v1"):
        raise ValueError("Unsupported transition policy")
    names, occupied = [], required | {"metadata.json"}
    for run in metadata["runs"]:
        enabled = bool(metadata["configs"][run["workload"]].get("transition_suite"))
        if not enabled:
            if "transition" in run:
                raise ValueError("Unexpected transition trace")
            continue
        name = run.get("transition")
        if (declared is None or not isinstance(name, str) or not name
                or Path(name).name != name or name in (".", "..") or name in occupied):
            raise ValueError("Missing, invalid or aliased transition artifact")
        occupied.add(name)
        names.append(name)
    return names


def check(directory, report_data=None):
    """Validate evidence, optionally retaining compact per-file report inputs."""
    metadata = json.loads((directory / "metadata.json").read_text())
    if metadata["schema"] == 1:
        from legacy_check import check as legacy_check
        return legacy_check(directory)
    if metadata["schema"] != 2 or metadata["status"] != "complete":
        raise ValueError("Unsupported or incomplete campaign")
    if "study_freeze" in metadata:
        from study import enforce
        if metadata.get("phase") != "confirmation":
            raise ValueError("Frozen evidence must be confirmation")
        frozen = metadata["study_freeze"]
        from report import identity
        if frozen.get("freeze_id") != identity({k:v for k,v in frozen.items() if k != "freeze_id"}):
            raise ValueError("Changed frozen study")
        enforce(frozen, metadata)
    artifacts = metadata["artifact_sha256"]
    required = {"source.tar.gz", "dependencies.tar.gz", "inventory.json", "build.log",
                "verification.txt", "linked-libraries.txt", "paper.bin", "paper-audit.bin"}
    if metadata.get("external_dependency_sha256"):
        required.add("external-dependencies.tar.gz")
    required.update(metadata["resources"].values())
    required.update(run[key] for run in metadata["runs"] for key in ("raw", "stderr"))
    runtime_files = runtime_artifacts(metadata, required)
    required.update(runtime_files)
    transition_files = transition_artifacts(metadata, required)
    required.update(transition_files)
    execution_files = validate_campaign(metadata)
    if set(execution_files) & (required | {"metadata.json"}):
        raise ValueError("Execution artifact aliases another artifact")
    required.update(execution_files)
    omissions_path = directory/"bundle-omissions.json"
    omissions = set()
    if omissions_path.exists():
        omitted = json.loads(omissions_path.read_text())
        allowed = {"dependencies.tar.gz", "external-dependencies.tar.gz", "paper.bin", "paper-audit.bin"}
        if (omitted.get("schema") != 1 or omitted.get("metadata_sha256") != digest(directory/"metadata.json")
                or not set(omitted.get("files", [])) <= allowed):
            raise ValueError("Invalid audit-only bundle omissions")
        omissions = set(omitted["files"])
        if any((directory/name).exists() for name in omissions):
            raise ValueError("An omitted dependency artifact is unexpectedly present")
    if not required <= artifacts.keys():
        raise ValueError("Missing artifact checksums")
    for filename in runtime_files+transition_files+execution_files:
        path = directory/filename
        if path.is_symlink() or not path.is_file():
            raise ValueError(f"Missing or aliased runtime artifact: {filename}")
    for filename, expected in artifacts.items():
        if filename in omissions:
            continue
        if Path(filename).name != filename or digest(directory/filename) != expected:
            raise ValueError(f"Artifact checksum mismatch: {filename}")
    for filename in runtime_files:
        validate_profile(json.loads((directory/filename).read_text()))
    # The runner adds this only after its first complete evidence check finishes.
    if "runtime" in metadata:
        validate_profile(metadata["runtime"])
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
        if archive_name in omissions:
            continue
        with tarfile.open(directory/archive_name) as archive:
            for filename, expected in metadata[field].items():
                content = archive.extractfile(filename)
                if content is None or hashlib.sha256(content.read()).hexdigest() != expected:
                    raise ValueError(f"Archived source/dependency checksum mismatch: {filename}")
            if field == "source_sha256":
                registry_path = next(name for name in (
                    "docs/whitepaper/benchmarks/lib/backends.json",
                    "docs/whitepaper/benchmarks/backends.json", "benchmark/paper/backends.json")
                    if name in metadata[field])
                document = json.load(archive.extractfile(registry_path))
                registry = normalize_registry(document, metadata.get("build_features", ()))
    if registry != json.loads((directory/"inventory.json").read_text()):
        raise ValueError("Compiled registry differs from archived source")
    if ("benchmark/paper/analysis_accuracy.hpp" in metadata["source_sha256"]
            and metadata.get("analysis_accuracy_policy") != "spectrum-norms-v1"):
        raise ValueError("Missing analysis policy for archived implementation")
    if any(c.get("workload_schema") == 3 for c in metadata["configs"]):
        if (metadata.get("analysis_accuracy_policy") != "spectrum-norms-v1"
                or metadata.get("scalar_analysis_audit_policy") != "all-publications-v1"):
            raise ValueError("Explicit workloads require complete numerical policies")
    scalar_policy = metadata.get("scalar_analysis_audit_policy")
    if scalar_policy not in (None, "all-publications-v1"):
        raise ValueError("Unknown scalar numerical coverage policy")
    if ("benchmark/paper/scalar_analysis_audit.hpp" in metadata["source_sha256"]
            and scalar_policy != "all-publications-v1"):
        raise ValueError("Missing scalar numerical coverage policy for archived implementation")
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
        if config.get("transition_suite"):
            for label in ("timing", "allocation"):
                validate_transition_resources(resource[label]["provider_info"], config, registry)
        elif registry[config["backend"]]["kind"] in ("external", "scheduled-analysis", "analysis4"):
            for label in ("timing", "allocation"):
                validate_provider_info(resource[label]["provider_info"], registry[config["backend"]], config)
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
        summary, details = read_observations(directory/run["raw"], config, registry,
                                             report=report_data is not None)
        if summary != run["summary"]:
            raise ValueError(f"Summary mismatch: {identity}")
        if metadata.get("execution_policy"):
            validate_process(json.loads((directory/run["execution"]).read_text()),
                             metadata["execution_policy"], config, summary, directory/run["raw"],
                             metadata["sleep_protection"].get("pid", 0))
        descriptor = registry[config["backend"]]
        external = descriptor["kind"] in ("external", "scheduled-analysis", "analysis4")
        scalar = (scalar_policy is not None and descriptor["kind"] in ("core", "legacy")
                  and descriptor["channels"] == 1)
        transition = bool(config.get("transition_suite"))
        if transition:
            trace = json.loads((directory/run["transition"]).read_text())
            validate_transition_trace(trace, config, registry)
            if len(trace["publications"]) != summary["publication_audit_rows"]:
                raise ValueError("Transition trace/raw coverage mismatch")
        if external and not transition:
            report = json.loads((directory/run["stderr"]).read_text())
            if len(report["provider_instances"]) != config["count"]:
                raise ValueError("Missing measured provider instances")
            for instance in report["provider_instances"]:
                validate_provider_info(instance, registry[config["backend"]], config)
                if registry[config["backend"]]["kind"] == "scheduled-analysis":
                    validate_hybrid_info(instance, config)
        if not transition and (contract["boundary"] in ("inverse-job", "chain") or ((external or scalar) and contract["boundary"] == "analysis")):
            accuracy = json.loads((directory/run["stderr"]).read_text())
            validate_synthesis_accuracy(accuracy, config, run["summary"]["publication_audit_rows"], registry,
                                        metadata.get("analysis_accuracy_policy"))
            if scalar:
                validate_scalar_audit(accuracy, config, contract, run["summary"]["publication_audit_rows"])
        if contract["boundary"] == "transform":
            accuracy = json.loads((directory/run["stderr"]).read_text())
            validate_transform_accuracy(accuracy, contract)
        if report_data is not None:
            if config.get("workload_schema") == 3:
                from metrics import read as scheduling_metrics
                details["scheduling"] = scheduling_metrics(directory/run["raw"], config, contract,
                    json.loads((directory/run["execution"]).read_text()))
            report_data[run["raw"]] = details
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
    if contract["step_count"] is None:
        bins = contract["outputs_per_channel"]
        for key, expected in (("checked_bins", bins), ("direct_bins", bins if bins <= 256 else 17)):
            if type(accuracy.get(key)) is not int or accuracy[key] != expected:
                raise ValueError("Missing or incorrect external transform reference coverage")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    print(f"Verified {check(args.directory)} runs, source archive, artifact hashes and summaries")

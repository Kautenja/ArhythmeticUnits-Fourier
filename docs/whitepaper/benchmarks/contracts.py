"""Capabilities and resolved contracts, independent of backend naming conventions."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path


def load_registry(path=None, features=()):
    document = json.loads((Path(path) if path else Path(__file__).with_name("backends.json")).read_text())
    return normalize_registry(document, features)


def normalize_registry(document, features=()):
    """Reject malformed descriptors before emitting code or resolving evidence."""
    if document["schema"] != 1:
        raise ValueError("Unsupported backend registry")
    features = set(features)
    if features - {"fftw", "vdsp"}:
        raise ValueError("Unknown optional build feature")
    registry = {}
    for item in document["backends"]:
        descriptor = dict(document["defaults"], **item)
        if set(descriptor) != set(document["defaults"]) | {"id"}:
            raise ValueError("Unknown capability field")
        for key, default in dict(document["defaults"], id="").items():
            value = descriptor[key]
            if type(value) is not type(default):
                raise ValueError("Invalid capability type: " + key)
            if isinstance(value, str) and any(ord(c) < 32 or ord(c) > 126 or c in '\\"' for c in value):
                raise ValueError("Invalid capability string: " + key)
        if (descriptor["size_min"] < 1 or descriptor["size_max"] < descriptor["size_min"]
                or descriptor["size_multiple"] < 1 or descriptor["channels"] < 1):
            raise ValueError("Invalid capability bounds")
        for key, allowed in {
                "boundary": {"analysis", "module", "transform", "inverse-job", "chain", "control"},
                "schedule": {"balanced", "immediate", "legacy-budget", "transform", "none"},
                "step_model": {"none", "opaque", "radix2-real", "radix2-complex", "radix2-inverse"},
                "precision": {"float", "double"}}.items():
            if descriptor[key] not in allowed:
                raise ValueError("Unknown capability semantics: " + key)
        if descriptor["id"] in registry:
            raise ValueError("Duplicate backend identity")
        feature = descriptor.get("build_feature", "")
        if feature:
            descriptor["available"] = feature in features
            descriptor["reason"] = "" if descriptor["available"] else "Optional benchmark feature is disabled: " + feature
        registry[descriptor["id"]] = descriptor
    return registry


REGISTRY = load_registry()


def descriptor(name, registry=None):
    registry = REGISTRY if registry is None else registry
    if name not in registry:
        raise ValueError(f"Unknown backend: {name}")
    result = registry[name]
    if not result["available"]:
        raise ValueError(f"Unavailable backend {name}: {result['reason']}")
    return result


def validate_config(config, registry=None, measurement=False):
    if "transition_control" in config and not config.get("transition_suite"):
        raise ValueError("Transition control requires a suite")
    if "transition_suite" in config and not config["transition_suite"]:
        raise ValueError("Empty transition suite")
    if measurement and config.get("transition_suite"):
        from transitions import validate_transition_config
        validate_transition_config(config, registry)
    d = descriptor(config["backend"], registry)
    bounds = dict(n=(d["size_min"], d["size_max"]), hop=(1, 65536), block=(1, 65536),
                  count=(1, 64), load=(0, 4096), voices=(1, d["max_voices"]),
                  rate=(8000, 192000), cache_mib=(0, 256), smooth=(0, 1))
    for key, (low, high) in bounds.items():
        if type(config[key]) is not int or not low <= config[key] <= high:
            raise ValueError(f"Invalid {key} for {d['id']}")
    n, hop = config["n"], config["hop"]
    offset = config.get("callback_offset", 0)
    if (type(offset) is not int or not 0 <= offset < hop
            or (offset and (config["state"] == "startup" or d["boundary"] == "transform"))):
        raise ValueError("Unsupported callback origin offset")
    if ((d["power_of_two"] and n & (n-1)) or n % d["size_multiple"]
            or (d["fixed_n"] and n != d["fixed_n"])
            or (d["fixed_hop"] and hop != d["fixed_hop"])):
        raise ValueError("Unsupported transform size or hop")
    if config["alignment"] not in ("aligned", "staggered") or config["state"] not in ("steady", "startup", "live"):
        raise ValueError("Unsupported alignment/state")
    if ((config["state"] == "live" and not d["live"])
            or (config["smooth"] and not d["smoothing"])
            or (config["state"] == "startup" and config["alignment"] != "aligned")):
        raise ValueError("Unsupported smoothing/state combination")
    if d["boundary"] == "chain" and hop > n-2:
        raise ValueError("Overlap-save requires H <= N-2")
    transform = d["boundary"] == "transform"
    passes = (("complete", "incremental", "phases", "steps") if d["step_model"] in
              ("radix2-real", "radix2-complex", "radix2-inverse") else ("complete",))
    if config["pass_name"] not in (passes if transform else ("callback", "throughput")):
        raise ValueError("Unsupported measurement pass")
    if transform and (config["count"] != 1 or config["load"] or config["cache_mib"]
                      or config["smooth"] or config["state"] != "steady" or config["alignment"] != "aligned"):
        raise ValueError("Unused transform options must be neutral")
    if measurement:
        for key, low, high in (("callbacks", 1, 1000000), ("warm_hops", 0, 4096)):
            if type(config[key]) is not int or not low <= config[key] <= high:
                raise ValueError(f"Invalid {key}")
        if not transform and config["callbacks"]*config["block"] < 2*hop:
            raise ValueError("Measure at least two complete hops")
    return d


def resolve_contract(config, registry=None):
    d = validate_config(config, registry)
    n, hop = config["n"], config["hop"]
    model = d["step_model"]
    butterflies = n//4*((n//2).bit_length()-1) if model == "radix2-real" else n//2*(n.bit_length()-1)
    steps = butterflies+(n if model == "radix2-inverse" else 0) if model in ("radix2-real", "radix2-complex", "radix2-inverse") else None
    delay = 0
    if d["schedule"] == "balanced":
        delay = hop-1
    elif d["schedule"] == "legacy-budget":
        work = n//4*((n//2).bit_length()-1)
        quota = (work+hop-1)//hop
        delay = (work+quota-1)//quota-1
    origin, center, playback = "input frame endpoint", (n-1)/2, -1
    outputs = n//2+1
    if d["boundary"] == "inverse-job":
        origin, center, outputs = "spectrum release", 0, n
    elif d["boundary"] == "chain":
        center, outputs, playback = (hop-1)/2, hop, hop-1+delay
    elif d["boundary"] == "transform":
        origin, center, outputs = "buffered transform", 0, n
    elif d["boundary"] == "control":
        origin, center, outputs = "none", 0, 0
    return dict(backend=d["id"], boundary=d["boundary"], precision=d["precision"], channels=d["channels"],
                layout=d["layout"], normalization=d["normalization"], origin=origin,
                outputs_per_channel=outputs, publication_delay_samples=delay,
                center_offset_samples=center, playback_delay_samples=playback, step_count=steps,
                step_model=model, operation=d["operation"], plan=d["plan"])

SYNTHESIS_BACKENDS = {
    f"{family}-{mode}-{precision}"
    for family in ("inverse-stream", "ols-identity", "ols-fir")
    for mode in ("batch", "incremental")
    for precision in ("float", "double")
}


def synthesis_contract(config):
    backend = config["backend"]
    if backend not in SYNTHESIS_BACKENDS:
        raise ValueError(f"Unknown synthesis backend: {backend}")
    n, hop = config["n"], config["hop"]
    if (not isinstance(n, int) or not isinstance(hop, int) or n < 128
            or n > 16384 or n & (n-1) or not 1 <= hop <= 65536):
        raise ValueError("Invalid synthesis length/hop")
    if (config["pass_name"] not in ("callback", "throughput") or config["smooth"]
            or config["state"] not in ("steady", "startup") or config["voices"] != 1):
        raise ValueError("Unsupported synthesis pass, smoothing, state, or voices")
    if config["state"] == "startup" and config["alignment"] != "aligned":
        raise ValueError("Startup must be aligned")
    inverse = backend.startswith("inverse-stream-")
    if not inverse and hop > n-2:
        raise ValueError("Overlap-save requires H <= N-2")
    delay = 0 if "-batch-" in backend else hop-1
    return dict(
        family="inverse-job" if inverse else "overlap-save",
        transform="complex-to-complex", normalization="inverse includes 1/N",
        origin="spectrum release" if inverse else "input frame endpoint",
        outputs_per_publication=n if inverse else hop,
        publication_delay_samples=delay,
        center_offset_samples=0 if inverse else (hop-1)/2,
        playback_delay_samples=-1 if inverse else hop-1+delay,
        operation="inverse" if inverse else "identity" if "-identity-" in backend else "three-tap FIR",
    )

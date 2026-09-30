"""Opt-in v3 controls; never reinterpret archived v1/v2 workloads."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import math
import struct

DEFAULTS = dict(workload_schema=3, window="hann", octave=0, temporal_mode="alpha", temporal_value=0,
                fixture="mixed", fixture_seed=305419896, decay_samples=4096, active_ports=1,
                execution_regime="continuous", experimental_policy="existing")
FIELDS = set(DEFAULTS)
FIXTURES = ("mixed", "silence", "decay", "impulse", "dc", "nyquist", "off-bin", "weak", "noise", "independent")


def expand(config):
    return dict(DEFAULTS, **config) if config.get("workload_schema") == 3 else dict(config)


def float32(value):
    return struct.unpack("f", struct.pack("f", value))[0]


def alpha(config):
    if config["temporal_mode"] == "alpha":
        return float32(config["temporal_value"])
    value = float32(config["temporal_value"])
    return float32(math.exp(float32(float32(-10*float32(config["hop"]/config["rate"]))/value))) if value else 0.


def validate(config, descriptor):
    if "workload_schema" not in config:
        if FIELDS & config.keys():
            raise ValueError("Explicit workload controls require workload_schema=3")
        return
    if type(config["workload_schema"]) is not int or config["workload_schema"] != 3 or not FIELDS <= config.keys():
        raise ValueError("Incomplete or unknown explicit workload schema")
    if (config["smooth"] or config["state"] == "live" or config.get("transition_suite")
            or descriptor["boundary"] not in ("analysis", "module", "control") or descriptor["kind"] == "core4"):
        raise ValueError("v3 controls require stationary analysis/module/control with neutral legacy smooth")
    for key, low, high in (("octave", 0, 2.5), ("temporal_value", 0, 10)):
        if type(config[key]) not in (float, int) or not math.isfinite(config[key]) or not low <= config[key] <= high:
            raise ValueError("Invalid explicit control: " + key)
    if config["temporal_mode"] not in ("alpha", "module-seconds") or (config["temporal_mode"] == "alpha" and config["temporal_value"] >= 1):
        raise ValueError("Invalid temporal smoothing mode/value")
    if not 0 <= alpha(config) < 1:
        raise ValueError("Temporal alpha must remain below one after binary32 conversion")
    if config["window"] not in ("hann", "boxcar", "blackman-harris", "flattop") or config["fixture"] not in FIXTURES:
        raise ValueError("Unknown window/fixture")
    if descriptor["operation"] == "shipped-default" and (
            config["window"] != "flattop" or config["octave"] or config["temporal_value"] or config["rate"] != 48000):
        raise ValueError("Shipped-default module controls must remain at their 48 kHz defaults")
    for key, low, high in (("fixture_seed", 0, 2**32-1), ("decay_samples", 1, 10**9),
                           ("active_ports", 1, descriptor["channels"])):
        if type(config[key]) is not int or not low <= config[key] <= high:
            raise ValueError("Invalid explicit control: " + key)
    if (config["experimental_policy"] != "existing" or config["execution_regime"] not in ("continuous", "paced")
            or (config["execution_regime"] == "paced" and config["pass_name"] != "callback")):
        raise ValueError("Unavailable experimental/execution policy")
    if config["fixture"] == "independent" and descriptor["channels"] == 1:
        raise ValueError("Independent-channel fixture requires multiple channels")
    if descriptor["boundary"] == "module":
        if config["temporal_mode"] != "module-seconds" or config["temporal_value"] > 2.5 or config["octave"] not in (0, 1/3, 1, 2):
            raise ValueError("Unsupported module panel smoothing controls")
    elif config["active_ports"] != descriptor["channels"]:
        raise ValueError("Partial ports require a module workload")


def contract_fields(config):
    if config.get("workload_schema") != 3:
        return {}
    return dict(workload={key: config[key] for key in DEFAULTS}, effective_octave=float32(config["octave"]),
                effective_temporal_alpha=alpha(config),
                input_contract="finite-float-v3; abs(input)<=1; silence/decay may flush subnormals")

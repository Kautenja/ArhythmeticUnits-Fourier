"""Explicit latency and output contracts for the synthesis baseline families."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later

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

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


def validate_rows(path, config):
    """Check observation counts and publication age/cadence from raw records."""
    counts = Counter()
    publications = Counter()
    previous = {}
    backend, mode = config["backend"], config["pass_name"]
    n, hop, block = config["n"], config["hop"], config["block"]
    delay = hop-1
    if backend.startswith("legacy-batch"):
        delay = 0
    elif backend.startswith("legacy-incremental"):
        butterflies = n//4 * ((n//2).bit_length()-1)
        quota = (butterflies+hop-1)//hop
        delay = (butterflies+quota-1)//quota-1
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream):
            counts[row["kind"]] += 1
            if row["kind"] != "publication":
                continue
            analyzer, sample = int(row["analyzer"]), int(row["sample"])
            offset = analyzer*hop//config["count"] if config["alignment"] == "staggered" else 0
            if not 0 <= analyzer < config["count"] or not 0 <= sample < config["callbacks"]*block:
                raise ValueError("Publication index outside workload")
            if (sample+offset)%hop != delay or (analyzer in previous and sample-previous[analyzer] != hop):
                raise ValueError("Publication cadence mismatch")
            ages = (float(row["endpoint_age_samples"]), float(row["center_age_samples"]),
                    float(row["callback_visible_age_samples"]))
            if ages != (delay, delay+(n-1)/2, delay+block-1-sample%block):
                raise ValueError("Publication age mismatch")
            previous[analyzer] = sample
            publications[analyzer] += 1
    expected = Counter(timer=1024)
    frames = config["callbacks"]
    if mode in ("callback", "throughput"):
        expected[mode] = frames if mode == "callback" else 1
        if backend != "driver":
            for analyzer in range(config["count"]):
                offset = analyzer*hop//config["count"] if config["alignment"] == "staggered" else 0
                bias = hop-1-delay
                count = (frames*block+offset+bias)//hop-(offset+bias)//hop
                if publications[analyzer] != count:
                    raise ValueError("Missing publication records")
                expected["publication"] += count
    elif mode in ("complete", "incremental"):
        expected[mode] = frames
    else:
        real, inverse = backend.startswith("rfft"), backend.startswith("ifft")
        butterflies = n//4*((n//2).bit_length()-1) if real else n//2*(n.bit_length()-1)
        expected["buffer"] = frames
        expected["butterfly_step" if mode == "steps" else "butterflies"] = frames*(butterflies-int(real) if mode == "steps" else 1)
        if real:
            expected["reconstruct_step" if mode == "steps" else "last_butterfly_and_reconstruction"] = frames
        if inverse:
            expected["normalize_step" if mode == "steps" else "normalization"] = frames*(n if mode == "steps" else 1)
    if counts != expected:
        raise ValueError(f"Observation count mismatch: {counts} != {expected}")


def check(directory):
    metadata = json.loads((directory / "metadata.json").read_text())
    if metadata["schema"] != 1 or metadata["status"] != "complete":
        raise ValueError("Unsupported or incomplete campaign")
    for filename, expected in metadata["artifact_sha256"].items():
        if Path(filename).name != filename or digest(directory/filename) != expected:
            raise ValueError(f"Artifact checksum mismatch: {filename}")
    with tarfile.open(directory/"source.tar.gz") as archive:
        for filename, expected in metadata["source_sha256"].items():
            content = archive.extractfile(filename)
            if content is None or hashlib.sha256(content.read()).hexdigest() != expected:
                raise ValueError(f"Archived source checksum mismatch: {filename}")
    identities = set()
    for run in metadata["runs"]:
        identity = (run["workload"], run["repeat"])
        if identity in identities:
            raise ValueError(f"Duplicate run: {identity}")
        identities.add(identity)
        config = metadata["configs"][run["workload"]]
        validate_rows(directory/run["raw"], config)
        if summarize(directory/run["raw"], config) != run["summary"]:
            raise ValueError(f"Summary mismatch: {identity}")
        if config["pass_name"] in ("complete", "incremental", "phases", "steps"):
            accuracy = json.loads((directory/run["stderr"]).read_text())
            values = [accuracy[key] for key in ("max_abs_error", "max_reference", "roundtrip_max_abs_error")]
            tolerance = 2e-5 if config["backend"].endswith("float") else 1e-10
            if (not all(math.isfinite(value) and value >= 0 for value in values)
                    or values[0]/max(1, values[1]) >= tolerance or values[2] >= tolerance):
                raise ValueError(f"Invalid numerical report: {identity}")
    expected = {(index, repeat) for index in range(len(metadata["configs"]))
                for repeat in range(metadata["repeats"])}
    if identities != expected:
        raise ValueError("Missing or unexpected workload/repetition")
    return len(identities)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    print(f"Verified {check(args.directory)} runs, source archive, artifact hashes and summaries")

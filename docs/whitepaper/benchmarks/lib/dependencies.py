"""Retain optional benchmark libraries and available build/source evidence."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib
import json
from pathlib import Path


def checksum(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fftw_inputs(prefix):
    prefix = Path(prefix).resolve()
    result = {"fftw/"+name: prefix/name for name in
              ("include/fftw3.h", "lib/libfftw3.a", "lib/libfftw3f.a")}
    for path in result.values():
        if not path.is_file():
            raise ValueError("Missing optional FFTW input: " + str(path))
    provenance = prefix/"provenance.json"
    if provenance.is_file():
        result["fftw/provenance.json"] = provenance
        document = json.loads(provenance.read_text())
        for name, expected in document.get("files", {}).items():
            key = "fftw/"+name
            if key in result and checksum(result[key]) != expected:
                raise ValueError("Installed FFTW library/header differs from build provenance")
        # Retain hash-identified optional source/build evidence; unavailable older
        # provenance paths remain explicitly absent rather than invented.
        evidence = {}
        if document.get("source_archive"):
            evidence["source.tar.gz"] = dict(path=document["source_archive"], sha256=document["source_sha256"])
        if document.get("source_manifest"):
            evidence["source-manifest.json"] = dict(path=document["source_manifest"], sha256=document["source_manifest_sha256"])
        for precision, files in document.get("build_evidence", {}).items():
            for name, item in files.items():
                if Path(name).name != name:
                    raise ValueError("Invalid dependency evidence name")
                evidence[precision+"-"+name] = item
        for name, item in evidence.items():
            path = Path(item["path"])
            if path.is_file():
                if checksum(path) != item["sha256"]:
                    raise ValueError("FFTW source/build evidence changed: " + name)
                result["fftw/"+name] = path
    return result

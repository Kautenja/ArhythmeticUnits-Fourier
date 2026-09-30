#!/usr/bin/env python3
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build an optional, pinned serial FFTW dependency for paper benchmarks only."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import tarfile
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parents[3]
VERSION = "3.3.10"
URL = f"https://www.fftw.org/fftw-{VERSION}.tar.gz"
SHA256 = "56c932549852cddcfafdab3820b0200c7742675be92179e59e6215b340e26467"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fresh_source(archive, work):
    """Extract to a never-reused directory, preserving all previous build data."""
    fresh = Path(tempfile.mkdtemp(prefix="build-", dir=work)).resolve()
    source = fresh / f"fftw-{VERSION}"
    with tarfile.open(archive) as bundle:
        for member in bundle.getmembers():
            target = (fresh / member.name).resolve()
            if source not in target.parents and target != source:
                raise RuntimeError("Unexpected archive member")
            if not member.isfile() and not member.isdir():
                raise RuntimeError("Unexpected non-file archive member")
        bundle.extractall(fresh)
    return fresh, source


def source_manifest(source):
    """Fingerprint the exact relative file names, permissions and source bytes."""
    files = {str(path.relative_to(source)): {
        "sha256": digest(path), "mode": path.stat().st_mode & 0o777,
    } for path in sorted(source.rglob("*")) if path.is_file()}
    encoded = json.dumps(files, sort_keys=True, separators=(",", ":")).encode()
    return files, hashlib.sha256(encoded).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, default=ROOT / ".build/deps/fftw")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    prefix = args.prefix.resolve()
    work = ROOT / ".build/deps/fftw-source"
    work.mkdir(parents=True, exist_ok=True)
    archive = work / f"fftw-{VERSION}.tar.gz"
    if not archive.exists():
        urllib.request.urlretrieve(URL, archive)
    if digest(archive) != SHA256:
        raise RuntimeError("FFTW source archive SHA-256 mismatch")
    fresh, source = fresh_source(archive, work)
    manifest, source_sha = source_manifest(source)
    manifest_path = fresh / "source-manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Building from fresh verified source in {fresh}", flush=True)
    machine = platform.machine().lower()
    simd = (["--enable-neon"] if machine in {"arm64", "aarch64"} else
            ["--enable-sse2", "--enable-avx", "--enable-avx2"]
            if machine in {"x86_64", "amd64"} else [])
    compiler = os.environ.get("CC", "clang" if platform.system() == "Darwin" else "cc")
    flags = "-O3 -fPIC"
    commands = []
    precision_simd = {}
    for precision in ("double", "float"):
        directory = fresh / f"build-{precision}"
        directory.mkdir()
        # FFTW 3.3.10's NEON codelets support only single precision.
        selected_simd = [] if precision == "double" and simd == ["--enable-neon"] else simd
        precision_simd[precision] = selected_simd
        configure = [str(source / "configure"), f"--prefix={prefix}",
                     "--enable-static", "--disable-shared", "--disable-fortran",
                     "--disable-threads", "--disable-openmp", *selected_simd,
                     f"CC={compiler}", f"CFLAGS={flags}"]
        if precision == "float":
            configure.append("--enable-float")
        with (directory / "build.log").open("w") as log:
            for command in (configure, ["make", f"-j{args.jobs}"], ["make", "install"]):
                commands.append({"cwd": str(directory), "argv": command})
                subprocess.run(command, cwd=directory, stdout=log, stderr=subprocess.STDOUT,
                               check=True)
    files = [prefix / "include/fftw3.h", prefix / "lib/libfftw3.a",
             prefix / "lib/libfftw3f.a"]
    if source_manifest(source)[1] != source_sha:
        raise RuntimeError("FFTW source changed during the build")
    provenance = {
        "schema": 1, "version": VERSION, "source_url": URL, "source_sha256": SHA256,
        "source_archive": str(archive), "source_root": str(source),
        "source_tree_sha256": source_sha, "source_manifest": str(manifest_path),
        "source_manifest_sha256": digest(manifest_path), "build_root": str(fresh),
        "machine": machine, "platform": platform.platform(), "threads": 1,
        "compiler": subprocess.check_output([compiler, "--version"], text=True),
        "cflags": flags, "simd_configure_options": precision_simd, "commands": commands,
        "files": {str(path.relative_to(prefix)): digest(path) for path in files},
        "config_headers": {precision: digest(fresh / f"build-{precision}/config.h")
                           for precision in ("float", "double")},
        "build_evidence": {
            precision: {name: {"path": str(fresh / f"build-{precision}" / name),
                               "sha256": digest(fresh / f"build-{precision}" / name)}
                        for name in ("config.h", "config.log", "build.log")}
            for precision in ("float", "double")},
    }
    (prefix / "provenance.json").write_text(json.dumps(provenance, indent=2) + "\n")
    print(f"Built FFTW {VERSION} float and double serial libraries in {prefix}")


if __name__ == "__main__":
    main()

"""Portable audit bundles with safe extraction and explicit omitted dependencies."""

# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import io
import json
from pathlib import Path, PurePosixPath
import shutil
import tarfile
import tempfile
from paths import ROOT, BENCHMARKS
from run import digest, save
from study import evidence

OMIT = {
    "dependencies.tar.gz",
    "external-dependencies.tar.gz",
    "paper.bin",
    "paper-audit.bin",
}


def file_map(directory):
    result = {}
    for p in sorted(directory.rglob("*")):
        if p.is_symlink():
            raise ValueError("Evidence bundle cannot contain symbolic links")
        if p.is_file():
            result[p.relative_to(directory).as_posix()] = digest(p)
    return result


def pack(campaigns, output, selection=None):
    if output.exists():
        raise ValueError("Bundle destination already exists")
    if any(
        output.resolve() == p.resolve() or p.resolve() in output.resolve().parents
        for p in campaigns
    ):
        raise ValueError("Bundles must stay outside immutable campaigns")
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        records = []
        for i, p in enumerate(campaigns):
            m = evidence(p)
            destination = root / "campaigns" / f"{i:03d}"
            destination.mkdir(parents=True)
            for name in m["artifact_sha256"]:
                if name in OMIT:
                    continue
                if Path(name).name != name:
                    raise ValueError("Invalid campaign artifact name")
                shutil.copy2(p / name, destination / name)
            shutil.copy2(p / "metadata.json", destination / "metadata.json")
            omitted = sorted(OMIT & m["artifact_sha256"].keys())
            save(
                destination / "bundle-omissions.json",
                dict(
                    schema=1,
                    files=omitted,
                    metadata_sha256=digest(destination / "metadata.json"),
                ),
            )
            records.append(
                dict(
                    path=destination.relative_to(root).as_posix(),
                    phase=m["phase"],
                    omitted=omitted,
                    metadata_sha256=digest(p / "metadata.json"),
                )
            )
        tooling = root / "tooling/docs/whitepaper/benchmarks"
        tooling.mkdir(parents=True)
        for name in ("bench.py", "report-requirements.txt"):
            shutil.copy2(BENCHMARKS / name, tooling / name)
        for name in ("lib", "profiles", "guides"):
            shutil.copytree(
                BENCHMARKS / name,
                tooling / name,
                ignore=shutil.ignore_patterns("__pycache__", "*.pyc"),
            )
        shutil.copy2(ROOT / "LICENSE.md", root / "LICENSE.md")
        shutil.copy2(ROOT / "LICENSE.md", root / "tooling/LICENSE.md")
        if selection:
            from publication import checked_report

            s = json.loads(selection.read_text())
            report = (selection.parent / s["report"]).resolve()
            data, _ = checked_report(report)
            if {v["metadata_sha256"] for v in data["sources"]} != {
                r["metadata_sha256"] for r in records
            }:
                raise ValueError("Selection and packaged campaign identities differ")
            shutil.copytree(report, root / "report")
            # Adapt only location fields in packaged derivations; raw evidence is unchanged.
            mapping = {r["metadata_sha256"]: r["path"] for r in records}
            data = json.loads((root / "report/evidence.json").read_text())
            for source in data["sources"]:
                source["directory"] = "../" + mapping[source["metadata_sha256"]]
            save(root / "report/evidence.json", data)
            save(
                root / "report/manifest.json",
                {
                    p.name: digest(p)
                    for p in (root / "report").iterdir()
                    if p.is_file() and p.name != "manifest.json"
                },
            )
            s["report"] = "report"
            s["report_manifest_sha256"] = digest(root / "report/manifest.json")
            save(root / "selection.json", s)
        (root / "README.md").write_text(
            "# Fourier Audit Bundle\n\n"
            "Run from this extracted directory:\n\n```shell\n"
            "python3 tooling/docs/whitepaper/benchmarks/bench.py check campaigns/*\n"
            "python3 tooling/docs/whitepaper/benchmarks/bench.py report campaigns/* --phase "
            + records[0]["phase"]
            + " --output regenerated --no-plots\n```\n\n"
            "Install report-requirements.txt to regenerate SVG/PNG figures. SDK/native binaries and dependency archives "
            "are omitted; their recorded hashes and plan/platform identities remain. Raw data, first-party source archives, "
            "logs and numerical policies are retained. This verifies derivation, not access to the original SDK bytes. "
            "To rerun timings, extract a campaign source.tar.gz into a new checkout, supply a licensed Rack SDK, "
            "acquire the pinned FFTW dependency if requested, and follow the included workflow guide. "
            "System Accelerate and Rack SDK binaries are not redistributed. Hardware timings will differ.\n"
        )
        manifest = dict(
            schema=1,
            kind="fourier-audit-bundle-v1",
            campaigns=records,
            files=file_map(root),
        )
        save(root / "bundle.json", manifest)
        output.parent.mkdir(parents=True, exist_ok=True)
        with output.open("xb") as stream, tarfile.open(
            fileobj=stream, mode="w:gz"
        ) as archive:
            for p in sorted(root.rglob("*")):
                if p.is_file():
                    archive.add(
                        p, arcname=p.relative_to(root).as_posix(), recursive=False
                    )


def unpack(bundle, output):
    if output.exists():
        raise ValueError("Extraction requires a new directory")
    with tarfile.open(bundle) as archive:
        members = archive.getmembers()
        names = [m.name for m in members]
        if len(set(names)) != len(names):
            raise ValueError("Duplicate bundle member")
        for m in members:
            p = PurePosixPath(m.name)
            if not m.isfile() or p.is_absolute() or ".." in p.parts or "\\" in m.name:
                raise ValueError("Unsafe/non-file bundle member")
        manifest = json.load(archive.extractfile("bundle.json"))
        if (
            manifest.get("kind") != "fourier-audit-bundle-v1"
            or manifest.get("schema") != 1
        ):
            raise ValueError("Unknown bundle format")
        if set(names) != set(manifest["files"]) | {"bundle.json"}:
            raise ValueError("Incomplete bundle inventory")
        import hashlib

        for name, expected in manifest["files"].items():
            if hashlib.sha256(archive.extractfile(name).read()).hexdigest() != expected:
                raise ValueError("Bundle checksum mismatch")
        output.mkdir(parents=True)
        for m in members:
            path = output / m.name
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open("xb") as stream:
                shutil.copyfileobj(archive.extractfile(m), stream)
    for item in manifest["campaigns"]:
        path = output / item["path"]
        if output.resolve() not in path.resolve().parents:
            raise ValueError("Invalid packaged campaign path")
        evidence(path, item["phase"])
    return manifest

"""Explicit, non-destructive-first retirement of superseded campaign directories."""

# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from paths import ROOT
from report import identity
from study import write_new, confirmations
from run import digest
from bundles import file_map, unpack
from publication import verify_export


def report_references(root):
    """Scan large reports once with bounded memory; no evidence is modified."""
    found = {}
    pattern = re.compile(rb'"metadata_sha256"\s*:\s*"([a-f0-9]{64})"')
    for report in (root / ".build").rglob("evidence.json"):
        tail = b""
        with report.open("rb") as stream:
            while chunk := stream.read(1024 * 1024):
                chunk = tail + chunk
                for match in pattern.finditer(chunk):
                    fingerprint = match[1].decode("ascii")
                    found.setdefault(fingerprint, set()).add(report)
                tail = chunk[-256:]
    return found


def references(path, root=ROOT, reports=None):
    # Both full relative paths and directory basenames count as retained uses.
    try:
        files = subprocess.check_output(
            ["git", "ls-files"], cwd=root, text=True
        ).splitlines()
    except subprocess.CalledProcessError:
        files = []
    terms = {str(path), path.name}
    try:
        terms.add(str(path.relative_to(root)))
    except ValueError:
        pass
    found = []
    for name in files:
        p = root / name
        if (
            p.suffix not in (".md", ".tex", ".py", ".json", ".cpp", ".hpp", ".mk")
            or not p.is_file()
        ):
            continue
        if any(t in p.read_text(errors="replace") for t in terms):
            found.append(name)
    metadata = path / "metadata.json"
    if metadata.exists():
        fingerprint = digest(metadata)
        if reports is None:
            reports = report_references(root)
        for report in reports.get(fingerprint, set()):
            if path == report.parent or path in report.parents:
                continue
            found.append(str(report.relative_to(root)))
    return sorted(set(found))


def candidate(path, root=ROOT, reports=None):
    if path.is_symlink():
        raise ValueError("Symlink retirement candidates are unsupported")
    path = path.resolve()
    allowed = (root / ".build").resolve()
    if allowed not in path.parents or path == allowed or path.is_symlink():
        raise ValueError(
            "Retirement is restricted to explicit campaign directories beneath .build"
        )
    if not (path / "metadata.json").is_file():
        raise ValueError(
            "Candidate is not a campaign; reports/dependencies are not inferred disposable"
        )
    m = json.loads((path / "metadata.json").read_text())
    if m.get("phase") not in ("pilot", "confirmation"):
        raise ValueError("Only superseded pilot/confirmation campaigns can be retired")
    return dict(
        path=str(path),
        phase=m["phase"],
        status=m.get("status"),
        metadata_sha256=digest(path / "metadata.json"),
        source_sha256=m.get("source_sha256"),
        dependencies=m.get("external_dependency_sha256"),
        config_file_sha256=m.get("config_file_sha256"),
        campaign_manifest=m.get("campaign_manifest"),
        tree_sha256=identity(file_map(path)),
        references=references(path, root, reports),
    )


def inventory_plan(candidates, output, root=ROOT):
    reports = report_references(root)
    rows = [candidate(p, root, reports) for p in candidates]
    if len({r["path"] for r in rows}) != len(rows):
        raise ValueError("Duplicate retirement candidate")
    if any(
        Path(a["path"]) in Path(b["path"]).parents
        for a in rows
        for b in rows
        if a is not b
    ):
        raise ValueError("Nested retirement candidates are unsupported")
    write_new(
        output,
        dict(
            schema=1,
            kind="fourier-retirement-v1",
            candidates=rows,
            reason="Superseded comparison evidence; retained uses must be resolved before execution",
        ),
    )


def retire(plan, replacement, publication, receipt, execute=False, root=ROOT):
    p = json.loads(plan.read_text())
    if p.get("kind") != "fourier-retirement-v1" or p.get("schema") != 1:
        raise ValueError("Invalid retirement plan")
    reports = report_references(root)
    rows = [candidate(Path(r["path"]), root, reports) for r in p["candidates"]]
    if rows != p["candidates"]:
        raise ValueError(
            "Candidate identities or retained references changed; regenerate preview"
        )
    with tempfile.TemporaryDirectory() as temporary:
        extracted = Path(temporary) / "bundle"
        bundle = unpack(replacement, extracted)
        campaigns = [extracted / r["path"] for r in bundle["campaigns"]]
        records = confirmations(campaigns)
    exported = verify_export(publication)
    if exported["fixture"]:
        raise ValueError("Fixture publication cannot authorize evidence retirement")
    expected = {r["metadata_sha256"] for r in bundle["campaigns"]}
    if expected != {s["metadata_sha256"] for s in exported["sources"]}:
        raise ValueError("Replacement bundle and published derivation differ")
    if any(r["references"] for r in rows):
        raise ValueError(
            "Retained manuscript/spec/test uses block retirement; keep these candidates"
        )
    for r in rows:
        path = Path(r["path"])
        if any(
            path == q.resolve() or path in q.resolve().parents
            for q in (replacement, publication, receipt, plan)
        ):
            raise ValueError(
                "Retirement would remove its own replacement, plan or receipt"
            )
    result = dict(
        schema=1,
        status="eligible-preview",
        candidates=rows,
        replacement_sha256=digest(replacement),
        publication_receipt_sha256=digest(publication / "receipt.json"),
        reason=p["reason"],
    )
    if execute:
        # Persist exact identities before mutation; preserve a receipt even if removal is interrupted.
        result["status"] = "removal-started"
        write_new(receipt, result)
        for r in rows:
            shutil.rmtree(r["path"])
        result["status"] = "retired"
        receipt.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    return result

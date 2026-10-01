"""Explicit, provenance-checked selection and LaTeX export; never edits prose."""

# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import csv
import json
import math
import os
from pathlib import Path
import re
import shutil
import tempfile
from paths import ROOT, BENCHMARKS
from run import digest, save
from study import write_new, confirmations
from report import collect, tables, identity


def read_csv(path):
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream))


def checked_report(path):
    manifest = json.loads((path / "manifest.json").read_text())
    for name, expected in manifest.items():
        if Path(name).name != name or digest(path / name) != expected:
            raise ValueError("Changed report artifact: " + name)
    data = json.loads((path / "evidence.json").read_text())
    campaigns = [(path / s["directory"]).resolve() for s in data["sources"]]
    for p, s in zip(campaigns, data["sources"]):
        if digest(p / "metadata.json") != s["metadata_sha256"]:
            raise ValueError("Stale report: campaign metadata changed")
    current = {p.name: digest(p) for p in (BENCHMARKS / "lib").glob("*.py")}
    if data.get("tooling_sha256") != current:
        raise ValueError("Stale report tools; regenerate before selecting/exporting")
    fresh = collect(campaigns, data["phase"])
    if data.get("schema") != fresh["schema"]:
        raise ValueError("Report schema disagrees with checked workloads")
    if fresh["schema"] == 2:
        def scheduling(document):
            return {(r["stratum"], identity(r["config"]), p["session"], p["repeat"]): p.get("scheduling")
                    for r in document["records"] for p in r["processes"]}
        if scheduling(data) != scheduling(fresh):
            raise ValueError("Stale or edited scheduling evidence")
    # Re-derive the source of numeric claims; checksums alone do not establish a statistic.
    with tempfile.TemporaryDirectory() as temporary:
        tables(fresh, Path(temporary))
        extra = tuple("scheduling-"+name+".csv" for name in ("processes", "analyzers", "hops", "phases", "budgets", "sessions")) if fresh["schema"] == 2 else ()
        for name in (
            "results.csv",
            "accuracy-coverage.csv",
            "transitions-responses.csv",
            "transitions-callback-costs.csv",
            "transitions-publications.csv",
        )+extra:
            # Path columns differ after relocation; compare numeric/identity columns.
            old = read_csv(path / name)
            new = read_csv(Path(temporary) / name)
            strip = lambda rows: [
                {k: v for k, v in r.items() if k != "raw"} for r in rows
            ]
            if strip(old) != strip(new):
                raise ValueError("Stale or edited derived numeric table: " + name)
    return data, campaigns


def letters(index):
    result = ""
    while True:
        result = chr(65 + index % 26) + result
        index = index // 26 - 1
        if index < 0:
            return result


def selection_manifest(report, output, fixture=False):
    data, campaigns = checked_report(report)
    if not fixture:
        confirmations(campaigns)
    rows = read_csv(report / "results.csv")
    selected = [
        dict(
            config_sha256=r["config_sha256"],
            stratum=r["stratum"],
            statistic="mean_session_cost",
            units=r["cost_unit"],
            macro="StudyCost" + letters(i),
        )
        for i, r in enumerate(rows)
    ]
    figures = [
        dict(
            file=f["preview"],
            sha256=digest(report / f["preview"]),
            kind=f["kind"],
            workloads=[identity(c) for c in f["workloads"]],
            stratum=f["stratum"],
        )
        for f in data["figures"]
    ]
    value = dict(
        schema=1,
        kind="fourier-selection-v1",
        fixture=fixture,
        phase=data["phase"],
        report=os.path.relpath(report.resolve(), output.resolve().parent),
        report_manifest_sha256=digest(report / "manifest.json"),
        includes=dict(table="results.tex", macros="numbers.tex", figures="figures.tex"),
        rows=selected,
        figures=figures,
        sources=[
            dict(
                metadata_sha256=s["metadata_sha256"],
                source_sha256=s["provenance"]["source_sha256"],
            )
            for s in data["sources"]
        ],
    )
    write_new(output, value)


def tex(value):
    return "".join(
        {
            "\\": r"\textbackslash{}",
            "_": r"\_",
            "%": r"\%",
            "&": r"\&",
            "#": r"\#",
            "$": r"\$",
            "{": r"\{",
            "}": r"\}",
            "^": r"\textasciicircum{}",
            "~": r"\textasciitilde{}",
        }.get(c, c)
        for c in str(value)
    )


def export(selection, output, fixture=False):
    s = json.loads(selection.read_text())
    report = (selection.parent / s["report"]).resolve()
    if (
        s.get("schema") != 1
        or s.get("kind") != "fourier-selection-v1"
        or s.get("fixture") != fixture
    ):
        raise ValueError("Selection/export fixture policy mismatch")
    if digest(report / "manifest.json") != s["report_manifest_sha256"]:
        raise ValueError("Stale selection: report changed")
    data, campaigns = checked_report(report)
    if s["phase"] != data["phase"]:
        raise ValueError("Selection/report phases differ")
    production = (ROOT / "docs/whitepaper/generated").resolve()
    if fixture:
        if (ROOT / "docs/whitepaper").resolve() == output.resolve() or (
            ROOT / "docs/whitepaper"
        ).resolve() in output.resolve().parents:
            raise ValueError("Fixture exports must stay outside the manuscript tree")
    else:
        confirmations(campaigns)
        if production not in output.resolve().parents:
            raise ValueError(
                "Production export belongs under docs/whitepaper/generated/<selection>"
            )
    if s["sources"] != [
        dict(
            metadata_sha256=v["metadata_sha256"],
            source_sha256=v["provenance"]["source_sha256"],
        )
        for v in data["sources"]
    ]:
        raise ValueError("Selection source identities changed")
    if s.get("includes") != dict(
        table="results.tex", macros="numbers.tex", figures="figures.tex"
    ):
        raise ValueError("Unsupported include destinations")
    if not s["rows"] and not s["figures"]:
        raise ValueError("Empty publication selection")
    rows = read_csv(report / "results.csv")
    indexed = {(r["stratum"], r["config_sha256"]): r for r in rows}
    claims = []
    macros = set()
    units = {
        "mean_session_cost": None,
        "max_abs_error": "reference-output amplitude",
        "max_relative_l2": "dimensionless",
        "max_relative_linf": "dimensionless",
        "median_session_p99_us": "us",
        "observed_callback_max_us": "us",
        "publication_delay_ms": "ms",
        "playback_delay_ms": "ms",
        "spectrum_center_age_ms": "ms",
        "cpp_live_heap_bytes_max": "bytes",
    }
    for item in s["rows"]:
        r = indexed[(item["stratum"], item["config_sha256"])]
        stat = item["statistic"]
        macro = item["macro"]
        if stat not in units or not re.fullmatch("[A-Za-z]+", macro) or macro in macros:
            raise ValueError("Unknown statistic or duplicate/invalid macro")
        expected = r["cost_unit"] if units[stat] is None else units[stat]
        if item["units"] != expected:
            raise ValueError("Selection units differ from statistic")
        value = float(r[stat])
        if not math.isfinite(value):
            raise ValueError("Unavailable/non-finite selected statistic")
        macros.add(macro)
        claims.append(
            dict(
                item,
                value=value,
                backend=r["backend"],
                n=r["n"],
                hop=r["hop"],
                block=r["block"],
                cost_session_range=[r["session_min"], r["session_max"]],
                sessions=r["sessions"],
                processes=r["processes"],
            )
        )
    catalog = {f["preview"]: f for f in data["figures"]}
    for f in s["figures"]:
        original = catalog[f["file"]]
        if (
            Path(f["file"]).name != f["file"]
            or digest(report / f["file"]) != f["sha256"]
            or f["kind"] != original["kind"]
            or f["workloads"] != [identity(c) for c in original["workloads"]]
            or f["stratum"] != original["stratum"]
        ):
            raise ValueError("Figure selection/provenance mismatch")
    output.mkdir(parents=True, exist_ok=False)
    label = (
        "FIXTURE ONLY — NOT PAPER EVIDENCE"
        if fixture
        else "Checked frozen confirmation evidence"
    )
    numbers = ["% " + label] + [
        r"\newcommand{" + chr(92) + c["macro"] + "}{" + format(c["value"], ".10g") + "}"
        for c in claims
    ]
    table = [
        "% " + label,
        r"\begingroup\scriptsize",
        r"\begin{longtable}{lllllrr}",
        r"Workload & Backend & N/H/B & Statistic & Units & Value & Sessions \\",
        r"\hline",
    ]
    for c in claims:
        table.append(
            " & ".join(
                [
                    tex(c["config_sha256"][:12]),
                    tex(c["backend"]),
                    f"{c['n']}/{c['hop']}/{c['block']}",
                    tex(c["statistic"]),
                    tex(c["units"]),
                    format(c["value"], ".6g"),
                    c["sessions"],
                ]
            )
            + r" \\"
        )
    table += [r"\end{longtable}", r"\endgroup"]
    figures = ["% " + label]
    for i, f in enumerate(s["figures"]):
        name = "figure-" + str(i) + ".png"
        shutil.copy2(report / f["file"], output / name)
        include_name = (
            name
            if fixture
            else (
                output.resolve().relative_to((ROOT / "docs/whitepaper").resolve())
                / name
            ).as_posix()
        )
        figures += [
            r"\begin{figure}[ht]",
            r"\centering",
            r"\includegraphics[width=\linewidth]{" + include_name + "}",
            r"\caption{" + tex(f["kind"]) + "}",
            r"\end{figure}",
        ]
        # Bound the float queue even when the initial selection contains many plots.
        if (i + 1) % 4 == 0:
            figures.append(r"\clearpage")
    figures.append(r"\clearpage")
    for name, lines in [
        ("numbers.tex", numbers),
        ("results.tex", table),
        ("figures.tex", figures),
    ]:
        (output / name).write_text("\n".join(lines) + "\n")
    include_prefix = (
        ""
        if fixture
        else output.resolve()
        .relative_to((ROOT / "docs/whitepaper").resolve())
        .as_posix()
        + "/"
    )
    (output / ("fixture.tex" if fixture else "preview.tex")).write_text(
        r"\documentclass{article}"
        + "\n"
        + r"\usepackage[margin=1cm]{geometry}"
        + "\n"
        + r"\usepackage{graphicx,longtable}"
        + "\n"
        + r"\begin{document}"
        + "\n"
        + tex(label)
        + "\n"
        + r"\input{"
        + include_prefix
        + "numbers.tex}"
        + "\n"
        + r"\input{"
        + include_prefix
        + "results.tex}"
        + "\n"
        + r"\clearpage"
        + "\n"
        + r"\input{"
        + include_prefix
        + "figures.tex}"
        + "\n"
        + r"\end{document}"
        + "\n"
    )
    shutil.copy2(selection, output / "selection.json")
    receipt = dict(
        schema=1,
        fixture=fixture,
        phase=data["phase"],
        selection_sha256=digest(selection),
        report_manifest_sha256=digest(report / "manifest.json"),
        sources=s["sources"],
        claims=claims,
        figures=s["figures"],
        report=os.path.relpath(report, output.resolve()),
        tooling_sha256=data["tooling_sha256"],
        artifacts={p.name: digest(p) for p in output.iterdir() if p.is_file()},
    )
    save(output / "receipt.json", receipt)
    verify_export(output)


def verify_export(output, fresh=True):
    r = json.loads((output / "receipt.json").read_text())
    if r.get("schema") != 1:
        raise ValueError("Unknown publication receipt")
    required = {
        "selection.json",
        "numbers.tex",
        "results.tex",
        "figures.tex",
        "fixture.tex" if r["fixture"] else "preview.tex",
    }
    if not required <= r.get("artifacts", {}).keys():
        raise ValueError("Incomplete publication asset receipt")
    for name, expected in r["artifacts"].items():
        if Path(name).name != name or digest(output / name) != expected:
            raise ValueError("Stale publication include/asset: " + name)
    if digest(output / "selection.json") != r["selection_sha256"]:
        raise ValueError("Changed publication selection")
    if fresh:
        report = (output / r["report"]).resolve()
        if digest(report / "manifest.json") != r["report_manifest_sha256"]:
            raise ValueError("Stale publication report")
        data, campaigns = checked_report(report)
        if not r["fixture"]:
            confirmations(campaigns)
    return r

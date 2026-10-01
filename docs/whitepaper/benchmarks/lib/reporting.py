"""Complete research reports and relocatable provenance around the shared reporter."""

# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
from collections import defaultdict
import csv
import json
import os
from pathlib import Path
import statistics
from report import report, comparison_key, stationary_records, identity
from hybrid_report import BACKENDS, LIMITATIONS
from run import digest, save
from paths import BENCHMARKS


def attribution(data, output):
    groups = defaultdict(dict)
    for r in stationary_records(data):
        if r["config"]["backend"] in BACKENDS:
            groups[comparison_key(r)][r["config"]["backend"]] = r
    rows = []
    for group, members in sorted(groups.items()):
        missing = sorted(set(BACKENDS) - members.keys())
        if missing:
            rows.append(
                dict(group=group, status="unmatched", missing=",".join(missing))
            )
            continue
        for a, b in (
            (BACKENDS[1], BACKENDS[0]),
            (BACKENDS[4], BACKENDS[3]),
            (BACKENDS[1], BACKENDS[2]),
            (BACKENDS[1], BACKENDS[5]),
        ):
            numerator = {
                (p["session"], p["repeat"]): p["cost"] for p in members[a]["processes"]
            }
            denominator = {
                (p["session"], p["repeat"]): p["cost"] for p in members[b]["processes"]
            }
            if numerator.keys() != denominator.keys():
                raise ValueError("Unmatched attribution process identities")
            for (session, repeat), value in numerator.items():
                base = denominator[(session, repeat)]
                rows.append(
                    dict(
                        group=group,
                        status="paired" if base else "zero-denominator",
                        numerator=a,
                        denominator=b,
                        session=session,
                        repeat=repeat,
                        ratio=value / base if base else None,
                        numerator_config=identity(members[a]["config"]),
                        denominator_config=identity(members[b]["config"]),
                    )
                )
    columns = [
        "group",
        "status",
        "missing",
        "numerator",
        "denominator",
        "session",
        "repeat",
        "ratio",
        "numerator_config",
        "denominator_config",
    ]
    with (output / "attribution.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, columns)
        writer.writeheader()
        writer.writerows(rows)
    (output / "attribution.md").write_text(
        "# Scheduling Attribution\n\n" + LIMITATIONS + "\n\n"
        "See attribution.csv for paired process ratios and explicit unmatched groups. "
        "Missing controls are never substituted. Inspect the same workloads in results.csv "
        "for costs, ages, storage and session ranges.\n"
    )


def supplemental_figures(data, output):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    plt.rcParams.update({"svg.hashsalt": "fourier-evidence-v1", "font.size": 8})
    catalog = []
    groups = defaultdict(list)
    for r in stationary_records(data):
        groups[comparison_key(r)].append(r)
    for key, rows in sorted(groups.items()):
        rows.sort(key=lambda r: r["config"]["backend"])
        fig, axes = plt.subplots(1, 2, figsize=(10, 4))
        labels = [r["config"]["backend"] for r in rows]
        has_norms = any(
            p["accuracy"] and "analysis" in p["accuracy"]
            for r in rows
            for p in r["processes"]
        )
        for i, r in enumerate(rows):
            heaps = [
                v["measurements"]["allocation"]["setup"]["live_bytes"]
                for v in r["resources"]
            ]
            axes[0].plot(i, statistics.mean(heaps) / 1024, ".")
            axes[0].vlines(i, min(heaps) / 1024, max(heaps) / 1024)
            norms = [
                p["accuracy"]["analysis"]["max_relative_linf"]
                for p in r["processes"]
                if p["accuracy"] and "analysis" in p["accuracy"]
            ]
            errors = (
                norms
                if has_norms
                else [
                    p["accuracy"]["max_abs_error"]
                    for p in r["processes"]
                    if p["accuracy"]
                ]
            )
            if errors:
                axes[1].plot(i, max(errors), ".")
        for ax in axes:
            ax.set_xticks(range(len(labels)), labels, rotation=30, ha="right")
            ax.grid(alpha=0.2)
        axes[0].set_ylabel("C++ live heap (KiB); native/stack additional")
        axes[1].set_ylabel(
            "Maximum process relative Linf error"
            if has_norms
            else "Maximum absolute error (reference-output amplitude)"
        )
        # A scientific linear axis keeps sub-epsilon ranges visibly labeled.
        axes[1].ticklabel_format(axis="y", style="sci", scilimits=(0, 0))
        c = rows[0]["config"]
        fig.suptitle(
            f"{data['phase'].upper()} | N={c['n']} H={c['hop']} B={c['block']} | {key}"
        )
        name = "storage-accuracy-" + key
        fig.tight_layout()
        fig.savefig(output / (name + ".svg"), metadata={"Date": None})
        fig.savefig(output / (name + ".png"), dpi=120)
        plt.close(fig)
        catalog.append(
            dict(
                file=name + ".svg",
                preview=name + ".png",
                kind="storage and numerical accuracy",
                workloads=[r["config"] for r in rows],
                stratum=rows[0]["stratum"],
            )
        )
    for r in data["records"]:
        if not r["config"].get("transition_suite"):
            continue
        fig, axes = plt.subplots(2, 1, figsize=(8, 5), sharex=True)
        for p in r["processes"]:
            t = p["transition"]["tables"]
            axes[0].plot(
                [x["generation"] for x in t["responses"]],
                [
                    (
                        x["first_publication_latency_ms"]
                        if x["first_publication_latency_ms"] is not None
                        else float("nan")
                    )
                    for x in t["responses"]
                ],
                ".-",
                alpha=0.6,
            )
            axes[1].plot(
                [x["generation"] for x in t["costs"]],
                [x["observed_max_ns"] / 1000 for x in t["costs"]],
                ".-",
                alpha=0.6,
            )
        axes[0].set_ylabel("Request to publication (ms)")
        axes[1].set(
            xlabel="Requested generation; gaps = no response",
            ylabel="Event-window observed max (us)",
        )
        for ax in axes:
            ax.grid(alpha=0.2)
        c = r["config"]
        fig.suptitle(
            f"{data['phase'].upper()} | {c['backend']} | N={c['n']} H={c['hop']} | no-change={c['transition_control']}"
        )
        name = "response-" + identity(c)[:16]
        fig.tight_layout()
        fig.savefig(output / (name + ".svg"), metadata={"Date": None})
        fig.savefig(output / (name + ".png"), dpi=120)
        plt.close(fig)
        catalog.append(
            dict(
                file=name + ".svg",
                preview=name + ".png",
                kind="parameter response and event cost",
                workloads=[c],
                stratum=r["stratum"],
            )
        )
    return catalog


def derive(campaigns, output, phase, plots=True):
    count = report(campaigns, output, phase, plots)
    data = json.loads((output / "evidence.json").read_text())
    attribution(data, output)
    if plots:
        data["figures"] += supplemental_figures(data, output)
    # Only filesystem locations become relative. SDK/compiler strings remain exact provenance.
    roots = [p.resolve() for p in campaigns]

    def relocate(item):
        if isinstance(item, dict):
            for k, v in item.items():
                if (
                    k in ("directory", "raw", "path")
                    and isinstance(v, str)
                    and Path(v).is_absolute()
                    and any(Path(v) == p or p in Path(v).parents for p in roots)
                ):
                    item[k] = os.path.relpath(v, output.resolve())
                else:
                    relocate(v)
        elif isinstance(item, list):
            for v in item:
                relocate(v)

    relocate(data)
    data["tooling_sha256"] = {
        p.name: digest(p) for p in sorted((BENCHMARKS / "lib").glob("*.py"))
    }
    data["audit_only_dependencies"] = any(
        (p / "bundle-omissions.json").exists() for p in campaigns
    )
    save(output / "evidence.json", data)
    save(
        output / "manifest.json",
        {
            p.name: digest(p)
            for p in sorted(output.iterdir())
            if p.is_file() and p.name != "manifest.json"
        },
    )
    return count

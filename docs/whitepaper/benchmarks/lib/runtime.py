#!/usr/bin/env python3
"""Explain campaign wall time without changing the benchmark measurements."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import datetime as dt
import json
import math
from pathlib import Path
import time


class RuntimeProfile:
    """Disjoint, coarse runner phases, including the phase that fails."""

    def __init__(self, clock=time.perf_counter_ns):
        self.clock = clock
        self.started = self.previous = clock()
        self.phase = "provenance"
        self.context = {}
        self.events = []

    def switch(self, phase, **context):
        now = self.clock()
        self.events.append(dict(phase=self.phase, ns=now-self.previous, **self.context))
        self.previous, self.phase, self.context = now, phase, context

    def finish(self, status):
        self.switch("finished")
        phases = {}
        for event in self.events:
            phases[event["phase"]] = phases.get(event["phase"], 0) + event["ns"]
        return dict(schema=1, unit="ns", status=status,
                    total_ns=self.previous-self.started, phases_ns=phases,
                    events=self.events, finished_utc=dt.datetime.now(dt.timezone.utc).isoformat(),
                    boundary="After configuration resolution through final evidence validation; "
                             "excludes final telemetry save/print and later report generation")


def validate_profile(profile, allow_failed=False):
    """Reject corrupt diagnostic records; legacy archives may omit them."""
    if (not isinstance(profile, dict) or type(profile.get("schema")) is not int
            or profile.get("schema") != 1 or profile.get("unit") != "ns"
            or profile.get("status") not in (("complete", "failed") if allow_failed else ("complete",))):
        raise ValueError("Invalid runtime profile schema, unit or status")
    phases = profile.get("phases_ns")
    if not isinstance(phases, dict) or not phases or not all(isinstance(k, str) and k for k in phases):
        raise ValueError("Invalid runtime phases")
    values = [profile.get("total_ns"), *phases.values()]
    if not all(type(v) in (int, float) and math.isfinite(v) and v >= 0 for v in values):
        raise ValueError("Invalid runtime duration")
    if not math.isclose(sum(phases.values()), profile["total_ns"], rel_tol=1e-9, abs_tol=1):
        raise ValueError("Runtime phases do not partition total time")
    if "events" in profile:
        if not isinstance(profile["events"], list):
            raise ValueError("Invalid runtime events")
        totals = {}
        for event in profile["events"]:
            if not isinstance(event, dict):
                raise ValueError("Invalid runtime event")
            name, duration = event.get("phase"), event.get("ns")
            if not isinstance(name, str) or name not in phases or type(duration) is not int or duration < 0:
                raise ValueError("Invalid runtime event")
            totals[name] = totals.get(name, 0) + duration
        if totals != phases:
            raise ValueError("Runtime events disagree with phase totals")


def format_runtime(directory, metadata=None):
    """Human-readable diagnostics; native phases nest within subprocess wall time."""
    if metadata is None:
        metadata = json.loads((directory/"metadata.json").read_text())
    profile = metadata.get("runtime")
    if profile is None:
        return "This archive predates campaign runtime accounting."
    validate_profile(profile, allow_failed=True)
    total = profile["total_ns"]
    boundary = "through evidence validation" if profile["status"] == "complete" else "stopped on failure"
    lines = [f"Campaign wall time: {total/1e9:.3f} s ({boundary})",
             "Runner phases (disjoint):"]
    for phase, duration in sorted(profile["phases_ns"].items(), key=lambda item: -item[1]):
        lines.append(f"  {phase:24s} {duration/1e9:10.3f} s  {100*duration/total if total else 0:6.2f}%")
    native = {}
    for run in metadata.get("runs", []):
        if "runtime" not in run:
            continue
        child = json.loads((directory/run["runtime"]).read_text())
        validate_profile(child)
        for phase, duration in child["phases_ns"].items():
            native[phase] = native.get(phase, 0) + duration
    if native:
        subprocess_ns = profile["phases_ns"].get("benchmark_process", 0)
        lines.append("Native phases (inside benchmark_process; do not add to runner totals):")
        for phase, duration in sorted(native.items(), key=lambda item: -item[1]):
            lines.append(f"  {phase:24s} {duration/1e9:10.3f} s")
        gap = subprocess_ns - sum(native.values())
        label = "process launch/exit gap" if profile["status"] == "complete" else "unattributed/failed process"
        lines.append(f"  {label:24s} {gap/1e9:10.3f} s")
    measured = sum(group["total_ns"] for run in metadata.get("runs", [])
                   for kind, group in run["summary"]["groups"].items() if kind != "timer")
    lines += [f"Retained measured intervals: {measured/1e9:.6f} s (nested, not an additional phase)",
              "Diagnostic wall times are not algorithm timings or speedup evidence.",
              profile["boundary"] + "."]
    return "\n".join(lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    print(format_runtime(args.directory))

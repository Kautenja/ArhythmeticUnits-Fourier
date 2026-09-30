"""Local execution contracts, sleep protection and bounded preparation gates."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import csv
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
import time
import uuid

POLICY = dict(schema=1, regime="continuous", thread_policy="inherit", fpu_policy="inherit",
              session_settle_seconds=180, process_settle_ms=1000, throughput_chunks=8,
              overrun="catch-up-no-drop-no-rebase", warmup="after-process-settling",
              conditioning="outside-compute-inside-release-to-finish")
ENVIRONMENT = ("PAPER_EXECUTION_PATH", "PAPER_EXECUTION_REGIME", "PAPER_THREAD_POLICY",
               "PAPER_FPU_POLICY", "PAPER_THROUGHPUT_CHUNKS", "PAPER_PROCESS_SETTLE_MS",
               "PAPER_SESSION_SETTLE_SECONDS")


def validate_policy(value, configs=()):
    if not isinstance(value, dict) or set(value) != set(POLICY):
        raise ValueError("Missing or unknown execution policy fields")
    for key in ("schema", "thread_policy", "fpu_policy", "overrun", "warmup", "conditioning"):
        if value[key] != POLICY[key] or type(value[key]) is not type(POLICY[key]):
            raise ValueError("Unsupported execution policy: " + key)
    if value["regime"] not in ("continuous", "paced"):
        raise ValueError("Execution regime unavailable; Rack engine needs its Phase 4 harness")
    for key, low, high in (("session_settle_seconds", 180, 3600),
                           ("process_settle_ms", 1, 3600000), ("throughput_chunks", 2, 4096)):
        if type(value[key]) is not int or not low <= value[key] <= high:
            raise ValueError("Invalid execution policy: " + key)
    if value["regime"] == "paced" and any(c["pass_name"] != "callback" for c in configs):
        raise ValueError("Paced execution requires a callback-only workload matrix")
    return value


def policy_from_args(args):
    value = dict(POLICY)
    for key in ("regime", "session_settle_seconds", "process_settle_ms", "throughput_chunks",
                "thread_policy", "fpu_policy"):
        value[key] = getattr(args, "execution_regime" if key == "regime" else key, value[key])
    return validate_policy(value)


def add_arguments(parser):
    parser.add_argument("--execution-regime", choices=("continuous", "paced", "rack-engine"), default="continuous")
    parser.add_argument("--session-settle-seconds", type=int, default=180)
    parser.add_argument("--process-settle-ms", type=int, default=1000)
    parser.add_argument("--throughput-chunks", type=int, default=8)
    parser.add_argument("--thread-policy", choices=("inherit",), default="inherit")
    parser.add_argument("--fpu-policy", choices=("inherit",), default="inherit")


def child_environment(policy, path, base):
    result = dict(base)
    for key in ENVIRONMENT:
        result.pop(key, None)
    result.update(PAPER_EXECUTION_PATH=str(path), PAPER_EXECUTION_REGIME=policy["regime"],
                  PAPER_THREAD_POLICY=policy["thread_policy"], PAPER_FPU_POLICY=policy["fpu_policy"],
                  PAPER_THROUGHPUT_CHUNKS=str(policy["throughput_chunks"]),
                  PAPER_PROCESS_SETTLE_MS=str(policy["process_settle_ms"]),
                  PAPER_SESSION_SETTLE_SECONDS=str(policy["session_settle_seconds"]))
    return result


def capture(command):
    return subprocess.check_output(command, text=True, stderr=subprocess.STDOUT, timeout=15).strip()


def assertion_present(text, pid):
    owned = [line for line in text.splitlines() if re.search(r"\bpid " + str(pid) + r"\(caffeinate\):", line)]
    return all(any(kind in line for line in owned)
               for kind in ("PreventUserIdleSystemSleep", "PreventSystemSleep"))


def power_state(battery, settings):
    if "'AC Power'" not in battery:
        raise ValueError("Benchmark readiness requires AC power")
    match = re.search(r"^\s*lowpowermode\s+(\d+)\s*$", settings, re.M)
    if not match or match.group(1) != "0":
        raise ValueError("Cannot establish Low Power Mode disabled; check local power settings")
    return dict(source="AC", low_power_mode=0)


class ReadinessError(ValueError):
    def __init__(self, message, snapshot):
        super().__init__(message)
        self.snapshot = snapshot


def host_snapshot(owner, read=capture, system=None):
    system = platform.system() if system is None else system
    result = dict(utc=dt.datetime.now(dt.timezone.utc).isoformat(), platform=system,
                  isolation="User must disconnect networking/Bluetooth and stop agents; not inferred",
                  limitations="Pre/post snapshots cannot rule out transient power changes, thermal throttling or interruptions")
    if system != "Darwin":
        result.update(power="unavailable", sleep_assertions="not-applicable")
        return result
    commands = dict(battery=["pmset", "-g", "batt"], effective=["pmset", "-g"],
                    custom=["pmset", "-g", "custom"], assertions=["pmset", "-g", "assertions"])
    result["raw"] = {}
    try:
        for key, command in commands.items():
            result["raw"][key] = read(command)
        result["power"] = power_state(result["raw"]["battery"], result["raw"]["effective"])
        if not assertion_present(result["raw"]["assertions"], owner):
            raise ValueError("Required caffeinate assertions are missing for the recorded owner")
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        raise ReadinessError(str(error), result) from error
    result["sleep_owner"] = owner
    # Availability is explicit; absence is never evidence of stable clocks.
    for key, command in (("thermal", ["pmset", "-g", "therm"]),
                         ("processes", ["ps", "-axo", "pid,pcpu,comm"])):
        try:
            result[key] = read(command)
        except (OSError, subprocess.SubprocessError) as error:
            result[key] = "unavailable: " + str(error)
    return result


class SleepProtection:
    """Scoped caffeinate process, retained until every measured child has exited."""
    def __init__(self, system=None, spawn=subprocess.Popen, read=capture, sleep=time.sleep):
        self.system = platform.system() if system is None else system
        self.spawn, self.read, self.sleep = spawn, read, sleep
        self.process, self.pid = None, 0
        self.record = dict(schema=1, platform=self.system, command=[], status="incomplete")

    def __enter__(self):
        if self.system != "Darwin":
            self.record["status"] = "not-applicable"
            return self
        command = ["/usr/bin/caffeinate", "-is", "-w", str(os.getpid())]
        self.record.update(command=command, started_utc=dt.datetime.now(dt.timezone.utc).isoformat())
        self.process = self.spawn(command, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL)
        self.pid = self.process.pid
        self.record["pid"] = self.pid
        try:
            for attempt in range(20):
                self.check()
                text = self.read(["pmset", "-g", "assertions"])
                self.record["last_assertions"] = text
                if assertion_present(text, self.pid):
                    self.record.update(status="active", initial_assertions=text)
                    return self
                self.sleep(.1)
            raise ValueError("caffeinate did not establish both assertions within 20 bounded probes")
        except BaseException:
            self.__exit__(*sys.exc_info())
            raise

    def check(self):
        if self.system == "Darwin" and (self.process is None or self.process.poll() is not None):
            self.record["status"] = "lost"
            raise ValueError("caffeinate exited before measurement finished")

    def __exit__(self, kind, value, traceback):
        try:
            if kind is None:
                self.check()
                if self.system == "Darwin":
                    text = self.read(["pmset", "-g", "assertions"])
                    if not assertion_present(text, self.pid):
                        raise ValueError("caffeinate assertions disappeared before completion")
                    self.record.update(status="complete", final_assertions=text)
            elif self.record["status"] != "lost":
                self.record["status"] = "failed"
        except BaseException:
            self.record["status"] = "lost"
            raise
        finally:
            self.record["finished_utc"] = dt.datetime.now(dt.timezone.utc).isoformat()
            if self.process is not None:
                if self.process.poll() is None:
                    self.process.terminate()
                try:
                    self.process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    self.process.wait()


class Gate:
    """Preparation order verified independently of wall time or host performance."""
    def __init__(self, policy, guard, clock=time.monotonic_ns, sleep=time.sleep):
        self.policy, self.guard, self.clock, self.sleep = policy, guard, clock, sleep
        self.state, self.events = "preparing", []

    def prepared(self, binary, expected):
        if self.state != "preparing":
            raise ValueError("Preparation cannot occur after stabilization")
        if hashlib.sha256(binary.read_bytes()).hexdigest() != expected:
            raise ValueError("Prepared executable bytes changed")
        self.events.append(dict(event="prepared", ns=self.clock(), binary_sha256=expected))
        self.state = "prepared"

    def settle(self):
        if self.state != "prepared":
            raise ValueError("Settle only after all preparation/probes/checkpoints")
        self.guard.check()
        start = self.clock()
        self.events.append(dict(event="settle-start", ns=start))
        target = start + self.policy["session_settle_seconds"]*1000000000
        # Bounded wait, no resource probing or repeated search for a favorable run.
        while True:
            remaining = target-self.clock()
            if remaining <= 0:
                break
            self.sleep(min(1, remaining/1e9))
            self.guard.check()
        finish = self.clock()
        if finish > target + 60000000000:
            raise ValueError("Stabilization wait exceeded its sixty-second scheduling allowance")
        self.events.append(dict(event="settle-finish", ns=finish))
        self.state = "ready"

    def launch(self):
        if self.state != "ready":
            raise ValueError("Measurement requested before stabilization")
        self.guard.check()
        self.events.append(dict(event="launch", ns=self.clock()))
        self.state = "launched"


def validate_process(data, policy, config, raw_summary, raw_path=None, sleep_owner=None):
    """Authenticate execution boundaries separately from historical v2 CSV rows."""
    if data.get("schema") != 1 or data.get("status") != "complete" or data.get("fixture"):
        raise ValueError("Invalid execution evidence")
    for key in ("regime", "thread_policy", "fpu_policy", "process_settle_ms", "throughput_chunks"):
        if data.get(key) != policy[key]:
            raise ValueError("Execution sidecar differs from frozen policy: " + key)
    def integer(value, minimum=0):
        if type(value) is not int or value < minimum:
            raise ValueError("Invalid execution coordinate")
        return value
    start, finish = integer(data.get("settle_start_ns")), integer(data.get("settle_finish_ns"))
    if not policy["process_settle_ms"]*1000000 <= finish-start <= policy["process_settle_ms"]*1000000+60000000000:
        raise ValueError("Missing process stabilization")
    integer(data.get("clock_resolution_ns"), 1)
    integer(data.get("sleep_owner"))
    if sleep_owner is not None and data["sleep_owner"] != sleep_owner:
        raise ValueError("Child sleep protection differs from campaign guard")
    for field in ("pre_timer_ns", "post_timer_ns"):
        if not isinstance(data.get(field), list) or len(data[field]) != 1024:
            raise ValueError("Missing pre/post calibration")
        for value in data[field]:
            integer(value)
    threads = data.get("threads")
    if not isinstance(threads, list) or len(threads) != 1:
        raise ValueError("Missing measuring-thread metadata")
    for thread in threads:
        for key in ("before", "after"):
            state = thread[key]
            if state.get("fpu_kind") not in ("arm64-fpcr", "x86-mxcsr-control", "unavailable"):
                raise ValueError("Invalid FPU metadata")
            integer(state.get("fpu_control"))
            for field in ("scheduler", "priority", "qos"):
                integer(state.get(field), -1)
        if any(thread["before"][key] != thread["after"][key] for key in ("fpu_kind", "fpu_control")):
            raise ValueError("FPU control changed")
    rows, mode = data.get("observations"), config["pass_name"]
    expected = (config["callbacks"] if mode == "callback" else
                min(config["callbacks"], policy["throughput_chunks"]) if mode == "throughput" else 0)
    if not isinstance(rows, list) or len(rows) != expected:
        raise ValueError("Missing execution observations")
    cursor, previous, total = 0, 0, 0
    for row in rows:
        sample, samples = integer(row.get("sample")), integer(row.get("samples"), 1)
        wake = integer(row.get("wake_ns"))
        began, ended = integer(row.get("start_ns")), integer(row.get("finish_ns"))
        if sample != cursor or wake < previous or began < wake or ended < began:
            raise ValueError("Reordered/overlapping execution observations")
        if mode == "callback" and samples != config["block"]:
            raise ValueError("Changed callback extent")
        if samples % config["block"]:
            raise ValueError("Partial callback in throughput chunk")
        if policy["regime"] == "paced":
            release = (2*sample*1000000000+config["rate"])//(2*config["rate"])
            deadline = (2*(sample+samples)*1000000000+config["rate"])//(2*config["rate"])
            if row.get("release_ns") != release or row.get("deadline_ns") != deadline or wake < release:
                raise ValueError("Invalid paced release/deadline")
        elif row.get("release_ns") != -1 or row.get("deadline_ns") != -1:
            raise ValueError("Continuous execution cannot claim scheduled releases")
        total += ended-began
        cursor, previous = cursor+samples, ended
    if rows and (cursor != config["callbacks"]*config["block"] or
                 total != raw_summary["groups"][mode]["total_ns"]):
        raise ValueError("Execution samples/durations disagree with raw observations")
    if rows and raw_path is not None:
        with raw_path.open(newline="") as stream:
            raw = [r for r in csv.DictReader(stream) if r["kind"] == mode]
        expected_rows = rows if mode == "callback" else [dict(sample=0, samples=cursor, start_ns=0, finish_ns=total)]
        if len(raw) != len(expected_rows):
            raise ValueError("Raw execution row count differs")
        for index, (actual, expected) in enumerate(zip(raw, expected_rows)):
            coordinates = dict(index=index, analyzer=0, sample=expected["sample"], samples=expected["samples"])
            if (any(int(actual.get(key, "")) != value for key, value in coordinates.items()) or
                    float(actual["ns"]) != expected["finish_ns"]-expected["start_ns"]):
                raise ValueError("Raw execution row differs from retained timestamps")


def validate_campaign(metadata):
    """Historical archives remain readable; new sources cannot omit their policy."""
    policy = metadata.get("execution_policy")
    if policy is None:
        if "benchmark/paper/execution.hpp" in metadata.get("source_sha256", {}):
            raise ValueError("New measurement sources require an execution contract")
        if any("execution" in item for item in metadata["runs"]):
            raise ValueError("Undeclared execution sidecar")
        return []
    validate_policy(policy, metadata["configs"])
    events = metadata.get("execution_events", [])
    if [event.get("event") for event in events] != ["prepared", "settle-start", "settle-finish", "launch"]:
        raise ValueError("Missing ordered preparation/stabilization boundaries")
    times = [event.get("ns") for event in events]
    if (any(type(value) is not int or value < 0 for value in times) or times != sorted(times)
            or not policy["session_settle_seconds"]*1000000000 <= times[2]-times[1]
                <= (policy["session_settle_seconds"]+60)*1000000000
            or events[0].get("binary_sha256") != metadata["binary_sha256"]):
        raise ValueError("Invalid stabilization or executable identity")
    guard = metadata.get("sleep_protection", {})
    before, after = metadata.get("host_before", {}), metadata.get("host_after", {})
    if before.get("platform") != after.get("platform") or before.get("power") != after.get("power"):
        raise ValueError("Missing/changed host state")
    if before.get("platform") == "Darwin":
        pid = guard.get("pid")
        if (type(pid) is not int or pid <= 0 or guard.get("status") != "complete"
                or guard.get("command", [])[:3] != ["/usr/bin/caffeinate", "-is", "-w"]):
            raise ValueError("Missing complete caffeinate lifetime")
        for key in ("initial_assertions", "final_assertions"):
            if not assertion_present(guard.get(key, ""), pid):
                raise ValueError("Incomplete sleep assertions")
        for snapshot in (before, after):
            raw = snapshot.get("raw", {})
            if (power_state(raw.get("battery", ""), raw.get("effective", "")) != snapshot["power"]
                    or not assertion_present(raw.get("assertions", ""), pid)):
                raise ValueError("Invalid retained host snapshot")
    elif not before.get("platform") or guard.get("status") != "not-applicable":
        raise ValueError("Unspecified sleep protection availability")
    names = [item.get("execution") for item in metadata["runs"]]
    if any(not isinstance(name, str) or not name or Path(name).name != name or name in (".", "..") for name in names):
        raise ValueError("Missing or unsafe execution sidecar path")
    if len(names) != len(set(names)):
        raise ValueError("Aliased execution sidecars")
    return names


def retain_native_guard(directory, record):
    """Finalize only this wrapper's native output, never a colliding old run."""
    if directory is None or not (directory/"results.json").is_file():
        return
    path = directory/"results.json"
    try:
        native = json.loads(path.read_text())
    except (ValueError, OSError):
        return  # A truncated native manifest cannot qualify as a baseline.
    if native.get("execution_guard_token") != record["token"]:
        return
    (directory/"execution-guard.json").write_text(json.dumps(record, indent=2)+"\n")
    if record["status"] != "complete":
        native.update(status="failed", execution_guard_error=record.get("error", "Incomplete guard"))
        path.write_text(json.dumps(native, indent=2)+"\n")


def main():
    parser = argparse.ArgumentParser(description="Protected local benchmark command; never downloads or builds.")
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--native-development-gate", action="store_true")
    add_arguments(parser)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command:
        parser.error("A command is required")
    policy = policy_from_args(args)
    if args.native_development_gate and ("--development" not in command or command.count("--output") != 1):
        parser.error("Native gate requires the development runner and its output directory")
    native_output = Path(command[command.index("--output")+1]).resolve() if args.native_development_gate else None
    if native_output is not None and native_output.exists():
        parser.error("Native output already exists; use a fresh directory")
    args.record.parent.mkdir(parents=True, exist_ok=True)
    with args.record.open("x") as stream:
        record = dict(status="incomplete", command=command, execution_policy=policy, token=uuid.uuid4().hex)
        guard = SleepProtection()
        record["sleep_protection"] = guard.record
        try:
            with guard:
                record["before"] = host_snapshot(guard.pid)
                gate = Gate(policy, guard)
                executable = Path(command[0]).resolve()
                expected = hashlib.sha256(executable.read_bytes()).hexdigest()
                gate.prepared(executable, expected)
                record["events"] = gate.events
                if not args.native_development_gate:
                    gate.settle()
                    gate.launch()
                env = child_environment(policy, "", dict(os.environ, PAPER_SLEEP_OWNER=str(guard.pid),
                                                          PAPER_GUARD_TOKEN=record["token"]))
                subprocess.run(command, env=env, check=True)
                if args.native_development_gate:
                    manifest = native_output/"results.json"
                    native = json.loads(manifest.read_text())
                    interval = native.get("session_stabilization", {})
                    seconds = interval.get("seconds")
                    duration = interval.get("finish_ns", 0)-interval.get("start_ns", 0)
                    if (native.get("status") != "complete" or native.get("execution_guard_token") != record["token"]
                            or seconds != policy["session_settle_seconds"]
                            or not seconds*1e9 <= duration <= (seconds+60)*1e9):
                        raise ValueError("Missing native post-preflight stabilization")
                    record["native_stabilization"] = interval
                record["after"] = host_snapshot(guard.pid)
                if (record["before"]["power"] != record["after"]["power"] or
                        hashlib.sha256(executable.read_bytes()).hexdigest() != expected):
                    raise ValueError("Power state or executable changed during command")
            record["status"] = "complete"
        except BaseException:
            record.update(status="invalid", error=str(sys.exc_info()[1]))
            if isinstance(sys.exc_info()[1], ReadinessError):
                record["readiness_failure"] = sys.exc_info()[1].snapshot
            raise
        finally:
            json.dump(record, stream, indent=2)
            stream.write("\n")
            retain_native_guard(native_output, record)


if __name__ == "__main__":
    main()

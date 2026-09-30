"""Measurement boundaries using fake clocks, guarded stubs and retained fixtures."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch
from types import SimpleNamespace
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"lib"))
from execution import (POLICY, Gate, SleepProtection, assertion_present, child_environment,
                       host_snapshot, power_state, retain_native_guard,
                       validate_campaign, validate_policy, validate_process)
from paths import ROOT


def assertions(pid=123):
    return (f'pid {pid}(caffeinate): [id] PreventUserIdleSystemSleep named: "fixture"\n'
            f'pid {pid}(caffeinate): [id] PreventSystemSleep named: "fixture"\n')


class Clock:
    def __init__(self):
        self.ns = 1000
    def now(self):
        return self.ns
    def sleep(self, seconds):
        self.ns += round(seconds*1e9)


def process_fixture(regime="continuous", mode="callback"):
    policy = dict(POLICY, regime=regime)
    config = dict(pass_name=mode, callbacks=3, block=48, rate=48000)
    thread = dict(fpu_kind="arm64-fpcr", fpu_control=0, scheduler=0, priority=0, qos=0)
    rows = []
    for i in range(3):
        start = i*1000000
        rows.append(dict(sample=i*48, samples=48, release_ns=start if regime == "paced" else -1,
                         wake_ns=start, start_ns=start+20, finish_ns=start+120,
                         deadline_ns=start+1000000 if regime == "paced" else -1))
    data = dict(schema=1, status="complete", regime=regime, thread_policy="inherit", fpu_policy="inherit",
                process_settle_ms=1000, throughput_chunks=8, settle_start_ns=1, settle_finish_ns=1000000001,
                clock_resolution_ns=1, sleep_owner=0, pre_timer_ns=[0]*1024, post_timer_ns=[2]*1024,
                threads=[dict(before=thread, after=dict(thread))], observations=rows)
    summary = dict(groups={mode: dict(total_ns=300)})
    return policy, config, data, summary


class ExecutionTests(unittest.TestCase):
    def test_unavailable_policies_and_ambient_environment(self):
        self.assertEqual(validate_policy(dict(POLICY)), POLICY)
        for key, value in (("regime", "rack-engine"), ("thread_policy", "performance-core"),
                           ("fpu_policy", "ftz"), ("schema", True), ("session_settle_seconds", 0),
                           ("throughput_chunks", 0), ("process_settle_ms", -1)):
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate_policy(dict(POLICY, **{key: value}))
        with self.assertRaises(ValueError):
            validate_policy(dict(POLICY, regime="paced"), [dict(pass_name="throughput")])
        env = child_environment(POLICY, Path("fixture.json"),
                                dict(PAPER_EXECUTION_REGIME="bogus", PAPER_THREAD_POLICY="realtime", KEEP="yes"))
        self.assertEqual(env["PAPER_EXECUTION_REGIME"], "continuous")
        self.assertEqual(env["PAPER_THREAD_POLICY"], "inherit")
        self.assertEqual(env["KEEP"], "yes")

    def test_power_and_assertions_fail_closed(self):
        self.assertEqual(power_state("Now drawing from 'AC Power'", " lowpowermode 0\n"),
                         dict(source="AC", low_power_mode=0))
        for battery, settings in (("Battery Power", "lowpowermode 0"),
                                  ("'AC Power'", "lowpowermode 1"), ("'AC Power'", "")):
            with self.assertRaises(ValueError):
                power_state(battery, settings)
        self.assertTrue(assertion_present(assertions(), 123))
        self.assertFalse(assertion_present(assertions(), 12))
        self.assertFalse(assertion_present(assertions().splitlines()[0], 123))
        self.assertFalse(assertion_present(assertions().replace("caffeinate", "unrelated"), 123))

    def test_guard_lifetime_and_early_exit(self):
        process = Mock(pid=123)
        process.poll.return_value = None
        spawn, read = Mock(return_value=process), Mock(return_value=assertions())
        with SleepProtection(system="Darwin", spawn=spawn, read=read) as guard:
            guard.check()
            self.assertEqual(guard.record["status"], "active")
        self.assertEqual(guard.record["status"], "complete")
        process.terminate.assert_called_once()
        process.wait.assert_called_once_with(timeout=5)
        self.assertEqual(spawn.call_args.args[0][:3], ["/usr/bin/caffeinate", "-is", "-w"])
        process.reset_mock(); process.poll.return_value = 1
        with self.assertRaisesRegex(ValueError, "exited"):
            with SleepProtection(system="Darwin", spawn=spawn, read=read):
                self.fail("Lost sleep protection reached the body")
        process.wait.assert_called_once()

    def test_missing_assertions_have_bounded_wait_and_cleanup(self):
        process = Mock(pid=123); process.poll.return_value = None
        sleep = Mock()
        with self.assertRaisesRegex(ValueError, "20 bounded probes"):
            with SleepProtection(system="Darwin", spawn=Mock(return_value=process),
                                 read=Mock(return_value=""), sleep=sleep):
                self.fail("Unprotected execution")
        self.assertEqual(sleep.call_count, 20)
        process.terminate.assert_called_once()

    def test_gate_orders_preparation_and_detects_mutated_bytes(self):
        with tempfile.TemporaryDirectory() as temp:
            binary = Path(temp)/"fixture.bin"; binary.write_bytes(b"prepared fixture")
            digest = hashlib.sha256(binary.read_bytes()).hexdigest()
            clock, guard = Clock(), Mock()
            gate = Gate(POLICY, guard, clock.now, clock.sleep)
            with self.assertRaises(ValueError): gate.launch()
            with self.assertRaises(ValueError): gate.settle()
            binary.write_bytes(b"changed")
            with self.assertRaisesRegex(ValueError, "bytes changed"): gate.prepared(binary, digest)
            binary.write_bytes(b"prepared fixture")
            gate.prepared(binary, digest); gate.settle(); gate.launch()
            self.assertEqual([e["event"] for e in gate.events],
                             ["prepared", "settle-start", "settle-finish", "launch"])
            self.assertEqual(gate.events[2]["ns"]-gate.events[1]["ns"], 180000000000)
            with self.assertRaises(ValueError): gate.prepared(binary, digest)
            with self.assertRaises(ValueError): gate.settle()
            with self.assertRaises(ValueError): gate.launch()

    def test_valid_sidecars_and_corrupt_timestamps_calibration_and_modes(self):
        for regime in ("continuous", "paced"):
            policy, config, data, summary = process_fixture(regime)
            validate_process(data, policy, config, summary)
            mutations = [lambda d: d.update(status="failed"), lambda d: d.update(fixture=True),
                         lambda d: d["observations"].pop(),
                         lambda d: d["observations"][1].update(sample=0),
                         lambda d: d["observations"][0].update(finish_ns=-1),
                         lambda d: d["observations"][0].update(wake_ns=100),
                         lambda d: d["observations"][0].update(deadline_ns=23),
                         lambda d: d["pre_timer_ns"].pop(),
                         lambda d: d["post_timer_ns"].append(2),
                         lambda d: d.update(settle_finish_ns=5),
                         lambda d: d["threads"][0]["after"].update(fpu_control=1)]
            for mutate in mutations:
                broken = copy.deepcopy(data); mutate(broken)
                with self.subTest(regime=regime, mutate=mutate), self.assertRaises(ValueError):
                    validate_process(broken, policy, config, summary)

    def test_new_sources_cannot_strip_execution_policy(self):
        self.assertEqual(validate_campaign(dict(runs=[], source_sha256={})), [])
        with self.assertRaisesRegex(ValueError, "execution contract"):
            validate_campaign(dict(runs=[], source_sha256={"benchmark/paper/execution.hpp": "fixture"}))

    def test_campaign_checker_authenticates_sidecars_and_rejects_unprotected_runs(self):
        import test_contracts
        from check import check
        from run import digest, save
        from observations import summarize
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            metadata = test_contracts.ContractTests().campaign(root)
            config = metadata["configs"][0]
            policy, _, data, _ = process_fixture()
            data["sleep_owner"] = 123
            data["observations"] = data["observations"][:2]
            for i, row in enumerate(data["observations"]):
                row.update(sample=i*64, samples=64)
            raw = root/"raw.csv"
            raw.write_text("kind,ns,index,analyzer,sample,samples\n"+"timer,20,,,,\n"*1024+
                           "callback,100,0,0,0,64\ncallback,100,1,0,64,64\n")
            metadata["runs"][0].update(summary=summarize(raw, config), execution="execution.json")
            save(root/"execution.json", data)
            metadata["artifact_sha256"].update({name: digest(root/name) for name in ("raw.csv", "execution.json")})
            snapshot = dict(platform="Darwin", power=dict(source="AC", low_power_mode=0),
                            raw=dict(battery="'AC Power'", effective="lowpowermode 0", assertions=assertions()))
            metadata.update(execution_policy=policy, host_before=snapshot, host_after=copy.deepcopy(snapshot),
                sleep_protection=dict(status="complete", pid=123, command=["/usr/bin/caffeinate", "-is", "-w", "999"],
                                      initial_assertions=assertions(), final_assertions=assertions()),
                execution_events=[dict(event="prepared", ns=0, binary_sha256=metadata["binary_sha256"]),
                                  dict(event="settle-start", ns=1), dict(event="settle-finish", ns=180000000001),
                                  dict(event="launch", ns=180000000002)])
            save(root/"metadata.json", metadata)
            self.assertEqual(check(root), 1)
            mutations = [lambda m: m["sleep_protection"].update(status="lost"),
                         lambda m: m["sleep_protection"].update(final_assertions=""),
                         lambda m: m["execution_events"][2].update(ns=2),
                         lambda m: m["execution_events"][0].update(binary_sha256="changed"),
                         lambda m: m["host_after"]["raw"].update(effective="lowpowermode 1"),
                         lambda m: m["artifact_sha256"].pop("execution.json"),
                         lambda m: m["runs"][0].update(execution="raw.csv"),
                         lambda m: m["runs"][0].update(execution="../execution.json"),
                         lambda m: m["runs"].clear()]
            for mutation in mutations:
                altered = copy.deepcopy(metadata); mutation(altered)
                save(root/"metadata.json", altered)
                with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                    check(root)
            save(root/"metadata.json", metadata)
            altered = copy.deepcopy(data); altered["observations"][0]["finish_ns"] += 1
            save(root/"execution.json", altered)
            with self.assertRaisesRegex(ValueError, "checksum"):
                check(root)
            metadata["artifact_sha256"]["execution.json"] = digest(root/"execution.json")
            save(root/"metadata.json", metadata)
            with self.assertRaisesRegex(ValueError, "durations disagree"):
                check(root)

    def test_readiness_failure_retains_diagnostic_inputs(self):
        from execution import ReadinessError
        values = {("pmset", "-g", "batt"): "'AC Power'",
                  ("pmset", "-g"): "System-wide settings unavailable",
                  ("pmset", "-g", "custom"): "fixture",
                  ("pmset", "-g", "assertions"): assertions()}
        with self.assertRaises(ReadinessError) as failed:
            host_snapshot(123, read=lambda command: values[tuple(command)], system="Darwin")
        self.assertEqual(failed.exception.snapshot["raw"]["effective"], values[("pmset", "-g")])
        self.assertIn("transient power changes", failed.exception.snapshot["limitations"])

    def test_failed_native_guard_invalidates_only_its_owned_result(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path = root/"results.json"
            native = dict(status="complete", execution_guard_token="fixture-owner")
            path.write_text(json.dumps(native))
            original = path.read_bytes()
            record = dict(token="unrelated", status="invalid", error="Lost protection")
            retain_native_guard(root, record)
            self.assertEqual(path.read_bytes(), original)
            self.assertFalse((root/"execution-guard.json").exists())
            record["token"] = "fixture-owner"
            retain_native_guard(root, record)
            self.assertEqual(json.loads(path.read_text())["status"], "failed")
            self.assertEqual(json.loads((root/"execution-guard.json").read_text()), record)
            path.write_text(json.dumps(native))
            record["status"] = "complete"
            retain_native_guard(root, record)
            self.assertEqual(json.loads(path.read_text())["status"], "complete")

    def test_native_wrapper_delegates_gate_and_preserves_post_run_failures(self):
        import execution
        for failure in (None, "power", "gate"):
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                binary = root/"fixture.bin"; binary.write_text("never executed")
                output, record_path = root/"native", root/"guard.json"
                argv = ["execution.py", "--record", str(record_path), "--native-development-gate",
                        "--", str(binary), "--development", "--output", str(output)]
                guard = SleepProtection(system="fixture")
                snapshot = dict(platform="fixture", power="unavailable")
                snapshots = [snapshot, dict(snapshot, power="changed") if failure == "power" else snapshot]
                def child(command, env, check):
                    self.assertEqual(command, argv[5:])
                    self.assertTrue(check)
                    self.assertEqual(env["PAPER_SESSION_SETTLE_SECONDS"], "180")
                    output.mkdir()
                    native = dict(status="complete", execution_guard_token=env["PAPER_GUARD_TOKEN"],
                                  session_stabilization=dict(seconds=180, start_ns=100,
                                      finish_ns=200 if failure == "gate" else 180000000100))
                    (output/"results.json").write_text(json.dumps(native))
                with patch.object(sys, "argv", argv), \
                        patch("execution.SleepProtection", return_value=guard), \
                        patch("execution.host_snapshot", side_effect=snapshots), \
                        patch("execution.subprocess.run", side_effect=child), \
                        patch.object(Gate, "settle", side_effect=AssertionError("Native gate must follow preflight")):
                    if failure:
                        with self.assertRaises(ValueError): execution.main()
                    else:
                        execution.main()
                record = json.loads(record_path.read_text())
                self.assertEqual(record["status"], "invalid" if failure else "complete")
                self.assertEqual(json.loads((output/"results.json").read_text())["status"],
                                 "failed" if failure else "complete")
                self.assertEqual(json.loads((output/"execution-guard.json").read_text()), record)

    def test_runner_builds_and_probes_before_settling_and_uses_retained_executable(self):
        import run
        from contracts import load_registry, resolve_contract
        from runtime import RuntimeProfile
        for fail in (False, True):
            with self.subTest(fail=fail), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                source = root/"source"
                source.mkdir()
                (source/"Makefile").write_text("fixture build")
                (source/"plugin.json").write_text("{}")
                rack = root/"rack"; rack.mkdir()
                binary = root/"build/paper"; binary.parent.mkdir()
                output = root/"campaign"; output.mkdir()
                log = []
                clock, guard = Clock(), Mock(pid=0)
                args = SimpleNamespace(rack_dir=rack, cxx="fixture-c++", fftw_prefix=None,
                    enable_vdsp=False, freeze=None, seed=1, repeats=1, notes="fixture",
                    session_id="fixture", host_id="fixture", config=None,
                    execution_policy=dict(POLICY), sleep_guard=guard)
                config = dict(run.BASE, backend="driver", callbacks=3, block=48, warm_hops=0)
                registry = load_registry()
                metadata = {}
                real_gate = Gate
                def gate_factory(policy, owner):
                    gate = real_gate(policy, owner, clock.now, clock.sleep)
                    old_prepared, old_launch = gate.prepared, gate.launch
                    def prepared(path, expected):
                        log.append("prepared")
                        self.assertIn("resources", log)
                        old_prepared(path, expected)
                    def launch():
                        log.append("launch"); old_launch()
                    gate.prepared, gate.launch = prepared, launch
                    return gate
                def build_or_run(command, **kwargs):
                    if command[0] == "make":
                        log.append("build")
                        binary.write_bytes(b"fixture timing executable")
                        binary.with_name("paper-audit").write_bytes(b"fixture audit executable")
                        kwargs["stdout"].write("fixture -c benchmark/paper/benchmark.cpp\n"*2)
                    elif "--verify" in command:
                        log.append("verify")
                    else:
                        self.assertEqual(command[0], str(output/"paper.bin"))
                        self.assertEqual(log[-1], "launch")
                        self.assertNotEqual(command[0], str(binary))
                        # Replacing the shared build output cannot change the
                        # already-retained executable used for this invocation.
                        binary.write_bytes(b"unrelated rebuild")
                        log.append("measurement-stub")
                        kwargs["stdout"].write("kind,ns,index,analyzer,sample,samples\n"+"timer,1,,,,\n"*1024)
                        for i in range(3):
                            kwargs["stdout"].write(f"callback,100,{i},0,{i*48},48\n")
                        if fail:
                            raise ValueError("fixture child failed")
                        _, _, data, _ = process_fixture()
                        Path(kwargs["env"]["PAPER_EXECUTION_PATH"]).write_text(json.dumps(data))
                        Path(kwargs["env"]["PAPER_RUNTIME_PATH"]).write_text(json.dumps(dict(
                            schema=1, status="complete", unit="ns", total_ns=1, phases_ns=dict(fixture=1))))
                def output_of(command, **kwargs):
                    if "--inventory" in command:
                        log.append("inventory"); return json.dumps(registry)
                    if "--describe" in command:
                        log.append("describe"); return json.dumps(resolve_contract(config, registry))
                    self.assertIn("--resources", command)
                    log.append("resources")
                    return "{}"  # Contents are outside this ordering fixture.
                def snapshot(_):
                    log.append("host")
                    return dict(platform="fixture", power="unavailable")
                with patch("run.ROOT", source), patch("run.BINARY", binary), \
                        patch("run.source_inputs", return_value=[source/"Makefile", source/"plugin.json"]), \
                        patch("run.capture", return_value="fixture provenance"), \
                        patch("run.subprocess.run", side_effect=build_or_run), \
                        patch("run.subprocess.check_output", side_effect=output_of), \
                        patch("run.Gate", side_effect=gate_factory), \
                        patch("run.host_snapshot", side_effect=snapshot), \
                        patch("run.platform.platform", return_value="fixture"), \
                        patch("run.platform.processor", return_value="fixture"), \
                        patch("run.platform.system", return_value="fixture"):
                    if fail:
                        with self.assertRaisesRegex(ValueError, "child failed"):
                            run.run_campaign(args, output, [config], [], {}, registry, "smoke", None,
                                             RuntimeProfile(clock.now), metadata)
                        self.assertEqual(metadata["status"], "incomplete")
                        self.assertTrue((output/"workload-0000-repeat-00.csv").is_file())
                        self.assertIn("active_job", metadata)
                    else:
                        run.run_campaign(args, output, [config], [], {}, registry, "smoke", None,
                                         RuntimeProfile(clock.now), metadata)
                        self.assertEqual(metadata["status"], "complete")
                        self.assertEqual(len(metadata["runs"]), 1)
                self.assertLess(log.index("build"), log.index("verify"))
                self.assertLess(log.index("verify"), log.index("prepared"))
                self.assertLess(log.index("resources"), log.index("prepared"))
                self.assertLess(log.index("host"), log.index("prepared"))

    def test_native_fake_clock_fixtures(self):
        with tempfile.TemporaryDirectory() as temp:
            binary = Path(temp)/"verify-execution"
            command = shlex.split(os.environ.get("CXX", "c++"))
            result = subprocess.run(command+["-std=c++11", "-Wall", "-Wextra", "-pedantic", "-pthread",
                           str(ROOT/"test/paper/verify_execution.cpp"), "-o", str(binary)],
                           capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            result = subprocess.run([str(binary)], check=True, capture_output=True, text=True, timeout=20)
            self.assertIn("no measured performance data", result.stdout)

    def test_stream_and_sidecar_use_synthetic_clock_end_to_end(self):
        from generate_registry import generate
        from observations import summarize
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            generate(root/"registry.generated.hpp")
            binary = root/"verify-stream"
            command = shlex.split(os.environ.get("CXX", "c++"))
            result = subprocess.run(command+["-std=c++11", "-Wall", "-Wextra", "-pedantic", "-pthread",
                "-I"+temp, str(ROOT/"test/paper/verify_execution_stream.cpp"), "-o", str(binary)],
                capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            for regime, mode in (("continuous", "callback"), ("paced", "callback"),
                                 ("continuous", "throughput")):
                with self.subTest(regime=regime, mode=mode):
                    path = root/(regime+mode+".json")
                    raw = root/(regime+mode+".csv")
                    policy = dict(POLICY, regime=regime, throughput_chunks=3)
                    env = child_environment(policy, path, os.environ)
                    result = subprocess.run([str(binary), mode], env=env,
                        capture_output=True, text=True, timeout=20)
                    self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
                    raw.write_text(result.stdout)
                    data = json.loads(path.read_text())
                    self.assertTrue(data["fixture"])
                    config = dict(pass_name=mode, callbacks=10, block=17, rate=44100)
                    summary = summarize(raw, config)
                    with self.assertRaisesRegex(ValueError, "Invalid execution evidence"):
                        validate_process(data, policy, config, summary)
                    # Test structural validation independently from the fixture
                    # exclusion above. This modified dict is never stored.
                    validate_process(dict(data, fixture=False), policy, config, summary, raw)
                    self.assertEqual(len(data["observations"]), 3 if mode == "throughput" else 10)
                    self.assertTrue(all(value == 100 for value in data["pre_timer_ns"]+data["post_timer_ns"]))
            failed = root/"failed.json"
            result = subprocess.run([str(binary), "failure"],
                env=child_environment(POLICY, failed, os.environ), capture_output=True, text=True, timeout=20)
            self.assertEqual(result.returncode, 1, result.stdout+result.stderr)
            self.assertIn("Fixture processing failure", result.stderr)
            data = json.loads(failed.read_text())
            self.assertTrue(data["fixture"])
            self.assertEqual(data["status"], "failed")
            self.assertEqual(len(data["observations"]), 1)
            self.assertEqual(data["observations"][0]["samples"], 17)


if __name__ == "__main__":
    unittest.main()

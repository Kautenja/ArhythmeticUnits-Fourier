"""Coarse runtime accounting, archive integrity and failure lifecycle checks."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
from contextlib import redirect_stdout
import io
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from check import check
import run
from run import digest, save
from runtime import RuntimeProfile, format_runtime, validate_profile
import test_contracts


def native_profile():
    return dict(schema=1, status="complete", unit="ns", total_ns=400,
                phases_ns={"setup":100, "measurement":200, "correctness_replay":100})


class RuntimeTests(unittest.TestCase):
    def campaign(self, directory, aggregate=True):
        metadata = test_contracts.ContractTests().campaign(directory)
        metadata["runtime_profile"] = "coarse-wall-v1"
        filename = "workload-0000-repeat-00.runtime.json"
        save(directory/filename, native_profile())
        metadata["runs"][0]["runtime"] = filename
        metadata["artifact_sha256"][filename] = digest(directory/filename)
        if aggregate:
            profile = RuntimeProfile(clock=iter((0, 100, 600, 800)).__next__)
            profile.switch("benchmark_process")
            profile.switch("validation")
            metadata["runtime"] = profile.finish("complete")
        save(directory/"metadata.json", metadata)
        return metadata

    def test_legacy_and_both_telemetry_finalization_states(self):
        for state in ("legacy", "before_aggregate", "complete"):
            with self.subTest(state=state), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                if state == "legacy":
                    metadata = test_contracts.ContractTests().campaign(directory)
                    save(directory/"metadata.json", metadata)
                else:
                    self.campaign(directory, aggregate=state == "complete")
                self.assertEqual(check(directory), 1)

    def test_missing_modified_and_unhashed_sidecars_fail(self):
        for change in ("missing", "modified", "unhashed", "symlink"):
            with self.subTest(change=change), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                metadata = self.campaign(directory)
                name = metadata["runs"][0]["runtime"]
                path = directory/name
                if change == "missing":
                    path.unlink()
                elif change == "modified":
                    path.write_text(path.read_text()+" ")
                elif change == "unhashed":
                    del metadata["artifact_sha256"][name]
                    save(directory/"metadata.json", metadata)
                else:
                    original = directory/"original-runtime.json"
                    path.rename(original)
                    try:
                        path.symlink_to(original.name)
                    except OSError:
                        self.skipTest("This environment cannot create symbolic links")
                with self.assertRaises(ValueError):
                    check(directory)

    def test_policy_and_filename_aliases_fail(self):
        mutations = [lambda m: m.update(runtime_profile="unknown"),
                     lambda m: m.update(runtime_profile=None),
                     lambda m: m.pop("runtime_profile"),
                     lambda m: m["runs"][0].pop("runtime")]
        for filename in ("metadata.json", "raw.csv", "stderr.txt", "resources.json",
                         "source.tar.gz", "../outside.json", ".", "", None):
            mutations.append(lambda m, name=filename: m["runs"][0].update(runtime=name))
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            metadata = self.campaign(directory)
            for index, mutate in enumerate(mutations):
                with self.subTest(mutation=index):
                    altered = copy.deepcopy(metadata)
                    mutate(altered)
                    save(directory/"metadata.json", altered)
                    with self.assertRaises(ValueError):
                        check(directory)
            altered = copy.deepcopy(metadata)
            altered["repeats"] = 2
            altered["runs"].append(dict(altered["runs"][0], repeat=1))
            save(directory/"metadata.json", altered)
            with self.assertRaisesRegex(ValueError, "aliased runtime"):
                check(directory)

    def test_invalid_native_and_aggregate_profiles_fail(self):
        profiles = [None, [], 7,
                    dict(native_profile(), schema=2),
                    dict(native_profile(), unit="us"),
                    dict(native_profile(), status="failed"),
                    dict(native_profile(), phases_ns={}),
                    dict(native_profile(), phases_ns={"":400}),
                    dict(native_profile(), phases_ns=[]),
                    dict(native_profile(), total_ns=401.1),
                    dict(native_profile(), events=None),
                    dict(native_profile(), events=[None]),
                    dict(native_profile(), events=[dict(phase="setup", ns=1)])]
        for value in (float("nan"), float("inf"), -1, True, "400"):
            profiles.append(dict(native_profile(), total_ns=value))
            profiles.append(dict(native_profile(), phases_ns={"measurement":value}))
        for profile in profiles:
            with self.subTest(profile=profile):
                with self.assertRaises(ValueError):
                    validate_profile(profile)
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            metadata = self.campaign(directory)
            name = metadata["runs"][0]["runtime"]
            for placement in ("native", "aggregate"):
                for profile in profiles:
                    with self.subTest(placement=placement, profile=profile):
                        altered = copy.deepcopy(metadata)
                        save(directory/name, profile if placement == "native" else native_profile())
                        altered["artifact_sha256"][name] = digest(directory/name)
                        if placement == "aggregate":
                            altered["runtime"] = profile
                        save(directory/"metadata.json", altered)
                        with self.assertRaises(ValueError):
                            check(directory)

    def test_disjoint_clock_partition_repeated_phases_and_context(self):
        profile = RuntimeProfile(clock=iter((0, 10, 40, 60, 90)).__next__)
        profile.switch("benchmark_process", workload=0, repeat=0)
        profile.switch("metadata_checkpoint")
        profile.switch("benchmark_process", workload=0, repeat=1)
        result = profile.finish("complete")
        validate_profile(result)
        self.assertEqual(result["total_ns"], 90)
        self.assertEqual(result["phases_ns"],
                         dict(provenance=10, benchmark_process=60, metadata_checkpoint=20))
        events = [event for event in result["events"] if event["phase"] == "benchmark_process"]
        self.assertEqual([event["repeat"] for event in events], [0, 1])
        self.assertEqual([event["ns"] for event in events], [30, 30])

    def test_human_summary_keeps_native_and_measurements_nested(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            metadata = self.campaign(directory)
            profile = metadata["runtime"]
            for key in profile["phases_ns"]:
                profile["phases_ns"][key] *= 10000000
            for event in profile["events"]:
                event["ns"] *= 10000000
            profile["total_ns"] *= 10000000
            child = native_profile()
            child["total_ns"] *= 10000000
            child["phases_ns"] = {key:value*10000000 for key, value in child["phases_ns"].items()}
            save(directory/metadata["runs"][0]["runtime"], child)
            text = format_runtime(directory, metadata)
            self.assertIn("Campaign wall time: 8.000 s", text)
            self.assertIn("inside benchmark_process; do not add to runner totals", text)
            self.assertRegex(text, r"process launch/exit gap\s+1\.000 s")
            self.assertIn("Retained measured intervals: 0.000000 s (nested, not an additional phase)", text)
            self.assertNotIn("Campaign wall time: 12.000 s", text)

    def test_failed_summary_does_not_label_unrecorded_child_as_launch_overhead(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            metadata = self.campaign(directory)
            # Only the first child completed and supplied a sidecar. The second
            # failed before its run record was appended, but its wall time remains.
            profile = RuntimeProfile(clock=iter((0, 100, 600, 800, 1500)).__next__)
            profile.switch("benchmark_process", repeat=0)
            profile.switch("metadata_checkpoint")
            profile.switch("benchmark_process", repeat=1)
            metadata["runtime"] = profile.finish("failed")
            metadata["status"] = "invalid"
            validate_profile(metadata["runtime"], allow_failed=True)
            self.assertEqual(metadata["runtime"]["phases_ns"]["benchmark_process"], 1200)
            self.assertEqual(len(metadata["runs"]), 1)
            text = format_runtime(directory, metadata)
            self.assertIn("stopped on failure", text)
            self.assertIn("unattributed/failed process", text)
            self.assertNotIn("process launch/exit gap", text)

    def test_runner_success_and_checker_failure_account_for_final_phase(self):
        for fail in (False, True):
            with self.subTest(fail=fail), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                config = directory/"config.json"
                save(config, [dict(backend="driver", n=128, hop=32, block=32)])
                output = directory/"campaign"
                profile = RuntimeProfile(clock=iter((0, 10, 30, 45)).__next__)

                def campaign(args, target, configs, features, external_inputs, registry,
                             phase, manifest, runtime, metadata):
                    metadata.update(status="complete", runs=[])
                    runtime.switch("artifact_hashes")
                    runtime.switch("validation")
                    if fail:
                        raise ValueError("fixture checker failure")

                argv = ["run.py", str(output), "--config", str(config), "--hops", "2", "--warm-hops", "0"]
                with patch("sys.argv", argv), patch("run.RuntimeProfile", return_value=profile), \
                        patch("run.run_campaign", side_effect=campaign), redirect_stdout(io.StringIO()):
                    if fail:
                        with self.assertRaisesRegex(ValueError, "fixture checker failure"):
                            run.main()
                    else:
                        run.main()
                metadata = json.loads((output/"metadata.json").read_text())
                self.assertEqual(metadata["status"], "invalid" if fail else "complete")
                self.assertEqual(metadata["runtime"]["status"], "failed" if fail else "complete")
                self.assertEqual(metadata["runtime"]["total_ns"], 45)
                self.assertEqual(metadata["runtime"]["phases_ns"],
                                 dict(provenance=10, artifact_hashes=20, validation=15))
                if not fail:
                    self.assertEqual(metadata["finished_utc"], metadata["runtime"]["finished_utc"])
                    validate_profile(metadata["runtime"])

    def test_native_opt_in_completion_and_failure(self):
        source = Path(__file__).resolve().parents[3]/"test/paper/verify_runtime.cpp"
        with tempfile.TemporaryDirectory(prefix="fourier-runtime-") as temp:
            directory = Path(temp)
            binary = directory/"verify"
            command = shlex.split(os.environ.get("CXX", "c++"))
            build = subprocess.run(command + ["-std=c++11", "-O3", "-funsafe-math-optimizations",
                                              "-Wall", "-Wextra", "-pedantic", str(source), "-o", str(binary)],
                                   capture_output=True, text=True, timeout=120)
            self.assertEqual(build.returncode, 0, build.stdout+build.stderr)
            for mode, enabled in (("complete", True), ("failure", True),
                                  ("disabled", True), ("complete", False)):
                with self.subTest(mode=mode, enabled=enabled):
                    sidecar = directory/f"{mode}-{enabled}.json"
                    env = dict(os.environ)
                    env.pop("PAPER_RUNTIME_PATH", None)
                    if enabled:
                        env["PAPER_RUNTIME_PATH"] = str(sidecar)
                    result = subprocess.run([str(binary), mode], env=env,
                                            capture_output=True, text=True, timeout=120)
                    self.assertEqual(result.returncode, 1 if mode == "failure" else 0,
                                     result.stdout+result.stderr)
                    self.assertEqual(sidecar.exists(), mode == "complete" and enabled)
                    if sidecar.exists():
                        validate_profile(json.loads(sidecar.read_text()))


if __name__ == "__main__":
    unittest.main()

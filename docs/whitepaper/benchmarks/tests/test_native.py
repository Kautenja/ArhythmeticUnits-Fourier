"""All native outputs and metadata, with synthetic clocks only for timing seams."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"lib"))
from paths import ROOT
from contracts import load_registry, resolve_contract
from generate_registry import generate
from check import check, validate_provider_info, validate_synthesis_accuracy
from native import validate_audit, validate_diagnostic
from run import BASE, digest, save
from workloads import expand
from observations import read_observations
from reporting import derive as report
from publication import checked_report, selection_manifest, export, verify_export
from bundles import pack, unpack
from test_scheduling_metrics import campaign


class NativeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rack = Path(os.environ.get("RACK_DIR", ROOT/"../..")).resolve()
        if not (cls.rack/"include/rack.hpp").exists(): raise unittest.SkipTest("Rack headers required")
        cls.temp = tempfile.TemporaryDirectory(prefix="fourier-native-")
        cls.addClassCleanup(cls.temp.cleanup)
        cls.root = Path(cls.temp.name); cls.binary = cls.root/"verify-native"
        cls.features = []; flags = []; libraries = []
        if platform.system() == "Darwin":
            cls.features.append("vdsp"); flags += ["-DPAPER_HAVE_VDSP"]; libraries += ["-framework", "Accelerate"]
        prefix = Path(os.environ.get("FFTW_PREFIX", ROOT/".build/deps/fftw"))
        if (prefix/"include/fftw3.h").exists():
            cls.features.append("fftw"); flags += ["-DPAPER_HAVE_FFTW", "-I"+str(prefix/"include")]
            libraries += [str(prefix/"lib/libfftw3f.a"), str(prefix/"lib/libfftw3.a")]
        cls.registry = load_registry(features=cls.features)
        generate(cls.root/"registry.generated.hpp", cls.features)
        cmd = shlex.split(os.environ.get("CXX", "c++"))+["-std=c++11", "-O3", "-funsafe-math-optimizations", "-DTEST", "-pthread",
            "-I"+str(cls.rack/"include"), "-I"+str(cls.rack/"dep/include"), "-I"+cls.temp.name]+flags+[
            str(ROOT/"test/paper/verify_native_analysis.cpp"), "-L"+str(cls.rack), "-lRack"]+libraries+["-o", str(cls.binary)]
        result = subprocess.run(cmd, text=True, capture_output=True, timeout=180)
        if result.returncode: raise AssertionError(result.stdout+result.stderr)
        cls.env = dict(os.environ, DYLD_LIBRARY_PATH=str(cls.rack), LD_LIBRARY_PATH=str(cls.rack),
                       PAPER_DIAGNOSTIC_EXECUTION_PATH=str(cls.root/"diagnostic-execution.json"))

    def invoke(self, *args):
        result = subprocess.run([str(self.binary), *args], env=self.env, text=True, capture_output=True, timeout=180)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        return result

    def config(self, backend):
        return expand(dict(BASE, workload_schema=3, backend=backend, n=128, hop=37, block=16,
                           count=2, alignment="staggered", callback_offset=3, warm_hops=2, callbacks=20,
                           active_ports=self.registry[backend]["channels"],
                           fixture="independent" if self.registry[backend]["channels"] == 4 else "noise"))

    def test_all_output_references_core_equality_and_cpp_allocations(self):
        self.assertIn("passed without benchmark timing", self.invoke().stdout)

    def test_complete_channel_endpoints_and_provider_contracts(self):
        names = ["pffft-native-batch-float", "pffft-native-hybrid-float", "pffft-native-unordered-hybrid-float"]
        for provider in self.features:
            names += [provider+"-native-batch-float", provider+"-native4-batch-float", provider+"-native4-hybrid-float"]
        for backend in names:
            with self.subTest(backend=backend):
                config = self.config(backend); contract = resolve_contract(config, self.registry)
                result = self.invoke("stream", backend)
                raw = self.root/"raw.csv"; raw.write_text(result.stdout)
                summary, _ = read_observations(raw, config, self.registry)
                report = json.loads(result.stderr)
                validate_synthesis_accuracy(report, config, summary["publication_audit_rows"], self.registry, "spectrum-norms-v1")
                validate_audit(report, config, contract)
                for info in report["provider_instances"]: validate_provider_info(info, self.registry[backend], config)
                trace = json.loads(self.invoke("trace", backend).stdout)
                validate_diagnostic(trace, self.registry, allow_fixture=True)
                changed = copy.deepcopy(report)
                changed["native_audit"]["instances"][0]["channels"][0]["first_endpoint"] += 1
                with self.assertRaises(ValueError): validate_audit(changed, config, contract)
                changed = copy.deepcopy(report["provider_instances"][0]); changed["native_batch_channels"] += 1
                with self.assertRaises(ValueError): validate_provider_info(changed, self.registry[backend], config)
                changed = copy.deepcopy(report); changed["analysis"]["values"] -= 1
                with self.assertRaises(ValueError): validate_synthesis_accuracy(changed, config, summary["publication_audit_rows"], self.registry, "spectrum-norms-v1")

    def test_diagnostics_preserve_units_and_use_fake_clock_only(self):
        backend = "pffft-native-hybrid-float"
        trace = json.loads(self.invoke("trace", backend).stdout)
        timed = json.loads(self.invoke("stages", backend).stdout)
        overhead = json.loads(self.invoke("overhead", backend).stdout)
        execution = json.loads((self.root/"diagnostic-execution.json").read_text())
        self.assertTrue(execution["fixture"])
        self.assertEqual(execution["status"], "complete")
        for data in (trace, timed, overhead):
            self.assertTrue(data["fixture_clock"])
            with self.assertRaises(ValueError): validate_diagnostic(data, self.registry)
            validate_diagnostic(data, self.registry, allow_fixture=True)
        for field, value in (("sample", 1), ("count", 999), ("stage", True)):
            changed = copy.deepcopy(trace); changed["events"][0][field] = value
            with self.assertRaises(ValueError): validate_diagnostic(changed, self.registry, allow_fixture=True)
        changed = copy.deepcopy(trace); changed["events"].pop()
        with self.assertRaises(ValueError): validate_diagnostic(changed, self.registry, allow_fixture=True)
        changed = copy.deepcopy(trace); changed["accuracy"]["values"] -= 1
        with self.assertRaises(ValueError): validate_diagnostic(changed, self.registry, allow_fixture=True)
        expected = [128, 1, 65, 65]
        frames = {}
        for event in trace["events"]:
            self.assertIsNone(event["ns"])
            frame = event["sample"]//37
            units = frames.setdefault(frame, [0]*4)
            stage = event["stage"]
            self.assertEqual(event["first"], units[stage]); units[stage] += event["count"]
            self.assertLessEqual(units[stage], expected[stage])
            self.assertTrue(all(units[i] == expected[i] for i in range(stage)))
        for frame, units in frames.items():
            if (frame+1)*37 <= trace["samples"]: self.assertEqual(units, expected)
        strip = lambda d: [{k: v for k, v in e.items() if k != "ns"} for e in d["events"]]
        self.assertEqual(strip(trace), strip(timed)); self.assertEqual(strip(trace), strip(overhead))
        self.assertTrue(all(e["ns"] == 100 for e in timed["events"]+overhead["events"]))
        self.assertEqual(trace["accuracy"], timed["accuracy"])
        self.assertEqual(trace["accuracy"], overhead["accuracy"])

    def test_native_synthetic_report_export_bundle_and_missing_audit(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); path = root/"campaign"
            metadata = campaign(path)  # Explicit synthetic zero-spectrum archive.
            c = metadata["configs"][0]; c["backend"] = "pffft-native-hybrid-float"
            metadata["contracts"]["0"] = resolve_contract(c)
            info = json.loads(self.invoke("info", c["backend"]).stdout)
            info.update(retained_input_samples_per_channel=c["n"]+c["hop"], publication_delay_samples=c["hop"]-1)
            resources = json.loads((path/"resources.json").read_text())
            for item in resources.values(): item["provider_info"] = info
            save(path/"resources.json", resources)
            accuracy = json.loads((path/"stderr.txt").read_text()); scalar = accuracy.pop("audit")["instances"][0]
            accuracy["provider_instances"] = [info]
            accuracy["native_audit"] = dict(policy="native-all-channels-v1", instances=[dict(
                instance=0, first_sample=scalar["first_sample"], samples=scalar["samples"],
                publications=scalar["checked_spectra"], channels=[dict(channel=0,
                    first_endpoint=scalar["first_endpoint"], last_endpoint=scalar["last_endpoint"],
                    spectra=scalar["checked_spectra"], bins=scalar["checked_bins"])])])
            save(path/"stderr.txt", accuracy)
            metadata["artifact_sha256"].update({name: digest(path/name) for name in ("resources.json", "stderr.txt")})
            save(path/"metadata.json", metadata)
            self.assertEqual(check(path), 1)
            report([path], root/"report", "smoke", False)
            checked_report(root/"report")
            self.assertIn("native-all-channels-v1", (root/"report/accuracy-coverage.csv").read_text())
            selection_manifest(root/"report", root/"selection.json", True)
            export(root/"selection.json", root/"export", True); verify_export(root/"export")
            pack([path], root/"bundle.tar.gz", root/"selection.json")
            unpack(root/"bundle.tar.gz", root/"unpacked"); checked_report(root/"unpacked/report")
            del accuracy["native_audit"]
            save(path/"stderr.txt", accuracy)
            metadata["artifact_sha256"]["stderr.txt"] = digest(path/"stderr.txt")
            save(path/"metadata.json", metadata)
            with self.assertRaises(ValueError): check(path)


if __name__ == "__main__": unittest.main()

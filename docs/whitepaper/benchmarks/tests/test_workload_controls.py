"""Explicit workload contracts and untimed DSP fixtures, never performance data."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
import math
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"lib"))
from paths import ROOT
from workloads import DEFAULTS, FIELDS, expand, alpha
from contracts import validate_config, resolve_contract, load_registry
from run import BASE, command
from profiles import load, measured
from generate_registry import generate


class WorkloadControlsTests(unittest.TestCase):
    def test_explicit_fields_and_rejected_ambiguous_controls(self):
        c = expand(dict(BASE, workload_schema=3, n=128, hop=37, callbacks=20, warm_hops=0))
        validate_config(c, measurement=True)
        self.assertEqual(json.loads(command(c)[-1]), {k: c[k] for k in FIELDS})
        self.assertEqual(resolve_contract(c)["workload"]["window"], "hann")
        for change in (dict(smooth=1), dict(workload_schema=2), dict(window="typo"),
                       dict(fixture_seed=-1), dict(fixture_seed=True), dict(octave=float("nan")),
                       dict(temporal_value=1), dict(temporal_value=.999999999), dict(experimental_policy="future"), dict(active_ports=2),
                       dict(fixture="independent"), dict(state="live")):
            with self.subTest(change=change), self.assertRaises(ValueError):
                validate_config(dict(c, **change))
        with self.assertRaises(ValueError): validate_config(dict(BASE, fixture="silence"))
        before = resolve_contract(BASE)
        self.assertNotIn("workload", before)
        self.assertAlmostEqual(alpha(dict(c, temporal_mode="module-seconds", temporal_value=.1)),
                               math.exp(-10*37/48000/.1), places=7)

    def test_explicit_profile_does_not_append_a_preset(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"explicit.json"
            document = dict(schema=3, seed=7, repeats=2, hops=8, frames=8, warm_hops=0,
                            workloads=[dict(workload_schema=3, fixture="silence", n=128, hop=37)])
            path.write_text(json.dumps(document))
            _, configs, _, options = load(path, "rack", "Darwin")
            self.assertEqual(len(configs), 1)
            self.assertEqual(configs[0]["fixture"], "silence")
            self.assertEqual(len(measured(configs, options, load_registry())), 1)
            document["transitions"] = True
            path.write_text(json.dumps(document))
            with self.assertRaises(ValueError): load(path, "rack", "Darwin")

    def test_supplied_explicit_profile_keeps_modules_and_declares_optional_omissions(self):
        for variant, expected in (("rack", 5), ("macos", 6)):
            _, configs, manifest, _ = load("controls", variant, "Darwin")
            self.assertEqual(len(configs), expected)
            self.assertTrue({"fourier", "spectre"} <= {c["backend"] for c in configs})
            self.assertEqual(manifest["omitted_by_provider"], {"vdsp": 1} if variant == "rack" else {})

    def test_untimed_all_bin_fixtures(self):
        rack = Path(os.environ.get("RACK_DIR", ROOT/"../..")).resolve()
        if not (rack/"include/rack.hpp").exists():
            self.skipTest("Untimed Rack SIMD fixtures require Rack headers")
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            generate(root/"registry.generated.hpp")
            binary = root/"verify-controls"
            compile = shlex.split(os.environ.get("CXX", "c++"))+[
                "-std=c++11", "-O3", "-funsafe-math-optimizations", "-pthread",
                "-I"+str(rack/"include"), "-I"+str(rack/"dep/include"), "-I"+temp,
                str(ROOT/"test/paper/verify_workload_controls.cpp"), "-L"+str(rack), "-lRack", "-o", str(binary)]
            result = subprocess.run(compile, text=True, capture_output=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            env = dict(os.environ, DYLD_LIBRARY_PATH=str(rack), LD_LIBRARY_PATH=str(rack))
            result = subprocess.run([str(binary)], env=env, text=True, capture_output=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            self.assertIn("108 explicit", result.stdout)
            # Exercise the real C++ JSON/contract code with no timing entry point.
            for window in ("hann", "boxcar", "blackman-harris"):
                for mode, value in (("alpha", 0), ("alpha", .8), ("module-seconds", .1), ("module-seconds", 1)):
                    c = expand(dict(BASE, workload_schema=3, n=128, hop=37, block=17, callbacks=20,
                                    warm_hops=0, state="startup", window=window, temporal_mode=mode,
                                    temporal_value=value, octave=1/3, fixture="decay", decay_samples=50))
                    result = subprocess.run([str(binary), json.dumps({k: c[k] for k in FIELDS})],
                                            env=env, text=True, capture_output=True, timeout=20)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(json.loads(result.stdout), resolve_contract(c))

    def test_untimed_module_controls(self):
        rack = Path(os.environ.get("RACK_DIR", ROOT/"../..")).resolve()
        if not (rack/"include/rack.hpp").exists():
            self.skipTest("Untimed module fixtures require Rack headers")
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); generate(root/"registry.generated.hpp")
            binary = root/"verify-modules"
            compile = shlex.split(os.environ.get("CXX", "c++"))+[
                "-std=c++11", "-O3", "-funsafe-math-optimizations", "-DTEST", "-pthread",
                "-I"+str(rack/"include"), "-I"+str(rack/"dep/include"), "-I"+temp,
                str(ROOT/"test/paper/verify_module_workload.cpp"), "-L"+str(rack), "-lRack", "-o", str(binary)]
            result = subprocess.run(compile, text=True, capture_output=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            result = subprocess.run([str(binary)], env=dict(os.environ, DYLD_LIBRARY_PATH=str(rack), LD_LIBRARY_PATH=str(rack)),
                                    text=True, capture_output=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            self.assertIn("passed without timing", result.stdout)


if __name__ == "__main__":
    unittest.main()

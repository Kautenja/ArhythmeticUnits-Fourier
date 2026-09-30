"""Headless Rack/PFFFT provider checks, optional when no Rack SDK is present."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"lib"))
from paths import ROOT, BENCHMARKS, HISTORY

import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import unittest


class PffftTests(unittest.TestCase):
    def test_independent_fixtures(self):
        root = ROOT
        rack = Path(os.environ.get("RACK_DIR", str(root/"../.."))).resolve()
        if not (rack/"include/dsp/fft.hpp").is_file():
            self.skipTest("Rack SDK is unavailable; set RACK_DIR for PFFFT checks")
        with tempfile.TemporaryDirectory(prefix="fourier-pffft-") as temp:
            binary = Path(temp)/("verify.exe" if sys.platform == "win32" else "verify")
            command = shlex.split(os.environ.get("CXX", "c++"))
            command += ["-std=c++11", "-O2", "-Wall", "-Wextra", "-I"+str(rack/"include"),
                        "-I"+str(rack/"dep/include"), str((ROOT/"test/paper/verify_pffft.cpp")),
                        "-L"+str(rack), "-lRack", "-o", str(binary)]
            result = subprocess.run(command, capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            env = dict(os.environ)
            for variable in ("DYLD_LIBRARY_PATH", "LD_LIBRARY_PATH", "PATH"):
                env[variable] = str(rack)+os.pathsep+env.get(variable, "")
            result = subprocess.run([str(binary)], env=env, capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            lines = result.stdout.splitlines()
            self.assertEqual(lines[-1], "PFFFT independent fixtures passed")
            real, complex_plan = (json.loads(line) for line in lines[:2])
            self.assertEqual(real["aligned_io_payload_bytes"], 2*128*4)
            self.assertEqual(complex_plan["aligned_io_payload_bytes"], 4*128*4)
            self.assertEqual(real["source_derived_stack_scratch_payload_bytes"], 128*4)
            self.assertEqual(complex_plan["source_derived_stack_scratch_payload_bytes"], 2*128*4)
            self.assertIsNone(real["native_plan_bytes"])

    def test_long_analysis_streams(self):
        root = ROOT
        rack = Path(os.environ.get("RACK_DIR", str(root/"../.."))).resolve()
        if not (rack/"include/dsp/fft.hpp").is_file():
            self.skipTest("Rack SDK is unavailable")
        from generate_registry import generate
        with tempfile.TemporaryDirectory(prefix="fourier-analysis-") as temp:
            generate(Path(temp)/"registry.generated.hpp")
            binary = Path(temp)/("verify.exe" if sys.platform == "win32" else "verify")
            command = shlex.split(os.environ.get("CXX", "c++"))
            subprocess.run(command+["-std=c++11", "-O3", "-funsafe-math-optimizations", "-I"+temp,
                "-I"+str(rack/"include"), "-I"+str(rack/"dep/include"),
                str((ROOT/"test/paper/verify_analysis_streams.cpp")), "-L"+str(rack), "-lRack", "-o", str(binary)],
                check=True, capture_output=True, text=True, timeout=120)
            env = dict(os.environ)
            for variable in ("DYLD_LIBRARY_PATH", "LD_LIBRARY_PATH", "PATH"):
                env[variable] = str(rack)+os.pathsep+env.get(variable, "")
            result = subprocess.run([str(binary)], env=env, check=True, capture_output=True, text=True, timeout=120)
            rows = [json.loads(line) for line in result.stdout.splitlines()]
            self.assertEqual(len(rows), 18)
            self.assertTrue(all("legacy_pointwise_failures" in r["accuracy"] for r in rows))
            self.assertTrue(any(r["accuracy"]["zero_vectors"] for r in rows))


if __name__ == "__main__":
    unittest.main()

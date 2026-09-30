"""Build and exercise synthesis baselines without Rack, Catch2, or an audio device."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest
from generate_registry import generate


class SynthesisTests(unittest.TestCase):
    def test_independent_waveforms_and_failure_detection(self):
        source = (Path(__file__).resolve().parents[3]/"test/paper/verify_synthesis.cpp")
        with tempfile.TemporaryDirectory(prefix="fourier-synthesis-") as directory:
            binary = Path(directory)/"verify"
            generate(Path(directory)/"registry.generated.hpp")
            command = shlex.split(os.environ.get("CXX", "c++"))
            build = subprocess.run(command + ["-std=c++11", "-O2", "-Wall", "-Wextra",
                                              "-pedantic", "-I"+directory, str(source), "-o", str(binary)],
                                   capture_output=True, text=True, timeout=120)
            self.assertEqual(build.returncode, 0, build.stdout+build.stderr)
            result = subprocess.run([str(binary)], capture_output=True,
                                    text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            self.assertIn("Synthesis baselines passed", result.stdout)


if __name__ == "__main__":
    unittest.main()

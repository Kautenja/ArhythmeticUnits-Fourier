"""Validate the optional benchmark provider without Rack or a timing campaign."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import tempfile
import unittest


class VdspTests(unittest.TestCase):
    @unittest.skipUnless(platform.system() == "Darwin", "vDSP requires macOS Accelerate")
    def test_independent_outputs_and_cpp_allocation_audit(self):
        source = (Path(__file__).resolve().parents[3]/"test/paper/verify_vdsp.cpp")
        with tempfile.TemporaryDirectory(prefix="fourier-vdsp-") as directory:
            binary = Path(directory)/"verify"
            command = shlex.split(os.environ.get("CXX", "c++"))
            build = subprocess.run(command + ["-std=c++11", "-O2", "-Wall", "-Wextra",
                                              "-pedantic", "-DPAPER_HAVE_VDSP",
                                              "-DPAPER_ALLOCATION_AUDIT", str(source),
                                              "-framework", "Accelerate", "-o", str(binary)],
                                   capture_output=True, text=True, timeout=120)
            self.assertEqual(build.returncode, 0, build.stdout+build.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            identity = json.loads(result.stdout.splitlines()[0])
            self.assertEqual(identity["provider"], "Apple Accelerate/vDSP")
            self.assertEqual(identity["persistent_buffer_bytes"], 128*4)
            self.assertEqual(identity["setup_count"], 1)
            self.assertIsNone(identity["setup_bytes"])
            self.assertIsNone(identity["native_execution_allocations"])
            self.assertIn("vDSP references passed", result.stdout)

    def test_disabled_header_has_no_platform_dependency(self):
        provider = Path(__file__).resolve().parents[3]/"benchmark/paper/vdsp.hpp"
        command = shlex.split(os.environ.get("CXX", "c++"))
        result = subprocess.run(command + ["-std=c++11", "-fsyntax-only", "-x", "c++", "-"],
                                input='#include "'+str(provider)+'"\nint main() {}\n',
                                capture_output=True, text=True, timeout=120)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)


if __name__ == "__main__":
    unittest.main()

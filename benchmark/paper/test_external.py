"""Provider-independent negative checks for the shared external evidence paths."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

from generate_registry import generate
from check import validate_provider_info
from contracts import REGISTRY


class ExternalTests(unittest.TestCase):
    def test_nan_output_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            binary = Path(temp)/"verify"
            generate(Path(temp)/"registry.generated.hpp")
            command = shlex.split(os.environ.get("CXX", "c++"))
            subprocess.run(command+["-std=c++11", "-O2", "-I"+temp,
                           str(Path(__file__).with_name("verify_external.cpp")), "-o", str(binary)],
                           check=True, capture_output=True, text=True, timeout=120)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)

    def test_native_provider_evidence_required(self):
        descriptor = REGISTRY["pffft-analysis-float"]
        valid = dict(provider="rack-pffft", precision="float", plan_policy="exact-size", native_plan_bytes=None)
        validate_provider_info(valid, descriptor)
        for value in ({}, dict(valid, provider="fftw"), dict(valid, precision="double"),
                      dict(valid, plan_policy="")):
            with self.assertRaises(ValueError):
                validate_provider_info(value, descriptor)


if __name__ == "__main__":
    unittest.main()

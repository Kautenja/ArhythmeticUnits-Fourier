"""Numerical policy failure detection and retained pointwise diagnostics."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest
from check import validate_synthesis_accuracy
from run import BASE


class AnalysisAccuracyTests(unittest.TestCase):
    def test_scale_silence_corruption_and_diagnostics(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp)/"verify"
            subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + ["-std=c++11", "-O3", "-funsafe-math-optimizations",
                str(Path(__file__).with_name("verify_analysis_accuracy.cpp")), "-o", str(binary)],
                check=True, capture_output=True, text=True)
            result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
            metrics = json.loads(result.stdout)
        self.assertEqual(metrics["legacy_pointwise_failures"], 1)
        # Embed the real diagnostic into a full-size publication report.
        config = dict(BASE, backend="pffft-analysis-float", n=128, callbacks=2)
        metrics.update(values=65)
        report = dict(max_abs_error=abs(metrics["worst_pointwise"]["actual"]-metrics["worst_pointwise"]["reference"]),
                      max_reference=4096, checked_samples=65, publications=1, playback_checked_samples=0,
                      analysis=metrics)
        validate = lambda r: validate_synthesis_accuracy(r, config, 1, required_policy="spectrum-norms-v1")
        validate(report)
        for mutate in (lambda r: r.pop("analysis"),
                       lambda r: r["analysis"].update(policy="unknown"),
                       lambda r: r["analysis"].update(max_relative_l2=.1),
                       lambda r: r["analysis"].update(max_relative_linf=float("nan")),
                       lambda r: r["analysis"].update(legacy_pointwise_failures=0),
                       lambda r: r["analysis"].update(vectors=0),
                       lambda r: r["analysis"].update(tolerance=.1),
                       lambda r: r["analysis"]["worst_pointwise"].update(actual=0)):
            broken = copy.deepcopy(report); mutate(broken)
            with self.assertRaises((ValueError, KeyError)):
                validate(broken)
        old = dict(report); old.pop("analysis")
        validate_synthesis_accuracy(old, config, 1)  # Historical schema preserved.


if __name__ == "__main__":
    unittest.main()

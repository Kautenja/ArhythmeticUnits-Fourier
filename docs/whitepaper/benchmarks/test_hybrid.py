"""Hybrid scheduling, native-call attribution, and retained-frame regressions."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

from generate_registry import generate
from check import validate_hybrid_info
from contracts import resolve_contract
from run import BASE
from hybrid_report import assemble, BACKENDS


class HybridTests(unittest.TestCase):
    def test_retained_frame_dependencies_and_allocation(self):
        with tempfile.TemporaryDirectory() as temp:
            generate(Path(temp)/"registry.generated.hpp")
            binary = Path(temp)/"verify"
            command = shlex.split(os.environ.get("CXX", "c++"))
            subprocess.run(command+["-std=c++11", "-O2", "-DPAPER_ALLOCATION_AUDIT", "-I"+temp,
                           str((Path(__file__).resolve().parents[3]/"test/paper/verify_hybrid.cpp")), "-o", str(binary)],
                           check=True, capture_output=True, text=True, timeout=120)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)

    def test_matched_attribution_and_missing_control(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root/"resources.json").write_text("{}")
            configs = [dict(BASE, backend=name, callbacks=2, warm_hops=0) for name in BACKENDS]
            metadata = dict(configs=configs,
                            resources={str(i): "resources.json" for i in range(6)},
                            contracts={str(i): resolve_contract(c) for i, c in enumerate(configs)},
                            runs=[dict(workload=i, repeat=0, raw=str(i)+".csv",
                                       summary=dict(groups=dict(callback=dict(mean_ns=64*(i+1)))))
                                  for i in range(6)])
            group = assemble(root, metadata)[0]
            self.assertEqual(group["comparisons"][0]["process_ratios"], [2])
            self.assertEqual(group["backends"][BACKENDS[1]]["observations"][0]["ns_per_engine_sample"], 2)
            metadata["configs"] = configs[:-1]
            with self.assertRaisesRegex(ValueError, "all six"):
                assemble(root, metadata)

    def test_contract_and_attribution_evidence(self):
        c = dict(BASE, backend="pffft-hybrid-float", n=128, hop=37)
        contract = resolve_contract(c)
        self.assertEqual(contract["publication_delay_samples"], 36)
        self.assertIsNone(contract["step_count"])
        self.assertEqual(contract["step_model"], "opaque")
        valid = dict(mode="hybrid", task_units=259, prepare_units=128,
                     native_calls_per_frame=1, magnitude_units=65, output_units=65,
                     retained_input_samples=165, fft_sample_offset=18, publication_delay_samples=36,
                     cost_model="unequal tasks; native FFT including conversion is indivisible")
        validate_hybrid_info(dict(analysis_schedule=valid), c)
        for key, value in (("fft_sample_offset", 0), ("publication_delay_samples", 0),
                           ("retained_input_samples", 128), ("native_calls_per_frame", 2)):
            with self.assertRaises(ValueError):
                validate_hybrid_info(dict(analysis_schedule=dict(valid, **{key: value})), c)
        batch = resolve_contract(dict(c, backend="pffft-scheduled-batch-float"))
        self.assertEqual(batch["publication_delay_samples"], 0)


if __name__ == "__main__":
    unittest.main()

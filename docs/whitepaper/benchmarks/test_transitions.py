"""Positive/negative transition replay, manifest and event-cost table fixtures."""
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

from generate_registry import generate
from transitions import expected_publications, manifest, transition_tables, validate_transition_trace

ROOT = Path(__file__).resolve().parents[3]


def configuration():
    return dict(backend="core-float", pass_name="callback", n=128, hop=16,
                block=16, callbacks=26, count=1, voices=1, rate=48000,
                state="startup", alignment="aligned", load=0, smooth=0,
                cache_mib=0, warm_hops=0, callback_offset=0,
                transition_suite="interactive-v1", transition_control=False)


class TransitionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory()
        cls.root = Path(cls.directory.name)
        generate(cls.root/"registry.generated.hpp")
        binary = cls.root/"transitions"
        command = shlex.split(os.environ.get("CXX", "c++")) + ["-std=c++11", "-O3", "-funsafe-math-optimizations", "-I", str(cls.root),
            str(ROOT/"test/paper/verify_transitions.cpp"), "-o", str(binary)]
        subprocess.run(command, check=True, capture_output=True, text=True, timeout=120)
        cls.trace_path, cls.raw_path = cls.root/"transition.json", cls.root/"raw.csv"
        with cls.raw_path.open("w") as output:
            subprocess.run([str(binary)], check=True, stdout=output, stderr=subprocess.PIPE,
                           env=dict(os.environ, PAPER_TRANSITION_PATH=str(cls.trace_path)), text=True, timeout=120)
        cls.trace = json.loads(cls.trace_path.read_text())

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def test_production_replay_and_response_cost_tables(self):
        validate_transition_trace(self.trace, configuration())
        tables = transition_tables(self.trace, configuration(), self.raw_path)
        self.assertEqual(len(tables["responses"]), 11)
        self.assertEqual(tables["responses"][0]["application_latency_samples"], 0)
        self.assertEqual(tables["responses"][0]["first_publication_latency_samples"], 15)
        self.assertEqual(tables["responses"][6]["outcome"], "replaced")
        self.assertEqual(tables["responses"][-1]["outcome"], "pending-no-response")
        self.assertIsNone(tables["responses"][-1]["first_publication_latency_ms"])
        self.assertTrue(all(row["callback_indices"] for row in tables["costs"]))
        self.assertTrue(all(row["mean_ns"] <= row["observed_max_ns"] for row in tables["costs"]))
        self.assertEqual(len(tables["publications"]), len(expected_publications(configuration())))

    def test_negative_identity_coverage_and_numerical_fixtures(self):
        mutations = [lambda t: t["events"].pop(), lambda t: t["publications"].pop(),
                     lambda t: t["events"][0].update(application_sample=33),
                     lambda t: t["events"][6].update(replaced_by=-1),
                     lambda t: t["publications"][0].update(generation=1),
                     lambda t: t["publications"][0].update(endpoint=1),
                     lambda t: t["publications"][0].update(publication_sample=14),
                     lambda t: t["publications"][0].update(history_start=1),
                     lambda t: t["publications"][0].update(bins=64),
                     lambda t: t["publications"][0]["accuracy"].update(values=64),
                     lambda t: t["publications"][0]["accuracy"].update(max_relative_l2=.1),
                     lambda t: t["publications"][0]["accuracy"].update(max_relative_linf=float("nan")),
                     lambda t: t["publications"][0]["accuracy"]["worst_pointwise"].update(actual=float("inf")),
                     lambda t: t.update(memory_policy="")]
        for mutation in mutations:
            with self.subTest(mutation=mutation):
                broken = copy.deepcopy(self.trace)
                mutation(broken)
                with self.assertRaises(ValueError):
                    validate_transition_trace(broken, configuration())

    def test_no_change_and_immediate_policy(self):
        c = configuration()
        c["transition_control"] = True
        self.assertTrue(all(event["settings"] == manifest(c)["initial"] for event in manifest(c)["events"]))
        immediate = dict(c, backend="pffft-scheduled-batch-float")
        self.assertTrue(all(p["endpoint"] == p["publication_sample"] for p in expected_publications(immediate)))
        self.assertTrue(all(p["publication_sample"]-p["endpoint"] == 15 for p in expected_publications(c)))

    def test_odd_hop_and_rounded_horizon(self):
        c = dict(configuration(), hop=37, block=64, callbacks=16)
        self.assertEqual(manifest(c)["horizon"], 1024)
        self.assertEqual(manifest(c)["events"][-1]["request_sample"], 1023)
        self.assertTrue(expected_publications(c))


if __name__ == "__main__":
    unittest.main()

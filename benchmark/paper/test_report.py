"""Numerical/provenance/uncertainty report fixtures; not measurement evidence."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import json
import importlib.util
from pathlib import Path
import tempfile
import unittest

from report import collect, report, comparison_key, variation
from run import save
import test_contracts


class ReportTests(unittest.TestCase):
    def fixture(self, directory, phase="smoke", session="one"):
        directory.mkdir()
        metadata = test_contracts.ContractTests().campaign(directory)
        metadata.update(phase=phase, host_id="fixture-host", session_id=session, revision="fixture-revision")
        save(directory/"metadata.json", metadata)
        return directory

    def test_variation_uses_sessions_not_pooled_callbacks(self):
        result = variation([dict(session="a", cost=1), dict(session="a", cost=3), dict(session="b", cost=10)])
        self.assertEqual(result["mean_of_session_means"], 6)
        self.assertEqual(result["observed_session_min"], 2)
        self.assertEqual(result["observed_session_max"], 10)
        self.assertIsNone(result["confidence_interval"])

    def test_phase_provenance_storage_and_determinism(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            campaign = self.fixture(root/"campaign")
            self.assertEqual(report([campaign], root/"out", "smoke", False), 1)
            report([campaign], root/"other", "smoke", False)
            for name in ("results.csv", "report.md", "evidence.json", "manifest.json"):
                self.assertEqual((root/"out"/name).read_bytes(), (root/"other"/name).read_bytes())
            result = json.loads((root/"out/evidence.json").read_text())
            row = result["records"][0]
            self.assertEqual(row["processes"][0]["cost"], 100/64)
            self.assertIsNone(row["resources"][0]["measurements"]["timing"]["native_allocation_bytes"])
            self.assertIsNone(row["processes"][0]["accuracy"])
            self.assertIn("preflight only", row["processes"][0]["numerical_status"])
            self.assertIn("source_sha256", result["sources"][0]["provenance"])
            self.assertIn("SMOKE", (root/"out/report.md").read_text())
            with self.assertRaises(ValueError):
                collect([campaign], "confirmation")
            with self.assertRaises(ValueError):
                collect([campaign, campaign], "smoke")
            with self.assertRaises(ValueError):
                report([campaign], campaign/"report", "smoke", False)
            changed = copy.deepcopy(row)
            changed["contract"]["boundary"] = "inverse-job"
            self.assertNotEqual(comparison_key(row), comparison_key(changed))
            changed = copy.deepcopy(row)
            changed["contract"]["precision"] = "double"
            self.assertNotEqual(comparison_key(row), comparison_key(changed))

    @unittest.skipUnless(importlib.util.find_spec("matplotlib"), "Optional plotting environment required")
    def test_figure_determinism(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            campaign = self.fixture(root/"campaign")
            report([campaign], root/"first", "smoke")
            report([campaign], root/"second", "smoke")
            files = json.loads((root/"first/manifest.json").read_text())
            self.assertTrue(any(name.endswith(".svg") for name in files))
            self.assertTrue(any(name.endswith(".png") for name in files))
            for name in files:
                self.assertEqual((root/"first"/name).read_bytes(), (root/"second"/name).read_bytes(), name)

    def test_confirmation_session_floor_and_host_strata(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            a = self.fixture(root/"a", "confirmation", "one")
            b = self.fixture(root/"b", "confirmation", "two")
            c = self.fixture(root/"c", "confirmation", "three")
            with self.assertRaisesRegex(ValueError, "three"):
                collect([a,b], "confirmation")
            data = collect([a,b,c], "confirmation")
            self.assertEqual(data["records"][0]["variation"]["sessions"], 3)
            metadata = json.loads((c/"metadata.json").read_text())
            metadata["host_id"] = "different-host"
            save(c/"metadata.json", metadata)
            with self.assertRaisesRegex(ValueError, "three"):
                collect([a,b,c], "confirmation")


if __name__ == "__main__":
    unittest.main()

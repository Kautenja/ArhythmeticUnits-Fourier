"""Numerical/provenance/uncertainty report fixtures; not measurement evidence."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"lib"))

import copy
import csv
import json
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from check import check
from report import ages, callback_tails, collect, report, comparison_key, tables, variation
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

    def test_checked_reporting_parses_and_hashes_raw_once(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            campaign = self.fixture(root/"campaign")
            raw = campaign/"raw.csv"
            counts = dict(text=0, binary=0)
            original = Path.open
            def counted(path, *args, **kwargs):
                if path == raw:
                    mode = args[0] if args else kwargs.get("mode", "r")
                    counts["binary" if "b" in mode else "text"] += 1
                return original(path, *args, **kwargs)
            with patch.object(Path, "open", counted):
                data = collect([campaign], "smoke")
            self.assertEqual(counts, dict(text=1, binary=1))
            process = data["records"][0]["processes"][0]
            self.assertEqual(process["ecdf"], [[100., .5], [100., 1.]])
            self.assertEqual(process["observation_count"], 2)

    def test_invalid_run_does_not_export_completed_observations_or_report(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            campaign = self.fixture(root/"campaign")
            metadata = json.loads((campaign/"metadata.json").read_text())
            metadata["runs"][0]["summary"]["groups"]["callback"]["mean_ns"] += 1
            save(campaign/"metadata.json", metadata)
            exported = {}
            with self.assertRaises(ValueError):
                check(campaign, report_data=exported)
            self.assertEqual(exported, {})
            with self.assertRaises(ValueError):
                report([campaign], root/"out", "smoke", False)
            self.assertFalse((root/"out").exists())

    def test_tail_summary_weights_sessions_and_retains_rare_maximum(self):
        processes = [dict(session=session, timing=dict(p99_ns=p99, observed_max_ns=maximum))
                     for session, p99, maximum in (("a", 1, 4), ("a", 3, 200), ("b", 10, 12), ("c", 20, 21))]
        result = callback_tails(processes)
        self.assertEqual(result["session_median_p99_ns"], dict(a=2, b=10, c=20))
        self.assertEqual(result["median_session_p99_ns"], 10)
        self.assertEqual(result["observed_session_p99_min_ns"], 2)
        self.assertEqual(result["observed_session_p99_max_ns"], 20)
        self.assertEqual(result["observed_max_ns"], 200)

    def test_human_units_coverage_and_unavailable_tails(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            campaign = self.fixture(root/"campaign")
            report([campaign], root/"out", "smoke", False)
            with (root/"out/results.csv").open() as stream:
                result = next(csv.DictReader(stream))
            with (root/"out/process-timings.csv").open() as stream:
                process = next(csv.DictReader(stream))
            self.assertEqual(float(result["median_session_p99_us"]), .1)
            self.assertEqual(float(result["observed_callback_max_us"]), .1)
            self.assertAlmostEqual(float(result["mean_serial_audio_time_percent"]), .0075)
            self.assertAlmostEqual(float(result["p99_budget_percent"]), .0075)
            self.assertAlmostEqual(float(process["callback_budget_us"]), 1e6*64/48000)
            self.assertAlmostEqual(float(process["observation_window_seconds"]), 128/48000)
            self.assertEqual(int(process["observations"]), 2)
            self.assertEqual(float(process["timer_p99_us"]), .02)
            self.assertEqual(float(process["compute_budget_exceedance_fraction"]), 0)
            self.assertEqual(process["config_sha256"], result["config_sha256"])
            for key in ("rate", "count", "alignment", "load", "smooth", "voices", "cache_mib", "warm_hops", "callbacks"):
                self.assertEqual(result[key], process[key])
            data = collect([campaign], "smoke")
            row = data["records"][0]
            row["config"]["pass_name"] = "throughput"
            row["tails"] = None
            tables(data, root/"out")
            with (root/"out/results.csv").open() as stream:
                result = next(csv.DictReader(stream))
            for field in ("median_session_p99_us", "observed_callback_max_us", "callback_budget_us", "p99_budget_percent"):
                self.assertEqual(result[field], "")
            with (root/"out/process-timings.csv").open() as stream:
                process = next(csv.DictReader(stream))
            self.assertEqual(process["callback_budget_us"], "")
            self.assertEqual(process["compute_budget_exceedance_fraction"], "")
        contract = dict(boundary="analysis", publication_delay_samples=479, center_offset_samples=1023.5,
                        playback_delay_samples=None)
        self.assertAlmostEqual(ages(contract, 48000)["spectrum_center_age_ms"], 1502.5/48)
        self.assertIsNone(ages(contract, 48000)["playback_delay_ms"])
        contract.update(boundary="chain", playback_delay_samples=958)
        self.assertIsNone(ages(contract, 48000)["spectrum_center_age_ms"])
        self.assertAlmostEqual(ages(contract, 48000)["playback_delay_ms"], 958/48)
        contract.update(boundary="transform", publication_delay_samples=-1, center_offset_samples=-1,
                        playback_delay_samples=-1)
        self.assertTrue(all(value is None for value in ages(contract, 48000).values()))

    def test_phase_provenance_storage_and_determinism(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            campaign = self.fixture(root/"campaign")
            self.assertEqual(report([campaign], root/"out", "smoke", False), 1)
            report([campaign], root/"other", "smoke", False)
            for name in ("results.csv", "process-timings.csv", "report.md", "evidence.json", "manifest.json"):
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
            self.assertTrue(any(name.startswith("cost-tail-") for name in files))
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

"""Regression checks for the measurement protocol and artifact integrity."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
import tarfile
import tempfile
import unittest

from check import check, validate_rows, validate_synthesis_accuracy
from contracts import SYNTHESIS_BACKENDS, synthesis_contract
from run import BASE, digest, matrix, summarize


class ProtocolTests(unittest.TestCase):
    def test_synthesis_matrix_contracts(self):
        rows = matrix("synthesis")
        self.assertEqual({r["backend"] for r in rows}, SYNTHESIS_BACKENDS)
        for row in rows:
            contract = synthesis_contract(row)
            if "-incremental-" in row["backend"]:
                self.assertIn(dict(row, backend=row["backend"].replace("-incremental-", "-batch-")), rows)
            if row["backend"].startswith("ols-"):
                self.assertEqual(contract["playback_delay_samples"], row["hop"]-1+contract["publication_delay_samples"])
        for backend in SYNTHESIS_BACKENDS:
            self.assertIn(dict(BASE, backend=backend), rows)
        smoke_path = Path(__file__).with_name("configs")/"synthesis-smoke.json"
        smoke = [dict(BASE, **row) for row in json.loads(smoke_path.read_text())]
        self.assertEqual(len(smoke), 56)
        self.assertEqual({r["backend"] for r in smoke if r["backend"] in SYNTHESIS_BACKENDS}, SYNTHESIS_BACKENDS)
        with self.assertRaisesRegex(ValueError, "Overlap-save"):
            synthesis_contract(dict(BASE, backend="ols-fir-batch-float", n=128, hop=128))
        with self.assertRaisesRegex(ValueError, "Unsupported"):
            synthesis_contract(dict(BASE, backend="inverse-stream-batch-float", smooth=1))
        with self.assertRaisesRegex(ValueError, "Unknown"):
            synthesis_contract(dict(BASE, backend="ols-unknown-batch-float"))

    def test_synthesis_full_output_audits(self):
        config = dict(BASE, backend="ols-fir-batch-float", callbacks=32, n=128, hop=32, block=2)
        accuracy = dict(max_abs_error=1e-7, max_reference=1., checked_samples=128,
                        playback_checked_samples=64, publications=2)
        validate_synthesis_accuracy(accuracy, config, 2)
        for changes in (dict(max_abs_error=0.01), dict(max_abs_error=float("nan")),
                        dict(checked_samples=127), dict(playback_checked_samples=0), dict(publications=1)):
            with self.assertRaisesRegex(ValueError, "numerical"):
                validate_synthesis_accuracy(dict(accuracy, **changes), config, 2)

    def test_synthesis_age_semantics(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"raw.csv"
            header = ("kind,ns,analyzer,sample,endpoint_age_samples,center_age_samples,"
                      "callback_visible_age_samples,playback_delay_samples\n")
            rows = "timer,40,,,,,,\n"*1024 + "callback,100,,,,,,\n"*2
            config = dict(BASE, backend="ols-fir-incremental-float", callbacks=2, n=128, hop=32, block=32)
            publications = "publication,0,0,31,31,46.5,31,62\npublication,0,0,63,31,46.5,31,62\n"
            path.write_text(header+rows+publications)
            validate_rows(path, config)
            path.write_text(header+rows+publications.replace(",62", ",31"))
            with self.assertRaisesRegex(ValueError, "Playback"):
                validate_rows(path, config)
            config["backend"] = "inverse-stream-incremental-float"
            path.write_text(header+rows+publications.replace("46.5", "31").replace(",62", ",-1"))
            validate_rows(path, config)

    def test_quantiles_and_budget_are_per_callback(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"raw.csv"
            path.write_text("kind,ns\ntimer,40\ncallback,100\ncallback,200\n"
                            "callback,300\ncallback,1500\npublication,0\n")
            config = dict(BASE, callbacks=4, block=1, rate=1000000)
            result = summarize(path, config)
            callback = result["groups"]["callback"]
            self.assertEqual(callback["p50_ns"], 200)
            self.assertEqual(callback["p99_ns"], 1500)
            self.assertEqual(callback["observed_compute_budget_exceedances"], 1)
            self.assertEqual(callback["ns_per_engine_sample"], 525)
            self.assertEqual(callback["simulated_compute_utilization"], .525)
            self.assertEqual(result["publication_audit_rows"], 1)

    def test_invalid_measurements_fail(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"raw.csv"
            for value in ("nan", "inf", "-1"):
                path.write_text(f"kind,ns\ntimer,40\ncallback,{value}\n")
                with self.assertRaises(ValueError):
                    summarize(path, dict(BASE, callbacks=1))

    def test_paper_has_matched_controls_and_operating_factors(self):
        rows = matrix("paper")
        for core in [row for row in rows if row["backend"] == "core-float"]:
            for backend in ("legacy-batch-float", "legacy-incremental-float"):
                self.assertIn(dict(core, backend=backend), rows)
        self.assertEqual({r["block"] for r in rows if r["backend"] == "core-float"}, {1, 16, 64, 256})
        self.assertEqual({r["count"] for r in rows}, {1, 4, 16})
        self.assertEqual({r["state"] for r in rows}, {"steady", "startup", "live"})
        self.assertTrue(any(r["cache_mib"] for r in rows))
        self.assertTrue(any(r["backend"] == "ifft-double" and r["pass_name"] == "steps" for r in rows))
        for row in rows:
            if row["backend"] == "spectre":
                self.assertEqual((row["n"], row["hop"]), (2048, 1024))

    def test_publication_age_and_missing_calls_fail(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"raw.csv"
            header = "kind,ns,analyzer,sample,endpoint_age_samples,center_age_samples,callback_visible_age_samples\n"
            rows = "timer,40,,,,,\n"*1024 + "callback,100,,,,,\n"*2
            publication = "publication,0,0,1023,1023,2046.5,1023\n"
            path.write_text(header+rows+publication)
            config = dict(BASE, callbacks=2, block=512)
            validate_rows(path, config)
            path.write_text(header+rows+publication.replace("2046.5", "2047"))
            with self.assertRaisesRegex(ValueError, "age"):
                validate_rows(path, config)
            path.write_text(header+rows)
            with self.assertRaisesRegex(ValueError, "Missing"):
                validate_rows(path, config)

    def test_missing_run_and_modified_artifact_fail(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            raw = directory/"raw.csv"
            raw.write_text("kind,ns\n" + "timer,40\n"*1024 + "callback,100\n")
            source = directory/"source.cpp"
            source.write_text("// test source\n")
            with tarfile.open(directory/"source.tar.gz", "w:gz") as archive:
                archive.add(source, arcname="source.cpp")
            config = dict(BASE, backend="driver", callbacks=1)
            metadata = dict(schema=1, status="complete", configs=[config], repeats=1,
                            source_sha256={"source.cpp": digest(source)},
                            artifact_sha256={"raw.csv": digest(raw)},
                            runs=[dict(workload=0, repeat=0, raw="raw.csv", summary=summarize(raw, config))])
            path = directory/"metadata.json"
            path.write_text(json.dumps(metadata))
            self.assertEqual(check(directory), 1)
            metadata["repeats"] = 2
            path.write_text(json.dumps(metadata))
            with self.assertRaisesRegex(ValueError, "Missing"):
                check(directory)
            metadata["repeats"] = 1
            path.write_text(json.dumps(metadata))
            raw.write_text(raw.read_text().replace("100", "101"))
            with self.assertRaisesRegex(ValueError, "checksum"):
                check(directory)


if __name__ == "__main__":
    unittest.main()

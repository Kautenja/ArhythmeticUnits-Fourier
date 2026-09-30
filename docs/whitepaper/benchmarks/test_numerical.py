"""Explicit numerical coverage rejects truncation and preserves old limitations."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import csv
from pathlib import Path
import tempfile
import unittest

from contracts import resolve_contract
from numerical import coverage_tables, validate_scalar_audit


def fixture():
    config = dict(backend="core-float", n=128, hop=37, block=16, callbacks=8,
                  count=2, alignment="staggered", callback_offset=5, state="live",
                  warm_hops=2, rate=48000, smooth=1, pass_name="callback",
                  load=0, voices=1, cache_mib=0)
    contract = resolve_contract(config)
    instances = [dict(instance=0, first_sample=227, samples=128, expected_spectra=3,
                      checked_spectra=3, checked_bins=195, first_endpoint=222, last_endpoint=296),
                 dict(instance=1, first_sample=245, samples=128, expected_spectra=4,
                      checked_spectra=4, checked_bins=260, first_endpoint=222, last_endpoint=333)]
    audit = dict(policy="all-publications-v1", status="full",
                 reference_precision="binary64 FFT / long-double direct DFT",
                 reference_mantissa_bits=53, direct_mantissa_bits=53,
                 interval_arithmetic="binary32", expected_spectra=7, checked_spectra=7,
                 expected_bins=455, checked_bins=455, instances=instances)
    accuracy = dict(audit=audit, publications=7, checked_samples=455, max_abs_error=1e-6,
                    reference="independent oracle", analysis=dict(policy="spectrum-norms-v1",
                    vectors=7, values=455, max_relative_l2=1e-7, max_relative_linf=2e-7,
                    zero_vectors=0, legacy_pointwise_failures=0))
    return config, contract, accuracy


class NumericalTests(unittest.TestCase):
    def test_valid_and_truncated_coverage(self):
        config, contract, accuracy = fixture()
        validate_scalar_audit(accuracy, config, contract, 7)
        for key in ("expected_spectra", "checked_spectra", "expected_bins", "checked_bins"):
            damaged = copy.deepcopy(accuracy)
            damaged["audit"][key] -= 1
            with self.assertRaises(ValueError):
                validate_scalar_audit(damaged, config, contract, 7)
        for field in ("instance", "first_sample", "samples", "expected_spectra", "checked_spectra",
                      "checked_bins", "first_endpoint", "last_endpoint"):
            damaged = copy.deepcopy(accuracy)
            damaged["audit"]["instances"][0][field] += 1
            with self.assertRaises(ValueError):
                validate_scalar_audit(damaged, config, contract, 7)
        for missing in ("audit", "analysis"):
            damaged = copy.deepcopy(accuracy)
            del damaged[missing]
            with self.assertRaises(ValueError):
                validate_scalar_audit(damaged, config, contract, 7)
        damaged = copy.deepcopy(accuracy)
        damaged["audit"]["instances"].pop()
        with self.assertRaises(ValueError):
            validate_scalar_audit(damaged, config, contract, 7)

    def test_coverage_table_does_not_upgrade_historical_records(self):
        config, contract, accuracy = fixture()
        records = []
        for backend, evidence in (("core-float", accuracy), ("pffft-analysis-float", dict(accuracy)),
                                  ("core-float", None)):
            if backend.startswith("pffft"):
                evidence.pop("audit")
            records.append(dict(config=dict(config, backend=backend), contract=contract,
                                processes=[dict(session="fixture", repeat=0, accuracy=evidence,
                                                publication_audit_rows=7)]))
        with tempfile.TemporaryDirectory() as temp:
            rows = coverage_tables(dict(records=records), Path(temp))
            self.assertEqual([r["coverage_policy"] for r in rows],
                             ["all-publications-v1", "archived-vector-counts", "preflight-only"])
            with (Path(temp)/"accuracy-coverage.csv").open() as stream:
                parsed = list(csv.DictReader(stream))
            self.assertEqual(parsed[2]["checked_bins"], "")
            self.assertEqual(parsed[0]["expected_bins"], "455")
            self.assertIn("unavailable", (Path(temp)/"accuracy-coverage.md").read_text())
            self.assertEqual({row["absolute_error_units"] for row in rows}, {"unnormalized FFT magnitude"})


if __name__ == "__main__":
    unittest.main()

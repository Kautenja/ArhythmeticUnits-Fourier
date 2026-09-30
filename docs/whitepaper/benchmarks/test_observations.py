"""Shared CSV summaries preserve historical arithmetic and checked evidence."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import statistics
import tempfile
import unittest

from observations import read_observations, summarize, validate_rows
from run import BASE


class ObservationTests(unittest.TestCase):
    def write_timings(self, path, values):
        path.write_text("kind,ns\n"+"timer,20\n"*1024
                        +"".join(f"callback,{value}\n" for value in values))

    def test_complete_historical_summary_and_report_details(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"raw.csv"
            self.write_timings(path, [100000, 200000, 300000, 1500000])
            config = dict(BASE, backend="driver", callbacks=4, block=64, rate=48000)
            expected = dict(publication_audit_rows=0, groups=dict(
                timer=dict(observations=1024, total_ns=20480., mean_ns=20.,
                           p50_ns=20., p95_ns=20., p99_ns=20., observed_max_ns=20.),
                callback=dict(observations=4, total_ns=2100000., mean_ns=525000.,
                              p50_ns=200000., p95_ns=1500000., p99_ns=1500000., observed_max_ns=1500000.,
                              ns_per_engine_sample=2100000./256,
                              simulated_compute_utilization=2100000./(1e9*256/48000),
                              budget_ns=1e9*64/48000, observed_compute_budget_exceedances=1)))
            self.assertEqual(summarize(path, config), expected)
            summary, details = read_observations(path, config, report=True)
            self.assertEqual(summary, expected)
            self.assertEqual(details, dict(observation_count=4,
                ecdf=[[100000., .25], [200000., .5], [300000., .75], [1500000., 1.]],
                callback_visible_age_range=None, playback_delay_range=None))
            self.assertIsNone(read_observations(path, config)[1])

    def test_original_order_floating_totals_and_nearest_rank_ties(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"raw.csv"
            # Sorting before summing changes the historical result on Python
            # versions with ordinary sequential float sum. Preserve the active
            # interpreter's sum/mean semantics rather than imposing math.fsum.
            values = [1e16, 1., 1., 1., 1., 3., 1e-16]
            self.write_timings(path, values)
            config = dict(BASE, backend="driver", callbacks=len(values))
            result = read_observations(path, config)[0]["groups"]["callback"]
            self.assertEqual(result["total_ns"], sum(values))
            self.assertEqual(result["mean_ns"], statistics.mean(values))
            self.assertEqual(result["ns_per_engine_sample"], sum(values)/(len(values)*config["block"]))
            for values, quantiles in (([4]+[3]*4+[2]*45+[1]*50, (1., 2., 3.)),
                                     (list(range(101, 0, -1)), (51., 96., 100.))):
                self.write_timings(path, values)
                config["callbacks"] = len(values)
                result = read_observations(path, config)[0]["groups"]["callback"]
                self.assertEqual(tuple(result[key] for key in ("p50_ns", "p95_ns", "p99_ns")), quantiles)

    def test_checked_reads_reject_malformed_timing_counts_and_publications(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"raw.csv"
            config = dict(BASE, backend="core-float", n=128, hop=32, block=32, callbacks=2)
            header = ("kind,ns,analyzer,sample,endpoint_age_samples,center_age_samples,"
                      "callback_visible_age_samples,playback_delay_samples\n")
            timings = "timer,20,,,,,,\n"*1024+"callback,100,,,,,,\n"*2
            publications = "publication,0,0,31,31,94.5,31,-1\npublication,0,0,63,31,94.5,31,-1\n"
            valid = header+timings+publications
            path.write_text(valid)
            validate_rows(path, config)
            summary, details = read_observations(path, config, report=True)
            self.assertEqual(summary["publication_audit_rows"], 2)
            self.assertEqual(details["callback_visible_age_range"], [31., 31.])
            self.assertEqual(details["playback_delay_range"], [-1., -1.])
            malformed = [valid.replace("callback,100", "callback,"+value, 1)
                         for value in ("nan", "inf", "-1", "invalid")]
            malformed += [valid.replace("timer,20,,,,,,\n", "", 1),
                          valid.replace("callback,100,,,,,,\n", "", 1),
                          header+timings+publications.splitlines(keepends=True)[0],
                          valid.replace("0,0,31,31,94.5", "0,0,30,31,94.5", 1),
                          valid.replace("31,94.5", "31,95.5", 1),
                          valid.replace("publication,0,0,31", "publication,0,1,31", 1),
                          valid.replace("publication,0,0,63", "publication,0,0,31", 1),
                          valid.replace("publication,0", "publication,nan", 1)]
            for text in malformed:
                with self.subTest(changed=text[-110:]):
                    path.write_text(text)
                    with self.assertRaises(ValueError):
                        validate_rows(path, config)
                    with self.assertRaises(ValueError):
                        read_observations(path, config, report=True)


if __name__ == "__main__":
    unittest.main()

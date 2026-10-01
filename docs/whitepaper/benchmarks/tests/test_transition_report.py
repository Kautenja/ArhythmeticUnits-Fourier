"""Fast report-boundary fixtures; lifecycle validation is tested separately."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"lib"))

import csv
import json
from pathlib import Path
import tempfile
import unittest

from report import stationary_records, transition_report_tables


def record(control):
    """Supply already checked process evidence without a compiler or campaign."""
    config = dict(backend="core-float", n=128, hop=16, block=16, rate=48000,
                  transition_suite="interactive-v1", transition_control=control)
    initial = dict(n=128, hop=16, window=8, rate=48000, octave=0., alpha=0.)
    events, responses, costs = [], [], []
    for generation, outcome in enumerate(("published", "replaced", "pending-no-response"), 1):
        request = generation*32
        application = request if outcome == "published" else -1
        publication = request+15 if outcome == "published" else -1
        replacement = 3 if outcome == "replaced" else -1
        events.append(dict(generation=generation, request_sample=request, reason="fixture-request",
            application_sample=application, first_publication_sample=publication,
            replaced_by=replacement, outcome=outcome,
            settings=dict(initial, window=8 if control else 11)))
        responses.append(dict(generation=generation, request_sample=request, reason="fixture-request",
            outcome=outcome, replaced_by=replacement,
            application_latency_samples=0 if application >= 0 else None,
            application_latency_ms=0. if application >= 0 else None,
            first_publication_latency_samples=15 if publication >= 0 else None,
            first_publication_latency_ms=15/48 if publication >= 0 else None))
        costs.append(dict(generation=generation, window_start=request-16, window_end=request+64,
            callback_indices=[generation, generation+1], observations=2,
            mean_ns=1500., p99_ns=2000., observed_max_ns=2000., timer_p99_ns=25.))
    publications = []
    for generation, endpoint in ((0, 0), (1, 32)):
        publications.append(dict(instance=0, channel=0, generation=generation, endpoint=endpoint,
            publication_sample=endpoint+15, history_start=0, bins=65,
            max_abs_error=1e-7, max_reference=4., accuracy=dict(policy="spectrum-norms-v1",
                tolerance=3e-4, vectors=1, values=65, zero_vectors=0,
                max_relative_l2=1e-8, max_relative_linf=2.5e-8, legacy_pointwise_failures=0,
                max_legacy_scaled_error=1e-7,
                worst_pointwise=dict(endpoint=endpoint, bin=3, channel=0, actual=.5, reference=.5000001))))
    trace = dict(suite="interactive-v1", control=control, horizon=160, initial=initial, events=events,
                 time_origin="input sample", latch="latest at frame boundary",
                 retention="length clears history", memory_policy="prepared plans before timing")
    process = dict(session="fixture-session", repeat=0, raw="fixture.csv", raw_sha256="raw-hash",
                   transition=dict(trace=trace, sha256="trace-hash",
                       tables=dict(responses=responses, costs=costs, publications=publications)))
    return dict(stratum="fixture-stratum", host="fixture-host", config=config,
                contract=dict(precision="float"), processes=[process])


class TransitionReportTests(unittest.TestCase):
    def test_mixed_records_preserve_controls_outcomes_costs_and_errors(self):
        ordinary = dict(config=dict(backend="core-double", transition_suite=""))
        data = dict(phase="smoke", records=[ordinary, record(False), record(True)])
        self.assertEqual(stationary_records(data), [ordinary])
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            transition_report_tables(data, output)
            def rows(name):
                with (output/("transitions-"+name+".csv")).open(newline="") as stream:
                    return list(csv.DictReader(stream))
            responses, costs, publications = rows("responses"), rows("callback-costs"), rows("publications")
            self.assertEqual((len(responses), len(costs), len(publications)), (6, 6, 4))
            self.assertEqual({r["control"] for r in responses}, {"True", "False"})
            self.assertEqual({r["outcome"] for r in responses},
                             {"published", "replaced", "pending-no-response"})
            for row in responses:
                self.assertEqual(row["transition_sha256"], "trace-hash")
                if row["outcome"] != "published":
                    self.assertEqual(row["first_publication_latency_ms"], "")
                self.assertEqual(json.loads(row["requested_settings"])["window"],
                                 8 if row["control"] == "True" else 11)
            for row in costs:
                self.assertEqual(float(row["mean_us"]), 1.5)
                self.assertEqual(float(row["p99_us"]), 2.)
                self.assertEqual(float(row["observed_max_us"]), 2.)
                self.assertEqual(float(row["timer_p99_us"]), .025)
                self.assertEqual(len(json.loads(row["callback_indices"])), 2)
                self.assertEqual(row["applied_generation_count"], "1")
            for row in publications:
                self.assertEqual(float(row["max_abs_error"]), 1e-7)
                self.assertEqual(float(row["max_reference"]), 4.)
                self.assertEqual(row["accuracy_policy"], "spectrum-norms-v1")
                self.assertEqual(row["checked_bins"], "65")
                self.assertEqual(row["absolute_error_units"], "unnormalized FFT magnitude")
                self.assertEqual(row["endpoint_age_samples"], "15")
                self.assertEqual(row["raw_sha256"], "raw-hash")
            markdown = (output/"transitions.md").read_text()
            for phrase in ("SMOKE", "not WCET", "prepared plans before timing", "pending-no-response"):
                self.assertIn(phrase, markdown)

    def test_missing_transition_trace_is_not_treated_as_stationary(self):
        broken = record(False)
        broken["processes"][0].pop("transition")
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(ValueError, "missing checked per-process trace"):
                transition_report_tables(dict(phase="smoke", records=[broken]), Path(temporary))


if __name__ == "__main__":
    unittest.main()

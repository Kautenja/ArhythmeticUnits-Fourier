"""Synthetic schedules, immutable retained observations, and checked v3 reports."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import csv
import gzip
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"lib"))
from contracts import resolve_contract
from metrics import derive, read
from run import BASE, digest, save
from workloads import expand
from check import check
from report import variation, callback_tails
from paths import ROOT
from reporting import derive as report
from publication import checked_report, selection_manifest, export, verify_export
from bundles import pack, unpack
from study import POLICIES, freeze, enforce
from observations import summarize
from execution import POLICY
import test_contracts
from test_execution import process_fixture
from test_evidence_integration import write_rows


def callback_rows(config, values):
    return [dict(kind="callback", index=i, analyzer=0, sample=i*config["block"], samples=config["block"], ns=value)
            for i, value in enumerate(values)]


def campaign(path):
    path.mkdir()
    m = test_contracts.ContractTests().campaign(path)
    config = expand(dict(m["configs"][0], workload_schema=3, backend="core-float", fixture="silence"))
    rows = [dict(kind="timer", ns=20) for _ in range(1024)]+callback_rows(config, [100, 2000000])
    for index, sample in enumerate((31, 63, 95, 127)):
        rows.append(dict(kind="publication", ns=0, analyzer=0, sample=sample, index=index, samples=0,
                         endpoint_age_samples=31, center_age_samples=94.5,
                         callback_visible_age_samples=31+63-sample%64, playback_delay_samples=-1))
    write_rows(path/"raw.csv", rows)
    _, _, execution, _ = process_fixture()
    execution["observations"] = [dict(sample=0, samples=64, release_ns=-1, deadline_ns=-1,
                                     wake_ns=0, start_ns=20, finish_ns=120),
                                 dict(sample=64, samples=64, release_ns=-1, deadline_ns=-1,
                                     wake_ns=1000, start_ns=1020, finish_ns=2001020)]
    save(path/"execution.json", execution)
    instance = dict(instance=0, first_sample=128, samples=128, expected_spectra=4, checked_spectra=4,
                    checked_bins=260, first_endpoint=128, last_endpoint=224)
    accuracy = dict(reference="synthetic zero-spectrum fixture; never measured", max_abs_error=0,
        max_reference=0, checked_samples=260, publications=4, playback_checked_samples=0, provider_instances=[],
        analysis=dict(policy="spectrum-norms-v1", tolerance=3e-4, vectors=4, values=260, zero_vectors=4,
                      legacy_pointwise_failures=0, max_relative_l2=0, max_relative_linf=0, max_legacy_scaled_error=0,
                      worst_pointwise=dict(endpoint=0, bin=0, channel=0, actual=0, reference=0)),
        audit=dict(policy="all-publications-v1", status="full", reference_precision="binary64 FFT / long-double direct DFT",
                   reference_mantissa_bits=53, direct_mantissa_bits=53, interval_arithmetic="binary32",
                   expected_spectra=4, checked_spectra=4, expected_bins=260, checked_bins=260, instances=[instance]))
    save(path/"stderr.txt", accuracy)
    m.update(configs=[config], contracts={"0": resolve_contract(config)}, phase="smoke", host_id="fixture-host",
             session_id="fixture-only", revision="fixture", notes="Synthetic fixture, no performance data", seed=7,
             execution_policy=dict(POLICY), sleep_protection=dict(status="not-applicable"),
             host_before=dict(platform="fixture", power="unavailable"), host_after=dict(platform="fixture", power="unavailable"),
             execution_events=[dict(event="prepared", ns=0, binary_sha256=m["binary_sha256"]),
                               dict(event="settle-start", ns=1), dict(event="settle-finish", ns=180000000001),
                               dict(event="launch", ns=180000000002)], **POLICIES)
    m["runs"][0].update(execution="execution.json", summary=summarize(path/"raw.csv", config))
    m["artifact_sha256"] = {p.name: digest(p) for p in path.iterdir() if p.is_file() and p.name != "metadata.json"}
    save(path/"metadata.json", m)
    return m


class SchedulingMetricsTests(unittest.TestCase):
    def test_offsets_staggering_partial_edges_and_nondivisible_callbacks(self):
        c = dict(BASE, n=128, hop=37, block=16, count=2, alignment="staggered", callback_offset=5, callbacks=8)
        rows = callback_rows(c, [10, 20, 30, 40, 50, 60, 70, 80])
        result = derive(rows, c, resolve_contract(c))
        self.assertEqual(result["compute"]["median_ns"], 45)
        self.assertIsNone(result["device"])
        a, b = result["analyzers"]
        self.assertEqual(a["offset_samples"], 5)
        self.assertEqual(b["offset_samples"], 23)
        self.assertEqual([r["endpoint_sample"] for r in a["hops"]], [-5, 32, 69, 106])
        self.assertEqual(a["hops"][1]["callbacks"], [2, 3, 4])
        self.assertEqual(a["hops"][1]["maximum_ns"], 50)
        self.assertEqual(a["hops"][2]["maximum_ns"], 70)
        self.assertEqual(a["complete_hop_peaks"]["mean_ns"], 60)
        self.assertEqual(a["hops"][1]["shared_callbacks"], [4])
        self.assertTrue(a["hops"][0]["partial"] and a["hops"][-1]["partial"])
        self.assertEqual(len(a["phase_profile"]), 8)
        bad = copy.deepcopy(rows); bad[1]["sample"] = 0
        with self.assertRaises(ValueError): derive(bad, c, resolve_contract(c))

    def test_callbacks_larger_than_hops_never_fabricate_independent_costs(self):
        c = dict(BASE, n=128, hop=16, block=64, callbacks=2)
        result = derive(callback_rows(c, [100, 200]), c, resolve_contract(c))
        hops = result["analyzers"][0]["hops"]
        self.assertEqual([r["maximum_ns"] for r in hops], [100]*4+[200]*4)
        self.assertTrue(all(r["shared_callbacks"] == r["callbacks"] for r in hops))
        self.assertEqual(result["compute"]["count"], 2)
        self.assertEqual(result["budgets"][-1]["observations"], 2)
        self.assertEqual(result["analyzers"][0]["shared_callback_count"], 2)

    def test_paced_lateness_and_compute_budgets_are_separate(self):
        policy, c, data, _ = process_fixture("paced")
        c = dict(BASE, **c, n=128, hop=32)
        data["observations"][2].update(finish_ns=3100000)
        durations = [r["finish_ns"]-r["start_ns"] for r in data["observations"]]
        result = derive(callback_rows(c, durations), c, resolve_contract(c), data)
        self.assertEqual(result["paced"]["release_deadline_exceedances"], 1)
        self.assertEqual(result["budgets"][-1]["exceedances"], 1)
        self.assertEqual(result["elapsed_measurement_ns"], 3100000)
        data["observations"][1]["deadline_ns"] = 0
        with self.assertRaises(ValueError): derive(callback_rows(c, durations), c, resolve_contract(c), data)

    def test_v3_report_export_bundle_and_corruption_rejection(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path = root/"campaign"
            metadata = campaign(path)
            self.assertEqual(check(path), 1)
            report([path], root/"report", "smoke", False)
            data, _ = checked_report(root/"report")
            self.assertEqual(data["schema"], 2)
            metric = data["records"][0]["processes"][0]["scheduling"]
            self.assertEqual(metric["analyzers"][0]["complete_hop_peaks"]["count"], 4)
            self.assertEqual(metric["budgets"][-1]["exceedances"], 1)
            selection_manifest(root/"report", root/"selection.json", True)
            export(root/"selection.json", root/"export", True)
            verify_export(root/"export")
            pack([path], root/"bundle.tar.gz", root/"selection.json")
            unpack(root/"bundle.tar.gz", root/"unpacked")
            checked_report(root/"unpacked/report")
            evidence_path = root/"report/evidence.json"
            original_evidence = json.loads(evidence_path.read_text())
            manifest = json.loads((root/"report/manifest.json").read_text())
            for mutation in (lambda d: d.update(schema=99),
                             lambda d: d["records"][0]["processes"][0]["scheduling"]["compute"].update(median_ns=0)):
                damaged = copy.deepcopy(original_evidence); mutation(damaged); save(evidence_path, damaged)
                manifest[evidence_path.name] = digest(evidence_path); save(root/"report/manifest.json", manifest)
                with self.assertRaises(ValueError): checked_report(root/"report")
            save(evidence_path, original_evidence)
            manifest[evidence_path.name] = digest(evidence_path); save(root/"report/manifest.json", manifest)
            original = json.loads((path/"stderr.txt").read_text())
            for mutation in (lambda a: a["analysis"].update(values=259),
                             lambda a: a["analysis"].update(max_relative_l2=.1),
                             lambda a: a["audit"]["instances"].clear()):
                damaged = copy.deepcopy(original); mutation(damaged)
                save(path/"stderr.txt", damaged)
                metadata["artifact_sha256"]["stderr.txt"] = digest(path/"stderr.txt")
                save(path/"metadata.json", metadata)
                with self.assertRaises(ValueError): check(path)
            save(path/"stderr.txt", original)
            metadata["artifact_sha256"]["stderr.txt"] = digest(path/"stderr.txt")
            save(path/"metadata.json", metadata)
            damaged = copy.deepcopy(metadata); damaged["runs"].clear(); save(path/"metadata.json", damaged)
            with self.assertRaises(ValueError): check(path)
            damaged = copy.deepcopy(metadata); damaged["configs"][0]["window"] = "boxcar"; save(path/"metadata.json", damaged)
            with self.assertRaises(ValueError): check(path)
            save(path/"metadata.json", metadata)
            table = root/"report/scheduling-hops.csv"
            table.write_text(table.read_text().replace("2000000.0", "3000000.0"))
            manifest = json.loads((root/"report/manifest.json").read_text())
            manifest[table.name] = digest(table); save(root/"report/manifest.json", manifest)
            with self.assertRaisesRegex(ValueError, "derived numeric table"): checked_report(root/"report")

    def test_every_explicit_control_survives_freeze_and_enforcement(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); path = root/"pilot"
            metadata = campaign(path); metadata["phase"] = "pilot"
            save(path/"metadata.json", metadata)
            frozen = freeze([path], metadata["configs"], dict(repeats=1, seed=7), "rack",
                            "Synthetic freeze integrity fixture only", root/"freeze.json", fixture=True)
            enforce(frozen, metadata)
            for field in ("window", "octave", "temporal_mode", "temporal_value", "fixture", "fixture_seed",
                          "decay_samples", "active_ports", "voices", "execution_regime", "experimental_policy"):
                changed = copy.deepcopy(metadata)
                changed["configs"][0][field] = "changed"
                with self.subTest(field=field), self.assertRaises(ValueError): enforce(frozen, changed)

    def test_retained_raw_peaks_phases_slow_process_and_inverse_exceedances(self):
        fixture = Path(__file__).parent/"fixtures/scheduling"
        records = json.loads((fixture/"provenance.json").read_text())["records"]
        expected = dict(core=(3418.45166015625, 3416, 2.245398693947507),
                        vdsp=(9414.70947265625, 9417, 27.380311671049867),
                        inverse=(10156.23388671875, 10041, 6.619268689745168))
        for record in records:
            with self.subTest(process=record["label"]):
                raw = gzip.decompress((fixture/(record["label"]+".csv.gz")).read_bytes())
                self.assertEqual(hashlib.sha256(raw).hexdigest(), record["raw_sha256"])
                original = gzip.decompress((ROOT/record["metadata"]).read_bytes())
                self.assertEqual(hashlib.sha256(original).hexdigest(), record["metadata_sha256"])
                metadata = json.loads(original)
                self.assertEqual(metadata["artifact_sha256"][record["original_raw"]], record["raw_sha256"])
                run = next(r for r in metadata["runs"] if r["raw"] == record["original_raw"])
                c = record["config"]; contract = metadata["contracts"][str(run["workload"])]
                self.assertEqual(c, metadata["configs"][run["workload"]])
                result = derive(list(csv.DictReader(io.StringIO(raw.decode()))), c, contract)
                old = record["summary"]["groups"][c["pass_name"]]
                self.assertEqual(result["compute"]["count"], old["observations"])
                self.assertEqual(result["compute"]["maximum_ns"], old["observed_max_ns"])
                self.assertAlmostEqual(result["timed_compute_ns"]/result["samples"], old["ns_per_engine_sample"])
                self.assertIsNone(result["elapsed_measurement_ns"])
                if record["label"] == "slow-hybrid":
                    self.assertAlmostEqual(old["ns_per_engine_sample"], 390.32320165634155)
                    self.assertEqual(result["analyzers"], [])
                    self.assertIsNone(result["throughput_chunks"])
                    continue
                self.assertEqual(result["budgets"][-1]["exceedances"], old["observed_compute_budget_exceedances"])
                analyzer = result["analyzers"][0]
                self.assertTrue(all(h["endpoint_source"] == "publication-replay" for h in analyzer["hops"]))
                if record["label"] == "inverse-misses":
                    self.assertEqual(result["boundary"], "inverse-job")
                    self.assertEqual(result["budgets"][-1]["exceedances"], 64)
                    self.assertIsNone(result["device"])
                else:
                    mean, median, imbalance = expected[record["label"]]
                    self.assertEqual(analyzer["complete_hop_peaks"]["count"], 2048)
                    self.assertAlmostEqual(analyzer["complete_hop_peaks"]["mean_ns"], mean)
                    self.assertEqual(analyzer["complete_hop_peaks"]["median_ns"], median)
                    self.assertAlmostEqual(analyzer["phase_mean_max_over_min"], imbalance)

    def test_historical_headline_hierarchical_summaries(self):
        processes = {}
        for path in sorted((ROOT/"docs/whitepaper/data/comparison-012/campaigns").glob("confirm-*-primary.metadata.json.gz")):
            metadata = json.loads(gzip.decompress(path.read_bytes()))
            for run in metadata["runs"]:
                c = metadata["configs"][run["workload"]]
                if c["backend"] not in ("core-float", "vdsp-analysis-float") or any(c[k] != v for k, v in
                        dict(n=4096, hop=1024, block=64, smooth=0).items()): continue
                timing = run["summary"]["groups"][c["pass_name"]]
                processes.setdefault((c["backend"], c["pass_name"]), []).append(dict(
                    session=metadata["session_id"], cost=timing["ns_per_engine_sample"], timing=timing))
        for rows in processes.values(): self.assertEqual((len(rows), len({r["session"] for r in rows})), (9, 3))
        for backend, p99 in (("core-float", 3.42), ("vdsp-analysis-float", 9.50)):
            self.assertEqual(round(callback_tails(processes[backend, "callback"])["median_session_p99_ns"]/1000, 2), p99)
        costs = [variation(processes[b, "throughput"])["mean_of_session_means"] for b in ("core-float", "vdsp-analysis-float")]
        self.assertEqual(round(costs[0]/costs[1], 2), 2.27)


if __name__ == "__main__":
    unittest.main()

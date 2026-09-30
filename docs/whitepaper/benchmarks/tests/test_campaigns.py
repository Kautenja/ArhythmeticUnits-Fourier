"""Host inventories, equal-channel workloads, and callback-origin contracts."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"lib"))
from paths import ROOT, BENCHMARKS, HISTORY


import copy
import json
from pathlib import Path
import tempfile
import unittest

from campaigns import document, resolve, inventory, CHANNELS
from contracts import load_registry, validate_config
from run import BASE
from check import validate_rows, validate_provider_info, validate_synthesis_accuracy


class CampaignTests(unittest.TestCase):
    def test_manifests_and_host_subsets(self):
        for phase in ("smoke", "pilot", "extensions"):
            manifest = document(phase)
            self.assertEqual(manifest, json.loads(((HISTORY/"configs")/("external-"+phase+".json")).read_text()))
            for variant, features, system in (("rack", (), "Linux"), ("portable", ("fftw",), "Linux"),
                                               ("macos", ("fftw", "vdsp"), "Darwin")):
                registry = load_registry(features=features)
                rows, evidence = resolve(manifest, variant, registry, system, BASE)
                self.assertEqual(len(rows), len({json.dumps(c, sort_keys=True) for c in rows}))
                self.assertEqual(inventory(rows, registry)["workloads"], len(rows))
                for row in rows:
                    validate_config(row, registry)
                if variant != "macos":
                    self.assertIn("vdsp", evidence["omitted_by_provider"])
                if phase == "extensions":
                    self.assertTrue(any(c["callback_offset"] for c in rows))
                    self.assertTrue(any(registry[c["backend"]]["channels"] == 4 for c in rows))
                    self.assertTrue(any(registry[c["backend"]]["precision"] == "double" for c in rows))
        with self.assertRaises(ValueError):
            resolve(document("pilot"), "portable", load_registry(), "Linux", BASE)
        with self.assertRaises(ValueError):
            resolve(document("pilot"), "macos", load_registry(features=("fftw", "vdsp")), "Linux", BASE)

    def test_equal_independent_channels_and_audits(self):
        registry = load_registry(features=("fftw", "vdsp"))
        rows, _ = resolve(document("extensions"), "macos", registry, "Darwin", BASE)
        simd_rows = [c for c in rows if c["backend"] == "core-independent4-simd"]
        for simd in simd_rows:
            for name in CHANNELS:
                self.assertIn(dict(simd, backend=name), rows)
                self.assertEqual(registry[name]["channels"], 4)
        config = dict(BASE, backend="pffft-analysis4-float", n=128, callbacks=2)
        accuracy = dict(max_abs_error=0, max_reference=1, publications=2, checked_samples=520, playback_checked_samples=0)
        validate_synthesis_accuracy(accuracy, config, 2, registry)
        with self.assertRaises(ValueError):
            validate_synthesis_accuracy(dict(accuracy, checked_samples=130), config, 2, registry)
        native = dict(provider="rack-pffft", precision="float", plan_policy="exact-size", native_plan_bytes=None)
        info = dict(input_contract="independent-four-v1", scalar_instances=4, native_instances=[native]*4)
        validate_provider_info(info, registry[config["backend"]])
        with self.assertRaises(ValueError):
            validate_provider_info(dict(info, native_instances=[native]), registry[config["backend"]])

    def test_callback_origin_is_separate_from_staggering(self):
        c = dict(BASE, backend="pffft-hybrid-float", n=128, hop=8, block=4, callbacks=4, callback_offset=3)
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"rows.csv"
            header = "kind,ns,analyzer,sample,endpoint_age_samples,center_age_samples,callback_visible_age_samples\n"
            rows = "timer,1,,,,,\n"*1024 + "callback,10,,,,,\n"*4
            pubs = "publication,0,0,4,7,70.5,10\npublication,0,0,12,7,70.5,10\n"
            path.write_text(header+rows+pubs)
            validate_rows(path,c)
            path.write_text(header+rows+pubs.replace(",4,7", ",3,7"))
            with self.assertRaisesRegex(ValueError, "cadence"):
                validate_rows(path,c)
        for change in (dict(callback_offset=8), dict(state="startup"), dict(callback_offset=-1)):
            with self.assertRaises(ValueError):
                validate_config(dict(c, **change))


if __name__ == "__main__":
    unittest.main()

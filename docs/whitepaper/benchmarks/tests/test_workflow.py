"""Bounded workflow, export, relocation and retirement gate fixtures."""

# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
import copy
import io
import json
import os
import subprocess
import tarfile
import tempfile
import unittest
from unittest.mock import patch
from contextlib import redirect_stdout
from paths import ROOT, BENCHMARKS
from profiles import load, measured
from contracts import load_registry
from study import freeze, read_freeze, enforce, confirmations, POLICIES
from run import save, digest
from report import identity
from reporting import derive
from publication import selection_manifest, export, verify_export
from bundles import pack, unpack
from retirement import inventory_plan, retire, report_references
from workflow import main
import test_contracts


def fixture(path, phase="smoke", session="one"):
    path.mkdir(parents=True)
    m = test_contracts.ContractTests().campaign(path)
    m.update(
        phase=phase,
        host_id="fixture-host",
        session_id=session,
        revision="fixture",
        seed=7,
        **POLICIES,
    )
    save(path / "metadata.json", m)
    return m


class WorkflowTests(unittest.TestCase):
    def test_report_dependency_scan_crosses_chunks_without_parsing_large_json(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            report = root / ".build/report/evidence.json"
            report.parent.mkdir(parents=True)
            fingerprint = "a" * 64
            # Place the source identity across the scanner's 1 MiB boundary.
            report.write_text(
                " " * (1024 * 1024 - 24) + json.dumps({"metadata_sha256": fingerprint})
            )
            with patch(
                "retirement.json.loads", side_effect=AssertionError("No full parse")
            ):
                self.assertEqual(report_references(root), {fingerprint: {report}})

    def test_profile_counts_and_confirmation_freeze_policy(self):
        for variant, count in [("rack", 52), ("portable", 66), ("macos", 80)]:
            _, configs, _, options = load("smoke", variant, "Darwin")
            self.assertEqual(
                len(
                    measured(
                        configs,
                        options,
                        load_registry(
                            features={
                                "rack": [],
                                "portable": ["fftw"],
                                "macos": ["fftw", "vdsp"],
                            }[variant]
                        ),
                    )
                ),
                count,
            )
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            p = root / "pilot"
            m = fixture(p, "pilot")
            options = dict(repeats=1, seed=7, hops=4, frames=2, warm_hops=0)
            frozen = freeze(
                [p],
                m["configs"],
                options,
                "rack",
                "Synthetic fixture rationale",
                root / "freeze.json", fixture=True,
            )
            self.assertEqual(read_freeze(root / "freeze.json"), frozen)
            enforce(frozen, m)
            for key, value in [
                ("source_sha256", {}),
                ("compiler", "changed"),
                ("compile_commands", ["-ffast-math"]),
                ("repeats", 2),
                ("configs", []),
            ]:
                with self.subTest(key=key), self.assertRaises(ValueError):
                    enforce(frozen, dict(m, **{key: value}))
            with self.assertRaises(ValueError):
                freeze(
                    [p],
                    [dict(m["configs"][0], n=256)],
                    options,
                    "rack",
                    "x",
                    root / "bad.json",
                )
            altered = copy.deepcopy(frozen)
            altered["options"]["seed"] = 8
            save(root / "freeze.json", altered)
            with self.assertRaises(ValueError):
                read_freeze(root / "freeze.json")
            paths = []
            for i in range(3):
                path = root / f"confirm-{i}"
                c = fixture(path, "confirmation", str(i))
                c["study_freeze"] = frozen
                save(path / "metadata.json", c)
                paths.append(path)
            self.assertEqual(len(confirmations(paths, allow_fixture=True)), 3)
            with self.assertRaises(ValueError):
                confirmations(paths[:2], allow_fixture=True)
            with self.assertRaises(ValueError):
                confirmations([paths[0], paths[0], paths[1]], allow_fixture=True)
            c = json.loads((paths[-1] / "metadata.json").read_text())
            c["phase"] = "smoke"
            save(paths[-1] / "metadata.json", c)
            with self.assertRaises(ValueError):
                confirmations(paths, allow_fixture=True)

    def test_fixture_export_freshness_and_production_rejection(self):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            campaign = root / "campaign"
            fixture(campaign)
            derive([campaign], root / "report", "smoke", False)
            selection_manifest(root / "report", root / "selection.json", True)
            export(root / "selection.json", root / "paper", True)
            receipt = verify_export(root / "paper")
            self.assertTrue(receipt["fixture"])
            self.assertEqual(len(receipt["claims"]), 1)
            self.assertIn("FIXTURE", (root / "paper/results.tex").read_text())
            with self.assertRaises(ValueError):
                export(root / "selection.json", root / "production", False)
            with self.assertRaises(ValueError):
                export(
                    root / "selection.json",
                    BENCHMARKS.parent / "generated/forbidden-fixture",
                    True,
                )
            (root / "paper/numbers.tex").write_text("stale")
            with self.assertRaises(ValueError):
                verify_export(root / "paper")
            selected = json.loads((root / "selection.json").read_text())
            selected["rows"][0]["units"] = "seconds"
            save(root / "selection.json", selected)
            with self.assertRaises(ValueError):
                export(root / "selection.json", root / "wrong-units", True)
            (root / "report/results.csv").write_text("corrupted")
            with self.assertRaises(ValueError):
                selection_manifest(root / "report", root / "stale.json", True)

    def test_bundle_relocation_and_independent_regeneration(self):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            p = root / "original"
            fixture(p)
            derive([p], root / "report", "smoke", False)
            selection_manifest(root / "report", root / "selection.json", True)
            pack([p], root / "evidence.tar.gz", root / "selection.json")
            manifest = unpack(root / "evidence.tar.gz", root / "elsewhere")
            for name in ("LICENSE", "LICENSING.md"):
                for directory in (root / "elsewhere", root / "elsewhere/tooling"):
                    self.assertEqual(
                        (directory / name).read_bytes(), (ROOT / name).read_bytes()
                    )
            relocated = root / "elsewhere/campaigns/000"
            self.assertFalse((relocated / "paper.bin").exists())
            self.assertTrue((relocated / "bundle-omissions.json").exists())
            derive([relocated], root / "new-report", "smoke", False)
            self.assertEqual(
                (root / "report/results.csv").read_bytes(),
                (root / "new-report/results.csv").read_bytes(),
            )
            command = [
                sys.executable,
                str(root / "elsewhere/tooling/docs/whitepaper/benchmarks/bench.py"),
                "check",
                str(relocated),
            ]
            self.assertEqual(
                subprocess.run(
                    command, capture_output=True, text=True, timeout=20
                ).returncode,
                0,
            )
            export(root / "elsewhere/selection.json", root / "relocated-paper", True)
            omissions = json.loads((relocated / "bundle-omissions.json").read_text())
            omissions["files"].append("raw.csv")
            save(relocated / "bundle-omissions.json", omissions)
            from check import check

            with self.assertRaises(ValueError):
                check(relocated)
            with self.assertRaises(ValueError):
                unpack(root / "evidence.tar.gz", root / "elsewhere")
            # Never extract paths or links supplied by a malformed bundle.
            with tarfile.open(root / "bad.tar.gz", "w:gz") as archive:
                member = tarfile.TarInfo("../escape")
                member.size = 1
                archive.addfile(member, io.BytesIO(b"x"))
            with self.assertRaises(ValueError):
                unpack(root / "bad.tar.gz", root / "unsafe")
            self.assertFalse((root / "escape").exists())

    def test_retirement_rejects_early_and_changed_candidates(self):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            candidate = root / ".build/old"
            fixture(candidate, "pilot")
            with patch("retirement.references", return_value=[]):
                inventory_plan([candidate], root / "plan.json", root)
            replacement = root / "replacement"
            fixture(replacement)
            pack([replacement], root / "replacement.tar.gz")
            with patch("retirement.references", return_value=[]), self.assertRaises(
                ValueError
            ):
                retire(
                    root / "plan.json",
                    root / "replacement.tar.gz",
                    root / "paper",
                    root / "receipt.json",
                    True,
                    root,
                )
            self.assertTrue(candidate.exists())
            with patch(
                "retirement.references", return_value=["retained.tex"]
            ), self.assertRaises(ValueError):
                retire(
                    root / "plan.json",
                    root / "replacement.tar.gz",
                    root / "paper",
                    root / "receipt.json",
                    True,
                    root,
                )
            (candidate / "extra").write_text("changed")
            with patch("retirement.references", return_value=[]), self.assertRaises(
                ValueError
            ):
                retire(
                    root / "plan.json",
                    root / "replacement.tar.gz",
                    root / "paper",
                    root / "receipt.json",
                    True,
                    root,
                )
            with self.assertRaises(ValueError):
                inventory_plan([root / "replacement"], root / "forbidden.json", root)

    # Isolate retirement mechanics; separate tests reject all synthetic evidence.
    @patch('study.require_real_evidence')
    def test_eligible_retirement_requires_independent_bundle_and_receipt(self, real_gate):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            pilot = root / "pilot"
            m = fixture(pilot, "pilot")
            options = dict(repeats=1, seed=7, hops=4, frames=2, warm_hops=0)
            f = freeze(
                [pilot],
                m["configs"],
                options,
                "rack",
                "Synthetic retirement fixture",
                root / "freeze.json",
            )
            campaigns = []
            for i in range(3):
                p = root / f"confirmation-{i}"
                m = fixture(p, "confirmation", str(i))
                m["study_freeze"] = f
                save(p / "metadata.json", m)
                campaigns.append(p)
            derive(campaigns, root / "report", "confirmation", False)
            selection_manifest(root / "report", root / "selection.json")
            publication = root / "docs/whitepaper/.build/exports/fixture-validation"
            with patch("publication.ROOT", root):
                export(root / "selection.json", publication)
            pack(campaigns, root / "replacement.tar.gz", root / "selection.json")
            old = root / ".build/old"
            fixture(old, "pilot")
            with patch("retirement.references", return_value=[]):
                inventory_plan([old], root / "plan.json", root)
                preview = retire(
                    root / "plan.json",
                    root / "replacement.tar.gz",
                    publication,
                    root / "receipt.json",
                    False,
                    root,
                )
                self.assertEqual(preview["status"], "eligible-preview")
                self.assertTrue(old.exists())
                result = retire(
                    root / "plan.json",
                    root / "replacement.tar.gz",
                    publication,
                    root / "receipt.json",
                    True,
                    root,
                )
            self.assertEqual(result["status"], "retired")
            self.assertFalse(old.exists())
            self.assertEqual(
                json.loads((root / "receipt.json").read_text())["candidates"][0][
                    "metadata_sha256"
                ],
                preview["candidates"][0]["metadata_sha256"],
            )

    def test_status_failure_restart_and_plan_are_bounded(self):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            fixture(root / "failed")
            m = json.loads((root / "failed/metadata.json").read_text())
            m.update(
                status="invalid",
                failure={"message": "fixture interruption"},
                active_job={"workload": 0, "repeat": 0, "stderr": "stderr.txt"},
            )
            save(root / "failed/metadata.json", m)
            output = io.StringIO()
            with redirect_stdout(output):
                self.assertEqual(main(["status", str(root / "failed"), "--logs"]), 0)
            self.assertIn("fixture interruption", output.getvalue())
            with redirect_stdout(io.StringIO()):
                self.assertEqual(
                    main(
                        [
                            "plan",
                            "--profile",
                            "smoke",
                            "--output",
                            str(root / "plan.json"),
                        ]
                    ),
                    0,
                )
            self.assertFalse((root / "plan.json").parent.joinpath("paper.bin").exists())
            self.assertEqual(main(["run", "--output", str(root / "failed")]), 1)
            with patch("workflow.preflight", side_effect=KeyboardInterrupt):
                self.assertEqual(main(["setup"]), 130)


if __name__ == "__main__":
    unittest.main()

# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
"""Optional serial FFTW adapter checks; build the local dependency first."""
import io
import json
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile
import unittest

import build_fftw

ROOT = Path(__file__).resolve().parents[3]
PREFIX = Path(os.environ.get("FFTW_PREFIX", ROOT / ".build/deps/fftw"))


class FftwBuildIsolationTest(unittest.TestCase):
    def test_each_extraction_ignores_modified_previous_source_and_objects(self):
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            archive = work / "source.tar.gz"
            with tarfile.open(archive, "w:gz") as bundle:
                member = tarfile.TarInfo(f"fftw-{build_fftw.VERSION}/configure")
                member.size = 8
                member.mode = 0o755
                bundle.addfile(member, io.BytesIO(b"original"))
            first, source = build_fftw.fresh_source(archive, work)
            expected = build_fftw.source_manifest(source)[1]
            (source / "configure").write_text("modified")
            (first / "build-double").mkdir()
            (first / "build-double/stale.o").write_bytes(b"stale")
            second, clean_source = build_fftw.fresh_source(archive, work)
            self.assertNotEqual(first, second)
            self.assertEqual(build_fftw.source_manifest(clean_source)[1], expected)
            self.assertFalse((second / "build-double").exists())
            self.assertEqual((source / "configure").read_text(), "modified")
            self.assertTrue((first / "build-double/stale.o").exists())


class FftwAdapterTest(unittest.TestCase):
    @unittest.skipUnless((PREFIX / "include/fftw3.h").exists(), "Optional FFTW dependency unavailable")
    def test_independent_canonical_references_and_plan_evidence(self):
        with tempfile.TemporaryDirectory() as temporary:
            executable = Path(temporary) / "verify-fftw"
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++11", "-O2",
                            "-Wall", "-Wextra", f"-I{PREFIX / 'include'}",
                            str(Path(__file__).with_name("verify_fftw.cpp")),
                            str(PREFIX / "lib/libfftw3f.a"),
                            str(PREFIX / "lib/libfftw3.a"), "-o", str(executable)], check=True)
            evidence = json.loads(subprocess.check_output([str(executable)], text=True))
        self.assertEqual(evidence["checked_bins"], 115200)
        self.assertLess(evidence["float_max_scaled_error"], 2e-5)
        self.assertLess(evidence["double_max_scaled_error"], 1e-10)
        self.assertEqual(evidence["measure"]["plan_policy"], "FFTW_MEASURE")
        self.assertEqual(evidence["estimate"]["plan_policy"], "FFTW_ESTIMATE")
        for policy in ("measure", "estimate"):
            info = evidence[policy]
            self.assertFalse(info["imported_wisdom"])
            self.assertTrue(info["forget_wisdom_before_planning"])
            self.assertEqual(info["threads"], 1)
            self.assertIn("fftw-", info["version"])
            self.assertIn("wisdom", info["exported_wisdom"])
            self.assertGreater(info["native_buffer_requested_bytes"], 0)
            self.assertIsNone(info["plan_storage_bytes"])
            self.assertIsNone(info["native_execution_allocations"])
        self.assertIsNone(evidence["measure"]["real_plan"])
        self.assertTrue(evidence["measure"]["forward_plan"])
        self.assertTrue(evidence["measure"]["inverse_plan"])
        self.assertTrue(evidence["estimate"]["real_plan"])
        self.assertIsNone(evidence["estimate"]["forward_plan"])


if __name__ == "__main__":
    unittest.main()

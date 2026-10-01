"""Regression coverage for the archived split-source exporter."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest


class ManuscriptExportTests(unittest.TestCase):
    def test_portable_manuscript_export_retains_explicit_images(self):
        import shutil

        with tempfile.TemporaryDirectory() as t:
            root = Path(t)
            paper = root / "paper"
            (paper / "tools").mkdir(parents=True)
            (paper / "generated").mkdir()
            shutil.copy2(
                Path(__file__).resolve().parents[1] / "manuscript.py", paper / "tools/manuscript.py"
            )
            (paper / "fourier.tex").write_text(
                r"\documentclass{article}\begin{document}\includegraphics{generated/figure.png}\end{document}"
            )
            (paper / "generated/figure.png").write_bytes(b"fixture-image")
            command = [
                sys.executable,
                str(paper / "tools/manuscript.py"),
                "--archive",
                str(root / "source.tar.gz"),
            ]
            result = subprocess.run(command, capture_output=True, text=True, timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr)
            with tarfile.open(root / "source.tar.gz") as archive:
                self.assertEqual(
                    set(archive.getnames()), {"fourier.tex", "generated/figure.png"}
                )
                self.assertEqual(
                    archive.extractfile("generated/figure.png").read(), b"fixture-image"
                )
            (paper / "fourier.tex").write_text(r"\includegraphics{../../private.png}")
            self.assertNotEqual(
                subprocess.run(
                    command, capture_output=True, text=True, timeout=20
                ).returncode,
                0,
            )


if __name__ == '__main__':
    unittest.main()

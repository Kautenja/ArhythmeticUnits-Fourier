"""Check real manuscript evidence and rejection of edited or missing results."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import study_paper
from check_paper import check_links, check_study_assets


class PaperEvidenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.assets = study_paper.generate()
        cls.tex = (study_paper.PAPER / 'fourier.tex').read_text()

    def test_paper_matches_freshly_derived_evidence(self):
        check_study_assets(self.tex, self.assets)

    def test_changed_result_is_rejected(self):
        original = self.assets['conference-host.tex']
        self.assertIn('423', original)
        changed = self.tex.replace(original, original.replace('423', '424', 1))
        with self.assertRaisesRegex(ValueError, 'conference-host.tex'):
            check_study_assets(changed, self.assets)

    def test_missing_or_duplicate_block_is_rejected(self):
        for name in ('macros.tex', 'conference-baselines.tex',
                     'conference-horizons.tex', 'conference-host.tex'):
            block = ('% BEGIN STUDY ASSET: ' + name + '\n' + self.assets[name] +
                     '% END STUDY ASSET: ' + name + '\n')
            for replacement in ('', block + block):
                with self.subTest(asset=name, replacement=bool(replacement)):
                    with self.assertRaisesRegex(ValueError, name):
                        check_study_assets(self.tex.replace(block, replacement), self.assets)

    def test_modified_measurements_are_rejected_before_aggregation(self):
        with tempfile.TemporaryDirectory() as temp:
            source = Path(temp) / 'study-014'
            shutil.copytree(study_paper.SOURCE, source)
            # Even valid JSON with an extra newline must fail the recorded hash.
            evidence = source / 'evidence.json'
            evidence.write_bytes(evidence.read_bytes() + b'\n')
            with patch.object(study_paper, 'SOURCE', source):
                with self.assertRaisesRegex(ValueError, 'Changed evidence: evidence.json'):
                    study_paper.generate()

    def test_local_links_are_checked_but_build_output_is_excluded(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            target = root / 'paper.tex'
            target.write_text('fixture')
            readme = root / 'README.md'
            readme.write_text('[paper](paper.tex) [anchor](#section) [web](https://example.com)')
            build = root / '.build'
            build.mkdir()
            (build / 'README.md').write_text('[ignored](missing.tex)')
            check_links(root.rglob('*.md'))
            target.unlink()
            with self.assertRaisesRegex(ValueError, 'Broken local link'):
                check_links([readme])


if __name__ == '__main__':
    unittest.main()

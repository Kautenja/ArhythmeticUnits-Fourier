#!/usr/bin/env python3
"""Check the standalone paper directly against retained evidence, without writing."""
import json
import math
from pathlib import Path
import re

from study_paper import generate

ROOT = Path(__file__).resolve().parents[3]
PAPER = ROOT / 'docs/whitepaper'


def check_links(paths):
    for path in paths:
        path = Path(path)
        if {'.build', '.build-legacy', 'build', '__pycache__'} & set(path.parts):
            continue
        source = path.read_text()
        targets = re.findall(r'\]\(([^)]+)\)', source)
        targets += re.findall(r'^\[[^]]+\]:\s+(\S+)', source, re.MULTILINE)
        for target in targets:
            if '://' in target or target.startswith('#'):
                continue
            target = target.split('#')[0]
            if not (path.parent / target).exists():
                raise ValueError(f'Broken local link in {path}: {target}')


def check_study_assets(tex, assets):
    """Compare every marked study block with freshly derived TeX."""
    names = ('macros.tex', 'conference-baselines.tex',
             'conference-horizons.tex', 'conference-host.tex')
    for name in names:
        pattern = (re.escape('% BEGIN STUDY ASSET: ' + name + '\n') + '(.*?)' +
                   re.escape('% END STUDY ASSET: ' + name + '\n'))
        matches = re.findall(pattern, tex, re.DOTALL)
        if matches != [assets[name]]:
            raise ValueError(f'Changed, missing, or duplicate study block: {name}')


def main():
    tex = (PAPER / 'fourier.tex').read_text()
    check_study_assets(tex, generate())

    keys = re.findall(r'\\bibitem\{([^}]+)\}', tex)
    assert len(keys) == len(set(keys))
    citations = {key for group in re.findall(r'\\cite\{([^}]+)\}', tex) for key in group.split(',')}
    assert citations == set(keys), (citations - set(keys), set(keys) - citations)
    labels = re.findall(r'\\label\{([^}]+)\}', tex)
    # Listings define their labels in the environment option list.
    labels += re.findall(r'\blabel=\{([^}]+)\}', tex)
    assert len(labels) == len(set(labels))
    for label in re.findall(r'\\(?:eqref|ref)\{([^}]+)\}', tex):
        assert label in labels, label

    check_links([ROOT / 'README.md', ROOT / 'CONTRIBUTING.md', *PAPER.rglob('*.md')])

    # Check the elementary balanced-quota identities independently of the FFT.
    for work in range(1,257):
        for horizon in range(1,129):
            quotas = [(s*work)//horizon - ((s-1)*work)//horizon for s in range(1,horizon+1)]
            assert sum(quotas) == work and quotas[-1] > 0
            assert max(quotas) == math.ceil(work/horizon)

    title = 'Scheduling FFT-Based Spectral Analysis in the Audio Processing Loop'
    assert f'pdftitle={{{title}}}' in tex
    printed_title = re.search(r'\\title\{\\textbf\{(.*?)\}\}', tex, re.DOTALL).group(1)
    assert ' '.join(printed_title.replace(r'\\', ' ').split()) == title
    assert f'  title: "{title}"' in (ROOT / 'CITATION.cff').read_text()
    assert title in (PAPER / 'CITATION.bib').read_text().replace('{', '').replace('}', '')
    for path in [ROOT / 'README.md', PAPER / 'README.md']:
        assert title in ' '.join(path.read_text().split()), path
    assert 'not yet deposited on arXiv' in (ROOT / 'CITATION.cff').read_text()
    assert 'kauten2026fourier' in (PAPER / 'CITATION.bib').read_text()
    assert 'manuscript version 5' in tex
    version = json.loads((ROOT / 'plugin.json').read_text())['version']
    assert f'Release v{version}, manuscript version 5' in tex
    assert f'pdfsubject={{Release v{version}; manuscript version 5.' in tex
    assert f'Preprint / v{version}' in tex
    for path in [ROOT / 'CITATION.cff', ROOT / 'README.md', PAPER / 'CITATION.bib']:
        assert 'Manuscript version 5' in path.read_text(), path
        assert f'Release v{version}; Manuscript version 5' in path.read_text(), path
    assert r'\documentclass[10pt,letterpaper,twocolumn]{article}' in tex
    assert r'\appendix' not in tex
    assert not re.search(r'\\(?:input|include|includegraphics)\s*(?:\[.*?\])?\{', tex)
    assert set(PAPER.glob('*.tex')) == {PAPER / 'fourier.tex'}

    # Check the conference paper's worked example and its block bound.
    work, horizon = 2048 + 1024 * 11 + 2 * 2049, 1024
    quotas = [(s * work) // horizon - ((s - 1) * work) // horizon
              for s in range(1, horizon + 1)]
    assert work == 17410 and quotas.count(17) == 1022 and quotas.count(18) == 2
    assert max(sum((quotas * 2)[s:s + 64]) for s in range(horizon)) == 1089
    for value in ('W=17410', '1022 calls', '1089'):
        assert value in tex, value
    print('Passed: two-column conference source, supplement exclusion, and worked scheduling example.')

    # The active manuscript has one canonical source, like the RackNES paper.
    for folder in ('sections', 'figures', 'tables'):
        assert not (PAPER / folder).exists(), f'Duplicate manuscript source folder: {folder}'
    print(f'Passed: {len(keys)} references, local links, current evidence, metadata, '
          '32768 balanced schedules, and standalone source integrity.')


if __name__ == '__main__':
    main()

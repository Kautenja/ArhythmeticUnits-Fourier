#!/usr/bin/env python3
"""Check manuscript links, citation keys, provenance, and numerical tables."""
import csv
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import subprocess
import sys
import tarfile

ROOT = Path(__file__).resolve().parents[3]
PAPER = ROOT / 'docs/whitepaper'
tex = (PAPER / 'fourier.tex').read_text()
meta = json.loads((PAPER / 'data/metadata.json').read_text())
with tarfile.open(PAPER / 'data/source.tar.gz') as archive:
    for path, expected in meta['source_sha256'].items():
        source = archive.extractfile(path)
        assert source is not None, path
        assert hashlib.sha256(source.read()).hexdigest() == expected, path
for path, expected in meta['data_sha256'].items():
    assert hashlib.sha256((PAPER / 'data' / path).read_bytes()).hexdigest() == expected, path

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

for path in [ROOT / 'README.md', *PAPER.rglob('*.md')]:
    if path.is_relative_to(PAPER) and 'build' in path.relative_to(PAPER).parts:
        continue
    for target in re.findall(r'\]\(([^)]+)\)', path.read_text()):
        if '://' not in target and not target.startswith('#'):
            assert (path.parent / target.split('#')[0]).exists(), (path, target)
    for target in re.findall(r'^\[[^]]+\]:\s+(\S+)', path.read_text(), re.MULTILINE):
        if '://' not in target:
            assert (path.parent / target.split('#')[0]).exists(), (path, target)

verification = list(csv.DictReader((PAPER / 'data/verification.csv').open()))
assert sum(int(r['schedules']) for r in verification) == 1320
assert sum(int(r['reference_bins']) for r in verification) == 6048
assert 'All 1320' in tex and '6048 complex bins' in tex
for precision, limit in [('float', 8.69e-8), ('double', 1.06e-14)]:
    actual = max(float(r['max_scaled_error']) for r in verification if r['precision'] == precision)
    assert actual <= limit and actual > limit * 0.98

rows = list(csv.DictReader((PAPER / 'data/timing.csv').open()))
summary = json.loads((PAPER / 'data/summary.json').read_text())
assert len(rows) == 144 and len(summary) == 16
for row in summary:
    selected = [r for r in rows if int(r['n']) == row['n'] and int(r['hop']) == row['hop'] and r['mode'] == row['mode']]
    assert len(selected) == 9
    for field in ['frame_ns', 'call_p99_ns', 'call_max_ns', 'start_median_ns', 'finish_median_ns']:
        values = [float(r[field]) for r in selected]
        assert row[field + '_median'] == statistics.median(values)
        assert row[field + '_min'] == min(values)
        assert row[field + '_max'] == max(values)

half = {(r['n'], r['mode']): r for r in summary if r['hop'] * 2 == r['n']}
for n in [1024, 2048, 4096, 16384]:
    complete, inc = half[n, 'complete'], half[n, 'incremental']
    lo, hi = inc['call_max_ns_paired_ratio_ci95']
    table_row = f"{n:,} & {complete['frame_ns_median']/1000:.3f} & {inc['frame_ns_median']/1000:.3f} & {complete['call_max_ns_median']/1000:.3f} & {inc['call_max_ns_median']/1000:.3f} & [{lo:.3f}, {hi:.3f}]"
    assert table_row in tex, table_row
    b = (n // 4) * (n.bit_length() - 2)
    h = n // 2
    q = math.ceil(b / h)
    length = math.ceil(b / q)
    cadence_row = f'{n:,} & {h:,} & {b:,} & {q} & {length:,} & {h/length:.3f}'
    assert cadence_row in tex, cadence_row

points = re.findall(r'\(([\d.]+),([\d.]+)\) \+= \(0,([\d.]+)\) -= \(0,([\d.]+)\)', tex)
assert len(points) == 8
for (mode, index), point in zip([(m,i) for m in ['complete','incremental'] for i in range(4)], points):
    n = [1024,2048,4096,16384][index]
    x, y, plus, minus = map(float, point)
    data = half[n,mode]
    assert x == n
    assert abs(y - data['call_max_ns_median']/1000) < 0.001
    assert abs(y + plus - data['call_max_ns_max']/1000) < 0.002
    assert abs(y - minus - data['call_max_ns_min']/1000) < 0.002

# Check the elementary balanced-quota identities independently of the FFT.
for work in range(1,257):
    for horizon in range(1,129):
        quotas = [(s*work)//horizon - ((s-1)*work)//horizon for s in range(1,horizon+1)]
        assert sum(quotas) == work and quotas[-1] > 0
        assert max(quotas) == math.ceil(work/horizon)

assert 'Fourier: Resumable FFT Scheduling for Real-Time Spectral Analysis' in (ROOT / 'CITATION.cff').read_text()
assert 'not yet deposited on arXiv' in (ROOT / 'CITATION.cff').read_text()
assert 'kauten2026fourier' in (PAPER / 'CITATION.bib').read_text()
print(f'Passed: {len(keys)} references, local links, source/data hashes, numerical tables, plot coordinates, and 32768 balanced schedules.')

# The prototype campaign is separate from the original FFT campaign and from
# production timings. Check its archive and independently derive the new table.
subprocess.run([sys.executable, str(PAPER / 'data/pipeline/check.py')], check=True)
pipeline = list(csv.DictReader((PAPER / 'data/pipeline/timing.csv').open()))
for mode, title in [('legacy', 'Legacy'), ('stage_burst', 'Stage bursts'),
                    ('serial', 'Serial one hop'), ('pipeline', 'Four-hop overlap')]:
    selected = [r for r in pipeline if int(r['n']) == 4096 and int(r['hop']) == 1024
                and math.isclose(float(r['octave']), 1/3, abs_tol=1e-6) and r['mode'] == mode]
    hops = [float(r['hop_ns']) / 1000 for r in selected if int(r['block']) == 0]
    blocks = [r for r in selected if int(r['block']) == 64]
    maxima = [float(r['max_ns']) / 1000 for r in blocks]
    age = 4095 if mode in ('stage_burst', 'pipeline') else 1023
    assert len(hops) == len(maxima) == 12
    assert all(int(r['age_samples']) == age for r in blocks)
    line = (f'{title} & {statistics.median(hops):.3f} & {statistics.median(maxima):.3f}'
            f' & {min(maxima):.3f}--{max(maxima):.3f} & {age}')
    assert line in tex, line

# Verify the worked whole-pipeline example and the quotient/remainder algorithm.
work, horizon = 8194, 1024
base, remainder = divmod(work, horizon)
error = done = 0
counts = []
for phase in range(horizon):
    quota = base
    error += remainder
    if error >= horizon:
        error -= horizon
        quota += 1
    counts.append(quota)
    done += quota
    assert done == (phase + 1) * work // horizon
assert counts.count(8) == 1022 and counts.count(9) == 2
assert 'W=8194' in tex
print('Passed: prototype table, frame ages, and whole-pipeline schedule example.')

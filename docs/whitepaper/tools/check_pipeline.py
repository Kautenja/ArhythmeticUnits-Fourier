#!/usr/bin/env python3
"""Verify the historical pipeline artifact without depending on current DSP."""
import csv
import hashlib
import json
from pathlib import Path
import tarfile

DIRECTORY = Path(__file__).resolve().parents[1]/"data/pipeline"
metadata = json.loads((DIRECTORY / 'metadata.json').read_text())
with tarfile.open(DIRECTORY / 'source.tar.gz') as archive:
    for name, expected in metadata['source_sha256'].items():
        source = archive.extractfile(name)
        assert source is not None, name
        assert hashlib.sha256(source.read()).hexdigest() == expected, name
for name, expected in metadata['data_sha256'].items():
    assert hashlib.sha256((DIRECTORY / name).read_bytes()).hexdigest() == expected, name
assert len(list(csv.DictReader((DIRECTORY / 'timing.csv').open()))) == 2880
assert len(list(csv.DictReader((DIRECTORY / 'baseline.csv').open()))) == 27
print('Passed: historical source archive and campaign hashes, 2880 timing rows, 27 phase rows.')

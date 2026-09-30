"""Immutable local preparation identities; these helpers never launch measurements."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
import platform
from run import digest, source_inputs, save
from study_plan import identity


def host_identity():
    return dict(platform=platform.platform(), machine=platform.machine(), host=platform.node(),
                python=platform.python_version())


def sdk_inputs(rack):
    return sorted({p for base in (rack/'include', rack/'dep/include', rack/'src/engine')
                   for p in base.rglob('*') if p.is_file() and p.name != '.DS_Store'} |
                  {p for p in rack.glob('*.mk')} | {p for p in rack.glob('libRack.*') if p.is_file()} |
                  {p for p in (rack/'dep/pffft').glob('pffft.[ch]') if p.is_file()} | {rack/'src/system.cpp'})


def snapshot(paths):
    return {str(p.resolve()): digest(p) for p in paths}


def assert_unchanged(inputs):
    for name, expected in inputs.items():
        p = Path(name)
        if not p.is_file() or digest(p) != expected:
            raise ValueError('Preparation is stale: '+name+'; prepare again into a fresh directory')


def seal(directory, plan, inputs, root, **extra):
    value = dict(schema=1, kind='fourier-offline-preparation-v1', status='prepared', plan=plan,
                 inputs=inputs, root=str(root.resolve()), host=host_identity(),
                 source_inventory=[str(p.resolve()) for p in source_inputs(root)], **extra)
    value['artifacts'] = {str(p.relative_to(directory)): digest(p) for p in sorted(directory.rglob('*')) if p.is_file()}
    value['manifest_id'] = identity(value)
    save(directory/'manifest.json', value)
    return value


def validate(directory, root=None):
    directory = Path(directory).resolve()
    file = directory/'manifest.json'
    if not file.is_file(): raise ValueError('No prepared manifest; run make benchmark-study-prepare while online')
    m = json.loads(file.read_text())
    if (m.get('schema') != 1 or m.get('kind') != 'fourier-offline-preparation-v1'
            or m.get('status') != 'prepared' or m.get('fixture')
            or m.get('manifest_id') != identity({k:v for k,v in m.items() if k != 'manifest_id'})):
        raise ValueError('Invalid, changed, or synthetic preparation manifest')
    root = Path(root or m['root']).resolve()
    if str(root) != m['root'] or m['host'] != host_identity():
        raise ValueError('Preparation host/workspace identity changed')
    if m['source_inventory'] != [str(p.resolve()) for p in source_inputs(root)]:
        raise ValueError('Source membership changed; prepare again')
    assert_unchanged(m['inputs'])
    files = {str(p.relative_to(directory)) for p in directory.rglob('*') if p.is_file()} - {'manifest.json'}
    if files != set(m['artifacts']): raise ValueError('Prepared artifact membership changed')
    for name, expected in m['artifacts'].items():
        p = directory/name
        if p.is_symlink() or not p.is_file() or digest(p) != expected:
            raise ValueError('Prepared artifact changed: '+name)
    if m['plan']['plan_id'] != identity({k:v for k,v in m['plan'].items() if k != 'plan_id'}):
        raise ValueError('Prepared plan changed')
    return m

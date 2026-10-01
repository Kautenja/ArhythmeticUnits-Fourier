#!/usr/bin/env python3
"""Validate original offline sessions and derive compact evidence; never execute DSP."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path
import shutil
import statistics
import sys

PAPER = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PAPER/'benchmarks/lib'))
from check import check
from contracts import load_registry
from offline import check_engine_group
from study_plan import identity, jobs


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def load(path):
    return json.loads(path.read_text())


def distribution(values):
    if not values:
        return None
    values = sorted(values)
    return dict(count=len(values), mean=statistics.mean(values), median=statistics.median(values),
                p95=values[math.ceil(.95*len(values))-1], p99=values[math.ceil(.99*len(values))-1], maximum=max(values))


def hop_peaks(durations, block, origin, endpoints, hop):
    """Max whole-block duration in each complete endpoint-to-next-endpoint hop.

    Includes the entire hop for immediate batch and delayed publication alike.
    A shared block is retained in both intersected hops, not apportioned.
    Analyzer-indexed peaks remain dependent aggregate-engine observations.
    """
    end = origin + len(durations)*block
    peaks = []
    for endpoint in sorted(set(endpoints)):
        if endpoint < origin or endpoint + hop > end:
            continue
        first = (endpoint-origin)//block
        last = (endpoint+hop-origin+block-1)//block
        peaks.append(max(durations[first:last]))
    return peaks


def timing(durations, block, rate, intervals, paced, rates=None):
    rates = rates or [rate]*len(durations)
    result = dict(blocks=distribution(durations), observations=len(durations),
                  compute_ns_per_sample=sum(durations)/(len(durations)*block),
                  compute_budget_misses=sum(v > block*1e9/fs for v,fs in zip(durations,rates)))
    if paced:
        result.update(release_misses=sum(r['finish_ns'] > r['deadline_ns'] for r in intervals),
                      wake_lateness=distribution([r['wake_ns']-r['release_ns'] for r in intervals]),
                      release_to_finish=distribution([r['finish_ns']-r['release_ns'] for r in intervals]))
    return result


def compact_process(directory, m, rec):
    config = m['configs'][rec['workload']]
    c = config if m['family'] == 'stream' else config['workload']
    execution = load(directory/rec['execution'])
    out = dict(session=m['session_id'], group=directory.name, family=m['family'], workload=rec['workload'],
               repeat=rec['repeat'], config_id=identity(config), raw=rec['raw'],
               raw_sha256=digest(directory/rec['raw']), started_utc=rec['started_utc'],
               observation_design=rec['observation_design'])
    if m['family'] == 'stream':
        rows = list(csv.DictReader((directory/rec['raw']).open()))
        values = [float(r['ns']) for r in rows if r['kind'] == 'callback']
        pubs = [r for r in rows if r['kind'] == 'publication']
        peaks = []
        for analyzer in range(c['count']):
            offset = c['callback_offset'] + (analyzer*c['hop']//c['count'] if c['alignment']=='staggered' else 0)
            endpoints = range(-offset, len(values)*c['block'], c['hop'])
            peaks.extend(hop_peaks(values,c['block'],0,endpoints,c['hop']))
        accuracy = load(directory/rec['stderr'])
        out.update(audits=[accuracy['analysis']] if 'analysis' in accuracy else [],
                   publication_age_samples=distribution([float(r['endpoint_age_samples']) for r in pubs]),
                   timer=distribution([float(r['ns']) for r in rows if r['kind']=='timer']))
        # Module output audits can nest per instance; preserve the full compact audit.
        out['accuracy'] = {k:v for k,v in accuracy.items() if k not in ('provider_instances','module_instances','controls')}
    else:
        document = load(directory/rec['raw']); result = document['result']
        values = [r['duration_ns'] for r in result['observations']]
        peaks = []
        if not config['events']:
            for node in result['replay']['nodes']:
                peaks.extend(hop_peaks(values,c['block'],result['origin'],
                    [p['endpoint'] for p in node['publications']],c['hop']))
        summary=rec['summary']
        out.update(audits=[n['accuracy']['analysis'] for n in result['replay']['nodes']],
                   aggregate_cpu_ns_per_sample=result['aggregate_cpu_ns']/(len(values)*c['block']),
                   publication_age_samples=distribution(summary['publication_age_samples']),
                   consumed_age_lower=distribution([a[0] for a in summary['consumed_age_bounds_samples']]),
                   consumed_age_upper=distribution([a[1] for a in summary['consumed_age_bounds_samples']]),
                   skipped_publications=summary['skipped_publications'],
                   consumer_polls=len(summary['consumed_age_bounds_samples']),
                   final_drain_snapshots=summary['final_drain_snapshots'], lifecycle=result['lifecycle'])
    rates = [r['rate'] for r in result['observations']] if m['family']=='engine' else None
    out.update(timing(values,c['block'],c['rate'],execution['observations'],c['execution_regime']=='paced',rates))
    out['hop_peaks']=distribution(peaks)
    return out


def import_sessions(root, destination):
    destination.mkdir(parents=True, exist_ok=True)
    evidence=dict(schema=1, policy='study-014-descriptive-v1', sessions=[], groups={}, processes=[])
    inputs={}; manifest_id=None; expected_source=None
    for label in ('pilot-01','pilot-02'):
        session=root/label; doc=load(session/'session.json'); manifest=load(session/'prepared-manifest.json')
        if doc['status']!='complete' or doc['fixture'] or not (session/'COMPLETE').is_file():
            raise ValueError('Incomplete or synthetic session')
        if identity({k:v for k,v in manifest.items() if k!='manifest_id'})!=manifest['manifest_id']:
            raise ValueError('Changed prepared identity')
        plan=manifest['plan']
        if identity({k:v for k,v in plan.items() if k!='plan_id'})!=plan['plan_id']:
            raise ValueError('Changed study plan')
        manifest_id=manifest_id or manifest['manifest_id']
        if manifest['manifest_id']!=manifest_id or doc['manifest_id']!=manifest_id:
            raise ValueError('Sessions do not use identical preparation')
        archive=root/(label+'.handback.tar.gz')
        sha=digest(archive)
        if sha != Path(str(archive)+'.sha256').read_text().split()[0]:
            raise ValueError('Handback checksum mismatch')
        checksums=load(session/'checksums.json')
        for name, expected in checksums.items():
            path=session/name
            if path.is_symlink() or not path.resolve().is_relative_to(session.resolve()) or digest(path)!=expected:
                raise ValueError('Session artifact mismatch: '+name)
        expected_groups=[g['name'] for g in plan['groups']]
        if doc['groups']!=expected_groups: raise ValueError('Study groups missing/reordered')
        actual_jobs=[]
        for group in plan['groups']:
            directory=session/group['name']; m=load(directory/'metadata.json')
            if (m['configs']!=group['configs'] or m['repeats']!=group['repeats'] or m['phase']!='pilot'
                    or m['seed']!=plan['design']['sessions'][label] or m['prepared_manifest_id']!=manifest_id):
                raise ValueError('Group differs from predeclared pilot')
            registry=load_registry(features=m['build_features'])
            if group['family']=='stream': check(directory)
            else: check_engine_group(directory,m,registry)
            actual_jobs += [dict(group=group['name'],family=group['family'],workload=r['workload'],repeat=r['repeat']) for r in m['runs']]
            evidence['groups'][group['name']]=dict(family=group['family'],configs=group['configs'],repeats=group['repeats'])
            for rec in sorted(m['runs'],key=lambda r:(r['workload'],r['repeat'])):
                evidence['processes'].append(compact_process(directory,m,rec))
            filename=label+'-'+group['name']+'-metadata.json.gz'
            (destination/filename).write_bytes(gzip.compress((directory/'metadata.json').read_bytes(),mtime=0))
            source=digest(directory/'source.tar.gz')
            expected_source=expected_source or source
            if source!=expected_source: raise ValueError('Measured sources differ')
            print('Validated',label,group['name'],len(m['runs']),'processes',flush=True)
        if actual_jobs!=jobs(plan,label): raise ValueError('Process order differs from design')
        filename=label+'-session.json.gz'
        (destination/filename).write_bytes(gzip.compress((session/'session.json').read_bytes(),mtime=0))
        evidence['sessions'].append(dict(label=label,raw_directory=str(session),archive=str(archive),
            archive_sha256=sha,manifest_id=manifest_id,session_sha256=digest(session/'session.json'),
            checksums_sha256=digest(session/'checksums.json'),host={k:m[k] for k in ('cpu_model','platform','compiler','revision','binary_sha256','rack_revision')},
            declaration=doc['declaration'],finished_utc=doc['finished_utc']))
    if len(evidence['processes'])!=776: raise ValueError('Expected 776 processes')
    shutil.copyfile(root/'pilot-01/Baselines/source.tar.gz',destination/'source.tar.gz')
    (destination/'prepared-manifest.json.gz').write_bytes(gzip.compress((root/'pilot-01/prepared-manifest.json').read_bytes(),mtime=0))
    content=json.dumps(evidence,sort_keys=True,indent=2,allow_nan=False)+'\n'
    (destination/'evidence.json').write_text(content)
    for p in sorted(destination.iterdir()):
        if p.is_file() and p.suffix not in ('.md',) and p.name!='receipt.json': inputs[p.name]=digest(p)
    (destination/'receipt.json').write_text(json.dumps(dict(schema=1,policy=evidence['policy'],files=inputs,
        importer_sha256=digest(Path(__file__)),source_sha256=expected_source,
        validation_sha256={name:digest(PAPER/'benchmarks/lib'/name) for name in
            ('check.py','engine_host.py','offline.py','execution.py','contracts.py','observations.py')}),sort_keys=True,indent=2)+'\n')
    print('Validated and derived all 776 processes; original evidence unchanged.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root',type=Path)
    parser.add_argument('--output',type=Path,default=PAPER/'data/study-014')
    args=parser.parse_args(); import_sessions(args.root,args.output)

"""Strict complete-module Rack profiles and retained block/consumer observations."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import math
from contracts import load_registry, validate_config
from run import BASE
from workloads import expand

POLICY = 'rack-engine-observations-v1'
RACK_REVISION = '8c33d966d329e4a6e354593b2b5f9ac2df5a03bd'
EVENTS = {'reset', 'freeze', 'resume', 'sample-rate', 'window', 'band', 'geometry'}
DEFAULTS = dict(schema=1, native='', threads=1, analyzers=1, background=0,
                consumer_hz=0, stall_every=0, stall_polls=2, blocks=128, warm_hops=2, events=[])


def resolve(value, registry=None):
    registry = registry or load_registry()
    if not isinstance(value, dict) or set(value)-set(DEFAULTS)-{'workload'}:
        raise ValueError('Unknown Rack engine profile fields')
    p = dict(DEFAULTS, **value)
    if p['schema'] != 1 or not isinstance(p.get('workload'), dict):
        raise ValueError('Expected engine profile schema 1 with a workload')
    c = expand(dict(BASE, **p['workload']))
    c.update(callbacks=p['blocks'], warm_hops=p['warm_hops'])
    validate_config(c, registry, measurement=True)
    if (c['workload_schema'] != 3 or c['count'] != 1 or c['load'] or c['cache_mib']
            or c['callback_offset'] or c['pass_name'] != 'callback' or c['rate'] != 48000
            or c['state'] not in ('startup', 'steady')):
        raise ValueError('Engine profiles require explicit isolated stationary 48 kHz module controls')
    d = registry[c['backend']]
    if d['kind'] not in ('fourier', 'spectre'):
        raise ValueError('Engine profile requires a complete module')
    if p['native']:
        n = registry[p['native']]
        if n['kind'] != 'native-analysis' or n['precision'] != 'float' or n['channels'] != d['channels']:
            raise ValueError('Native complete-module channel/precision mismatch')
    allowed = dict(threads=(1, 4), analyzers=(0, 1, 4, 16), background=(0, 16, 64), consumer_hz=(0, 30, 60))
    if any(type(p[k]) is not int or p[k] not in v for k, v in allowed.items()) or c['block'] not in (64, 256):
        raise ValueError('Unsupported engine graph/cadence')
    for k, low, high in (('blocks', 2, 100000), ('warm_hops', 0, 128), ('stall_polls', 0, 10), ('stall_every', 0, 1000000)):
        if type(p[k]) is not int or not low <= p[k] <= high:
            raise ValueError('Engine profile count outside bounds: '+k)
    if p['stall_every'] and (not p['consumer_hz'] or p['stall_every'] < 2):
        raise ValueError('Consumer stalls require a cadence')
    previous = -1
    if not isinstance(p['events'], list): raise ValueError('Events must be an array')
    for e in p['events']:
        if (not isinstance(e, dict) or set(e) != {'block', 'kind'} or e['kind'] not in EVENTS
                or type(e['block']) is not int or not previous < e['block'] < p['blocks']):
            raise ValueError('Invalid or unordered engine event')
        previous = e['block']
    if p['events'] and (c['execution_regime'] != 'continuous' or p['consumer_hz']):
        raise ValueError('Lifecycle diagnostics require a separate continuous pass')
    p['workload'] = {k: v for k, v in c.items() if k not in ('callbacks', 'warm_hops')}
    return p


def validate(document, registry=None, allow_fixture=False):
    """Validate authenticated payload content; caller separately checks file hashes."""
    p = resolve(document['profile'], registry)
    r, c = document['result'], p['workload']
    if (r.get('schema') != 1 or r.get('policy') != POLICY or type(r.get('fixture')) is not bool
            or (r['fixture'] and not allow_fixture)):
        raise ValueError('Unsupported or synthetic engine evidence')
    rows = r['observations']
    origin = 1 + math.ceil(p['warm_hops']*c['hop']/c['block'])*c['block']
    if r['origin'] != origin or len(rows) != p['blocks'] or type(r['aggregate_cpu_ns']) is not int or r['aggregate_cpu_ns'] < 0:
        raise ValueError('Missing engine intervals/CPU record')
    rate, previous_finish = c['rate'], -1
    changes = {e['block']: e['kind'] for e in p['events']}
    misses = []
    for i, row in enumerate(rows):
        if changes.get(i) == 'sample-rate': rate = 96000
        if (row['block'] != i or row['frame'] != origin+i*c['block'] or row['samples'] != c['block']
                or row['rate'] != rate or not previous_finish <= row['wake_ns'] <= row['start_ns'] <= row['finish_ns']
                or row['duration_ns'] != row['finish_ns']-row['start_ns']):
            raise ValueError('Invalid full-engine block observation')
        previous_finish = row['finish_ns']
        release = (2*i*c['block']*1000000000+rate)//(2*rate) if c['execution_regime'] == 'paced' else -1
        deadline = (2*(i+1)*c['block']*1000000000+rate)//(2*rate) if c['execution_regime'] == 'paced' else -1
        if row['release_ns'] != release or row['deadline_ns'] != deadline:
            raise ValueError('Engine release/deadline differs from policy')
        misses.append(row['duration_ns'] > c['block']*1e9/rate if deadline < 0 else row['finish_ns'] > deadline)
    if ([{k: e[k] for k in ('block', 'kind')} for e in r['lifecycle']] != p['events']
            or any(type(e['duration_ns']) is not int or e['duration_ns'] < 0 for e in r['lifecycle'])):
        raise ValueError('Missing lifecycle intervals')
    replay = r['replay']
    if replay['policy'] != 'rack-module-replay-v1' or replay['origin'] != origin or len(replay['nodes']) != p['analyzers']:
        raise ValueError('Missing independent module replay')
    channels = 4 if c['backend'].startswith('fourier') else 1
    spectra, peaks, publication_age = {}, [], []
    for i, node in enumerate(replay['nodes']):
        pubs, audit = node['publications'], node['accuracy']
        count = len(pubs); a = audit['analysis']
        if (audit['publications'] != count or audit['values'] != count*channels*(c['n']//2+1)
                or a['values'] != audit['values'] or a['vectors'] != count*channels
                or a['policy'] != ('module-decay-ftz-v1' if c['fixture'] == 'decay' else 'spectrum-norms-v1') or a['tolerance'] != 3e-4
                or any(not math.isfinite(a[k]) or not 0 <= a[k] <= 3e-4 for k in ('max_relative_l2', 'max_relative_linf'))):
            raise ValueError('Incomplete all-output engine audit')
        if c['fixture'] == 'decay':
            tail = a.get('tail', {})
            floor = 64*c['n']*2**-126
            if c['backend'].startswith('fourier'):
                # Initial profile controls: shipped default slope 4.5, comparison slope 0.
                slope = 4.5 if c['backend'] == 'fourier-default' else 0.
                bins = c['n']//2+1
                low = 10**(slope*math.log2(2**-23)/20)
                high = 10**(slope*math.log2((c['n']//2)/bins*c['rate']/2000+2**-23)/20)
                floor = max(floor, 2*10**(12/20)*bins/min(low, high)*2**-126)
            if (not math.isclose(tail.get('absolute_limit', 0), floor, rel_tol=1e-12, abs_tol=0) or type(tail.get('vectors')) is not int
                    or not 0 <= tail['vectors'] <= a['vectors']
                    or not 0 <= tail.get('max_absolute_error', math.inf) <= floor
                    or not 0 <= tail.get('max_reference', math.inf) <= floor/3e-4
                    or any(not math.isfinite(tail.get(k, math.inf)) or tail[k] < 0
                           for k in ('max_relative_l2', 'max_relative_linf'))):
                raise ValueError('Invalid separately flagged FTZ decay diagnostics')
        last = -1
        for pub in pubs:
            if pub['sequence'] <= last or pub['bins'] != c['n']//2+1 or pub['published_at'] < pub['endpoint']:
                raise ValueError('Invalid engine publication trace')
            last = pub['sequence']; spectra[i, pub['sequence']] = pub
            if pub['published_at'] >= origin:
                publication_age.append(pub['published_at']-pub['endpoint'])
                first = max(0, (pub['endpoint']-origin)//c['block'])
                final = (pub['published_at']-origin)//c['block']
                peaks.append(max(row['duration_ns'] for row in rows[first:final+1]))
    seen, ages, drains = set(), [], 0
    for row in r['consumption']:
        key = row['node'], row['sequence']
        pub = spectra.get(key)
        if (not p['consumer_hz'] or not pub or key in seen or row['endpoint'] != pub['endpoint']
                or row['published_at'] != pub['published_at'] or not origin <= row['before'] <= row['after'] <= origin+p['blocks']*c['block']
                or type(row['drain']) is not bool or row.get('stable') is not True
                or type(row.get('checksum')) is not int or not 0 <= row['checksum'] < 2**64):
            raise ValueError('Invalid concurrent snapshot observation')
        seen.add(key)
        if row['drain']: drains += 1
        else: ages.append([max(0, row['before']-pub['endpoint']), max(0, row['after']+c['block']-pub['endpoint'])])
    measured = {key for key, pub in spectra.items() if pub['published_at'] >= origin}
    return dict(blocks=len(rows), duration_budget_misses=sum(misses), aggregate_cpu_ns=r['aggregate_cpu_ns'],
                hop_block_peaks_ns=peaks, publication_age_samples=publication_age,
                consumed_age_bounds_samples=ages, final_drain_snapshots=drains,
                skipped_publications=len(measured-seen) if p['consumer_hz'] else None,
                consumer_scope='headless copy/checksum; age bounds use completed-block watermarks; no rendering or device underrun claim')

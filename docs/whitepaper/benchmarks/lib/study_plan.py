"""Resolve the versioned spec-014 pilot design without starting any executable."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib
import json
import math
from pathlib import Path
import random
import re
from contracts import load_registry, validate_config
from engine_host import resolve as resolve_engine
from execution import POLICY, validate_policy
from run import BASE
from workloads import expand
from paths import BENCHMARKS

PROFILE = BENCHMARKS/'profiles/study-014.json'


def identity(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False).encode()).hexdigest()


def session_seeds(value):
    if not isinstance(value, dict) or not value:
        raise ValueError('Session seeds must be a nonempty distinct mapping')
    for label, seed in value.items():
        if (not isinstance(label, str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}', label)
                or type(seed) is not int or not 0 <= seed < 2**32):
            raise ValueError('Invalid session label/seed')
    if len(set(value.values())) != len(value): raise ValueError('Session seeds must be distinct')
    return value


def resolve(path=PROFILE, features=('vdsp', 'fftw')):
    design = json.loads(Path(path).read_text())
    if (design['schema'] != 1 or design['kind'] != 'fourier-study-preparation-v1' or design['phase'] != 'pilot'
            or not set(design['required_features']) <= set(features)):
        raise ValueError('Study requires the declared pilot design and all native providers')
    seeds = session_seeds(design['sessions'])
    if set(seeds) != {'pilot-01', 'pilot-02'}: raise ValueError('Prepare both declared pilot sessions')
    registry = load_registry(features=features)
    groups = []
    for spec in design['groups']:
        configs = []
        if spec['family'] not in ('stream', 'engine'): raise ValueError('Unknown study family')
        if type(spec['repeats']) is not int or not 2 <= spec['repeats'] <= 20: raise ValueError('Invalid pilot repeat count')
        if type(spec['warm_hops']) is not int or not 0 <= spec['warm_hops'] <= 128: raise ValueError('Invalid study warmup')
        for case in spec['cases']:
            if spec['family'] == 'stream':
                for backend in case['backends']:
                    for controls in case['settings']:
                        c = expand(dict(BASE, workload_schema=3, backend=backend, **controls))
                        c['execution_regime'] = spec['regime']; c['warm_hops'] = spec['warm_hops']
                        if type(spec['hops']) is not int or not 2 <= spec['hops'] <= 65536: raise ValueError('Invalid observation length')
                        c['callbacks'] = math.ceil(spec['hops']*c['hop']/c['block'])
                        validate_config(c, registry, measurement=True); configs.append(c)
            else:
                for native in case['native']:
                    for shape in case['shapes']:
                        shape = dict(shape)
                        module = case['module']; fourier = module.startswith('fourier')
                        c = expand(dict(BASE, workload_schema=3, backend=module, window='flattop',
                                        hop=1440 if fourier else 1024, active_ports=4 if fourier else 1,
                                        fixture='independent' if fourier else 'noise', temporal_mode='module-seconds',
                                        block=shape.pop('block', 64), alignment=shape.pop('alignment', 'aligned'),
                                        execution_regime=spec['regime']))
                        c.update(case.get('workload', {}))
                        samples = case.get('samples', spec['samples'])
                        if type(samples) is not int or samples < 2*c['hop']: raise ValueError('Invalid host observation length')
                        configs.append(resolve_engine(dict(workload=c, native=native, blocks=math.ceil(samples/c['block']),
                                                            warm_hops=spec['warm_hops'], **shape), registry))
        if not configs or len({identity(c) for c in configs}) != len(configs): raise ValueError('Empty or duplicate group workload')
        groups.append(dict(name=spec['name'], family=spec['family'], hypothesis=spec['hypothesis'],
                           repeats=spec['repeats'], configs=configs, execution_policy=validate_policy(dict(POLICY, regime=spec['regime']))))
    if len(groups) != 7 or {g['name'] for g in groups} != {'Baselines','Granularity','Scaling','Modules','Host','Stress','Lifecycle'}:
        raise ValueError('Missing required study group')
    result = dict(schema=1, kind='fourier-resolved-pilot-v1', design=design, groups=groups)
    result['plan_id'] = identity(result)
    return result


def jobs(plan, session):
    """Stable group order; each process order is predeclared, independent of results."""
    seed = session_seeds(plan['design']['sessions'])[session]
    result = []
    for group in plan['groups']:
        order = [(repeat, index) for repeat in range(group['repeats']) for index in range(len(group['configs']))]
        random.Random(seed).shuffle(order)
        result += [dict(group=group['name'], family=group['family'], workload=index, repeat=repeat) for repeat, index in order]
    return result


def observation_design(config, publications=0):
    """Descriptive ranks/coverage, never a rare-event confidence guarantee."""
    n = config['callbacks'] if config['pass_name'] == 'callback' else 1
    ranks = {str(p): max(1, math.ceil(p*n)) for p in (.5, .95, .99, .999)}
    return dict(observations=n, quantile_ranks=ranks, observations_above_rank={p:n-r for p,r in ranks.items()},
                engine_samples=config['callbacks']*config['block'], effective_hops=config['callbacks']*config['block']/config['hop'],
                simulated_seconds=config['callbacks']*config['block']/config['rate'], publication_vectors=publications,
                inadequate_tail_sample=n < 10000,
                limitation='Dependent within-process observations; counts and zero misses establish no rare-event probability or WCET.')


def summary(plan):
    counts = {g['name']:len(g['configs']) for g in plan['groups']}
    processes = sum(len(g['configs'])*g['repeats'] for g in plan['groups'])
    paced = 0
    for g in plan['groups']:
        if g['execution_policy']['regime'] != 'paced': continue
        for c in g['configs']:
            samples = c['blocks']*c['workload']['block'] if g['family'] == 'engine' else c['callbacks']*c['block']
            paced += samples/48000*g['repeats']
    return dict(plan_id=plan['plan_id'], readiness=plan['design']['readiness'], workloads=counts, processes_per_session=processes,
                sessions=list(plan['design']['sessions']), nominal_paced_audio_seconds=paced,
                process_settle_seconds=processes, session_settle_seconds=180,
                estimate_scope='Design arithmetic only, not measured runtime; excludes setup, planning, warmup, replay, I/O, overrun and teardown.')


if __name__ == '__main__':
    print(json.dumps(summary(resolve()), indent=2))

"""Offline launch integration with stub children and synthetic clocks only."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import io
import json
import os
from pathlib import Path
import socket
import sys
import tarfile
import tempfile
import unittest
from contextlib import ExitStack, redirect_stdout
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'lib'))
import offline
import run
from execution import Gate, SleepProtection, POLICY
from engine_host import resolve as engine_profile
from workloads import expand
from study_plan import identity
import test_contracts
from test_execution import Clock, process_fixture
from check import check


class OfflineTests(unittest.TestCase):
    def fixture(self, root):
        package = root/'package'; base = package/'base'; base.mkdir(parents=True)
        m = test_contracts.ContractTests().campaign(base)
        m.update(phase='pilot', build_features=[], runtime_profile='coarse-wall-v1', fixture=True, revision='fixture', execution_policy=dict(POLICY))
        run.save(base/'metadata.json', m)
        engine = package/'engine'; engine.mkdir()
        p = engine_profile(dict(workload=expand(dict(run.BASE, workload_schema=3, backend='fourier-default',
            hop=1440, active_ports=4, window='flattop', temporal_mode='module-seconds')), analyzers=0, blocks=48, warm_hops=0))
        for suffix in ('verification.json','resources.json','stderr'): (engine/('Host-0000.'+suffix)).write_text('{}')
        groups = [dict(name='Baselines', family='stream', configs=m['configs'], repeats=2, execution_policy=dict(POLICY)),
                  dict(name='Host', family='engine', configs=[p], repeats=2, execution_policy=dict(POLICY))]
        plan = dict(design=dict(sessions={'pilot-01':17,'pilot-02':19}), groups=groups, plan_id='synthetic-plan')
        value = dict(fixture=True, manifest_id='synthetic-only', plan=plan, rack_dir=str(root/'rack'),
            fftw_prefix=str(root/'fftw'), external_inputs={}, host=dict(host='fixture-host'),
            artifacts={'base/paper.bin':run.digest(base/'paper.bin')})
        run.save(package/'manifest.json', value)
        return package, value

    def exercise(self, failure=None):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t); package,m = self.fixture(root)
            clock = Clock(); events = []; guard = SleepProtection(system='fixture')
            real_stage = offline.stage_groups
            def stage(*args):
                events.append('stage'); return real_stage(*args)
            def make_gate(policy, owner):
                self.assertEqual(events, ['verify', 'stage'])
                result = Gate(policy, owner, clock.now, clock.sleep)
                return result
            def validate(*args):
                events.append('verify')
                if failure == 'stale': raise ValueError('stale input fixture')
                return m
            def child(command, **kwargs):
                self.assertNotIn('make', command); self.assertNotIn('--verify', command)
                self.assertEqual(clock.ns, 180000001000)
                self.assertEqual(guard.record['status'], 'not-applicable')
                events.append('child')
                env = kwargs['env']; self.assertEqual(env['PAPER_PROCESS_SETTLE_MS'], '1000')
                _, _, data, _ = process_fixture(); data['observations'] = data['observations'][:2]
                for i,r in enumerate(data['observations']): r.update(sample=i*64, samples=64)
                if '--engine' in command:
                    profile = json.loads(Path(command[-1]).read_text())
                    data['observations'] = [dict(sample=i*64, samples=64, release_ns=-1, deadline_ns=-1,
                        wake_ns=i*1000, start_ns=i*1000+20, finish_ns=i*1000+120) for i in range(profile['blocks'])]
                    self.assertEqual(env['PAPER_HOST_EXECUTION_PATH'], env['PAPER_EXECUTION_PATH'])
                    rows = [dict(block=i, frame=1+i*64, rate=48000, duration_ns=100,
                                 **{k:v for k,v in r.items() if k != 'sample'}) for i,r in enumerate(data['observations'])]
                    document = dict(profile=profile, result=dict(schema=1, policy='rack-engine-observations-v1',
                        fixture=False, observations=rows, aggregate_cpu_ns=500, origin=1, lifecycle=[], consumption=[],
                        replay=dict(policy='rack-module-replay-v1', origin=1, nodes=[])))
                    json.dump(document, kwargs['stdout'])
                else:
                    self.assertEqual(command[1], 'driver')
                    kwargs['stdout'].write('kind,ns,index,analyzer,sample,samples\n'+'timer,20,,,,\n'*1024+
                                           'callback,100,0,0,0,64\ncallback,100,1,0,64,64\n')
                    run.save(Path(env['PAPER_RUNTIME_PATH']), dict(schema=1,status='complete',unit='ns',total_ns=1,phases_ns={'fixture':1}))
                if failure == 'interrupt': raise KeyboardInterrupt('fixture child interruption')
                if failure == 'child': raise ValueError('fixture child failure')
                run.save(Path(env['PAPER_EXECUTION_PATH']), data)
            with ExitStack() as stack:
                stack.enter_context(patch('socket.socket', side_effect=AssertionError('Network forbidden in offline fixture')))
                stack.enter_context(patch('offline.validate', side_effect=validate))
                stack.enter_context(patch('offline.SleepProtection', return_value=guard))
                stack.enter_context(patch('offline.Gate', side_effect=make_gate))
                stack.enter_context(patch('offline.stage_groups', side_effect=stage))
                stack.enter_context(patch('offline.host_snapshot', return_value=dict(platform='fixture', power='unavailable')))
                stack.enter_context(patch('run.subprocess.run', side_effect=child))
                stack.enter_context(redirect_stdout(io.StringIO()))
                if failure:
                    with self.assertRaises((ValueError, KeyboardInterrupt)):
                        offline.launch(package, 'pilot-01', root/'results', True)
                else: offline.launch(package, 'pilot-01', root/'results', True)
            output = root/'results/pilot-01'
            if failure == 'stale':
                self.assertFalse(output.exists()); self.assertNotIn('child', events); return
            session = json.loads((output/'session.json').read_text())
            self.assertEqual(session['status'], 'interrupted' if failure == 'interrupt' else 'failed' if failure else 'complete')
            archive = output.with_name('pilot-01.handback.tar.gz')
            self.assertTrue(archive.is_file()); self.assertIn(run.digest(archive), archive.with_suffix('.gz.sha256').read_text())
            with tarfile.open(archive) as tar:
                self.assertTrue(all(Path(name).name not in offline.RESTRICTED for name in tar.getnames()))
                self.assertIn('Baselines/bundle-omissions.json', tar.getnames())
                self.assertIn('prepared-manifest.json', tar.getnames())
            if failure:
                self.assertEqual(events.count('child'), 1)
                self.assertTrue((output/'Baselines/workload-0000-repeat-01.csv').exists() or
                                (output/'Baselines/workload-0000-repeat-00.csv').exists())
            else:
                self.assertEqual(events.count('child'), 4)
                self.assertEqual(session['groups'], ['Baselines','Host'])
                self.assertEqual(check(output/'Baselines'), 2)
                self.assertTrue((output/'COMPLETE').is_file())
                with self.assertRaisesRegex(ValueError, 'already exists'):
                    offline.launch(package, 'pilot-01', root/'results', True)
            with self.assertRaisesRegex(ValueError, 'undeclared'):
                offline.launch(package, '../escape', root/'results', True)

    def test_serial_dispatch_packaging_and_complete_integrity(self): self.exercise()
    def test_interrupted_child_retains_partial_archive(self): self.exercise('interrupt')
    def test_failed_child_retains_partial_archive(self): self.exercise('child')
    def test_stale_inputs_never_launch_child(self): self.exercise('stale')

    def test_lock_and_declaration(self):
        with tempfile.TemporaryDirectory() as t:
            with offline.serial_lock(Path(t)/'lock'):
                with self.assertRaisesRegex(ValueError, 'active'):
                    with offline.serial_lock(Path(t)/'lock'): pass
            with self.assertRaisesRegex(ValueError, 'declaration'):
                offline.launch(Path(t), 'pilot-01', Path(t), False)

    def test_make_dispatch_has_no_build_prerequisite(self):
        for action in ('run','check','prepare'):
            result = __import__('subprocess').run(['make','-n','benchmark-study-'+action,'SESSION=pilot-02'],
                cwd=run.ROOT, capture_output=True, text=True, check=True)
            self.assertIn('bench.py study-'+action, result.stdout)
            self.assertNotIn(' -c ', result.stdout)
            if action == 'run': self.assertIn('--session "pilot-02"', result.stdout)

    def test_resource_checker_requires_explicit_untimed_policy(self):
        from check import validate_resources
        with tempfile.TemporaryDirectory() as t:
            root=Path(t); test_contracts.ContractTests().campaign(root)
            resource=json.loads((root/'resources.json').read_text())
            for item in resource.values():
                item['timing_policy']='untimed-v1'
                for phase in ('setup','execution','destruction'): item[phase]['ns']=None
            validate_resources(resource)
            bad=copy.deepcopy(resource); bad['timing']['setup']['ns']=0
            with self.assertRaises(ValueError): validate_resources(bad)
            bad=copy.deepcopy(resource); bad['timing'].pop('timing_policy')
            with self.assertRaises(ValueError): validate_resources(bad)


if __name__ == '__main__': unittest.main()

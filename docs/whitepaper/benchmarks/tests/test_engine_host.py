"""Actual engine correctness plus explicitly synthetic recording-path fixtures."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'lib'))
from paths import ROOT
from contracts import load_registry
from generate_registry import generate
from engine_host import resolve, validate
from workloads import expand
from run import BASE


class EngineHostTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rack = Path(os.environ.get('RACK_DIR', ROOT/'../..')).resolve()
        if not (cls.rack/'include/rack.hpp').exists(): raise unittest.SkipTest('Rack source/SDK required')
        cls.temp = tempfile.TemporaryDirectory(prefix='fourier-engine-fixture-')
        cls.addClassCleanup(cls.temp.cleanup)
        cls.root = Path(cls.temp.name); cls.binary = cls.root/'verify-engine'
        features, flags, libraries = [], [], []
        if platform.system() == 'Darwin':
            features += ['vdsp']; flags += ['-DPAPER_HAVE_VDSP']; libraries += ['-framework', 'Accelerate']
        prefix = ROOT/'.build/deps/fftw'
        if (prefix/'include/fftw3.h').exists():
            features += ['fftw']; flags += ['-DPAPER_HAVE_FFTW', '-I'+str(prefix/'include')]
            libraries += [str(prefix/'lib/libfftw3f.a'), str(prefix/'lib/libfftw3.a')]
        cls.registry = load_registry(features=features)
        generate(cls.root/'registry.generated.hpp', features)
        command = shlex.split(os.environ.get('CXX', 'c++'))+[
            '-std=c++11', '-O3', '-funsafe-math-optimizations', '-DTEST', '-pthread',
            '-I'+str(cls.rack/'include'), '-I'+str(cls.rack/'dep/include'), '-I'+str(cls.root)]+flags+[
            str(ROOT/'test/paper/verify_engine_host.cpp'), '-L'+str(cls.rack), '-lRack']+libraries+['-o', str(cls.binary)]
        result = subprocess.run(command, capture_output=True, text=True, timeout=180)
        if result.returncode: raise AssertionError(result.stdout+result.stderr)
        cls.env = dict(os.environ, DYLD_LIBRARY_PATH=str(cls.rack), LD_LIBRARY_PATH=str(cls.rack),
                       PAPER_HOST_EXECUTION_PATH=str(cls.root/'execution.json'), PAPER_PROCESS_SETTLE_MS='1')

    def invoke(self, *args):
        result = subprocess.run([str(self.binary), *args], env=self.env, capture_output=True, text=True, timeout=180)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        return result.stdout

    def profile(self, **options):
        c = expand(dict(BASE, workload_schema=3, backend='fourier-default', n=2048, hop=1440, block=256,
                        window='flattop', temporal_mode='module-seconds', active_ports=4, fixture='independent'))
        c = {k: v for k, v in c.items() if k not in ('callbacks', 'warm_hops')}
        return resolve(dict(workload=c, blocks=32, **options), self.registry)

    def test_defaults_extreme_threads_counts_and_lifecycle(self):
        self.assertIn('passed without benchmark timing', self.invoke())

    def test_recording_cpu_budgets_consumer_and_replay(self):
        for options in (dict(threads=1, background=16, consumer_hz=30),
                        dict(threads=4, analyzers=4, background=64, consumer_hz=60, stall_every=2),
                        dict(threads=4, analyzers=0), dict(threads=1, events=[dict(block=4, kind='reset')])):
            p = self.profile(**options)
            if p['threads'] == 1: p['workload']['block'] = 64; p['blocks'] = 64
            path = self.root/'profile.json'; path.write_text(json.dumps(p))
            data = json.loads(self.invoke('record', str(path)))
            summary = validate(data, self.registry, allow_fixture=True)
            self.assertEqual(summary['blocks'], p['blocks'])
            self.assertTrue(json.loads((self.root/'execution.json').read_text())['fixture'])
            with self.assertRaises(ValueError): validate(data, self.registry)
            for mutate in (lambda d: d['result']['observations'].pop(),
                           lambda d: d['result'].__setitem__('aggregate_cpu_ns', -1),
                           lambda d: d['result']['observations'][0].__setitem__('deadline_ns', 10)):
                bad = copy.deepcopy(data); mutate(bad)
                with self.assertRaises(ValueError): validate(bad, self.registry, allow_fixture=True)

    def test_paced_and_native_complete_modules(self):
        for native in ('', 'vdsp-native4-batch-float', 'vdsp-native4-hybrid-float', 'fftw-native4-batch-float', 'fftw-native4-hybrid-float'):
            if native and native not in self.registry: continue
            p = self.profile(native=native, threads=4, analyzers=1)
            p['workload']['execution_regime'] = 'paced'
            self.env['PAPER_EXECUTION_REGIME'] = 'paced'
            try:
                path = self.root/'profile.json'; path.write_text(json.dumps(p))
                data = json.loads(self.invoke('record', str(path)))
                validate(data, self.registry, allow_fixture=True)
            finally: self.env.pop('PAPER_EXECUTION_REGIME', None)

    def test_long_decay_all_paths_under_rack_fpu(self):
        original = json.loads((ROOT/'docs/whitepaper/benchmarks/profiles/engine/long-decay-regression.json').read_text())
        for module, native in (('fourier-default', ''), ('fourier-default', 'vdsp-native4-hybrid-float'),
                              ('fourier-default', 'fftw-native4-hybrid-float'), ('spectre-default', ''),
                              ('spectre-default', 'pffft-native-hybrid-float')):
            if native and native not in self.registry: continue
            p = copy.deepcopy(original); p['native'] = native; p['workload']['backend'] = module
            if module.startswith('spectre'): p['workload'].update(hop=1024, active_ports=1)
            path = self.root/'long-decay.json'; path.write_text(json.dumps(p))
            result = json.loads(self.invoke('verify', str(path)))
            audit = result['nodes'][0]['accuracy']['analysis']
            self.assertEqual(audit['policy'], 'module-decay-ftz-v1')
            self.assertGreater(audit['tail']['vectors'], 0)
            self.assertGreater(audit['zero_vectors'], 0)
            self.assertLessEqual(audit['tail']['max_absolute_error'], audit['tail']['absolute_limit'])

    def test_reject_ambiguous_or_unsupported_profiles(self):
        p = self.profile()
        for changes in (dict(threads=2), dict(analyzers=2), dict(consumer_hz=20), dict(blocks=True),
                        dict(unknown=1), dict(consumer_hz=30, events=[dict(block=0, kind='reset')])):
            with self.subTest(changes=changes), self.assertRaises(ValueError): resolve(dict(p, **changes), self.registry)


if __name__ == '__main__': unittest.main()

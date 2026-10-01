"""Synthetic-only seed/freeze and immutable preparation failure tests."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'lib'))
from study import freeze, enforce, confirmations, seed_for_session, require_real_evidence
from prepared import seal, validate, snapshot
from run import source_inputs, save
from study_plan import resolve
from test_workflow import fixture


class StudyPoliciesTests(unittest.TestCase):
    def test_predeclared_seeds_and_fixture_boundary(self):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t); pilots = [root/'pilot-1', root/'pilot-2']
            metadata = [fixture(p, 'pilot', str(i)) for i,p in enumerate(pilots)]
            options = dict(repeats=5, seed=7)
            policy = dict(schema=1, kind='predeclared-session-seeds-v1', sessions={'c1': 1,'c2': 2,'c3': 3})
            with self.assertRaisesRegex(ValueError, 'Synthetic'):
                freeze(pilots, metadata[0]['configs'], options, 'rack', 'fixture', root/'forbidden.json')
            f = freeze(pilots, metadata[0]['configs'], options, 'rack', 'fixture', root/'freeze.json',
                       fixture=True, session_seed_policy=policy)
            for session, seed in policy['sessions'].items():
                m = dict(metadata[0], session_id=session, seed=seed, repeats=5)
                enforce(f, m)
                with self.assertRaises(ValueError): enforce(f, dict(m, seed=99))
            with self.assertRaises(ValueError): seed_for_session(f, 'new-label')
            with self.assertRaises(ValueError): enforce(f, dict(m, revision='real', fixture=False))
            with self.assertRaises(ValueError): require_real_evidence(dict(m, study_freeze=f))
            legacy = freeze(pilots, metadata[0]['configs'], dict(repeats=1, seed=7), 'rack', 'fixture',
                            root/'legacy.json', fixture=True)
            self.assertEqual(seed_for_session(legacy, 'any-label'), 7)
            with self.assertRaises(ValueError): enforce(legacy, dict(metadata[0], seed=2))
            records = [dict(metadata[0], phase='confirmation', session_id=f'c{i+1}', seed=i+1,
                            repeats=5, study_freeze=f, started_utc=f'2026-10-0{i+1}T12:00:00+00:00') for i in range(3)]
            with patch('study.evidence', side_effect=records):
                self.assertEqual(len(confirmations(pilots+[root/'third'], allow_fixture=True)), 3)
            with patch('study.evidence', side_effect=records), self.assertRaises(ValueError):
                confirmations(pilots+[root/'third'])
            records[2]['started_utc'] = records[0]['started_utc']
            with patch('study.evidence', side_effect=records), self.assertRaises(ValueError):
                confirmations(pilots+[root/'third'], allow_fixture=True)

    def test_prepared_artifacts_source_membership_and_host(self):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t); (root/'src').mkdir(); package = root/'.build/prepared'; package.mkdir(parents=True)
            for name in ('Makefile','plugin.json','src/example.hpp'): (root/name).write_text('fixture source')
            (package/'paper.bin').write_bytes(b'fixture - not executable')
            plan = resolve(); inputs = snapshot(source_inputs(root))
            m = seal(package, plan, inputs, root)
            self.assertEqual(validate(package)['manifest_id'], m['manifest_id'])
            (root/'src/new.hpp').write_text('new')
            with self.assertRaisesRegex(ValueError, 'membership'): validate(package)
            (root/'src/new.hpp').unlink(); (package/'paper.bin').write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError, 'artifact changed'): validate(package)
            (package/'paper.bin').write_bytes(b'fixture - not executable')
            (root/'src/example.hpp').write_text('changed')
            with self.assertRaisesRegex(ValueError, 'stale'): validate(package)
            (root/'src/example.hpp').write_text('fixture source')
            with patch('prepared.host_identity', return_value={}):
                with self.assertRaisesRegex(ValueError, 'host'): validate(package)
            bad = copy.deepcopy(m); bad['plan']['groups'][0]['repeats'] += 1; save(package/'manifest.json', bad)
            with self.assertRaisesRegex(ValueError, 'manifest'): validate(package)


if __name__ == '__main__': unittest.main()

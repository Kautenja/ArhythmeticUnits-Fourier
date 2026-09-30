"""Planning arithmetic only: no benchmark process is launched by these tests."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'lib'))
from study_plan import resolve, summary, jobs, session_seeds, observation_design
from run import BASE


class StudyPlanTests(unittest.TestCase):
    def test_required_groups_providers_and_fixed_membership(self):
        plan = resolve(); description = summary(plan)
        self.assertEqual(description['readiness'], 'blocked')
        self.assertEqual(description['processes_per_session'], 388)
        self.assertEqual(description['workloads'], dict(Baselines=24, Granularity=39, Scaling=16,
                                                       Modules=9, Stress=12, Host=83, Lifecycle=11))
        with self.assertRaises(ValueError): resolve(features=('vdsp',))
        one, two = jobs(plan, 'pilot-01'), jobs(plan, 'pilot-02')
        self.assertEqual(one, jobs(plan, 'pilot-01')); self.assertNotEqual(one, two)
        self.assertCountEqual(one, two)
        self.assertEqual(len({json.dumps(j,sort_keys=True) for j in one}), 388)
        with self.assertRaises(KeyError): jobs(plan, '../pilot-01')

    def test_seed_policy_and_descriptive_tail_warning(self):
        for seeds in ({}, {'bad/label': 1}, {'one': True}, {'one': 2**32}, {'one': 1,'two': 1}):
            with self.subTest(seeds=seeds), self.assertRaises(ValueError): session_seeds(seeds)
        result = observation_design(dict(BASE, callbacks=100, warm_hops=0), 6)
        self.assertEqual(result['quantile_ranks']['0.99'], 99)
        self.assertEqual(result['observations_above_rank']['0.99'], 1)
        self.assertTrue(result['inadequate_tail_sample'])
        self.assertEqual(result['engine_samples'], 6400)


if __name__ == '__main__': unittest.main()

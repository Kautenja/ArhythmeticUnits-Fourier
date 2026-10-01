"""Regression examples for coordinate-preserving fresh-study analysis."""
import unittest
import math
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from study_import import hop_peaks,timing


class StudyAnalysisTests(unittest.TestCase):
    def test_weighted_first_credit_bound_requires_boundary_carry(self):
        weights = [4, 1, 2, 1, 4, 2]
        starts = {}; cursor = 0
        for weight in weights:
            starts[cursor] = weight; cursor += weight
        work = cursor; needs_carry = False
        for horizon in range(1, 33):
            for start in range(horizon):
                for length in range(1, 2*horizon+1):
                    first = start*work//horizon
                    last = (start+length)*work//horizon
                    cost = sum(starts.get(i % work, 0) for i in range(first, last))
                    quota = math.ceil(length*work/horizon)
                    needs_carry |= cost > quota
                    self.assertLessEqual(cost, quota+max(weights)-1)
        self.assertTrue(needs_carry)

    def test_immediate_batch_still_uses_the_complete_hop(self):
        # A burst late in a hop must not disappear because publication was immediate.
        self.assertEqual(hop_peaks([2,9,3,8],4,100,[100,108],8),[9,8])

    def test_partial_edges_excluded_and_shared_blocks_retained(self):
        # Complete hops [3,9), [9,15); block [8,12) contributes to both.
        self.assertEqual(hop_peaks([1,2,99,3],4,0,[-3,3,9,15],6),[99,99])

    def test_duplicate_endpoints_are_not_extra_replicates(self):
        self.assertEqual(hop_peaks([4,3],4,0,[0,0,8],8),[4])

    def test_rate_change_uses_each_blocks_budget(self):
        r=timing([1000000,1000000],64,48000,[],False,[48000,96000])
        self.assertEqual(r['compute_budget_misses'],1)

    def test_release_miss_separates_wake_delay_from_compute(self):
        r=timing([10],64,48000,[dict(wake_ns=2000000,start_ns=2000000,
            finish_ns=2000010,release_ns=0,deadline_ns=1333333)],True)
        self.assertEqual(r['compute_budget_misses'],0)
        self.assertEqual(r['release_misses'],1)
        self.assertEqual(r['wake_lateness']['median'],2000000)


if __name__=='__main__':unittest.main()

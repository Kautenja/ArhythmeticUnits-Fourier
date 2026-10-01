#!/usr/bin/env python3
"""Compare preserved spec 011 summaries without discarding timing outliers."""
import argparse
import csv
import gzip
import json
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', default='baseline-b')
    parser.add_argument('--candidate', default='combined-a')
    parser.add_argument('--backend', help='Exact backend name; default includes all')
    args = parser.parse_args()
    with gzip.open(Path(__file__).with_name('measurements.json.gz'), 'rt') as stream:
        measurements = json.load(stream)
    baseline = measurements[args.baseline]
    candidate = measurements[args.candidate]
    for run in (baseline, candidate):
        if run['status'] != 'complete':
            raise ValueError('Cannot compare an incomplete run')
    writer = csv.writer(sys.stdout)
    writer.writerow(['workload', 'baseline_cost', 'candidate_cost',
                     'cost_ratio', 'baseline_p99_ns', 'candidate_p99_ns', 'p99_ratio'])
    for key, measured in candidate['results'].items():
        if args.backend and key.split('/')[0] != args.backend:
            continue
        original = baseline['results'][key]  # Reject unmatched configurations.
        if baseline['workloads'][key] != candidate['workloads'][key]:
            raise ValueError('Incompatible workload configuration')
        before = original['median_p99_ns']
        after = measured['median_p99_ns']
        writer.writerow([key, original['mean_cost'], measured['mean_cost'],
                         measured['mean_cost'] / original['mean_cost'], before, after,
                         after / before if before else ''])


if __name__ == '__main__':
    main()

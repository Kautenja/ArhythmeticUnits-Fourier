#!/usr/bin/env python3
"""Summarize spec 009 runs without pooling repetitions across experiments."""
import csv
import json
import statistics
import sys
from pathlib import Path


def read_run(directory):
    result = json.loads((directory / 'results.json').read_text())
    if result['status'] != 'complete':
        raise ValueError(f'Incomplete experiment: {directory}')
    return result


def phases(directory, result):
    """Aligned 64-sample callbacks in live H=1024 frames; timer stays included."""
    groups = {}
    for run in result['runs']:
        config = result['workloads'][run['workload']]
        if not (config['backend'] == 'core-float'
                and config['pass_name'] == 'callback'
                and config['state'] == 'live' and config['hop'] == 1024
                and config['block'] == 64 and config['count'] == 1
                and config['callback_offset'] == 0):
            continue
        warm_frames = (config['n'] + 1023) // 1024 + config['warm_hops']
        with (directory / run['raw']).open() as stream:
            for row in csv.DictReader(stream):
                if row['kind'] != 'callback':
                    continue
                sample = int(row['sample'])
                window = 'hann' if (warm_frames + sample // 1024) % 2 else 'blackman-harris'
                key = f"N={config['n']}/{window}/offset={sample % 1024}"
                groups.setdefault(key, []).append(float(row['ns']))
    return {key: statistics.median(values) for key, values in sorted(groups.items())}


def main():
    baseline_path = Path(sys.argv[1])
    baseline = read_run(baseline_path)
    output = {}
    for argument in sys.argv[1:]:
        path = Path(argument)
        candidate = read_run(path)
        if candidate['workloads'] != baseline['workloads']:
            raise ValueError(f'Mismatched workloads: {path}')
        rows = {}
        for key, measured in candidate['results'].items():
            original = baseline['results'][key]
            rows[key] = dict(measured,
                cost_ratio=measured['mean_cost'] / original['mean_cost'],
                p99_ratio=measured['median_p99_ns'] / original['median_p99_ns'])
        output[path.name] = {
            'results': rows,
            'median_callback_ns_by_live_frame_offset': phases(path, candidate),
            'source_identity': candidate['sources_fnv1a64'],
            'evidence': candidate['evidence'],
        }
    print(json.dumps(output, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()

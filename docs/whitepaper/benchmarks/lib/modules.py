"""Complete module-output evidence, separate from core/native analysis records."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import math
from workloads import alpha

POLICY = "all-module-outputs-v1"


def validate(accuracy, config, publications, contract):
    from check import validate_analysis_accuracy
    expected = publications*contract['channels']*(config['n']//2+1)
    if (accuracy.get('module_policy') != POLICY or accuracy.get('checked_samples') != expected
            or accuracy.get('publications') != publications or accuracy.get('playback_checked_samples') != 0
            or len(accuracy.get('module_controls', [])) != config['count']):
        raise ValueError('Missing complete module-output audit')
    validate_analysis_accuracy(accuracy, config, publications, contract, 'spectrum-norms-v1')
    for controls in accuracy['module_controls']:
        fields = dict(policy='module-controls-v1', backend=config['backend'], n=config['n'], hop=config['hop'],
                      rate=config['rate'], active_ports=config.get('active_ports', contract['channels']),
                      voices_per_active_port=config['voices'], ac_coupled=True, snapshot_metadata_instrumentation=True)
        if any(controls.get(k) != v for k, v in fields.items()):
            raise ValueError('Module effective controls differ from workload')
        if not controls.get('params') or not all(math.isfinite(v) for v in controls['params']):
            raise ValueError('Missing quantized module parameters')
        seconds = config['temporal_value'] if config.get('workload_schema') == 3 else (.1 if config['smooth'] else 0)
        effective = alpha(config) if config.get('workload_schema') == 3 else (
            math.exp(-10*(config['hop']/config['rate'])/seconds) if seconds else 0)
        if not math.isclose(controls['time_seconds'], seconds, rel_tol=1e-6, abs_tol=1e-8) or not math.isclose(
                controls['alpha'], effective, rel_tol=2e-6, abs_tol=1e-7):
            raise ValueError('Module smoothing differs from effective controls')

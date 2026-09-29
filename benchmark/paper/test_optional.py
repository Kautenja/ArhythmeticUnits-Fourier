"""Optional provider availability, policy, and dependency evidence regressions."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
import tempfile
import unittest

from check import validate_provider_info
from contracts import load_registry, validate_config
from dependencies import checksum, fftw_inputs
from run import BASE


class OptionalTests(unittest.TestCase):
    def test_fftw_requires_explicit_feature(self):
        config = dict(BASE, backend="fftw-analysis-float")
        with self.assertRaisesRegex(ValueError, "disabled"):
            validate_config(config)
        registry = load_registry(features=("fftw",))
        validate_config(config, registry)
        validate_config(dict(config, backend="fftw-ols-fir-double"), registry)
        with self.assertRaises(ValueError):
            validate_config(dict(config, backend="fftw-fft-float", pass_name="steps"), registry)

    def test_fftw_plan_policy_and_actual_plan_required(self):
        registry = load_registry(features=("fftw",))
        descriptor = registry["fftw-ols-fir-double"]
        valid = dict(provider="fftw", precision="double", threads=1, plan_policy="FFTW_MEASURE",
                     imported_wisdom=False, version="fftw-3.3.10", exported_wisdom="wisdom",
                     forward_plan="forward", inverse_plan="inverse")
        validate_provider_info(valid, descriptor)
        for changes in (dict(threads=2), dict(plan_policy="FFTW_ESTIMATE"), dict(imported_wisdom=True),
                        dict(forward_plan=None), dict(inverse_plan=None)):
            with self.assertRaises(ValueError):
                validate_provider_info(dict(valid, **changes), descriptor)

    def test_optional_dependency_bytes_match_provenance(self):
        with tempfile.TemporaryDirectory() as temp:
            prefix = Path(temp)
            with self.assertRaises(ValueError):
                fftw_inputs(prefix)
            for name in ("include/fftw3.h", "lib/libfftw3.a", "lib/libfftw3f.a"):
                path = prefix/name
                path.parent.mkdir(exist_ok=True)
                path.write_text(name)
            provenance = {"files": {str(p.relative_to(prefix)): checksum(p) for p in prefix.rglob("*") if p.is_file()}}
            (prefix/"provenance.json").write_text(json.dumps(provenance))
            self.assertEqual(len(fftw_inputs(prefix)), 4)
            (prefix/"lib/libfftw3.a").write_text("changed native implementation")
            with self.assertRaisesRegex(ValueError, "differs"):
                fftw_inputs(prefix)


if __name__ == "__main__":
    unittest.main()

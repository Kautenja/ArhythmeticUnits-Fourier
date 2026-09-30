"""Central numerical/transition wiring and authenticated evidence regressions."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import csv
import os
from pathlib import Path
import shlex
import subprocess
import tarfile
import tempfile
import unittest

from check import check, transition_artifacts
from contracts import resolve_contract
from generate_registry import generate
from observations import read_observations, summarize
from run import BASE, command, digest, save
import test_contracts
from transitions import expected_publications

ROOT = Path(__file__).resolve().parents[3]
FIELDS = ("kind", "ns", "analyzer", "sample", "index", "samples", "endpoint_age_samples",
          "center_age_samples", "callback_visible_age_samples", "playback_delay_samples")


def configuration():
    return dict(BASE, backend="core-float", n=128, hop=16, block=16,
                callbacks=26, count=1, voices=1, state="startup", alignment="aligned",
                warm_hops=0, callback_offset=0, load=0, smooth=0, cache_mib=0,
                transition_suite="interactive-v1", transition_control=False)


def raw_rows(config):
    rows = [dict(kind="timer", ns=20) for _ in range(1024)]
    rows += [dict(kind="callback", ns=100, index=i, sample=i*config["block"], samples=config["block"], analyzer=0)
             for i in range(config["callbacks"])]
    for publication in expected_publications(config):
        sample = publication["publication_sample"]
        age = sample-publication["endpoint"]
        rows.append(dict(kind="publication", ns=0, analyzer=0, sample=sample,
                         endpoint_age_samples=age,
                         center_age_samples=age+(publication["n"]-1)/2,
                         callback_visible_age_samples=age+config["block"]-1-sample%config["block"],
                         playback_delay_samples=-1))
    return rows


def write_rows(path, rows):
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=FIELDS)
        writer.writeheader()
        writer.writerows(rows)


class EvidenceIntegrationTests(unittest.TestCase):
    def test_command_preserves_static_protocol_and_transition_mode(self):
        config = configuration()
        static = dict(config)
        static.pop("transition_suite")
        static.pop("transition_control")
        ordinary = command(static)
        self.assertEqual(ordinary[1:5], ["core-float", "callback", "128", "16"])
        self.assertEqual(ordinary[-2:], ["0", "v2"])
        changed = command(config)
        self.assertEqual(changed[1:4], ["--transition", "interactive-v1", "change"])
        self.assertEqual(changed[4:], ordinary[1:])
        control = command(dict(config, transition_control=True))
        self.assertEqual(control[1:4], ["--transition", "interactive-v1", "control"])
        self.assertEqual(control[4:], ordinary[1:])

    def test_transition_policy_and_artifact_aliases(self):
        metadata = dict(configs=[configuration()], transition_policy="fourier-transitions-v1",
                        runs=[dict(workload=0, transition="trace.json")])
        required = {"raw.csv", "stderr.txt", "runtime.json", "source.tar.gz"}
        self.assertEqual(transition_artifacts(metadata, required), ["trace.json"])
        mutations = [lambda m: m.pop("transition_policy"),
                     lambda m: m.update(transition_policy="unknown"),
                     lambda m: m["runs"][0].pop("transition"),
                     lambda m: m["configs"][0].pop("transition_suite"),
                     lambda m: m["runs"].append(dict(m["runs"][0]))]
        for name in ("metadata.json", "raw.csv", "stderr.txt", "runtime.json", "source.tar.gz",
                     "../trace.json", "nested/trace.json", "", ".", "..", None):
            mutations.append(lambda m, name=name: m["runs"][0].update(transition=name))
        for mutation in mutations:
            changed = copy.deepcopy(metadata)
            mutation(changed)
            with self.subTest(metadata=changed), self.assertRaises(ValueError):
                transition_artifacts(changed, required)
        self.assertEqual(transition_artifacts(dict(configs=[dict(BASE)], runs=[dict(workload=0)]), set()), [])

    def test_transition_sidecar_authentication_precedes_interpretation(self):
        # Use the real campaign checker; each mutation must fail at its intended
        # authentication gate, before an invalid trace body can be interpreted.
        for change, expected_error in (("unhashed", "Missing artifact checksums"),
                                       ("changed", "checksum mismatch"),
                                       ("missing", "Missing or aliased"),
                                       ("symlink", "Missing or aliased")):
            with self.subTest(change=change), tempfile.TemporaryDirectory() as temp:
                directory = Path(temp)
                metadata = test_contracts.ContractTests().campaign(directory)
                metadata["configs"] = [configuration()]
                metadata["transition_policy"] = "fourier-transitions-v1"
                metadata["runs"][0]["transition"] = "trace.json"
                trace = directory/"trace.json"
                trace.write_text('{"fixture":"authentication only"}\n')
                metadata["artifact_sha256"][trace.name] = digest(trace)
                if change == "unhashed":
                    del metadata["artifact_sha256"][trace.name]
                elif change == "changed":
                    trace.write_text('{"fixture":"changed"}\n')
                elif change == "missing":
                    trace.unlink()
                else:
                    trace.rename(directory/"original.json")
                    try:
                        trace.symlink_to("original.json")
                    except OSError:
                        self.skipTest("Symbolic links unavailable")
                save(directory/"metadata.json", metadata)
                with self.assertRaisesRegex(ValueError, expected_error):
                    check(directory)

    def test_variable_publication_ages_and_raw_rejections(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"raw.csv"
            for backend in ("core-float", "pffft-hybrid-float", "pffft-scheduled-batch-float"):
                config = dict(configuration(), backend=backend)
                rows = raw_rows(config)
                publications = [row for row in rows if row["kind"] == "publication"]
                self.assertGreater(len({row["center_age_samples"] for row in publications}), 1)
                write_rows(path, rows)
                summary, details = read_observations(path, config, report=True)
                self.assertEqual(summary["publication_audit_rows"], len(publications))
                self.assertEqual(details["observation_count"], config["callbacks"])
                visible = [row["callback_visible_age_samples"] for row in publications]
                self.assertEqual(details["callback_visible_age_range"], [min(visible), max(visible)])
                start = 1024+config["callbacks"]
                mutations = [lambda r: r.pop(), lambda r: r.append(dict(r[-1])),
                             lambda r: r[1024].update(sample=1),
                             lambda r: r[1024].update(index=1),
                             lambda r: r[1024].update(samples=15),
                             lambda r: r[1024].update(analyzer=1),
                             lambda r: r[start].update(sample=r[start]["sample"]+1),
                             lambda r: r[start].update(center_age_samples=r[start]["center_age_samples"]+1),
                             lambda r: r[start].update(endpoint_age_samples=r[start]["endpoint_age_samples"]+1),
                             lambda r: r[start].update(analyzer=1),
                             lambda r: r[start+1].update(r[start]),
                             lambda r: r[start].update(callback_visible_age_samples=float("nan"))]
                for mutation in mutations:
                    changed = copy.deepcopy(rows)
                    mutation(changed)
                    write_rows(path, changed)
                    with self.subTest(backend=backend, mutation=mutation), self.assertRaises(ValueError):
                        read_observations(path, config, report=True)

    def test_historical_scalar_preflight_and_new_policy_requirement(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            metadata = test_contracts.ContractTests().campaign(directory)
            config = dict(metadata["configs"][0], backend="core-float")
            metadata["configs"] = [config]
            metadata["contracts"]["0"] = resolve_contract(config)
            rows = [dict(kind="timer", ns=20) for _ in range(1024)]
            rows += [dict(kind="callback", ns=100) for _ in range(config["callbacks"])]
            for sample in (31, 63, 95, 127):
                rows.append(dict(kind="publication", ns=0, analyzer=0, sample=sample,
                                 endpoint_age_samples=31, center_age_samples=94.5,
                                 callback_visible_age_samples=31+63-sample%64, playback_delay_samples=-1))
            write_rows(directory/"raw.csv", rows)
            metadata["runs"][0]["summary"] = summarize(directory/"raw.csv", config)
            metadata["artifact_sha256"]["raw.csv"] = digest(directory/"raw.csv")
            save(directory/"metadata.json", metadata)
            self.assertEqual(check(directory), 1)  # Old scalar report is preflight-only.
            changed = copy.deepcopy(metadata)
            changed["scalar_analysis_audit_policy"] = "unknown"
            save(directory/"metadata.json", changed)
            with self.assertRaisesRegex(ValueError, "Unknown scalar"):
                check(directory)
            # New implementation provenance cannot omit the policy declaration.
            header_name = "benchmark/paper/scalar_analysis_audit.hpp"
            with tarfile.open(directory/"source.tar.gz", "w:gz") as archive:
                archive.add(ROOT/"docs/whitepaper/benchmarks/backends.json",
                            arcname="docs/whitepaper/benchmarks/backends.json")
                archive.add(ROOT/header_name, arcname=header_name)
            metadata["source_sha256"][header_name] = digest(ROOT/header_name)
            metadata["artifact_sha256"]["source.tar.gz"] = digest(directory/"source.tar.gz")
            save(directory/"metadata.json", metadata)
            with self.assertRaisesRegex(ValueError, "Missing scalar numerical coverage policy"):
                check(directory)

    def test_scalar_cpp_fixtures_under_supported_arithmetic_flags(self):
        rack = Path(os.environ.get("RACK_DIR", str(ROOT.parents[1]))).resolve()
        if not (rack/"include/rack.hpp").is_file():
            self.skipTest("Scalar Rack-type fixture requires RACK_DIR with Rack headers")
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            generate(directory/"registry.generated.hpp")
            binary = directory/"verify-scalar"
            args = shlex.split(os.environ.get("CXX", "c++"))
            args += ["-std=c++11", "-O3", "-funsafe-math-optimizations", "-DTEST",
                     "-I"+str(rack/"include"), "-I"+str(rack/"dep/include"), "-I"+temp,
                     str(ROOT/"test/paper/verify_scalar_analysis.cpp"), "-L"+str(rack), "-lRack", "-o", str(binary)]
            subprocess.run(args, check=True, capture_output=True, text=True, timeout=120)
            environment = dict(os.environ)
            for key in ("DYLD_LIBRARY_PATH", "LD_LIBRARY_PATH", "PATH"):
                environment[key] = str(rack)+os.pathsep+environment.get(key, "")
            result = subprocess.run([str(binary)], check=True, capture_output=True, text=True,
                                    env=environment, timeout=120)
            self.assertIn("198 scalar factor cases and eight negative fixtures passed", result.stdout)


if __name__ == "__main__":
    unittest.main()

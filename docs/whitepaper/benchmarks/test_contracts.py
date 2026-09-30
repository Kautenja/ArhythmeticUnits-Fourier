"""Capability contracts and schema-2 evidence failure regression checks."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import json
from pathlib import Path
import tarfile
import tempfile
import unittest

from check import check, validate_rows, validate_resources, validate_transform_accuracy
from contracts import REGISTRY, normalize_registry, resolve_contract, validate_config
from run import BASE, digest, matrix, save, summarize


class ContractTests(unittest.TestCase):
    def test_registry_rejects_ambiguous_semantics(self):
        original = json.loads(Path(__file__).with_name("backends.json").read_text())
        for field, value in (("schedule", "mystery"), ("channels", "four"),
                             ("size_multiple", 0), ("step_model", "guessed")):
            altered = copy.deepcopy(original)
            altered["backends"][0][field] = value
            with self.assertRaises(ValueError):
                normalize_registry(altered)
        original["backends"].append(original["backends"][0])
        with self.assertRaisesRegex(ValueError, "Duplicate"):
            normalize_registry(original)

    def test_capabilities_for_all_profiles(self):
        for profile in ("smoke", "synthesis", "paper"):
            for row in matrix(profile):
                validate_config(row)
        for changes in (dict(backend="fftw"), dict(backend="missing"), dict(n=129),
                        dict(backend="spectre", n=128), dict(backend="driver", smooth=1),
                        dict(backend="fft-double", pass_name="callback")):
            with self.assertRaises(ValueError):
                validate_config(dict(BASE, **changes))

    def test_opaque_contract_does_not_infer_steps_from_name(self):
        registry = copy.deepcopy(REGISTRY)
        registry["rfft-misleading"] = dict(registry["fftw"], id="rfft-misleading", available=True)
        c = dict(BASE, backend="rfft-misleading", pass_name="complete", callbacks=2)
        contract = resolve_contract(c, registry)
        self.assertIsNone(contract["step_count"])
        with self.assertRaisesRegex(ValueError, "pass"):
            validate_config(dict(c, pass_name="steps"), registry)
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp)/"raw.csv"
            path.write_text("kind,ns\n"+"timer,20\n"*1024+"complete,100\n"*2)
            validate_rows(path, c, registry)
        accuracy = dict(max_abs_error=0, max_reference=1, roundtrip_max_abs_error=0)
        validate_transform_accuracy(accuracy, contract)
        for value in (float("nan"), -1, .1):
            with self.assertRaises(ValueError):
                validate_transform_accuracy(dict(accuracy, max_abs_error=value), contract)

    def campaign(self, directory, registry_name="docs/whitepaper/benchmarks/backends.json"):
        config = dict(BASE, backend="driver", n=128, hop=32, callbacks=2, warm_hops=0)
        raw = directory/"raw.csv"
        raw.write_text("kind,ns\n"+"timer,20\n"*1024+"callback,100\n"*2)
        for filename in ("stderr.txt", "build.log", "verification.txt", "linked-libraries.txt", "paper.bin", "paper-audit.bin"):
            (directory/filename).write_text("fixture\n")
        registry_path = Path(__file__).with_name("backends.json")
        with tarfile.open(directory/"source.tar.gz", "w:gz") as archive:
            archive.add(registry_path, arcname=registry_name)
        dependency = directory/"libRack.fixture"
        dependency.write_text("dependency bytes\n")
        with tarfile.open(directory/"dependencies.tar.gz", "w:gz") as archive:
            archive.add(dependency, arcname=dependency.name)
        save(directory/"inventory.json", REGISTRY)
        resources = {}
        for label, instrumented in (("timing", False), ("allocation", True)):
            phase = dict(ns=100, allocations=1 if instrumented else None,
                         allocated_bytes=64 if instrumented else None,
                         live_bytes=64 if instrumented else None, peak_bytes=64 if instrumented else None)
            resources[label] = dict(schema=1, instrumented=instrumented, object_bytes=64, operations=320,
                                    unknown_reason="native allocators not intercepted", native_allocation_bytes=None,
                                    stack_scratch_bytes=None, setup=phase, execution=phase, destruction=phase)
        save(directory/"resources.json", resources)
        metadata = dict(schema=2, status="complete", repeats=1, configs=[config],
                        contracts={"0": resolve_contract(config)}, resources={"0": "resources.json"},
                        source_sha256={registry_name: digest(registry_path)},
                        sdk_sha256={dependency.name: digest(dependency)},
                        binary_sha256=digest(directory/"paper.bin"), audit_binary_sha256=digest(directory/"paper-audit.bin"),
                        runs=[dict(workload=0, repeat=0, raw="raw.csv", stderr="stderr.txt", summary=summarize(raw, config))])
        metadata["artifact_sha256"] = {p.name: digest(p) for p in directory.iterdir() if p.is_file()}
        return metadata

    def test_pre_refactor_archive_remains_readable(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            metadata = self.campaign(directory, "benchmark/paper/backends.json")
            save(directory/"metadata.json", metadata)
            self.assertEqual(check(directory), 1)

    def test_campaign_failures(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            metadata = self.campaign(directory)
            save(directory/"metadata.json", metadata)
            self.assertEqual(check(directory), 1)
            mutations = [lambda m: m.update(repeats=2),
                         lambda m: m["runs"].append(m["runs"][0]),
                         lambda m: m["contracts"]["0"].update(layout="packed"),
                         lambda m: m["contracts"]["0"].update(normalization="wrong"),
                         lambda m: m["sdk_sha256"].update({"libRack.fixture": "0"*64}),
                         lambda m: m["artifact_sha256"].pop("raw.csv"),
                         lambda m: m["resources"].clear()]
            for mutate in mutations:
                altered = copy.deepcopy(metadata)
                mutate(altered)
                save(directory/"metadata.json", altered)
                with self.assertRaises(ValueError):
                    check(directory)
            save(directory/"metadata.json", metadata)
            (directory/"dependencies.tar.gz").write_bytes(b"changed library archive")
            with self.assertRaisesRegex(ValueError, "checksum"):
                check(directory)

    def test_resource_unknowns_and_instrumentation(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            self.campaign(directory)
            resources = json.loads((directory/"resources.json").read_text())
            validate_resources(resources)
            for label, key, value in (("timing", "instrumented", True),
                                      ("allocation", "native_allocation_bytes", 0),
                                      ("allocation", "object_bytes", 65)):
                altered = copy.deepcopy(resources)
                altered[label][key] = value
                with self.assertRaises(ValueError):
                    validate_resources(altered)


if __name__ == "__main__":
    unittest.main()

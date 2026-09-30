"""Validate explicit scalar audit coverage without upgrading historical records."""
# Copyright 2026 Arhythmetic Units
# SPDX-License-Identifier: GPL-3.0-or-later

POLICY = "all-publications-v1"


def validate_scalar_audit(accuracy, config, contract, publications):
    """Require every configured instance, expected publication and positive bin."""
    audit = accuracy.get("audit")
    if not isinstance(audit, dict) or audit.get("policy") != POLICY or audit.get("status") != "full":
        raise ValueError("Missing or unknown scalar all-publication audit")
    if (audit.get("reference_precision") != "binary64 FFT / long-double direct DFT"
            or audit.get("reference_mantissa_bits") != 53
            or type(audit.get("direct_mantissa_bits")) is not int
            or audit["direct_mantissa_bits"] < 53
            or audit.get("interval_arithmetic") != "binary32"):
        raise ValueError("Invalid scalar reference arithmetic identity")
    instances = audit.get("instances")
    if not isinstance(instances, list) or len(instances) != config["count"]:
        raise ValueError("Missing scalar audit instance")
    samples = config["callbacks"]*config["block"]
    hop, n = config["hop"], config["n"]
    delay = contract["publication_delay_samples"]
    warm = 0 if config["state"] == "startup" else ((n+hop-1)//hop+config["warm_hops"])*hop
    total = 0
    for index, item in enumerate(instances):
        offset = config.get("callback_offset", 0)
        if config["alignment"] == "staggered":
            offset += index*hop//config["count"]
        first_sample = warm+offset
        first_publication = first_sample+(delay-first_sample)%hop
        expected = max(0, (first_sample+samples-1-first_publication)//hop+1)
        expected_fields = {"instance": index, "first_sample": first_sample,
                           "samples": samples, "expected_spectra": expected,
                           "checked_spectra": expected, "checked_bins": expected*(n//2+1),
                           "first_endpoint": first_publication-delay if expected else 0,
                           "last_endpoint": first_publication-delay+(expected-1)*hop if expected else 0}
        if not isinstance(item, dict) or any(type(item.get(k)) is not int or item[k] != v
                                            for k, v in expected_fields.items()):
            raise ValueError("Scalar instance coverage or endpoint mismatch")
        total += expected
    totals = {"expected_spectra": total, "checked_spectra": total,
              "expected_bins": total*(n//2+1), "checked_bins": total*(n//2+1)}
    if any(type(audit.get(k)) is not int or audit[k] != v for k, v in totals.items()):
        raise ValueError("Scalar aggregate coverage mismatch")
    if (total != publications or accuracy.get("publications") != total
            or accuracy.get("checked_samples") != totals["checked_bins"]):
        raise ValueError("Scalar coverage does not match publication observations")
    analysis = accuracy.get("analysis", {})
    if analysis.get("vectors") != total or analysis.get("values") != totals["checked_bins"]:
        raise ValueError("Scalar coverage does not match numerical vectors")
    return audit


def coverage_tables(data, output):
    """Export checked per-process analysis coverage, preserving historical limits."""
    import csv
    import hashlib
    import json
    from pathlib import Path

    output = Path(output)
    columns = ["stratum", "config_sha256", "backend", "session", "repeat", "precision",
               "n", "hop", "instances", "channels_per_instance", "mode", "state",
               "status", "coverage_policy", "accuracy_policy", "reference", "reference_precision",
               "expected_spectra", "checked_spectra", "expected_bins", "checked_bins",
               "max_relative_l2", "max_relative_linf", "max_abs_error", "absolute_error_units",
               "zero_spectra", "legacy_pointwise_failures"]
    rows = []
    for record in data["records"]:
        config, contract = record["config"], record["contract"]
        if contract["boundary"] not in ("analysis", "module"):
            continue
        for process in record["processes"]:
            accuracy = process.get("accuracy") or {}
            analysis, audit = accuracy.get("analysis", {}), accuracy.get("audit", {})
            publications = process["publication_audit_rows"]
            expected = publications*contract["channels"]
            checked = analysis.get("vectors")
            status = "preflight only; no per-run all-output audit"
            coverage_policy = "preflight-only"
            if audit:
                validate_scalar_audit(accuracy, config, contract, publications)
                status, coverage_policy = "full", audit["policy"]
            elif accuracy.get("native_audit"):
                from native import validate_audit
                validate_audit(accuracy, config, contract)
                status, coverage_policy = "full; simultaneous channels per instance", "native-all-channels-v1"
            elif accuracy.get("module_policy"):
                from modules import validate
                validate(accuracy, config, publications, contract)
                status, coverage_policy = "full; independent conditioning and display mapping", "all-module-outputs-v1"
            elif analysis:
                # These counts were validated by check.py on archive ingestion.
                # Never confer the new scalar policy on an older native archive.
                status, coverage_policy = "existing all-output numerical replay", "archived-vector-counts"
            rows.append(dict(stratum=record.get("stratum", ""),
                config_sha256=hashlib.sha256(json.dumps(config, sort_keys=True).encode()).hexdigest(),
                backend=config["backend"], session=process["session"], repeat=process["repeat"],
                precision=contract["precision"], n=config["n"], hop=config["hop"],
                instances=config["count"], channels_per_instance=contract["channels"],
                mode=config["pass_name"], state=config["state"], status=status,
                coverage_policy=coverage_policy, accuracy_policy=analysis.get("policy", "not recorded"),
                reference=accuracy.get("reference", "preflight only"),
                reference_precision=audit.get("reference_precision", "see archived reference; precision not separately recorded"),
                expected_spectra=expected, checked_spectra=checked,
                expected_bins=expected*contract["outputs_per_channel"], checked_bins=analysis.get("values"),
                max_relative_l2=analysis.get("max_relative_l2"), max_relative_linf=analysis.get("max_relative_linf"),
                max_abs_error=accuracy.get("max_abs_error"), absolute_error_units="unnormalized FFT magnitude",
                zero_spectra=analysis.get("zero_vectors"), legacy_pointwise_failures=analysis.get("legacy_pointwise_failures")))
    with (output/"accuracy-coverage.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows)
    text = ["# Numerical Accuracy And Coverage", "",
            "Per-process evidence; relative errors are dimensionless. Absolute errors",
            "use unnormalized FFT magnitude units. Empty fields are unavailable, not zero.",
            "Historical preflight-only records retain that limitation. Pointwise failures",
            "are diagnostics under the stated spectrum-norm policy, not hidden exclusions.", "",
            "| Backend | Session / Repeat | Status / Policy | Spectra Checked / Expected | Bins Checked / Expected | Relative L2 / Linf | Maximum Absolute Error |",
            "| --- | --- | --- | --- | --- | --- | --- |"]
    def show(value):
        return "unavailable" if value is None else str(value)
    for row in rows:
        text.append(f"| {row['backend']} | {row['session']} / {row['repeat']} | "
                    f"{row['status']} / {row['coverage_policy']} / {row['accuracy_policy']} | "
                    f"{show(row['checked_spectra'])} / {row['expected_spectra']} | "
                    f"{show(row['checked_bins'])} / {row['expected_bins']} | "
                    f"{show(row['max_relative_l2'])} / {show(row['max_relative_linf'])} | "
                    f"{show(row['max_abs_error'])} |")
    (output/"accuracy-coverage.md").write_text("\n".join(text)+"\n")
    return rows

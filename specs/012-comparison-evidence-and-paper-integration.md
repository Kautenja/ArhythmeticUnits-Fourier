# Comparison Evidence And Paper Integration

Status: PLANNED

Created: September 29, 2026

Replacement measurement status: NOT RUN.
Evidence retirement status: DEFERRED.

## Goal And Ownership

Collect validated replacement comparison evidence and integrate it into the
whitepaper using the completed benchmark workflow. This spec takes ownership
of former FR-14, final-measurement acceptance criteria and actual retirement
from [archived spec 004](archive/004-external-fft-comparison.md#closure-and-follow-up).
It preserves those obligations while closing the implementation project.
The transfer itself launches no measurements and deletes no evidence.

The tested [workflow handoff](../docs/whitepaper/benchmarks/guides/workflow.md)
provides the commands. The archived spec retains the
[measurement sequence](archive/004-external-fft-comparison.md#final-measurement-sequence),
[comparison contracts](archive/004-external-fft-comparison.md#shared-contracts),
[contender decisions](archive/004-external-fft-comparison.md#optional-contender-decision)
and [retirement gate](archive/004-external-fft-comparison.md#replacement-and-retirement-gate).
Those scientific and preservation constraints remain applicable here.

## Expected Behavior

-   A new pilot captures current source/dependency/compiler identities,
    numerical coverage, transition behavior and actual host conditions.
    It justifies the final matrix and observation lengths before freezing.
-   A smoke report can validate mechanics but cannot populate production
    manuscript results. Historical confirmation runs remain historical.
-   A changed source revision needs a new pilot/freeze. Three process runs
    in one machine session do not become three independent sessions by
    changing their labels.
-   An unfavorable result is retained and discussed. A fast transform does
    not establish a better callback tail or newer spectrum.
-   Old evidence with retained manuscript, test or baseline dependencies
    stays in place even after new comparison measurements pass.

## Requirements And Acceptance Criteria

- [ ] Record the measured revision, supported host/provider variants,
    exact source/dependency/compiler identities, host preparation and
    resolved workload inventories. Validate the implementation readiness
    gate before starting; restart affected evidence after any corrective
    implementation change. Keep benchmarks separate from the Rack modules.
- [ ] Run focused primary and extension pilots following the archived
    measurement sequence. Cover forward transforms, analysis, inverse jobs,
    complete identity/FIR chains, matched scheduling controls, independent
    channels, supported double precision and parameter transitions. Keep
    their different boundaries and time origins separate. Include startup,
    smoothing, phase and unchanged transition controls where declared.
- [ ] Review timer resolution, process/session variability, tail counts,
    numerical errors, resource limits and runtime/disk requirements. Record
    explicit rationale for durations, repetitions and every matrix reduction;
    do not select only favorable workloads. Freeze each study from checked
    pilot evidence with its exact configuration and policy identities.
- [ ] Collect at least three genuinely independent confirmation sessions
    per frozen study, increasing coverage when pilot variation warrants it.
    Measure serially on a prepared, otherwise idle host. Separate different
    revisions, hosts, compilers and experimental variants. Acquire ARM64
    and x86-64 evidence before cross-architecture claims; otherwise limit
    the paper's scope to measured platforms.
- [ ] Verify raw artifacts, frozen matrix completeness, all-output scalar
    numerical coverage, inverse/chain references, transition traces and
    source/dependency provenance. Generate cost, tail, storage, accuracy,
    age, attribution and transition views with process/session uncertainty,
    observation counts and duration. Preserve regressions and missing cases.
    Mark replacement measurements VALIDATED only when these gates pass.
- [ ] Review explicit selection manifests and export numeric macros,
    tables and figures. Integrate only checked confirmation outputs into
    the manuscript and pin its measured revision. Preserve independently
    cited historical experiments; remove obsolete preliminary claims only
    after resolving their evidence dependencies. Do not transcribe numbers
    manually or rewrite old data to match current source.
- [ ] Discuss benefits, regressions, crossover regimes, numerical accuracy,
    confounds, uncertainty and measured platform limits. State that KISS FFT,
    feedforward STFT and sliding/hopping contenders were not measured under
    the recorded deferrals. Do not present the measured subset as a global
    optimum. Update citation metadata consistently with actual artifacts;
    no invented DOI, publication date or indexing claim.
- [ ] Package the selected evidence and verify extraction and independent
    numerical rederivation using the bundled tooling. Preserve raw data,
    measured source/configuration and exact dependency identities. Record
    omitted native/SDK bytes and distinguish derivation from timing replication.
- [ ] Refresh exact retirement candidates and resolve retained uses under
    the archived gate. Preview first; execute only eligible removals backed
    by an independent validated replacement bundle and publication receipt.
    Retain required historical experiments, optimization baselines, source
    fixtures and unrelated dependencies. Record removed and retained paths,
    hashes, replacement identities and reasons here. Unresolved dependencies
    require retention, not forced deletion.
- [ ] Pass artifact checks, paper build, portable source compilation, local
    link checks and visual PDF review. Record actual hosts, commands, results,
    limitations and supersession evidence here. Mark this spec COMPLETE and
    archive it only after the required evidence and manuscript gates pass.

## Launch And Validation

From the repository root, after preparing the SDK, provider dependencies and
plotting environment documented in the workflow guide:

```shell
python3 docs/whitepaper/benchmarks/bench.py setup --variant macos
python3 docs/whitepaper/benchmarks/bench.py plan --profile pilot --variant macos --output .build/pilot-plan.json
python3 docs/whitepaper/benchmarks/bench.py run --profile pilot --variant macos --output .build/pilot-primary --host m1-pro --session pilot-01 --notes 'Actual power, thermal, affinity and host activity notes'
python3 docs/whitepaper/benchmarks/bench.py check .build/pilot-primary
python3 docs/whitepaper/benchmarks/bench.py estimate .build/pilot-primary
```

Use truthful host/session notes and new output names. Use `rack` or `portable`
when those are the chosen provider variants. The guide supplies extension,
freeze and separately prepared session commands; choose final counts after
reviewing pilots, not from an unvalidated example. Its confirmation paths
lead to these explicit validation and publication steps:

```shell
python3 docs/whitepaper/benchmarks/bench.py check .build/confirm-01 .build/confirm-02 .build/confirm-03
python3 docs/whitepaper/benchmarks/bench.py report .build/confirm-01 .build/confirm-02 .build/confirm-03 --phase confirmation --output .build/confirmation-report
python3 docs/whitepaper/benchmarks/bench.py select .build/confirmation-report --output .build/paper-selection.json
python3 docs/whitepaper/benchmarks/bench.py export .build/paper-selection.json --output docs/whitepaper/generated/comparison
python3 docs/whitepaper/benchmarks/bench.py check-export docs/whitepaper/generated/comparison
make -C docs/whitepaper check
make -C docs/whitepaper
make -C docs/whitepaper arxiv
```

Review the selection before export and insert the chosen includes during
editorial integration. Follow the workflow's bundle, extraction, regeneration
and retirement commands. No command here implies that these runs have occurred.

## Non-Goals And Current Evidence

This follow-up does not implement new FFT frameworks, change production DSP,
integrate benchmark adapters into VCV Rack, or reopen deferred contenders
without a separate research rationale. GPU, worker-thread and general STFT
reconstruction studies remain outside scope. It does not publish to arXiv,
push a release or promise Google Scholar indexing.

Spec 004's 78 tests, 80-workload smoke, fixture PDF and portable rederivation
verify tooling only. Its 16 inventoried historical pilot/confirmation
directories remain retained. No new replacement pilot, confirmation,
manuscript result integration or real evidence retirement has been performed
for this spec. Completion evidence belongs here as those steps are executed.

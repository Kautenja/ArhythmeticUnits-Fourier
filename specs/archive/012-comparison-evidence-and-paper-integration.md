# Comparison Evidence And Analysis Handoff

Status: COMPLETE

Created: September 29, 2026

Completed and archived: September 30, 2026

Replacement measurement status: VALIDATED.
Evidence retirement status: DEFERRED; 16 retained dependencies, no removals.

## Goal And Ownership

Collect validated replacement comparison evidence and prepare checked results
for the next analysis/writing agent using the completed benchmark workflow.
The user narrowed this scope on September 29, 2026: complete and archive the
results handoff, without finishing the paper. Editorial selection, scientific
interpretation, manuscript insertion and citation/publication updates remain
for that later agent. Generated results and traceable exports are in scope.
This spec takes ownership of former FR-14, final-measurement acceptance
criteria and actual retirement
from [archived spec 004](004-external-fft-comparison.md#closure-and-follow-up).
Measurement and preservation obligations remain applicable. The former
manuscript-writing obligations are deferred explicitly to the next agent;
their absence does not block this results-only completion.

The tested [workflow handoff](../../docs/whitepaper/benchmarks/guides/workflow.md)
provides the commands. The archived spec retains the
[measurement sequence](004-external-fft-comparison.md#final-measurement-sequence),
[comparison contracts](004-external-fft-comparison.md#shared-contracts),
[contender decisions](004-external-fft-comparison.md#optional-contender-decision)
and [retirement gate](004-external-fft-comparison.md#replacement-and-retirement-gate).
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

- [x] Record the measured revision, supported host/provider variants,
    exact source/dependency/compiler identities, host preparation and
    resolved workload inventories. Validate the implementation readiness
    gate before starting; restart affected evidence after any corrective
    implementation change. Keep benchmarks separate from the Rack modules.
- [x] Run focused primary and extension pilots following the archived
    measurement sequence. Cover forward transforms, analysis, inverse jobs,
    complete identity/FIR chains, matched scheduling controls, independent
    channels, supported double precision and parameter transitions. Keep
    their different boundaries and time origins separate. Include startup,
    smoothing, phase and unchanged transition controls where declared.
- [x] Review timer resolution, process/session variability, tail counts,
    numerical errors, resource limits and runtime/disk requirements. Record
    explicit rationale for durations, repetitions and every matrix reduction;
    do not select only favorable workloads. Freeze each study from checked
    pilot evidence with its exact configuration and policy identities.
- [x] Collect at least three independently prepared confirmation sessions
    per frozen study, increasing coverage when pilot variation warrants it.
    Measure serially on a prepared, otherwise idle host. Separate different
    revisions, hosts, compilers and experimental variants. Acquire ARM64
    and x86-64 evidence before cross-architecture claims; otherwise limit
    the paper's scope to measured platforms.
- [x] Verify raw artifacts, frozen matrix completeness, all-output scalar
    numerical coverage, inverse/chain references, transition traces and
    source/dependency provenance. Generate cost, tail, storage, accuracy,
    age, attribution and transition views with process/session uncertainty,
    observation counts and duration. Preserve regressions and missing cases.
    Mark replacement measurements VALIDATED only when these gates pass.
- [x] Prepare traceable selection manifests, checked tables/figures and numeric
    exports for the next agent. Pin the measured revision and retain complete
    reports, raw data and unfavorable results. Do not insert results into
    manuscript prose or rewrite historical data. State unmeasured contenders,
    platform limits, precision/coverage differences and interpretation limits
    in the handoff; scientific analysis and editorial selection are deferred.
- [x] Package the selected evidence and verify extraction and independent
    numerical rederivation using the bundled tooling. Preserve raw data,
    measured source/configuration and exact dependency identities. Record
    omitted native/SDK bytes and distinguish derivation from timing replication.
- [x] Refresh exact retirement candidates and resolve retained uses under
    the archived gate. Preview first; execute only eligible removals backed
    by an independent validated replacement bundle and publication receipt.
    Retain required historical experiments, optimization baselines, source
    fixtures and unrelated dependencies. Record removed and retained paths,
    hashes, replacement identities and reasons here. Unresolved dependencies
    require retention, not forced deletion.
- [x] Pass artifact, export, portable rederivation and local link checks.
    Visually inspect representative scientific plots. Record hosts, commands,
    results, limitations and evidence locations here and provide a durable
    handoff in `docs/whitepaper/data/`. Archive this spec when the results are
    verified and accessible. The manuscript build, final PDF review, writing
    and citation updates belong to the next agent and are not claimed done.

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

Review the selection before export. Leave include insertion and manuscript
compilation to the next agent; the commands above also document that later
step. Follow the workflow's bundle, extraction, regeneration
and retirement commands. No command here implies that these runs have occurred.

## Non-Goals And Current Evidence

This follow-up does not finish the paper, implement new FFT frameworks,
change production DSP,
integrate benchmark adapters into VCV Rack, or reopen deferred contenders
without a separate research rationale. GPU, worker-thread and general STFT
reconstruction studies remain outside scope. It does not publish to arXiv,
push a release or promise Google Scholar indexing.

Spec 004's 78 tests, 80-workload smoke, fixture PDF and portable rederivation
verify tooling only. Its 16 inventoried historical pilot/confirmation
directories remain retained. Campaign execution is authorized. Completion
evidence is recorded below as pilots, confirmations, reports and portable
bundles pass. No manuscript
result integration or evidence deletion is implied by the scope change.

## Execution Evidence

September 30, 2026: the initial primary pilot stopped before measurements
because Finder modified `.DS_Store` bytes inside the source inventory. That
invalid attempt is retained at `.build/spec012/pilot-primary`. The runner now
excludes Finder metadata (and still hashes working/untracked research sources),
with a regression proving source changes remain detectable. All 79 tests and
paper artifact checks passed before restarting. No DSP or timing-loop code
changed. The measured baseline is `12ac323` plus this retained runner/test fix;
source archives and hashes, rather than HEAD alone, identify every campaign.

The replacement primary pilot (`.build/spec012/pilot-primary-02`) passed 1272
process runs across 424 workloads; the extension pilot
(`.build/spec012/pilot-extensions`) passed 2673 runs across 891 workloads.
Within-session max/min process-mean ratios had median/p90/max
1.01138/1.04948/7.75455 (primary) and 1.02050/1.11771/5.03489 (extensions).
Timer p99 was 42 ns in both. Outliers remain. These diagnostics justify longer
descriptive windows; they do not establish convergence or an algorithm ranking.

The freeze plan at `.build/spec012/freeze-plan.json` partitions all 1299
unique nominal workloads, removing only 16 exact transition duplicates between
the two pilot profiles. No contender or unfavorable configuration is omitted.
Each group has three process repeats in each of three separately prepared
measurement blocks, totaling 11691 confirmation processes:

| Group | Workloads | Measured Hops | Callback Observations Per Process |
| --- | ---: | ---: | ---: |
| Primary | 408 | 2048 | 8192 / 32768 / 131072 |
| Standard extensions | 719 | 512 | 2056 / 8192 |
| Short-hop extensions | 110 | 4096 | 2368 |
| Single-sample extensions | 46 | 64 | 65536 |
| Transitions | 16 | 64 | 37 / 1024 |

Stationary cases use 64 warmup hops; transitions retain zero warmup and their
declared event schedule. Isolated transforms use 1024 frames. Freeze files
contain exact resolved configurations, seeds, policy/provenance, pilot hashes
and rationale. Seeds remain fixed within each frozen group; group order rotates
across sessions. At least 180 seconds without builds, tests, reporting or
benchmarks separates session preparation from preceding work. Pre/post power,
thermal-warning, load and process snapshots are retained under
`.build/spec012/host/`. Sessions are temporally separated blocks on one laptop;
they do not constitute independent machine/day replication or prove statistical
independence. Report observed session ranges, not inferential confidence bounds.

The machine is an Apple M1 Pro with 16 GiB RAM, macOS 26.6.2 ARM64, on AC power
with Low Power Mode off. No CPU affinity, real-time priority, temperature or
frequency control is imposed. Ordinary system services and Codex remain possible
confounds. Thermal-warning snapshots are not continuous temperature traces.
Cross-architecture claims, precise rare-event tails, WCET and device-underrun
claims are excluded. Single-sample p99 does not characterize sparse FFT bursts.

Session 1 completed all 3897 frozen confirmation runs and final checks.
Its recorded quiet interval was 207.088 seconds; pre/post snapshots retained
AC power at 100%, Low Power Mode off and no recorded thermal/performance
warning. The post-session snapshot is dated 2026-09-30T04:56:49Z.
The five campaign directories are `.build/spec012/confirm-01-GROUP`, with
GROUP equal to primary, extensions, short-hop, single-sample or transitions.

Session 2 completed all 3897 frozen runs and final checks in the rotated
order short-hop, transitions, primary, extensions, single-sample. Its quiet
interval was 213.085 seconds. The pre/post snapshots again record AC power
at 100%, Low Power Mode off and no recorded thermal/performance warning.
The post-session snapshot is dated 2026-09-30T05:54:11Z; campaign directories
are `.build/spec012/confirm-02-GROUP`.

Session 3 completed all 3897 frozen runs and final checks in the order
extensions, single-sample, transitions, short-hop, primary. Its quiet interval
was 210.130 seconds. The pre/post snapshots retain AC power at 100%, Low Power
Mode off and no recorded thermal/performance warning. The final post-session
snapshot is dated 2026-09-30T06:51:11Z. All 11691 confirmation processes and
3945 pilot processes passed. All 17 successful campaign source-tree identities
match; their compressed original metadata and measured first-party source
archive are preserved in `docs/whitepaper/data/comparison-012/`.

The refreshed `.build/spec012/retirement-plan.json` inventories the same 16
historical campaign directories. Every candidate still has retained spec or
report references. `docs/whitepaper/data/comparison-012/retention.json` records
those exact paths, metadata/tree hashes and reasons. No historical campaign,
optimization baseline, dependency or manuscript data was removed. Manuscript
integration and resolution of its historical dependencies remain later work.

All five checked reports completed: 408 primary, 719 standard-extension,
110 short-hop, 46 single-sample and 16 transition workloads. Their 672 PNG/SVG
figure pairs and eight CSV table types per group retain all configurations,
process/session variation, numerical coverage, attribution and transition
outcomes. The compact tracked tables are exact copies verified against the
full report manifests. The source reports remain under `.build/spec012/reports/`.

Ten representative plots across all five groups were visually inspected,
including analysis, inverse jobs, complete chains, tails, storage/accuracy and
transitions. They are diagnostic views, not finished manuscript figures.
`verification/visual-review.json` in the handoff records their hashes and the
remaining editorial issues: crowded labels/annotations, additive offsets on
some narrow numerical axes and single-length panels that cannot show scaling.
These presentation refinements and final PDF review remain with the writing
agent; they do not alter the recorded numbers.

The public export command already performs a fresh receipt verification.
The smaller groups additionally passed standalone `check-export`. After the
primary export passed its internal fresh verification, a redundant standalone
check was deliberately interrupted (exit 130); no measurement, source artifact
or completed export was changed. The extension export uses its internal fresh
verification without adding that duplicate pass. Portable bundle rederivation
is a separate required check, not replaced by export success.


## Results Handoff And Closure

September 30, 2026: all results-only requirements are complete. The
[durable handoff](../../docs/latex/deprecated/whitepaper/data/comparison-012/README.md) contains
40 exact CSV tables, all 672 diagnostic PNGs through the five generated export
sets, source/design/host records, original compressed campaign metadata,
selection/receipt identities and preservation/verification records. The
[coverage inventory](../../docs/latex/deprecated/whitepaper/data/comparison-012/coverage.json)
includes 217 periodic inverse-job workloads, 24 isolated IFFT workloads and
434 complete identity/FIR-chain workloads, in addition to the forward/analysis
and transition pathways. No production DSP or Rack module was modified.

Six verified portable bundles are preserved locally outside `.build` in
`docs/whitepaper/data/comparison-012/bundles/`: five confirmation groups and the
combined successful pilots. `bundles.json` records their exact sizes and
SHA-256 hashes. These large archives are ignored by Git; the compact tables,
figures, metadata, measured source archive, design and bundle identities are
tracked. A fresh clone alone does not contain the raw observations. The next
agent must retain/copy or deposit the bundles before removing this workspace
or claiming public raw-data availability. Original campaign and full report
directories also remain under `.build/spec012/`. Ordinary `make clean` cannot
remove the additional bundle copies. Nothing was uploaded externally.

Each confirmation bundle was extracted into a new location, checked and used
to regenerate tables with its own bundled CLI. All eight CSV types matched
exactly after normalizing only raw paths and provenance directories whose
metadata hashes were checked. All values, configuration/source identities,
counts and other provenance fields remained strict comparisons. Both pilot
reviews reproduced exactly from their relocated, validated evidence. Native
executables, SDK bytes and dependency archives are omitted explicitly by the
bundle policy; recorded hashes do not imply those omitted bytes were inspected.
This validates statistical derivation, not a new timing run on another host.
The comparison helper needed a 16 MiB CSV field limit for long embedded provider
plans; retrying that comparison did not change evidence or numerical assertions.

The command families actually executed, from the repository root, were:

```shell
MPLCONFIGDIR="$PWD/.build/matplotlib" .build/paper-report-env/bin/python -m unittest discover -s docs/whitepaper/benchmarks/tests -p 'test_*.py'
python3 docs/whitepaper/benchmarks/bench.py run --profile pilot --variant macos --output .build/spec012/pilot-primary-02 --host m1-pro-16gb-local --session spec012-pilot-primary
python3 docs/whitepaper/benchmarks/bench.py run --profile extensions --variant macos --output .build/spec012/pilot-extensions --host m1-pro-16gb-local --session spec012-pilot-extensions
make -C docs/whitepaper check
git diff --cached --check
```

The complete actual launch options and host notes are in the retained original
metadata; do not reuse these immutable destinations. Five subset freezes used
the existing `study.freeze` API against checked pilots; their full manifests
preserve the exact configs, options, inclusion rationale and pilot hashes.
Every confirmation used the public `run --freeze ... --variant macos` command
with its actual host/session notes. Group orders and exact preparation intervals
are recorded above and in the handoff's `design/` and `host/` records. The
following is the primary derivation/extraction command pattern also applied
to the other four groups:

```shell
MPLCONFIGDIR="$PWD/.build/matplotlib" .build/paper-report-env/bin/python docs/whitepaper/benchmarks/bench.py report .build/spec012/confirm-01-primary .build/spec012/confirm-02-primary .build/spec012/confirm-03-primary --phase confirmation --output .build/spec012/reports/primary
python3 docs/whitepaper/benchmarks/bench.py select .build/spec012/reports/primary --output .build/spec012/selections/primary.json
python3 docs/whitepaper/benchmarks/bench.py export .build/spec012/selections/primary.json --output docs/whitepaper/generated/comparison-012-primary
python3 docs/whitepaper/benchmarks/bench.py bundle .build/spec012/confirm-01-primary .build/spec012/confirm-02-primary .build/spec012/confirm-03-primary --selection .build/spec012/selections/primary.json --output .build/spec012/bundles/primary.tar.gz
python3 docs/whitepaper/benchmarks/bench.py unpack .build/spec012/bundles/primary.tar.gz --output .build/spec012/relocated/primary
```

Inside each extracted directory, the bundled command was
`python3 tooling/docs/whitepaper/benchmarks/bench.py report campaigns/* --phase confirmation --output regenerated --no-plots`.
Successful relocation scratch copies were then removed; original campaigns,
reports and independent bundles remain. The handoff records exact bundle and
CSV identities. Git attributes preserve checksummed file bytes, including CSV
CRLF, across clients; a staged CSV was checked byte-for-byte against its report.

Validation passed: all 79 workflow tests before pilots; forced native benchmark
build/preflight and complete campaign checks; all five reports and fresh export
receipts; six bundle extraction checks; exact rederivation of all confirmation
tables and pilot diagnostics; historical paper artifact checks; handoff hashes,
archived source bytes, local links/anchors and staged diff checks. Paper checks
retained 31 references and 32768 balanced schedules. No full Rack plugin build,
manual Rack session, manuscript compilation/PDF review, arXiv upload or citation
update is claimed. Those absent publication tasks are explicitly outside this
results-only scope.

Analysis, editorial selection, manuscript claims/insertion and publication
work belong to the next agent. The same-host/same-night session limits,
unmeasured contenders, missing architectures, diagnostic plotting refinements
and retained historical dependencies remain explicit in the handoff. No claim
of inferential independence, universal superiority, WCET or device underrun
performance follows from completing this spec.

Closing integrity verification checked 809 compact handoff/export files,
all six independently preserved bundle hashes, original metadata and archived
source bytes, and 157 local links/anchors. No unresolved validation failure
remains within the results-only scope.

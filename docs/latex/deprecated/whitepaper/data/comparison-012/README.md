# Comparison Evidence For Analysis And Writing

This directory is the results-only handoff from
[completed spec 012](../../../../specs/archive/012-comparison-evidence-and-paper-integration.md). The September 30,
2026 campaign measures Fourier's benchmark adapters and their first-party
controls on one Apple M1 Pro laptop. It does not insert new claims or figures
into the manuscript. All 11691 confirmation processes and 3945 pilot processes passed.
All five confirmation groups passed portable table rederivation; both pilot
reviews also reproduced exactly from their checked bundle.

The measured revision is `12ac3236e332b7d0a01b2d750fa07a0849132341` plus the
runner/test fix excluding Finder metadata from source inventories. The exact
archived source hashes identify the experiment; HEAD alone does not. That fix
followed a rejected pilot with zero measurements and passed all 79 workflow
tests before the successful pilots. No production DSP or Rack module changed.

## Files And Access

| Location | Contents |
| --- | --- |
| [bundles.json](bundles.json) | Verified local archive locations, sizes and SHA-256 hashes |
| [index.json](index.json) | Original campaign paths, measured revision, hashes, sessions and the excluded zero-measurement attempt |
| [coverage.json](coverage.json) | Workload, precision, process, numerical-coverage and figure inventories |
| [design/](design/) | Resolved pilot plans, pilot review, five complete freezes and the group-order plan |
| [host/](host/) | Original preparation, quiet-interval and pre/post host snapshots |
| [campaigns/](campaigns/) | Gzip copies of original campaign metadata; decompress to inspect exact commands, plans, numerical policies and artifact hashes |
| [measured-source.tar.gz](measured-source.tar.gz) | Exact first-party source archive from the first primary confirmation |
| [tables/primary/](tables/primary/) | Primary cost, process, coverage, attribution and implementation tables |
| [tables/extensions/](tables/extensions/) | Channel, precision, phase, load, rate, state and cache extensions |
| [tables/short-hop/](tables/short-hop/) | Separate short-hop observation windows |
| [tables/single-sample/](tables/single-sample/) | Single-sample callbacks, with sparse-burst interpretation limits |
| [tables/transitions/](tables/transitions/) | Request/application/publication outcomes, callback windows and every audited publication |
| [verification/](verification/) | Portable rederivation checks and explicit dependency omissions |
| [retention.json](retention.json) | The 16 historical campaigns retained with unresolved spec/report dependencies |

Each table directory contains all eight CSV tables and an exact copy of the
source report manifest. Generated Markdown reports, large `evidence.json` files
and SVG figures remain in the full local reports and portable bundles. The
source manifest identifies that complete report, not only this compact copy. The committed tables retain the report's
original raw-path strings and hashes. Relocated bundles map campaign locations
by metadata hash; filesystem location is not a numerical identity.
`implementations.csv` contains large embedded provider-plan fields. With
Python's standard CSV reader, set `csv.field_size_limit(16 * 1024 * 1024)`
before reading it.

The five independent exports are under
[`../../generated/`](../../generated/), in `comparison-012-primary`,
`comparison-012-extensions`, `comparison-012-short-hop`,
`comparison-012-single-sample` and `comparison-012-transitions`. They retain all
stationary mean-cost rows, all diagnostic PNGs, selection manifests and checked
receipts. Each is a separate include set with its own default macro names;
choose unique macro names in a new selection before combining groups in the
manuscript. No generated include was inserted into the paper, and the optional
preview documents were not compiled as finished manuscript pages.

The verified portable audit bundles are **local, untracked** in
[bundles/](bundles/README.md), outside `.build` so ordinary `make clean` does not
remove them. The handoff manifest records their sizes and SHA-256 identities.
Original campaigns and full JSON/SVG reports also remain in `.build/spec012/`.
This keeps several gigabytes of raw observations and derivations out of Git.
A fresh clone has the compact tables, plots, source, metadata and design; it
needs the local bundles to rederive statistics. Copy or deposit them before
removing this workspace or advertising publicly available raw data. No external
archive was uploaded. After cleaning `.build`, regenerate reports/selections
from the bundles before checking fresh exports.

The [handoff manifest](manifest.json) verifies the committed compact files and
all five export sets. From the repository root:

```shell
python3 - <<'CHECK'
import hashlib, json
from pathlib import Path
study = Path("docs/whitepaper/data/comparison-012")
manifest = json.loads((study / "manifest.json").read_text())
for name, expected in manifest["files"].items():
    assert hashlib.sha256((study / name).read_bytes()).hexdigest() == expected, name
print("Verified", len(manifest["files"]), "handoff/export files")
CHECK
```

From the repository root, check an export against the retained raw evidence:

```shell
python3 docs/whitepaper/benchmarks/bench.py check-export docs/whitepaper/generated/comparison-012-primary
```

For portable rederivation, use a new extraction directory and then the bundled
CLI. The example reproduces primary tables; substitute another bundle name
for the other four frozen groups:

```shell
python3 docs/whitepaper/benchmarks/bench.py unpack docs/whitepaper/data/comparison-012/bundles/primary.tar.gz --output .build/spec012-reproduce-primary
cd .build/spec012-reproduce-primary
python3 tooling/docs/whitepaper/benchmarks/bench.py report campaigns/* --phase confirmation --output rederived --no-plots
```

The bundle includes raw observations, original first-party source/configuration,
policies, logs, report/selection and exact dependency identities. Native
executables, Rack SDK bytes and dependency archives are explicitly omitted.
Their original hashes remain recorded; bundle checks do not inspect omitted
bytes. Regenerating statistics does not rerun timings or establish access to
the original SDK. Repeating measurements requires the archived source and the
recorded dependencies on a separately prepared host.

## Study Design

The primary and extension pilots cover forward transforms, streaming analysis,
periodic inverse jobs, complete overlap-save identity/FIR chains, scheduling
controls, independent channels, supported double precision and parameter
transitions. Five freezes partition all 1299 distinct nominal workloads. Only
16 exact transition duplicates shared by the pilot profiles were removed.
No contender or unfavorable configuration was dropped based on its timings.

Each workload has three fresh process repeats in each of three separately
prepared measurement blocks. These are temporal blocks on the same laptop
and night, not independent hardware/day replication or proof of statistical
independence. Group order rotates; each frozen group's seed remains fixed.
At least 180 seconds without builds, tests, reporting or benchmarks precedes
each block. All measurements run serially; reporting follows the final block.

| Group | Workloads | Hops | Callback Observations Per Process |
| --- | ---: | ---: | ---: |
| Primary | 408 | 2048 | 8192 / 32768 / 131072 |
| Standard extensions | 719 | 512 | 2056 / 8192 |
| Short-hop extensions | 110 | 4096 | 2368 |
| Single-sample extensions | 46 | 64 | 65536 |
| Transitions | 16 | 64 | 37 / 1024 |

Stationary workloads use 64 warmup hops; transitions start without warmup
and retain their declared event schedules. Isolated transforms use 1024
frames. Exact configurations, order, numerical policy and source/dependency
identities belong to the frozen manifests and campaign metadata.

## Interpretation Boundaries

-   Compare matching operation, precision, channel count, state, smoothing,
    hop, callback origin, load and cache policy. Keep transform execution,
    streaming analysis, periodic inverse jobs and complete chains separate.
-   Inverse jobs release an independently specified complex spectrum every
    H samples and store all N normalized complex outputs. Their completion
    delay is distinct from an analyzer's spectrum-center age and a chain's
    input-to-delivery delay. N outputs every H calls do not imply N=H audio.
-   The chain includes retention, buffering, the complex forward transform,
    N spectral multiplications, normalized inverse, overlap-save extraction
    and continuous delivery. Identity and the three-tap FIR are application
    and correctness fixtures; they do not establish that FFT convolution is
    preferable to direct evaluation of that short filter. This is not an
    STFT overlap-add reconstruction experiment.
-   Match scalar and four-channel work deliberately. SIMD implementation
    checks and scalar all-output numerical audits have different coverage;
    a missing scalar norm is not zero error. Unsupported precision/backend
    combinations remain absent, not silently substituted.
-   Use the per-process and per-session summaries and observed ranges.
    Do not pool callback observations as independent experimental repeats.
    No convergence, inferential confidence interval or precise rare-event
    tail is claimed. With single-sample callbacks, p99 can miss sparse FFT
    bursts despite the large callback count; inspect maxima and frame counts.
-   Keep timer controls and outliers. No timer subtraction or favorable-case
    trimming was performed. Sub-tick observations are not precise timings
    of individual operations. Simulated budget exceedances are neither audio
    device underruns nor worst-case execution-time bounds.
-   The host used AC power with Low Power Mode off. Ordinary macOS services
    and Codex remained possible confounds. No affinity, real-time priority,
    frequency isolation or continuous temperature trace was available.
    Pre/post warning snapshots cannot establish constant temperature.
-   C++ live-heap probes do not include every provider-native allocation or
    stack workspace. Keep those omissions visible in storage comparisons.
    FFTW uses fresh per-process single-threaded MEASURE plans without imported
    wisdom; retained plan identities are part of the measured experiment.
-   Results apply to the recorded macOS ARM64 platform, compiler and plans.
    No x86-64 campaign was run. vDSP is a platform-library baseline. KISS FFT,
    feedforward STFT, sliding/hopping DFT, GPU and worker-thread solutions
    were not measured; their absence cannot establish superiority over them.

## Next Agent's Work

1.  Read the frozen configurations and checked tables before choosing claims.
    Separate batch-library competitiveness from the matched batch, immediate
    hybrid, balanced hybrid and resumable scheduling comparisons. Preserve
    cases where batch processing wins.
2.  Examine process/session ranges, timer resolution, observation windows,
    numerical coverage and transition response/cost/error together. Reopen
    measurement design if the intended claim needs longer tails, other
    hosts/days, or excluded overlap-reuse methods.
3.  Select a small, justified set of figures and table rows from the complete
    evidence. The supplied figures are diagnostic views; shorten crowded axis
    labels and refine layout for the final manuscript. Some accuracy axes use
    additive offsets and some length panels contain only one N; consult
    [the visual review](verification/visual-review.json) before interpreting
    or publishing them. Trace every reported value through its configuration, stratum,
    units, source identity and publication receipt. Do not hand-transcribe
    a favorable subset or mix historical and replacement measurements.
4.  Write the interpretation, revise limitations and closest-work discussion,
    insert the chosen generated includes, and perform the manuscript/PDF and
    portable source-export checks. Citation and publication metadata updates
    belong to that writing task. No arXiv upload or external publication has
    been performed here.
5.  Resolve historical manuscript/report dependencies before any evidence
    retirement. Retention is intentional while those uses remain.

# Analyzer Optimization And Comparative Evidence

Status: COMPLETE

## Goal

Reduce the VCV analyzer's processing overhead while preserving numerical
accuracy, bounded one-hop work, parameter latching, and saved-patch behavior.
Use measured ablations to distinguish implementation improvements from a
possible DSP research contribution. Preserve spec 004's confirmation archives.
This spec owns the first measured implementation iteration; event-conditioned
parameter response, energy measurement, and a new publication campaign are
follow-on work rather than outcomes inferred from this iteration.

## Requirements And Behavior

-   Retain the exact input frames, publication indices, output-bin schedule,
    window normalization, smoothing, and reset/freeze behavior.
-   Keep scalar float/double and Rack SIMD support, with no new processing
    allocation, locking, or unbounded work.
-   Measure an unchanged baseline and individual candidates using the existing
    C++ development protocol, identical compiler/provider/workload settings,
    and serial timing. Retain unfavorable results and repetition variation.
-   Start with administrative overhead: ring addressing and stage dispatch.
    Change one mechanism per screening step before combining candidates.
-   Validate the selected implementation with independent numerical checks,
    live-control regressions, Rack SIMD checks, and a plugin build.
-   Record parameter-response, energy, and publication evidence gaps explicitly.
    CPU savings alone do not establish battery savings or new FFT mathematics.

## Non-Goals

No changes to module controls, amplitude meaning, latency, UI ownership,
dependencies, or existing user work. No relaxed error budgets, approximate
windows, audio-rate modulation promise, or claim of device deadline guarantees.
ML feature extraction is a possible later application; these benchmarks do
not validate a particular model's feature contract.

## Acceptance And Validation

1.  Repeated matched development measurements identify whether a candidate
    improves cost and how callback tails change, including live and SIMD cases.
2.  Preserve arithmetic order where practical; deterministic regression tests
    cover relevant boundary, wraparound, and control transitions.
3.  Run these commands from the repository root with the existing Rack SDK:

    ```shell
    make -j2 benchmark-dev-build PAPER_VDSP=1 PAPER_FFTW_PREFIX=.build/deps/fftw
    make test/dsp/test_spectrum_analysis
    make test-spectrum-points
    make -j2 all
    git diff --check
    ```

4.  Keep exact experiment commands, decisions, results, and limitations below.
    Development results remain separate from publication confirmation. A new
    frozen, independently repeated campaign is required for final paper claims.

## Experiment Evidence

September 29, 2026: starting a focused optimization pass after FR-11 and the
fast benchmark pathway. The host reports AC power and 100% charge. Ordinary
desktop services and existing unrelated Spectre edits remain present. No
exclusive host, energy instrumentation, or independent-day replication is
claimed. The current production analyzer is the before-change control.

### Screening And Isolation

The first 240-workload screens used three repetitions and 128 measured hops
through the full development profile, with FFTW and vDSP enabled. Artifacts
remain in the original checkout under `.build/optimization-008/`:

-   `baseline-full`: unchanged analyzer.
-   `ring-full`: replace modulo addressing with bounded wrap checks. Results
    were mixed across size/live/SIMD cases; this candidate was reverted.
-   `dispatch-full`: dispatch once per contiguous stage segment within the
    existing quota. Common/large scalar cases improved, but N=128 regressed.
-   `adaptive-full`: add a direct single-unit path. This run is INVALID:
    the source-stability check detected concurrent changes to Spectrogram.cpp.
    It is retained as failed evidence and excluded from measured conclusions.

Further experiments run in the managed `analyzer-optimization` worktree at
the original HEAD, with only this task's changes. The Rack SDK and prepared
FFTW libraries are read from the original project. No uncommitted Spectre UI
changes are copied there. Do not pool these measurements with the original
checkout's screens or infer module changes from cross-checkout comparisons.

### Publication And Application Gates

The selected change must preserve the existing exact quotas. For each call,
split its quota only at stage boundaries and execute each segment in order;
there are at most four segments. The original per-unit loop handles schedules
with a base quota at most one (at most two units including the remainder).
This is an implementation optimization, not
new FFT mathematics or a claimed novel scheduling theorem.

The next research experiment should measure processing tails conditioned on
parameter-change events, request-to-first-correct-output time, and recovery
after length/hop changes. Compare bounded cache rebuilding and small resumable
kernels against both ordinary libraries and the hybrid. Include weak tones
beside strong ones and silence recovery with identical numerical contracts.
No event-response improvement follows merely from preserving publication age.

Energy claims need measured energy per analyzed second or frame under stated
power/thermal conditions. Lower CPU time is an opportunity for energy savings,
not evidence of battery-life improvement. ML comparisons would additionally
need the model's exact framing, power/log/mel normalization, and feature output
contract; visualizer magnitudes are not interchangeable with model features.

### Isolated Measurements And Selected Change

The isolated checkout starts at
`cbd21b32351374685e904b00167380936c90da13`. The selected change groups stage
dispatch for dense quotas, retains the original loop for sparse quotas, and
extracts unchanged output processing into a shared helper. It adds no buffers
or persistent state. FFT/window/smoothing arithmetic, unit counts, frame
latching, and publication/output-bin schedules are unchanged.

All runs below passed full preflight, their applicable untimed numerical
replays, and the runner's source/build/artifact checks. Five repetitions share
one process per run. All use 512 measured hops, 64 warmup hops, and the default
fixed seed. They are same-host/day development evidence, not independent
publication confirmation sessions.

| Run | Workloads | Repetitions | Purpose |
| --- | ---: | ---: | --- |
| `baseline-a` | 240 | 1200 | Original production implementation |
| `candidate-a` | 240 | 1200 | Stage segments plus a single-unit shortcut; small-window regression retained |
| `sparse-a` | 240 | 1200 | Selected stage segments plus original sparse loop |
| `baseline-b` | 47 | 235 | Rebuilt original implementation on the fixed repeat matrix |
| `sparse-b` | 47 | 235 | Rebuilt selected implementation on the same repeat matrix |

The [repeat matrix](../../docs/whitepaper/benchmarks/history/configs/optimization-008-repeat.json)
includes N=128/2048/16384, smoothing off/on, live H=257, scalar double,
independent scalar/SIMD channels, both headless modules, and unchanged PFFFT
batch/hybrid controls. The broader A matrix includes additional sizes/hops,
startup, load, FFTW/vDSP, inverse, and filtering paths.

Observed changes below are candidate/baseline comparisons of mean cost and
median per-repetition p99, separately for A and B. Ranges include smoothing
off/on where applicable. H=1024 and B=64 except the stated live H=257 case.

| Workload | Mean Cost Reduction | P99 Reduction |
| --- | --- | --- |
| Scalar float, N=2048 | 22.3--24.5% | 13.4--17.7% |
| Scalar float, N=16384 | 27.8--37.7% | 12.5--13.4% |
| Scalar float, live window/bands, N=2048/H=257 | 13.0--17.7% | 7.0--9.8% |
| Scalar double, N=2048 | 23.7--25.1% | 14.1--15.3% |
| Four independent SIMD channels, N=2048 | 6.5--8.9% | 2.8--5.4% |
| Headless Fourier module | 7.2--9.2% | 14.0--15.2% |
| Headless Spectre module | 21.1--22.8% | 13.3--18.3% |

**Regression:** scalar N=128 cost increased 3.9--9.6%, about 0.30--0.76
ns/engine sample; p99 increased 0--7.7%. Retaining the sparse loop reduced but
did not eliminate the regression. The candidate is selected for the material
gains at normal/large analyzer windows, not as a universal optimization for
small transforms. N=512 was near parity in the broad run. Threshold portability
and further sparse-path improvements remain open.

For N=16384, smoothing off, the selected core used 155.08/132.72 ns/sample
in A/B versus hybrid 127.12/126.96 and ordinary PFFFT 44.81/44.03. Its p99 was
17.166/16.958 us versus hybrid 24.125/24.375 and ordinary PFFFT 40.292/39.709.
Thus the core approaches hybrid cost with lower observed p99; ordinary PFFFT
remains much cheaper and publishes earlier. In A, one core repetition cost
246.95 ns/sample while the minimum was 131.56; it is retained in the mean.
Do not substitute the favorable B result for the full range.

Unchanged PFFFT batch/hybrid control cost-ratio medians were 0.9909/1.0024 in
A and 0.9977/1.0047 in B. A retains a hybrid outlier ratio of 2.3976; B control
ranges were 0.9668--1.0179 and 0.9958--1.0221. These controls support the large
directional improvements, not precise confidence intervals for small effects.
Other broad-matrix native providers also varied between builds/runs. No timer
subtraction or outlier removal was used.

### Commands And Retained Artifacts

Isolated data live under
`/Users/christiankauten/.codex/worktrees/analyzer-optimization/Fourier/.build/optimization-008`.
Each run retains the development manifest, exact arguments, source fingerprints
and patch, binary/environment identity, all repetitions, raw observations,
numerical diagnostics, and applicable comparison CSV. Baseline/final header
FNV-1a-64 fingerprints are `8d37326a9589c831`/`a7e86481b94bb1a0`.
These are change-detection identifiers, not a publication source archive.

The builds use the original project's prepared SDK/dependency paths:

```shell
export RACK_DIR=/Users/christiankauten/Documents/Projects/Rack
export PAPER_FFTW_PREFIX="$RACK_DIR/plugins/Fourier/.build/deps/fftw"
export DYLD_LIBRARY_PATH="$RACK_DIR"
export LD_LIBRARY_PATH="$RACK_DIR"
make -j2 benchmark-dev-build PAPER_VDSP=1
```

The exact measurement settings for the selected A/B comparisons were:

```shell
.build/benchmark/rack/paper --development --profile full --output .build/optimization-008/baseline-a --repeats 5 --hops 512 --warm-hops 64 --label 'Spec 008 isolated baseline A; AC; shared-process development evidence'
.build/benchmark/rack/paper --development --profile full --output .build/optimization-008/sparse-a --baseline .build/optimization-008/baseline-a --repeats 5 --hops 512 --warm-hops 64 --label 'Spec 008 stage segments with original sparse loop A; AC; development evidence'
.build/benchmark/rack/paper --development --profile full --config docs/whitepaper/benchmarks/configs/optimization-008-repeat.json --output .build/optimization-008/baseline-b --repeats 5 --hops 512 --warm-hops 64 --label 'Spec 008 rebuilt baseline B; fixed 47-workload repeat matrix; AC'
.build/benchmark/rack/paper --development --profile full --config docs/whitepaper/benchmarks/configs/optimization-008-repeat.json --output .build/optimization-008/sparse-b --baseline .build/optimization-008/baseline-b --repeats 5 --hops 512 --warm-hops 64 --label 'Spec 008 rebuilt segmented candidate B; fixed 47-workload repeat matrix; AC'
```

Between runs, the original or selected header was restored and the build
completed before measurement. Reproduction requires new output paths and the
matching source version for each run. Do not run these commands successively
against one unchanged executable and call it a before/after comparison.
The original checkout retains `.build/optimization-008/summarize.py` and
`repeated-comparison.json`, which join the 47 matching workload identities
without treating individual callbacks as independent repeats.

### Validation And Completion

September 29, 2026: retained the segmented dispatcher with sparse fallback in
the original checkout. Preserved unrelated Spectre changes and the existing
FR-11 artifacts. The measured final header matches the integrated header.
Updated the contributor architecture description and added the fixed repeat
matrix. No commit, push, release, or manuscript performance-claim update was
made by this task.

-   Standalone DSP: 2,167,052 assertions in nine cases passed. The added
    float/double cases compare direct DFT sums across live size/hop/window
    transitions, ring wraparound and frozen capture, and verify exact per-call
    output counts, bin order and publication indices. They pass against both
    the baseline and the selected candidate without relaxing existing tests.
-   AddressSanitizer/UndefinedBehaviorSanitizer: the same 2,167,052 assertions
    passed with `make INSTRUMENT=asan-ubsan test/dsp/test_spectrum_analysis`.
-   Rack/SIMD integration: 3,964,514 assertions in seven cases passed with
    `make test-spectrum-points`, in both the main and isolated checkouts.
-   Rack plugin: `make -j2 all` passed; the isolated checkout compiled and
    linked the actual C++11 plugin. Existing Rack deprecation warnings and
    optional FFTW deployment-target linker warnings remain.
-   All five isolated development runs above completed. The two selected
    comparisons retain all 2,870 repetitions across baseline/candidate A/B,
    plus the rejected shortcut's 1,200 repetitions separately.
-   `git diff --check`, the archived spec's local link, and the 47 unique
    workload entries passed checks.

No interactive Rack/audio-device session, energy measurement, x86-64 test,
independent-day repetition, or new publication confirmation was performed.
The sparse-window regression and run/build variability remain explicit
limitations. The publication and application gates above are follow-on
experiments, not unfinished acceptance criteria for this implementation pass.

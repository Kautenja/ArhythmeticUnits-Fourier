# Fast C++ Benchmark Development

Status: COMPLETE

## Goal

Provide a short feedback loop for FFT/analyzer optimization using the same
C++ adapters, timing boundaries, and correctness checks as the publication
experiments. Keep Python archival and plotting tools compatible with existing
evidence. Do not change production DSP or the ongoing Spectre UI work.

## Behavior And Requirements

-   `make benchmark-fast` incrementally builds only the timing executable and
    runs a fixed small matrix in one process, with no Python campaign runner,
    allocation audit executable, forced rebuild, or bootstrap analysis.
-   `make benchmark-full` runs a broader C++ development validation matrix,
    including sizes and settings absent from the tuning set. Both modes allow
    exact backend/pass/size selection and explicit measurement counts.
-   Preserve measured DSP, output stores, numerical checks, cadence replay,
    and separate timer observations. Short measurements are development
    evidence, never automatically promoted to publication confirmation.
-   Retain raw observations, contracts, numerical reports, build/environment
    identity, workload order, and per-repetition summaries. C++ comparison
    rejects mismatched workloads/environments and reports cost ratios without
    pooling callback observations into independent repetitions.
-   Support explicit frozen JSON workload arrays so the complete existing
    comparison matrix can also be exercised through the C++ runner.
-   Retain the publication archiver and its fresh-process/session semantics.
    Shared-process development repetitions are not independent sessions.

## Non-Goals

No FFT algorithm optimization, changed accuracy tolerance, universal speedup
claim, live audio deadline test, Python plotting rewrite, or replacement of
previous publication artifacts. Native library calls and canonical adapter
costs remain distinctly described; this pass does not invent native-only
measurements from adapter timings.

## Acceptance And Validation

1.  Deterministic C++ tests cover matrix selection, statistics/units,
    invalid options, and comparison compatibility/failure behavior.
2.  Fast and full paths execute serially and fail on numerical errors.
3.  A repeated unchanged build reuses its executable. Record actual fast and
    full wall times without representing them as a DSP improvement.
4.  Execute the complete frozen comparison inventory with installed providers
    after implementation; preserve its output separately from confirmation.
5.  Run existing paper infrastructure tests and an archived-protocol smoke
    campaign to check compatibility, plus applicable DSP/build checks.

The commands and completion evidence below satisfy these criteria.

## Implementation And Validation Evidence

September 29, 2026: implemented native `--development` execution, explicit
fast/full matrices, exact filters, frozen-array input, raw CSV/numerical
artifacts, descriptive statistics and matching baseline comparisons. A C++
registry generator removes the Python build dependency. The publication CLI,
adapters, and timed DSP loops retain their previous semantics. C++ runner
regressions are included in `test-rack` and its existing CI job.

The user explicitly selected C++ benchmark execution/comparison with Python
publication archival and plotting retained. Shared-process development runs
are labeled separately from independent-process/session confirmation.
FNV-1a-64 fingerprints detect changed development inputs/artifacts; the
publication archiver remains responsible for SHA-256 and full source bytes.

Validation on the local M1 Pro/macOS host, using the configured Rack tree and
Apple Clang through `c++`:

```shell
make -j2 benchmark-dev-build test-benchmark-dev PAPER_VDSP=1 PAPER_FFTW_PREFIX=.build/deps/fftw
make -j2 test/dsp/test_spectrum_analysis test/dsp/test_fft check-build
make -j2 all test-rack PAPER_VDSP=1 PAPER_FFTW_PREFIX=.build/deps/fftw
python3 -m unittest discover -s benchmark/paper -p 'test_*.py'
make benchmark-fast PAPER_VDSP=1 PAPER_FFTW_PREFIX=.build/deps/fftw BENCHMARK_DEV_OUT=.build/dev-final-baseline
make benchmark-fast PAPER_VDSP=1 PAPER_FFTW_PREFIX=.build/deps/fftw BENCHMARK_DEV_OUT=.build/dev-final-candidate BENCHMARK_DEV_ARGS="--baseline .build/dev-final-baseline-fast"
DYLD_LIBRARY_PATH=../.. .build/benchmark/rack/paper --development --profile full --output .build/dev-final-full
```

-   C++ runner: 124 assertions in seven test cases passed.
-   Spectrum analysis: 1,934,764 assertions in seven cases passed; FFT:
    39,037 assertions in 20 cases passed. Build-system fixtures passed.
-   Rack plugin build passed. All six headless/runner suites passed. This is
    not a manual Rack/audio-device session.
-   Paper tooling: 36 tests, one optional matplotlib plotting test skipped;
    all executed checks passed. All four native registry feature combinations
    matched the Python reference registry exactly after JSON decoding.
-   An unchanged incremental benchmark build emitted no commands and reused
    its executable.
-   Final fast baseline/candidate: 0.525085/0.5179 seconds, each 24 workloads
    times three repetitions. Full development: 24.5162 seconds, 240 workloads
    times three repetitions. These are runner wall times on this host, not
    DSP speedup evidence. Earlier full-development check: 24.4664 seconds.
-   Existing Rack SDK deprecation warnings and FFTW archive deployment-target
    linker warnings remain. No dependency or production-code fixes were
    attempted for those unrelated warnings.

### Complete Frozen Inventory

All 1,283 frozen configurations completed with three repetitions each, for
3,849 successful measurements and no excluded or replaced workloads. These
used the full frozen observation windows, with 1,024 transform frames and
64 warmup hops, through the native shared-process runner. They are development
validation, not a replacement confirmation campaign or an independent session.

| Group | Configurations | Repetitions | Measured Hops | Wall Time (s) |
| --- | ---: | ---: | ---: | ---: |
| Primary | 408 | 1,224 | 2,048 | 749.485 |
| Extensions | 719 | 2,157 | 512 | 905.069 |
| Short-hop | 110 | 330 | 4,096 | 81.1583 |
| Single-sample | 46 | 138 | 64 | 8.10765 |

For each group, the exact command was the following, with `GROUP` and `HOPS`
replaced by the corresponding table row:

```shell
DYLD_LIBRARY_PATH=../.. LD_LIBRARY_PATH=../.. .build/benchmark/rack/paper --development --profile full --config benchmark/paper/configs/external-confirmation-GROUP.json --output .build/dev-complete-GROUP --repeats 3 --hops HOPS --frames 1024 --warm-hops 64 --label 'Full inventory validation of C++ development runner; shared process, not publication confirmation'
```

Source/build/dependency/executable stability checks passed in each group.
A separate retained C++ validation program in `.build/dev-validation/check.cpp`
checked file fingerprints, repetition completeness, and statistics recomputed
from every raw CSV. All 4,713 repetitions across these four groups and the
final fast-baseline, fast-candidate, and full-development artifacts passed.

### Publication Compatibility And Final Usability Checks

The original publication CLI and Python archiver completed 129 smoke workloads
and passed their full archive checker, including phase/step paths excluded from
the development profiles:

```shell
python3 benchmark/paper/run.py .build/dev-publication-compat --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2 --enable-vdsp --fftw-prefix .build/deps/fftw
```

Expected-failure checks rejected deliberately corrupted copied raw data,
attempted output-directory reuse, an empty backend selection, and invalid
observation counts. The intentional corruption fixture is marked invalid;
the original baseline remains intact.

After the long measurement run, the only implementation change was appending
a shell process ID to Make's default timestamped output prefix. This prevents
sub-second runs from colliding. No measurement code changed. Rebuilt and ran
the final default configuration without FFTW/vDSP:

```shell
make -j2 benchmark-dev-build test-benchmark-dev
make benchmark-fast
make benchmark-fast
make benchmark-full BENCHMARK_DEV_OUT=.build/dev-default-final
```

All 124 runner assertions passed again. The two default fast runs completed
in 0.535008 and 0.505929 seconds with distinct output paths. The default full
profile completed all 210 workloads/630 repetitions in 23.4679 seconds.
Their raw files and recomputed summaries also passed the separate C++ checker,
bringing total independently checked development repetitions to 5,343. The
final unchanged default build emitted no commands.
Local Markdown links and `git diff --check` passed. No manual Rack session,
audio-device deadline measurement, Linux run, or Windows run was performed.
The optional plotting check remains skipped because matplotlib is absent.

Existing Spectre source, preset, manual, panel, and test changes were preserved.
No production DSP was changed by this task.

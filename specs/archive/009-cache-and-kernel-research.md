# Cache Rebuild And Resumable Kernel Research

Status: COMPLETE

## Goal

Investigate the two follow-ups from spec 008: the uneven cost of scheduled
cache rebuilding during control changes, and traversal overhead inside the
resumable FFT. Seek lower callback bursts and total processing cost without
changing numerical accuracy or parameter-to-publication delay.

## Requirements

-   Reuse the isolated benchmark checkout and existing development runner.
    Preserve the FR-11 and spec 008 evidence and unrelated manual changes.
-   Compare an unchanged baseline, one FFT traversal candidate, and one cache
    scheduling candidate separately. Keep rejected candidates and raw results.
-   Preserve input frames, window coefficients, arithmetic precision, output
    order, and one-hop publication. A scheduling experiment may redistribute
    intermediate work; explicitly distinguish that from the shipped exact
    per-sample unit contract.
-   Use repeated matched workloads, including live changes, small and large
    transforms, scalar double, SIMD, modules, and unchanged native controls.
-   Do not infer energy savings, novelty, or device deadline guarantees from
    a development timing experiment. Do not promote a prototype without
    checking the changed contract and relevant numerical regressions.

## Experiments

The production cache rebuild is already incremental: each packing unit
rebuilds two window coefficients, and each output unit rebuilds one smoothing
interval when needed. The hypothesis concerns unequal unit costs, not an
existing whole-cache rebuild at a frame boundary.

1.  Execute contiguous butterflies within a group as a resumable segment,
    hoisting twiddle stride and group-boundary bookkeeping out of the inner
    loop. Retain the original sparse path and the exact unit schedule.
    Also screen the smaller change of caching only the twiddle stride.
2.  Test additional scheduling weight for dirty window preparation, spending
    fewer expensive preparation operations per call while retaining the same
    frame publication index. Check the cost shifted into subsequent stages.
3.  Use raw callback observations to distinguish total-cost changes from
    changes in the location and height of bursts. Live workloads alternate
    Hann/Blackman-Harris and smoothing each frame; they are stress tests, not
    measurements of an isolated UI event or audio-rate modulation.

## Scheduling Mechanism

Let M=N/2, K=M+1 and B=(M/2)log2(M). The existing schedule distributes
W=M+B+2K units across H samples. The experimental dirty-window schedule uses
W'=4M+B+2K instead. Each preparation pair occupies four virtual units: execute
it at the first unit, then advance over the other three. Butterfly,
reconstruction and output units keep weight one. Clean frames return to W.

The per-call quota remains the difference of adjacent floors. A dirty
preparation segment executes at most ceil(ceil(W'/H)/4) pairs per call, but
later stages can execute up to ceil(W'/H) operations. This reduces the bound
on expensive preparation operations by increasing later-stage quotas; it is
not a reduction in arithmetic or a measured worst-case execution-time bound.
For N=16384/H=1024, preparation occupies approximately 32% of the hop instead
of 11%. No new coefficient tables, approximate windows, or twiddle recurrence
are introduced.

The refined prototype specializes the weight-one/four cases and only updates
quotas when the weight changes. The initial general-division implementation
is retained as a separate ablation. Neither changes band-cache scheduling.

This intentionally changes intermediate bin-emission positions and the public
work-count meaning. It cannot replace the production header without updating
the scheduling contract, exact-bin timing tests, and report derivation.
Full-frame publication remains at jH+H-1; therefore this experiment does not
make a latched parameter visible sooner. Mid-frame requests still wait for
the next frame boundary.

## Reproduction

The isolated checkout is
`/Users/christiankauten/.codex/worktrees/analyzer-optimization/Fourier`, on
`codex/cache-kernel-research`, based on `a26acdb`. Its previous local changes
were verified byte-for-byte against committed spec 008 before advancing the
checkout. Prior ignored measurement archives were retained. New raw data,
manifests, build logs and frozen headers are under `.build/research-009/`.

The retained [matrix](../../docs/latex/deprecated/whitepaper/benchmarks/history/configs/research-009.json) contains
55 workloads. It extends spec 008's 47-workload repeat matrix with large live
core/PFFFT/hybrid comparisons and a live H=1024 core case. Each run uses five
repetitions, 512 measured hops, 64 warmup hops and the default fixed seed.
Every run selects full preflight, including available FFTW and vDSP checks.
Timing runs are serialized; compilation and differential tests are outside
measurement. These are shared-process repetitions on the same host/day, not
independent publication sessions.

In a checkout with the experiment artifacts present, first save the original
header and prepare the build. The exact SDK/provider paths used here are:

```shell
export RACK_DIR=/Users/christiankauten/Documents/Projects/Rack
export PAPER_FFTW_PREFIX=/Users/christiankauten/Documents/Projects/Rack/plugins/Fourier/.build/deps/fftw
mkdir -p .build/research-009
git show a26acdb:src/dsp/spectrum_analysis.hpp > .build/research-009/baseline.hpp
make -j2 benchmark-dev-build PAPER_VDSP=1
```

The baseline invocation was:

```shell
DYLD_LIBRARY_PATH="$RACK_DIR" .build/benchmark/rack/paper \
    --development --profile full \
    --config docs/whitepaper/benchmarks/configs/research-009.json \
    --output .build/research-009/baseline-a \
    --repeats 5 --hops 512 --warm-hops 64 --label baseline-a
```

Use fresh output names when replaying. Each candidate patch applies separately
to the baseline, never on top of another candidate. For example:

```shell
git apply --check docs/whitepaper/benchmarks/experiments/cache-kernel-009/weighted-window-fast.patch
git apply docs/whitepaper/benchmarks/experiments/cache-kernel-009/weighted-window-fast.patch
make -j2 benchmark-dev-build PAPER_VDSP=1
DYLD_LIBRARY_PATH="$RACK_DIR" .build/benchmark/rack/paper \
    --development --profile full \
    --config docs/whitepaper/benchmarks/configs/research-009.json \
    --output .build/research-009/weighted-fast-a \
    --baseline .build/research-009/baseline-a \
    --repeats 5 --hops 512 --warm-hops 64 --label weighted-fast-a
git apply -R docs/whitepaper/benchmarks/experiments/cache-kernel-009/weighted-window-fast.patch
```

The other patches are `butterfly-segments.patch`, `cached-stride.patch`, and
`weighted-window.patch` in the same directory. Their runs are `kernel-a`,
`stride-a`, and `weighted-a`, all compared with `baseline-a`. A second rebuilt
`baseline-b` and `weighted-fast-b` repeat the same 55 workloads; the latter
compares with `baseline-b`. Raw observations and rejected variants are kept.

The [differential check](../../docs/latex/deprecated/whitepaper/data/research/cache-kernel-009/equivalence.cpp)
loads the frozen header under renamed types and compares published bins with
the candidate. Five lengths (4, 8, 128, 2048, 16384), six hops (1, 3, 37, 257,
1024, 4096), and all 15 windows cover 450 frames per precision, float/double.
It also exercises smoothing, rate changes, rejected mid-frame configuration,
capture pauses, and reset during a partial cache rebuild. Run it while the
selected candidate patch is applied:

```shell
c++ -std=c++11 -O3 -I src/dsp -I .build/research-009 \
    docs/whitepaper/benchmarks/experiments/cache-kernel-009/equivalence.cpp \
    -o .build/research-009/equivalence
.build/research-009/equivalence allow-redistribution
```

Omit `allow-redistribution` for the FFT candidates: they must also match each
individual output-bin callback position. This differential check supplements
the runner's independent numerical preflight; equivalence to production alone
does not prove production's mathematical correctness.

## Results And Decision

September 29, 2026: seven runs completed, each with 55 workloads and five
repetitions (1,925 workload repetitions total). The M1 Pro host was on AC
power at 100% charge, running Apple LLVM 21.0.0. Compiler/provider settings
were identical across comparisons and are captured in each manifest. Normal
desktop activity remained present. Build logs retain the existing FFTW
deployment-target linker warnings; these are macOS ARM measurements only.

The tables use mean callback-pass cost in ns/engine sample and the median
of the five per-repetition callback p99 values. B=64 and rate=48 kHz. A/B
are separately rebuilt, same-day comparisons, not independent sessions.

### Cache Scheduling

The initial general-division weighted prototype reduced live p99 by
21--61%, but increased live mean cost by 12--15%. It also penalized steady
scalar processing: N=16384 cost rose 17--18% and p99 rose about 51--52%.
Specializing the one/four-weight cases substantially reduced that overhead,
without eliminating it.

For the refined prototype, each range below spans comparison A and B:

| Live Workload | Baseline P99 | Weighted P99 | P99 Reduction | Mean Cost Increase |
| --- | ---: | ---: | ---: | ---: |
| N=2048, H=1024 | 12.54--12.96 us | 5.63--5.71 us | 55.2--55.9% | 4.9--5.3% |
| N=2048, H=257 | 27.13--27.29 us | 18.17--19.08 us | 30.1--33.0% | 3.1--9.3% |
| N=16384, H=1024 | 104.83--105.00 us | 38.88--39.38 us | 62.4--63.0% | 8.4--12.0% |

The N=16384 callback profile identifies the mechanism. Baseline A's first
64-sample Blackman-Harris callback has a median of 104.75 us, whereas
butterfly-dominated callbacks are around 6.8--7.8 us. Weighting spreads
preparation over more callbacks and compresses later stages. The smaller
relative benefit at H=257 illustrates the observation granularity: a
64-sample callback already spans about a quarter of the hop.

**Remaining penalty:** refined steady scalar N=2048 cost increased
1.7--6.3%; N=16384 increased 5.7--9.5%. Module results were much smaller and
mixed: Fourier cost increased 1.0--2.0%, Spectre ranged from a 0.4% decrease
to a 2.6% increase. Independent SIMD ranged from a 0.5% decrease to a 5.3%
increase. These results do not support shipping the prototype as an overall
CPU or battery optimization.

### Competitive Signal

Within the two refined-prototype runs, the large live workload has this
tradeoff (N=16384, H=1024, B=64, alternating windows/bands):

| Backend | Mean Cost | Callback P99 | Frame Publication Age |
| --- | ---: | ---: | ---: |
| Weighted core prototype | 297.39--298.36 ns/sample | 38.88--39.38 us | 1023 samples |
| PFFFT hybrid | 245.11--245.97 ns/sample | 42.92 us | 1023 samples |
| Ordinary PFFFT analysis | 170.08--170.92 ns/sample | 204.21--204.92 us | 0 samples |

The prototype achieves about 8--9% lower p99 than the hybrid here, with
about 21% more total cost and the same publication age. Against ordinary
PFFFT it has much lower bursts, but approximately 75% more cost and older
outputs. At N=2048/H=257 the hybrid still wins both cost and p99: its p99 is
14.00--14.25 us versus 18.17--19.08 us for the prototype. There is no blanket
competitive win.

Unchanged controls expose host variation. PFFFT batch callback cost ratios
span 1.005--1.057 in A and 0.929--1.024 in B; hybrid ratios span
0.983--1.027 and 0.974--1.008. The large cache-tail reduction is substantially
larger than this variation. Small mean-cost differences and the roughly
8--9% hybrid tail advantage need fresh independent confirmation.

### FFT Candidates

The contiguous butterfly-segment candidate did not improve common scalar
workloads: N=2048 steady mean cost increased 4.4--5.1%, and N=16384 increased
1.2--1.4%. Independent SIMD cost fell 7.2% in this screen. An unfavorable
live N=2048/H=1024 mean-cost result of +39.7% is retained, including its
repetition spread; p99 rose 4.3%. Do not promote a scalar optimization on the
strength of the isolated SIMD observation.

Caching only the twiddle stride was near parity in common scalar cases:
N=2048 ranged from +0.3% to +1.5% cost, and N=16384 from -0.4% to -0.2%.
Double N=2048 improved 2.3%, and independent SIMD improved 4.3% in this
screen, but headless module cost did not materially improve. These two FFT
candidates were not selected or combined with the scheduler. Larger resumable
codelets remain an untested hypothesis; these measurements do not establish
their likely speedup.

### Validation And Retained Evidence

-   All seven benchmark runs completed full preflight, their applicable
    untimed replays, source-stability checks, and artifact validation.
-   Each of the four prototypes passed the expanded differential check:
    1,672,380 published bins exactly equal to baseline values, with matching
    publication times. Both FFT variants also matched individual bin callback
    positions. Each check compiled with C++11 and `-O3`.
-   The refined weighted prototype passed the same check under ASan/UBSan:

    ```shell
    clang++ -std=c++11 -O1 -g -fsanitize=address,undefined \
        -fno-omit-frame-pointer -I src/dsp -I .build/research-009 \
        docs/whitepaper/benchmarks/experiments/cache-kernel-009/equivalence.cpp \
        -o .build/research-009/equivalence-sanitized
    .build/research-009/equivalence-sanitized allow-redistribution
    ```

-   All four patches pass `git apply --check` against the production header.
    The matrix contains 55 distinct workloads; the reporting script ran on
    all seven archives. `git diff --check` passed.
-   The isolated production header was restored and its benchmark rebuilt.
    No prototype was applied to the main production source. No commit,
    plugin installation, release, or manual Rack/device session was performed.
    A standalone plugin build and the entire DSP suite were not rerun for
    this research-only delivery; benchmark compilation instantiates Rack code
    but does not substitute for a shipped plugin build or audio-engine test.

The [summary script](../../docs/latex/deprecated/whitepaper/data/research/cache-kernel-009/summarize.py)
reads completed manifests, rejects mismatched workloads, and retains timing
values without subtracting timer overhead. `comparison.json` in the local
archive compares all runs with `baseline-a`; `repeat-comparison.json` compares
only the B pair with `baseline-b`. Generate the latter with:

```shell
python3 docs/whitepaper/benchmarks/experiments/cache-kernel-009/summarize.py \
    .build/research-009/baseline-b .build/research-009/weighted-fast-b \
    > .build/research-009/repeat-comparison.json
```

## Research Recommendation

Prioritize cache-aware scheduling over further minor FFT bookkeeping changes.
The measurable objective is lower parameter-change bursts at fixed numerical
accuracy and publication latency, subject to an explicit total-cost budget.
Before production adoption, remove the observed steady-state penalty and
test weights by window, transform size, hop, and observed stage cost. Extend
the study to band-only changes, single mid-frame requests, request-to-first
correct output, and length/hop transitions. The current live matrix changes
window and bands together every frame; it cannot attribute band-only cost or
represent typical human control frequency.

For a paper, formulate and test the joint contract: bounded expensive
operations per call, complete-frame response delay, numerical error, and
total CPU/memory cost. Include the ordinary library's latency advantage and
the hybrid's wins. Check scheduling prior art before claiming novelty, then
freeze and repeat the chosen design across independent sessions and an x86
host. Neither weighted work units nor these implementation tweaks alone
establish a new DSP algorithm. Energy-per-frame and application-specific ML
feature contracts still need separate measurements.

## Validation And Completion

Use the development runner's frozen configuration, preflight, source checks,
and retained raw CSV. Run DSP and Rack numerical/scheduling tests for any
candidate proposed for production. Record exact commands, decisions, measured
effects, and limitations here. Investigations may complete with a rejected
prototype; success does not require shipping an unsupported optimization.

# Host Efficiency And Exact Analysis

Status: COMPLETE

Completed: 2026-09-29

## Goal

Reduce actual Fourier/Spectre engine cost, especially at 16384 points, while
preserving numerical accuracy, frame publication, and responsive controls.
Test several independent candidates before selecting production changes.
Relate the results to the paper's matched analyzer comparisons without
claiming novelty for ordinary caching or ring-index arithmetic.

## Requirements

-   Preserve saved patches, IDs, window normalization, magnitude conventions,
    smoothing history, cancellation, ownership, and allocation-free processing.
-   Use matched baseline/candidate compiler settings and workloads, repeated
    measurements, unchanged external controls, and whole-callback tail checks.
-   Measure both core analysis and actual modules. Include 30 ms hops and
    N=2048/16384, plus the existing small/short-hop/live-control workloads.
-   Check the running Rack application with the same patch and settings.
    Separate sampled module meters from whole-process and callback cost.
-   Keep the user's modified example patch untouched. Preserve the prior
    installed plugin before swapping local builds for the in-app comparison.
-   Reject numerical shortcuts, control lag, or average-cost wins that cause
    substantial callback-tail regressions. No release or commit is requested.

## Candidates

1.  Avoid unity slope transcendental work and cache frame-invariant display
    geometry incrementally in scheduled output units.
2.  Replace bounded ring-index remainders with conditional wrap operations.
3.  Skip unused prefix/band/EMA work when smoothing is off, retaining correct
    history for subsequent transitions.
4.  Skip exactly unity twiddle multiplications in resumable SIMD segments.

## Acceptance And Validation

-   Independent coordinate oracle, scalar/SIMD numerical comparisons, live
    controls, cancellation, allocation checks, and exact publication tests.
-   `make -j2 test/dsp/test_spectrum_analysis test-rack all`
-   `make INSTRUMENT=asan-ubsan test/dsp/test_spectrum_analysis`
-   `make RACK_TEST_INSTRUMENT=asan-ubsan test-spectrum-points`
-   `make -j2 benchmark-dev-build PAPER_FFTW_PREFIX=.build/deps/fftw PAPER_VDSP=1`
-   Run `docs/whitepaper/benchmarks/history/configs/host-011.json` through the development runner
    with full preflight, five repetitions, 512 hops, and 64 warmup hops.
-   Record in-app observations, research positioning, failures, rejected
    candidates, exact commands, and measurement limitations before completion.

## Evidence Location

Baseline source, binaries, build logs, and measurement runs are retained in
`.build/host-011/` in this checkout. Do not clean this ignored evidence.
The production baseline is commit `bb748ec`, including spec 010's scheduler.
Only `patches/SpectrumAnalyzer-Fundamental.vcv` was modified at task start.

## Interactive Setup

The available automated app is Rack Pro 2.6.3, not the source-built Free 2.6.0
shown in the user's screenshots. The test patch's local KautenjaDSP/Sine
plugin was absent. A temporary symlink to that existing local build stalled
Pro while loading; the test process was stopped and the link removed. No
external download or change to that plugin was made.

Separate JSON patch copies substitute Fundamental 2.6.1 VCO sine outputs,
with pitch converted to semitones and clamped to its supported range. This
changes the test signals and some oscillator positions, so only matched
before/after readings in these copies are comparable. Original patch bytes
remain untouched. Analyzer settings match the supplied patch, with separate
2048/16384 copies. Host log reports 48 kHz/256-sample Core Audio output;
settings report one engine thread and 30 Hz display limit.

Baseline 16384-point observations (top/middle/bottom Fourier; all Spectre):
initial 1.5/1.7/1.6%; settled 1.6/1.9/1.9%, then 1.6/1.9/1.8%; Spectre 0.1%.
These are rounded, sampled UI observations, not statistically independent
replicates or audio callback deadline measurements.

## Initial Screens

Each screen uses all 95 workloads, five repetitions, 512 measured hops and
64 warmup hops, with full preflight and post-run source/artifact validation.
Measurements run serially, with Rack closed and no concurrent builds/tests.
Compiler: Apple LLVM 21, SDK C++11 `-O3 -funsafe-math-optimizations`, same
FFTW/vDSP builds and hardware. Host was on AC power at 100% charge.

-   `coordinates-a`: exact geometry reuse plus a zero-slope unity fast path,
    unchanged FFT core. Fourier steady mean cost falls 19.0--23.9%, callback
    p99 44.5--53.0%; analysis-control live mean falls 11.4--17.8%, p99
    27.1--35.2%. Unchanged scalar/SIMD core median cost ratios are 0.992/0.993;
    PFFFT batch/hybrid medians 0.998/0.995. This screen supports further trial.
-   `ring-a`: conditional wrap instead of integer remainder, independently of
    the coordinate changes. Fourier cost ranges 1.000--1.027 of baseline;
    independent SIMD median ratio 1.035. No useful consistent improvement;
    rejected. The scalar regression suite passed unchanged.
-   `smoothing-a`: skip prefix/band work when octave smoothing is off, and
    directly store the current magnitude when EMA alpha is zero. Fourier
    mean falls 0.1--4.8%; scalar median ratio 0.982; Spectre ratio 0.925.
    Some scalar workloads and native controls have unfavorable outliers.
    Later baseline/follow-up results below supersede the initial Spectre estimate;
    scalar regression suite passed.

The first new coordinate-oracle test incorrectly assumed bit-identical
compiler evaluation of independently inlined formulas under the SDK's unsafe
math flags; it exposed a one-ULP x-coordinate difference. Its explicit
rounding bound now uses four float epsilons (relative/absolute), tighter than
the existing coordinate seam's tolerance. Existing assertions were not
relaxed. The cache itself stores computed values without approximation.

## Selected Implementation

-   Keep exact display geometry reuse, with an explicit unity gain when slope
    is zero. Cache entries contain the frequency coordinate and slope gain;
    magnitude processing still runs for every new bin. Changes to length,
    sample rate, frequency bounds, slope, or frequency scale invalidate in
    constant time. Rebuild costs at most one entry per scheduled output, with
    a valid-prefix counter that tolerates interrupted and out-of-order use.
-   Keep the optional smoothing bypass. Disabled frequency smoothing needs
    neither prefix sums nor band endpoints. Zero EMA alpha stores the current
    value directly, so later averaging retains the correct history.
-   Keep the exact unity-twiddle shortcut in dense SIMD butterfly segments.
    Scalar and sparse traversal stay unchanged. All scheduling credit, bin
    timestamps, frame publication, capture/freeze, and mailbox ownership stay
    unchanged. This is ordinary algebraic simplification, not new FFT math.
-   Reject conditional ring wrapping: the measured implementation did not
    improve cost consistently. Production retains its original remainders.

`sizeof(SpectrumAnalyzer)` grows from 1,232 to 66,816 bytes in the local probe
(an additional 65,584 bytes, approximately 64 KiB). These figures exclude
existing dynamically allocated analysis/display buffers. The new cache is
module-owned and preallocated, with no sample-path allocation or UI access.
No control IDs, JSON meanings, slugs, amplitude units, or display decimation
change. Existing sample-rate/lifecycle allocation limitations remain.

## Ablations And Repeated Baselines

`zero-a` measures only the zero-slope shortcut: Fourier mean cost improves
0.3--2.2%. `refactor-a` uses the geometry/magnitude helper split without cache
reuse: cost is 0.1--6.8% higher than repeated baseline B. Full caching remains
10.7--23.7% faster than the zero-slope-only screen. These ablations support
reuse as the source of the large gain, rather than helper inlining alone.
The two ablations use the nine Fourier workloads with the same five
repetitions and measured/warmup lengths as the larger screen.

`unity-a` measures the unity-twiddle shortcut independently: Fourier mean
cost improves 1.7--3.3%; larger SIMD cases improve 2.5--5.5%, while small
cases range from a 1.4% improvement to a 0.3% regression. Unchanged controls
also move by about 1%, so the small effects need cautious interpretation.

Spectre's first baseline was unusually high: mean cost was 27.538 ns/sample
in A, 26.057 in repeated baseline B, and 26.023 in the cache-only screen
(which does not change Spectre). Thus the smoothing screen's apparent 7.5%
Spectre improvement is not accepted as a reliable effect. Combined A costs
25.491 ns/sample, about 2.2% below baseline B.

## Numerical And Behavioral Validation

The regular scalar suite passes 2,180,346 assertions in 13 cases, both normally
and under ASan/UBSan. The updated Rack coordinate/SIMD suite passes 5,085,418
assertions in nine cases, normally and under ASan/UBSan. The complete
`test-rack` target passes, including module amplitudes, serialization,
allocation checks, ownership/publication, and benchmark-runner checks. The
macOS ARM64 Rack plugin builds with the SDK's C++11 settings.

Differential probes compare 450 changing frames against the frozen baseline
header: lengths 4/8/128/2048/16384, hops 1/3/37/257/1024/4096, all 15 windows,
changing smoothing/sample rate, capture pauses, mid-frame configure rejection,
cancellation, and reset. Scalar float/double cover 1,672,380 bins; SIMD covers
3,344,760 lane bins with independent inputs and a silent lane.

-   With ordinary `-O3`, combined scalar float/double results match exactly.
-   With Rack's `-funsafe-math-optimizations`, combined float matches exactly.
    Double maximum absolute/relative differences are 1.4211e-14/4.4348e-16;
    SIMD differences are 3.8147e-6/2.3828e-7. The relative maxima correspond to
    ordinary rounding, not signal-model approximation.
-   The original exact-equality probes deliberately fail for double and SIMD
    under those aggressive flags; their logs are retained. Separate rounding
    probes measure the differences and enforce `16*epsilon*max(1,scale)`.
    Cadence, emission positions, rejection, freeze, and reset checks remain
    exact. Existing independent DFT/FFT accuracy assertions are unchanged.
-   The isolated unity-twiddle SIMD probe matches all 3,344,760 lane bins
    exactly even under the SDK flags. The combined rounding differences arise
    with the smoothing branch reorganization/compiler arithmetic choices.

No Windows/Linux/x86 build, hardware energy measurement, OS worst-case timing
proof, or audio-rate modulation guarantee is claimed. Manual Rack checks are
separate from the automated DSP and plugin-build results.

## Research Interpretation

The useful result is lower end-to-end analyzer cost while retaining the
one-hop schedule, complete bins, accuracy, and immediate next-frame settings.
The 64 KiB geometry cache trades memory for repeated arithmetic; it cannot
reduce the transform's asymptotic cost. Cache invalidation introduces no
all-bin preparation pass or extra control latency. Changing geometry every
frame is an essential adverse case, not interchangeable with changing only
window/analysis smoothing in the primary runner's live workloads.

[FFTW3's design paper](https://www.fftw.org/fftw-paper-ieee.pdf) already
establishes planning, codelets, SIMD, and repeated-execution optimization.
Ordinary caching and unity-twiddle removal are not defensible standalone
novelty claims. The narrower contribution is the complete resumable analyzer
with bounded preparation/output work and verified control behavior, supported
by host-level deployment evidence. Its study should compare total work,
callback concentration, accuracy, latency, and control transitions together.

Matched mono analysis at N=16384/H=1440/48 kHz, smoothing off, illustrates the
remaining tradeoff in combined A: core float costs 92.820 ns/sample with
10.167 us callback p99; PFFFT batch analysis costs 33.289 ns/sample with
40.083 us p99; PFFFT hybrid costs 90.700 ns/sample with 22.625 us p99.
Thus our path has lower measured bursts, but does not beat optimized batch
PFFFT on average CPU. These are 64-sample callback observations, not raw FFT
throughput or a comparison of four-channel Fourier against mono PFFFT.

[Rack's meter documentation](https://vcvrack.com/manual/MenuBar) describes
sampled module processing meters and their own CPU overhead. Rounded meters
corroborate deployment behavior but cannot establish confidence intervals or
callback deadlines. Battery gains are plausible from less work, not measured.
These development runs must not replace the paper's historical campaigns or
be presented as a new independent publication campaign.

## Combined Measurements

Two independent five-repetition candidate runs (`combined-a`, `combined-b`)
use the full 95-workload matrix against repeated baseline B. Cost is mean
ns/engine-sample across repetitions; p99 is the median of each repetition's
64-sample callback p99. Timers remain included. The table reports the range
of reductions across the two runs at 48 kHz, H=1440, with smoothing disabled.

| Fourier Workload | Mean Cost Reduction | Callback P99 Reduction |
| --- | --- | --- |
| N=2048, steady | 22.2--22.9% | 46.6--47.7% |
| N=16384, steady | 27.4--28.0% | 51.5--51.9% |
| N=2048, analysis controls changing | 14.5--15.4% | 30.4--31.6% |
| N=16384, analysis controls changing | 20.8--21.8% | 37.3--37.4% |

Across all nine Fourier configurations (including smoothing), reductions are
11.4--28.0% mean and 25.8--51.9% p99. SIMD core median cost ratios are
0.955/0.956; scalar core medians are 0.979/0.987. Unchanged PFFFT batch control
medians are 0.998/1.007 and hybrid medians 0.994/1.006. Thus small core effects
have less margin over environmental variation than the module/cache result.

The second run contains isolated unfavorable repetitions: Spectre's mean
rises 7.6% (per-repeat range 25.365--38.127 ns/sample, p99 unchanged from
combined A), and one scalar throughput mean rises 9.6%. A focused follow-up
retains all 28 matching N=2048 workloads from scalar, Fourier, Spectre, and
PFFFT batch, with seven repetitions and the same hop counts. Spectre returns
to 25.451 ns/sample (range 25.371--25.590), 2.3% below baseline B. The affected
scalar throughput returns to 20.315 ns/sample, 2.2% below baseline. Fourier's
smoothing-on steady case returns to 47.070 ns/sample, matching combined A.
The apparent regressions were not reproducible; all original outliers remain
in the evidence. No post-hoc repetition deletion or pooled confidence claim
is used.

The broader built-in full profile (`baseline-full`, `combined-full`) covers
240 workloads, three repetitions, 128 measured and 32 warmup hops. No changed
core/module row has mean regression above 5% or p99 regression above 10% in
that sweep. This is descriptive screening, not a universal guarantee.

The supplemental module probe covers all three magnitude scales and
N=2048/16384 with nonzero slope, four independent input signals, and
256-sample timing blocks. Two candidate runs use five repetitions of 512
measured hops after 64 warmup hops plus 2N samples. Steady geometry improves
mean cost 14.9--28.8%; changing slope, bounds, and frequency scale every frame
improves mean 2.2--4.9%. The worst live p99 change is +0.5%, with all others
approximately unchanged or lower. Reuse is not expected when every frame
invalidates the cache. Per-repetition aggregates are preserved; unlike the
primary runner, this supplemental probe does not retain individual blocks.

## In-App Result And Restoration

Both builds were installed while Rack was closed. Candidate bytes matched
`plugin.dylib` and the tested backup exactly. The same stock-oscillator patch
copies and 48 kHz/256-sample/one-thread/30 Hz settings were used for both.

| Patch | Baseline Fourier (Top/Middle/Bottom) | Candidate Fourier | Spectre |
| --- | --- | --- | --- |
| 2048 | 0.3 / 0.3 / 0.3% | 0.3 / 0.3 / 0.3% | 0.1% throughout |
| 16384, settled | 1.6 / 1.9 / 1.8--1.9% | 1.3 / 1.6 / 1.5% | 0.1% throughout |

An earlier candidate reading was 1.4/1.7/1.6%. These observations support a
visible long-transform improvement, approximately 0.3 percentage points per
module in the settled reading, without claiming a precise percentage speedup
from the rounded meters. The 2048-point meters cannot resolve the measured
headless gain. Dynamic spectra and spectrograms displayed and continued
updating after both patch loads; this was not an audio-listening test.

Attempted in-app frequency-scale clicks failed with the automation server's
`noWindowsAvailable` error, even after reconnecting. Keyboard/file-dialog
operations continued working. Interactive knob/drag response is therefore
not claimed as manually verified; deterministic control-transition tests and
the live module probe provide the executable evidence instead.

The original Pro session (`Before-spec-006-check.vcv`) and disabled meter
setting were restored; Rack was closed normally, then its original installed
plugin restored and byte-compared successfully. The temporary dependency link
is absent. The optimized workspace build remains available. The user's
modified example patch was not edited or saved over. No commit, push, release,
or change to the user's source-built Free installation was performed.

## Reproduction Commands And Preserved Artifacts

Run from the repository root with the Rack SDK two levels above, the existing
local FFTW prefix, and Apple's Accelerate framework available. Source snapshots,
raw timings, binaries, and full logs remain in `.build/host-011/`. Reproduction
patches, probes, stock test copies, control CSVs, and compressed descriptive
results/source identities are preserved in
[`docs/whitepaper/benchmarks/experiments/host-011`](../../docs/whitepaper/data/research/host-011).
The compact `measurements.json.gz` intentionally excludes per-run raw data and
Git status; use the ignored full archives for raw-block reanalysis. It retains
configuration, compiler/environment, preflight, executable/source identities,
and every workload's mean/range/tail summary, including unfavorable outcomes.

Use a separate checkout of baseline `bb748ec` for each independent candidate.
Apply exactly one named experiment patch, then rebuild; `refactor.patch`
changes helpers without using the cache. The selected combination is
`coordinates.patch` plus `smoothing.patch` plus `unity.patch`. Do not overwrite
an active checkout's uncommitted work to reproduce an ablation.

```shell
make -j2 benchmark-dev-build all PAPER_FFTW_PREFIX=.build/deps/fftw PAPER_VDSP=1
DYLD_LIBRARY_PATH=../.. .build/benchmark/rack/paper \
    --development --profile full \
    --config docs/whitepaper/benchmarks/configs/host-011.json \
    --output .build/host-011/reproduction \
    --repeats 5 --hops 512 --warm-hops 64 --label reproduction
python3 docs/whitepaper/benchmarks/experiments/host-011/summarize.py --backend fourier
python3 docs/whitepaper/benchmarks/experiments/host-011/summarize.py \
    --candidate combined-b --backend fourier
python3 docs/whitepaper/benchmarks/experiments/host-011/summarize.py \
    --baseline baseline-full --candidate combined-full
```

For broad full-profile reproduction omit `--config` and use three repetitions,
128 hops, and 32 warmup hops. For the focused follow-up use the preserved
`followup.json`, seven repetitions, 512 hops, and 64 warmup hops. Baseline
runs use the same commands/settings against baseline sources; pass their
output directory as `--baseline` for runner-generated comparisons. Run Rack,
compilation, and timing separately. Measurement summaries are not formal
publication evidence or evidence of reduced battery energy.

The supplemental control probe build/run command used the following flags
(the output filename distinguishes the baseline and candidate binaries):

```shell
c++ -std=c++11 -stdlib=libc++ -DTEST -fPIC -I. -I../../include -I../../dep/include \
    -O3 -funsafe-math-optimizations -fno-omit-frame-pointer \
    -march=armv8-a+fp+simd -mmacosx-version-min=10.9 \
    docs/whitepaper/benchmarks/experiments/host-011/control_probe.cpp \
    -o .build/host-011/controls-reproduction -L../.. -lRack
DYLD_LIBRARY_PATH=../.. .build/host-011/controls-reproduction \
    > .build/host-011/controls-reproduction.csv
```

For differential reproduction, extract the reference and compile the probes:

```shell
git show bb748ec:src/dsp/spectrum_analysis.hpp > .build/host-011/baseline-analysis.hpp
c++ -std=c++11 -O3 -funsafe-math-optimizations \
    -Isrc/dsp -I.build/host-011 \
    docs/whitepaper/benchmarks/experiments/host-011/rounding-scalar.cpp \
    -o .build/host-011/rounding-scalar-reproduction
.build/host-011/rounding-scalar-reproduction
c++ -std=c++11 -O3 -funsafe-math-optimizations -march=armv8-a+fp+simd \
    -I../../include -I../../dep/include -Isrc/dsp -I.build/host-011 \
    docs/whitepaper/benchmarks/experiments/host-011/rounding-simd.cpp \
    -o .build/host-011/rounding-simd-reproduction -L../.. -lRack
DYLD_LIBRARY_PATH=../.. .build/host-011/rounding-simd-reproduction
```

`equivalence-scalar.cpp` and `equivalence-simd.cpp` retain the stricter exact
checks (expected to fail under unsafe math as described above). Compile the
scalar exact probe without `-funsafe-math-optimizations` to reproduce its
passing strict build. `equivalence-unity.cpp` additionally needs the
`unity-analysis.hpp` snapshot from the baseline plus `unity.patch`.

## Completion Checks

-   Production DSP, Rack regressions, ASan/UBSan, and the ARM64 plugin build
    passed as detailed above. The promoted scalar/SIMD reproduction commands
    were compiled and rerun successfully with the same measured differences.
-   `make -C docs/whitepaper check` passed historical archive/campaign hashes,
    2,880 timing rows, 27 phase rows, 28 references, local links, numerical
    tables, plot coordinates, and 32,768 balanced schedules.
-   `make -C docs/whitepaper` built the 23-page report. The changed production
    pages were rendered and visually checked; the dispatch listing now floats
    with its caption to avoid an orphaned caption after table repagination.
-   Summary-script baseline/candidate and broad-profile commands, JSON parsing,
    reproduction patch reconstruction, relative-path checks, and
    `git diff --check` passed. Existing SDK deprecation and benchmark-only FFTW
    deployment-target warnings remain; no new build failure is unresolved.
-   The skipped interactive knob check and unmeasured platforms/energy are
    explicit limitations, not implied successful validation.

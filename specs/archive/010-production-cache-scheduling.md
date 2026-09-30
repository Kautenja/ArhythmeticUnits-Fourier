# Production Cache Scheduling

Status: COMPLETE

## Goal

Apply the successful spec 008/009 mechanisms to the production analyzer and
validate them through the actual Fourier/Spectre processing paths. Preserve
the already shipped dense stage dispatch and sparse fallback. Integrate the
weighted window rebuild with cheaper bookkeeping, and trial the butterfly
segment kernel only for SIMD, where the earlier screen showed a benefit.

## Requirements And Non-Goals

-   Preserve frame contents, numerical precision, complete-frame publication,
    parameter latching, input lifetime, reset/freeze semantics, and saved
    patches. Keep all processing allocation-free with bounded work.
-   Dirty window preparation receives four scheduling units per pair. Clean
    preparation receives one. Keep actual window and FFT arithmetic unchanged.
-   Account for Fourier's more expensive output callback: use two scheduling
    units per output bin there, and retain one for Spectre/default DSP users.
-   Make the public work count describe the active frame, or the next frame
    when idle, including immediately after configure/reset/publication.
-   Update exact output-bin scheduling checks and the technical derivation.
    Do not relax numerical assertions. Output remains private until complete.
-   Retain only integrations supported by repeated matched measurements;
    record regressions and uncertainty. Test the weighted scheduler separately
    from the SIMD kernel before accepting their combination.
-   No module controls, dependencies, release, install, or battery/novelty claim.

## Acceptance And Validation

1.  Deterministic tests verify weighted and clean quotas, first correct output
    after a mid-frame request, rejected configurations, cancellation during a
    cache rebuild, dense/sparse schedules, and scalar/SIMD numerical agreement.
2.  Compare the existing frozen 55-workload matrix before/after, retaining raw
    records in the existing isolated worktree. Include actual headless modules,
    native controls, live changes, and steady cases. Do not time during tests
    or builds, or modify measured source during a run.
3.  Run DSP tests, headless Rack integration tests, ASan/UBSan at the changed
    DSP seam, and a C++11 Rack plugin build. Record manual-session limitations.
4.  Keep exact commands, outcomes, decisions, and remaining tradeoffs here.
    Archive the spec only after the production integration is verified.

## Implementation

Window preparation is specialized for dirty/clean caches. A dirty segment
executes the multiples of four in its half-open unit interval, using two
integer ceiling calculations per segment. It does not iterate over skipped
credit or calculate a division/minimum for each coefficient pair. Clean
preparation has no per-pair cache-state branch. The cached preparation-unit
count adds one `size_t` to the analyzer; no new arrays are allocated.

`update_schedule()` runs when configuration/reset changes the next frame and
when a dirty frame completes. Thus the work-count API never exposes the stale
weight from the prior frame. Existing exact-quota tests read the active work
count before processing, since completion now selects the next clean quota.
New tests independently derive the 82/130-unit counts for N=32, exercise a
rejected mid-frame request followed by next-frame latching, and check the
first correct publication against a direct DFT.

Output weight is a compile-time parameter, restricted to one or two. Fourier
uses two for four-channel coordinate mapping; Spectre/default DSP users use
one. Dense output segments skip credit with integer ceiling endpoints;
sparse calls emit only at the first unit for each bin. The last real output
may precede the final credit, but publication remains exactly at the hop end.
New tests independently check N=32's 99/147-unit clean/dirty output-weight-two
frames, exact emission positions, and unchanged values/publication across
dense and sparse schedules.

The butterfly-segment kernel is selected only for non-scalar arithmetic in
dense schedules. Float/double and sparse SIMD retain the original traversal.
The kernel respects the existing quota, splits at group boundaries, and uses
the same twiddle entries and arithmetic order. Additional SIMD tests compare
every bin callback position and all four lane values through size/hop/cache
transitions and interrupted capture.

The first combined integration weighted only window preparation. Actual live
module benchmarks exposed a Fourier regression: p99 grew from 17.33 us to
23.50--23.54 us (about 36%), although Spectre improved substantially. Fourier's
coordinate callback became more concentrated. The final output weight of two
addresses that measured bottleneck. The failed combined variant is retained
in the evidence; it is not the production selection.

Engine-to-display ownership remains unchanged. Fourier writes only its
producer-owned curve slot and publishes after a complete hop. Spectre writes
its producer-owned column and publishes at that same boundary. Changed
intermediate emission times do not expose partial spectra to the display.

## Measurement Method

Experiments reuse the attached `analyzer-optimization` worktree on branch
`codex/production-cache-scheduling`, based on `f7e9095`. Old archives are
preserved; new evidence is in `.build/production-010/`. Timing is serialized
and isolated from the user's ongoing manual edits. All builds use the same
Rack SDK and FFTW libraries as spec 009, with vDSP enabled.

The 55-workload matrix from spec 009 is measured with five repetitions,
512 measured hops and 64 warmup hops. The original header is compared with
weighted-only and combined candidates; a second rebuilt baseline/combined
pair checks reproducibility. A broader 240-workload profile uses three
repetitions, 128 measured hops and 32 warmup hops. It covers startup, short
hops, additional lengths, staggered analyzers and background load.

The separate [module matrix](../../docs/whitepaper/benchmarks/configs/production-010-modules.json)
measures actual Fourier/Spectre live window/band changes at N=2048/H=1024,
with smoothing off/on and callback/throughput passes. It checks whether
rebalance of the lower-level DSP also benefits the complete engine pipeline,
including Fourier's coordinate mapping. Each case uses five repetitions and
512 measured hops. All runs use the runner's full preflight and default
fixed seed; these are same-host development comparisons, not independent
publication sessions or audio-device measurements.

## Completion Evidence

September 29, 2026: selected the weighted window preparation, SIMD butterfly
segments, and Fourier output weight two. Production source is integrated in
the main checkout and built locally. Earlier dense dispatch and the sparse
fallback remain in place. No controls, serialization, normalization, or
complete-frame latency changed.

### Runs And Reproduction

Fourteen runs completed full preflight and source/artifact checks, totaling
4,245 workload repetitions. The M1 Pro host ran Apple LLVM 21.0.0 on AC power.
Ordinary desktop activity remained present. All observations, including large
outliers and rejected integrations, are retained.

| Runs | Matrix | Repetitions Each | Role |
| --- | ---: | ---: | --- |
| `baseline-a`, `baseline-b` | 55 | 5 | Original production controls |
| `weighted-a` | 55 | 5 | Efficient window weighting alone |
| `combined-a`, `combined-b` | 55 | 5 | Window weighting plus SIMD kernel, before output balancing |
| `final-a`, `final-b` | 55 | 5 | Selected production implementation |
| `baseline-full`, `combined-full`, `final-full` | 240 | 3 | Broader coverage |
| `baseline-modules`, `combined-modules` | 8 | 5 | Actual live modules; exposes the Fourier regression |
| `output-balanced-modules-a`, `output-balanced-modules-b` | 8 | 5 | Selected output-balanced live modules |

The two final module runs share the same baseline module measurement; they
are repeated candidate processes, not two independently paired sessions.
`combined-modules` preceded `baseline-modules`; the output-balanced runs
followed it. The final 55-case A/B runs compare with their respective earlier
baselines. Do not pool these observations into a publication confidence claim.

From the isolated checkout, the build and final B invocation were:

```shell
export RACK_DIR=/Users/christiankauten/Documents/Projects/Rack
export PAPER_FFTW_PREFIX=/Users/christiankauten/Documents/Projects/Rack/plugins/Fourier/.build/deps/fftw
make -j2 benchmark-dev-build PAPER_VDSP=1
DYLD_LIBRARY_PATH="$RACK_DIR" .build/benchmark/rack/paper \
    --development --profile full \
    --config docs/whitepaper/benchmarks/configs/research-009.json \
    --output .build/production-010/final-b \
    --baseline .build/production-010/baseline-b \
    --repeats 5 --hops 512 --warm-hops 64 --label final-b
```

Use fresh output names when replaying. The full matrix omits `--config` and
uses `--repeats 3 --hops 128 --warm-hops 32`. The module matrix substitutes
`docs/whitepaper/benchmarks/configs/production-010-modules.json` and compares with
`baseline-modules`. Every manifest contains its exact invocation, compiler
settings, source identities, artifact hashes and workload definitions.

The archive includes `baseline.hpp`, `weighted.hpp`, `combined.hpp`, and
`output-balanced.hpp`; the last is selected. It also retains the original and
selected `SpectrumAnalyzer.cpp`, plus `final.patch` relative to `f7e9095`.
Reproducing the original requires restoring both original production files:
the new Fourier declaration uses a template argument absent from the old
header. The spec 009 summary script generated `comparison-a.json`,
`comparison-b.json`, `comparison-full.json`, and `comparison-modules.json` in
the same archive, each against its corresponding baseline.

### Selected Results

Figures below use mean callback-pass cost and the median per-repetition p99.
For actual modules, N=2048, H=1024, B=64 and sample rate=48 kHz. Live changes
alternate windows and smoothing every frame; this is a stress workload.

| Module And State | Baseline P99 | Selected P99 | P99 Change | Mean Cost Change |
| --- | ---: | ---: | ---: | ---: |
| Fourier, steady | 14.88--15.13 us | 9.33--9.46 us | -36.4% to -38.3% | -1.3% to +2.2% |
| Fourier, live | 17.33 us | 13.08--13.33 us | -23.1% to -24.5% | +1.2% to +2.4% |
| Spectre, live | 12.42--12.67 us | 4.83--5.04 us | -60.2% to -61.8% | -1.6% to +1.1% |

Fourier's broader full-profile steady run also reduced p99 by 39.0%, with
0.4% lower mean cost. Spectre's steady results were mixed: A reduced p99 by
9.8%, B increased it by 20.3% (2.46 to 2.96 us), and the full profile reduced
it by 6.6%. Retain that unfavorable B result; do not claim a universal
steady-state improvement.

Independent four-lane SIMD analysis used 2.8--3.2% less mean cost across the
final A/B/full comparisons, with p99 between unchanged and 2.8% higher. The
weighted-only versus initial-combined ablation reduced that SIMD cost from
29.33 to 28.30 ns/sample, supporting the kernel's inclusion. Scalar FFT
traversal remains unchanged.

Scalar core live N=2048/H=1024 p99 fell 62.5--63.8%, with mean cost between
0.9% lower and 0.6% higher. N=16384/H=1024 p99 fell 63.3--64.6%, with
7.5--10.3% more mean cost. At N=2048/H=257, A/B p99 fell 37.7--45.9%, with
mean cost between 2.0% lower and 4.5% higher. The full profile retained a
14.0% cost increase for that workload. Its additional H=37 and H=509 live
cases reduced p99 by 9.4% and 58.1%, with cost increases of 2.6% and 8.1%.

Normal N=2048 steady scalar cost remained close to baseline (-1.7% to +0.6%
in A/B), though p99 increased about 2--6%. Small/other-size cases also have
mixed results. This integration is selected for materially smaller module
and control-change bursts, with explicit cost tradeoffs, not as a universal
CPU or battery optimization.

### Variation And Limitations

-   Final A's N=128 unsmoothed scalar cost was 13.45 ns/sample versus 8.01
    baseline (+67.9%). Four repetition costs were 8.06--8.09, while one was
    34.95; all remain in the mean. P99 was unchanged. Final B and full show
    -3.3% and +0.3% mean-cost changes for that case.
-   Baseline B's N=16384 unsmoothed scalar mean includes a 331.31 ns/sample
    repetition among others near 134--137. Its 174.44 aggregate makes final
    B appear 23.3% faster. That is not evidence of a corresponding algorithmic
    speedup. The prior combined-B H=257 run also contains large unfavorable
    repetitions. Their causes were not instrumented.
-   Unchanged PFFFT batch callback cost ratios span 0.921--0.999 in final A
    and 0.988--1.011 in B. Hybrid ratios span 0.922--1.028 and 0.984--1.002.
    Small cost differences are comparable to this host variation. The large
    module-tail reductions are the stronger signal.
-   These are same-host macOS ARM development results. No x86/device repeat,
    energy instrumentation, audio callback deadline guarantee, or new DSP
    novelty claim is established. UI/GPU work is excluded from timing.

### Correctness And Build Validation

Commands below ran from the main checkout against the selected implementation:

```shell
make -j2 test/dsp/test_spectrum_analysis test-spectrum-points
make -j2 test-rack all
make INSTRUMENT=asan-ubsan test/dsp/test_spectrum_analysis
make -j2 RACK_TEST_INSTRUMENT=asan-ubsan test-spectrum-points
python3 docs/whitepaper/experiments/check.py
git diff --check
```

-   Scalar DSP: 2,180,346 assertions in 13 cases passed, also under ASan/UBSan.
-   Spectrum/Rack SIMD: 4,093,426 assertions in eight cases passed, also under
    ASan/UBSan. This includes allocation checks, snapshot ownership, and live
    cache/hop transitions. All remaining headless Rack suites passed.
-   The C++11 macOS ARM plugin linked successfully. No plugin install, release,
    commit, or manual Rack/audio-device session was performed.
-   The spec 009 differential program compiled with C++11/`-O3` against the
    frozen original header and final default analyzer. All 1,672,380 published
    float/double bins matched exactly, with identical publication times.
    Output-weight-two equivalence and emission positions are covered by the
    new deterministic tests above.
-   The technical report compiled successfully in the built-in LaTeX editor.
    Its checker passed historical hashes/tables and the clean/dirty scheduling
    examples for both output weights. Historical timing data was not changed
    or relabeled as measurements of this implementation.

Validation logs are copied into `.build/production-010/`. Production source
bytes were checked against the measured worktree after the final runs. The
user's concurrent manual reorganization remains separate and preserved.

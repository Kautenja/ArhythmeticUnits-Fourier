# External FFT Comparison For The Scheduling Paper

Compare forward transforms, inverse transforms, complete frequency-domain DSP,
and the production one-hop analyzer with practical FFT libraries using the
existing publication protocol. Establish where resumable execution earns its
CPU, storage, completion-latency, and sample-delivery costs, including cases
where batch processing is preferable. The Rack analyzer is an application
case study, not the boundary of the reusable framework's evaluation.

Status: IN PROGRESS

Created: September 29, 2026

This spec supersedes the detailed plan formerly in
[`benchmark/paper/comparisons.md`](../benchmark/paper/comparisons.md).
The [protocol README](../benchmark/paper/README.md) defines current measurement
semantics. External adapters and external campaign configurations remain
future work. First-party inverse and complete-chain baselines are implemented;
their smoke validation does not constitute external comparison evidence.

## Review Of The Current Work

The reviewed checkout is `4e58290`. Its relevant improvements are:

-   [`SpectrumAnalysis`](../src/dsp/spectrum_analysis.hpp) schedules packing,
    windowing, butterflies, positive-bin reconstruction, prefix sums, smoothing,
    and output over exactly one hop. Cache rebuilding is scheduled; retained
    input, prepared plans, cancellation, and output ownership are explicit.
    The original frame-sized boundary passes have been addressed in production.
-   The [publication driver](../benchmark/rack/paper.cpp) measures production
    cores, actual headless modules, and fixed-cadence legacy controls. It adds
    callback observations, separate throughput passes, analyzer alignment/load,
    startup/live/cache-pressure cases, and FFT/RFFT/IFFT phase measurements.
    Untimed replay checks publication cadence and algorithmic spectrum age.
-   The runner retains raw observations, source archives, build commands,
    dependency hashes, order, and numerical reports. Its artifact validator
    independently recomputes summaries and checks completeness.
-   The [manuscript](../docs/whitepaper/fourier.tex) now relates the work to
    time-distributed transforms, overlap reuse, visualizer latency, practical
    backends, numerical error, and measurement methodology. Historical and
    prototype timings are explicitly separated from production behavior.

The main evidence gap is now external comparison and measured production
performance. More bibliography expansion is lower priority. The current
expanded paper profile is a coverage matrix, not automatically a suitable
first comparison campaign. Fresh processes are not independent host sessions.
Synchronous block measurements are not device underrun measurements.

## Questions And Behavior Examples

1.  At the same input windows, hop, precision, and required outputs, how do
    aggregate cost, callback-duration tails, storage, and publication age differ
    between production Fourier and an optimized batch analyzer?
2.  Does suspending the transform itself provide useful additional peak
    reduction after the surrounding analysis passes have been scheduled?
3.  How do these tradeoffs change with transform length, callback size,
    analyzer count, phase alignment, smoothing, and available host budget?
4.  How does the same scheduling choice affect periodically released complex
    inverse jobs, including buffering, normalization, and complete output?
5.  In forward-transform -> spectral operation -> inverse-transform processing,
    how do total cost, callback peaks, correct sample delivery, and algorithmic
    latency compare? Does a transform-only advantage survive integration?

For N=4096, H=1024, and 64-sample blocks, both adapters must represent the
window ending at jH and emit all 2049 positive-bin magnitudes. An immediate
batch adapter publishes at jH; the production core publishes at jH+1023.
Report that difference and callback-end visibility explicitly. Delaying a
batch result without changing its execution time is not peak reduction.

If a library produces packed DC/Nyquist or unordered coefficients, its adapter
must interpret or convert that layout and charge the necessary work. A
transform-only result cannot stand in for a smoothed analysis result. Four
independent SIMD lanes must be compared with four independent scalar channels,
not with one scalar transform.

For an overlap-save filter with N=4096 and H=1024, a frame ending at jH
produces the filtered samples for jH-1023 through jH. Starting their playback
at jH+d incurs 1023+d samples of delivery delay. A batch control has d=0;
the current incremental baseline has d=1023. That 2046-sample delivery delay
is distinct from the 1023-sample computation delay. Verify the entire delivered
waveform against direct filtering rather than reporting only frame completion.

## First-Party Evidence Pathways

Forward, inverse, and complete-chain baselines are required before external
comparisons. The [protocol](../benchmark/paper/README.md#inverse-and-end-to-end-baselines)
documents the implemented semantics and reproduction commands.

| Family | Implemented Baseline | Required External Comparison |
| --- | --- | --- |
| Isolated transforms | Complex FFT, real FFT, and normalized complex IFFT; float/double; complete/incremental, buffer/compute/output phases and individual steps | Equivalent transform kind and precision, charging conversion and inverse normalization |
| Periodic inverse jobs | `inverse-stream-{batch,incremental}-{float,double}`; analytical non-Hermitian spectra, all N output stores, exact release cadence, callback/throughput and instance/load sweeps | Same spectrum releases, full complex output, normalization, and completion-age contract |
| End-to-end frequency-domain DSP | `ols-{identity,fir}-{batch,incremental}-{float,double}`; complex FFT, transfer multiplication, normalized IFFT, overlap-save extraction and continuous sample delivery | Same complex input, transfer function, valid outputs, delivery sink, and measured algorithmic latency |
| Spectral analyzer | Existing production core, same-arithmetic legacy controls, and actual headless modules | Existing matched analysis contract; module costs remain separately labeled |

The new benchmark-only implementation is in
[`synthesis.hpp`](../benchmark/paper/synthesis.hpp). Its logic and independent
verifier need no Rack types; the shared campaign executable still links Rack
for its module workloads. Both modes preserve the current transform APIs'
bulk buffering. Preparation inside the chain must be counted even if it is
represented by one indivisible scheduling unit. These are executable baselines,
not a claim that the analyzer's fully distributed preparation also exists in
the reusable inverse or synthesis path.

The inverse streaming fixtures alternate two known complex spectra without
calling a forward transform to manufacture expected results. Isolated inverse
passes also cover dense complex inputs. The chain includes an identity control
and the causal FIR `[0.5, -0.25, 0.125]`, validated independently in time domain.
Overlap-save requires H <= N-2; no STFT windows or overlap-add normalization
are involved. The complete path includes retention, buffering, transforms,
N spectral multiplications, normalized inverse output, H valid output stores,
buffer exchange, and per-sample delivery through a common observable checksum.
The short FIR is a correctness/application fixture, not a claim that FFT-based
filtering is preferable to evaluating three taps directly.

Preserve the distinction between release-to-completion delay for inverse jobs,
frame-end-to-publication age for analysis, and input-to-delivery delay for
filtered samples. Metadata and artifact checks must encode their different
time origins. Inverse jobs with N outputs every H calls are periodic transform
tasks, not necessarily an audio stream with N=H.

## Contenders And Order

| Priority | Candidate | Question It Answers | Readiness And Route |
| --- | --- | --- | --- |
| 1 | [Rack/PFFFT][rack-fft] | Would using the FFT already available in this host be preferable? | Ordered `dsp::RealFFT` for analysis; ordered `dsp::ComplexFFT` forward/inverse for matched inverse and chain work. Pin the linked implementation; adapters pending. |
| 2 | [FFTW3][fftw-real] | How does an optimized portable library with reusable plans compare? | Single-threaded float real-to-complex and complex forward/backward plans, with explicit inverse scaling; double separately. Adapters and dependency capture pending. |
| 3 | [Apple Accelerate/vDSP][vdsp] | What is the practical platform-library alternative on the Apple measurement host? | macOS real and complex transforms with reusable setup and explicit packing/scaling; adapters pending. This is a platform baseline, not an open-source implementation. |
| Reserve | [KISS FFT][kiss] | What changes with a small, portable C implementation and different setup/storage tradeoffs? | Upstream provides a real-transform API. No local integration verified; add only if this question remains material after the first three. |
| Academic follow-on | [Garrido's feedforward STFT][garrido] | Does reusing work across overlapping windows change the cost/age frontier? | The paper supplies algorithm descriptions; a matched CPU implementation, supported hops/windowing, and accuracy validation need feasibility review. Not a ready drop-in backend. |

Keep `core-*` and `legacy-*` as controls. PFFFT through Rack and direct PFFFT
are the same algorithm family, not independent contenders. A direct variant
is justified for a specific caller-owned scratch or layout question. The
inspected Rack wrapper passes null scratch pointers; inspect the pinned PFFFT
source and report actual scratch behavior rather than assuming it.

Windowed sliding/hopping methods are a second academic route if the pilot
identifies high overlap as important. [Rafii's window kernels][rafii] provide
windowing context; choose a specific update algorithm with its own stability
analysis. Check implementation availability, license, precision, supported
hops, endpoint convention, and real-input support before committing to a
reimplementation. Charge updates between requested publications and include
window-kernel work. Computing every sliding spectrum and publishing every Hth
one is different from an algorithm that exploits H directly.

Time-distributed convolution and control papers remain essential conceptual
comparators, but their complete applications are different workloads.
[Battenberg and Avizienis][battenberg] also motivate a later worker-thread
comparison. Neither a batch library nor a new interpretation of academic
pseudocode should be presented as the authors' measured implementation.

## Requirements

### Comparable Adapters

-   Extend the existing protocol rather than adding a parallel timing harness.
    Keep external dependencies optional for ordinary plugin/test builds.
    A requested unavailable backend must fail clearly; inventory output may
    label it unavailable. Never silently substitute another implementation.
-   Record supported precision, transform kind, sizes, channel count, output
    layout, thread policy, setup policy, and publication contract. Update the
    C++ dispatch, Python workload validation, and artifact checker together.
    The current checker infers delays and transform steps from backend names;
    external and hybrid backends need explicit validated contracts. Do not
    apply the radix-2 step formula to an opaque library execution.
-   Provide two measurement boundaries: (a) real-transform execution from
    prepared windowed samples through canonical positive complex bins,
    including required packing/scaling/reordering; (b) complete streaming
    analysis including retention, windowing, magnitudes, smoothing, and K
    output stores. Native packed-layout processing may be a separately
    labeled optimization with equivalent required output. Keep module/UI
    results separate from both. Also require isolated complex inverse,
    periodic inverse jobs, and complete complex filtering boundaries as
    specified above. A forward-only adapter is not a completed contender.
-   Share input bytes and settings with existing controls: normalized periodic
    Hann, common float window coefficients, smoothing off/on, alpha=0.8 when
    selected, and identical zero-padding and frame endpoints. Promote the same
    input bytes for double. Audit exact cadence, startup, and live settings.
    Report unsupported combinations explicitly. Use each batch result as soon
    as available in the primary comparison; report its lower age.
-   Preallocate plans and buffers outside steady-state timing. Record setup
    and destruction separately, persistent/scratch storage, alignment, and
    allocation behavior. Do not interpret the core's maximum-capacity plans
    and an exact-size library plan as equivalent memory policies; report both
    capacities and test sensitivity where material.
-   Start FFTW with single-threaded `FFTW_MEASURE`, no imported wisdom, and a
    fresh process as documented in [planner flags][fftw-flags]. Restore inputs
    after planning. Retain exported plan/wisdom information when available to
    explain plan variation. An `ESTIMATE` sensitivity study is a separate
    configuration. Record library versions, build/SIMD options, source and
    binary hashes, compiler/FP flags, and OS/SDK/framework identity. Hashing
    a wrapper alone does not identify its linked implementation. For opaque
    platform libraries, state what cannot be independently rebuilt or hashed.

### Correctness Before Timing

-   Verify canonical complex bins against independent direct sums for small
    N and selected bins for larger N, and compare all large-size bins against
    an independently implemented transform. Report the reference's actual
    precision; long double is not necessarily wider than double on the host.
-   Cover silence, impulse, DC, Nyquist, bin-centered/off-bin tones, and seeded
    noise. Verify processed magnitudes, smoothing history, live window changes,
    startup, ring wraparound, frame endpoints, and publication indices.
-   Define absolute and scale-aware tolerances before collecting results.
    Preserve maximum absolute and normalized errors and normalization/sign
    checks. Different factorizations need not be bitwise equal. Round trips
    complement independent references; they cannot detect every paired error.
-   Retain current IFFT evidence and require external inverse comparisons. Match
    complex-to-complex versus complex-to-real semantics and include the
    normalization needed for the same output contract. Test non-Hermitian
    spectra so that dropping the imaginary output cannot pass. Preserve
    all-sample analytical inverse and direct identity/FIR checks in untimed
    replay, including startup, valid-output boundaries, and delivery latency.
    Independent references must not share the tested forward/inverse pair.
-   Before an overlap-reuse comparison, add long-stream error traces, weak tones
    beside strong ones, and silence after a strong signal. Recompute independent
    references throughout the stream; define phase error only above a stated
    reference-magnitude threshold. Include any periodic refresh cost.

### Attribution Experiment

After the batch pilot, add a benchmark-only hybrid using a competitive external
FFT: spread preparation and postprocessing across the hop, but execute the FFT
as one indivisible library call. Define its stage schedule before measuring;
verify dependencies and exact H-1 publication age, and charge all retained-input
and scheduling costs. The library call is not equivalent to one constant-cost
butterfly unit and receives no work-count timing guarantee.

Compare full batch, hybrid, and production resumable analysis. This tests
whether a resumable FFT earns a useful improvement beyond scheduling its
surrounding passes. Use matched surrounding operations where possible and
state remaining differences in arithmetic, storage, and data layout. Existing
legacy batch/incremental controls isolate scheduling within their arithmetic;
comparing the legacy controls with the production core does not isolate
scheduling alone. If a causal claim about production scheduling overhead is
needed, also supply a batch execution of the same positive-bin pipeline.

## Staged Measurement Plan

1.  **Adapter gate:** Implement and verify Rack/PFFFT first, then FFTW and
    vDSP. Produce a backend inventory and a short smoke configuration for all
    available adapters. Require real forward analysis and complex forward,
    inverse, periodic inverse, and end-to-end float controls; double follows
    for supported libraries. Do not replace a production backend.
2.  **Focused pilot:** Start with N=2048/4096/16384, H=1024, blocks of 16/64/256,
    48 kHz, float, one analyzer, steady state, smoothing off/on, and separate
    callback/throughput passes. Include the core and matched legacy controls.
    Use a resolved configuration file and report the workload count. Inspect
    timer resolution and session variability before choosing repetitions.
    Include inverse jobs and both overlap-save controls as separate families;
    do not combine their costs or time origins in a single ranking.
3.  **Discriminating extensions:** Add N=128, H=257, 96 kHz, 1/4/16 analyzers,
    aligned/staggered phases, fixed background load, and startup/live/cache
    pressure as focused sweeps. Include callback-origin phase offsets where
    needed: analyzer staggering alone does not vary every shared relationship
    between the callback grid and frame schedule. Add equal four-channel work
    for `core-simd4`; double is a separate supported-backend sweep.
4.  **Hybrid and confirmation:** Run the attribution experiment, then repeat
    the frozen comparison across independent sessions. Use at least three
    sessions as an initial coverage floor, with final run lengths and counts
    justified by pilot variability and the tail events of interest. Repeat a
    focused portable subset on ARM64 and x86-64 before making cross-architecture
    claims. Missing hardware limits claims; it does not justify invented data.
5.  **Publication integration:** Generate tables and figures from validated
    campaigns. Pin the measured production revision, distinguish historical
    evidence, and reorganize the paper around the measured tradeoffs. Preserve
    prior campaign archives. Promote academic/worker/host comparisons only
    when a remaining research question warrants their added scope.

Measure serially on otherwise idle hosts; never run compilation or other
benchmarks concurrently. Record power/thermal conditions, host activity,
session identity, actual order, build flags, and plan policy. Fresh processes
and seeded ordering are useful but do not replace independent sessions.
Account for rebuild/layout variation if the pilot finds it relevant.

Retain raw callback observations and full per-session summaries. Report
aggregate cost and uncertainty at the process/session level, callback tail
distributions with counts and duration, and observed maxima with comparable
observation windows. Never pool callbacks as independent experimental repeats
or treat a quantile based on too few tail observations as precise. Keep timer
controls; do not mechanically subtract them or driver overhead. Call simulated
budget exceedances by that name, not audio underruns or worst-case bounds.

## Outputs And Acceptance Criteria

- [x] First-party inverse jobs and complete identity/FIR chains have batch and
    incremental controls, independent all-output validation, latency contracts,
    workload configurations, and raw-artifact checks. Publication campaigns
    and external comparisons remain outstanding.
- [ ] Three primary backend adapters pass independent numerical and matched
    analysis/inverse/complete-chain checks on supported hosts; unavailable
    cases are explicit. Required output normalization is included in cost.
- [ ] Dependency/setup/storage/publication contracts and exact reproduction
    commands are documented. Archived artifacts identify measured sources,
    libraries, flags, workloads, and numerical checks without relying on HEAD.
- [ ] Runner/checker regressions reject wrong scaling/layout, missing outputs,
    wrong publication age, unsupported configurations, altered dependencies,
    duplicate/missing runs, and invalid timing values.
- [ ] A retained pilot justifies the final matrix, repetitions, session count,
    and observation duration. A frozen confirmation campaign follows the pilot;
    cases favoring batch processing remain in the reported matrix.
- [ ] The hybrid comparison separates the practical value of suspending the
    FFT from scheduling the surrounding work, with remaining confounds stated.
- [ ] Generated outputs include an implementation/provenance/error/storage
    table; matched workload/cost/age table; transform and full-analysis cost
    versus N; callback tail distributions; and cost versus spectrum-age plots.
    Inverse and complete-chain panels report release/completion and sample
    delivery latency, respectively, with their independent numerical evidence.
    Each output identifies its campaign and includes uncertainty or variation
    appropriate to that statistic. No upstream performance chart substitutes
    for these measurements.
- [ ] The paper reports benefits, regressions, crossover regimes, accuracy,
    limitations, and measured platform scope. Its tables/figures and citation
    metadata are updated consistently; build, artifact checks, and PDF review
    pass. An unfavorable result still satisfies this spec if supported.

## Non-Goals

This spec does not replace the production FFT, change saved patches, optimize
code to win a benchmark, require every cited algorithm to be implemented, or
claim new FFT mathematics. GPU comparisons, a user-facing synthesis application,
general windowed STFT reconstruction, and general-purpose FFT-library rankings
are outside scope. The benchmark-only overlap-save operation is in scope.

Worker threads require queueing, completion, dropped-job, and contention
measurements beyond the synchronous protocol. Actual device deadlines and
screen latency require a host experiment with display consumption and stated
observation duration. These are follow-ons if the paper makes those claims,
not outcomes inferred from this comparison's block measurements.

## Validation Commands

Run from the repository root with Python 3, SCons, and the configured Rack SDK
and compiler. The commands below exist today; the shown runtime library path
assumes the default `../..` Rack layout. Substitute the configured SDK directory
for a different layout; Windows needs its DLL search path configured.

```shell
python3 -m unittest discover -s benchmark/paper -p 'test_*.py'
scons test/dsp/test_spectrum_analysis.cpp
make benchmark-paper-build
DYLD_LIBRARY_PATH="../.." LD_LIBRARY_PATH="../.." build/benchmark/rack/paper --verify
make -C docs/whitepaper check
git diff --check
```

The Python suite now compiles the host-independent synthesis verifier using
`CXX` (default `c++`) and C++11. It does not need Rack or Catch2 for that check.
The following first-party configuration exists now; use new directories:

```shell
python3 benchmark/paper/run.py build/paper-inverse-smoke --config benchmark/paper/configs/synthesis-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 benchmark/paper/check.py build/paper-inverse-smoke
python3 benchmark/paper/run.py build/paper-synthesis-session-01 --profile synthesis --list
```

The dedicated smoke config contains 48 streaming and eight isolated inverse
workloads; the synthesis factor profile contains 216 streaming workloads.
Its controls require no analyzer settings and reject live-window/smoothing
options. Resolve feasible H/N combinations per family before measurement.

The implementation must add tracked `external-smoke.json` and
`external-pilot.json` under `benchmark/paper/configs/`, with explicit supported
backends per host. Those files do not exist yet. Once supplied, these are the
required smoke/pilot commands; output directories must be new:

```shell
python3 benchmark/paper/run.py build/paper-external-smoke --config benchmark/paper/configs/external-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 benchmark/paper/check.py build/paper-external-smoke
python3 benchmark/paper/run.py build/paper-external-pilot --config benchmark/paper/configs/external-pilot.json --list
python3 benchmark/paper/run.py build/paper-external-pilot --config benchmark/paper/configs/external-pilot.json
python3 benchmark/paper/check.py build/paper-external-pilot
```

Document dependency installation/build commands when versions and integration
are selected, and record actual host notes with each campaign. Before the final
campaign, add its frozen configuration and exact commands, including selected
repetition counts, run lengths, seeds, and session directories. Supply a
deterministic figure/table generator and its validation command as part of
implementation. Smoke checks are not publication measurements.

## Review Evidence And Remaining Work

September 29, 2026: source/protocol/manuscript review completed. The focused
production suite passed 1,934,764 assertions in seven cases; five Python
protocol tests passed. The benchmark build target was up to date, and its
`--verify` passed 48 scalar configurations against two controls. The manuscript
artifact check passed both historical campaigns, 28 references, and schedule,
table, and plot consistency checks. No new publication campaign, external
adapter, plugin build, or interactive Rack session was performed for this spec.
Local documentation links and `git diff --check` also passed. At that review,
implementation and measurement acceptance boxes remained open.

### Inverse And Complete-Chain Baseline Completion

September 29, 2026: added the twelve inverse-job/identity/FIR adapters,
explicit latency contracts, full-output numerical replay, a standalone C++
verifier, the 56-workload smoke config, and the 216-workload synthesis profile.
The paper profile now has 1127 workloads; the general smoke profile has 129.
Only the first-party baseline acceptance item is complete.

Validation performed:

-   `python3 -m unittest discover -s benchmark/paper -p 'test_*.py'`: nine tests
    passed, including standalone C++11 compilation without Rack/Catch2,
    analytical inverse and direct-filter fixtures, deliberate output/scaling
    corruption, latency-contract checks, and missing numerical coverage.
-   `make benchmark-paper-build` and the expanded `--verify`: passed. Existing
    Rack SDK deprecation warnings remain. This is a benchmark executable
    build, not a plugin build or an interactive host test.
-   `python3 benchmark/paper/run.py build/paper-synthesis-baseline-verified
    --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2`, followed by
    `python3 benchmark/paper/check.py build/paper-synthesis-baseline-verified`:
    129 runs passed, including 48 new streaming runs and the existing isolated
    inverse passes. The new runs checked 336 publications, 28672 output values,
    and 7168 playback samples. Sources, build flags, raw observations and
    numerical reports are retained in that ignored build directory.
-   The dedicated smoke config resolves successfully with `--list`;
    manuscript artifact checks, local documentation links, and
    `git diff --check` pass. Historical paper results are unchanged.

These short runs verify executable evidence pathways; they do not justify a
speedup, tail-latency ranking, or device deadline claim. External adapters,
the hybrid comparison, statistically justified campaigns, and publication
figures remain open. No production DSP algorithm or plugin behavior was
changed for this baseline work.

[rack-fft]: https://github.com/VCVRack/Rack/blob/v2/include/dsp/fft.hpp
[fftw-real]: https://www.fftw.org/fftw3_doc/Real_002ddata-DFTs.html
[fftw-flags]: https://www.fftw.org/fftw3_doc/Planner-Flags.html
[vdsp]: https://developer.apple.com/library/archive/documentation/Performance/Conceptual/vDSP_Programming_Guide/UsingFourierTransforms/UsingFourierTransforms.html
[kiss]: https://github.com/mborgerding/kissfft
[garrido]: https://www.diva-portal.org/smash/get/diva2:1014928/FULLTEXT01.pdf
[rafii]: https://zafarrafii.com/Documents/Journals/Rafii%20-%20Sliding%20Discrete%20Fourier%20Transform%20with%20Kernel%20Windowing%20-%202018.pdf
[battenberg]: https://ericbattenberg.com/pdf/partconvDAFx2011.pdf

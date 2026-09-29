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
semantics. FR-1 through FR-6 are implemented, including Rack/PFFFT, optional
FFTW/vDSP, inverse and complete-chain baselines, and the matched hybrid
comparison. FR-7 through FR-9 are explicitly deferred for the current paper
under the [optional contender decision](#optional-contender-decision).
FR-10's final campaign/report tooling remains the next required stage.
Implementation smoke checks do not constitute publication comparison evidence.

Complete implementation, correctness checks, and benchmark tooling first.
Each framework or algorithm has its own functional requirement (FR), with an
implementation/integration step followed by a benchmark implementation step.
Short smoke runs validate the tooling during development; all comparative
metric gathering, including the pilot, waits until FR-11's final measurement
phase on a prepared host. Individual implementation FRs can complete before
that phase; this spec remains in progress until the results and paper pass.

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
| 1 | [Rack/PFFFT][rack-fft] | Would using the FFT already available in this host be preferable? | Ordered `dsp::RealFFT` for analysis; ordered `dsp::ComplexFFT` forward/inverse for matched inverse and chain work. Implemented as benchmark-only adapters; wrapper/source/library identities retained. |
| 2 | [FFTW3][fftw-real] | How does an optimized portable library with reusable plans compare? | Single-threaded float real-to-complex and complex forward/backward plans, with explicit inverse scaling; double separately. Implemented with explicit optional build, per-instance plans, and dependency archives. |
| 3 | [Apple Accelerate/vDSP][vdsp] | What is the practical platform-library alternative on the Apple measurement host? | macOS float/double real and complex adapters with reusable setup, explicit packing/scaling, and retained platform identity are implemented. This is a platform baseline, not an open-source implementation. |
| Reserve | [KISS FFT][kiss] | What changes with a small, portable C implementation and different setup/storage tradeoffs? | Feasible portable library reserve; FR-7 deferred because the current study has no distinct minimal-dependency or embedded setup/storage question. |
| Academic follow-on | [Garrido's feedforward STFT][garrido] | Does reusing work across overlapping windows change the cost/age frontier? | FR-8 deferred: an overlap-reuse study needs its own matched CPU, hop/window and numerical validation. Reassess the later partial-overlap formulation if reopened. |

Keep `core-*` and `legacy-*` as controls. PFFFT through Rack and direct PFFFT
are the same algorithm family, not independent contenders. A direct variant
is justified for a specific caller-owned scratch or layout question. The
inspected Rack wrapper passes null scratch pointers; inspect the pinned PFFFT
source and report actual scratch behavior rather than assuming it.

Windowed sliding/hopping methods are the highest-priority academic follow-up,
but FR-9 is deferred for the current paper. Reopen it when high overlap or
selected-bin tracking becomes a central comparison claim.
[Rafii's window kernels][rafii] provide windowing context; choose a specific update algorithm with its own stability
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

## Optional Contender Decision

September 29, 2026; reviewed implementation `4e78d2d`, current manuscript scope,
FR-3 through FR-6 evidence paths, and the primary sources linked below.
**Defer FR-7, FR-8 and FR-9 for this paper's current scope.** These are explicit
scope decisions, not completed adapter implementations or measured rankings.
No smoke timings were used to select or exclude a contender.

| Stage | Decision | Marginal Evidence Value Now | Reopen When |
| --- | --- | --- | --- |
| FR-7: KISS FFT | DEFERRED | Adds another batch implementation; existing optimized libraries and same-arithmetic controls already address the scheduling questions. | A concrete embedded, minimal-dependency, fixed-point, or setup/storage deployment question needs a small C implementation. |
| FR-8: Garrido feedforward STFT | DEFERRED | Adds a different cross-frame algorithm, not another implementation of the same periodic FFT job. Requires a separately validated CPU/window/hop experiment. | The paper explicitly studies full-spectrum overlap reuse or offers comparative claims about feedforward methods. |
| FR-9: Windowed sliding/hopping | DEFERRED; first academic follow-up | Most useful challenge to high-overlap or selected-bin claims, but those are not the present contribution. | Such claims become central, or a target venue/reviewer requires an empirical overlap-reuse comparison. |

### FR-7 Evidence And Rationale

[KISS FFT's upstream source][kiss] provides a compact C implementation;
its [complex API][kiss-complex-api] supports transform direction and caller
storage for setup, and its [real API][kiss-real-api] exposes forward/inverse
real transforms. The project documents BSD licensing and configurable
arithmetic. Feasibility is not the reason for deferral. No local KISS build,
allocation audit, or performance result was produced by this review.

Our assessment is that another batch library has lower marginal value than
finishing matched SIMD/channel, precision, resource and uncertainty reporting.
FFTW and Rack/PFFFT already provide distinct portable batch baselines; vDSP
adds the platform baseline. KISS is neither presumed slower nor dismissed as
an inferior implementation. Reopening FR-7 requires a concrete deployment
question and the full forward/inverse/chain contracts, not merely another row
in a speed chart.

### FR-8 Evidence And Rationale

[Garrido 2016][garrido] supplies feedforward pseudocode, real-input symmetry,
and a small MATLAB timing example; it does not establish CPU performance for
our workloads. Its one-sample-hop rectangular formulation needs adaptation to
our requested hops and windows. [Eleftheriadis, Garrido and Karakonstantis
2023][fd-stft] explicitly addresses partial overlap and windowing, so a future
study should not treat the 2016 restrictions as limits of the whole family.
Its hardware results likewise cannot rank our software adapters.

The reviewed materials do not qualify a pinned, licensed CPU package for this
harness. This is a bounded availability review, not a claim that no such code
exists or that reimplementation is infeasible. A local port needs explicit
float/double behavior, real-spectrum layout, startup/endpoints, supported hops,
window costs, buffer traffic and independent accuracy checks. It would be an
analysis-only candidate unless inverse/chain support were separately supplied.
These obligations answer overlap reuse rather than the current scheduling
attribution question; retain the literature discussion and defer implementation.

### FR-9 Evidence And Rationale

[Rafii's window kernels][rafii] make windowed overlap reuse a credible route;
they do not select or validate the underlying update recurrence. In particular,
windowing and blanket assertions that recursion is unstable are not valid
reasons to exclude it. [Lyons's 2023 author description][lyons-stable] supplies
a specific stable single-bin update candidate, building on Lyons and Howard
2021, with windowing discussion. Stability still does not substitute for our
finite-precision accuracy tests.

If reopened, start with that identified recurrence and an exact periodic-Hann
kernel, checking DC/Nyquist, phase convention and the extra neighboring bins
needed by windowing. For H>1 also assess [Park and Ko's hopping DFT][hopping],
which targets updates by a hop; computing every sliding spectrum and discarding
intermediate outputs is a different cost contract. Qualify source/licensing
before reuse and label any local reimplementation. Compare matched full-bin
and selected-bin subsets separately, charge all intervening updates and refresh
work, and retain long-stream weak-tone, post-signal silence, complex-bin and
thresholded phase-error evidence. This is a substantial follow-up experiment,
not a missing inverse-transform baseline.

### Consequences For The Current Paper

-   Keep the contribution a same-thread scheduling/implementation study with
    measured batch, hybrid and resumable controls. Inverse jobs and complete
    chains remain first-class required evidence; an analysis-only overlap
    method would not replace them.
-   Limit any cost/age frontier or superiority statement to the measured
    implementations and workloads. High-overlap smoke cases test coverage;
    they do not establish superiority over sliding, hopping or feedforward
    methods. No global best-transform, smallest-memory or universal real-time
    claim is supported by these deferrals.
-   Keep the existing overlap-reuse citations and complementary-experiment
    discussion. FR-12 must state that these alternatives were not measured.
    If the intended claims broaden, reopen FR-9 (and FR-8 where relevant)
    before freezing that broader campaign. A reviewer can require more evidence;
    this scope decision is not a publication-acceptance guarantee.
-   Proceed to FR-10, then the FR-11 pilot and confirmation campaign, then
    FR-12 integration. Resolve four-channel comparability, supported double
    workloads, report fixtures and provenance first. Do not replace this work
    with more library implementations or choose exclusions from favorable
    timing results. Future scope changes need their own recorded rationale.

Validation: local documentation links/anchors and referenced paths,
`make -C docs/whitepaper check`, and `git diff --check` passed. This decision
changes documentation only; no new DSP tests, plugin build, native adapter,
manual Rack session or timing campaign was performed.

## Functional Requirements

Implement the required FRs in order, completing each candidate's integration
and correctness checks before its benchmark wiring. FR-1 records completed
baseline work; FR-2 supplies shared protocol support. FR-3 through FR-6 are
required contenders/experiments. FR-7 through FR-9 retain the reserve and
academic scope above: record inclusion or deferral before FR-11, and complete
both steps for any included candidate. An unchecked optional FR does not block
measurement when its deferral and reason are recorded. Do not add candidates
mid-campaign based on favorable timings; later additions need a new campaign.

For each implementation FR, record files, dependency decisions, exact build
and verification commands, supported combinations, and results in this spec.
Benchmark completion means the workload, runner, checker, and smoke artifacts
work; it does not require a performance result. Shared contracts below apply
to every candidate without duplicating them in each checklist.

### FR-1: First-Party Transform And Streaming Baselines

#### Implementation And Integration

- [x] Retain the existing FFT/RFFT/IFFT and production analyzer controls; add
    batch/incremental periodic inverse jobs and complete identity/FIR chains.
- [x] Verify independent all-output references, normalization, release cadence,
    valid-output boundaries, and sample-delivery latency. Preserve the bulk
    preparation limitations described in First-Party Evidence Pathways.

#### Benchmark Implementation

- [x] Add isolated, periodic inverse, and complete-chain workloads, callback
    and throughput passes, explicit latency contracts, and artifact checks.
- [x] Supply the synthesis smoke configuration and factor profile; pass short
    replay/artifact checks. Evidence is recorded below. Final measurements
    for these controls remain part of FR-11.

### FR-2: Shared Adapter And Evidence Contracts

#### Implementation And Integration

- [x] Extend the existing adapter interface with explicit capability and
    latency contracts; keep dependencies optional and reject unavailable
    requested backends. Preserve ordinary plugin and standalone test builds.
- [x] Provide shared independent numerical fixtures and tolerances for the
    transform, analyzer, inverse-job, and complete-chain boundaries. Check
    startup, settings changes, output completeness, and time origins.

#### Benchmark Implementation

- [x] Update C++ dispatch, Python workload validation, and artifact checking
    together. Replace backend-name assumptions where external/hybrid contracts
    require it; opaque library calls have no radix-2 step-count claim.
- [x] Capture linked implementation identity, plans, setup/destruction,
    persistent/scratch storage, allocation behavior, and supported workloads.
- [x] Add regressions rejecting wrong scaling/layout, missing outputs, wrong
    publication age, unsupported configurations, altered dependencies,
    duplicate/missing runs, and invalid timing values.

### FR-3: Rack/PFFFT

#### Implementation And Integration

- [x] Pin the Rack/PFFFT implementation and integrate ordered real analysis
    and complex forward/inverse adapters. Include periodic inverse jobs and
    complete identity/FIR chains with explicit inverse normalization.
- [x] Inspect the pinned wrapper and PFFFT scratch behavior, document actual
    precision/size support, and pass the shared independent correctness checks.
    A direct PFFFT variant needs a specific scratch/layout question and remains
    part of this algorithm family.

#### Benchmark Implementation

- [x] Wire every supported boundary into the existing runner and checker,
    charging packing, ordering, normalization, and all required output stores.
- [x] Add matched control workloads and short smoke coverage, including setup,
    storage, linked dependency provenance, callback, and throughput paths.
    Verify artifacts without interpreting development timings as results.

### FR-4: FFTW3

#### Implementation And Integration

- [x] Add optional single-threaded float real-to-complex and complex
    forward/backward plans, then separate double support. Integrate analysis,
    periodic inverse jobs, and identity/FIR chains; verify packing and scaling.
- [x] Use `FFTW_MEASURE`, no imported wisdom, and fresh processes as the primary
    plan policy. Restore inputs after planning and pass independent checks.
    An `ESTIMATE` sensitivity study is a separate configuration.

#### Benchmark Implementation

- [x] Register matched workloads for each supported precision and boundary;
    capture plan creation/destruction, storage, build/SIMD options, linked
    library identity, and exported plan/wisdom information when available.
- [x] Add smoke/artifact coverage for plan policy, full output, normalization,
    latency, and unsupported combinations. Document dependency/build commands.

### FR-5: Apple Accelerate/vDSP

#### Implementation And Integration

- [x] Add macOS real and complex adapters with reusable setup, explicit native
    packing/scaling, and the matched analyzer, inverse-job, and filtering paths.
- [x] Verify each supported precision and transform boundary independently;
    make platform unavailability explicit. Record OS/SDK/framework identity
    and what cannot be independently rebuilt or hashed.

#### Benchmark Implementation

- [x] Register supported workloads, setup/storage accounting, and required
    conversion work in the existing runner/checker with matched controls.
- [x] Pass macOS smoke/artifact checks and unavailable-platform regressions.
    Keep the platform-library scope explicit in metadata and generated outputs.

### FR-6: Hybrid Scheduled Analysis

#### Implementation And Integration

- [x] Build the benchmark-only hybrid using Rack/PFFFT as the initial external
    FFT, so implementation does not depend on a timing pilot. Parameterize the
    adapter seam if practical; any additional library variant is named explicitly.
- [x] Spread preparation and postprocessing across the hop while executing the
    FFT as one indivisible call. Define the schedule, verify stage dependencies
    and exact H-1 publication age, and charge retained input and scheduling.
- [x] Match surrounding operations to the batch and production controls where
    possible; record arithmetic, layout, and storage differences. Legacy
    batch/incremental controls isolate scheduling within their arithmetic;
    legacy-versus-production comparisons do not isolate scheduling alone.
    If claiming production scheduling overhead causally, also implement batch
    execution of that same positive-bin pipeline before the measurement gate.

#### Benchmark Implementation

- [x] Add matched full-batch, hybrid, and resumable analyzer workloads and
    checker contracts. The indivisible FFT call has no constant-cost butterfly
    interpretation or work-count timing guarantee.
- [x] Pass numerical, cadence, and smoke/artifact checks across the planned
    workload range. Build the attribution report path now; measure it in FR-11.

### FR-7: KISS FFT (Optional)

Decision: DEFERRED for the current paper; see the
[optional contender decision](#optional-contender-decision).
Unchecked implementation work below is conditional, not a readiness blocker.

#### Implementation And Integration

- [x] Review the portable setup/storage question and record deferral.
- [ ] If reopened, pin the source/license/build configuration and integrate real
    and complex transforms plus matched streaming paths and independent checks.

#### Benchmark Implementation

- [ ] If included, add supported workloads, conversion/setup/storage accounting,
    provenance, and smoke/artifact checks under the same adapter contract.
    A forward-only adapter is not a completed library contender.

### FR-8: Garrido Feedforward STFT (Optional)

Decision: DEFERRED for the current paper; see the
[optional contender decision](#optional-contender-decision).
Unchecked implementation work below is conditional, not a readiness blocker.

#### Implementation And Integration

- [x] Review implementation availability/license, CPU feasibility, precision,
    supported hops/windowing, endpoint convention, and real-input support.
    Record inclusion or a concrete reason for deferral.
- [ ] If included, implement a matched overlap-reuse analysis path and verify
    numerical output, publication cadence, and long-stream stability. Clearly
    distinguish a local reimplementation from the authors' measured code.

#### Benchmark Implementation

- [ ] If included, register the supported analysis subset with matched controls,
    charging all overlap reuse, windowing, updates, output, and refresh work.
    Mark inverse/filtering boundaries unsupported unless actually implemented.
- [ ] Add long-stream error, phase-threshold, weak/strong-tone, and post-signal
    silence checks plus smoke/artifact coverage before final measurement.

### FR-9: Windowed Sliding/Hopping Transform (Optional)

Decision: DEFERRED for the current paper; highest-priority academic follow-up.
See the [optional contender decision](#optional-contender-decision).
Unchecked implementation work below is conditional, not a readiness blocker.

#### Implementation And Integration

- [x] Identify a specific update/stability route and assess its fit separately
    from Garrido's method. Record deferral; Rafii's window kernels alone do not
    select a transform. Source/license and supported-subset qualification remain
    prerequisites if the named candidate is reopened.
- [ ] If included, integrate and independently verify the chosen algorithm,
    window kernels, endpoint/cadence contract, and long-stream stability.

#### Benchmark Implementation

- [ ] If included, add matched supported analysis workloads and numerical/smoke
    checks, charging intermediate updates, window kernels, and periodic refresh.
    Distinguish computing every spectrum from exploiting hop H directly.
- [ ] Record unsupported boundaries and expose error/storage/cost/age output
    through the common runner/checker without a separate timing harness.

### FR-10: Campaign And Report Tooling

- [ ] Add tracked `external-smoke.json` and `external-pilot.json` configurations
    with explicit supported backends per host. Resolve and report workload
    counts, matched controls, independent channel counts, and family constraints.
- [ ] Implement the focused extension sweeps described below, including the
    hybrid, four-channel SIMD comparison, and supported double workloads.
- [ ] Supply deterministic table/figure generation with fixture-based checks
    for numerical, provenance, storage, workload, latency, and uncertainty
    outputs. Smoke data must remain clearly labeled and outside paper results.
- [ ] Record exact reproduction commands and supply the tooling evidence for
    the readiness gate below. Final counts/durations remain pilot decisions.

### FR-11: Final Metric Gathering

- [ ] After the readiness gate passes, prepare otherwise idle measurement
    hosts and record power/thermal conditions, host activity, toolchain,
    dependency identity, and session/order policy. Build before timed passes.
- [ ] Run and retain the focused pilot below for all included candidates and
    controls. Use variability, timer resolution, and tail-event counts to
    justify the final matrix, repetitions, session count, and duration.
- [ ] Freeze the confirmation configurations and exact commands, then run
    serially across independent sessions with all artifact checks passing.
    Preserve cases favoring batch and report missing hardware explicitly.
- [ ] Collect the required cost, tail, storage, error, completion/publication,
    and sample-delivery metrics. Generate validated reports without conflating
    workload families, time origins, or simulated deadlines with device data.

### FR-12: Paper Integration And Completion

- [ ] Integrate FR-11's generated tables/figures into the paper, pin the measured
    revision, and preserve historical campaigns and their interpretation.
- [ ] Report benefits, regressions, crossover regimes, numerical accuracy,
    confounds, uncertainty, and measured platform scope. Explicitly delimit
    unmeasured overlap-reuse alternatives under the optional contender decision;
    do not present a measured-subset frontier as a global optimum. Update citations
    consistently; an unfavorable supported result still satisfies this spec.
- [ ] Pass artifact checks, the paper build, and PDF review; record evidence
    here and archive the completed spec only after all required criteria pass.

## Shared Contracts

### Comparable Adapters

-   Extend the existing protocol rather than adding a parallel timing harness.
    Keep external dependencies optional for ordinary plugin/test builds.
    A requested unavailable backend must fail clearly; inventory output may
    label it unavailable. Never silently substitute another implementation.
-   Record supported precision, transform kind, sizes, channel count, output
    layout, thread policy, setup policy, and publication contract. Update the
    C++ dispatch, Python workload validation, and artifact checker together.
    The checker resolves explicit validated contracts from the archived registry;
    external and hybrid backends must declare their supported boundaries. Do not
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
-   Follow each backend's setup policy (including FR-4's FFTW planning policy
    and [planner flags][fftw-flags]). Record library versions, build/SIMD
    options, source and binary hashes, compiler/FP flags, and OS/SDK/framework
    identity. Hashing a wrapper alone does not identify its linked implementation.
    For opaque platform libraries, state what cannot be independently rebuilt
    or hashed.

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

## Implementation Readiness Gate

Before FR-11 starts, FR-1 through FR-6 and FR-10 must be complete, and each
optional FR must either pass both steps or have a recorded deferral. FR-7
through FR-9 now satisfy this decision requirement through the
[recorded deferrals](#optional-contender-decision); they are not implemented.
FR-10 remains required before measurement. Require independent correctness
checks, supported-host builds, verified smoke artifacts, resolved workload
inventories, explicit dependency/setup/storage/latency
contracts, and tested report generation. No speedup or final metric is needed
to pass this gate. A short run that emits timing fields validates mechanics;
its measurements cannot establish a ranking or enter the final paper.

Implement and test all selected algorithms and measurement paths before
preparing the clean measurement environment. Do not require an early pilot to
choose the hybrid library or unblock another implementation FR. If the final
pilot exposes a correctness/tooling defect, return to implementation, revalidate,
and restart affected measurements under a newly identified revision/configuration.
Do not mix pre-fix and post-fix observations into one confirmation campaign.

## Final Measurement Sequence

The following sequence belongs entirely to FR-11, after implementation:

1.  **Focused pilot:** Start with N=2048/4096/16384, H=1024, blocks of 16/64/256,
    48 kHz, float, one analyzer, steady state, smoothing off/on, and separate
    callback/throughput passes. Include the core, matched legacy controls,
    all included external candidates, and the implemented hybrid. Use a resolved
    configuration and report the workload count. Inspect timer resolution and
    session variability before choosing repetitions. Include inverse jobs and
    both overlap-save controls as separate families; do not combine their costs
    or time origins in a single ranking.
2.  **Focused extensions:** Use the implemented sweeps for N=128, H=257,
    96 kHz, 1/4/16 analyzers, aligned/staggered phases, fixed background load,
    and startup/live/cache pressure. Resolve valid family-specific combinations.
    Include callback-origin offsets where needed: analyzer staggering alone
    does not vary every relationship between the callback grid and frame
    schedule. Include equal four-channel work for `core-simd4` and a separate
    supported-backend double sweep. Any reduction from the planned matrix
    needs an explicit rationale; do not select only favorable results.
3.  **Frozen confirmation:** Include the full-batch/hybrid/resumable attribution
    comparison and repeat the frozen matrix across independent sessions. Use
    at least three sessions as an initial coverage floor, with final run lengths
    and counts justified by pilot variability and the tail events of interest.
    Repeat a focused portable subset on ARM64 and x86-64 before making
    cross-architecture claims. Missing hardware limits claims; it does not
    justify invented data.
4.  **Validated reporting:** Generate FR-10's tables/figures from checked final
    artifacts for FR-12. Preserve prior archives and distinguish pilot, smoke,
    and confirmation evidence. New academic/worker/host questions discovered
    here are follow-on campaigns, not prerequisites for this implementation.

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

- [x] FR-1: First-party inverse jobs and complete identity/FIR chains have
    batch and incremental controls, independent all-output validation, latency
    contracts, workload configurations, and raw-artifact checks. Publication
    campaigns and external comparisons remain outstanding.
- [x] FR-3 through FR-5: Three primary backend adapters pass independent
    numerical and matched analysis/inverse/complete-chain checks on supported
    hosts; unavailable cases are explicit. Required output normalization is
    included in cost.
- [ ] FR-2 and FR-10: Dependency/setup/storage/publication contracts and exact
    reproduction commands are documented. Archived artifacts identify measured
    sources, libraries, flags, workloads, and numerical checks without relying
    on HEAD.
- [ ] FR-2: Runner/checker regressions reject wrong scaling/layout, missing
    outputs, wrong publication age, unsupported configurations, altered
    dependencies, duplicate/missing runs, and invalid timing values.
- [ ] FR-11: A retained pilot justifies the final matrix, repetitions, session
    count, and observation duration. A frozen confirmation campaign follows the
    pilot; cases favoring batch processing remain in the reported matrix.
- [ ] FR-6 and FR-11: The hybrid comparison separates the practical value of
    suspending the FFT from scheduling the surrounding work, with remaining
    confounds stated.
- [ ] FR-10 and FR-11: Generated outputs include an
    implementation/provenance/error/storage table; matched workload/cost/age
    table; transform and full-analysis cost versus N; callback tail
    distributions; and cost versus spectrum-age plots. Inverse and
    complete-chain panels report release/completion and sample delivery latency,
    respectively, with their independent numerical evidence. Each output
    identifies its campaign and includes uncertainty or variation appropriate
    to that statistic. No upstream performance chart substitutes for these
    measurements.
- [ ] FR-12: The paper reports benefits, regressions, crossover regimes,
    accuracy, limitations, and measured platform scope. Its tables/figures and
    citation metadata are updated consistently; build, artifact checks, and PDF
    review pass. An unfavorable result still satisfies this spec if supported.

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

Run from the repository root with Python 3, Make, and the configured Rack SDK
and compiler. The commands below exist today; the shown runtime library path
assumes the default `../..` Rack layout. Substitute the configured SDK directory
for a different layout; Windows needs its DLL search path configured.

```shell
python3 -m unittest discover -s benchmark/paper -p 'test_*.py'
make test/dsp/test_spectrum_analysis
make benchmark-paper-build
DYLD_LIBRARY_PATH="../.." LD_LIBRARY_PATH="../.." .build/benchmark/rack/paper --verify
make -C docs/whitepaper check
git diff --check
```

The Python suite now compiles the host-independent synthesis verifier using
`CXX` (default `c++`) and C++11. It does not need Rack or Catch2 for that check.
The following first-party configuration exists now; use new directories:

```shell
python3 benchmark/paper/run.py .build/paper-inverse-smoke --config benchmark/paper/configs/synthesis-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 benchmark/paper/check.py .build/paper-inverse-smoke
python3 benchmark/paper/run.py .build/paper-synthesis-session-01 --profile synthesis --list
```

The dedicated smoke config contains 48 streaming and eight isolated inverse
workloads; the synthesis factor profile contains 216 streaming workloads.
Its controls require no analyzer settings and reject live-window/smoothing
options. Resolve feasible H/N combinations per family before measurement.

The implementation must add tracked `external-smoke.json` and
`external-pilot.json` under `benchmark/paper/configs/`, with explicit supported
backends per host. Those files do not exist yet. Once supplied, these are the
required implementation smoke and inventory commands; output directories must
be new. Listing the pilot resolves workloads without measuring them:

```shell
python3 benchmark/paper/run.py .build/paper-external-smoke --config benchmark/paper/configs/external-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 benchmark/paper/check.py .build/paper-external-smoke
python3 benchmark/paper/run.py .build/paper-external-pilot --config benchmark/paper/configs/external-pilot.json --list
```

Only in FR-11, after the readiness gate and host preparation, run the pilot:

```shell
python3 benchmark/paper/run.py .build/paper-external-pilot --config benchmark/paper/configs/external-pilot.json
python3 benchmark/paper/check.py .build/paper-external-pilot
```

Document dependency installation/build commands when versions and integration
are selected, and record actual host notes with each campaign. Before the final
campaign, add its frozen configuration and exact commands, including selected
repetition counts, run lengths, seeds, and session directories. Supply a
deterministic figure/table generator and its validation command as part of
implementation. Smoke checks are not publication measurements.

## Review Evidence And Remaining Work

### Requirement Breakdown

September 29, 2026: reorganized the work into FR-1 through FR-12 with separate
implementation/integration and benchmark implementation checklists. Retained
first-party baseline completion and optional candidate scope. Moved the pilot,
comparative sweeps, and confirmation measurements behind the implementation
readiness gate; the hybrid now starts with Rack/PFFFT without waiting for a
pilot. This planning change adds no adapters or new performance evidence.

Validation: local links/anchors, referenced paths and command definitions,
FR numbering and paired checklists, and `git diff --check` passed. No DSP
tests, Rack build/session, or measurement campaign was run for this
documentation-only change.

### FR-6 Hybrid Scheduled Analysis Completion

September 29, 2026: added benchmark-only `pffft-hybrid-float` and
`pffft-scheduled-batch-float`, parameterized over the native provider seam.
Only the Rack/PFFFT float variants are registered. Both execute the same
positive-bin arithmetic, cache updates, storage capacities and task dispatcher.
The ordinary PFFFT batch, legacy batch/incremental and production resumable
analyzers remain separately named controls. No plugin DSP or Rack module code
changed.

With K=N/2+1, W=N+1+2K dependency-ordered tasks comprise N input/window
stores, one opaque native call, K magnitude/prefix sums and K band/EMA/output
stores. The hybrid uses balanced quotient/remainder quotas and publishes at
jH+H-1; the paired batch completes at jH. The FFT (including provider packing
and positive-bin conversion) executes at offset `ceil((N+1)H/W)-1` in the
hybrid. N+H retained slots preserve unread frame input without a boundary
snapshot copy. Live window and band cache updates run inside their respective
tasks. H=1, nondividing hops, and H>W are supported. Native work remains
indivisible; these task counts are not butterfly counts or timing bounds.

The pair has identical persistent DSP state; the dispatch-only backend label
is discarded from its stored configuration so different name lengths do not
bias the C++ storage comparison. Metadata records task partition, native-call
count/offset, retained capacity and publication delay for each actual instance
and resource probe. The checker validates these fields and independent
all-output/cadence reports. C++ allocation evidence does not cover native
allocator activity or stack scratch; existing PFFFT limitations still apply.

A numerical preflight exposed float octave-boundary rounding changing with
compiler loop transformations: N=16384, H=37, bin 7021 selected lower bin
6254 in the bulk loop and 6255 in the per-bin task. External benchmark paths
now evaluate interval arithmetic in binary64, retaining float octave factors;
the independent oracle computes its intervals at the same precision and sums
bands independently. Magnitudes, window values, prefix sums and EMA retain
their existing precision. The original error tolerances were not relaxed.
Production/legacy interval arithmetic remains unchanged. Their differences in
FFT arithmetic, layouts, fusion, plan capacity and storage remain explicit
confounds; this stage makes no causal production-scheduling-overhead claim
and therefore does not add a same-production-pipeline batch ablation.

[The benchmark guide](../benchmark/paper/README.md#hybrid-scheduling-attribution)
defines the schedule and reproduction commands.
[The attribution generator](../benchmark/paper/hybrid_report.py) checks the
archived campaign before producing JSON and Markdown in a separate directory.
It requires all six matched controls, preserves process-level cost/tail
summaries, ages and resource records, and labels four comparison types.
It records campaign-metadata and generator hashes. It neither estimates FFT
cost by subtracting measurements nor treats callbacks as independent repeats.
Short smoke runs establish the reporting path, not comparative conclusions.

Validation on the current Apple Silicon host:

-   `python3 -m unittest discover -s benchmark/paper -p 'test_*.py'`: all
    27 tests passed. New checks cover every supported power of two, H=1/37/
    257/65536, retained-input wraparound, native-call placement, live caches,
    postprocessing dependencies, zero observed C++ execution allocations,
    deliberately corrupted retention, missing controls and altered metadata.
-   `make benchmark-paper-build PAPER_VDSP=1 PAPER_FFTW_PREFIX=.build/deps/fftw`
    and `DYLD_LIBRARY_PATH=../.. .build/benchmark/rack/paper --verify`: passed
    with all providers enabled. Hybrid preflight checks exact paired output,
    the ordinary batch control, and independent numerical references at
    N=128/2048/16384, including long idle quotas and live smoothing.
-   `python3 benchmark/paper/run.py .build/paper-fr6-hybrid-final --config benchmark/paper/configs/hybrid-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2`
    and `python3 benchmark/paper/check.py .build/paper-fr6-hybrid-final`: all
    120 runs passed. The matrix spans 20 matched conditions, all six controls,
    callback/throughput passes, startup, live settings, aligned/staggered banks,
    load/cache pressure and callback/sample-rate variations. Paired adapters
    have equal persistent C++ storage and zero observed execution allocations.
-   `python3 benchmark/paper/hybrid_report.py .build/paper-fr6-hybrid-final .build/paper-fr6-attribution-final`:
    generated and checked 20 groups and 80 explicitly qualified comparisons.
-   `make check-build`: five tests passed. `make -j2` and
    `make -C docs/whitepaper check` passed. Local documentation paths/links
    and `git diff --check` passed. Existing Rack/host-library compiler warnings
    remain; no interactive Rack session was run.

FR-6 is complete. Spec 004 remains in progress: optional candidate decisions,
matched SIMD/double comparisons, final evidence integration, and FR-11's
pilot/confirmation measurements remain separate stages.

### FR-5 Apple Accelerate/vDSP Completion

September 29, 2026: implemented all seven native workload boundaries in float
and double using optional macOS vDSP real/complex transforms. Each instance
reuses one exact-size radix-2 setup. Timed execution includes native split
packing, real-output factor-of-two correction, natural-order stores, and
normalized inverse output. The shared adapters supply analysis, periodic
inverse jobs, and complete overlap-save identity/FIR chains. Independent
all-output fixtures cover every supported power of two from 128 to 16384.
See [provider details](../benchmark/paper/providers/vdsp.md).

`--enable-vdsp` and `PAPER_VDSP=1` enable the runner and paper executables,
respectively. Non-macOS opt-in rejects before building; a disabled build
rejects requested vDSP workloads explicitly. Optional features can coexist,
and runner options override ambient Make feature variables. The provider uses
the narrow vDSP header to avoid unrelated Apple graphics types colliding with
Rack names. No production sources or module interfaces change.

Campaigns retain actual-instance setup/storage descriptions, loaded framework
image paths, OS product/build versions, compiler SDK/deployment macros,
SDKROOT and the default xcrun SDK version/build. The default SDK query is
labeled separately from the compiler macros. Apple's system framework cannot
be independently rebuilt here; a dyld-cache image path is not a standalone
binary hash. Native setup size, internal scratch, and native allocator counts
remain explicitly unknown. Zero observed C++ execution allocations is not a
claim about Apple's native allocation behavior or a hard real-time bound.

Validation on Apple Silicon/macOS:

-   `make benchmark-paper-build PAPER_VDSP=1` and
    `DYLD_LIBRARY_PATH=../.. .build/benchmark/rack/paper --verify`: passed,
    including both precisions and matched forward/inverse/complete chains.
-   `python3 -m unittest discover -s benchmark/paper -p 'test_*.py'`: all
    24 tests passed, including independent vDSP fixtures, C++ allocation
    checks, unavailable-platform rejection, and required platform evidence.
-   `python3 benchmark/paper/run.py .build/paper-fr5-vdsp --enable-vdsp --config benchmark/paper/configs/vdsp-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2`
    and `python3 benchmark/paper/check.py .build/paper-fr5-vdsp`: all 112
    native/control runs and archived evidence passed. This includes startup,
    smoothing/live analysis, staggered instances, load, and cache pressure.
-   A separate six-run combined-feature campaign passed with
    `--enable-vdsp --fftw-prefix .build/deps/fftw`: each provider ran float
    inverse transforms and complete FIR chains at N=128, H=37. Its resolved
    configurations, commands, dependencies, and sources are retained in
    `.build/paper-fr5-combined`; its artifact checker passed. This checks
    provider coexistence, not comparative performance.
-   `make -j2 PAPER_VDSP=1 PAPER_FFTW_PREFIX=.build/deps/fftw`: plugin build
    passed. Its compile/link commands contain no optional-provider flags;
    `otool -L plugin.dylib` shows neither FFTW nor Accelerate as a direct
    dependency. `make check-build`: all five build-isolation tests passed.
-   `make benchmark-paper-build` restored the default compiled inventory;
    it matched Python's disabled registry and rejected a requested vDSP job.
    Artifact rechecks of FR-3 and FR-4 campaigns also passed.
-   `make -C docs/whitepaper check`, local documentation link/path checks,
    and `git diff --check`: passed. Existing Rack header warnings remain;
    no interactive Rack session was run.

FR-3 through FR-5 are complete and remain entirely benchmark/research code.
These short campaigns establish implementation coverage, not a performance
ranking. FR-6's benchmark-only hybrid comparison is the next implementation
stage; publication pilot and confirmation measurements remain in FR-11.

### FR-4 FFTW Completion

September 29, 2026: implemented all seven external workload boundaries in float
and double using optional serial FFTW 3.3.10 static libraries. The primary
policy is `FFTW_MEASURE`, one thread, fresh processes, forgotten prior wisdom,
and restored inputs after planning. Every actual timed/resource instance
retains its own plan text; process-global wisdom is labeled separately.
The registered baseline does not silently use the low-level `ESTIMATE` option.
See [provider details](../benchmark/paper/providers/fftw.md).

`--fftw-prefix` enables the runner and matching generated C++ inventory;
`PAPER_FFTW_PREFIX` enables only paper-executable compilation/linking. Without
it, FFTW workloads reject explicitly and ordinary builds need no FFTW files.
The local build helper verifies the upstream archive and uses fresh source
and build directories on every invocation. It checks the extracted source
manifest after building and records library/header bytes, source archive,
configure commands, compiler/SIMD policy, and config/log evidence. Campaigns
archive that evidence and check dependency mutations. This ARM64 build uses
NEON for float; FFTW 3.3.10's double implementation here is scalar. Native
archives retain the host toolchain deployment target; this validation covers
the current macOS host, not older OS compatibility.

The wrapper preallocates its arrays, but inspected FFTW buffered execution
paths can allocate native scratch depending on the selected plan. Native
execution counts, internal scratch, and opaque plan storage remain explicitly
unknown to the C++ allocation audit. This is not an allocation-free claim.

Validation:

-   `python3 benchmark/paper/providers/build_fftw.py --jobs 2`: fresh pinned
    float/double builds passed, with source-tree integrity verified afterward.
-   `python3 -m unittest discover -s benchmark/paper/providers -p 'test_fftw.py'`:
    two tests passed, including 115,200 independent numerical bin checks and
    deliberate stale-source/object isolation. Common protocol and optional
    feature/dependency/plan-policy regression tests also passed.
-   `make benchmark-paper-build PAPER_FFTW_PREFIX=.build/deps/fftw` and the
    expanded executable `--verify`: passed. Rebuilding without that option
    restored the disabled inventory and rejected a requested FFTW workload.
-   `python3 benchmark/paper/run.py .build/paper-fr4-fftw --fftw-prefix .build/deps/fftw --config benchmark/paper/configs/fftw-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2`
    and `python3 benchmark/paper/check.py .build/paper-fr4-fftw`: all 112
    native/control runs, actual plan records, resource pairs, and archived
    dependency/source/build evidence passed.
-   `make check-build` and `git diff --check`: passed. Plugin sources and
    runtime dependencies are unchanged; no interactive Rack session was run.

FR-4 is complete. These are implementation smoke checks; final comparative
measurements remain deferred to FR-11.

### FR-3 Rack/PFFFT Completion

September 29, 2026: implemented seven float Rack/PFFFT operations in the
benchmark framework, covering complex/real forward, normalized complex inverse,
analysis, periodic inverse jobs, and complete overlap-save identity/FIR chains.
The provider uses ordered Rack wrappers and aligned native transfer buffers;
analysis writes K positive bins, while isolated RFFT reconstructs all N bins
for the existing full-complex control. Production module sources and plugin
link dependencies are unchanged. See [provider evidence](../benchmark/paper/providers/pffft.md)
for inspected revisions, hashes, native scratch formulas, and ABI limitations.

The common external driver audits all output values, cadence, startup, live
settings, band smoothing/EMA, and delivered samples. It retains metadata from
each actual timed instance and each resource-probe instance outside timing.
Transform setup scope is plans and owned output, with caller fixtures outside
the probe. Native PFFFT stack scratch is source-derived; opaque setup bytes and
compiler stack overhead remain unknown. Captured source identity alone does
not establish that an opaque linked SDK binary was rebuilt from that source.

Validation:

-   `PYTHONPATH=benchmark/paper python3 -m unittest test_run test_contracts test_synthesis test_external test_pffft`:
    passed, including provider fixtures at every power of two from 128 through
    16384, positive-bin bounds, normalization, non-Hermitian inverse, invalid
    operations/sizes, and rejection of NaNs hidden by error accumulation.
-   `make benchmark-paper-build` and the executable `--verify`: passed.
    Shared streaming preflight covers small, medium, and maximum sizes,
    non-dividing hops, wraparound, smoothing, and live controls.
-   `python3 benchmark/paper/run.py .build/paper-fr3-pffft --config benchmark/paper/configs/pffft-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2`
    followed by `python3 benchmark/paper/check.py .build/paper-fr3-pffft`:
    all 56 native/control runs and their resource pairs passed.
-   `make check-build` and `git diff --check`: passed. No interactive Rack
    session or publication-performance campaign was run.

FR-3 is complete. Smoke measurements establish executable coverage only;
statistical comparisons remain FR-11 work.

### Shared Adapter And Evidence Contracts Completion

September 29, 2026: FR-2 is complete. The canonical
[`backends.json`](../benchmark/paper/backends.json) registry feeds Python and a
generated C++ descriptor table. It declares 28 implemented backends and three
explicitly unavailable external families. Dispatch, capability validation,
resolved latency/output contracts, and schema-2 artifact checks use these
fields. Opaque calls do not inherit radix-2 step counts. Schema-1 artifacts
retain their original checker for compatibility.

Shared independent fixtures cover all-bin complex/real forward and complex
inverse transforms, analyzer startup and live window changes, analytical
inverse jobs, and direct time-domain identity/FIR output. Separate resource
probes record ordinary setup/destruction timings and instrumented C++
allocation counts, retained requested bytes, and observed peak growth.
Native allocators, allocator overhead, and stack scratch are explicitly
unknown; each provider must inspect those paths during integration. These
one-adapter probes are not comparative performance results. Campaigns retain
both binaries, the compiled registry, resolved contracts, resource reports,
loader identities, and SDK dependency bytes, with build/campaign mutation
checks and offline archive validation.

Validation performed:

-   `python3 -m unittest discover -s benchmark/paper -p 'test_*.py'`: 14 tests
    passed, including standalone C++11 synthesis/reference checks, malformed
    and unavailable capabilities, opaque transforms, incorrect scale/layout,
    missing output/coverage, wrong publication/playback age, changed dependency
    evidence, duplicate/missing runs, and non-finite/negative measurements.
-   `make benchmark-paper-build` and the expanded executable `--verify`:
    passed. All-bin direct-DFT fixtures and independent analyzer magnitudes
    supplement the 48 matched scalar configurations and synthesis fixtures.
-   `python3 benchmark/paper/run.py .build/paper-fr2-verified --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2`
    and `python3 benchmark/paper/check.py .build/paper-fr2-verified`: 129 runs
    and 129 pairs of resource reports passed. Each resolved workload agreed
    between C++ and Python. Artifacts are retained in that ignored directory.
-   `make test/dsp/test_spectrum_analysis`: 1,934,764 assertions in seven cases
    passed through the standalone build path. `make -j2`: plugin build passed;
    existing SDK deprecation/literal warnings remain. No interactive Rack
    session was run; no production source behavior changed.
-   `make -C docs/whitepaper check`, local documentation path checks, and
    `git diff --check`: passed. Historical paper results remain unchanged.

Next is FR-3, the ordered Rack/PFFFT adapters for forward, inverse, analyzer,
and complete-chain workloads. External integrations and publication campaigns
remain open; the overall spec remains in progress. Short smoke observations
establish tooling correctness, not a speedup or real-time deadline guarantee.

### Initial Review

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

[kiss-complex-api]: https://github.com/mborgerding/kissfft/blob/master/kiss_fft.h
[kiss-real-api]: https://github.com/mborgerding/kissfft/blob/master/kiss_fftr.h
[fd-stft]: https://oa.upm.es/88002/3/FD-STFT_2.pdf
[lyons-stable]: https://www.dsprelated.com/showarticle/1533.php
[hopping]: https://doi.org/10.1109/MSP.2013.2292891

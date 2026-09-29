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
FR-10's campaign/report tooling is implemented and smoke-validated. FR-11 is
complete: all 11547 confirmation processes passed across three prepared M1 Pro
sessions, with validated tables, figures, and matched attribution under
[confirmation evidence](#fr-11-confirmation-evidence). FR-12 manuscript
integration remains outstanding. The initial numerical acceptance issue,
provisional coverage, and versioned policy correction remain recorded under
[pilot evidence](#fr-11-pilot-evidence).
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

- [x] Add tracked `external-smoke.json` and `external-pilot.json` configurations
    with explicit supported backends per host. Resolve and report workload
    counts, matched controls, independent channel counts, and family constraints.
- [x] Implement the focused extension sweeps described below, including the
    hybrid, four-channel SIMD comparison, and supported double workloads.
- [x] Supply deterministic table/figure generation with fixture-based checks
    for numerical, provenance, storage, workload, latency, and uncertainty
    outputs. Smoke data must remain clearly labeled and outside paper results.
- [x] Record exact reproduction commands and supply the tooling evidence for
    the readiness gate below. Final counts/durations remain pilot decisions.

### FR-11: Final Metric Gathering

- [x] After the readiness gate passes, prepare otherwise idle measurement
    hosts and record power/thermal conditions, host activity, toolchain,
    dependency identity, and session/order policy. Build before timed passes.
- [x] Run and retain the focused pilot below for all included candidates and
    controls. Use variability, timer resolution, and tail-event counts to
    justify the final matrix, repetitions, session count, and duration.
- [x] Freeze the confirmation configurations and exact commands, then run
    serially across independent sessions with all artifact checks passing.
    Preserve cases favoring batch and report missing hardware explicitly.
- [x] Collect the required cost, tail, storage, error, completion/publication,
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
FR-10 now supplies the campaign/report tooling evidence below. Require independent correctness
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
- [x] FR-2 and FR-10: Dependency/setup/storage/publication contracts and exact
    reproduction commands are documented. Archived artifacts identify measured
    sources, libraries, flags, workloads, and numerical checks without relying
    on HEAD.
- [x] FR-2: Runner/checker regressions reject wrong scaling/layout, missing
    outputs, wrong publication age, unsupported configurations, altered
    dependencies, duplicate/missing runs, and invalid timing values.
- [x] FR-11: A retained pilot justifies the final matrix, repetitions, session
    count, and observation duration. A frozen confirmation campaign follows the
    pilot; cases favoring batch processing remain in the reported matrix.
- [x] FR-6 and FR-11: The hybrid comparison separates the practical value of
    suspending the FFT from scheduling the surrounding work, with remaining
    confounds stated.
- [x] FR-10 and FR-11: Generated outputs include an
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

FR-10 supplies tracked smoke, pilot, and extension manifests under
`benchmark/paper/configs/`. The
[protocol README](../benchmark/paper/README.md#external-campaigns-and-reports)
provides current variant-specific commands, dependency prerequisites and
resolved counts. Listing the pilot does not measure it. For the full macOS
variant, with the documented FFTW prefix already prepared:

```shell
python3 benchmark/paper/run.py --config benchmark/paper/configs/external-pilot.json --variant macos --enable-vdsp --fftw-prefix .build/deps/fftw --describe-matrix
```

Only in FR-11, after the readiness gate and host preparation, run the pilot
using explicit host/session identities, observation lengths, and host notes.
Its retained evidence determines the frozen confirmation settings.

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

## FR-10 Implementation Evidence

September 29, 2026: Campaign and reporting tooling is implemented under
`benchmark/`, with no production DSP, module, UI, or plugin dependency changes.
The [protocol commands](../benchmark/paper/README.md#external-campaigns-and-reports)
define reproduction, host variants, evidence phases, and statistical limits.

-   Generated smoke/pilot/extensions manifests resolve 156/270/537 workloads
    for `rack`, 206/339/706 for `portable`, and 256/408/875 for `macos`, before
    repetition. These are inventories, not publication measurements. Missing
    requested optional libraries fail; provider omissions are explicit.
-   Extension paths include the hybrid, four independent channels, supported
    double workloads, callback-origin offsets, non-divisible hops, sample rate,
    callback size, load, cache, startup/live changes, and analyzer alignment.
    The new SIMD and scalar banks consume identical independent channel bytes
    and validate every published channel/bin against independent references.
    Existing scaled-lane controls retain their separate meaning.
-   The v2 invocation carries a common callback offset independently of analyzer
    staggering. v1 and archived configurations retain zero-offset semantics.
    Startup and isolated transforms reject incompatible offsets.
-   Reports validate archived artifacts before creating workload/cost/age/error/
    storage tables, implementation/provenance tables, cost-versus-N figures,
    per-process callback CDFs, and family-specific cost/age panels. They retain
    raw-data hashes, timing summaries, native plans, numerical evidence, counts,
    observation windows, and generator/output hashes.
-   Host/source/dependency/build strata, precisions, operations and independent
    channel contracts remain separate. Session means receive equal weight;
    observed session ranges are descriptive, not confidence intervals.
    Confirmation reports require at least three labeled sessions per workload;
    duplicate workloads in one session and mixed evidence phases fail.
-   Native allocation unknowns remain unknown. Existing scalar core/legacy
    analysis has preflight numerical coverage rather than per-run error reports;
    those report cells explicitly remain unavailable. New independent banks
    and external analysis retain all-output numerical replay evidence.

Validation performed on Apple Silicon/macOS:

-   `.build/paper-report-env/bin/python -m unittest discover -s benchmark/paper
    -p 'test_*.py'`: all 34 tests passed, including host variants, missing-feature
    rejection, equal-channel counts, offset cadence, provenance separation,
    session weighting, phase rejection, and byte-identical fixture figures.
    The four report tests passed again after final reporting refinements.
-   `python3 benchmark/paper/campaigns.py --check`: passed. The README's
    `--describe-matrix` command resolves 408 pilot workloads; the same command
    with `external-extensions.json` resolves 875. These commands only list work.
-   The exact 256-workload smoke command in the protocol README passed,
    including the forced optional-provider benchmark builds and native
    preflight. Artifacts are retained in `.build/paper-fr10-smoke` with 109
    analysis, 38 inverse-job, 76 complete-chain, and 33 transform runs.
    Numerical replay records 1464 publications and 364111 checked output values,
    including 20352 playback samples. First-party transform selected-bin checks
    have their own reports and are additional to these replay counts.
-   `python3 benchmark/paper/check.py .build/paper-fr10-smoke`: verified all 256
    runs, archived sources/dependencies, summaries, contracts, resources and
    hashes. The same command on `.build/paper-fr6-hybrid-final` verified its 120
    historical v1 runs without rewriting that archive.
-   `.build/paper-report-env/bin/python benchmark/paper/report.py
    .build/paper-fr10-smoke --output .build/paper-fr10-report-final --phase smoke`:
    generated 256 evidence rows and 75 SVG/PNG figure pairs. Representative
    analysis, independent-channel, inverse, complete-chain and cost-versus-N
    panels were visually inspected. Data and plots prominently remain SMOKE;
    none were copied into manuscript results. The report records its final
    generator hash and plotting environment independently of campaign sources.
-   `make check-build`: all five build-isolation tests passed. `make -j2` built
    the Rack plugin. `make -C docs/whitepaper check` passed the historical
    artifact, citation, numerical-table and plot checks. Documentation links
    and `git diff --check` passed. No interactive Rack session was performed.

Existing SDK deprecation warnings and FFTW's macOS deployment-target linker
warnings remain. Optional plotting dependencies emit upstream pyparsing
warnings but tests and figure generation pass. Native execution was checked
on this ARM64 macOS host only; other host variants were resolved and validated
as configurations, not measured or rebuilt on other hardware.
No pilot, confirmation measurement, cross-architecture result, or paper ranking
is produced by this stage. FR-11 must select durations/repetitions from its
retained pilot and run on prepared hosts. FR-12 integrates final evidence.

## FR-11 Pilot Evidence

September 29, 2026: FR-10 was committed as `1f88307`. The user requested that
FR-11 start while the laptop was otherwise idle. The host is an Apple M1 Pro
with 10 logical CPUs and 16 GiB RAM, running macOS 26.6.2 (25G83), on AC power
at 100% battery, with Low Power Mode off. `pmset -g therm` reported no recorded
thermal/performance warnings; that is not a continuous temperature trace.
Codex and ordinary OS services remain active. No CPU affinity, real-time
priority, or system power setting was changed. `caffeinate -i` prevents idle
sleep only for the campaign. Preparation snapshots, including process activity,
are retained in `.build/paper-fr11-host/pre-pilot.json`.

### Initial Pilot And Numerical Gate

The full macOS matrix resolved 408 workloads. The initial pilot used three
fresh-process repetitions, 128 measured hops/transform frames, 64 warmup hops,
and seed 20260929. `.build/paper-fr11-pilot-01` retains four completed runs and
the failed fifth invocation; its status remains incomplete and none of its
observations enter a report. The failure occurred for scheduled batch PFFFT
analysis at N=16384, H=1024, B=256, smoothing off.

Investigation identified two separate issues:

1.  The binary32 analysis reference itself lost weak-bin precision. At input
    endpoint 15360, bin 4040, the direct DFT magnitude was about 0.697928914;
    the float oracle gave 0.697670519 and PFFFT gave 0.698003888. An executable
    direct-DFT regression failed before correcting the oracle and passed after.
    Large analysis references now use a binary64 FFT on the same already rounded
    frame bytes; small sizes retain direct sums. Measured provider arithmetic
    and the existing acceptance tolerance are unchanged. The 34-test suite and
    full native preflight pass after this correction.
2.  With the corrected reference, actual pointwise failures remain in native
    float PFFFT and vDSP for unsmoothed N=16384. At endpoint 39936, bin 8136,
    PFFFT returned 0.758558571 against direct DFT 0.758989308: absolute error
    about 0.000430737, exceeding the unchanged 0.0003 weak-bin threshold.
    Scheduled and immediate PFFFT had identical frame bytes and output here.
    This is distinct from the repaired reference error.

An untimed diagnostic survey ran each external provider for 220000 samples
at N=2048/4096/16384, H=1024, with smoothing off/on. It retains executable
source and CSV under `.build/paper-fr11-host/numerical-survey.*`. All checked
cases at 2048/4096 and smoothed 16384 passed the pointwise threshold. Among
1761495 checked unsmoothed N=16384 bins per provider, PFFFT and vDSP each had
three violations; FFTW had none. Maximum pointwise errors scaled by
`max(1, abs(reference_bin))` were 0.000430763, 0.000310481, and 0.000231147,
respectively. Maximum absolute errors divided by the largest reference
magnitude were approximately 1.19e-7, 1.79e-7, and 1.19e-7. These are diagnostic
error measurements on this signal, not general error bounds or speed rankings.

The benchmark gate remains strict. Do not silently increase its tolerance,
claim native algorithms are exact, or treat normwise agreement as a pointwise
pass. A justified, documented numerical acceptance policy and stronger
large-size/weak-tone validation are required before the full confirmation
matrix can proceed. First-party scalar controls still have their previously
stated preflight-only numerical coverage; this finding establishes no accuracy
advantage for them.

### Provisional Pilot Coverage

To collect useful pilot evidence without choosing providers based on success,
the whole matched group of 32 unsmoothed N=16384 analysis workloads is withheld,
including providers/controls that did not fail. All 376 remaining workloads
retain their original settings, including inverse jobs, identity/FIR chains,
isolated transforms, and smoothed N=16384 analysis. The exact eligible and
withheld arrays are retained in `.build/paper-fr11-host/pilot-eligible.json`
and `pilot-withheld.json`. This provisional reduction is a correctness-gate
response, not a finalized publication matrix. It cannot close FR-11.

The corrected sources, including uncommitted oracle changes, are archived and
hashed by the new campaign; the failed and corrected campaigns are never pooled.
The invocation from the repository root is:

```shell
caffeinate -i python3 benchmark/paper/run.py .build/paper-fr11-pilot-02 --config .build/paper-fr11-host/pilot-eligible.json --enable-vdsp --fftw-prefix .build/deps/fftw --phase pilot --host-id m1-pro-16gb-local --session-id fr11-pilot-02 --repeats 3 --hops 128 --frames 128 --step-frames 2 --warm-hops 64 --seed 20260929 --notes 'Use the full host, numerical-gate, and provisional-exclusion notes retained in metadata.json'
```

The final argument above abbreviates human-readable notes; the actual
campaign metadata retains their complete text. Counts/durations are initial
pilot choices, not a precision guarantee. Session variation and confirmation
settings remain unresolved until the correctness issue is resolved and the
remaining pilot coverage is complete.

### Provisional Pilot Results And Next Gate

The corrected provisional campaign completed all 1128 runs (376 workloads,
three process repetitions each). The report generator validated the campaign
before emitting 376 evidence rows and 65 SVG/PNG figure pairs:

```shell
.build/paper-report-env/bin/python benchmark/paper/report.py .build/paper-fr11-pilot-02 --output .build/paper-fr11-pilot-02-report --phase pilot
```

Representative analysis, inverse-job and full-chain figures were visually
checked. The campaign retains raw observations, numerical reports, binary and
source/dependency archives, native resource measurements and execution order.
The report remains PILOT and is not integrated into manuscript results.
`pilot-statistics.json` and a checksum manifest in `.build/paper-fr11-host`
retain descriptive summaries and diagnostic artifact identities.

Within this one session, the median workload ratio of maximum to minimum
process mean cost was 1.01768; the 90th percentile was 1.11543 and the largest
was 1.31722 (vDSP RFFT, N=4096). These ratios describe three process repeats,
not confidence intervals or independent session variation. Empty timer
controls had median zero, p95/p99 of 42 ns, and an observed maximum of 45625 ns
across the campaign. Quantization and occasional interruptions remain in the
raw evidence; no timer correction or outlier removal was applied.

Each callback process observed 8192, 2048, or 512 callbacks for blocks of
16, 64, or 256, respectively, spanning 131072 simulated engine samples
(about 2.731 seconds at 48 kHz). The largest blocks therefore supply only
about five observations in the upper 1%; these runs cannot substantiate a
precise p99 or rare-event maximum. A candidate confirmation length of 4096
hops would produce 16384 observations even at B=256 (about 164 upper-1%
observations), but it is not yet frozen or justified for rarer tails.

Post-campaign power remained AC/100%, and macOS still reported no recorded
thermal/performance warnings. The snapshot is retained in `post-pilot.json`.
No continuous thermal instrumentation or external x86-64 measurement exists.
No further benchmark is running at the end of this work.

The next gate is to resolve and document numerical acceptance for strong/weak
spectra at large N, preserving pointwise errors even if an independently
justified normwise criterion is considered. Then restore the full matched
pilot, execute the focused extensions, assess variation across genuinely
separate sessions, and freeze confirmation lengths/repetitions. Three
independent sessions remain the minimum coverage floor; process repeats here
cannot satisfy it. FR-11 remains IN PROGRESS and no confirmation run has been
started.

## Numerical Policy Resolution

September 29, 2026: committed the preceding pilot/reference progress as
`1c96df6`, then resolved the acceptance-policy mismatch with a versioned,
benchmark-only spectrum-level contract. The old pilot failures and provisional
exclusions above describe the earlier policy and remain preserved.

The [numerical acceptance document](../benchmark/paper/numerical-accuracy.md)
derives the distinction between pointwise and vector-relative error, cites
benchFFT's primary methodology, specifies exact equations and scope, and
provides native reproduction commands. Per published channel, both relative
L2 and Linf errors must satisfy the existing analysis tolerance (`3e-4` float,
`1e-10` double). Quiet signals have no absolute unit floor, and an exactly zero
reference requires exactly zero output. Different frames/channels never share
a denominator. This deliberately replaces the old pointwise acceptance
contract; it does not claim a per-bin relative-error guarantee.

The pipeline's analysis tolerance, rather than the tighter isolated FFT
budget, is appropriate because prefix sums, bands and EMA also introduce
roundoff. An exploratory check with the isolated float budget exposed this
distinction on smoothed startup/strong-weak fixtures. No new tolerance was
fitted to the pilot's failing bins; the previously specified analysis budgets
are retained. These are engineering limits, not proven error bounds.

The old pointwise threshold is still evaluated for every bin, and its failure
counts, largest scaled error and worst endpoint/channel/bin/actual/reference
values remain in numerical reports. Generated CSV tables expose the norm
maxima and pointwise counts. New campaign metadata and archived source
identity require the new policy diagnostics; older archives retain their
original validation meaning. Synthesis, isolated-transform, cadence,
normalization, output-count and native-layout checks remain unchanged.

Validation before restoring the full pilot:

-   The 36-test Python suite passed, including new rejection cases for wrong
    gain/layout, missing weak tones, NaN, nonzero silence, deletion of quiet
    signals and cross-frame/channel masking. Checker mutations reject missing
    or altered policy, tolerance, norms, counts and pointwise diagnostics.
-   All 90 native long-stream/dynamic-range cases passed with PFFFT, FFTW and
    vDSP, including both supported precisions, N=128/2048/4096/16384,
    smoothing off/on, amplitudes 1 and 1e-6, and silence after strong/weak tones
    plus seeded noise. Selected large oracle bins were independently checked
    with direct sums. `.build/paper-numerical-policy/streams.jsonl` retains
    the individual case reports; this is an untimed correctness experiment.
-   Those native cases checked 17771874 float values in 2658 spectra and
    11847916 double values in 1772 spectra. Worst relative L2/Linf errors were
    1.56271e-4/2.17885e-4 for float and 6.06915e-15/5.35462e-15 for double.
    The float diagnostic retained 516664 old pointwise violations across all
    these repeated, adversarial checks. Passing the norm gate does not erase
    that limitation; no uniform weak-bin accuracy is inferred.

No production DSP, Rack module, provider algorithm, packing, precision, or
measured pipeline arithmetic changed. Corrected campaign binaries and sources
are retained separately from all earlier pilots.

### Restored Full Pilot Validation

The 15-workload `.build/paper-numerical-policy-channels` smoke campaign passed
all artifact checks. It covers every independent four-channel backend at
N=16384, H=257, with smoothing off/on and live cache changes. This establishes
that the new policy also works at the SIMD/bank seam; its timings are smoke
evidence only. The negative-policy fixture was additionally rerun under
`-O3 -funsafe-math-optimizations`, matching the native benchmark math policy.

The full macOS pilot was then restored without exclusions and rerun serially:

```shell
caffeinate -i python3 benchmark/paper/run.py .build/paper-fr11-pilot-03 --config benchmark/paper/configs/external-pilot.json --variant macos --enable-vdsp --fftw-prefix .build/deps/fftw --phase pilot --host-id m1-pro-16gb-local --session-id fr11-pilot-03 --repeats 3 --hops 128 --frames 128 --step-frames 2 --warm-hops 64 --seed 20260929
.build/paper-report-env/bin/python benchmark/paper/report.py .build/paper-fr11-pilot-03 --output .build/paper-fr11-pilot-03-report --phase pilot
```

The actual invocation also supplied detailed `--notes`, retained in metadata,
covering the policy correction, native checks, host identity, prior power/
thermal snapshots, active Codex/OS services, and session limitations. All 1224
runs across all 408 workloads completed. Forced native benchmark builds,
preflight, numerical/cadence/resource checks, source/dependency archives and
report validation passed. The report contains 408 evidence rows and 68 SVG/PNG
figure pairs; all 140 generated artifact hashes passed. A restored unsmoothed
N=16384 comparison panel was visually inspected.

Audited external/scheduled analysis in the restored pilot had maximum relative
L2/Linf errors of about 2.343e-7/2.382e-7. All 96 old pointwise violations remain
in the report tables: 24 for each of PFFFT immediate, PFFFT scheduled batch,
PFFFT hybrid and vDSP, and zero for FFTW. These are counts across repeated
replays, not independent numerical events. Quiet-bin accuracy is still a
reported limitation, rather than a failure silently removed from the evidence.

The current checker also verified all 1128 historical provisional-pilot runs
under their original policy. Manuscript artifact checks, local documentation
links and `git diff --check` passed. No new Rack plugin build or interactive
Rack session was needed for these benchmark-only changes.

The numerical acceptance issue is resolved for the supported measured matrix,
and all provisional exclusions are lifted. FR-11 remains IN PROGRESS: focused
extensions, independent-session variability, frozen confirmation settings and
confirmation campaigns remain. The restored pilot is not pooled with older
sources/policies and is not substituted for confirmation. No benchmark remains
running at the end of this work.

## FR-11 Confirmation Freeze

September 29, 2026: committed numerical-policy resolution as `ef7beca`.
The complete 875-workload extension pilot then passed all 2625 runs and
artifact checks in `.build/paper-fr11-extension-pilot`. Its preparation
snapshot is `.build/paper-fr11-host/pre-extensions.json`: M1 Pro/16 GiB,
macOS 26.6.2, AC power at 100%, Low Power Mode off, no recorded thermal or
performance warnings. No CPU affinity or real-time scheduling is imposed;
macOS scheduling, core selection, frequency and ordinary services remain
possible timing confounds. Warning snapshots are not temperature traces.

The restored primary pilot's median/p90/worst within-session max/min process
mean-cost ratios were 1.01072/1.05975/4.18315; the extension pilot's were
1.01307/1.05992/3.69148. Large outliers remain in the data. Their presence
supports longer windows, randomized order, three process repeats and separate
measurement sessions, rather than trimming results or reporting tight error
bars. Timer p99 was 42 ns throughout both pilots; quantization matters for
very short calls and is never mechanically subtracted.

The frozen [plan](../benchmark/paper/configs/external-confirmation-plan.json)
retains all 408 primary and all 875 extension workloads, partitioned solely by
observation-length requirements. Its four configuration hashes are fixed before
confirmation. Each of three sessions uses three process repeats, 64 warmup
hops, 1024 isolated-transform frames, and these measured streaming lengths:

| Group | Workloads | Hops | Callback Observations Per Process | Interpretation |
| --- | ---: | ---: | --- | --- |
| Primary | 408 | 2048 | 8192/32768/131072 for B=256/64/16 | Same 2097152-sample window across primary block sizes; descriptive p99 and observed maxima |
| Standard extensions | 719 | 512 | 8192 at H=1024; 2056 at H=257 | Cost/age/accuracy sensitivities; smaller tail sample where stated |
| Short-hop extensions | 110 | 4096 | 2368 at H=37, B=64 | Raises pilot's 74 callbacks to an interpretable empirical distribution; no precise rare-tail claim |
| Single-sample extensions | 46 | 64 | 65536 at H=1024, B=1 | Ample per-sample observations without excessive raw-data volume; only 64 frame jobs per process |

Throughput passes use the same engine-sample windows as their callback peers
within a group. Callback rounding remains explicit in each resolved config.
The single-sample group has fewer FFT job releases despite many callback
observations; p99 cannot characterize its sparse FFT bursts. Preserve maxima
and full distributions but make no p99.9, WCET, device-underrun, or precise
rare-event claims. Compare maxima only with observation-window qualifications.
The frozen settings improve empirical coverage; they do not prove convergence.

All 1283 configurations are unique and passed runner inventory validation.
There are 3849 process runs per session and 11547 in the full confirmation.
No workload or unfavorable provider is omitted. The primary and extension
pilots remain separate evidence because confirmation introduces frozen-config
source bytes and longer windows.

Each session is a separately prepared measurement block, not a relabeled
process repeat. Before each block, wait at least 180 seconds without building,
testing or benchmarking, then record power, thermal-warning and process/load
snapshots. Run four campaigns serially with fresh forced builds, numerical
preflight and resource probes before timing. Rotate group order as recorded
in the plan, and use independent seeded process shuffles. Session s (1-based),
group index g in its recorded order (0-based), uses seed `20261000+100*s+g`.
Host identity is `m1-pro-16gb-local`; session labels are
`fr11-confirmation-01`, `-02`, and `-03`.

All sessions are on the same laptop and day, without reboot or independent
machine/day replication. Temporal separation, independent preparation and
randomization reduce shared transient effects but do not prove statistical
independence. Report observed session ranges, not inferential confidence
intervals. No x86-64 hardware is available here, so cross-architecture claims
are explicitly out of scope. This host limitation does not justify fabricated
portable measurements.

For each group, substitute its frozen config, hop count, session label and
seed into this command from the repository root (metadata retains full notes):

```shell
caffeinate -i python3 benchmark/paper/run.py .build/paper-fr11-confirmation-SS-GROUP --config benchmark/paper/configs/external-confirmation-GROUP.json --enable-vdsp --fftw-prefix .build/deps/fftw --phase confirmation --host-id m1-pro-16gb-local --session-id fr11-confirmation-SS --repeats 3 --hops GROUP_HOPS --frames 1024 --step-frames 2 --warm-hops 64 --seed SESSION_GROUP_SEED --notes 'Actual preparation, activity, power and session context'
```

The full command list, invocations, timestamps and preparation snapshots are
retained under `.build/paper-fr11-confirmation-control`. Sources, configs and
dependencies remain unchanged throughout confirmation; owning-spec progress
updates are outside the benchmark source archive. Only after all campaigns
pass their checks will confirmation tables, scientific figures and matched
hybrid attribution be generated for FR-12.

## FR-11 Confirmation Evidence

September 29, 2026: all 12 frozen campaigns completed and passed the runner's
full artifact checker. All 1283 configurations have three process repeats in
each of three separately prepared sessions: 11547 confirmation processes,
with no failed campaign, excluded workload, replacement measurement, timer
subtraction, or outlier removal. Primary and extension pilots remain separate.

| Session | Campaign Start (UTC) | Last Archive Check Complete (UTC) | Process Runs | Preparation |
| --- | --- | --- | ---: | --- |
| `fr11-confirmation-01` | 14:22:16 | 15:01:38 | 3849 | More than 180 s since the completed extension pilot; snapshot at 14:22:03 |
| `fr11-confirmation-02` | 15:05:42 | 15:45:06 | 3849 | Recorded quiet interval 220.35 s |
| `fr11-confirmation-03` | 15:49:12 | 16:28:37 | 3849 | Recorded quiet interval 216.99 s |

All pre/post snapshots record AC power at 100%, Low Power Mode off, and no
recorded thermal/performance warning. These are point observations, not
continuous temperature or frequency measurements. Load averages, ordinary
process activity, power settings, exact invocations, and start/end timestamps
are retained in `.build/paper-fr11-confirmation-control`. Default macOS
scheduling/core selection and ordinary services remain confounds.

Across the 1283 workloads, the median/p90/worst ratio of largest to smallest
session mean cost was 1.0122/1.0688/3.7010 (p90 uses the retained sorted
empirical order statistic). Typical variation was modest, but several severe
outliers materially affect means and rankings. Their cause is not identified
by these snapshots. They remain in every report; neither means nor observed
maxima should be presented without the process/session evidence.

Every campaign has the same source/dependency/build-policy provenance. The
measured revision is `ef7beca` plus the five frozen configuration files linked
above; full archived source hashes, not the revision alone, identify the
experiment. Twelve fresh timing builds and twelve audit builds each have their
own retained binary identity. This accounts for the actual rebuilt artifacts
without assuming identical binary bytes or claiming controlled address layout.
The campaigns are `.build/paper-fr11-confirmation-SS-GROUP`, where `SS` is
`01`, `02`, or `03` and `GROUP` is `primary`, `extensions`, `short-hop`, or
`single-sample`. Do not pool them with the earlier pilots.

### Numerical And Timer Evidence

All independent preflights and per-run checks passed. There are 9765 per-run
numerical reports; 1782 transform runs retain preflight-only coverage, explicitly
identified in the generated evidence. Of the per-run reports, 2970 contain
float analysis norms and 288 contain double analysis norms. Maximum observed
analysis relative L2/Linf errors were 6.881e-7/1.576e-6 for float and
5.943e-15/6.018e-15 for double. The 131184 old pointwise threshold violations
remain diagnostic evidence; normwise acceptance does not promise uniform
accuracy in weak bins. These counts include repeated measurements of the
same deterministic signals and are not independent error events.

Timer p99 remained 42 ns. It is retained alongside every process and is not
subtracted from measured costs. A callback p99, an observed maximum, and a
simulated budget exceedance are distinct descriptive statistics; none is a
WCET guarantee or a measured device underrun.

### Matched Scheduling Attribution

The retained attribution analysis contains 50 matched six-backend workloads
and 200 comparisons, each with all three sessions and three process repeats.
It uses FR-6's `hybrid_report.assemble` to retain original process-level pairs,
contracts, and storage evidence. Across-session cost ratios divide the two
matched session means, then report their mean and observed range. Tail ratios
use the median of each session's three process p99 values. No callback is an
independent statistical replicate. Cross-workload summaries below describe
this finite matrix, not a population or inferential confidence interval.

For the 18 primary callback workloads (three N, three B, smoothing off/on),
the mean-of-session cost-ratio median and workload range are:

| Numerator / Denominator | Median Cost Ratio | Workload Range | Lower Cost In All Three Sessions |
| --- | ---: | --- | ---: |
| Hybrid / matched scheduled PFFFT batch | 0.8661 | 0.7721--1.1159 | 16/18 |
| Hybrid / ordinary PFFFT batch | 2.0824 | 1.4653--2.9331 | 0/18 |
| Hybrid / core analyzer | 0.5946 | 0.5407--0.7221 | 18/18 |

Hybrid/scheduled-batch p99 ratios were below one in all three sessions for all
18 workloads; their across-workload median was 0.1690 and range
0.1229--0.3469. Hybrid/ordinary-batch p99 ratios had median 0.5316, range
0.3634--1.1121, and were below one in every session for 16/18 workloads.
Hybrid/core p99 ratios had median 1.1292, range 0.6140--3.9104, and were below
one in every session for only 6/18 workloads. Thus lower average hybrid cost
than the core does not imply lower callback tails. Short-hop callback results
also retain a hybrid/scheduled-batch mean cost ratio above one (1.1217).
The extension and throughput tables retain all other supported cases.

The matched scheduled PFFFT pair shares arithmetic, caches, storage, and task
dispatch; it measures quota execution and work placement. Ordinary PFFFT batch
uses different bulk loops and shorter retained storage. The core uses different
arithmetic/layout and maximum-size plans. Its comparison with the hybrid does
not isolate scheduling overhead or prove a universally superior FFT algorithm.

### Representative Inverse And Complete-Chain Results

These examples fix N=4096, H=1024, B=64, 48 kHz, float, one analyzer, steady
state, no background load, and smoothing off. Cost is the mean of the three
session means in ns/engine sample. P99 ranges are the observed range across
session medians of three per-process p99 values, in microseconds. Full tables
retain each process, session variation, and the other N/B/extension settings.

| Workload / Backend | Cost | Session P99 Range (us) | Completion Delay (samples) | Delivery Delay (samples) |
| --- | ---: | --- | ---: | ---: |
| Analysis / core | 52.816 | 4.834--4.834 | 1023 | N/A |
| Analysis / ordinary PFFFT | 14.772 | 9.500--9.667 | 0 | N/A |
| Analysis / matched scheduled PFFFT batch | 36.364 | 32.000--32.042 | 0 | N/A |
| Analysis / PFFFT hybrid | 31.258 | 5.416--5.458 | 1023 | N/A |
| Inverse / first-party batch | 78.502 | 77.792--78.333 | 0 | N/A |
| Inverse / first-party incremental | 72.473 | 10.166--10.542 | 1023 | N/A |
| Inverse / PFFFT | 12.574 | 8.208--8.208 | 0 | N/A |
| FIR chain / first-party batch | 151.474 | 149.875--150.709 | 0 | 1023 |
| FIR chain / first-party incremental | 151.966 | 17.000--17.375 | 1023 | 2046 |
| FIR chain / PFFFT | 25.580 | 20.709--20.750 | 0 | 1023 |

Completion is measured from the family-specific frame endpoint/job release;
chain delivery includes the stated buffering contract. Analyzer spectrum-center
age additionally includes (N-1)/2 samples. These origins are not interchangeable.
The inverse example favors native PFFFT batch on cost, p99, and completion delay
relative to the first-party incremental inverse. The FIR example buys a modest
p99 reduction relative to native batch at substantially higher average cost and
1023 additional samples of output delay. It sharply reduces p99 relative to the first-party
batch control while providing essentially the same average cost. These are
representative tradeoffs, not an assertion that every matrix case has the same
ordering. All unfavorable results and remaining algorithm/storage confounds
must survive FR-12 integration.

### Reproduction And Retained Outputs

Run from the repository root with the existing Matplotlib environment after
all 12 campaign archives have passed. The report command validates raw
artifacts again and rejects missing sessions, mixed source/dependency strata,
or duplicate workloads. Output directories must be new; use a different
output path when regenerating rather than overwriting retained evidence.

```shell
.build/paper-report-env/bin/python benchmark/paper/report.py .build/paper-fr11-confirmation-[0-9][0-9]-* --output .build/paper-fr11-confirmation-report --phase confirmation
python3 .build/paper-fr11-confirmation-control/summarize.py
```

The general report retains `results.csv`, `implementations.csv`, `evidence.json`,
`report.md`, a SHA-256 manifest, and SVG/PNG scientific figures. The attribution
report at `.build/paper-fr11-attribution-report` retains CSV/JSON/Markdown
comparisons, the original per-session FR-6 attribution groups, representative
primary examples, the campaign analysis script, and a hash manifest. The
control directory retains the exact resolved report invocation, preparation
snapshots, all 12 commands, completion statistics, and session-variation data.

The combined report passed with 1283 rows, nine processes and three sessions
per row, and one compatible provenance stratum (`52037559c4e674ce`). Initial
visual review found that long observations compressed the callback curves on
a linear x-axis. A post-measurement presentation change in `report.py` uses a
symlog x-axis, with a 1 ns linear region to retain quantized zeros. This is a
display scale, not a timing-resolution claim. Both versions use the same
retained deterministic ECDF grid (up to 4096 points per process, including
endpoints); statistics use the full raw observations. The archived measured sources,
raw data, observations, acceptance budgets, statistics, and exclusions are
unchanged.

The preferred final report is
`.build/paper-fr11-confirmation-report-readable`. It was rendered from the
already checked original evidence with this retained command, avoiding a
second measurement campaign or unnecessary raw-data recomputation:

```shell
.build/paper-report-env/bin/python .build/paper-fr11-confirmation-control/render_readable.py
.build/paper-report-env/bin/python -m unittest discover -s benchmark/paper -p test_report.py
```

All 288 SVG/PNG pairs were produced. The tables and cost-versus-length figures
were verified byte-identical to the original report, and all 582 final artifact
hashes passed. The render provenance records the input evidence/manifest and
both generator hashes. All four report tests passed, including deterministic
figure generation. Representative visual review covered the N=4096 analysis,
inverse, FIR-chain, independent four-channel and double-analysis panels, plus
real-transform and full-analysis cost-versus-N plots. Titles, units, legends,
family-specific age axes, session ranges, and retained outliers were checked.
The inventory, validation receipt and control records have their own manifest.

Final documentation checks passed all 17 local links, frozen configuration
hashes/counts, and `git diff --check`. The post-measurement edit changes only
report presentation; the existing four report regressions were the relevant
additional tests. No new Rack plugin build or manual Rack session was run for
this benchmark/report-only stage. FR-11 is COMPLETE; the owning spec remains
IN PROGRESS until FR-12 passes manuscript integration and review.

Raw campaigns and generated outputs are local ignored research artifacts;
they have not been published or added as large generated files to Git.
FR-12 must select and integrate final figures/tables, pin the archived source
identity, and preserve/package the evidence for the manuscript's eventual
public reproducibility path. ARM64/x86-64 replication, actual audio-device
measurements, independent-day/host replication, and the deferred FR-7--FR-9
methods remain outside the present empirical claims. No Rack module behavior,
module dependency, or shipped DSP source changed during FR-11 confirmation.

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

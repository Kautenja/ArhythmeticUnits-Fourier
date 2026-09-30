# Benchmark Reliability And Scheduling Preparation

Prepare validated benchmark implementations and simple offline launch commands
for the user to collect reliable measurements on a quiet laptop. The intended
evidence will support the full paper's discussion of scheduling, cost, and
execution granularity. Collecting and interpreting it are separate follow-ups.

Status: IN PROGRESS

Created: September 30, 2026

## Goal And Execution Boundary

Extend the existing benchmark workflow, implement focused experimental
variants, verify their correctness, and deliver a prepared local package with
the exact commands the user needs to launch measurements. Finish each phase's
preparation checks before dependent work; retain decisions and validation
evidence in this spec. Completion ends at the runnable handoff, without
requiring benchmark numbers, a performance winner, or manuscript changes.

The current implementation request covers Phase 1. Its completion evidence is
recorded below; unchecked phases remain future work. No measurements have been
collected by the agent. The user prefers strengthening the full paper before
choosing a venue and retaining the M1 Pro as the primary test platform. A second
architecture is optional follow-up; cross-architecture claims require actual
measurements on that architecture.

Implementation of this spec authorizes builds, dependency preparation, untimed
correctness/allocation/concurrency checks, and tests using synthetic timing
records, fake clocks, or stub measurement processes. It does not authorize the
agent to launch performance measurements, including development benchmarks,
timed smoke tests, pilots, calibration campaigns, or confirmation runs. Adapt
tests that currently enter real timing loops to use those test seams before
running them. Ordinary test-runner elapsed-time output is not benchmark evidence.

The user launches every measurement session after disconnecting networking,
turning off Bluetooth, and closing agents and other unnecessary applications.
The agent does not remain active to monitor the campaign. Once the user returns
the output, a separate task can inspect it, prepare any pilot-backed confirmation
freeze, and later integrate adequate evidence into the paper. Neither that
follow-up nor production optimization promotion is a completion gate here.

The study follows [spec 012's evidence handoff][evidence-spec] and
[spec 013's manuscript integration][paper-spec]. It uses the existing
[benchmark workflow][workflow], [C++ adapters][adapters],
[development runner][development], and [whitepaper guide][paper-guide].
Do not introduce another campaign runner or agent-loop infrastructure.

## Review Inputs And Established Findings

The planning discussion considered three peer-review simulations and their
[archived meta-review][reviews]. Their recommendations are
research leads, not independent experimental validation or venue decisions.
The requirements below are self-contained if those local reviews are absent.

The user also supplied the chat
[Review paper benchmark reliability][reliability-chat]. Its important findings
are preserved here so execution does not depend on continued chat access:

-   The existing headline values reproduce from retained observations:
    scalar core p99 is approximately 3.42 us versus 9.50 us for the current
    vDSP adapter at N=4096, H=1024, D=64, 48 kHz, without smoothing. Aggregate
    cost is 2.27 times higher and publication is 21.3 ms later. This is evidence
    for those adapters and conditions, not an optimized-library ranking.
-   At N=16384, one hybrid throughput process costs 390.32 ns/sample while
    the other eight cost 126.40--128.94 ns/sample. The reported 157.39 mean is
    about 22 percent above the nine-process median. Retain the slow process;
    neither E-core placement nor a particular background service is established
    as its cause. See the [per-process table][process-table].
-   A [host snapshot][host-snapshot] records `sandboxd` at 90.2 percent CPU.
    It does not establish the load during each measurement. More importantly,
    the [runner][runner] builds, archives, verifies, and probes resources
    after the manually observed quiet interval. That interval therefore does
    not establish host state at the start of timing.
-   Callbacks execute back-to-back. The representative observation window
    represents 43.7 seconds of audio but only tens of milliseconds of timed
    execution. This is a useful sustained-execution microbenchmark, distinct
    from callbacks arriving every D/f_s seconds.
-   The native adapters use scalar preparation/conversion loops. The hybrid
    dispatches surrounding work per element. Stronger implementations need
    measurement before adopting a reviewer's claimed speedup from another host.
-   The confirmation campaign omits the available module backends. Those
    backends force Hann and coupled smoothing settings; their final checks
    inspect selected bins rather than audit every module output.
-   Local Rack source has per-sample worker barriers and enables ARM64
    flush-to-zero in `system::resetFpuFlags()`. Reverify against the exact Rack
    revision used by the new study. A sample period is not a separate device
    deadline, and perfect division of work by thread count is not assumed.

## Questions And Expected Behavior

1.  Does the current scheduler reduce useful host-level peaks against efficient
    native analysis, and when does its added aggregate cost outweigh that gain?
2.  How much of each result comes from scheduling, kernel choice, layout,
    preparation/output code, caching, or SIMD channel parallelism?
3.  Does a hybrid or native sub-FFT schedule provide a better cost, peak, and
    publication-age tradeoff than individual butterflies?
4.  Can a shorter completion horizon recover freshness without unacceptable
    peaks, while preserving exact frame selection and publication cadence?
5.  Do conclusions survive prepared sessions, pacing, real module work,
    configuration changes, competing DSP, and display consumption on the M1 Pro?

Behavior examples:

-   If efficient vDSP eliminates the old p99 advantage, retain that result and
    revise the claim. Do not enlarge only the workloads on which the core wins.
-   A process that is three times slower remains in raw data and the primary
    summary. A predeclared sensitivity analysis may identify it separately;
    a speed threshold alone does not prove a CPU-core assignment.
-   With H=1024 and completion horizon H_c=256, frame endpoints remain 1024
    samples apart and publication occurs 255 samples after each endpoint.
    Capture continues through the remaining hop; no extra frame is selected.
-   A silent fixture must pass with the correct silent result. A check that
    requires positive output must become fixture-aware, not be bypassed.
-   Four staggered channels may represent different input endpoints. Their
    result is labeled as an alternative with different alignment semantics,
    not a drop-in equivalent to four simultaneous channel spectra.

## Scope And Preservation

-   Preserve old campaigns, freezes, source archives, numerical policies,
    generated publication assets, and their identities. Use new backend IDs
    and new output roots where implementations or contracts change. Reanalysis
    is new derived evidence and never rewrites old raw observations.
-   Keep reusable DSP independent of Rack and provider APIs. Experimental
    backends belong in `benchmark/paper/`; host integration belongs at the Rack
    boundary. Shared kernels must not introduce allocation, locks, I/O, timers,
    logging, or drawing into production sample processing.
-   Preserve frame selection, normalization, DC/Nyquist treatment, magnitude
    smoothing, channel independence, cancellation, ownership, and saved patches.
    No parameter/port/light renumbering, slug changes, or silent JSON changes.
-   Preserve GPL attribution, C++11 production compatibility, and the existing
    C++14 benchmark/test baseline. Follow the [C++ style guide][cpp-style] when
    implementation begins. Do not modify vendored providers to improve rankings.
-   Do not add a new convolution implementation, GPU backend, automatic tuner
    in production, mailbox redesign, licensing change, or panel feature here.
    Keep existing inverse/chain results as explicitly scoped controls.
-   Worker/UI-thread analysis remains an alternative. Exact-hop analysis can
    run on a worker with suitable buffering; do not claim engine-thread
    execution is necessary or universally superior without comparing it.
-   Public deposition, submission, push, and release are separate actions.
    Prepare reproducible launch inputs locally; do not invent collected data,
    public availability, or a DOI. New measurement collection, results analysis,
    manuscript revision, venue formatting, and acceptance promises are outside
    this implementation's scope.

## Phase 1: Establish Reliable Measurement Boundaries

Primary files: [runner][runner], `benchmark/paper/protocol.hpp`,
`benchmark/paper/{execution,measurement_clock,development}.hpp`, and
`benchmarks/lib/{execution,workflow,study,check}.py` under `docs/whitepaper/`.

- [x] Define a versioned execution contract distinguishing continuous,
      absolute-time-paced, and Rack-engine runs. Record compiler flags, binary,
      source, SDK/provider identities, power mode, timer resolution, requested
      and effective thread policy, and per-thread FPU state. Unsupported policy
      requests fail or are explicitly unavailable; they never silently succeed.
- [x] Require `/usr/bin/caffeinate -is` around every macOS benchmark launch,
      including development, smoke, pilot, confirmation, and Rack-host timing.
      Hold its assertions from before stabilization through the last measured
      child process; the wrapped runner must wait for all measurement children.
      `-i` prevents idle system sleep; `-s` prevents system sleep on AC power.
      Record the wrapper command, process lifetime, and assertion evidence
      before the stabilization gate and after measurement, outside timed loops.
      Missing or prematurely ended protection fails the run's integrity check;
      retain its observations and failure record without promoting it to valid
      confirmation evidence.
- [x] Require the user's M1 Pro sessions to use AC power with Low Power Mode off.
      Verify and record power source and effective power settings separately
      from sleep assertions; flag any change during the session. `caffeinate`
      does not disable Low Power Mode, fix CPU frequency, or prevent thermal
      throttling. Fail readiness when required settings cannot be established;
      do not silently change system settings or infer stable performance from
      an active sleep assertion.
- [x] Put build, archive/hash work, full preflight, inventory verification,
      and resource probes before a recorded stabilization gate. No rebuild or
      resource probe may occur between that gate and measurement. Execute
      verified prepared binary bytes, retaining source/dependency checks.
- [x] Define session settling and inter-process preparation policies. Account
      for FFTW planning, previous correctness replay, hashing, serialization,
      and calibration work. Deliberate warmup follows the declared steady-state
      protocol; startup remains a separate cold-analysis workload. Do not assume
      a quiet interval before compilation establishes either condition.
- [x] Configure pilot launches with at least 180 seconds of settling after
      session preparation. Implement timestamped host observations and
      calibration recording; later pilot review will freeze measurable
      start/flag conditions. Absence of a thermal warning is not a temperature
      or frequency measurement. Bound gate waiting and retain failure/timeout
      evidence without killing services or repeatedly restarting until a
      favorable run appears.
- [x] Support unattended serial execution without active interactive
      agent/editor work, builds, tests, or reporting during the campaign.
      Require fully local launch inputs and no network access, downloads,
      dependency resolution, remote logging, or agent connection. Document the
      user's manual preparation: networking disconnected, Bluetooth off, agents
      stopped, and unnecessary applications closed. Record verified state and
      user declarations separately; disconnected radios alone do not prove an
      idle host. Do not change those settings or terminate processes on behalf
      of the user.
- [x] For paced runs, retain scheduled release, actual start, and finish times;
      distinguish compute duration, wake-up lateness, and release-to-finish
      deadline misses. Use absolute deadlines and an explicit overrun policy:
      preserve logical sample order, never silently drop or rebase late work.
      Label catch-up execution. Include deliberately interleaved DSP in timing;
      keep synthetic cache-conditioning cost separately identified.
- [x] Implement configurable throughput chunks with original ordering retained.
      Prepare explicit initial duration, warmup, calibration-placement, and
      pacing policies for later pilot review. Do not subtract timer overhead
      or time every butterfly in the primary loop.
- [x] Record pre/post calibration and available host-state evidence. Keep all
      slow runs. A predeclared flag policy supports sensitivity analysis, not
      retrospective deletion or unsupported E-core/preemption attribution.
- [x] Add runner tests proving preparation precedes settling, measured binary
      identity cannot change, required sleep assertions cover the campaign,
      unsupported policies are visible, and failed or partial runs cannot
      become valid confirmation evidence. Test pacing arithmetic/overruns with
      deterministic clock fixtures separately from hardware smoke execution.

Gate: untimed checks and fake-clock/stub-process tests verify sleep protection,
power checks, pacing, stabilization order, and complete failure records. The
user's smoke/pilot launch is prepared; no real timing run is needed to pass.

## Phase 2: Workload Controls, Metrics, And Audits

Primary files: `benchmark/paper/{protocol,backend,analysis_reference}.hpp`,
`docs/whitepaper/benchmarks/lib/{profiles,contracts,observations,report}.py`,
the backend registry, and their existing verifier/test directories.

- [ ] Add explicit window, independent octave/temporal smoothing, fixture/seed,
      active-port/voice, execution-regime, and experimental-policy fields.
      Specify temporal smoothing in samples/alpha or physical time explicitly;
      preserve the modules' time-control convention. Include every effective
      field in C++/Python contracts, hashes, freezes, raw metadata, and grouping.
      Existing protocol versions keep their historical meanings.
- [ ] Add silence, signal-to-silence decay, impulse, DC/Nyquist, off-bin tones,
      weak signals, deterministic noise, and independent-channel fixtures.
      State supported finite input ranges and FPU-dependent expectations.
      Retain independent all-bin reference audits and existing error budgets;
      numerical changes do not justify loosening tolerances after timing.
- [ ] Implement per-hop maximum-callback distributions, per-phase profiles,
      peak/mean summaries, individual process points, means and medians,
      sample counts, elapsed duration, and actual budget-exceedance counts.
      Keep p99 and maxima as secondary descriptive outcomes.
- [ ] Define hop grouping using actual frame endpoints and callback intervals,
      including offsets, staggered instances, partial edge hops, D>H, and
      non-divisible H/D. A callback overlapping multiple hops cannot yield
      independent hop observations or a fabricated sub-callback duration.
      Test these cases with synthetic traces and retain original coordinates.
- [ ] Separate per-analyzer, aggregate-block, publication, consumer, and device
      metrics. Fractional-budget thresholds are explicit allocation scenarios;
      they are not actual device deadlines. Do not pool channels or callbacks
      as independent process/session replicates. Any confidence intervals must
      respect temporal clustering and state their limited same-host scope.
- [ ] Use retained raw evidence as regression fixtures for headline values,
      hop peaks, phase imbalance, slow hybrid processes, maxima, and exceedances.
      Keep inverse/chain misses in their own comparison boundary. New research
      interpretation belongs to the follow-up after user measurements.
- [ ] Allow small explicit-workload profiles without appending an entire
      preset. Revise fast development coverage to include efficient macOS
      native analysis when enabled, SIMD, and representative module work;
      keep optional-provider omissions explicit. Keep development and frozen
      publication repetition policies distinct.
- [ ] Extend report/export/bundle checks to the new metrics and schemas.
      Demonstrate that malformed timestamps, missing bins, missing processes,
      invalid norms, and mixed contracts fail. Historical archives remain
      readable by their recorded schema/tooling.

Gate: a fixture report demonstrates all metrics and audit failures, and the
old checked publication assets still reproduce unchanged.

## Phase 3: Credible Baselines And Matched Controls

Primary files: `benchmark/paper/{external,vdsp,pffft,fftw,hybrid,fourier,channels}.hpp`.

- [ ] Add efficient native analysis as new backends. Use two-span or otherwise
      efficient retained-input access, provider-appropriate window/packing,
      conversion, magnitude, and output kernels. Preserve mathematical scaling
      and audit floating-point differences independently. Inspect generated
      code or retained profiles when choosing variants. Defer claims about
      their actual cost until the user supplies measurements.
- [ ] For PFFFT, evaluate unordered output only with correct natural-frequency
      mapping wherever bands or display bins require it. Charge every necessary
      reorder/conversion/store to the declared boundary. Retain the existing
      portable scalar-glue adapters as historical controls.
- [ ] Add batched independent four-channel native analysis where supported,
      and per-channel/instance staggering. Record frame endpoints per channel;
      distinguish simultaneous spectra from staggered freshness. Equal channel
      counts alone do not establish equivalent output semantics.
- [ ] Add an efficient hybrid with contiguous stage-segment dispatch and a
      matched batch mode sharing its kernels and storage. Retain the old hybrid
      pair so later measurements can test its per-element dispatch overhead.
- [ ] Add a matched batch/distributed pair using the current core's arithmetic,
      layout, cache policy, and output contract. Existing legacy controls also
      change layout and boundary work, so do not substitute them for this pair.
      Factor only the needed execution seam; avoid a general scheduler framework
      or a copied production implementation that can silently diverge.
- [ ] Add diagnostic stage-cost measurements and untimed operation traces.
      Prepare instrumentation-overhead controls; primary measurement paths
      retain uninstrumented timing boundaries. Defer running these diagnostics
      to the user, and do not sum stage times as if they reproduce integrated
      cost.
- [ ] Verify every new backend with all-output references, frame-retention and
      publication checks, cold/live cache cases, allocation probes, and explicit
      opaque-provider limitations before its first performance pilot.

Gate: stronger native and matched scheduling controls produce correct outputs
under the same declared tasks. No required baseline is omitted for being faster.

## Phase 4: Modules, Rack, And Concurrent Consumers

Primary files: [module adapters][modules], `benchmark/paper/benchmark.cpp`,
benchmark-only Rack-engine integration, and `test/paper/`/`test/rack/` verifiers.

- [ ] Add explicit shipped-default and controlled-comparison module modes.
      Record actual quantized values after applying controls. Fourier defaults
      include four lanes, N=2048, a nominal 30 ms hop, and Flattop; Spectre
      has N=2048/H=1024. Cover Fourier N=16384 with the nominal 5 ms hop.
      Distinguish active ports from polyphonic voices summed into each port.
- [ ] Replace selected-bin module checks with an untimed all-output audit of
      input normalization, DC blocking/gain, spectra, smoothing, coordinate
      mapping, and publication. Use independent expectations where possible;
      checking production output against the same production calculation is
      insufficient. Keep timing free of reference work and audit polling.
- [ ] Build a headless harness using the actual pinned Rack engine. Include
      current modules and comparable benchmark-only native/hybrid analyzer
      modules with matched conditioning, display preparation, and publication.
      A trivial-sink native analyzer cannot stand in for a complete module.
- [ ] Prepare cases with 1/4 engine threads, 1/4/16 analyzers, D=64/256, 48 kHz,
      aligned/staggered phases, and a fixed background-DSP workload sweep.
      D=16 or higher-rate host stress extensions may follow pilot review in a
      later task. Specify identical background workloads across contenders;
      do not equalize total utilization separately and hide extra analyzer cost.
- [ ] Implement recording of full engine-block duration, aggregate CPU cost,
      hop peaks, publication age, and budget misses. An analyzer-free engine is
      a useful overhead control, not a value automatically subtracted from
      observations. Verify engine barriers, worker policy, lifecycle locking,
      and FPU mode against retained Rack source. Do not assume ideal
      cost/thread-count scaling.
- [ ] Prepare concurrent single-consumer workloads at representative 30/60 Hz
      rates, with deliberate stalls, and recording for producer impact,
      consumed-spectrum age, skipped updates, and snapshot consistency.
      Validate ownership and snapshot correctness untimed. Preallocate storage;
      do not add a second mailbox consumer or synchronize unsafely through the
      module's other fields. Separate headless consumption from actual rendering.
- [ ] Add controlled reset, freeze/resume, sample-rate, window, band, and geometry
      transitions. Separate lifecycle allocations and host-lock stalls from
      steady sample processing. Include silence decay under recorded FPU modes.
      Verify single-producer ownership during host-serialized reset/publication.
- [ ] Use deterministic concurrency tests and the existing mailbox TSan check
      outside timing. An eventual device-underrun claim additionally requires
      a user-run audio-device experiment with recorded device/driver/buffer
      policy and overload telemetry. Device experiments are a later extension;
      headless exceedances alone support no audible claim.

Gate: complete modules and pinned Rack-engine workloads pass untimed correctness
and ownership checks at defaults and demanding settings. Recording paths are
tested using fixtures; host performance measurement remains user-owned.

## Phase 5: Prepare Scheduling And Kernel Experiments

These are benchmark-only candidates. Implement and numerically validate the
following in order; record readiness or a reasoned deferral for each. Performance
evaluation, weight tuning, tradeoff curves, and winner selection happen after
the user returns measurements. Missing candidates limit later claims.

- [ ] Native sub-FFT scheduling: bounded native leaves, scheduled twiddles,
      permutations, reconstruction, and postprocessing. Include all copies and
      scratch costs; an opaque leaf is indivisible. Prepare a small predeclared
      leaf-size sweep against whole-native-FFT hybrid and butterfly scheduling
      without assuming either wins.
- [ ] Completion horizon: decouple H_c from H; test H/4, H/2, H, and immediate
      execution where supported. Define rounding for non-divisible H and
      1 <= H_c <= H. Preserve endpoint cadence, capture during idle phases,
      latching, cancellation, ownership, and publication at t_r+H_c-1.
      Update C++/Python delay contracts and ring-lifetime reasoning. Merely
      changing the quota denominator is not a complete implementation.
- [ ] Stage weights: expose preparation, reconstruction/magnitude, output,
      and dirty-band weight sweeps with scalar/SIMD and real module sinks.
      Disclose earlier M1 Pro tuning from [spec 010][weight-spec]. Specify
      tuning and held-out sizes, hops, and states before measurement. Defer
      tuning decisions; observed p99 will not establish a WCET bound.
- [ ] Kernel variants: use source inspection or existing profiles to prepare
      ring-index arithmetic, scalar butterfly segmentation,
      magnitude/reconstruction, and coordinate-mapping experiments separately.
      Do not convert magnitude smoothing to power smoothing, change
      coherent gain, or add signal-dependent shortcuts without a distinct
      contract. Capture before/after builds and test each change independently.
- [ ] Extend deterministic tests for zero-work calls, quotas crossing stages,
      sparse/dense paths, wraparound, H=1, H>N, changing H/H_c, all windows,
      DC/Nyquist, independent lanes, reset mid-cache-rebuild, and unchanged
      numerical/publication behavior where the contract promises it.
- [ ] Keep credit and time models separate. The reviewer-proposed bound
      `c0 + kappa * ceil(W/H)` does not follow from weights when an entire
      weighted operation executes on its first credit. For dirty N=4, H=16,
      o=1, W=15, a one-credit call can execute a weight-four operation. Any
      replacement cost model must account for indivisible boundary operations,
      dispatch/configuration overhead, and its explicit cost assumptions.
      Prepare held-out workloads for later validation of predictions without
      calling them WCET.

Gate: prepared candidates pass numerical/scheduling checks, deferred candidates
are documented, and the user can collect every planned comparison. No timing
result or optimization selection is required.

## Phase 6: Prepare The M1 Pro Study Plan

The primary host is the existing 10-core, 16 GiB M1 Pro. The prepared package
must capture actual OS, compiler, SDK, and provider identities; the user-run
launcher will capture current power and scheduling state. Do not inherit the
old campaign's software versions or host preparation claims.

Prepare the following focused groups rather than a full Cartesian product:

| Group | Initial Pilot Coverage | Main Question |
| --- | --- | --- |
| Baselines | N=2048/4096/16384, H=1024, D=64, scalar unsmoothed, core/matched/native/hybrid | Do efficient baselines change the original result? |
| Granularity | Selected N/H, leaf size, H_c, and weights; scalar and independent four-channel | Which granularity and delay meet the requirement? |
| Scaling | D=16/64/256, fixed H and separately fixed H/N, supported precision | Which scaling relationship explains the result? |
| Modules | Actual defaults, extreme Fourier settings, independent smoothing and active ports | Does the complete shipped path behave like the core? |
| Host | Phase 4 thread/count/block/load/consumer cases | Does reducing bursts improve available host capacity? |
| Stress | Non-divisible hops, cold/live caches, silence decay, lifecycle events | Which limits or regressions constrain the recommendation? |

- [ ] Write hypotheses, primary outcomes, initial observation lengths, process
      repetitions, allowed session order seeds, flagging policy, aggregation,
      and stopping rules into explicit versioned profiles. Prepare candidate
      practical thresholds in absolute time, miss-rate or sustainable fixed-load
      terms, together with spectrum-age/cost budgets. These are study-design
      inputs, not conclusions about expected performance.
- [ ] Prepare commands for at least two separately prepared user-run pilot
      sessions covering all required groups. Resolve implementation errors
      using correctness tests before handoff. Preserve all attempts; pilots
      will later inform drift, multimodality, resolution, phase coverage,
      duration, and storage decisions. No pilot run is required to close this
      preparation spec.
- [ ] Preserve the pilot-backed confirmation boundary. The initial package
      collects pilots; it must not fabricate a confirmation freeze without
      measurements or automatically promote pilot observations. After the user
      returns pilots, a separate analysis task can propose a freeze and supply
      confirmation commands for the user. Plan for at least three confirmation
      sessions on separate days and five fresh processes per primary cell per
      session, subject to the later pilot review. That decision and those runs
      are outside this spec's completion requirements.
- [ ] Extend freeze validation to support a predeclared, recorded session-seed
      policy while preserving workload membership and measurement semantics.
      Test it against synthetic pilot records. The existing fixed-seed freeze
      must not be bypassed, and fixture data must never qualify as real evidence.
- [ ] Make observation lengths configurable and retain effective hop counts,
      duration, quantile ranks, and process/session variation. Test summaries
      and inadequate-sample reporting using fixtures; hundreds of hops do not
      establish a precise rare-event probability.
- [ ] Enforce serial measurement of unchanged prepared artifacts. Changes to
      source, dependencies, profiles, or instrumentation invalidate the relevant
      preparation and any later freeze. Implement clear errors and new-output
      requirements instead of silently rebuilding or relabeling observations.
- [ ] Preserve every planned cell and slow observation. Implement completeness
      and integrity checks that distinguish a failed run from a disturbed but
      valid observation. No best-run selection, winner-based early stopping,
      silent omissions, or performance-driven automatic retries.

Gate: checked profiles, resolved workloads, immutable prepared identities, and
fixture-tested recording policies are ready for user collection. No measured
pilot, confirmation freeze, or statistical result is required.

## Phase 7: Offline Launch Package And User Handoff

Primary files: the existing Make/benchmark entry points, workflow library,
benchmark profiles, and their tests/documentation. Extend those components;
the convenience targets below must not introduce a second campaign engine.

- [ ] Add `make benchmark-study-prepare` to prepare all required groups in one
      operation. Resolve dependencies while connectivity is available, build
      executables, run untimed correctness checks, archive source/dependency
      identities, and save an explicit launch manifest. Preparation never
      invokes a timing pass, calibration benchmark, or implicit smoke campaign.
      Record unavailable providers as errors for required comparisons.
- [ ] Add `make benchmark-study-run SESSION=pilot-01` as the user entry point.
      Resolve groups, profiles, binary paths, host identity, order seed, output
      paths, and recording policy from the prepared manifest. Require an
      explicit, fresh session label. The target must have no build prerequisite
      and no network operation; absent, stale, or incompatible preparation is
      an actionable failure, never an automatic rebuild or dependency download.
- [ ] Have that launch apply `/usr/bin/caffeinate -is` automatically, retain its
      assertions for the entire runner and its children, check readiness,
      perform the declared stabilization wait, and execute groups serially.
      Any expensive verification precedes settling. Keep deliberate per-process
      setup and warmup within the declared Phase 1 policy. The user must not
      need to assemble flags, manually time a sleep interval, or keep an agent
      connected. Do not create a recurring task or remote monitor.
- [ ] Keep campaign outputs outside normal clean/build directories, defaulting
      to `~/Fourier-benchmarks/spec014/SESSION/`. Expand and print the actual
      absolute location. Retain raw observations, complete/failed status,
      prepared manifest, workload/seed policy, checksums, numerical audits,
      logs, and source/dependency identities needed for later analysis. Preserve
      failed/interrupted directories and reject collisions; never delete or
      overwrite an earlier session to retry it.
- [ ] Provide a short quiet-host checklist: connect AC, disable Low Power Mode,
      keep the lid open, disconnect Wi-Fi/Ethernet/other network connections,
      turn off Bluetooth, stop agents, and close unnecessary applications.
      Use the built-in keyboard/trackpad or wired controls as needed. Launch
      from a standalone terminal after closing Codex and other agent hosts,
      then leave the machine alone. Record user-declared isolation separately
      from observable host state, and never claim that all OS activity ceased.
      The launcher must not toggle radios, modify power settings, kill services,
      or require the internet to validate readiness.
- [ ] Print local progress/status at boundaries outside measured loops and a
      final result-directory path. Once timing ends, write an offline handback
      archive with its checksum and a concise README. Include everything needed
      for the follow-up except documented restricted SDK/provider binaries.
      Do not render reports, select favorable rows, rewrite the paper, or start
      another independent session automatically. No upload is required.
- [ ] Add `make benchmark-study-check` to test launch dispatch, stale-input
      rejection, sleep-assertion lifetime, quiet-gate ordering, offline operation,
      argument forwarding, output collisions, interrupted children, and final
      packaging using fake clocks, fixture records, and stub measurement
      processes. Untimed real numerical/concurrency checks remain separate.
      Distinguish synthetic artifacts unmistakably from measured campaigns.
- [ ] With dependencies already available locally, validate preparation and
      fixture-based launch checks with network calls blocked and no connected
      agent service required. Confirm that missing dependencies produce
      preparation instructions, not a fetch during launch. Document any untested
      real-host measurement behavior; obtaining actual performance numbers is
      the user's task.
- [ ] Deliver the exact repository path, successful preparation/check commands,
      manifest identity, copyable session launches, expected output/archive
      paths, completion/failure markers, and interruption instructions. Provide
      workload counts and labeled planning estimates; do not invent a measured
      runtime. Tell the user which archive to return once agents can be reopened.

Gate: the user can disconnect, close agents, and launch each independent session
with one short command against a fully prepared local package. All preparation
and untimed/fixture checks pass. Archive this spec as COMPLETE at that handoff,
recording any deliberate candidate deferrals and unverified timing behavior;
do not wait for measurements, interpretation, paper changes, or publication.

## Validation And Final Launch Commands

Run commands from the repository root. The [contributor guide][contributing]
and [workflow][workflow] specify the Rack SDK, compiler, optional providers,
Python, and existing prerequisites. During implementation, use only correctness
and fixture checks: no real benchmark run is authorized by this spec. Audit
any changed test path before running it; replace entry into real timing loops
with a fake clock or stub process. Do not weaken numerical checks to avoid work.

### Existing Preparation Checks

```shell
python3 -m unittest discover -s docs/whitepaper/benchmarks/tests -p 'test_*.py'
make test-benchmark-dev
make test/dsp/test_spectrum_analysis
make test
make test-mailbox INSTRUMENT=tsan
make test-rack
make -j2
python3 docs/whitepaper/tools/comparison_paper.py --check
make -C docs/whitepaper check
git diff --check
```

Run `make test/dsp/test_spectrum_analysis INSTRUMENT=asan-ubsan` for relevant
memory/lifecycle changes. Provider skips are reported, not counted as passes;
required macOS providers must be available in the prepared package. A passing
DSP test is not a successful Rack build, manual session, or benchmark. These
are future implementation commands, not claims of execution while editing the
spec. Documentation-only changes use link, command, and diff checks.

### Required Simple Command Interface

The following Make targets are requirements to implement; they do not exist at
this spec revision. Do not execute or present them as available until their
implementation and preparation checks pass. Exact profile filenames, resolved
workload lists, paths, and source identities belong in the generated manifest
and handoff README so the user's commands can stay short.

The agent prepares the package while dependencies and build tools are available:

```shell
make benchmark-study-prepare
make benchmark-study-check
```

The final handoff must give the absolute repository path and instruct the user
to open a standalone terminal there, apply the quiet-host checklist, and launch:

```shell
make benchmark-study-run SESSION=pilot-01
```

After that session finishes, in a separate prepared session:

```shell
make benchmark-study-run SESSION=pilot-02
```

Each command covers all required pilot groups from the prepared manifest and
applies `caffeinate -is` internally. These examples are user commands, never
agent validation steps. The final handoff should contain no long per-backend
CLI sequence, dependency installation, plotting step, or manual profile edits.
The user can reconnect and reopen an agent after collection to return the
printed archives. A second independently prepared session must not be simulated
by looping over two labels in the same launch.

### Follow-Up After The User Returns Results

Separate future work will check the returned evidence, assess pilot stability,
choose practical thresholds and adequate durations, and prepare any required
confirmation freeze and user launch commands. After sufficient evidence is
returned, it can compare cost/peak/age tradeoffs, retain unfavorable results,
produce checked figures/tables and portable evidence, revise the full paper,
and decide whether a verified optimization merits production integration.
Those tasks must not run automatically as part of this preparation spec.

### Completion Checklist

- [x] Phase 1: sleep protection, power/isolation checks, stabilization, pacing,
      and failure behavior verified with untimed checks and fixtures.
- [ ] Phase 2: explicit workloads, metrics, compatibility, and audits verified.
- [ ] Phase 3: native and matched scheduling controls numerically validated.
- [ ] Phase 4: module, Rack-engine, consumer, and stress workloads prepared and
      correctness/ownership checks passed.
- [ ] Phase 5: experimental variants prepared or explicitly deferred; no
      measured performance selection required.
- [ ] Phase 6: pilot profiles and future confirmation policy prepared and
      fixture-validated; no campaign executed by the agent.
- [ ] Phase 7: offline package, simple launch targets, handback format, and
      exact user commands delivered; no paper integration required.

## Execution Evidence

September 30, 2026: specification created from the paper-review discussion,
repository inspection, and the linked reliability review. No implementation,
new performance campaign, production change, or manuscript rewrite is claimed.
Spec-author validation passed `make -C docs/whitepaper check`, repository-link
checks, shell syntax checks, and argument parsing of all 18 benchmark CLI
examples without executing the planned campaigns. Reviewed the specification
against existing source, recorded observations, and contributor guidance.

September 30, 2026: added mandatory macOS `caffeinate -is` protection, AC-power
and Low Power Mode requirements, assertion-lifetime checks, and wrapped launch
examples. Verified option semantics against the installed `caffeinate(8)` and
`pmset(1)` manuals. Repository links, shell syntax, all 18 benchmark CLI examples
(including four wrapped launches), and `git diff --check` passed. No benchmark
campaign or machine power-setting change was performed for this documentation
update.

September 30, 2026: narrowed implementation to benchmark preparation and a
user-run offline handoff. Replaced measurement, confirmation-result, and paper
completion gates with untimed/fixture validation and planned simple Make
targets. Retained mandatory `caffeinate`, power checks, settling, and data
integrity requirements. Repository links, shell-example syntax, existing
command references, and `git diff --check` passed. The new Make targets remain
unimplemented requirements; no benchmark was launched or code changed.

### Phase 1 Implementation: September 30, 2026

The preparation-only specification was committed as `1892663` before Phase 1
implementation. The measurement-boundary code now provides:

-   Execution contract version 1, continuous and absolute-time-paced streaming,
    ordered throughput chunks, and calibration/thread/FPU sidecars. Paced
    overruns catch up without dropping or rebasing samples; conditioning remains
    separate from compute duration. Frozen comparisons and report strata include
    execution policy; historical archives remain readable.
-   A local macOS guard for campaign, native development, and standalone Make
    launches. It owns and verifies both `caffeinate -is` assertions, checks AC
    power and Low Power Mode separately, and retains readiness failures.
    Failed guards invalidate their own development output without modifying a
    colliding prior run. Numerical tests and help/list/build targets are exempt.
-   A recorded session gate after heavy preparation, with 180 seconds of
    settling by default; plans precede a further 1000 ms per-workload interval
    and deliberate warmup. Publication runs execute their retained binary.
    Native development settles after its in-process preflight. Partial
    observations and failed status survive ordinary processing/guard failures.
-   Deterministic clock fixtures and stub launches covering release arithmetic,
    overruns, sample ordering, preparation order, guard lifetime, power failures,
    binary replacement, sidecar integrity, and interrupted/failed evidence.
    Synthetic sidecars cannot qualify as measurements. Existing correctness
    fixtures that entered timing loops now use a synthetic clock.

Validation actually run from the repository root:

```shell
python3 -m unittest discover -v -s docs/whitepaper/benchmarks/tests -p 'test_*.py'
python3 scripts/test-build.py
make test-benchmark-dev PAPER_VDSP=1 PAPER_FFTW_PREFIX=
make -j2 benchmark-paper-build PAPER_VDSP=1 PAPER_FFTW_PREFIX=
DYLD_LIBRARY_PATH=/Users/christiankauten/Documents/Projects/Rack LD_LIBRARY_PATH=/Users/christiankauten/Documents/Projects/Rack .build/benchmark/rack/paper --verify
make -C docs/whitepaper check
git diff --check
```

Results: the Python suite passed (93 tests, one optional plotting-environment skip);
five Make-dispatch fixture tests passed; native development tests passed 127
assertions in seven cases. Both benchmark executables built with PFFFT/vDSP;
the compiled `--inventory` matched the Python registry with `features=['vdsp']`.
Untimed native preflight passed. The Python suite also exercised the available
FFTW adapter fixtures; the native build above deliberately disabled FFTW.
Existing Rack SDK deprecation warnings remain. Whitepaper checks verified
historical campaign/source hashes, 2880 timing rows, 27 phase rows, 31 references,
numerical tables, figures, and 32768 schedules. Documentation links, shell
syntax, and whitespace checks passed.

Limitations: no performance pass, live `caffeinate` launch, paced hardware run,
full plugin build, or manual Rack session was performed. Live power readiness
remains unverified: this agent environment's read-only `pmset -g` response did
not expose Low Power Mode, which the launcher correctly treats as unreadable
and would reject. Pre/post host snapshots cannot exclude transient power or
thermal changes; user isolation notes do not prove an idle host. Only the
calling thread's effective scheduler/QoS/FPU state is observable here; opaque
provider worker state is explicitly unavailable. Unsupported thread/FPU policies
and the future Rack-engine regime fail explicitly.

Phase 1's fixture/untimed gate is complete. Phases 2--7 remain pending, including
the prepared no-build launcher, isolated-host checklist/manifest, and final
one-command handoff. Existing measurement commands still perform preparation
before settling; they are not the promised Phase 7 offline package. Do not
archive this spec or launch measurements to close the remaining preparation.

Record subsequent phase dates, decisions, exact commands/results, artifact
locations, manual checks, and limitations here. Do not create a separate
completion diary or mark preparation COMPLETE merely because planning passed.

[evidence-spec]: archive/012-comparison-evidence-and-paper-integration.md
[paper-spec]: archive/013-comparison-paper.md
[weight-spec]: archive/010-production-cache-scheduling.md
[workflow]: ../docs/whitepaper/benchmarks/guides/workflow.md
[adapters]: ../benchmark/paper/README.md
[development]: ../docs/whitepaper/benchmarks/guides/DEVELOPMENT.md
[paper-guide]: ../docs/whitepaper/README.md
[cpp-style]: ../docs/style-guides/cpp.md
[contributing]: ../CONTRIBUTING.md
[runner]: ../docs/whitepaper/benchmarks/lib/run.py
[modules]: ../benchmark/paper/modules.hpp
[process-table]: ../docs/whitepaper/data/comparison-012/tables/primary/process-timings.csv
[host-snapshot]: ../docs/whitepaper/data/comparison-012/host/pre-confirm-01.json
[reviews]: ../docs/whitepaper/reviews/2026-09-30-Opus-5-5-M/meta-review.md
[reliability-chat]: codex://threads/01a0f32a-4bf7-7fd0-bfc2-c641595e07d8

# Benchmark Reliability And Scheduling Study

Strengthen the analyzer code package and its measurements so the full paper
can explain when scheduling improves host behavior, what it costs, and which
execution granularity is useful. Better numbers mean more reliable and
informative evidence; favorable timings are not a completion requirement.

Status: PLANNED

Created: September 30, 2026

## Goal And Execution Boundary

Extend the existing benchmark workflow, evaluate focused implementation
variants, collect controlled M1 Pro evidence, and revise the full technical
report around the results that survive. Work through the numbered phases
below. Finish each phase's acceptance checks before dependent work; retain
decisions and execution evidence in this spec.

The current request creates and commits this specification only. Its unchecked
items are future work, not completed implementation or measurements. The user
prefers strengthening the full paper before choosing a venue and retaining the
M1 Pro as the primary test platform. A second architecture is optional follow-up;
cross-architecture claims require actual measurements on that architecture.

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
    Prepare reproducible artifacts locally; do not invent public availability
    or a DOI. Venue formatting and acceptance promises are outside scope.

## Phase 1: Establish Reliable Measurement Boundaries

Primary files: [runner][runner], `benchmark/paper/protocol.hpp`,
`benchmark/paper/runtime.hpp`, and `benchmarks/lib/{workflow,study,runtime}.py`
under `docs/whitepaper/`.

- [ ] Define a versioned execution contract distinguishing continuous,
      absolute-time-paced, and Rack-engine runs. Record compiler flags, binary,
      source, SDK/provider identities, power mode, timer resolution, requested
      and effective thread policy, and per-thread FPU state. Unsupported policy
      requests fail or are explicitly unavailable; they never silently succeed.
- [ ] Require `/usr/bin/caffeinate -is` around every macOS benchmark launch,
      including development, smoke, pilot, confirmation, and Rack-host timing.
      Hold its assertions from before stabilization through the last measured
      child process; the wrapped runner must wait for all measurement children.
      `-i` prevents idle system sleep; `-s` prevents system sleep on AC power.
      Record the wrapper command, process lifetime, and assertion evidence
      before the stabilization gate and after measurement, outside timed loops.
      Missing or prematurely ended protection fails the run's integrity check;
      retain its observations and failure record without promoting it to valid
      confirmation evidence.
- [ ] Run the primary M1 Pro study on AC power with Low Power Mode disabled.
      Verify and record power source and effective power settings separately
      from sleep assertions; flag any change during the session. `caffeinate`
      does not disable Low Power Mode, fix CPU frequency, or prevent thermal
      throttling. Fail readiness when required settings cannot be established;
      do not silently change system settings or infer stable performance from
      an active sleep assertion.
- [ ] Put build, archive/hash work, full preflight, inventory verification,
      and resource probes before a recorded stabilization gate. No rebuild or
      resource probe may occur between that gate and measurement. Execute
      verified prepared binary bytes, retaining source/dependency checks.
- [ ] Define session settling and inter-process preparation policies. Account
      for FFTW planning, previous correctness replay, hashing, serialization,
      and calibration work. Deliberate warmup follows the declared steady-state
      protocol; startup remains a separate cold-analysis workload. Do not assume
      a quiet interval before compilation establishes either condition.
- [ ] Begin pilots with at least 180 seconds of settling after session
      preparation. Record timestamped host observations and calibrations near
      timing; use pilots to freeze measurable start/flag conditions. Absence of
      a thermal warning is not a temperature or frequency measurement. Bound
      gate waiting and retain failure/timeout evidence without killing services
      or repeatedly restarting until a favorable run appears.
- [ ] Support unattended serial execution without active interactive
      agent/editor work, builds, tests, or reporting during the campaign.
      Document the launch handoff; unavailable isolation remains a limitation.
      Do not automatically change system settings or terminate user processes.
- [ ] For paced runs, retain scheduled release, actual start, and finish times;
      distinguish compute duration, wake-up lateness, and release-to-finish
      deadline misses. Use absolute deadlines and an explicit overrun policy:
      preserve logical sample order, never silently drop or rebase late work.
      Label catch-up execution. Include deliberately interleaved DSP in timing;
      keep synthetic cache-conditioning cost separately identified.
- [ ] Measure throughput in multiple sufficiently long chunks with original
      ordering retained. Use pilot drift and timer observations to select chunk
      duration, warmup, calibration placement, and pacing duration. Do not
      subtract timer overhead or time every butterfly in the primary loop.
- [ ] Record pre/post calibration and available host-state evidence. Keep all
      slow runs. A predeclared flag policy supports sensitivity analysis, not
      retrospective deletion or unsupported E-core/preemption attribution.
- [ ] Add runner tests proving preparation precedes settling, measured binary
      identity cannot change, required sleep assertions cover the campaign,
      unsupported policies are visible, and failed or partial runs cannot
      become valid confirmation evidence. Test pacing arithmetic/overruns with
      deterministic clock fixtures separately from hardware smoke execution.

Gate: a short unpaced and paced smoke can be reproduced with verified sleep
protection and power settings, explicit timing boundaries, no hidden work
after stabilization, and complete failure records.
No publication rerun starts before this gate.

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
- [ ] Reanalyze existing raw evidence without rerunning timings: headline
      values, hop peaks, phase imbalance, slow hybrid process, maxima, and
      exceedances. Keep inverse/chain misses in their own comparison boundary.
      Do not attribute interruptions from a host snapshot alone.
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
      code or profiles before attributing cost to a source-level loop.
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
      pair; report whether its per-element dispatch inflated surrounding cost.
- [ ] Add a matched batch/distributed pair using the current core's arithmetic,
      layout, cache policy, and output contract. Existing legacy controls also
      change layout and boundary work, so do not substitute them for this pair.
      Factor only the needed execution seam; avoid a general scheduler framework
      or a copied production implementation that can silently diverge.
- [ ] Add diagnostic stage-cost measurements and untimed operation traces.
      Instrumented results explain mechanisms; primary results use unchanged
      uninstrumented timing boundaries. Report instrumentation overhead and
      avoid summing isolated stage times as if they reproduce integrated cost.
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
- [ ] Start with 1/4 engine threads, 1/4/16 analyzers, D=64/256, 48 kHz,
      aligned/staggered phases, and a fixed background-DSP workload sweep.
      Add D=16 or higher rates as declared stress extensions after the pilot.
      Freeze identical background workloads across contenders; do not equalize
      total utilization separately and hide extra analyzer cost.
- [ ] Measure full engine-block duration, aggregate CPU cost, hop peaks,
      publication age, and budget misses. An analyzer-free engine is a useful
      overhead control, not a value automatically subtracted from observations.
      Verify engine barriers, worker policy, lifecycle locking, and FPU mode
      against retained Rack source. Do not assume ideal cost/thread-count scaling.
- [ ] Exercise a concurrent single consumer at representative 30/60 Hz rates,
      with deliberate stalls. Measure producer impact, consumed-spectrum age,
      skipped updates, and snapshot consistency. Preallocate diagnostic storage;
      do not add a second mailbox consumer or synchronize unsafely through the
      module's other fields. Separate headless consumption from actual rendering.
- [ ] Add controlled reset, freeze/resume, sample-rate, window, band, and geometry
      transitions. Separate lifecycle allocations and host-lock stalls from
      steady sample processing. Include silence decay under recorded FPU modes.
      Verify single-producer ownership during host-serialized reset/publication.
- [ ] Use deterministic concurrency tests and the existing mailbox TSan check
      outside timing. If claiming device underrun reduction, additionally run
      an audio-device experiment with recorded device/driver/buffer policy and
      overload telemetry. Headless exceedances alone support no audible claim.

Gate: correct complete modules and pinned Rack-engine comparisons run at
defaults and demanding settings, with host overhead and consumer behavior
explicitly separated from the core microbenchmark.

## Phase 5: Scheduling And Kernel Research

These are benchmark-only candidates first. Evaluate the following in order;
record a concrete result or a reasoned deferral for each before closing the
phase. Missing candidates limit claims about optimal granularity.

- [ ] Native sub-FFT scheduling: bounded native leaves, scheduled twiddles,
      permutations, reconstruction, and postprocessing. Include all copies and
      scratch costs; an opaque leaf is indivisible. Sweep a small predeclared
      set of leaf sizes. Compare against whole-native-FFT hybrid and butterfly
      scheduling without assuming either wins.
- [ ] Completion horizon: decouple H_c from H; test H/4, H/2, H, and immediate
      execution where supported. Define rounding for non-divisible H and
      1 <= H_c <= H. Preserve endpoint cadence, capture during idle phases,
      latching, cancellation, ownership, and publication at t_r+H_c-1.
      Update C++/Python delay contracts and ring-lifetime reasoning. Merely
      changing the quota denominator is not a complete implementation.
- [ ] Stage weights: evaluate preparation, reconstruction/magnitude, output,
      and dirty-band costs with scalar/SIMD and real module sinks. Disclose
      earlier M1 Pro tuning from [spec 010][weight-spec]. Use held-out sizes,
      hops, and states after choosing weights; observed p99 is not a WCET bound.
- [ ] Profile-directed kernel work: investigate ring-index arithmetic, scalar
      butterfly segmentation, magnitude/reconstruction, and coordinate mapping
      separately. Do not convert magnitude smoothing to power smoothing, change
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
      Validate predictions on held-out workloads without calling them WCET.

Gate: candidate tradeoff curves and numerical/scheduling evidence support
selection or rejection. A result favoring efficient batch or hybrid is valid.

## Phase 6: Pilot, Freeze, And Confirm On The M1 Pro

The primary host is the existing 10-core, 16 GiB M1 Pro. Record actual OS,
compiler, SDK, provider, power, and scheduling state for each new campaign.
Do not inherit the old campaign's software versions or host preparation claims.

Use the following focused study groups rather than a full Cartesian product:

| Group | Initial Pilot Coverage | Main Question |
| --- | --- | --- |
| Baselines | N=2048/4096/16384, H=1024, D=64, scalar unsmoothed, core/matched/native/hybrid | Do efficient baselines change the original result? |
| Granularity | Selected N/H, leaf size, H_c, and weights; scalar and independent four-channel | Which granularity and delay meet the requirement? |
| Scaling | D=16/64/256, fixed H and separately fixed H/N, supported precision | Which scaling relationship explains the result? |
| Modules | Actual defaults, extreme Fourier settings, independent smoothing and active ports | Does the complete shipped path behave like the core? |
| Host | Phase 4 thread/count/block/load/consumer cases | Does reducing bursts improve available host capacity? |
| Stress | Non-divisible hops, cold/live caches, silence decay, lifecycle events | Which limits or regressions constrain the recommendation? |

- [ ] Before the pilot, state hypotheses and primary outcomes for each group.
      Before confirmation, freeze practical thresholds in absolute time,
      miss-rate or sustainable fixed-load terms, together with an acceptable
      spectrum-age/cost budget. Define outcomes before seeing confirmation;
      avoid a decision based only on a favorable relative p99 reduction.
- [ ] Run at least two separately prepared pilot sessions for the primary
      comparisons. Review drift, multimodality, timer resolution, phase
      coverage, numerical errors, run duration, and disk/runtime requirements.
      Resolve implementation errors before freezing; preserve failed attempts.
- [ ] Freeze small explicit profiles, observation lengths, fresh-process
      repetitions, allowed session order seeds, flagging policy, aggregation,
      and stopping rules. Start from at least three confirmation sessions on
      separate days and five fresh processes per primary cell per session;
      pilot variability may require more. Short development repetitions are
      not substitutes. Record any scientifically justified design change.
- [ ] Make session order independently randomized within a frozen, recorded
      seed policy. The current freeze fixes a seed; extend validation to permit
      the predeclared session seeds while keeping workload membership and all
      measurement semantics fixed. Do not bypass freeze checks to change order.
- [ ] Size observation windows for the reported statistic. Retain effective
      hop counts, duration, quantile ranks, and process/session variation;
      hundreds of hops do not establish a precise rare-event probability.
      Keep descriptive tails when available observations cannot support more.
- [ ] Measure serially using Phase 1 preparation. No sources, dependencies,
      profiles, or instrumentation change during a freeze. Changes require a
      new pilot/freeze for affected comparisons, never metadata relabeling.
- [ ] Complete the frozen matrix, including unfavorable cells. Distinguish
      integrity failure from a valid but disturbed observation. Keep unfiltered
      primary summaries and predeclared sensitivity views; no winner-based
      early stopping, best-run selection, or silent omission.

Gate: checked, complete same-host confirmation evidence answers the declared
questions. If no practical benefit survives, narrow the paper rather than
manufacturing a favorable workload or requesting more hardware automatically.

## Phase 7: Evidence Handoff, Paper, And Production Disposition

- [ ] Create a new versioned data handoff with compact tables, raw bundles,
      exact measured source, dependency/SDK identities, profiles/freezes,
      observation policy, diagnostics, and explicit redistribution omissions.
      Preserve raw bundles outside directories removed by ordinary clean rules.
- [ ] Verify fresh extraction and deterministic statistical rederivation using
      bundled tooling. Generate every selected number/plot with a receipt.
      Review timeline, cost/peak/age, phase, process-variation, and host-load
      figures. Do not hand-edit generated TeX/SVG assets.
- [ ] Audit each headline against task equivalence, numerical coverage,
      uncertainty, timing regime, and practical threshold. Explain native or
      hybrid wins, extra cost, latency, storage, and unresolved disturbances.
- [ ] Revise the full paper around application requirements, prior-work delta,
      whole-pipeline mechanism, experimental questions, results, and a usable
      decision rule. Keep historical/inverse/chain detail in clearly identified
      appendices where useful. Consolidate repeated caveats without removing
      qualifications necessary to interpret individual comparisons.
- [ ] Recheck cited primary sources and notation. Exact-hop contracts and
      standard mailbox ownership alone do not establish algorithmic novelty.
      Do not transplant unverified reviewer speedups, latency lower bounds,
      perceptual thresholds, or optimality claims. Build and visually review
      the full manuscript and portable source export before final delivery.
- [ ] Record production disposition separately: retain current behavior,
      promote a verified optimization, or defer a candidate. Experimental
      superiority does not automatically change plugin defaults. Any promotion
      needs compatibility checks, DSP/SIMD tests, Rack build, relevant manual
      checks, and fresh measurements if its integrated path differs from the
      measured candidate. Publication-only success is not product integration.
- [ ] Prepare local deposit-ready artifacts and document the remaining external
      publication steps. Public upload or submission requires its own request.
      Archive this spec as COMPLETE only after required phases and the full
      report are verified; record deliberate research deferrals and claim limits.

## Validation Commands

Run commands from the repository root. The [contributor guide][contributing]
and [workflow][workflow] specify the Rack SDK, compiler, optional providers,
Python plotting environment, and TeX prerequisites. These commands describe
future implementation validation, not checks performed while writing this spec.
Run only relevant suites while iterating; run the full applicable gates before
measurement. Tests, builds, reports, and sanitizers must not overlap timing.

### Existing Checks

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
the M1 Pro confirmation requires the selected macOS providers to be available.
Rack/manual/device checks record actual versions, settings, and observations
here; an automated DSP pass is not a successful interactive session.

### Planned Study Commands

Phase 2 will add versioned, explicit-workload profiles named
`reliability-smoke.json`, `reliability-baselines.json`,
`reliability-granularity.json`, `reliability-modules.json`, and
`reliability-host.json` under `docs/whitepaper/benchmarks/profiles/`.
These files do not exist at spec creation. The following uses the existing
public CLI after those profiles and Phase 1's internal stabilization gate
are implemented. Each profile declares its fixture/measurement phase; a smoke
profile must never be relabeled as a pilot or confirmation.

All macOS timing launches below use the system `caffeinate` utility. Apply the
same wrapper to development/Catch2 and Rack-host benchmark commands used in
this study. Run on AC power with Low Power Mode off, keep the lid open, and
allow the wrapped runner to complete before closing its terminal. During
preparation, retain `pmset -g batt`, `pmset -g custom`, and `pmset -g assertions`
output, plus `pmset -g` for effective settings, to establish power source,
settings, and the wrapper's active assertions. Capture initial snapshots before
the stabilization gate; repeat after timing while the wrapped runner is still
active. Keep probes outside measured loops. Follow Phase 1's lifetime and
power-change checks.

```shell
python3 docs/whitepaper/benchmarks/bench.py setup --variant macos --build
python3 docs/whitepaper/benchmarks/bench.py plan --profile docs/whitepaper/benchmarks/profiles/reliability-smoke.json --variant macos --output .build/spec014-smoke-plan.json
/usr/bin/caffeinate -is python3 docs/whitepaper/benchmarks/bench.py run --profile docs/whitepaper/benchmarks/profiles/reliability-smoke.json --variant macos --output .build/spec014-smoke
python3 docs/whitepaper/benchmarks/bench.py check .build/spec014-smoke
/usr/bin/caffeinate -is python3 docs/whitepaper/benchmarks/bench.py run --profile docs/whitepaper/benchmarks/profiles/reliability-baselines.json --variant macos --output .build/spec014-pilot-01 --host m1-pro-16gb-local --session spec014-pilot-01
/usr/bin/caffeinate -is python3 docs/whitepaper/benchmarks/bench.py run --profile docs/whitepaper/benchmarks/profiles/reliability-baselines.json --variant macos --output .build/spec014-pilot-02 --host m1-pro-16gb-local --session spec014-pilot-02
python3 docs/whitepaper/benchmarks/bench.py check .build/spec014-pilot-01 .build/spec014-pilot-02
python3 docs/whitepaper/benchmarks/bench.py estimate .build/spec014-pilot-01 .build/spec014-pilot-02
python3 docs/whitepaper/benchmarks/bench.py report .build/spec014-pilot-01 .build/spec014-pilot-02 --phase pilot --output .build/spec014-pilot-report
```

The two pilot launches above occur in separate prepared sessions, not one
unattended command block. Capture truthful host notes and automated metadata.
Freeze only after recording the Phase 6 design decision; the rationale below
identifies that decision and is not a replacement for the recorded evidence.

```shell
python3 docs/whitepaper/benchmarks/bench.py freeze .build/spec014-pilot-01 .build/spec014-pilot-02 --profile docs/whitepaper/benchmarks/profiles/reliability-baselines.json --variant macos --output .build/spec014-baselines-freeze.json --rationale 'Spec 014 Phase 6 decision and checked pilot report record thresholds, durations, repetitions, session ordering, and retained cells.'
/usr/bin/caffeinate -is python3 docs/whitepaper/benchmarks/bench.py run --freeze .build/spec014-baselines-freeze.json --variant macos --output .build/spec014-confirm-01 --host m1-pro-16gb-local --session spec014-confirm-01
```

Repeat that launch on separate days with `confirm-02` and `confirm-03` output
and session names, using the implemented frozen order-seed policy. Use fresh
paths for failed restarts and new freezes for other groups or revisions. Update
this section with exact additional CLI arguments if implementation adds them;
never imply an unimplemented flag was executed. After all sessions pass:

```shell
python3 docs/whitepaper/benchmarks/bench.py check .build/spec014-confirm-01 .build/spec014-confirm-02 .build/spec014-confirm-03
python3 docs/whitepaper/benchmarks/bench.py report .build/spec014-confirm-01 .build/spec014-confirm-02 .build/spec014-confirm-03 --phase confirmation --output .build/spec014-report
python3 docs/whitepaper/benchmarks/bench.py select .build/spec014-report --output .build/spec014-selection.json
python3 docs/whitepaper/benchmarks/bench.py export .build/spec014-selection.json --output docs/whitepaper/generated/comparison-014
python3 docs/whitepaper/benchmarks/bench.py check-export docs/whitepaper/generated/comparison-014
python3 docs/whitepaper/benchmarks/bench.py bundle .build/spec014-confirm-01 .build/spec014-confirm-02 .build/spec014-confirm-03 --selection .build/spec014-selection.json --output .build/spec014-evidence.tar.gz
python3 docs/whitepaper/benchmarks/bench.py unpack .build/spec014-evidence.tar.gz --output .build/spec014-rederived
make -C docs/whitepaper arxiv
```

Review the selection before export. Follow the extracted bundle's README to
rederive statistics and compare against the checked originals. Preserve a
verified copy of the new bundle outside `.build` before cleaning. After paper
integration, extract its source archive into an empty directory and run:

```shell
latexmk -pdf -pdflatex='pdflatex -no-shell-escape %O %S' -interaction=nonstopmode -halt-on-error fourier.tex
```

### Completion Checklist

- [ ] Phase 1: sleep protection, power settings, preparation, stabilization,
      pacing, and diagnostic boundaries verified.
- [ ] Phase 2: explicit workloads, new metrics, backward compatibility, and
      audits verified.
- [ ] Phase 3: efficient native and matched scheduling controls numerically validated.
- [ ] Phase 4: complete module, Rack-engine, consumer, and stress comparisons verified.
- [ ] Phase 5: research candidates evaluated or explicitly deferred with claim limits.
- [ ] Phase 6: preregistered practical criteria and complete controlled
      confirmation retained.
- [ ] Phase 7: portable evidence, revised full paper, and production disposition verified.

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

Record subsequent phase dates, decisions, exact commands/results, artifact
locations, manual checks, and limitations here. Do not create a separate
completion diary or mark the research complete merely because planning passed.

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

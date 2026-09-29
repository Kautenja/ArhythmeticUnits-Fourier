# External FFT Comparison Plan

Status: PLANNED. Updated September 29, 2026.

The next empirical comparison will measure Fourier against FFTW, Rack's
PFFFT implementation, and Apple Accelerate/vDSP. The question is whether
distributing analysis improves callback-duration tails enough to justify its
aggregate cost, storage, and publication delay when a fast batch FFT is
available. This plan extends the [measurement protocol](README.md); citations
and implementation readiness are separate from collected evidence.

## Selected Implementations

| Target | Distinct Value | Integration Route | Current Status |
| --- | --- | --- | --- |
| Fourier production core and same-arithmetic batch/incremental controls | Production behavior plus isolation of work placement within one FFT implementation | Existing `core-*` and `legacy-*` adapters | Implemented in the current suite; retain them in the comparison |
| [FFTW3](https://fftw.org/fftw-paper-ieee.pdf) | Portable optimized library with hardware-adaptive planning | Single-threaded real-to-complex float API, with double as a separate precision sweep; prepare and reuse plans | Cited; external adapter and dependency/build capture pending |
| [PFFFT through Rack](https://github.com/VCVRack/Rack/blob/v2/include/dsp/fft.hpp) | The FFT already available to a Rack plugin developer | Installed SDK's `dsp::RealFFT`, initially ordered real output; identify its linked PFFFT implementation | Wrapper inspected locally; external adapter and numerical validation pending |
| [Apple Accelerate/vDSP](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/vDSP_Programming_Guide/UsingFourierTransforms/UsingFourierTransforms.html) | Platform library on the Apple Silicon measurement host | macOS-only real FFT with reusable setup; link Accelerate and adapt packing/scaling | Documented API route; external adapter and numerical validation pending |

These names are planned targets, not accepted `--backend` values. Add adapters
to the existing protocol rather than creating a second timing harness.
PFFFT/Rack is one baseline: testing its wrapper and direct API does not make
them two independent algorithms. A direct-API variant is useful only if it
answers a specific scratch-storage or data-layout question, and must be
labeled accordingly. Record SIMD within a transform separately from Fourier's
four independent channel lanes.

[KISS FFT](https://github.com/mborgerding/kissfft) is deferred. Its documented
C real-transform API and Make/CMake build suggest modest integration effort,
but no local build or timing has been verified here. For this first matrix,
another portable batch implementation adds less distinct information than
the Rack, adaptive-library, and platform-library baselines plus our existing
same-arithmetic controls. Reconsider it for a specific question about small
dependency footprint, setup cost, or portability; do not add it solely to
increase the number of plotted series. It is not added to the manuscript's
bibliography in this revision.

The cited feedforward, hopping/windowed, and depth-first algorithms remain
follow-on candidates. Before measuring one, identify reusable author code or
specify a faithful implementation, supported hops/windows, precision, and
independent accuracy checks. A paper citation alone does not make an adapter
ready. Their status is implementation-feasibility review, after the library
baselines above; they are not silently represented by a batch FFT library.

## Matching And Acceptance

- [ ] Add each external adapter and explicit platform/dependency availability
    reporting. Unsupported targets must be reported, not replaced by another
    FFT. Start with the existing N=128/2048/16384 and H=257/1024 core/control
    sweeps; add N=4096 through a focused configuration. Use the shared float
    input and normalized periodic Hann window. Keep double results separate.
- [ ] Define two comparable measurements: real-transform execution including
    required packing/output conversion, and complete analysis including frame
    retention, windowing, magnitude processing, smoothing and K=N/2+1 outputs.
    Keep module/UI measurements separate. Charge all required reordering and
    DC/Nyquist handling; an unordered result is not a free substitute for the
    required spectrum. Compare four scalar channels with four SIMD lanes when
    measuring equivalent multi-channel work.
- [ ] Prepare plans and reusable working storage before timing. Report setup
    cost and persistent/scratch storage separately, including wrapper scratch
    behavior. Fix and record FFTW's planning flag, wisdom policy and thread
    count; start with a single-threaded `FFTW_MEASURE` plan without imported
    wisdom. Record vDSP setup capacity and OS/SDK versions, PFFFT revision and
    SIMD configuration, and each library's build/link provenance. Do not
    assume the current upstream fork matches the SDK's linked library.
- [ ] Verify the complete canonical complex spectrum against independent
    references on small sizes and selected bins at larger sizes, then verify
    matched processed outputs. Include DC, Nyquist, impulse, silence, tones,
    and deterministic noise. Establish tolerances from precision and the
    numerical contract before accepting results; do not require bitwise
    equality across different algorithms or widen limits after failures.
    Add the manuscript's long-stream error workloads before drawing conclusions
    about numerical drift or weak-signal accuracy.
- [ ] Verify actual window endpoints, hop cadence, startup and publication
    indices with the protocol's replay. Report a batch backend's earlier
    availability instead of forcing an artificial delay. Any common delayed
    publication policy must be a separately labeled experiment. Record matched
    supported configurations and explicit omissions for live-setting cases.
- [ ] Extend source/dependency capture and artifact validation for the external
    adapters, then run short correctness/smoke checks. Only afterward collect
    repeated serial campaigns on idle hosts across independent sessions, using
    the protocol's callback, throughput, analyzer-count and phase sweeps.
    Preserve raw observations, failed checks, compiler flags, metadata and
    source/library hashes. Label simulated budget exceedances separately from
    real device underruns; add a host experiment for device claims.

Current build and validation commands are in [Build And Run](README.md#build-and-run).
They do not yet execute the external targets. Adapter implementation must add
exact reproduction commands, dependency setup, and a resolved workload list
before marking any target ready. No new timing campaign was run for this plan.

## Planned Publication Outputs

Generate every table and figure from retained campaign data with a reproducible
script and source/workload identifiers. Keep existing historical manuscript
tables intact until the new evidence has been reviewed. Do not insert mock
numbers or use upstream benchmark charts as measurements of Fourier.

| Output | Contents And Comparison |
| --- | --- |
| Implementation table | Backend/revision, platform, compiler/options, precision, channel count, SIMD/thread policy, plan/setup policy, output format, setup cost, persistent/scratch storage and verified error |
| Matched workload table | N, H, callback block/rate, smoothing and analyzer count; per-frame/per-sample aggregate cost, callback p50/p95/p99 and observed maximum, simulated budget exceedances, and publication/visibility age |
| Cost-versus-size figure | Transform and complete-analysis panels with time units, identical workloads within each panel, and variation across independent sessions |
| Callback distribution figure | Empirical tail distributions at selected matched sizes/blocks, with observation counts and simulated compute budgets; preserve per-session variation and maxima |
| Cost-and-delay figure | Callback tail cost versus measured algorithmic publication age, with aggregate cost reported alongside; include analyzer scaling and aligned/staggered phases as separate series or panels |
| Numerical error figure, when supported | Error versus stream duration and signal level from the dedicated accuracy workloads, separated by precision; phase only where reference magnitude supports interpretation |

Choose repetition counts after a pilot estimates relevant variability. Report
effect sizes with uncertainty at the appropriate repetition/session level;
avoid treating every callback in one process as an independent experiment.
These outputs should make benefits, regressions, and crossover sizes visible,
including workloads where optimized batch processing is preferable.

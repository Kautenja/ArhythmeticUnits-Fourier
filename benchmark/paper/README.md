# Publication Measurement Protocol

This suite measures the current Fourier code for the scheduling paper. It
complements the Catch2 throughput benchmarks with individual observations,
matched frame controls, and a repeatable protocol for future FFT backends.
It does not update the manuscript's historical results or claim a speedup.

The [external comparison spec](../../specs/004-external-fft-comparison.md)
records the selected FFTW, Rack/PFFFT, and Apple Accelerate/vDSP baselines,
implementation status,
acceptance checks, and intended publication tables and figures. Those external
adapters are planned; the commands below currently run first-party backends.

## Build And Run

From the repository root, use Python 3 and the same Rack SDK/compiler needed
for the plugin. No external FFT library is needed. Build and check the
matched frame contracts independently:

```shell
make benchmark-paper-build
DYLD_LIBRARY_PATH="../.." LD_LIBRARY_PATH="../.." build/benchmark/rack/paper --verify
python3 -m unittest discover -s benchmark/paper -p 'test_*.py'
```

On Windows, put the Rack library directory on the DLL search path. The runner
sets the usual macOS/Linux library search variables for its child processes.
To validate every adapter quickly:

```shell
python3 benchmark/paper/run.py build/paper-smoke --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 benchmark/paper/check.py build/paper-smoke
```

For the paper matrix, first inspect the resolved workload list, then record a
campaign on an otherwise idle host. The output directory must be new:

```shell
python3 benchmark/paper/run.py build/paper-session-01 --profile paper --list
python3 benchmark/paper/run.py build/paper-session-01 --profile paper --notes 'Record actual power mode, affinity policy and competing activity here'
python3 benchmark/paper/check.py build/paper-session-01
```

The paper profile has 1127 distinct workloads. Defaults use seven fresh-process
repetitions per workload, at least 64 measured hops per streaming pass, 64
frames per aggregate transform pass, two frames per individual-step pass, and
64 warmup hops/frames. Full runs produce many raw observations and can take
substantial time. Build first and measure serially; do not run other benchmark
campaigns, builds, or tests concurrently. The runner forces a fresh build with
`make -B`, captures its commands, then runs one measurement process at a time.
`--rack-dir` and `--cxx` select the SDK and compiler for that build.

Repeat campaigns in new directories across independent sessions and machines.
Fresh processes within a session are not independent machine sessions. The
seeded job shuffle distributes mode/repetition order; metadata preserves the
actual order. `--seed`, `--repeats`, `--hops`, `--frames`, `--step-frames`, and
`--warm-hops` are explicit experimental settings, not automatic convergence
criteria. The short validation commands are not publication evidence.

## Workload Matrix

| Family | Factors |
| --- | --- |
| Production core and matched scalar controls | N=128/2048/16384, H=257/1024, smoothing off/on, blocks of 1/16/64/256 samples, plus a separately timed continuous throughput pass |
| Analyzer scaling | 1/4/16 instances, aligned/evenly staggered frame phases, 0/64 serial double DC blockers as fixed background work, callback and throughput passes at N=2048/H=1024 |
| Working state | Steady, startup with zero-padded input, alternating live window/band caches, or a 32 MiB cache-pressure sweep before each timed block |
| Actual modules | Fourier N=128/2048/16384, Spectre N=2048; H=1024, 48/96/192 kHz, mono/16-voice ports, smoothing off/on, blocks of 16/64/256 samples |
| Transform phases | FFT/RFFT/IFFT, float/double, N=128/2048/16384; buffering, butterfly work, RFFT final butterfly plus reconstruction, IFFT normalization, every individual step, full immediate and H=257 incremental frame execution |
| Inverse jobs | Batch/incremental complex IFFT, float/double, periodic spectrum release, all N output stores, callback/throughput, startup, instance count/alignment, background load and cache pressure |
| Frequency-domain filtering | Complex FFT, identity or three-tap transfer function, normalized complex IFFT, overlap-save extraction and sample delivery; matched batch/incremental, float/double and the same streaming factors |
| Driver controls | Same input traversal and background work, 1/4/16 rolling-checksum adapters, blocks of 1/16/64/256 samples, callback and throughput passes |

This is a set of factor sweeps, not the Cartesian product of every factor.
For a focused comparison or additional hops, block sizes, rates, or analyzer
counts, supply a JSON array of workload overrides. Omitted fields take the
baseline values in `run.py`; unknown fields are rejected. For example:

```json
[
  {"backend": "core-float", "n": 4096, "hop": 1024, "block": 64},
  {"backend": "legacy-batch-float", "n": 4096, "hop": 1024, "block": 64},
  {"backend": "legacy-incremental-float", "n": 4096, "hop": 1024, "block": 64}
]
```

Save that array as `build/paper-workloads.json`, then run:

```shell
python3 benchmark/paper/run.py build/paper-focused --config build/paper-workloads.json
```

`pass_name` is `callback` or `throughput` for streaming adapters and `complete`,
`incremental`, `phases`, or `steps` for transforms. Other overrides are `count`,
`alignment` (`aligned`/`staggered`), `load`, `smooth` (0/1), `voices`, `rate`,
`state` (`steady`/`startup`/`live`), and `cache_mib`. Spectre accepts only its
actual N=2048/H=1024. Fourier rejects requested hops that panel conversion
changes. Startup is aligned; other states permit offsets of floor(aH/count).
Counts, capacities, and CLI values are validated before measurement.

## Inverse And End-To-End Baselines

The standalone transform passes already measure inverse buffering, butterflies,
normalization, full execution, and individual steps. The following additional
adapters make inverse and complete frequency-domain operations explicit
streaming workloads:

| Backend Family | Operation |
| --- | --- |
| `inverse-stream-{batch,incremental}-{float,double}` | Release one of two alternating, predetermined non-Hermitian spectra every H logical samples; compute and store all N complex inverse samples |
| `ols-identity-{batch,incremental}-{float,double}` | Complex forward transform, N unity spectral multiplications, normalized inverse, and overlap-save output delivery |
| `ols-fir-{batch,incremental}-{float,double}` | Same path with frequency response of the causal FIR `[0.5, -0.25, 0.125]` |

These are first-party controls, not external-library adapters. Batch and
incremental variants execute the same arithmetic and required output stores.
Batch completes at the frame/release boundary. Incremental variants use a
balanced work quota and publish exactly H-1 calls later, waiting for the next
scheduled boundary. FFT input copying/permutation, inverse input conjugation,
and the chain's frame copy remain timed bulk operations. The inverse buffer
call inside the chain is one indivisible scheduled operation. These controls
do not establish a uniformly bounded complete synthesis pipeline.

Inverse jobs use analytically invertible complex spectra with DC and two
non-conjugate tone bins. Inputs are prepared before timing and alternate each
release; the common driver's scalar sample stream does not determine these
spectra. This isolates periodic inverse work without requiring an analyzer.
There is no synthesis window or audio playback for that family. General dense
complex spectra remain covered by the existing inverse transform passes.

The overlap-save adapters consume the common input as complex samples
`(x, -0.25*x)`. Each frame ending at jH transforms the latest N samples, applies
the selected transfer function, and retains the last H inverse samples. They
require H <= N-2 so that the three-tap FIR cannot circularly alias retained
outputs. Identity uses the same constraint and work path for comparison. No
window is applied: this is linear convolution through overlap-save, not a
windowed STFT synthesis or overlap-add experiment. Setup prepares the transfer
function outside timing. Timing includes input retention/conversion, forward
buffering/FFT, every spectral multiplication, inverse buffering/FFT/scaling,
valid-output extraction, buffer exchange, and delivery of one sample per call.
A common dependent checksum retains every delivered sample through the timed
interval; its cost is included. Construction and reference preparation are
excluded. No audio device or OS callback scheduler runs.

For a publication delay d, the valid output block represents input indices
jH-H+1 through jH and starts playback at jH+d. Algorithmic delivery delay is
therefore H-1+d samples: H-1 for batch and 2H-2 for incremental. This includes
block accumulation and computation scheduling, not driver/device latency or
the FIR's own phase response. Samples before the first publication are zero.
Compare full complex forward/inverse backends against this contract; real FFT
or complex-to-real specializations require a separately labeled workload.

The shared logic and independent verifier have no Rack dependency. Run the
Python tests with a C++11 compiler (`CXX` selects it); they compile and run the
standalone verifier without Catch2 or Rack:

```shell
python3 -m unittest discover -s benchmark/paper -p 'test_*.py'
```

The campaign executable still requires the Rack SDK because it also contains
the module adapters. The tracked 56-workload configuration covers all 12 new
adapters and eight isolated inverse passes. Use a new output directory:

```shell
python3 benchmark/paper/run.py build/paper-inverse-smoke --config benchmark/paper/configs/synthesis-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 benchmark/paper/check.py build/paper-inverse-smoke
python3 benchmark/paper/run.py build/paper-synthesis-session-01 --profile synthesis --list
```

The `synthesis` profile has 216 streaming workloads across N=128/2048/16384,
including selected H=257, 48/96 kHz, 1/4/16 instances, aligned/staggered
releases, startup, load, and cache pressure. It is also part of `paper`.
Smoothing, live analyzer controls, polyphonic voice summation, and transform
phase pass names are rejected for the new streaming adapters. Use the existing
`ifft-*` backends for isolated phase measurements. Smoke runs validate the
pathway; they do not support publication performance conclusions.

## Included Work And Comparability

`core-float`, `core-double`, and `core-simd4` use `SpectrumAnalysis` directly,
including windowing, butterflies, positive-bin reconstruction, prefix sums,
frequency/time smoothing, and K=N/2+1 output stores. The core prepares the
same maximum transform capacity (16384) at every selected N. Each SIMD
instance processes four scaled lanes; it is not one scalar analyzer.

`legacy-batch-float/double` and `legacy-incremental-float/double` use the
current `OnTheFlyRFFT`, with its full-complex reconstruction, a retained input
ring, frame copy, smoothing, EMA and K output stores. Both start a frame at
jH, using the same window endpoint and exact cadence as the core. The batch
control completes and publishes at jH. The incremental control publishes
when its budget finishes, then waits for the next jH; it never restarts early.
These are experimental adapters, not reconstructions of the old modules.
Batch versus incremental isolates scheduling of the same arithmetic. Comparing
either with the production core also changes storage, positive-bin
specialization, copying and cached smoothing bounds; it is not a pure test
of scheduling overhead.

All scalar controls use the same float input bytes, a normalized periodic
Hann window, and either no smoothing or one-third-octave smoothing with
alpha=0.8. Double promotes those same input bytes. Live workloads alternate
Hann/Blackman-Harris and smoothing off/on each frame, retaining the selected
alpha. Initial signal generation, plan/storage construction, and warmup are
outside timing. A startup pass skips warmup but still excludes construction;
use the existing module lifecycle benchmarks to measure allocation/setup.
The deterministic signal contains DC, two tones, and seeded noise.

`fourier` and `spectre` drive the actual module `process()` methods and every
port, including voltage writes, filtering, controls, lights, output mapping
and mailbox publication. They use the module's 100 ms time smoothing when
selected, rather than the core's fixed alpha. Polyphonic voices divide the
signal voltage before summation. There is no concurrent display reader,
rendering, engine worker, or audio device. These costs are different from
isolated transforms or magnitude-only cores and must stay separate in plots.

The background load is a fixed serial chain of the repository's double DC
blockers, independent of analyzer count. It models additional same-thread
DSP work, not asynchronous OS load. A driver control exposes traversal/load
cost, including a dependent checksum to retain every sample, but should not be mechanically subtracted from other results. Cache
pressure touches each cache line of the selected buffer before each interval;
that sweep is excluded from timing. It does not guarantee a particular cache
level was emptied. Throughput applies pressure once before the whole pass,
whereas callback passes apply it before each callback.

## Raw Records And Latency

Each process records 1024 empty clock intervals and either individual callback
observations, a continuous throughput interval, or transform phase/step
observations. The monotonic clock measures wall duration, including possible
preemption. Timer cost is retained and never subtracted. Single-sample and
single-step costs can approach clock resolution; use the separate throughput
pass for amortized cost. Records are allocated beforehand and written to CSV
after timing. Transform buffering is separate in phase passes and included in
both total-frame passes. RFFT reconstruction includes its final butterfly;
IFFT normalization includes output conjugation and division by N.

The streaming adapter is reconstructed for an untimed replay that polls actual
publications. It verifies phase and cadence and retains every publication's
sample index, endpoint age, window-center age, and age if consumed at the end
of its enclosing callback. Endpoint age is derived from the adapter's checked
window/schedule contract: H-1 for the core/modules, zero for the batch control,
and ceil(B/ceil(B/H))-1 for the incremental RFFT. Center age adds (N-1)/2.
Callback visibility adds the remaining samples in that block. These are
algorithmic sample ages, not measured device-to-screen latency; smoothing has
additional temporal response. Startup frames include zero padding. The replay
is distinct from the timed observations and cannot reveal timing-dependent
thread contention or dropped publications.

For inverse jobs, `endpoint_age_samples` measures release-to-publication delay
and `center_age_samples` equals it; no input-window midpoint is invented.
For overlap-save, the endpoint is the newest input in the valid output block,
and center age adds (H-1)/2, not (N-1)/2. The additive CSV column
`playback_delay_samples` records H-1+d for overlap-save publications and -1 for
families without audio delivery. `synthesis_contracts` in metadata identifies
these origins, output counts, operation, and normalization. The validator
checks these contracts and accepts older non-synthesis campaign CSVs.

CSV columns `sample` and `samples` count engine samples for streaming timing,
published sample indices for audit rows, and transform work indices/counts for
phase rows. `kind` distinguishes those units. The metadata summaries report
nearest-rank p50/p95/p99, observed maximum, mean, total, per-engine-sample cost,
and wall-time utilization against simulated audio duration. Callback compute
budget exceedances compare elapsed time with block/sample-rate. They are
**not device underruns**: callbacks run synchronously without a scheduler,
driver, deadline wakeups or sleep. Rare bursts may disappear from p99; retain
maxima, full distributions, and per-repetition variation. None is a WCET bound.

## Verification And Artifacts

Before a campaign, the executable checks matched scalar frame outputs and
publication delays across 48 scalar configurations against both controls
(96 pairings), including live settings. Each
streaming pass checks finite, meaningful output and replays exact cadence.
Transform passes verify work counts, incremental call counts and scheduling
agreement, compare 17 spread-out bins against independent long-double direct
sums (including DC/Nyquist), and check full forward/inverse round trips.
Their `.stderr` files retain absolute numerical error, reference scale,
round-trip error and work/call counts. Selected-bin references are not an
exhaustive numerical accuracy study; the deterministic DSP regression suites
remain necessary.

The pre-campaign `--verify` also checks inverse jobs and identity/FIR chains in
both precisions with batch/incremental scheduling. Every synthesis campaign
then independently audits each publication during its untimed replay. Inverse
outputs are checked against analytical complex signals. Every delivered chain
sample and every retained output block are compared against direct time-domain
identity/FIR evaluation at the correct delayed input index. These checks cover
all output samples, not only selected frequency bins. Per-run stderr records
maximum absolute error, reference scale, publication count, checked outputs,
and checked playback samples. The artifact checker rejects missing numerical
coverage, changed latency contracts, invalid errors, and incomplete records.

The standalone verifier additionally covers silence, boundary impulses, DC,
mixed/noisy signals, wraparound, non-dividing hops, the H=N-2 boundary, and
deliberate normalization/output corruption. Comparisons use absolute/relative
limits of 2e-5 for float and 1e-10 for double. This is validation of specific
fixtures and settings, not a general error bound or a synthesis quality study.

Each campaign retains:

-   Individual CSV observations and numerical reports for every repetition.
-   `metadata.json` with resolved workloads, commands/order, timestamps,
    source revision/dirty status, compiler, OS/architecture, host notes,
    per-run statistics and artifact hashes. Unavailable CPU information is
    marked explicitly; supply the actual hardware in notes when necessary.
-   `build.log` with exact optimization, architecture and floating-point flags.
    All adapters use the same SDK flags, including any unsafe math options.
-   An archive and hashes of the measured working source, a binary hash, and
    hashes of SDK headers, build rules and the linked Rack library. The SDK
    itself must be retained separately to reproduce its ABI and implementation.
-   `verification.txt` for the matched-frame checks. Failures leave an incomplete
    directory; only a fully successful campaign is marked complete.

`check.py` recomputes artifact/source hashes and all per-run summaries and
checks observation counts, publication cadence/ages, numerical limits, and
that every requested workload/repetition exists. It checks archived
sources rather than current checkout hashes, so later edits cannot redefine
what was measured. It does not establish an idle host or statistical adequacy.

Future optimized backends should implement the same adapter contract in
`protocol.hpp`: preallocate, process the same input and required outputs,
expose publication delay, and pass the matched frame/normalization checks.
Preserve each backend's precision, channel count, output contract, setup cost,
actual cadence and publication age. Do not compare an FFT-only routine with a
complete module. Worker-thread experiments will need a separate protocol for
job submission/completion, queueing and real callback deadlines; the current
synchronous driver cannot establish their behavior. GPU/display comparisons
also remain separate from these engine measurements.

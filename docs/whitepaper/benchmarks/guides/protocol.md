# Publication Measurement Protocol

For the maintained command sequence, start with the [workflow guide](workflow.md)
and [`bench.py`](../bench.py). This document is the detailed protocol reference.

This suite measures the selected Fourier source revision for the scheduling
paper. It complements the Catch2 throughput benchmarks with individual
observations, matched frame controls, and a repeatable comparison protocol.
Each archive identifies its measured sources; new runs do not update the
manuscript's historical results or establish a speedup automatically.

For routine optimization, use the [fast C++ development pathway](DEVELOPMENT.md):
`make benchmark-fast` runs a small fixed matrix, and `make benchmark-full`
checks a broader development matrix. They build incrementally, reuse this
protocol's C++ adapters and checks, retain raw observations, and compare
matching baselines. The Python runner described here remains the publication
archiver with fresh-process, resource-audit, and source/dependency archives.

The [external comparison spec](../../../../specs/archive/004-external-fft-comparison.md)
records the selected FFTW, Rack/PFFFT, and Apple Accelerate/vDSP baselines,
implementation status, acceptance checks, and intended publication tables and
figures. All three providers are implemented as benchmark-only adapters;
FFTW and macOS vDSP require explicit opt-in.
KISS FFT and academic overlap-reuse adapters are
[deferred for the current paper](../../../../specs/archive/004-external-fft-comparison.md#optional-contender-decision);
the decision records the evidence limits and conditions for reopening them.

## Layout

The C++ measurement suite and its source map live in
[`benchmark/paper`](../../../../benchmark/paper/README.md). The private [`lib/`](../lib/) directory owns
`run.py` (campaign orchestration and raw archives), `check.py` (artifact
validation), and `report.py` / `hybrid_report.py` (derived statistics and plots).
`observations.py` shares CSV validation, summary statistics and compact plot
data. Ordinary checking and reporting parse each CSV once and sort each timing
group once; reports reuse the verified artifact hash. Transition reports also
read callback coordinates to derive their event windows. Integrity checks,
original-order floating-point totals, raw files and command-line usage are
preserved.
`configs/` contains workload selections; `backends.json` describes capabilities.
The registry generators and optional `build_fftw.py` support benchmark builds.
Python tests here compile numerical verifiers from `test/paper/` as needed.

`experiments/` preserves the completed optimization studies and their recorded
artifacts byte-for-byte. These are historical reproduction sources, not active
benchmark cases. Keep new output in ignored `.build/` directories. Old campaign
archives retain their original paths and remain readable by `check.py`.

## Build And Run

From the repository root, use Python 3 and the same Rack SDK/compiler needed
for the plugin. No external FFT library is needed. Build and check the
matched frame contracts independently:

```shell
make benchmark-paper-build
DYLD_LIBRARY_PATH="../.." LD_LIBRARY_PATH="../.." .build/benchmark/rack/paper --verify
python3 -m unittest discover -s docs/whitepaper/benchmarks/tests -p 'test_*.py'
```

On Windows, put the Rack library directory on the DLL search path. The runner
sets the usual macOS/Linux library search variables for its child processes.
To validate every adapter quickly:

```shell
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-smoke --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 docs/whitepaper/benchmarks/lib/check.py .build/paper-smoke
```

For the paper matrix, first inspect the resolved workload list, then record a
campaign on an otherwise idle host. The output directory must be new:

```shell
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-session-01 --profile paper --list
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-session-01 --profile paper --notes 'Record actual power mode, affinity policy and competing activity here'
python3 docs/whitepaper/benchmarks/lib/check.py .build/paper-session-01
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

### Campaign Runtime Diagnostics

New runs record coarse wall-clock durations automatically and print a summary
after validation. Inspect the same summary later without rerunning experiments:

```shell
python3 docs/whitepaper/benchmarks/lib/runtime.py .build/paper-smoke
```

The runner separates build, archiving, preflight, resource probes, benchmark
processes, CSV summarization, metadata checkpoints, final integrity checks,
artifact hashing, and evidence validation. Its disjoint phases and per-job
events are retained in `metadata.json` under `runtime`, in nanoseconds. The
total starts after configuration resolution and ends after validation; it
excludes the final diagnostic save/print and separately invoked report/figure
generation. `finished_utc` now includes validation. A caught failure or
interruption retains the elapsed phases with failed runtime and invalid
campaign status; restart in a new directory. Forced termination cannot save
the in-memory runtime record.

Each measured process also writes a hashed `workload-*.runtime.json` sidecar.
These separate construction/planning, warmup, the measurement pass,
correctness replay, teardown, and CSV serialization. Native durations are
**inside** the runner's benchmark-process duration; never add the two levels.
The printed process launch/exit gap includes work outside the native scope,
such as process startup and sidecar serialization. `other` retains unclassified
native work, including transform teardown. Unused phases appear as zero.
For failed campaigns, the unaccounted process duration also includes failed
jobs that could not produce a complete native sidecar.

All new clocks and progress I/O sit outside individual measured intervals.
The native `measurement` phase includes outer-loop bookkeeping and deliberate
cache pressure; it is distinct from the sum of retained raw timing intervals.
Counts, warmups, provider planning, numerical coverage, and accuracy tolerances
are unchanged. This accounting identifies possible harness improvements; it
does not establish an algorithm speedup or replace repeated idle-host timing.
Short smoke runs disproportionately emphasize builds and startup, so do not
extrapolate their percentages to a publication campaign. Historical archives
without runtime diagnostics remain valid.

## External Campaigns And Reports

FR-10 supplies generated, tracked manifests for
[smoke](../history/configs/external-smoke.json), [pilot](../history/configs/external-pilot.json),
and [focused extensions](../history/configs/external-extensions.json). Their explicit
variants resolve these workload counts before process repetitions:

| Variant | Required Providers | Smoke | Pilot | Extensions |
| --- | --- | ---: | ---: | ---: |
| `rack` | Fourier, Rack/PFFFT | 156 | 270 | 537 |
| `portable` | Above plus FFTW | 206 | 339 | 706 |
| `macos` | Above plus Apple vDSP | 256 | 408 | 875 |

These are configuration inventories, not measured results. A variant excludes
only its declared providers; missing requested libraries fail. `macos` also
requires Darwin. The portable variants describe supported interfaces, not
verified builds on every architecture. Deferred contenders are absent.
Inspect the exact workloads, resolved contracts, precision/family counts,
independent channel counts, and declared omissions from the repository root:

```shell
python3 docs/whitepaper/benchmarks/lib/campaigns.py --check
python3 docs/whitepaper/benchmarks/lib/run.py --config docs/whitepaper/benchmarks/history/configs/external-pilot.json --variant macos --enable-vdsp --fftw-prefix .build/deps/fftw --describe-matrix
```

Use `rack` without feature flags, or `portable` with `--fftw-prefix`, on hosts
without vDSP. The FFTW prefix must first be prepared as described below.
Edit the generator in `campaigns.py` and run it to update tracked manifests.
The extensions cover four sizes, double, independent four-channel banks,
non-divisible hops, rate, load, cache pressure, startup/live changes, callback
size, analyzer count/alignment, and callback-origin offsets. They do not form
an unrestricted Cartesian product. Final matrix reductions require a recorded
rationale after the pilot.

For implementation validation, run a short smoke check:

```shell
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-fr10-smoke --config docs/whitepaper/benchmarks/history/configs/external-smoke.json --variant macos --enable-vdsp --fftw-prefix .build/deps/fftw --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2 --host-id apple-silicon-validation --session-id fr10-smoke
python3 docs/whitepaper/benchmarks/lib/check.py .build/paper-fr10-smoke
```

Manifest pilot/extensions runs default to phase `pilot`; legacy arrays and
smoke manifests default to `smoke`. `--phase confirmation` labels a frozen
final configuration. Non-smoke execution requires explicit `--host-id` and
`--session-id`; smoke manifests cannot be relabeled as publication evidence.
Use a stable host identity and distinct labels for actual independent
sessions. Record power, thermal state, host activity and ordering context in
`--notes`. Labels alone cannot prove independence. The matrix inventory and
input configuration hash are archived with every campaign. The historical
FR-11 confirmation and its source identities are recorded in the
[owning spec](../../../../specs/archive/004-external-fft-comparison.md#fr-11-confirmation-evidence).
FR-11 scalar numerical-audit parity, FR-12 parameter transitions and FR-13's
experiment-to-paper workflow are implemented and smoke-validated. Use the
[workflow handoff](workflow.md) for the maintained public command sequence,
including setup, progress/logs, failure recovery, checked exports and bundles.
[Spec 012](../../../../specs/012-comparison-evidence-and-paper-integration.md)
now owns pending replacement measurements and paper integration, transferred
from FR-14. Historical confirmation status does not transfer to new code.
Retain earlier results until the
[replacement and retirement gate](../../../../specs/archive/004-external-fft-comparison.md#replacement-and-retirement-gate)
passes; new campaigns use separate source identities and output directories.
Completing or archiving the implementation spec does not trigger cleanup.

Generate checked evidence tables and scientific SVG/PNG figures in a separate,
new directory. Matplotlib is an optional reporting dependency, isolated from
the plugin and benchmark executable:

```shell
python3 -m venv .build/paper-report-env
.build/paper-report-env/bin/python -m pip install -r docs/whitepaper/benchmarks/report-requirements.txt
.build/paper-report-env/bin/python docs/whitepaper/benchmarks/lib/report.py .build/paper-fr10-smoke --output .build/paper-fr10-report --phase smoke
.build/paper-report-env/bin/python -m unittest discover -s docs/whitepaper/benchmarks/tests -p 'test_*.py'
```

`--no-plots` generates tables/JSON with standard Python alone. The optional
plot fixture checks byte-identical SVG/PNG output in one plotting environment;
font/runtime differences across environments are not a reproducibility claim.
`results.csv` contains workload, cost, tail, channel, age, storage and error
fields; `process-timings.csv` exposes each process's timing and observation
window without pooling callbacks. Its `publication_audit_rows` counts the
separate untimed replay's checks, not jobs observed inside the timed interval.
`implementations.csv` joins native setup/storage/numerical evidence with source
and dependency provenance. `evidence.json` retains every process summary,
timer control, raw-data hash, numerical report and figure membership.
`report.md` explains interpretation and `manifest.json` hashes generated files.
The generator itself is hashed. Unknown native memory stays unknown.

Report generation validates input artifacts first, rejects mixed evidence
phases and duplicate workloads within a session, and separates host/source/
dependency/build identities, precision, family, operation and channel contract.
It requires three labeled sessions per workload for confirmation reports.
Costs average processes within sessions, then weight sessions equally;
vertical ranges show observed session means, not confidence intervals.
Callback CDFs remain per-process; quantiles, maxima, observation counts and
windows remain in JSON. Curves use a deterministic grid of at most 4096
points, while raw observations remain archived. Cost/age panels distinguish
spectrum-center age, inverse publication delay and chain sample delivery.
Callback-end visibility is retained separately. Simulated budget exceedances
are not device underruns, and observed maxima are not WCET bounds.

The independent bank adapters `core-independent4-float`,
`core-independent4-simd`, and `{pffft,fftw,vdsp}-analysis4-float` feed exactly
four distinct deterministic channel streams to four scalar instances or one
SIMD instance. Fixtures are precomputed outside resource/timing probes. Every
published bin of every channel is checked against independent recomputation,
including smoothing/live changes. `count` counts banks, so `count=4` means 16
independent channels. Resource probes describe one bank. The older
`core-simd4` scaled-lane control remains separate from these comparisons.
Existing scalar core/legacy analysis retains its preflight checks; reports
explicitly mark its absent per-run numerical summary instead of inventing an
error value. All of this code remains under `benchmark/`.

## Claims And Presentation

Choose the claim and matched workload before selecting a figure. Keep cases
favoring native batch execution, and distinguish practical comparisons from
controls that isolate scheduling. The intended mappings are:

| Question | Evidence Boundary | Figure Or Table |
| --- | --- | --- |
| Does distributing work reduce bursts? | Matched scheduled-batch/hybrid pair with shared arithmetic and storage | Cost versus callback p99, with publication delay and observed session ranges |
| Does suspending the FFT add practical value? | Core versus hybrid; arithmetic, layout and plan differences remain confounds | N/callback-size tradeoffs, including hybrid wins; no causal scheduling-overhead claim |
| How does the complete analyzer compare? | Core versus ordinary native analysis; equal windows, outputs and independent channels | Cost versus N, tails and spectrum age; scalar and four-channel groups separate |
| Are output and live-cache behavior correct? | Independent numerical/cadence checks and specified window/band changes | Accuracy/coverage table and steady/live comparison; report acceptance policy and oracle precision |
| Does the result extend beyond analysis? | Periodic inverse jobs and complete filtering chains | Separate cost/tail/delivery tables; preserve each family's time origin |
| Does it help the deployed modules? | Headless module processing plus separately recorded Rack observations | Application case study; identify host, patch, module settings and measured revision |

`state=live` alternates Hann/Blackman-Harris windows and disabled/one-third-
octave bands at frame boundaries. It stresses those cache transitions while
retaining N, hop and sample rate. It does not measure arbitrary mid-frame
control response, length changes, reset/sample-rate callbacks, display geometry
changes or visible UI latency. Cite separate tests or experiments for those
claims. Startup is a separate zero-padding condition, not a control change.

Core analysis ends at magnitude output stores. Actual headless modules also
include their input handling and engine-side display preparation/publication;
they do not run a display thread, graphics loop, audio device or Rack engine
worker. Their checks are not a substitute for the core's independent numerical
oracle. Module CPU cost, FFT throughput and visible display response answer
different questions. Module and correlated-lane fixtures are also distinct
from the independent four-channel comparison.

### Metric Definitions

-   **Mean cost:** average process costs within each session, then average
    session means equally. Keep callback and uninterrupted throughput passes
    separate. Streaming cost is ns per engine sample for all configured
    instances and included background work; transform cost is ns per transform.
-   **Serial audio-time cost:** `100 * cost_ns_per_sample * rate / 1e9`.
    This expresses synchronous work relative to simulated audio time. It is
    neither a measured Rack meter value nor whole-device CPU utilization.
-   **Callback p99:** compute each process's nearest-rank quantile, take the
    median within each session, then report the median of session medians.
    The accompanying range is the observed minimum/maximum of those session
    medians. The cost-versus-p99 figure uses microseconds for its p99 axis;
    the per-process CDFs retain distribution differences.
-   **Callback budget fraction:** `100 * duration_seconds / (block / rate)`.
    The denominator is the full callback interval, not an assigned analyzer
    allowance. A simulated exceedance is not an observed audio underrun.
-   **Observed maximum:** largest retained duration. Interpret it alongside
    process observation counts, frame-job counts and windows. The reported
    `observation_window_seconds` is the simulated audio span (engine samples
    divided by sample rate), not elapsed wall-clock time; the driver runs as
    fast as it can. Separate replay publication counts are not timed job
    counts. Unequal windows do not provide equally strong opportunities to
    observe rare events.
    Many idle sample calls can coexist with few FFT bursts, so p99 alone can
    miss the events of interest. A maximum is not a WCET bound.
-   **Algorithmic age:** samples converted to milliseconds using the workload's
    rate. Analysis uses spectrum-center age; inverse jobs use release-to-
    publication delay; chains use input-to-delivery delay. Callback-end
    visibility remains separate; none of these measures screen repaint delay.

Raw nanosecond/sample values remain available beside derived units. No timer
subtraction, callback pooling or inferential confidence interval is applied.
Three labeled sessions are a reporting coverage floor, not proof of statistical
independence. [Kalibera and Jones](https://kar.kent.ac.uk/33611/) motivate
repetitions at the build/process/iteration levels where variation arises and
effect-size intervals when their assumptions are justified.
[Mytkowicz et al.](https://sape.inf.usi.ch/publications/asplos09.html) show why
setup and layout bias can survive otherwise careful repetition. Our session
ranges are descriptive; they do not claim either source's full methodology.

For accuracy, [benchFFT](https://www.fftw.org/accuracy/method.html) motivates
independent references and vector-relative errors. Our binary64 large-frame
oracle is not arbitrary precision, and magnitude/smoothing errors do not
inherit a complex FFT's unitary error identities. Follow the
[versioned numerical policy](numerical-accuracy.md), retain weak-bin diagnostics,
and expose preflight-only coverage rather than inventing per-run error values.

Cross-architecture, battery-energy, worker-thread, overlap-reuse and hard
deadline claims require their own evidence. They are conditional follow-ups,
not extra contenders needed to rank the currently measured implementations.

## Shared Adapter Contracts

[`backends.json`](../lib/backends.json) is the canonical capability registry. Python
reads it directly; the optional benchmark build generates a C++ header from
it. Ordinary plugin and standalone DSP builds do not need that generator or
external FFT dependencies. Inspect available and planned backends with:

```shell
python3 docs/whitepaper/benchmarks/lib/run.py --inventory
DYLD_LIBRARY_PATH="../.." LD_LIBRARY_PATH="../.." .build/benchmark/rack/paper --inventory
```

Each descriptor declares provider, precision, channel count, transform/output
layout, normalization, supported sizes and settings, schedule, plan policy,
and dependency scope. Unavailable backend names fail before a campaign is created; they are not
substitutes for actual adapters. Use explicit operation/precision names. Add an
implementation and its correctness checks before marking a backend available.

The runner rejects unsupported or duplicate workloads before building. For
each workload it compares the executable's `--describe` result with Python's
resolved contract. Contracts distinguish input-frame endpoints, inverse
spectrum releases, and buffered transforms; they record output counts,
publication age, frame-center offset, playback delay, and plan policy. Only
explicit radix-2 implementations expose step counts. Opaque calls support
complete-transform measurement without invented butterfly or step counts.
Adapter dispatch and artifact validation use descriptor fields, not names.

The shared [`references.hpp`](../../../../benchmark/paper/references.hpp) supplies natural-order direct
DFT references, precision tolerances, and independent coherent-gain windowed
magnitudes. Preflight checks all bins of silence, shifted impulse, DC, Nyquist,
off-bin tone, and dense complex transform fixtures. Analyzer checks cover
zero-padded startup and alternating Hann/Blackman-Harris windows at a
non-dividing hop. Existing matched-control checks additionally cover smoothing
and live settings. Analytical inverse and direct time-domain filtering
references remain in [`synthesis.hpp`](../../../../benchmark/paper/synthesis.hpp). These fixtures supplement
the per-run numerical reports; they do not establish general error bounds.

## Setup And Storage Evidence

`benchmark-paper-build` produces an ordinary timing executable and a separate
`paper-audit` executable that intercepts C++ `new`/`delete`. Each workload gets
one untimed-by-the-campaign resource probe in each executable. Resource probes
measure one adapter, irrespective of the campaign's instance count, load,
alignment, or cache-pressure factor. A streaming probe runs from construction
for `2*N + 2*H` samples, including startup, output validation, and live settings
when requested; a transform probe executes two complete buffered transforms.
These observations are not callback/throughput results or repeated setup
performance estimates.

`resources-NNNN.json` separates ordinary setup/execution/destruction timings
from instrumented allocation observations. Setup retained heap bytes include
the adapter object, plans, buffers, and benchmark fixture storage; object size
is also reported separately and must not be added twice. Execution peak growth
over setup reports observed additional heap demand, not a complete scratch
size. Counts cover requested C++ allocation bytes, not allocator overhead,
RSS, native `malloc`/aligned allocators, or stack scratch. Native allocation
and stack fields remain `null` with a reason. Each future provider must audit
its own native allocation path before making a no-allocation or total-storage
claim. Residual bytes after destruction can reflect retained host allocations or
frees outside the interception scope; they are not automatically leaks.
Instrumented timings must not be used for performance comparisons.

Schema-2 campaigns retain both executable bytes, the compiled inventory,
resolved contracts, resource reports, loader identities, and an archive of Rack
headers/build rules/library bytes. The runner detects source or dependency
changes during the build and campaign. The checker compares archived dependency
bytes with their recorded hashes and resolves contracts from the archived
registry. System libraries are identified through loader output and OS metadata;
their bytes are not archived.

## Hybrid Scheduling Attribution

FR-6 adds `pffft-hybrid-float` and `pffft-scheduled-batch-float`. Both use
one shared positive-bin pipeline with identical arithmetic, exact-size PFFFT
plans, caches, N+H retained input slots, and task dispatch. The scheduled batch
runs all tasks at the frame endpoint; the hybrid distributes them across H
samples and publishes exactly H-1 samples later. The ordinary
`pffft-analysis-float` remains a practical batch control with bulk loops and
only N retained input slots.

For K=N/2+1, the shared sequence has W=N+1+2K tasks in dependency order:
N input/window stores, one native real FFT call, K magnitude/prefix sums,
and K smoothing/EMA/output stores. Sample s executes
`floor((s+1)W/H)-floor(sW/H)` tasks. The FFT executes at offset
`ceil((N+1)H/W)-1`; this call includes PFFFT packing, native work, and positive
complex output conversion. Its task weight of one is only a scheduling choice,
not a constant-cost operation or butterfly count. No timing bound follows.
Small H can put several stages on one sample; H>W permits idle quotas.

At each endpoint the adapter latches an index into its retained ring without
copying the frame. Extra H slots protect unread samples while the next hop's
inputs arrive. All input retention, quota calculation, preparation, magnitude
and prefix arithmetic, band averaging, EMA, and output stores are timed.
Live window and band cache rebuilds occur inside the corresponding tasks,
not in an O(N) boundary call. Setup prepares initial caches outside timing.
Publication occurs after the final store; partially written output is not
published to a consumer. These are single-threaded research adapters, without
a Rack display handoff.

The external batch and hybrid paths evaluate octave interval arithmetic in
double precision using their existing float octave factors. This avoids
float rounding at integer bin boundaries selecting different bins when one
loop is vectorized and another is scheduled a bin at a time. Magnitudes,
window coefficients, input, prefix sums and EMA remain float. The independent
oracle uses the same interval precision and independently sums each band.
Production/legacy bounds retain their original arithmetic. Together with
production's different FFT layout, fused stages, maximum-capacity plans and
storage, this prevents a causal claim about production scheduling overhead.
No same-production-pipeline batch ablation is supplied or claimed here.

From the repository root with the usual Rack SDK (no optional library needed):

```shell
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-fr6-hybrid --config docs/whitepaper/benchmarks/history/configs/hybrid-smoke.json --list
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-fr6-hybrid --config docs/whitepaper/benchmarks/history/configs/hybrid-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 docs/whitepaper/benchmarks/lib/check.py .build/paper-fr6-hybrid
python3 docs/whitepaper/benchmarks/lib/hybrid_report.py .build/paper-fr6-hybrid .build/paper-fr6-attribution
```

The smoke matrix has 20 matched conditions, each with six backends: the new
pair, ordinary PFFFT batch, legacy batch/incremental, and production resumable
analysis. It covers N=128/2048/16384, H=1/37/257/1024/65536, startup,
steady/live smoothing, aligned/staggered banks, background load, cache pressure,
and different callback sizes/sample rates. Numerical/schedule fixtures also
cover all eight supported powers of two. This matrix checks implementation
coverage; it is not a publication pilot or confirmation campaign under FR-13.

The [attribution generator](../lib/hybrid_report.py) validates the archived campaign
before producing JSON and Markdown outside its evidence directory. It requires
all six controls for every matched workload and retains per-process costs,
raw callback tail summaries, ages, resource/plan records, and explicit
interpretations for four comparisons. Ratios use matched process repetitions;
callbacks are not treated as independent experimental repeats. The report
keeps the task dispatcher/storage comparison distinct from the practical
external-versus-production comparison. It neither subtracts timings to infer
FFT costs nor claims confidence intervals from these short smoke observations.

## External Library Workloads

Rack/PFFFT is available through the existing benchmark's Rack dependency.
Its seven float operations are `pffft-{fft,rfft,ifft,analysis,inverse,ols-identity,ols-fir}-float`.
See [provider details](pffft.md) for the pinned source/library
identities, canonical layouts, normalization, and stack scratch behavior.
Run its matched smoke configuration from the repository root:

```shell
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-fr3-pffft --config docs/whitepaper/benchmarks/history/configs/pffft-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 docs/whitepaper/benchmarks/lib/check.py .build/paper-fr3-pffft
```

FFTW is opt-in and requires both serial precision archives. The pinned local
build helper uses fresh verified source and build directories on every run;
existing source/object trees cannot silently contaminate its provenance:

```shell
python3 docs/whitepaper/benchmarks/lib/build_fftw.py --jobs 2
python3 docs/whitepaper/benchmarks/lib/run.py --inventory --fftw-prefix .build/deps/fftw
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-fr4-fftw --fftw-prefix .build/deps/fftw --config docs/whitepaper/benchmarks/history/configs/fftw-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 docs/whitepaper/benchmarks/lib/check.py .build/paper-fr4-fftw
```

Its fourteen names are `fftw-{fft,rfft,ifft,analysis,inverse,ols-identity,ols-fir}-{float,double}`.
Without `--fftw-prefix`, these capabilities are explicitly unavailable. Direct
Make builds opt in with `PAPER_FFTW_PREFIX=.build/deps/fftw`. Feature/configuration
stamps rebuild the registry and paper binaries when flags change; plugin and
standalone DSP targets inherit none of these flags or libraries. See
[FFTW provider details](fftw.md) for the pinned source, planning,
precision/SIMD choices, and standalone correctness commands.

FFTW uses single-threaded `FFTW_MEASURE`, no imported wisdom, and restored
inputs after planning. Reports retain the actual per-instance plan text and
process-global wisdom after measurement; a separate inspection process cannot
identify a measured plan. Some FFTW plans may allocate native execution
scratch. Counts and internal scratch bytes remain unknown to the C++ audit,
and this limitation is retained with each instance. Optional dependency
archives retain static-library/header bytes and any supplied build/source
provenance. Prefixes without that provenance are explicitly labeled.

On macOS, enable the system Accelerate/vDSP provider explicitly:

```shell
python3 docs/whitepaper/benchmarks/lib/run.py --inventory --enable-vdsp
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-fr5-vdsp --enable-vdsp --config docs/whitepaper/benchmarks/history/configs/vdsp-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 docs/whitepaper/benchmarks/lib/check.py .build/paper-fr5-vdsp
```

Its fourteen names are `vdsp-{fft,rfft,ifft,analysis,inverse,ols-identity,ols-fir}-{float,double}`.
Direct Make builds opt in with `PAPER_VDSP=1`. The runner can combine
`--enable-vdsp` with `--fftw-prefix`; omitted options explicitly disable their
features even when ambient Make environment variables request them. Non-macOS
hosts reject the vDSP option before building. Only the paper binaries link
Accelerate. Reports retain exact-size setup policy, split-layout conversion,
OS/build identity, loaded framework path, SDK selection and deployment macros.
Opaque setup/storage/allocation quantities and the system framework's missing
binary hash remain explicit limitations. See
[vDSP provider details](vdsp.md) for scaling and independent checks.

[`external.hpp`](../../../../benchmark/paper/external.hpp) provides the common immediate batch analyzer,
periodic inverse jobs, and complete overlap-save identity/FIR paths. The
analyzer stores only K positive bins before magnitudes, band averaging and
EMA; isolated real transforms reconstruct all N bins to match the existing
full-complex RFFT control. These different output boundaries are deliberate.
Live windows/bands, input retention, zero-padding, inverse normalization,
valid-output extraction, and the continuous dependent delivery sink are
included where applicable. Batch results publish immediately at frame/release
index jH; chain playback adds H-1 samples of buffering latency.

Untimed replay checks all required outputs. Small analysis frames use direct
DFT references; larger frames use the independently implemented first-party
complex FFT with direct band sums and EMA. They share the specified float
window bytes. Analyzer tolerances are 3e-4 float and 1e-10 double; inverse and
filtering tolerances remain 2e-5 and 1e-10. Isolated transforms check all bins
against the independent implementation, direct DFT probes, and a round trip.
NaNs cannot be hidden by maximum-error accumulation.

Each external run's numerical report retains `provider_instances` from the
actual timed instances, collected after measurement. Resource probes likewise
record their own `provider_info` outside setup/execution/destruction timing
and C++ allocation counting. Native plan choices and storage evidence therefore
belong to the instance that was measured. Transform resource scope is plans
and owned canonical output; caller input/window fixtures are prepared outside
the probe, matching first-party scope. Native aligned transfer buffers and
opaque plan overhead remain additional provider costs. The runner retains
available PFFFT source bytes as well as its wrapper and linked Rack library;
source identities do not prove that an opaque SDK binary was rebuilt from them.

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

Save that array as `.build/paper-workloads.json`, then run:

```shell
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-focused --config .build/paper-workloads.json
```

`pass_name` is `callback` or `throughput` for streaming adapters and `complete`,
`incremental`, `phases`, or `steps` for transforms. Other overrides are `count`,
`alignment` (`aligned`/`staggered`), `load`, `smooth` (0/1), `voices`, `rate`,
`state` (`steady`/`startup`/`live`), `cache_mib`, and `callback_offset`. Spectre accepts only its
actual N=2048/H=1024. Fourier rejects requested hops that panel conversion
changes. Startup requires aligned analyzers and zero callback offset. Other
states permit a common offset in [0,H), plus floor(aH/count) for staggered
analyzers. Protocol v2 appends that common offset to the executable arguments;
v1 and archived configurations without it retain zero-offset semantics.
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
python3 -m unittest discover -s docs/whitepaper/benchmarks/tests -p 'test_*.py'
```

The campaign executable still requires the Rack SDK because it also contains
the module adapters. The tracked 56-workload configuration covers all 12 new
adapters and eight isolated inverse passes. Use a new output directory:

```shell
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-inverse-smoke --config docs/whitepaper/benchmarks/history/configs/synthesis-smoke.json --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 docs/whitepaper/benchmarks/lib/check.py .build/paper-inverse-smoke
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-synthesis-session-01 --profile synthesis --list
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
families without audio delivery. `contracts` in schema-2 metadata identifies
these origins, output counts, operation, and normalization. The validator
checks these contracts; schema-1 campaigns retain their original validator.

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

Large-size external analysis replay uses a binary64 FFT oracle on matched,
already rounded frame bytes; small sizes retain direct DFT sums. The versioned
[numerical acceptance policy](numerical-accuracy.md) checks each spectrum's
relative L2 and Linf errors against the existing analysis budgets, preserving
all former pointwise violations as diagnostics. Measured adapters are unchanged.
The historical FR-11 pilot's reference/policy failures and their resolution
remain recorded
in the [owning spec](../../../../specs/archive/004-external-fft-comparison.md#fr-11-pilot-evidence).

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
-   An archive and hashes of the measured working source, both executables,
    and SDK headers, build rules, and linked Rack library bytes. Loader output
    identifies system libraries separately from archived project dependencies.
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

## Scalar Numerical Coverage And Interactive Transitions

New scalar `core-*` and `legacy-*` runs audit every published bin of every
instance during the separate replay, using an independent binary64 FFT or
long-double direct DFT. `all-publications-v1` records expected and checked
spectra/bins, reference precision, and interval arithmetic alongside the
existing `spectrum-norms-v1` acceptance policy. Timed callbacks, throughput
loops, and resource probes do not execute this reference. Reports write
`accuracy-coverage.csv` and `accuracy-coverage.md`; historical scalar archives
remain explicitly preflight-only.

The `interactive-v1` suite adds `transition_suite` and `transition_control`
workload keys. It supports `core-float`, `core-double`, `pffft-hybrid-float`,
and `pffft-scheduled-batch-float`. The native controls extend the scheduled
analysis pipeline with two prepared exact-size plans and maximum storage;
their recorded engine policy distinguishes them from fixed-setting adapters.
Ordinary native analysis, FFTW, vDSP, modules, inverse processing, and four
channels currently have no transition adapter and are rejected.

Requests specify complete desired settings before an input sample. A benchmark
host wrapper retains the latest pending request and applies it only at the
analyzer's frame boundary; it never cancels an active transform. Length changes
clear logical input and EMA history; other changes preserve them. A previously
completed old spectrum may remain visible until the next complete publication.
Only complete spectra enter the reference checks. This models the existing
production configuration API, without adding module or device behavior.

The versioned sequence exercises length increases/decreases, hop changes,
window and frequency/time smoothing, no-op requests, pending replacement, and
a final request with explicitly recorded response or no-response status. Initial
N is 128 through 2048, H is at least 8, and the runner rounds a minimum 26H
horizon to complete callbacks. It uses one startup instance and no warmup or
added load. Configuration and dirty-cache work execute inside timed processing;
plan/buffer preparation belongs to the separately recorded setup and memory
policy. Every process retains an authenticated `.transition.json` sidecar.

Raw CSV publication ages are independently checked against the variable
schedule. Transition reports retain per-request application/publication delays
in samples and milliseconds, per-publication numerical errors and frame
identity, and callback mean/p99/observed maximum in declared event windows.
The paired no-change workload submits the same requests with unchanged settings.
These are algorithmic response times and observed compute costs, not device,
display, or worst-case execution-time guarantees. Smoke data verify the harness;
comparative metrics require later prepared-host measurements.

From the repository root, use new output directories:

```shell
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-numerical-smoke --config docs/whitepaper/benchmarks/history/configs/numerical-smoke.json --repeats 1 --hops 4 --warm-hops 2
python3 docs/whitepaper/benchmarks/lib/run.py .build/paper-transition-smoke --config docs/whitepaper/benchmarks/history/configs/transition-smoke.json --variant rack --repeats 1 --hops 26
python3 docs/whitepaper/benchmarks/lib/check.py .build/paper-transition-smoke
python3 docs/whitepaper/benchmarks/lib/report.py .build/paper-transition-smoke --output .build/paper-transition-report --phase smoke --no-plots
```

# Native Baselines And Matched Controls

These additional backends prepare stronger analysis comparisons for the study.
Correctness and generated-code inspection have been checked; their names do
not establish a performance advantage. The user collects timings separately
on the isolated laptop. Existing adapter identities and historical results
retain their original meanings.

## Backend Selection

Every name below accepts the existing v2 protocol and explicit v3 controls.
`float,double` denotes two separate suffixes. Availability follows the compiled
provider registry; `--inventory` and `--describe` perform no measurements.

| Backend Family | Implementation And Comparison |
| --- | --- |
| `core-matched-{batch,distributed}-{float,double}` | Current production arithmetic, storage, caches and output stores; only work placement differs |
| `pffft-native-{batch,hybrid}-float` | Direct real input, ordered packed output, direct natural-bin magnitudes |
| `pffft-native-unordered-{batch,hybrid}-float` | Unordered transform plus explicitly charged `pffft_zreorder`; retains the extra buffer |
| `vdsp-native-{batch,hybrid}-{float,double}` | Window multiplication into even/odd split input, native split magnitudes and real-FFT scaling |
| `vdsp-native4-{batch,hybrid}-{float,double}` | Four rows in one `vDSP_fftm_zrip` call, with simultaneous endpoints |
| `fftw-native-{batch,hybrid}-{float,double}` | Direct aligned input, serial real plan-many, native output magnitudes |
| `fftw-native4-{batch,hybrid}-{float,double}` | Four independent rows in one serial real plan-many execution |

The core pair calls a compile-time placement seam in `SpectrumAnalysis`.
The public `process()` API still uses the distributed policy. Both controls
keep the production sparse/dense dispatch choice, arithmetic order, full
storage capacity, dirty-cache work, and K magnitude stores. Batch completes at
endpoint jH; distributed completes H-1 samples later. Cold/live and retention
fixtures require bitwise identical spectra between these two controls.

Native batch/hybrid modes instantiate the same pipeline and provider kernels.
Each channel retains N+H input samples; preparation reads contiguous spans
across at most one ring wrap. No frame-boundary snapshot copy is needed. The
four dependency stages count C*N window/packing units, one opaque native
transform/reorder unit, C*K magnitude/prefix units, and C*K smoothing/stores,
where K=N/2+1. Hybrid divides these units over H samples and dispatches a
contiguous segment at each stage boundary. Batch performs all units at jH.
These units have unequal costs: a native FFT remains an indivisible burst.

Initial window/band caches are dirty and rebuilt inside the first frame.
Steady-state warmup completes that work; startup includes it. V2 `live` toggles
window/band settings, while v3 declares fixed independent controls. Native
vector segment lengths may change rounding; each mode must independently pass
the existing spectrum-norm budget. DC, Nyquist, coherent gain, octave intervals,
EMA, and all output stores are included. Native bands retain the external
adapters' binary64 interval calculation; the current core uses binary32.

PFFFT has no new four-channel batched API here. Its old analysis adapter and
`pffft-scheduled-batch-float` / `pffft-hybrid-float` pair remain useful controls
for canonical-complex conversion and per-element dispatch. Their cache policy
also differs, so a comparison with the new pipeline is not a pure dispatch
ablation. Ordered versus unordered PFFFT includes all necessary natural-bin
reordering and its storage. FFTW plans use `FFTW_MEASURE`, one thread and no
imported wisdom; plan text and exported wisdom are retained per instance.

## Channels And Freshness

Native4 uses independent channel state and one batched transform. With v3
`fixture=independent`, all four providers/core controls receive the same four
distinct prepared inputs. Other v3 fixtures use the same input bytes in each
channel. V2 uses the established independent-four fixture. Compare native4
against `core-independent4-float`, `core-independent4-simd`, and the existing
provider `analysis4` controls with identical controls and fixtures.

For instance staggering, select a one-channel native backend with `count=4`
and `alignment=staggered`. The existing stream offsets instance i by floor(iH/4);
the replay retains every instance/channel endpoint and every output bin. These
instances consume offset versions of the common scalar fixture, not four
distinct channel fixtures. Their spectra have different freshness. A native4
instance remains internally simultaneous even when multiple such instances
are staggered. Channel count alone does not make these configurations a
matched input/freshness comparison; keep their counts, fixtures and endpoints
visible in reports.

## Preparation And Numerical Checks

From the repository root with the Rack SDK and the optional providers already
available, these commands build or check correctness without timing a workload:

```shell
make -j2 benchmark-paper-build PAPER_VDSP=1 PAPER_FFTW_PREFIX=.build/deps/fftw
python3 -m unittest discover -s docs/whitepaper/benchmarks/tests -p 'test_native.py'
DYLD_LIBRARY_PATH=../.. LD_LIBRARY_PATH=../.. .build/benchmark/rack/paper --verify
DYLD_LIBRARY_PATH=../.. LD_LIBRARY_PATH=../.. .build/benchmark/rack/paper --development --profile full --list
```

Omit vDSP off macOS and use the actual FFTW prefix. Missing providers are
reported; a skip is not a pass. The dedicated verifier checks all bins and
channels against independent references at small and large N, retained-frame
wraparound, H=1/37/257/65536, startup/live caches, and the explicit fixture set.
It observes zero C++ allocations during repeated processing. This does not
observe native `malloc`, opaque plan storage, provider scratch or workers.
The campaign's separate resource probes preserve those limitations and record
requested native buffer sizes. No universal real-time-safety claim follows.

New native replay reports use `native-all-channels-v1` and retain first/last
endpoint, publication and bin counts for each instance/channel. The checker
requires the spectrum-norm policy, provider layout/batching metadata and full
coverage. Reports export that coverage alongside the existing numerical tables.

## Separate Stage Diagnostics

Prepend `--diagnostic trace`, `--diagnostic stages`, or `--diagnostic overhead`
to a native backend's ordinary arguments. Diagnostics require one instance,
aligned continuous callbacks, no cache/background load or callback offset,
and at most 65536 samples. They write `native-stage-diagnostic-v1` JSON with
the configuration, resolved contract, provider plan/layout, per-stage sample
coordinates, independent numerical replay and channel coverage.

-   `trace` performs the work and stores stage units without reading a clock.
-   `stages` times each stage segment, with trace storage outside that interval.
-   `overhead` times empty brackets and performs the stage work afterward.

Primary callback/throughput paths instantiate an observer with no clocks or
trace storage. Diagnostic timing perturbs execution and excludes input ingestion
and scheduler work outside the stages. Do not sum its intervals to reconstruct
integrated cost or subtract empty brackets to claim corrected primary timings.
These diagnostics cover the new native pipeline; they do not attribute internal
production FFT butterflies. Synthetic test clocks are explicitly marked and
rejected by the diagnostic validator unless fixture acceptance is requested.

For example, this is an untimed operation trace after building:

```shell
DYLD_LIBRARY_PATH=../.. LD_LIBRARY_PATH=../.. .build/benchmark/rack/paper --diagnostic trace pffft-native-hybrid-float callback 128 37 16 1 aligned 0 0 1 20 0 48000 startup 0 0 v2 > .build/native-trace.json
```

Real `stages` and `overhead` runs belong to the user. On macOS they require the
same `lib/execution.py --record ... -- COMMAND` wrapper used by standalone
measurements, which holds `caffeinate -is`, checks power and settles the host.
Set `PAPER_DIAGNOSTIC_EXECUTION_PATH` to a separate output JSON filename;
timed diagnostics require it and retain per-process timer calibration and
calling-thread/FPU/readiness evidence there. The wrapper's `--record` file
retains the sleep/power guard separately. Retain those files and source/build identity
when interpreting a diagnostic. The study's final prepared offline launcher
and pilot selections remain later phases; none of these preparation checks
collects performance results or updates the paper.

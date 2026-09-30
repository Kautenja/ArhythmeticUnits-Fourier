# Fast C++ Benchmark Development

Use this pathway to evaluate small FFT/analyzer changes without launching a
publication campaign. It reuses the same adapters, timing boundaries, output
stores, and numerical/cadence checks as `paper`. No production DSP changes are
part of the runner. Python remains available for publication archival and
scientific plots; it is not required to build or execute this pathway.

## Build And Iterate

Run from the repository root with the Rack SDK configured, using the same
compiler and SDK flags for baseline and candidate:

```shell
make -j2 benchmark-dev-build
make benchmark-fast
make benchmark-fast BENCHMARK_DEV_ARGS="--backend core-float --pass throughput"
make benchmark-fast BENCHMARK_DEV_ARGS="--list"
make benchmark-full
make test-benchmark-dev
```

`benchmark-dev-build` compiles only the timing executable and native registry
generator. Make tracks headers and build options and reuses unchanged objects.
The run targets serialize their selected profiles even with `-j`; unrelated
builds, tests, campaigns, and applications can still compete with timing.
Finish compilation first and measure on an otherwise idle host.

| Profile | Workload Selection | Default Observation Policy |
| --- | --- | --- |
| `fast` | 24 workloads: six matched float analyzers at N=2048/H=1024/B=64, smoothing off/on, callback and throughput | Three shared-process repetitions, 32 measured hops, eight warmup hops |
| `full` | Fast cases plus N=128/512/8192/16384, H=37/257/509, live/startup states, staggered banks/load, independent scalar/SIMD channels, modules, inverse/filtering paths, and compiled transform providers | Three shared-process repetitions, 128 measured hops or transform frames, 32 warmup hops |

The six primary analyzers are the production core, legacy batch/incremental,
ordinary PFFFT batch, and the matched scheduled PFFFT batch/hybrid pair. The
full profile contains 210 workloads with Rack/PFFFT alone and 240 with both
optional FFTW and vDSP enabled. Its additional sizes and hops provide checks
outside the fixed tuning set; once used for tuning they are no longer unseen
validation cases. Keep additional configurations or another host for final
confirmation.

Fast mode runs a focused independent scalar preflight; full mode runs the
existing complete preflight. `--verify` also enables the complete preflight
in fast mode. Every workload retains its existing untimed replay and checks.
Scalar core/legacy timing retains preflight-only numerical coverage; external
and independent-channel adapters additionally retain their per-run numerical
reports. No accuracy budget is relaxed. There is no skip-verification switch.

## Compare A Baseline And Candidate

Use explicit new output directories. Make appends the selected profile:

```shell
make benchmark-fast BENCHMARK_DEV_OUT=.build/before
# Make the intended DSP change, then rebuild before measuring.
make -j2 benchmark-dev-build
make benchmark-fast BENCHMARK_DEV_OUT=.build/after BENCHMARK_DEV_ARGS="--baseline .build/before-fast"
```

For a focused baseline, use identical filters on both runs. A ratio below one
means lower measured cost for the candidate. Inspect variation across repeats,
callback distributions, and delay contracts as well as the average. A small
ratio difference in a short run is a reason to investigate, not a speedup
claim. Alternate baseline/candidate runs across separately prepared sessions
before drawing conclusions about small changes.

Comparison requires identical workload/observation/warmup settings, repetition
count, preflight policy, compiler/flags, backend registry, host identity, and
recorded dependency fingerprints. Source/binary identities may differ, as
expected for an optimization. Changed conditions fail instead of silently
comparing an unmatched subset. Missing or modified baseline raw/diagnostic
files also fail fingerprint checks. Output directories are never overwritten, and
failures leave `status: failed`; interrupted runs remain `running`.

Each directory contains:

-   `results.json`: exact workload contracts, actual shuffled order, individual
    repetition statistics, aggregates, source/build/environment identity,
    invocation, preflight policy, and wall time.
-   `summary.csv`: cost units, mean cost, observed repeat range, callback p99,
    and observed maximum interval. Mean costs weight repetitions equally.
-   `workload-*-repeat-*.csv`: unchanged raw timer, measurement, and publication
    records, written after timing.
-   `workload-*-repeat-*.stderr`: existing numerical/provider diagnostics when
    supplied by that adapter; empty files do not imply numerical error zero.
-   `comparison.csv`: candidate/baseline cost ratios and callback p99 ratios.
    Throughput/transform rows have no callback-tail ratio.
-   `preflight.txt` and `source.patch`: preflight outcome and tracked source
    diff. Untracked sources have fingerprints but are not archived here.

Fingerprints use FNV-1a-64 for change detection, not cryptographic evidence.
Source, executable and recorded dependency/build changes during a campaign
invalidate it. The development directory is not a self-contained publication
archive. Preserve it before `make clean`, which removes `.build/`.

## Select Work And Observation Windows

Filters match existing workloads exactly; they do not change their settings.
Use `--list` first. Direct execution requires the Rack runtime library path:

```shell
DYLD_LIBRARY_PATH="../.." LD_LIBRARY_PATH="../.." .build/benchmark/rack/paper --development --help
DYLD_LIBRARY_PATH="../.." LD_LIBRARY_PATH="../.." .build/benchmark/rack/paper --development --profile full --n 8192 --backend core-float --list
```

On Windows use `.exe` and put the Rack runtime DLL directory on `PATH`.
Make handles Linux/macOS library paths. Supported filters are `--backend`,
`--pass`, `--n`, and `--hop`. Observation options are `--repeats`, `--hops`,
`--frames`, `--warm-hops`, and `--seed`. `--label` records an experiment note.
Streaming windows round up to complete callbacks. Throughput measures that
same sample count in one interval. Timer observations are never subtracted.
Phase/individual-step experiments remain in the publication runner.

For custom workloads, `--config` accepts the existing frozen JSON array format,
including the confirmation configurations. Invalid fields, unsupported
providers, duplicate workloads, and empty selections fail before measurement:

```shell
make benchmark-fast BENCHMARK_DEV_OUT=.build/custom BENCHMARK_DEV_ARGS="--config docs/whitepaper/benchmarks/history/configs/external-confirmation-primary.json --list"
```

That particular array requires FFTW and macOS vDSP. Build and run with the same
explicit optional provider settings; prepare FFTW using its existing provider
instructions first:

```shell
make -j2 benchmark-dev-build PAPER_FFTW_PREFIX=.build/deps/fftw PAPER_VDSP=1
make benchmark-full PAPER_FFTW_PREFIX=.build/deps/fftw PAPER_VDSP=1
```

## Publication Interpretation

Development repetitions share one process and allocator/runtime state. Fresh
adapter construction and FFTW's explicit planning policy are retained, but
these repetitions are not independent processes or independent host sessions.
Callback rows are observations within a repetition, not statistical replicates.
Short-window p99 values and maxima can miss sparse transform bursts. Raw
records, observation counts, timer cost and mean-cost ranges remain available.

Keep canonical transform, complete analysis, and module costs in separate
tables. Canonical external transforms include copying, layout conversion and
normalization; real-transform workloads may reconstruct all N bins. They are
not native-library-only execution measurements. Compare scalar and SIMD using
the recorded independent channel counts. Only the matched scheduled PFFFT pair
isolates work placement with shared arithmetic and storage.

Optimize for the joint tradeoff between throughput, callback tails, publication
delay, memory, and accuracy. After freezing a candidate, use the
[publication protocol](protocol.md) to archive source/dependency bytes with
SHA-256, audit allocation/setup, launch fresh processes, and repeat separately
prepared sessions. The development results do not become confirmation evidence
by changing a label. Actual device underruns, GPU work, and worst-case execution
bounds remain outside these headless measurements.

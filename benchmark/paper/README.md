# Paper Benchmarks

C++ workloads comparing Fourier with FFTW, Rack/PFFFT, and Apple
Accelerate/vDSP. This directory contains the measurement code; campaign
selection, archiving, statistics, and plots live in
[the whitepaper tooling](../../docs/whitepaper/benchmarks/README.md).

## Source Map

The suite groups adapters by approach and shares matched workload code:

| File | Responsibility |
| --- | --- |
| `benchmark.cpp` | Command-line entry point, backend dispatch, and verification preflight |
| `fourier.hpp` | Production analyzer, batch/incremental RFFT controls, and FFT/RFFT/IFFT cases |
| `fftw.hpp`, `pffft.hpp`, `vdsp.hpp` | One canonical transform adapter per external library |
| `external.hpp` | Shared transform, analysis, inverse-job, and filtering cases for external adapters |
| `synthesis.hpp` | First-party inverse jobs and overlap-save filtering controls |
| `hybrid.hpp` | Matched PFFFT batch/hybrid scheduling and attribution controls |
| `channels.hpp` | Independent four-channel scalar/SIMD comparisons |
| `modules.hpp` | Actual headless Fourier/Spectre module cases |
| `protocol.hpp`, `backend.hpp` | Raw records, streaming loop, workload validation, and contracts |
| `resources.hpp` | Separate setup, execution, destruction, and allocation probes |
| `references.hpp`, `analysis_accuracy.hpp` | Independent numerical references and acceptance policy |
| `development.hpp`, `development_matrix.hpp` | Native fast/full development runner shared with the Rack benchmarks |

Template workloads remain in headers so each adapter uses the same measurement
loop without a virtual call in the timed path. `benchmark.cpp` builds as one
translation unit, preserving the existing optimization boundary. Standalone
verifier programs live in [`test/paper`](../../test/paper); their Python tests
live with the whitepaper tooling.

## Build And Inspect

From the repository root, with a configured Rack SDK and C++ compiler:

```shell
make -j2 benchmark-paper-build
DYLD_LIBRARY_PATH="../.." LD_LIBRARY_PATH="../.." .build/benchmark/rack/paper --inventory
python3 docs/whitepaper/benchmarks/run.py --profile smoke --list
```

These commands build the timing/allocation executables and list workloads;
they do not measure them. Existing executable paths and `make benchmark-fast`
/ `make benchmark-full` commands remain available. Substitute your SDK library
path for `../..`; on Windows, use the `.exe` suffix and put that directory on
`PATH`. Optional backends use `PAPER_FFTW_PREFIX=/path/to/fftw` and
`PAPER_VDSP=1` (macOS only).

For numerical verification without a timing campaign:

```shell
python3 -m unittest discover -s docs/whitepaper/benchmarks -p 'test_*.py'
DYLD_LIBRARY_PATH="../.." LD_LIBRARY_PATH="../.." .build/benchmark/rack/paper --verify
```

## Measure One Case

The executable writes individual observations to stdout as CSV and numerical
checks to stderr. For example, this measures one scalar production analyzer:

```shell
DYLD_LIBRARY_PATH="../.." LD_LIBRARY_PATH="../.." \
    .build/benchmark/rack/paper core-float callback 2048 1024 64 1 aligned \
    0 0 1 64 8 48000 steady 0 0 v2 > .build/core.csv 2> .build/core.stderr
```

Arguments are backend, pass, N, hop, block size, instance count, alignment,
background load, smoothing, voices, measured callbacks, warmup hops, sample
rate, state, cache MiB, callback offset, and protocol version. Use `--inventory`
for backend capabilities or prepend `--describe` to inspect a case's contract
without timing it. FFTW/vDSP/PFFFT cases use the same protocol.

For publication evidence, use the
[campaign runner](../../docs/whitepaper/benchmarks/README.md#build-and-run) to
retain source/dependency identities and repeated raw observations. Its report
scripts derive tables and plots separately. No benchmark command updates the
paper or its archived results automatically.

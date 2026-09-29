# Recorded Evaluation

This directory contains the September 28, 2026 campaign used by manuscript
version 1. Values were measured from the real production header, not a
separate FFT implementation. See [metadata.json](metadata.json) for full
source digests, compiler output, timestamps, and data hashes.

## Provenance

-   Source revision: `b52e49c548ae681ec491c6e8b5ed78e4f92fe34d`.
-   Host: Apple M1 Pro, 16 GiB RAM, macOS 26.6.2, arm64.
-   Compiler: Apple Clang 21.0.0 (`clang-2100.1.1.101`).
-   Optimization: C++11, `-O3 -DNDEBUG`; no fast-math flag.
-   Command, from the repository root:

    ```shell
    python3 whitepaper/experiments/run.py --cpu 'Apple M1 Pro' --memory-gib 16
    ```

The command initially wrote to `whitepaper/build/evaluation/`. Its four
CSV/JSON outputs were copied here after successful completion. The compiled
binary remains a build product and is not archived. The compiler reported
an existing unused-function warning for `Math::Window::names`.

## Files And Units

| File | Meaning |
| --- | --- |
| `verification.csv` | 22 precision/length summaries, 1320 checked schedules, and 6048 independently checked complex bins |
| `timing.csv` | 144 mode observations: four lengths, two horizons, nine repetitions, two modes |
| `summary.json` | Medians, full observed ranges, and paired bootstrap statistics |
| `metadata.json` | Source/data SHA-256 digests, environment, commands, and clock calibration |

Timing fields end in `_ns` and use nanoseconds. Each timing row summarizes
64 frames in each of two passes after 16 warmup transforms. `frame_ns` is
mean frame time in the aggregate pass; `call_p99_ns` and `call_max_ns` come
from a separate instrumented pass. `start_median_ns` includes preparation;
`finish_median_ns` includes reconstruction. In complete mode they describe
the same first call. `summary.json` takes medians across the nine rows for
a size/horizon/mode, not across pooled samples from different repetitions.

Paired ratios are incremental divided by complete within the same
repetition. Percentile intervals use 10,000 resamples of the nine pairs,
seeded with 20260928. They quantify within-session variation, not
cross-machine uncertainty or guaranteed worst-case time. The empty timer
pair had median 0 ns and p99 42 ns; sub-tick observations are not precise
measurements of individual butterflies.

`max_scaled_error` is the maximum absolute bin error divided by the larger
of one and the sum of absolute windowed input samples. Direct DFT checks
cover powers of two from 4 through 128. Larger lengths, through 4096, have
incremental-versus-complete equivalence checks only; their zero reference
counts must not be read as independently measured zero error. Both
precisions use float window samples, matching the RFFT interface. The
reference's `long double` has 53 significand bits on this host.

## Scope

The numerical fixtures include silence, impulse, DC, Nyquist, a sinusoidal
mixture, and a fixed-seed pseudorandom sequence, each with rectangular and
periodic Hann windows. Timing uses the pseudorandom fixture and periodic
Hann only, for one scalar float channel.

Both timing modes produce one transform every requested H logical calls.
An incremental result that finishes early is retained until the next frame.
The real modules instead restart at completion; the difference is analyzed
in the manuscript and deliberately excluded from this controlled comparison.
Construction and fixture setup are excluded, while the production RFFT's
packing allocation and final reconstruction are included. Consuming output
outside each timed frame prevents dead-code elimination.

The campaign did not reserve a CPU core or a real-time thread, control CPU
frequency, isolate the machine, or run a Rack session. It measures neither
whole-plugin deadlines nor optimized-library competitiveness. Do not use
these observations to claim a universal speedup or real-time safety.

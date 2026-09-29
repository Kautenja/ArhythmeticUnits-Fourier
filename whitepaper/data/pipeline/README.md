# Complete Pipeline Campaign

Campaign date: 2026-09-29 UTC (2026-09-28 America/New_York).

This separate campaign compares the original scalar analysis arithmetic with
experimental serial and overlapped schedules. It does not replace the
manuscript version 1 evidence. See the [archived design](design.md)
for formulas, stage definitions, exact latency, ownership, and reproduction.

## Host And Workload

Apple M1 Pro, 16 GiB RAM, macOS arm64, Apple Clang 21.0.0. All timing modes
use C++11, `-O3 -DNDEBUG -Wall -Wextra -pedantic`, scalar float, identical
deterministic noise, a normalized periodic Hann window, alpha=0.8, and 48 kHz.
The matrix covers N=1024,4096,16384; H=N/4,N/2; smoothing off/on; four modes;
and throughput, per-sample, and 16/64/256-sample block passes. Twelve
repetitions rotate mode order evenly. Each pass has 20 warmup and 64 measured
hops. The empty timer median/p99 was 41/42 ns; it is not subtracted.

## Selected Results

The table selects H=N/4 and one-third-octave smoothing. All times are us.
Each value is the median across 12 repetitions. Block maxima are the observed
maximum within each repetition; the range is the minimum/maximum of those
12 maxima. These are synchronous simulated blocks, not audio callbacks.

| N | Mode | Time Per Hop | Sample p99 | Sample Maximum | 64-Sample Block Maximum | Block Maximum Range |
| ---: | --- | ---: | ---: | ---: | ---: | --- |
| 1024 | `legacy` | 12.481 | 0.083 | 5.292 | 6.646 | 6.625-7.083 |
| 1024 | `stage_burst` | 11.450 | 0.042 | 9.584 | 10.062 | 9.792-15.541 |
| 1024 | `serial` | 11.752 | 0.084 | 0.208 | 3.500 | 3.416-8.500 |
| 1024 | `pipeline` | 13.086 | 0.084 | 0.188 | 3.750 | 3.708-8.542 |
| 4096 | `legacy` | 56.009 | 0.042 | 23.146 | 24.625 | 24.291-36.292 |
| 4096 | `stage_burst` | 50.327 | 0.042 | 45.021 | 45.751 | 43.125-57.625 |
| 4096 | `serial` | 51.392 | 0.084 | 0.312 | 4.875 | 4.792-9.167 |
| 4096 | `pipeline` | 55.145 | 0.084 | 1.396 | 5.229 | 4.000-18.917 |
| 16384 | `legacy` | 233.635 | 0.042 | 86.916 | 89.709 | 87.792-101.958 |
| 16384 | `stage_burst` | 219.376 | 0.042 | 192.563 | 195.375 | 191.667-205.625 |
| 16384 | `serial` | 223.195 | 0.125 | 5.313 | 7.396 | 5.500-18.667 |
| 16384 | `pipeline` | 235.982 | 0.084 | 6.146 | 9.125 | 5.917-27.917 |

At N=4096, H=1024, serial reduces the median observed 64-sample maximum
from 24.625 to 4.875 us (about 80%) while retaining the baseline endpoint age
of 1023 samples. Overlap publishes at age 4095 samples, an extra 64 ms at
48 kHz. At N=16384, H=4096, that additional delay is 256 ms.

The matched `stage_burst` control exposes the scheduling cost. At N=4096,
H=1024, smoothed serial takes 51.392 us/hop versus 50.327 for stage bursts;
overlap takes 55.145. Thus spreading the candidate arithmetic has overhead.
The original baseline takes 56.009, but that comparison also includes
positive-bin specialization, fewer copies, and prepared smoothing bounds.

Across all 12 size/hop/smoothing settings, median paired serial/baseline
ratios for total hop time span 0.920-1.212; overlap/baseline spans 0.986-1.585.
Scheduling can reduce peaks while increasing total CPU. The evidence favors
further investigation of the one-hop serial candidate. It does not justify
paying the four-hop candidate's latency and memory costs in the plugin.

The sample p99 often makes the bursty baseline look cheaper because its
expensive calls occur less than 1% of the time. Peak and block observations
must accompany quantiles. The 41 ns timer overhead also approaches the typical
sample costs. Observed maxima vary with preemption and other uncontrolled
host activity; they are not worst-case execution-time bounds. One campaign
on one architecture does not establish portability or audio reliability.

## Baseline Phase Costs

The original phase pass reports nine repetitions of 64 frame-mean timings
after 16 warmup frames. Values below are the median across repetitions, us.
The reconstruction column includes the final butterfly. Postprocessing
includes full-spectrum smoothing/EMA and copying the positive output.

| N | Packing/Windowing | Reconstruction | Postprocessing |
| ---: | ---: | ---: | ---: |
| 1024 | 0.643 | 0.814 | 4.367 |
| 4096 | 3.218 | 3.236 | 18.541 |
| 16384 | 11.979 | 13.235 | 69.664 |

## Artifacts And Verification

-   [metadata.json](metadata.json): Compiler commands, host declarations,
    timestamps, source/data hashes, timer overhead, and scope.
-   [timing.csv](timing.csv): 2880 per-repetition summaries from the full matrix.
-   [summary.json](summary.json): Medians, ranges, and paired ratios.
-   [baseline.csv](baseline.csv): 27 original phase measurements.
-   [verification.txt](verification.txt): 7,991,437 assertions in seven cases.

`python3 whitepaper/data/pipeline/check.py` verifies the archived source and
data hashes. These hashes describe the original experiment, not today's
production source. The original full campaign checker remains in the source
archive.
Individual per-call observations are not retained. See the owning
[specification](../../../specs/archive/002-complete-analysis-pipeline.md) for the full
test/sanitizer commands and the existing manuscript-check failure.

## Reproduce The Historical Experiment

The alternate schedules and benchmark adapters were removed from the active
source tree after selecting the one-hop schedule for production. Their exact
sources, including the original dependency headers and test source, are in
[source.tar.gz](source.tar.gz). Do not extract this archive over the working
repository: it intentionally contains older versions of some files.

From the repository root, using Python 3 and a C++11 compiler:

```shell
python3 whitepaper/data/pipeline/check.py
paper_source=$(mktemp -d /tmp/fourier-paper.XXXXXX)
tar -xzf whitepaper/data/pipeline/source.tar.gz -C "$paper_source"
cd "$paper_source"
g++ -std=c++11 -O3 -DNDEBUG -Wall -Wextra -pedantic -Isrc whitepaper/experiments/pipeline/measure.cpp -o /tmp/fourier-paper-measure
/tmp/fourier-paper-measure > /tmp/fourier-paper-timing.csv
```

The archived `run.py` also needs SCons and the repository's unchanged Catch2
header at `dep/Catch2/single_include/catch2/catch.hpp` to run its original test
suite. Preserve the old campaign metadata rather than attributing its
measurements to the production implementation. Current production validation
is recorded in [spec 003](../../../specs/archive/003-one-hop-spectrum-analysis.md).

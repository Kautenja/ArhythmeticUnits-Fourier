# Spec 014 Fresh Study Evidence

This is the current manuscript's descriptive evidence: 776 successful fresh
processes, 194 planned cells, two processes per cell in each of two sessions.
The cells comprise 97 distinct stream and 94 engine configurations; three
stream bridge cells repeat across groups. Both sessions ran on September 30,
2026 in America/New_York, on one Apple M1 Pro. They remain pilots.

## Provenance And Reproduction

`receipt.json` hashes every compact evidence file and the importer/validators.
`evidence.json` contains exact configurations, raw file identities, compact
per-process distributions, numerical audits and session provenance. Compressed
metadata and session records preserve the original bytes, including power,
sleep assertions, source/dependency/build identities and process snapshots.
`source.tar.gz` is the exact measured first-party source archive, copied from
the original campaign. `prepared-manifest.json.gz` preserves the sealed plan.

The original evidence is outside the repository:

```text
/Users/christiankauten/Fourier-benchmarks/spec014/pilot-01/
/Users/christiankauten/Fourier-benchmarks/spec014/pilot-02/
/Users/christiankauten/Fourier-benchmarks/spec014/pilot-01.handback.tar.gz
/Users/christiankauten/Fourier-benchmarks/spec014/pilot-02.handback.tar.gz
```

Each archive has an adjacent `.sha256` file. Exact archive hashes also appear
in `evidence.json`. These are local retained artifacts, not a public deposit.
The handbacks omit executables and SDK/provider archives; their identities
remain recorded. Keep the full original directories for independent checking.

From the repository root, rederive into a new directory:

```shell
python3 docs/whitepaper/tools/study_import.py \
  /Users/christiankauten/Fourier-benchmarks/spec014 \
  --output /tmp/fourier-study-014-rederived
```

The importer verifies both archive checksums, every original directory hash,
expected groups/configurations/repeats/order, source identity, numerical audits
and execution records. It reads data and never executes a benchmark. The output
is deterministic, including compressed metadata. Compare it with this directory;
README/interpretation prose is not part of the generated evidence receipt.

From compact evidence, regenerate or check all publication assets:

```shell
python3 docs/whitepaper/tools/study_paper.py
python3 docs/whitepaper/tools/study_paper.py --check
python3 -m unittest discover -s docs/whitepaper/tools -p test_study_import.py
make -C docs/whitepaper check
```

The publication generator writes every cell, process and session to
[full-grid CSVs](../../generated/paper-study/), as well as the selected tables,
macros and vector figures. It checks source hashes and never collects timings.
A fresh clone can check the compact evidence; raw-data rederivation requires
the original local session directories.

## Interpretation Rules

-   Summarize processes within each session, then weight the two sessions
    equally. With two processes, a session median of process quantiles is their
    midpoint. Session ranges are descriptive, not confidence intervals.
-   The primary burst statistic is the median of maximum whole-block durations
    in complete endpoint-to-next-endpoint hops. Partial edges are excluded.
    Shared blocks remain in each intersected hop. Analyzer-indexed peaks are
    aggregate block times and dependent observations, not extra replicates.
-   For the engine, this differs from the runner's retained
    `hop_block_peaks_ns`, which spans endpoint to publication and can give an
    immediate batch path a shorter observation interval. The publication
    importer derives the same complete-hop interval for every path from raw
    observations and replay endpoints. It does not mutate the old summary.
-   Instrumented block wall cost, process CPU, compute-budget exceedances,
    wake lateness and synthetic release misses are distinct. No independent
    throughput pass was collected. No audio-device underrun rate is available.
-   Logical consumer-age bounds use completed-block sample watermarks and
    exclude final drains. They are not wall-clock age during backlog or repaint.
-   Long decay retains separately flagged `module-decay-ftz-v1` vectors, including
    relative error up to one. Their absolute floor is an engineering allowance.
    They cannot substantiate ordinary relative-accuracy claims.
-   All original processes, excursions and unfavorable results remain. The
    three repeated bridge cells retain their group identities rather than
    silently increasing a comparison's independent sample size.

## Important Findings And Limits

Matched distribution lowers the session hop-peak summary in all 24
family/length/session baseline comparisons. Native kernels and horizons expose
tradeoffs that the old scalar-glue adapters could not establish. Four aligned
Fourier modules on one actual Rack engine thread reduce the typical hop peak
from about 890 microseconds (vDSP batch) to 423 (native hybrid) or 201
(production), against a 1,333 microsecond block budget. Production costs more
in that case. Missed synthetic releases do not have a consistent ranking.

The empty four-thread engine consumes roughly three CPU-seconds per second of
nominal audio. Worker activity and possible power-state effects confound an
interpretation of the one/four-thread contrast as pure barrier or parallelism
cost. Background filler counts did not locate a reproducible saturation point.
The extreme complete-module cases expose real compute-budget exceedances but
lack paced native counterparts at that shape.

The operator declared a quiet offline setup; AC, Low Power Mode and caffeinate
checks passed. Snapshots still show macOS service activity, including sandboxd
and Spotlight. No process was excluded on that basis, and no particular timing
excursion is attributed to a named service. Both sessions use one host, build
and date. Longer separate-day runs, matched paced/continuous comparisons,
calibration, native extreme module controls, native sub-FFT leaves and physical
host/device measurements remain distinct follow-up work.

A post-collection reader repair makes consumer-age bounds JSON-stable (lists
rather than tuples). It affects no recorded numeric value or measured C++ code.
The engine fixture now checks JSON round-trip equality. Raw archives and
sessions passed their original hashes after that repair.

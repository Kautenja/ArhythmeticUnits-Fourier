# Complete Analysis Pipeline Experiment

Status: COMPLETE

Historical note: production integration is tracked in
[spec 003](003-one-hop-spectrum-analysis.md). The original executable
experiment and its dependencies are preserved in
[source.tar.gz](../../whitepaper/data/pipeline/source.tar.gz); its former
source paths and commands below describe that archived snapshot.

## Goal

Measure the bursts remaining around the incremental FFT, then evaluate a
preallocated, exact-cadence experimental scalar analysis pipeline. Keep the
shipping modules and their patch behavior unchanged. The experiment belongs
under `whitepaper/experiments/pipeline/`, outside production DSP.

## Behavior And Requirements

-   Input frames end at sample indices `0, H, 2H, ...`; negative indices are
    zero padded. Output snapshots contain DC through Nyquist magnitudes.
-   Compare the existing RFFT, at controlled exact H cadence, with four
    overlapping stages: window/pack/permutation, butterflies,
    reconstruction/magnitude prefix sum, smoothing/EMA/publication.
-   Each stage of W operations receives
    `floor((s+1) W/H) - floor(s W/H)` operations on sample s of each hop.
    Test the counts, dependencies, frame contents, cadence, and latency.
-   Overlapped frames publish at `frame_end + 4H - 1`; a one-hop serial
    schedule publishes at `frame_end + H - 1`. Input storage must
    retain a frame throughout preparation without a whole-frame snapshot.
-   Preallocate all processing and mailbox storage. Construct and destroy
    only while producer and consumer are stopped. Publish by ownership
    exchange, with no full-frame copy at publication.
-   Compare identical candidate arithmetic executed in stage-end bursts to
    separate scheduling from positive-spectrum pruning and precomputation.
-   Cover windows, smoothing, silence, impulse, DC, Nyquist, tones, noise,
    wraparound, irregular hops, skipped display reads, and allocation counts.

## Non-Goals

No plugin migration, configuration changes on the engine thread, new patch
semantics, UI coordinate/color mapping, external FFT comparison, paper
novelty claim, or hard real-time guarantee. This evaluates the analysis-to-
magnitude-snapshot boundary; it does not measure Rack or graphics.

## Acceptance And Validation

From the repository root, with C++11, Python 3, and SCons installed:

```shell
scons test/dsp/test_analysis_pipeline.cpp
scons test
python3 whitepaper/experiments/pipeline/run.py
python3 whitepaper/experiments/pipeline/run.py --check
make -C whitepaper check
git diff --check
```

Retain raw repeated measurements, compiler/host/source metadata, numerical
checks, and a summary. Measure per-sample and simulated block distributions
separately from uninstrumented throughput. Record observed peaks as samples,
not worst-case bounds. Validate the mailbox with a concurrent consumer and
run sanitizer checks where supported. No Rack build is required unless
production code changes.

## Initial Evidence

2026-09-29 UTC: the baseline scalar harness (Apple Clang, C++11, `-O3`,
`-DNDEBUG`, 48 kHz, H=N/4, normalized periodic Hann, one-third-octave
smoothing, EMA alpha=0.8) measured nine repeats, each with 16 warmup and
64 measured frames. At N=16384, medians of frame-mean isolated phase times
were 11.615 us for packing/windowing, 12.817 us for the last butterfly plus
reconstruction, and 67.601 us for smoothing/EMA/positive-output copying.
These costs justify the experimental implementation. Timer overhead is
included; they are not module/callback measurements. Reproduction retains a
fresh baseline campaign alongside the candidate comparison.

## Completion Evidence

Completed 2026-09-29 UTC (2026-09-28 local). The shipping modules and DSP
headers are unchanged. The deliverable is a tested experimental implementation
and controlled evidence, not a Rack integration or manuscript revision.

### Decisions And Results

-   Fused preparation, reconstruction/prefix sums, and finishing eliminate
    frame-sized copies at input and publication. A retained N+H input ring
    and prepared SPSC snapshot slots make steady-state processing allocation
    free. Configuration remains an off-thread/stopped-lifecycle operation.
-   The serial variant applies the same balanced quota to the entire ordered
    work sequence, preserving H-1 endpoint latency with one workspace. The
    overlapped variant mixes four stage workloads per sample using four
    workspaces, at 4H-1 endpoint latency. Both preserve exact H cadence.
-   All three candidate schedules share their arithmetic. The stage-burst
    control separates scheduling overhead from positive-bin specialization,
    reduced copying, and prepared smoothing bounds.
-   Archived 2880 timing records and 27 baseline phase records, with source
    and data hashes, compiler/host metadata, and per-repetition distributions.
    See [the campaign](../../whitepaper/data/pipeline/README.md) and
    [the design](../../whitepaper/data/pipeline/design.md).
-   At N=4096, H=1024 with smoothing, the median per-repetition 64-sample
    block maximum was 24.625 us for the baseline and 4.875 us for serial.
    Serial kept the same latency and used 51.392 us/hop versus 56.009 for
    the baseline. Across all settings, serial total-time ratios ranged from
    0.920 to 1.212 (medians of paired ratios), so there is no blanket speedup.
-   Overlap did not justify its added latency and storage on this host.
    Serial is the better candidate for further integration work; neither is
    enabled in the plugin. Stage units have different elapsed-time costs,
    and observed maxima remain sensitive to preemption and timer overhead.

### Validation Actually Run

From the repository root:

```shell
scons test/dsp/test_analysis_pipeline.cpp
scons test
python3 whitepaper/experiments/pipeline/run.py --cpu 'Apple M1 Pro' --memory-gib 16
python3 whitepaper/experiments/pipeline/run.py --check
```

All passed. The new suite passed 7,991,437 assertions in seven cases. It
covers independent direct-DFT magnitudes, comparison to the original RFFT and
legacy adapter, all candidate schedules, zero padding and ring wraparound,
sizes 4 through 16384, H=1 and irregular/long hops, sample rates 44.1/48/96
kHz, smoothing/window fixtures, allocation-free processing, held snapshots,
and concurrent consumption. The full standalone DSP suite also passed.

Address/undefined-behavior and thread-sanitizer checks passed:

```shell
g++ -std=c++11 -O1 -g -pthread -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc -Idep/Catch2/single_include/catch2 test/dsp/test_analysis_pipeline.cpp -o /tmp/fourier-pipeline-sanitize
/tmp/fourier-pipeline-sanitize
g++ -std=c++11 -O1 -g -pthread -fsanitize=thread -Isrc -Idep/Catch2/single_include/catch2 test/dsp/test_analysis_pipeline.cpp -o /tmp/fourier-pipeline-tsan
/tmp/fourier-pipeline-tsan 'Concurrent consumer*'
```

ASan/UBSan covered all seven cases; TSan covered the concurrent consumer
case. Existing unused `Window::names` warnings remain. Local Markdown links,
reported timing rows, source/data hashes, and `git diff --check` were checked.
No Rack build, manual Rack session, or actual audio-callback measurement was
run because production code and module integration were not changed.

### Existing Unresolved Check

`make -C whitepaper check` fails its recorded source hash for
`whitepaper/experiments/evaluate.cpp`. That file was already changed by
`6c1b02d` (DSP layout/namespace consolidation) after the archived manuscript
campaign; this task does not modify it. The new campaign passes its own
provenance check. The old campaign metadata and checker are preserved rather
than relabeling historical timings as measurements of today's sources.

The paper's original checksum mismatch still needs a separate decision about
historical source pinning or a new manuscript campaign. It is not hidden or
counted as a successful check here.

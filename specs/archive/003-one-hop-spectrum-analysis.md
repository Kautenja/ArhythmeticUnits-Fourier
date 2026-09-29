# Production One-Hop Spectrum Analysis

Status: COMPLETE

## Goal

Use the tested one-hop work schedule in Fourier and Spectre. Keep production
DSP small, generic, and independent of Rack. Retain historical experiment
sources/data only as paper artifacts, without alternate runtime policies.

## Behavior And Requirements

-   One exact H-call frame schedule includes packing/windowing/permutation,
    butterflies, reconstruction/prefix sums, smoothing/EMA, and per-bin output.
    Publish only completed frames. Changes to controls latch at frame starts.
-   Maintain scalar and four-lane SIMD magnitudes, window coherent gain,
    frequency smoothing, channel independence, and panel/JSON identities.
-   Prepare maximum transform storage at construction. Changing FFT length,
    window, or smoothing must not allocate or trigger whole-frame work in
    `process`. Window/band cache updates share the sample budget.
-   Retain frame input in an Nmax+Hmax ring. Growing hop capacity on a host
    sample-rate callback remains a documented configuration cost.
-   Fourier keeps its existing freeze behavior: stop capturing input, continue
    analyzing retained input. Spectre pauses capture and analysis together.
-   Fourier publishes complete curve snapshots through SPSC ownership exchange;
    Spectre writes its existing column mailbox incrementally. Avoid new
    unsynchronized coefficient reads or whole-frame publication copies.
-   Reset/sample-rate changes cancel unfinished frames and clear DSP history.
    UI snapshots remain valid until replaced by a complete publication.
-   Exact cadence intentionally replaces restart-on-FFT-completion cadence.
    No saved parameter/port IDs, module slugs, or JSON meanings change.

## Validation

```shell
scons test/dsp/test_spectrum_analysis.cpp
scons test
make -j4
make test-spectrum-points test-display-lifecycle test-serialization
make inspect-displays
git diff --check
```

Add deterministic reference, hop/phase, reconfiguration, reset, allocation,
SIMD, mailbox, and freeze checks. Run ASan/UBSan on standalone regression
coverage. Keep historical timing claims distinct from the integrated plugin;
record remaining limitations and checks not performed. Preserve experiment
reproduction sources as a paper archive, not active production dependencies.

## Completion Evidence

Completed 2026-09-29 UTC (2026-09-28 local). Both shipping modules now use
`SpectrumAnalysis<T>`; no alternate research policies or paper dependencies
remain in production or its tests. No commit, installation, or release was
performed. Concurrent release-guidance edits to `AGENTS.md` were left intact.
Saved parameter/port IDs, slugs, JSON keys, and defaults are intact.

### Implementation Decisions

-   One quotient/remainder scheduler budgets W=M+B+2K units over exactly H
    sample calls. Per-bin output callbacks include Fourier's coordinate
    conversion and Spectre's column writes in the schedule. The FFT/window
    and positive magnitude conventions match the existing analysis path.
-   A maximum-size scalar twiddle plan and bit-reversal table serve all
    supported lengths. SIMD lanes share scalar twiddles rather than compute
    separate approximate trigonometric plans. Scalar double keeps its own
    precision. Window and octave-band entries rebuild only as their units run.
-   Configuration latches at frame boundaries. Reset cancels partial cache
    rebuilds safely; logical input/EMA clearing avoids O(N) clears in the
    sample path. The Nmax+Hmax ring retains frames until packing completes.
-   Fourier's preallocated curve mailbox replaces mutable shared vectors and
    the readiness flag. Spectre writes its existing mailboxes directly; the
    redundant engine-side full-spectrum history has been removed. The UI
    remains the history owner. Bezier preparation clamps the first segment's
    preceding index rather than reading before the curve.
-   The new kernel's output is unnormalized. Scalar/SIMD tests compare
    amplitudes divided by N with a 32-float-epsilon absolute bound, accounting
    for different rounding under Rack's fast-math flags. An initial fixed
    raw-bin tolerance failed for tiny high-frequency bins as N increased;
    it was an inappropriate scale-independent numerical contract. Independent
    double-DFT and original-RFFT comparisons remain separate regressions.
-   Historical comparison sources, tests, driver, and dependency headers are
    preserved in the paper's source archive, with their original hashes.
    The active experimental directory and experiment-only test were removed.
    The paper data is historical evidence, not a benchmark of this integration.

### Validation Results

-   `scons test` passed every standalone suite. The new production suite
    passed 1,934,764 assertions in seven cases: original-RFFT and independent
    DFT comparisons, exact cadence/output quotas, varying frame contents,
    ring wraparound, all window functions, band/size changes, cancelled
    caches, reset, frozen capture, rejected settings, and allocation checks.
-   `make -j4` built `plugin.dylib`, including the scalar and Rack SIMD paths.
-   `make test-spectrum-points test-display-lifecycle test-serialization`
    passed: 3,807,481 assertions in four curve/SIMD cases, 101 assertions in
    nine display/lifecycle cases, and 5,253 assertions in six serialization
    cases. Tests cover complete/held curve snapshots, allocation-free live
    controls, exact Spectre cadence, and freeze/resume. The existing concurrent
    mailbox stress test also passed.
-   ASan/UBSan passed all seven standalone production cases with the command
    below. No assertion or existing numerical tolerance was weakened in the
    established suites.
-   `make inspect-displays` passed the native OpenGL inspection after running
    outside the sandbox so macOS window services were available. Five draws,
    scale/zoom changes and context recreation reported zero GL errors.
    Rendered spectrum/spectrogram images were visually inspected. The first
    sandboxed process could not connect to window services and was stopped.
-   `make -C manual` rebuilt Fourier (18 pages) and Spectre (17 pages).
    Final LaTeX passes had no undefined citations/references or box warnings;
    the changed scheduling pages were rendered and visually checked.
-   `python3 whitepaper/data/pipeline/check.py` verified the historical source
    archive and all original data hashes. Extracting that archive into a fresh
    temporary directory and compiling/running its timing executable's clock
    mode succeeded. Local Markdown links and `git diff --check` passed.

```shell
g++ -std=c++11 -O1 -g -pthread -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc -Idep/Catch2/single_include/catch2 test/dsp/test_spectrum_analysis.cpp -o /tmp/fourier-spectrum-sanitize
/tmp/fourier-spectrum-sanitize
```

### Remaining Limits

Fourier can allocate a larger retained-input ring during a host sample-rate
callback. Spectre reset still clears/publishes the entire history. These are
lifecycle costs, not ordinary per-sample processing or live panel changes.
Callbacks, graphics, unequal task costs, and OS scheduling prevent hard
real-time or constant-elapsed-time claims. Existing Rack header deprecation
warnings and the standalone unused `Window::names` warning remain.

Native inspection is not a complete interactive Rack/DAW session or a device
callback benchmark. No new whole-plugin performance numbers are claimed.
The pre-existing manuscript-v1 source-hash mismatch was recorded in
[spec 002](002-complete-analysis-pipeline.md). The documentation follow-up
below resolves it against recovered historical sources without rewriting
the measured-source metadata.

### Documentation And Commit Follow-Up

On 2026-09-28 local, the user requested a commit followed by a math/algorithm
audit. Implementation commit: `9e43231`.

-   Both manuals now derive W=M+B+2K, balanced per-sample quotas, exact
    publication age, retained-input capacity, prefix-sum smoothing, and EMA
    conventions. Quotient/remainder pseudocode distinguishes full-transform
    reference listings from the scheduled positive-spectrum implementation.
-   Manuscript version 2 distinguishes the original FFT campaign, the
    complete-pipeline prototype campaign, and production revision `9e43231`.
    It adds the work-bound proof, buffer ownership/cancellation semantics,
    controlled prototype results, and production validation limits. Citation
    metadata is aligned; no plugin version or publication status changed.
-   Recovered the original experiment sources from local Git into
    `whitepaper/data/source.tar.gz`. Every file matches its original recorded
    SHA-256. The consistency checker now verifies historical archives rather
    than comparing old measurements to edited production files. It also derives
    the new prototype table from raw observations and checks its frame ages.
-   `make -C whitepaper check` passes for both datasets, references, links,
    original tables/plot, and the complete-pipeline schedule example.
-   The paper compiled in the built-in LaTeX editor and with
    `make -C whitepaper` (17 pages). `make -C manual` produced Fourier
    (20 pages) and Spectre (19 pages). Final passes contain no unresolved
    references/citations or box warnings. The paper and changed manual pages
    were rendered and visually inspected.
-   Extracted the original source archive into a fresh temporary directory,
    compiled its driver, and ran `verify` and `clock` successfully. The
    historical unused `Window::names` warning remains. No new performance
    campaign was run and historical timings were not reassigned to production.
-   No DSP changes were made during this documentation follow-up, so the
    implementation validation above remains its evidence. Concurrent Makefile,
    integration-test, and testing-guide edits belong to other ongoing work and
    were excluded from the documentation commit.

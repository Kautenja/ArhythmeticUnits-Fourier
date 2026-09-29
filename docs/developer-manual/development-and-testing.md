# Development And Testing

Run the commands below from the repository root. The two build systems
serve different purposes: SCons verifies standalone DSP, while Make builds
the VCV Rack plugin.

## Dependencies

For standalone tests, use Python with SCons, a C++11-capable compiler
available as `g++`, and the repository's Catch2 headers. SCons accepts a
command-line compiler override such as `scons CXX=clang++ test`; an environment
`CXX` alone does not override its default.
On macOS, `g++` may resolve to Apple Clang.

Initialize any recorded submodules after cloning:

```shell
git submodule update --init --recursive
```

Catch2 v2's single-header `catch.hpp` is currently tracked directly at
`dep/Catch2/single_include/catch2/`, although `.gitmodules` also records that
dependency path. Do not substitute Catch2 v3 without a deliberate
build/test migration.

The plugin requires a compatible Rack 2 SDK or prepared Rack source tree,
including `plugin.mk`, headers, dependencies, and the Rack library. Its
Makefiles also use tools such as `jq`; inspect the selected Rack build
files for platform-specific requirements. The default `RACK_DIR=../..`
supports a checkout under `Rack/plugins/Fourier`.

## Standalone DSP Tests

Run all suites:

```shell
scons test
```

Run one suite through its alias, which includes the source `.cpp` suffix:

```shell
scons test/dsp/test_fft.cpp
```

Every test `.cpp` is a separate executable with its own
`CATCH_CONFIG_MAIN`. The SCons aliases build and execute the suites and are
marked `AlwaysBuild`. To pass Catch2 options, build the executable target
and invoke it directly:

```shell
scons build_test/dsp/test_fft
./build_test/dsp/test_fft --list-test-names-only
```

Use names returned by that executable when selecting individual cases.
Other suites cover DFT, windows, circular buffers, math helpers, IEEE-754
behavior, and triggers. Add focused tests directly under `test/dsp/`;
SCons discovers new `.cpp` files without a hand-maintained suite list.

Prefer deterministic `SCENARIO`/`GIVEN`/`WHEN`/`THEN` or `TEST_CASE` checks.
For transform changes, test known signals and independent expectations:
silence, impulse, DC, bin-centered sinusoids, forward/inverse behavior,
and incremental versus complete computation as applicable. Include supported
length boundaries, hop scheduling, and window normalization. Select error
tolerances from the numerical contract rather than widening them until a
test passes. Seed randomized fixtures when used.

Tests compile as C++11 without `-O3`; benchmarks use `-O3`. Scalar DSP test
success does not prove Rack SIMD instantiations compile or behave correctly.

The dedicated DC-blocker suite checks float and double impulse/step responses,
DC rejection, Nyquist gain, transition-width configuration, and reset. Its
separate SIMD suite requires the Rack headers and uses the plugin compiler
flags, without linking the Rack library or creating an engine:

```shell
scons test/dsp/test_dc_blocker.cpp
make test-dc-blocker-simd
```

The SIMD suite compares independent scalar filters with all four Rack SIMD
lanes under different signals and settings, including reconfiguration and
reset. It is separate from `scons test` and runs in CI's Rack job.

## Continuous Integration

The [DSP tests workflow](../../.github/workflows/dsp-tests.yml) runs
`scons test` on pull requests and pushes to `main`, including merges. Pushes
to other branches do not trigger a separate run. It checks out dependencies
recursively and verifies the Catch2 header is present. Its three DSP jobs use
Ubuntu 24.04 with GCC, macOS 14 with Apple Clang, and Windows 2022 with
MSYS2 UCRT64 GCC. Windows uses MSYS2's SCons and Python to preserve POSIX
paths and GNU build tools. Each job runs the same `scons test` command.

A separate Ubuntu 24.04 x64 job downloads the official Rack 2.6.3 SDK,
verifies its pinned SHA-256 checksum, and builds the plugin with `make -j2 all`.
It then runs all five headless Rack targets: `test-dc-blocker-simd`,
`test-serialization`, `test-display-lifecycle`, `test-spectrum-points`, and
`test-module-amplitudes`. The SDK lives under the runner's temporary directory;
`RACK_DIR` selects it for both the plugin and tests. System OpenGL, X11, and
audio libraries satisfy the Rack library's runtime dependencies even though
the tests do not open a window or audio device.

On Linux, SCons disables Catch2 2.13.3's optional POSIX signal handler
because it requires a constant `MINSIGSTKSZ`, which modern glibc no longer
provides. The Rack test job passes the same define through `EXTRA_CXXFLAGS`,
along with `-pthread` for the concurrent display tests. Assertions and process
failures still fail the job; only Catch2's extra signal diagnostics are
unavailable.

New updates cancel older runs for the same pull request or branch, and
DSP jobs have a 15-minute timeout and the Rack job has a 20-minute timeout to
limit usage. A failure on one platform does not cancel the other platforms
or the independent Rack job, so their results remain available.

Rack CI currently covers Linux x64 only. Plugin builds and Rack integration
tests on macOS and Windows, benchmarks, and manual UI checks remain separate
validation steps. The headless tests do not replace an interactive Rack session.

## Coverage And Sanitizers

The [instrumentation workflow](../../.github/workflows/instrumentation.yml)
runs on pull requests, pushes to `main`, and manual dispatch. Four independent
Ubuntu 24.04 jobs run DSP/Rack coverage and combined ASan/UBSan with Clang 18.
Rack jobs use the same pinned Rack 2.6.3 SDK as the ordinary Rack CI job.
A separate macOS 14 job runs only the standalone mailbox suite under TSan;
it needs no Rack SDK or shared library. Failures do not cancel other matrix
jobs. Reports and full compiler/test diagnostics are uploaded for 14 days,
including diagnostics from failed runs. Coverage summaries also appear in the
workflow summary. No external reporting account or token is required.

From the repository root, use Python 3, SCons, and Clang on Linux or macOS:

```shell
python3 scripts/check-instrumented.py coverage dsp
python3 scripts/check-instrumented.py coverage rack
python3 scripts/check-instrumented.py asan-ubsan dsp
python3 scripts/check-instrumented.py asan-ubsan rack
python3 scripts/check-instrumented.py tsan mailbox
```

Rack runs additionally require Make and the normal Rack dependencies. They
honor `RACK_DIR` or `--rack-dir /absolute/path/to/Rack-SDK`. All commands accept
`--jobs 2` (the default). The script honors an environment `CXX` compiler
executable, defaulting to `clang++`. Coverage also needs matching
`llvm-profdata` and `llvm-cov`; select explicit executables with
`LLVM_PROFDATA` and `LLVM_COV`. On macOS the script finds Xcode's tools through
`xcrun` when they are not on `PATH`. For Ubuntu's versioned tools:

```shell
CXX=clang++-18 LLVM_PROFDATA=llvm-profdata-18 LLVM_COV=llvm-cov-18 \
    python3 scripts/check-instrumented.py coverage dsp
```

### Coverage Reports

The [LLVM source coverage](https://clang.llvm.org/docs/SourceBasedCodeCoverage.html)
reports are separate by test workload:

-   `build/reports/coverage/dsp/`: standalone DSP suites, reporting `src/dsp/`.
-   `build/reports/coverage/rack/`: all five headless Rack suites, reporting
    first-party `src/`, including DSP templates instantiated by the modules.

Each contains `html/index.html`, `summary.txt`, `coverage.lcov`,
`coverage.json`, merged `coverage.profdata`, raw profiles, `test-status.json`, and `run.log`.
Coverage builds use `-femit-all-decls` so an unused inline declaration in one
suite cannot mask the executed body from another suite when LLVM combines
binaries. Catch2, test fixtures, SDK headers, and system libraries are excluded
from report totals. Reports cover compiled functions, not every file in the
repository: uninstantiated templates and unlinked code are not measured.
The two percentages must not be added or treated as a combined whole-plugin
coverage figure. Rack coverage includes headless display calls but does not
establish interactive UI, OpenGL, or complete patch-loading coverage.

Every invocation removes that suite's old reports and raw profiles, reruns
its tests, and merges only the new profiles. Unique process and binary profile
names prevent parallel tests from overwriting each other. If tests fail,
available coverage is still exported with a failed-run label in the summary
and `test-status.json`; the command and CI job still fail. Build failures or
missing profiles can prevent report generation. Coverage reporting has no
percentage threshold yet; assertion failures, missing profiles, or LLVM
reporting failures fail the command.

### Sanitizer Scope

Combined [ASan](https://clang.llvm.org/docs/AddressSanitizer.html) and
[UBSan](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) instrument
both standalone DSP tests and the actual module code compiled into the
headless Rack suites. Diagnostics fail immediately, with UBSan recovery
disabled. The runner uses `halt_on_error=1` for both runtimes and requests
UBSan stack traces. Leak checking uses the platform runtime's default
(enabled on Linux); no project suppression list is applied. Logs live at
`build/reports/asan-ubsan/dsp/run.log` and
`build/reports/asan-ubsan/rack/run.log`.

[TSan](https://clang.llvm.org/docs/ThreadSanitizer.html) runs separately from
ASan/UBSan. `test/threads/test_display_mailbox.cpp` checks retained-slot
ownership, superseded publications, coherent concurrent payloads, monotonic
sequence values, and final publication delivery. Its bounded producer sends
100000 snapshots; the reader inspects held storage during publication.
The runner sets `halt_on_error=1:exitcode=66` and saves
`build/reports/tsan/mailbox/run.log`. This focused run checks the
single-producer/single-consumer mailbox contract. It does not establish race
freedom for module controls, Rack internals, or an entire engine/UI session.
The mailbox suite also runs without instrumentation in `scons test` or alone
with `scons test-mailbox`.

Instrumented objects and executables live under `build/instrumented/`,
separate from ordinary tests and plugin products. DSP coverage uses `-O0`;
Rack coverage retains SDK optimization and floating-point flags. Sanitizers
use `-O1`, debug symbols, and frame pointers, retaining Rack's configured
floating-point behavior. Rack instrumentation covers first-party code and
included headers, while the supplied `libRack` remains uninstrumented.
Passing these checks is not an instrumented Rack host run or a plugin build.
Use ordinary builds and manual Rack checks separately. Do not use these
slower builds for performance measurements or distribute them as plugins.

## Rack Plugin Build

With the default Rack layout:

```shell
make -j4
```

For a separate SDK, substitute its real absolute path:

```shell
make -j4 RACK_DIR=/absolute/path/to/Rack-SDK
```

The build produces a platform-specific `plugin.dylib`, `plugin.so`, or
`plugin.dll`. A build does not install it into a running Rack session.
Use the selected SDK's install/package targets only when that action is
part of the task; they are not necessary for a compile check.

## Choosing Validation

-   **DSP behavior:** Run the focused suite while iterating, then `scons test`.
    Build the plugin when changed headers are consumed by Rack, especially
    for template or SIMD changes.
-   **Module state or controls:** Build the plugin, test extracted reusable
    logic where feasible, and verify affected behavior in Rack.
-   **Graphics or interaction:** Build the plugin and inspect the affected
    module in Rack, including its module-browser preview where applicable.
-   **Documentation:** Check relative links, referenced paths and commands,
    Markdown structure, and `git diff --check`. No full build is required
    solely for prose changes.

Run the headless Fourier and Spectre save/load regressions with the same
Rack dependency and repository Catch2 headers:

```shell
make test-serialization
```

These checks use the actual modules. Fourier coverage includes all
combinations of run, fill, Bezier, and AC-coupling settings, plus missing-field
defaults. Spectre coverage includes round trips for every supported color map
and fallback to Magma for missing or invalid saved values, including wrong
JSON types and out-of-range integers. The `test/rack/` tests are built by Make
and excluded from the standalone SCons suites. The suite also checks
context-menu setting changes through Rack's history API, including undo, redo, unchanged selections, preservation of other module
state, and missing modules. It does not exercise the Rack UI or loading a
complete patch file.

Run the headless Spectre texture-lifecycle checks:

```shell
make test-display-lifecycle
```

These checks exercise the actual display with NanoVG and an instrumented
texture backend. They cover context recreation while frozen, widget deletion,
unrendered previews, repeated cleanup, texture-creation failure, and ownership
when switching between live contexts. They also compare cached image bytes
with the original full-image calculation, check cache invalidation and history
wraparound. Standalone mailbox concurrency checks run through SCons and the
focused TSan command above.
They do not create an OpenGL window. In Rack, also check that Spectre resumes displaying
its frozen history after closing and reopening a host-managed editor.

For an optional native OpenGL inspection on a graphical desktop, using a Rack
tree with its `res/` assets available:

```shell
make inspect-displays
```

This briefly creates a native window, renders both displays, recreates the
window/context, changes scales, and changes zoom. It checks framebuffer handles
and GL errors and saves `build/test/rack/display-*.ppm` for visual inspection.
It is not a complete interactive Rack or DAW session.

Run the headless Fourier spectrum-coordinate regressions:

```shell
make test-spectrum-points
```

These checks exercise the actual SIMD module across FFT sizes, sample rates,
frequency scales, magnitude scales, and slope settings. They verify channel
independence, silence, and coordinate mapping, including out-of-range bins.
They do not render the curves or measure performance.

Run the analytical amplitude regressions for both actual modules:

```shell
make test-module-amplitudes
```

These checks drive Rack input ports through `process()` and compare published
Fourier curve ordinates and Spectre column magnitudes with closed-form DFT
expectations for Boxcar, Hann, Hamming, and Blackman-Harris windows. Explicit
cosine-series coefficients supply independent coherent gains and neighboring
bin weights; separated signal components avoid overlapping window lobes.
They cover bin-centered tones, DC, Nyquist, per-port gains,
mono and polyphonic sums, cancellation, muted gains, disconnected inputs with
stale voltage storage, and AC-coupling transitions. AC expectations include the
10 Hz DC blocker's steady-state frequency response after settling. Fourier
runs at 128, 2048, and 16384 samples; both modules run at 44.1 and 96 kHz.
The oracle uses explicit 5 V input normalization and Fourier's historical
`N/2+1` and +12 dB display scaling, without deriving expected amplitudes from
module output or production DSP helpers. A focused input-path regression also
checks DC rejection in short repeating sixteen-voice signals at 44.1, 96, and
192 kHz, without involving FFT or display rounding. These are headless
numerical checks, not a Rack UI session.

There is currently no automated UI gate in this repository.
Record manual checks as manual; do not imply that
standalone tests exercised the module widgets or patch loading.

For an affected module, useful Rack checks include silence and disconnected
inputs, a known-frequency signal, input/channel independence, run/freeze,
reset, sample-rate changes, FFT/window/hop controls, smoothing, scale and
frequency bounds, and context-menu options. Check save/reload with existing
presets or patches when persistence changes. The `patches/` examples can
help, but may require other plugins or local sample assets. Record the Rack
version, sample rate, relevant settings, and observed result.

## Benchmarks

The standalone Catch2 v2 benchmarks cover every computational DSP header.
They build with C++11 and `-O3`, independently of Rack. Run from the repository
root with the same dependencies as the standalone tests:

```shell
scons -j4 benchmark-build
scons benchmark
scons benchmark/dsp/benchmark_fft.cpp
```

`benchmark-build` only compiles; `benchmark` and the `.cpp` aliases always run.
SCons excludes `benchmark/rack/`, which requires the SDK and Make. A command-line
compiler override, such as `scons CXX=clang++ benchmark-build`, applies to the
benchmarks too. Use the same override for the subsequent run.

| Suite Under `benchmark/dsp/` | Timed Work Per Iteration |
| --- | --- |
| `benchmark_fft.cpp` | One complex FFT, real FFT, or inverse FFT at N=128, 2048, or 16384, using float and double; complete and H=256 incremental schedules; RFFT with and without octave smoothing; separate float plan construction |
| `benchmark_dft.cpp` | One caller-buffer DFT (Boxcar/Hann) or IDFT at N=32 or 128, float and double |
| `benchmark_spectrum_analysis.cpp` | One production analysis hop at N=128, 2048, or 16384 and H=257 or 1024, float and double; smoothing off/on; separate steady and live window/band cache rebuild workloads |
| `benchmark_window.cpp` | 2048 periodic coefficients for every selectable and parameterized window, float and double; cached multiplication and alternating cache replacement at N=128, 2048, and 16384 |
| `benchmark_processors.cpp` | 1024 DC-filter samples (float/double), circular-buffer insert/read operations, or trigger ticks; separate contiguous frame copy |
| `benchmark_math.cpp` | 1024 numeric, complex, voltage, interpolation, pitch, formatting, or color-map operations |

Compile-time constants, enum names, trivial accessors, and invalid-input/error
paths are not separate performance targets. These are representative workloads,
not a benchmark coverage percentage or a replacement for correctness tests.

Inputs combine two tones, DC, and a fixed-seed noise sequence. Generation and
storage preparation stay outside timing, except explicitly labeled plan
construction and string formatting, which include allocation. Transforms time
input buffering and all output work; incremental transforms restart every
iteration. Window/band rebuilds alternate settings every iteration so calibration
and repeated samples cannot turn into cache hits. Streaming filters and spectrum
analysis warm up before timing and retain state between iterations. Output
callbacks write all spectrum bins. Compiler barriers make block inputs and
outputs observable without adding a volatile operation to each sample. Block
measurements include traversal and output stores, not just scalar arithmetic.

Catch2 reports repeated-sample means, standard deviations, and bootstrap
confidence intervals (100 samples and 100000 resamples by default). Times are
**per benchmark iteration**: divide block time by its labeled sample/operation
count for amortized time per item, or spectrum hop time by H for time per engine
sample. These averages do not measure maximum per-sample latency or establish
hard real-time bounds. Plan construction includes destruction. Legacy smoothing
includes a fresh RFFT each iteration, avoiding repeated smoothing of old output.

For a quick executable smoke check, lower the sampling cost explicitly:

```shell
scons benchmark BENCHMARK_ARGS="--benchmark-samples 10 --benchmark-resamples 1000 --benchmark-warmup-time 10"
```

For focused runs, listing workloads, or machine-readable Catch2 XML:

```shell
./build_benchmark/dsp/benchmark_fft --list-test-names-only
./build_benchmark/dsp/benchmark_fft "[fft]" --reporter xml --out /tmp/fourier-fft.xml
```

Build first, then time serially on an otherwise idle host. SCons serializes
benchmark suites even with `-j`, but concurrent compilation or unrelated
processes can still distort measurements. Do not run Rack and standalone
benchmarks concurrently. For an optimization, compare baseline and candidate
with the same compiler, flags, host, signal, FFT size, hop length, and channel
count. Record the commit, OS/CPU, compiler version, build command/flags, benchmark
options, repeated results, and uncertainty. A smoke run or a single comparison
is not evidence of a speedup. A microbenchmark does not establish whole-patch
engine or display performance.

### Publication Experiments

The [publication measurement protocol](../../benchmark/paper/README.md) adds
raw callback/step observations, continuous throughput, matched fixed-cadence
RFFT controls, scalar/SIMD and module scaling, spectrum-age audits, background
DSP load, cache pressure, and FFT/RFFT/IFFT phase measurements. It captures
source/SDK hashes, build flags, independent transform checks and repeated
process observations for later backend comparisons. Run its short validation
campaign from the repository root:

```shell
python3 benchmark/paper/run.py build/paper-smoke --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 benchmark/paper/check.py build/paper-smoke
```

These simulate audio callbacks; compute budget exceedances are not device
underruns. The protocol explains timing overhead, output/latency contracts,
full matrix/custom workloads, and the additional sessions required for paper
results. The historical manuscript campaigns are preserved separately.

### Rack DSP Processing

With the normal Rack build dependencies, run from the repository root:

```shell
make build/benchmark/rack/dsp
make benchmark-dsp
make benchmark-dsp BENCHMARK_ARGS="--benchmark-samples 10 --benchmark-resamples 1000 --benchmark-warmup-time 10"
```

These targets honor `RACK_DIR` and use the SDK's optimization, architecture,
and floating-point flags, including flags that differ from standalone SCons.
`benchmark/rack/dsp.cpp` drives the actual modules at 48 kHz. Each iteration
processes 4096 engine samples with precomputed voltages, connected mono or
sixteen-voice inputs, AC coupling, Hann windows, and smoothing off or one-third
octave plus 100 ms time smoothing. Fourier covers all four SIMD ports at
N=128, 2048, and 16384 with H=256. Spectre uses N=2048 and H=1024.

Timing includes port writes, polyphonic summation, double-precision DC filters,
scalar/SIMD analysis, Fourier coordinate mapping, and producer mailbox
publication. Construction and warmup are excluded. There is no concurrent
display consumer, rendering, audio device, or host scheduling in this workload.
The headless output sanity assertions verify that processing publishes spectra;
the existing numerical regression suites remain the correctness oracle.

### Module Operating States And Lifecycle

`benchmark/rack/modules.cpp` complements the fixed DSP workloads by creating
both modules through their exported Rack model factories. From the repository
root, with the usual Rack dependencies:

```shell
make build/benchmark/rack/modules
make benchmark-modules
make benchmark-modules BENCHMARK_ARGS="--benchmark-samples 10 --benchmark-resamples 1000 --benchmark-warmup-time 10"
make benchmark-modules BENCHMARK_ARGS="[lifecycle]"
```

The executable is also included in `make benchmark-rack-build`. It uses the
same SDK flags and Catch2 statistics as the other Rack benchmarks. Workloads
are named by module, settings, and units:

| Workload | Timed Work Per Iteration |
| --- | --- |
| Operating states | 4096 samples with constructor-default controls at 44.1, 96, and 192 kHz; disconnected ports with stale voltages, connected silence, mono, sixteen voices, or frozen mono input |
| Shipped presets | 4096 samples with every `.vcvm` preset in the module's preset directory, sixteen voices per port, at 48 kHz; names, FFT lengths, and hops appear in results |
| Live controls | Alternating window, frequency/time smoothing, gain, and AC-coupling settings followed by 32768 samples at 48 kHz; Fourier also alternates N=128/16384 and H=240/1440 |
| Factory lifecycle | One factory construction and destruction at 44.1 or 192 kHz, including parameter metadata, DSP/storage allocation, and initial reset |
| Reset | One Rack reset event on an existing module at 44.1 or 192 kHz, including parameter defaults, the module callback, and Spectre's cleared-history publications |
| Sample-rate event | One engine-dispatched event alternating 44.1/192 kHz on a registered module, including the engine lock and callback; maximum storage is prepared beforehand |
| Saved state | One `toJson()` plus JSON destruction, or one `fromJson()` alternating preloaded presets, including all parameters and custom state |

Processing includes deterministic port writes, the complete `process()` path,
light updates, and spectrum publication. Every input port is exercised, with
different signal phases per port and voice. The generator and initial warmup
are outside timing. Frozen fixtures toggle the actual run button after warming
an active signal: Fourier continues analysis of retained input, while Spectre
stops analysis. Neither is treated as Rack's separate bypass mode.

Live-control iterations include enough samples for both the control latch and
the resulting analysis work. Construction and saved-state workloads explicitly
include their allocation/deallocation costs; preset discovery, file reads,
JSON parsing, and plugin registration are outside timing. Preset version
metadata is normalized to the current manifest to exclude Rack's historical
version diagnostic from measurements. JSON timing excludes text encoding,
filesystem saves, and complete Rack patch loading.

Lifecycle measurements use warmed process/allocator state. Repeated resets
still execute the real callback and clear/publish Spectre history each time.
The sample-rate workload does not measure first-time capacity growth; the
192 kHz constructor workload includes allocation at that capacity. These
benchmarks use an idle headless engine, with no audio device, rendering,
concurrent display reader, or host scheduling. They do not establish whole-Rack
CPU usage or maximum per-sample latency. Output/state assertions check that
fixtures publish meaningful spectra, freeze correctly, reset display state,
and round-trip complete module JSON; the Rack regression suites remain the
numerical and compatibility oracle.

### Rack Coordinates And Graphics

Build the Catch2 Rack benchmark executables, then run each target serially
from the repository root with the normal Rack dependencies:

```shell
make -j3 benchmark-rack-build
make benchmark-coordinates
make benchmark-graphics
```

Both run targets honor `RACK_DIR` and `BENCHMARK_ARGS` just like
`benchmark-dsp`. For example, smoke-check graphics or select only curve work:

```shell
make benchmark-graphics BENCHMARK_ARGS="--benchmark-samples 10 --benchmark-resamples 1000 --benchmark-warmup-time 10"
make benchmark-graphics BENCHMARK_ARGS="[curves]"
```

`benchmark/rack/coordinates.cpp` isolates `SpectrumCoordinates::map` from
FFT processing. Each iteration maps a complete four-lane frame at N=128,
2048, or 16384. It covers both frequency scales and all three magnitude
scales, with either full-band/zero-slope or 100-10000 Hz/4.5 dB-per-octave
settings at 48 kHz. The deterministic magnitudes differ by lane. Timings
include input traversal and coordinate output stores.

`benchmark/rack/graphics.cpp` uses the real module display methods with a
headless NanoVG backend. Frame boundaries use pixel ratio one; geometry
callbacks count tessellated vertices and texture callbacks copy uploaded
bytes into host memory. Each iteration measures one of these workloads:

| Workload | Included Work |
| --- | --- |
| Fourier curves | Four traces at N=128, 2048, or 16384 in a 660-by-350 display; full/cropped frequency ranges, straight/Bezier paths, stroke/filled modes; point remapping, clipping, Catmull-Rom conversion, and NanoVG tessellation |
| Axis artwork | One rebuild of either module's actual cached artwork callback, including background/grid paths, label formatting, and label layout; both frequency scales and all Fourier magnitude scales |
| Spectre unchanged | Polling retained column mailboxes and drawing the existing image/scan line, with no upload |
| Spectre crop/resize | Alternating bounds and display size, reusing identical cached pixels with no upload |
| Spectre recolor | Alternating slope on every iteration to force a full image rebuild/upload, for all seven color maps and both frequency scales |
| Spectre reopen | Constructing a display, importing all retained mailbox history, creating its first image, and destroying the display and image; includes allocations and deallocations |

Curve fixtures are precomputed normalized plot points with peaks and cropped
frequencies outside the visible rectangle. They intentionally isolate drawing
from spectrum mapping. Axis rebuilds invoke the cache's artwork callback so
label storage is cleared on every iteration, as in production. Path storage
is warmed before curve/axis measurements. Spectre's complete history is filled
through real engine processing before timing; no audio processing occurs in
these graphics timings. Its usual display size is 465 by 350, and its image
is always `N_STFT` by `N_FFT/2`, independent of display size.

Assertions check that curves reach the backend, label counts stay bounded,
every recolor iteration uploads, unchanged/cropped views do not upload, and
reopening preserves the rendered pixels and releases each image. These are
CPU measurements: they exclude GL framebuffer allocation/compositing, actual
font glyph rendering, GPU rasterization, driver transfer latency, display
synchronization, and concurrent producer/consumer contention. These graphics
workloads do not time menu actions or serialization. Use the existing
Rack regression tests and interactive checks for correctness, and record the
same comparison metadata and uncertainty as for DSP benchmarks.

### Display Preparation

```shell
make benchmark-display
```

This uses the actual Spectre module/display and a headless NanoVG texture
backend that copies uploaded bytes. It reports seven repetitions of frozen,
running, forced-rebuild, and engine-only workloads. Graphics workloads use
120 frames; running supplies 800 engine samples per frame (48 kHz / 60 Hz).
Engine-only supplies 32768 samples per iteration without drawing. It does not
measure driver upload latency, GPU rendering, framebuffer speed, or whole-patch
performance. The [raw display caching measurements](../../specs/archive/001-display-caching.csv)
retain the historical before/after timings; the removed specification in Git
history records the workload context and validation limits.

### One-Hop Spectrum Analysis

The production analyzer is tested independently of Rack and in both modules:

```shell
scons test/dsp/test_spectrum_analysis.cpp
make test-spectrum-points test-display-lifecycle test-serialization
```

Standalone coverage includes reference spectra, exact cadence, output quotas,
window/smoothing cache changes, reset, retained-input wraparound, and
allocation-free live settings. Rack coverage adds scalar/SIMD comparison,
curve ownership and publication, coordinate mapping, module allocation checks,
and Spectre's exact cadence and freeze/resume behavior.

The [historical pipeline campaign](../whitepaper/data/pipeline/README.md)
retains pre-integration timing evidence and reproduction sources in an archive.
It is not a benchmark of the current plugin. Verify that artifact separately:

```shell
python3 docs/whitepaper/data/pipeline/check.py
```

## User Manuals And Build Products

The existing multi-file LaTeX manuals use their own Makefiles and require
`pdflatex` and their referenced packages:

```shell
make -C docs/manual
```

Outputs are `docs/manual/build/Fourier.pdf` and `docs/manual/build/Spectre.pdf`.
The child Makefiles recreate their local build directories and stop on
LaTeX errors. Shell escape is disabled. Inspect rendered pages
when changing manual content or layout.

The [user manuals workflow](../../.github/workflows/manuals.yml) builds both
PDFs on relevant pull requests and pushes to `main`, and saves them as the
`user-manuals` workflow artifact. Ubuntu uses `texlive-latex-extra`,
`texlive-fonts-recommended`, `texlive-science`, and `poppler-utils`; CI checks
that both PDFs are nonempty and readable by `pdfinfo`.

Publishing a GitHub release triggers a build from its tag and uploads
`Fourier.pdf` and `Spectre.pdf` as release assets. These names match the
`manualUrl` links in `plugin.json`. A tag push alone does not publish a
release or upload assets. Publish a regular release as the latest release
for Rack's `/releases/latest/download/` links to resolve to these manuals;
prerelease assets are also uploaded but do not become the latest release.

To backfill an existing release, select **Actions > User manuals > Run
workflow** and enter its exact tag in `tag`. The workflow must first be on
the default branch. Sources and Makefiles come from that tag, so older tags
retain their original build behavior. The release must already exist and
allow asset changes. Reruns replace assets with the same names without
changing release notes or other assets. Only the upload job has
`contents: write`; builds use read-only repository permissions and require
no additional secrets. GitHub's built-in token does not trigger a release
workflow when another workflow creates the release with that token; use
the manual trigger in that case.

Keep generated binaries, object files, SCons caches, PDFs, and build folders
out of source changes. Use explicit SCons targets above: bare `scons` also
involves the standalone shared-library target and is not the test command.

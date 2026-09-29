# Contributing To Fourier

Help improve Fourier and Spectre through bug reports, documentation, tests,
and code. This guide gets you from a fresh checkout to a tested contribution.
It also covers the architecture, build targets, test coverage, benchmarks,
and platform limitations used when reviewing changes.

-   [Set up your environment](#set-up-your-environment)
-   [Understand the architecture](#architecture)
-   [Build and test](#development-and-testing)
-   [Choose validation for your change](#choosing-validation)
-   [Update manual figures](#manual-figures)
-   [Submit a pull request](#submit-a-pull-request)

## Before You Start

Search the [existing issues][issues] before reporting a bug or proposing a
feature. For a bug, include the Rack and plugin versions, operating system,
sample rate, steps to reproduce, and expected versus observed behavior.
A minimal patch or screenshot helps; mention any additional plugins or
sample files it needs. Discuss substantial behavior or interface changes
in an issue before investing in the implementation.

Read the [architecture overview][architecture] and the relevant
[C++][cpp-style] or [Markdown][markdown-style] style guide before editing.
If you use a coding agent, also have it follow [AGENTS.md](AGENTS.md).

## Set Up Your Environment

Fourier has two separate build paths:

| Work | Required Tools |
| --- | --- |
| Standalone DSP tests | Git, GNU Make, and a C++14 compiler |
| Rack plugin and integration tests | The above, plus `jq`, and a compatible Rack 2 SDK or prepared Rack source tree |
| Interactive module checks | A Rack 2 installation matching your plugin's platform and architecture |
| User manual PDFs | Make and a TeX distribution with `pdflatex` and the manual's packages |

You can work on reusable DSP and run its tests without installing Rack.
Markdown-only contributions do not require a C++ or TeX toolchain.

### Install The Test Tools

Run the commands for your platform from any directory. They set up the
standalone test tools; the Rack build has additional requirements below.

On macOS, install Apple's Command Line Tools if needed, then use an existing
Homebrew installation:

```shell
xcode-select --install
brew install git python
```

On Ubuntu or Debian, use the system packages:

```shell
sudo apt-get update
sudo apt-get install git build-essential python3 make
```

On Windows, install MSYS2 and use its **UCRT64** shell for standalone tests.
After completing MSYS2's initial package updates, install:

```shell
pacman -S --needed git mingw-w64-ucrt-x86_64-gcc python make
```

Use MSYS2's Python and Make in that shell, as in the standalone Windows CI
environment. For Rack plugin builds, follow VCV's platform toolchain
instructions below.

### Get The Source

From your projects directory, clone the repository. To submit a pull
request, fork it on GitHub first and substitute your fork's clone URL:

```shell
git clone https://github.com/Kautenja/ArhythmeticUnits-Fourier.git Fourier
cd Fourier
test -f dep/Catch2/catch_amalgamated.hpp
test -f dep/Catch2/catch_amalgamated.cpp
```

Catch2 3.16.0's amalgamated header and source are vendored in this repository;
no submodule initialization or system Catch2 installation is needed. See
[the dependency notes](dep/Catch2/README.md) for their source, license, and
update procedure. Tests and benchmarks use C++14; reusable DSP and the
shipped Rack plugin retain their C++11 baseline.

Run all remaining commands from this repository root unless noted otherwise.
Create a branch for your contribution; replace the example name with one
that describes your change:

```shell
git switch -c docs/contributor-setup
```

### Configure The Rack SDK

Follow [VCV's build environment instructions][rack-building] for your
operating system and download a Rack 2 SDK for your platform and CPU
architecture. An SDK avoids building the host from source. Install the
listed platform tools, including Make and `jq`; packaging also needs
`zstd`. Use checkout and SDK paths without spaces.

Replace the placeholder below with the absolute path to the extracted SDK.
It must contain `plugin.mk`, `include/`, `dep/include/`, and the Rack library:

```shell
export RACK_DIR=/absolute/path/to/Rack-SDK
test -f "$RACK_DIR/plugin.mk"
```

Keep `RACK_DIR` set for subsequent Make commands in the same shell. If the
checkout lives at `Rack/plugins/Fourier` inside a prepared Rack source tree,
you can omit the export: the default `RACK_DIR=../..` selects that tree.
An installed Rack application alone is not an SDK.

Continue with [standalone tests](#standalone-dsp-tests) and the
[Rack plugin build](#rack-plugin-build) below.

## Architecture

This section maps Fourier's source and the boundaries to preserve when
changing DSP, Rack modules, or displays.

### Source Map

-   `src/plugin.cpp` registers the two models; `src/plugin.hpp` connects Rack,
    shared helpers, and the plugin instance.
-   `src/SpectrumAnalyzer.cpp` contains the Fourier module, display, and
    widget. It analyzes four input ports using `simd::float_4` lanes.
-   `src/Spectrogram.cpp` contains the Spectre module, spectral image display,
    and widget. It uses scalar processing and a history of spectra.
-   `src/structs.hpp` contains shared display/analysis enums and conversions.
-   `src/dsp/spectrum_analysis.hpp` owns the bounded one-hop analysis schedule
    shared by the scalar and SIMD modules.
-   `src/dsp/` contains mostly header-only math, filters, triggers, and music
    theory in a flat set of focused headers. Consumers include the headers
    they use directly.
-   `src/rack_extensions/` contains Rack graphics and control helpers.
-   `test/dsp/` mirrors the flat DSP header layout. `test/functions.hpp` and
    `test/ieee754.hpp` provide test helpers.
-   `benchmark/dsp/` holds standalone DSP benchmarks; `benchmark/rack/`
    measures headless module processing and display preparation. See the
    testing section for workloads and interpretation.
-   `src/rack_extensions/panel.hpp` draws both panel formats and shares their
    geometry with the module controls. `panel_artwork.hpp` preserves the original
    branding and lettering as native vector paths. `res/` contains the font used
    by dynamic display text; panel rendering needs no SVG or Sketch files.
-   `docs/manual-fourier/` and `docs/manual-spectre/` contain LaTeX user manuals
    and illustrations. Contributor guidance lives in this file; style guides
    live in `docs/style-guides/`.
-   `patches/` and `presets/` provide Rack examples and saved module settings.

### Analysis Flow

Rack calls each module's `process(const ProcessArgs&)` for engine samples.
The modules normalize Eurorack voltages, maintain double-precision DC-blocker
state per input, and apply gain. The higher-precision filter state prevents
roundoff from accumulating as a DC offset in short repeating signals; FFT and
display storage remain float (four SIMD lanes in Fourier).
`SpectrumAnalysis<T>` retains input and distributes windowing/packing,
butterflies, real-spectrum reconstruction, magnitude prefix sums,
frequency/time smoothing, and the output callback over one exact hop.

For M=N/2, K=M+1, B=(M/2)log2(M), a frame contains W=pM+B+(1+o)K scheduling
units, where p=4 while rebuilding the window cache and p=1 otherwise. A pair
executes at its first unit; the remaining credit is skipped. This spreads
expensive coefficient preparation over more of the hop. Output weight o is
the analyzer's compile-time `OutputWeight` (default one). Fourier uses two
because its output callback maps four display coordinates on the engine
thread; Spectre uses one. Output bins also execute at their first credit.
Sample s executes `floor((s+1)W/H)-floor(sW/H)` units. A quotient/remainder
accumulator implements that schedule without per-sample quota division.
Dense schedules dispatch contiguous units in stage segments within the same
sample quota; sparse schedules retain the per-unit loop. SIMD butterfly
segments share stride/group setup while honoring that quota. Dirty and clean
frames have different intermediate bin-emission positions; complete-frame
publication and numerical processing remain the same. `work_per_frame()`
describes the active frame, or the next frame at a frame boundary, including
immediately after configure, reset, and publication.
Frames end at input indices jH and publish at jH+H-1, starting with zero
padding. This intentionally replaces the earlier restart-on-FFT-completion
cadence. Settings latch at each frame start; mid-frame changes apply next hop.
The original `OnTheFlyFFT/RFFT` APIs remain available for other DSP users.
The [technical report](docs/whitepaper/fourier.tex) derives the work bound,
input lifetime, smoothing, and timestamp conventions in its production
successor section; its appendix collects the supporting algorithms. The user
manuals focus on controls, displays, operating behavior, and practical setting
choices, and link to the report for mathematical details.

Maximum-size twiddle/permutation tables serve every supported FFT size.
Window and smoothing-bound changes rebuild their cached entries inside
scheduled units, without resizing processing storage. A length change clears
input/averaging logically. Reset and sample-rate callbacks cancel partial
frames and clear analysis history. Fourier's retained ring is prepared for
the maximum panel hop at the current sample rate.

Fourier maps each four-lane output bin through `SpectrumCoordinates` into a
producer-owned curve snapshot. A constant-size atomic exchange publishes the
complete snapshot; the display keeps the consumer slot until its next read.
Spectre writes each output bin directly into the next column mailbox and
publishes when the hop completes. Its coefficient history lives only in the
UI, which recolors changed columns. Slope, color map, frequency scale, or
sample rate rebuild all pixels; cropping/resizing reuse the image. Unchanged
draws do not upload an image. Both modules have no audio outputs.

Read the actual processing functions before changing run/freeze semantics:
the two modules do not currently gate their processing identically. Also
check `onReset`, `onSampleRateChange`, `dataToJson`, and `dataFromJson` for
state transitions affected by a change.

### Ownership And Threading

DSP headers must remain usable by the standalone test build without Rack.
Generic templates may be instantiated with Rack SIMD values by module code;
the generic header should not need to include Rack to support that use.

Both displays cache backgrounds and grids with Rack framebuffers, invalidated
by size, bounds, scale, sample rate, zoom, and graphics-context changes. Label
strings and positions are cached with the artwork; glyphs render in the live
context to avoid missing glyphs observed during framebuffer rebuilds.
Hover overlays remain live. Curve and column buffers cross the engine/UI
boundary only through single-producer/single-consumer ownership exchanges.
Displays must not consume each other's mailbox slots concurrently.

Keep analysis on the engine side and NanoVG calls on the display side.
Document who owns mutable buffers, who reads them, and when a reader can
observe an update. Avoid new unsynchronized engine/UI sharing or locks that
could block the engine. Do not assume existing shared vectors and readiness
flags make concurrent access safe.

Steady-state analysis and live FFT/window/smoothing changes do not allocate.
Construction prepares maximum-size storage. Increasing the host sample rate
can grow Fourier's retained-input ring in `onSampleRateChange`; that callback
may allocate. Spectre's reset still clears and publishes all history columns.
These lifecycle costs, UI work, OS scheduling, atomics, and unequal unit costs
prevent a broad claim of constant-time or hard real-time behavior. Existing
panel/menu state sharing is separate from the synchronized spectrum buffers.

### Compatibility

The plugin slug is `ArhythmeticUnits-Fourier`; the model slugs are
`SpectrumAnalyzer` and `Spectrogram`. These identities appear in saved
patches and must remain stable even if display names change.

Rack serializes parameter and port identities by their enum positions.
Append new IDs where possible, preserve existing positions, and account
for `ENUMS` ranges. Custom JSON keys and enum values also form a persistence
contract. Handle missing fields with sensible defaults and validate values
before they become sizes, indices, or DSP settings.

Treat channel routing, summed polyphonic inputs, voltage normalization,
FFT bin layout, DC/Nyquist treatment, window coherent gain, and decibel
scaling as behavior contracts. A refactor must preserve them unless changing
that contract is the explicit task.

### Build Boundaries

The root `Makefile` compiles `src/*.cpp` into the Rack plugin using the
selected Rack tree's `plugin.mk`. Nested `.cpp` files are not automatically
included by that wildcard.

`mk/standalone.mk` discovers standalone `.cpp` suites recursively under
`test` (excluding `test/rack`) and `benchmark/dsp`. Each suite includes DSP
headers directly; there is no standalone DSP library. `mk/rack.mk` supplies
headless Rack tests and benchmarks. Standalone goals skip the SDK entirely,
including when `RACK_DIR` points to a missing directory. Mixed invocations
such as `make test all` load the SDK but keep standalone flags independent.

All C++ objects, test/benchmark binaries, and instrumentation reports live
under `.build/`. The plugin binary and `dist/` retain Rack's expected paths.
The SDK's link, packaging, and installation recipes remain in control; the
root Makefile redirects its object and dependency paths into `.build/`.
`make clean` removes `.build/`, the plugin binaries, and `dist/` without an
SDK. Old `build/`, `build_test/`, and `build_benchmark/` directories are not
reused or automatically deleted; preserve any experiment results before
removing them locally. `.build-legacy/` is ignored and can hold old outputs
that should survive `make clean`.

## Development And Testing

Run the commands below from the repository root. Bare `make` builds the
Rack plugin; standalone tests and benchmarks require explicit targets.
`make check-build` uses Python 3 to verify build isolation, incremental
rebuilds, failure propagation, and benchmark serialization in a temporary
fixture without an SDK.

### Standalone DSP Tests

Use GNU Make (including macOS's bundled Make 3.81), a C++14-capable compiler,
and the supplied Catch2 sources. Standalone builds default to `g++`, which
may resolve to Apple Clang on macOS; instrumentation defaults to `clang++`.
Both environment and command-line `CXX` overrides are honored, for example
`make CXX=clang++ test`. Python 3 is needed for instrumentation, build-system
checks, and paper experiments, but not ordinary C++ suites.

Build and run all standalone suites:

```shell
make test
```

Run one suite through its alias (without the source `.cpp` suffix):

```shell
make test/dsp/test_fft
```

Every test `.cpp` is a separate executable linked with Catch2's supplied
`main`. Its amalgamated implementation is compiled once per build
configuration, with separate objects for tests, benchmarks, and each
instrumentation mode. The phony aliases always execute the suites.
`make test-build` only builds.
Use `TEST_ARGS="--list-tests"` to pass Catch2 options through Make, or build
the executable target and invoke it directly:

```shell
make .build/test/standalone/dsp/test_fft
./.build/test/standalone/dsp/test_fft --list-tests --verbosity quiet
```

On Windows, executable paths end in `.exe`. Make tracks header dependencies
and compiler/flag changes; unchanged builds reuse their objects.

Use names returned by that executable when selecting individual cases.
Other suites cover DFT, windows, circular buffers, math helpers, IEEE-754
behavior, and triggers. Add focused tests directly under `test/dsp/`;
Make discovers new `.cpp` files without a hand-maintained suite list.

Prefer deterministic `SCENARIO`/`GIVEN`/`WHEN`/`THEN` or `TEST_CASE` checks.
For transform changes, test known signals and independent expectations:
silence, impulse, DC, bin-centered sinusoids, forward/inverse behavior,
and incremental versus complete computation as applicable. Include supported
length boundaries, hop scheduling, and window normalization. Select error
tolerances from the numerical contract rather than widening them until a
test passes. Seed randomized fixtures when used.

Standalone tests compile as C++14 without `-O3`; benchmarks use C++14 and
`-O3`. Scalar DSP test success does not prove Rack SIMD instantiations compile
or behave correctly. Headless Rack tests also use C++14, while the plugin
build keeps the SDK's C++11 default.

The dedicated DC-blocker suite checks float and double impulse/step responses,
DC rejection, Nyquist gain, transition-width configuration, and reset. Its
separate SIMD suite requires the Rack headers and uses the plugin compiler
flags, without linking the Rack library or creating an engine:

```shell
make test/dsp/test_dc_blocker
make test-dc-blocker-simd
```

The SIMD suite compares independent scalar filters with all four Rack SIMD
lanes under different signals and settings, including reconfiguration and
reset. It is separate from `make test` and runs in CI's Rack job.

### Continuous Integration

The [DSP and Rack tests workflow](.github/workflows/dsp-tests.yml) runs on
pull requests, pushes to `main` (including merges), and version tags matching
`v*`. Other branch pushes do not trigger builds. Code checks run on tags;
release publication only rebuilds and uploads the PDFs, avoiding a second
six-job code matrix for the same release.

The three standalone jobs run `make -j2 test` with Ubuntu 24.04 GCC,
macOS 14 Apple Clang, and Windows 2022 MSYS2 UCRT64 GCC. Windows uses
MSYS2's Make to preserve POSIX paths and GNU build tools. Each job also
builds standalone benchmarks and lists their workloads without timing them.
These commands use a missing `RACK_DIR` to verify SDK independence. Linux
and macOS additionally run the small `make check-build` regression fixtures.

Three independent Rack jobs download the official Rack 2.6.3 SDKs, verify
pinned SHA-256 checksums, and run `make -j2 all` and `make -j2 test-rack`:

| Runner | Plugin Architecture | Toolchain |
| --- | --- | --- |
| Ubuntu 24.04 | Linux x64 | GCC |
| macOS 14 | macOS ARM64 | Apple Clang |
| Windows 2022 | Windows x64 | MSYS2 MINGW64 GCC (MSVCRT) |

Each Rack job runs five headless module/DSP suites: SIMD DC blocker,
serialization, display lifecycle, spectrum coordinates, and module amplitudes,
plus the C++ benchmark-runner checks. Linux and
macOS link and run against the SDK library. Linux installs the OpenGL, X11,
and audio runtime dependencies. The Windows SDK has an import library only,
so CI also verifies and extracts the matching Rack Free installer and adds
its DLL directory to the test process's `PATH`. It does not run the installer
or compile Rack. Windows tests use Catch2's ordinary `main()` entry point;
the plugin retains the SDK's flags.

The SDK lives under the runner's temporary directory, selected by `RACK_DIR`.
New updates cancel older runs for the same pull request or branch. DSP jobs
have a 15-minute timeout and Rack jobs have a 25-minute timeout. A failed
platform does not cancel the other matrix jobs. Plugin binaries are compiled
for validation, without packaging or publishing GitHub release binaries;
VCV Library distribution remains a separate release step.

Headless tests exercise actual module processing, state, and CPU-side display
behavior without a window or audio device. They do not replace an interactive
Rack session or GPU checks. Intel macOS builds, benchmarks, and manual UI
checks remain separate validation steps.

### Coverage And Sanitizers

The [instrumentation workflow](.github/workflows/instrumentation.yml)
runs on pull requests, pushes to `main`, and manual dispatch. Four independent
Ubuntu 24.04 jobs run DSP/Rack coverage and combined ASan/UBSan with Clang 18.
Rack jobs use the same pinned Rack 2.6.3 SDK as the ordinary Rack CI job.
A separate macOS 14 job runs only the standalone mailbox suite under TSan;
it needs no Rack SDK or shared library. Failures do not cancel other matrix
jobs. Reports and full compiler/test diagnostics are uploaded for 14 days,
including diagnostics from failed runs. Coverage summaries also appear in the
workflow summary. No external reporting account or token is required.

From the repository root, use Python 3, Make, and Clang on Linux or macOS:

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

#### Coverage Reports

The [LLVM source coverage](https://clang.llvm.org/docs/SourceBasedCodeCoverage.html)
reports are separate by test workload:

-   `.build/reports/coverage/dsp/`: standalone DSP suites, reporting `src/dsp/`.
-   `.build/reports/coverage/rack/`: headless Rack and benchmark-runner suites, reporting
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

#### Sanitizer Scope

Combined [ASan](https://clang.llvm.org/docs/AddressSanitizer.html) and
[UBSan](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) instrument
both standalone DSP tests and the actual module code compiled into the
headless Rack suites. Diagnostics fail immediately, with UBSan recovery
disabled. The runner uses `halt_on_error=1` for both runtimes and requests
UBSan stack traces. Leak checking uses the platform runtime's default
(enabled on Linux); no project suppression list is applied. Logs live at
`.build/reports/asan-ubsan/dsp/run.log` and
`.build/reports/asan-ubsan/rack/run.log`.

[TSan](https://clang.llvm.org/docs/ThreadSanitizer.html) runs separately from
ASan/UBSan. `test/threads/test_display_mailbox.cpp` checks retained-slot
ownership, superseded publications, coherent concurrent payloads, monotonic
sequence values, and final publication delivery. Its bounded producer sends
100000 snapshots; the reader inspects held storage during publication.
The runner sets `halt_on_error=1:exitcode=66` and saves
`.build/reports/tsan/mailbox/run.log`. This focused run checks the
single-producer/single-consumer mailbox contract. It does not establish race
freedom for module controls, Rack internals, or an entire engine/UI session.
The mailbox suite also runs without instrumentation in `make test` or alone
with `make test-mailbox`.

Instrumented objects and executables live under `.build/instrumented/`,
separate from ordinary tests and plugin products. DSP coverage uses `-O0`;
Rack coverage retains SDK optimization and floating-point flags. Sanitizers
use `-O1`, debug symbols, and frame pointers, retaining Rack's configured
floating-point behavior. Rack instrumentation covers first-party code and
included headers, while the supplied `libRack` remains uninstrumented.
Passing these checks is not an instrumented Rack host run or a plugin build.
Use ordinary builds and manual Rack checks separately. Do not use these
slower builds for performance measurements or distribute them as plugins.

### Rack Plugin Build

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

#### Install For Interactive Checks

With the Rack build and packaging dependencies installed, close Rack and run
from the repository root, using the same `RACK_DIR` as for the build:

```shell
make install
```

This builds a package in `dist/` and copies it into the selected Rack user
folder, replacing that installation of Fourier when Rack next loads it.
Reopen Rack and test the affected modules. See the
[VCV plugin tutorial][rack-tutorial] for installation and loading diagnostics.

### Choosing Validation

-   **DSP behavior:** Run the focused suite while iterating, then `make test`.
    Build the plugin when changed headers are consumed by Rack, especially
    for template or SIMD changes.
-   **Module state or controls:** Build the plugin, test extracted reusable
    logic where feasible, and verify affected behavior in Rack.
-   **Graphics or interaction:** Build the plugin and inspect the affected
    module in Rack, including its module-browser preview where applicable.
-   **Documentation:** Check relative links, referenced paths and commands,
    Markdown structure, and `git diff --check`. No full build is required
    solely for prose changes.

To run the headless Rack and benchmark-runner suites with the configured SDK and its runtime
dependencies, use:

```shell
make test-rack
```

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
and excluded from the standalone suites. The suite also checks
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
wraparound. Both displays also have hover-boundary checks for each plot edge,
the control strip, resized widgets, and null-module previews.
Standalone mailbox concurrency checks run through Make and the
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
and GL errors and saves `.build/test/rack/display-*.ppm` for visual inspection.
It is not a complete interactive Rack or DAW session.

To inspect the complete panels and controls with the same desktop prerequisites:

```shell
make inspect-panels
```

This renders both real module widgets with test signals and both null-module
browser previews. Each set covers light and dark themes, 75-percent zoom,
one-pixel density, native pixel density, and graphics-context recreation.
It verifies module dimensions, settled panel framebuffers, and GL errors, and
saves `.build/test/rack/panel-{live,preview}-N.ppm` for scenarios 0--17. Scenarios 0
and 4 show the light panel before and after context recreation; 1 is dark, 2 is
zoomed out, and 3 is dark at one-pixel density. Scenarios 5 and 6 check hover enter
and leave on the Fourier frequency control through Rack's event dispatcher;
only live controls highlight, changes stay within the control and its one-pixel
antialiasing fringe, and leaving restores identical pixels. Scenario 7 checks
the dark theme at 50-percent zoom. The inspector also checks that the
frequency labels stay consistent and rendering does not change their values.
Scenarios 8--12 move the cursor inside Fourier's plot and into each surrounding
gutter; 13--17 do the same for Spectre. They check event targets and verify that
leaving the plot restores the image without the cursor overlay.
On a standard-density desktop, native and one-pixel density are the same.
The inspector does not open an audio device
or exercise mouse dragging or patch loading.

For a visual comparison against an older panel export, the executable accepts
an optional final directory containing that version's four panel SVGs. For
example, with those files already saved in `.build/panel-reference/`:

```shell
DYLD_LIBRARY_PATH="$PWD/../.." LD_LIBRARY_PATH="$PWD/../.." \
    .build/test/rack/inspect_panels "$PWD/../.." "$PWD" \
    "$PWD/.build/test/rack/reference" "$PWD/.build/panel-reference"
```

This example uses the default Rack source-tree layout; substitute the SDK path
for each `../..` reference when applicable. Reference exports are optional local
comparison inputs, not shipped resources or build dependencies. Keep generated
screenshots in the ignored build directory. Panel dimensions and Rack IDs must
stay compatible with existing patches when changing the shared geometry.

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

### Benchmarks

For the FFT/analyzer optimization loop, start with the native C++ development
runner. From the repository root with the Rack SDK configured:

```shell
make -j2 benchmark-dev-build
make benchmark-fast
make benchmark-fast BENCHMARK_DEV_ARGS="--backend core-float --pass throughput"
make benchmark-full
```

The fast profile runs 24 fixed workloads with three repetitions in one process.
The full development profile adds sizes, hops, live settings, startup, channel
banks, transforms, and compiled external providers. Both reuse the paper's C++
adapters, timing loops, and numerical/cadence checks. Builds are incremental;
neither mode forces compilation or builds the allocation-audit executable.
Registry generation, execution, statistics, and baseline comparison are C++.
Use `make test-benchmark-dev` to check the runner itself; `test-rack` includes
these checks. See the [development workflow](benchmark/paper/DEVELOPMENT.md)
for baseline comparison, raw artifacts, filters, and publication boundaries.

These short runs are development feedback. The publication archiver below
retains its separate fresh-process and independent-session requirements.

The standalone Catch2 v3 benchmarks cover every computational DSP header.
They build with C++14 and `-O3`, independently of Rack. Run from the repository
root with the same dependencies as the standalone tests:

```shell
make -j4 benchmark-build
make benchmark
make benchmark/dsp/benchmark_fft
```

`benchmark-build` only compiles; `benchmark` and the per-suite aliases always
run. Standalone targets exclude `benchmark/rack/`, which requires the SDK.
A command-line compiler override, such as
`make CXX=clang++ benchmark-build`, applies to the benchmarks too. Use the same override for the subsequent run.

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
make benchmark BENCHMARK_ARGS="--benchmark-samples 10 --benchmark-resamples 1000 --benchmark-warmup-time 10"
```

For focused runs, listing workloads, or machine-readable Catch2 XML:

```shell
./.build/benchmark/standalone/dsp/benchmark_fft --list-tests --verbosity quiet
./.build/benchmark/standalone/dsp/benchmark_fft "[fft]" --reporter xml --out /tmp/fourier-fft.xml
```

Build first, then time serially on an otherwise idle host. Make serializes
benchmark suites even with `-j`, but concurrent compilation or unrelated
processes can still distort measurements. Do not run Rack and standalone
benchmarks concurrently. For an optimization, compare baseline and candidate
with the same compiler, flags, host, signal, FFT size, hop length, and channel
count. Record the commit, OS/CPU, compiler version, build command/flags, benchmark
options, repeated results, and uncertainty. A smoke run or a single comparison
is not evidence of a speedup. A microbenchmark does not establish whole-patch
engine or display performance.

#### Publication Experiments

The [publication measurement protocol](benchmark/paper/README.md) adds
raw callback/step observations, continuous throughput, matched fixed-cadence
RFFT controls, scalar/SIMD and module scaling, spectrum-age audits, background
DSP load, cache pressure, and FFT/RFFT/IFFT phase measurements. It captures
source/SDK hashes, build flags, independent transform checks and repeated
process observations for later backend comparisons. Run its short validation
campaign from the repository root:

```shell
python3 benchmark/paper/run.py .build/paper-smoke --repeats 1 --hops 4 --frames 2 --step-frames 1 --warm-hops 2
python3 benchmark/paper/check.py .build/paper-smoke
```

These simulate audio callbacks; compute budget exceedances are not device
underruns. The protocol explains timing overhead, output/latency contracts,
full matrix/custom workloads, and the additional sessions required for paper
results. The historical manuscript campaigns are preserved separately.

#### Rack DSP Processing

With the normal Rack build dependencies, run from the repository root:

```shell
make .build/benchmark/rack/dsp
make benchmark-dsp
make benchmark-dsp BENCHMARK_ARGS="--benchmark-samples 10 --benchmark-resamples 1000 --benchmark-warmup-time 10"
```

These targets honor `RACK_DIR` and use the SDK's optimization, architecture,
and floating-point flags, including flags that differ from standalone builds.
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

#### Module Operating States And Lifecycle

`benchmark/rack/modules.cpp` complements the fixed DSP workloads by creating
both modules through their exported Rack model factories. From the repository
root, with the usual Rack dependencies:

```shell
make .build/benchmark/rack/modules
make benchmark-modules
make benchmark-modules BENCHMARK_ARGS="--benchmark-samples 10 --benchmark-resamples 1000 --benchmark-warmup-time 10"
make benchmark-modules BENCHMARK_ARGS="[lifecycle]"
```

The executable is also included in `make benchmark-rack-build`. It uses the
same SDK optimization flags and Catch2 statistics as the other Rack
benchmarks. Workloads are named by module, settings, and units:

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

#### Rack Coordinates And Graphics

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

#### Display Preparation

```shell
make benchmark-display
```

This uses the actual Spectre module/display and a headless NanoVG texture
backend that copies uploaded bytes. It reports seven repetitions of frozen,
running, forced-rebuild, and engine-only workloads. Graphics workloads use
120 frames; running supplies 800 engine samples per frame (48 kHz / 60 Hz).
Engine-only supplies 32768 samples per iteration without drawing. It does not
measure driver upload latency, GPU rendering, framebuffer speed, or whole-patch
performance. Historical display caching measurements and their removed
specification remain in Git history, including the workload context and
validation limits.

#### One-Hop Spectrum Analysis

The production analyzer is tested independently of Rack and in both modules:

```shell
make test/dsp/test_spectrum_analysis
make test-spectrum-points test-display-lifecycle test-serialization
```

Standalone coverage includes reference spectra, exact cadence, output quotas,
window/smoothing cache changes, reset, retained-input wraparound, and
allocation-free live settings. Rack coverage adds scalar/SIMD comparison,
curve ownership and publication, coordinate mapping, module allocation checks,
and Spectre's exact cadence and freeze/resume behavior.

The [historical pipeline campaign](docs/whitepaper/data/pipeline/README.md)
retains pre-integration timing evidence and reproduction sources in an archive.
It is not a benchmark of the current plugin. Verify that artifact separately:

```shell
python3 docs/whitepaper/data/pipeline/check.py
```

### User Manuals And Build Products

The manuals share [publication styling](docs/latex/arhythmetic-manual.sty)
and [build rules](docs/latex/manual.mk). From the repository root, with
`latexmk`, `pdflatex`, and the referenced TeX packages installed, run:

```shell
make -C docs/manual-fourier
make -C docs/manual-spectre
```

Outputs remain `docs/manual-fourier/.build/manual.pdf` and
`docs/manual-spectre/.build/manual.pdf`. Like the white paper, each build
runs `latexmk` until references settle, then cleans auxiliary files. A
successful fresh build leaves only `manual.pdf`; sources and artwork are
read in place rather than copied into `.build`. A failed build retains its
log and intermediate files for diagnosis. Shell escape is disabled.
`BUILD` and `LATEXMK` can be overridden; `make -C docs/manual-fourier clean`
also removes the PDF. Keep page-review images outside these output folders.

The source layout is:

```text
docs/latex/
  arhythmetic-manual.sty  shared typography, covers, contents, and PDF metadata
  manual.mk              shared build and screenshot commands
  figures/               shared TikZ drawings and panel export target
docs/manual-{fourier,spectre}/
  manual.tex             version, module identity, and section order
  sections/              one lower-case TeX file per content section
  figures/               module-specific lower-case TikZ sources
  img/                   reviewed screenshot and original branding assets
  .build/manual.pdf      generated publication
```

The table of contents is generated from section headings, with thematic
separators defined in `manual.tex`. Keep `sec:` and `fig:` labels stable so
links survive reordering. Numbered subsections belong to the panel controls;
other subheadings should be unnumbered. The PDF outline includes sections
and controls. Page labels distinguish the cover, Roman-numbered contents,
Arabic-numbered body, and colophon.

The shared style writes title, author, subject, keywords, language, publisher,
and rights metadata to XMP, and preserves standard PDF Info fields for older
viewers. Update the module/version options in `manual.tex`, rather than
hard-coding metadata elsewhere. Font encoding and Unicode maps support text
search and copying; these are not claims of PDF/A or PDF/UA certification.
After style changes, inspect both manuals, metadata, bookmarks, links, and
page labels as well as the rendered pages.

<a id="spectre-manual-figures"></a>

#### Manual Figures

Each manual uses one module screenshot on its cover: the checked-in
`img/PanelLayout.png`. Generate it from the real Rack widgets, rather than
maintaining a separate screenshot imitation. Panel references use detailed
TikZ line drawings of the physical layout, with numbered callouts that match
the manual's control sections. Draw recognizable jacks, knobs, buttons, and
display controls at their real positions; keep screens schematic and omit
current values or captured spectra. Other explanatory figures remain
conceptual. Branding and Fourier's historical portrait remain separate.

Choose the source to update:

| Figure | Fourier Source | Spectre Source | When To Update |
| --- | --- | --- | --- |
| Cover module screenshot | [`PanelLayout.png`](docs/manual-fourier/img/PanelLayout.png) | [`PanelLayout.png`](docs/manual-spectre/img/PanelLayout.png) | Visible panel artwork or controls change |
| Annotated panel reference | [`panel-layout.tex`](docs/manual-fourier/figures/panel-layout.tex) | [`panel-layout.tex`](docs/manual-spectre/figures/panel-layout.tex) | Control positions, labels, or section numbering change |
| Window responses | Shared [`window-tradeoffs.tex`](docs/latex/figures/window-tradeoffs.tex) | Same shared source | The explanation of window behavior changes |
| Time/frequency smoothing | Shared [`smoothing.tex`](docs/latex/figures/smoothing.tex) | Same shared source | The explanation of smoothing changes |
| Color-range detail | Not used | [`color-range.tex`](docs/manual-spectre/figures/color-range.tex) | Color-control gestures or geometry change |
| Frame duration and bin spacing | [`frame-length.tex`](docs/manual-fourier/figures/frame-length.tex) | Not used | The explanation of FFT length changes |
| History scanning | Not used | [`history.tex`](docs/manual-spectre/figures/history.tex) | The explanation of history acquisition changes |
| Ideal harmonic series | [`harmonics.tex`](docs/manual-fourier/figures/harmonics.tex) | Not used | The harmonic comparison exercise changes |
| Before/after routing | [`filter-patch.tex`](docs/manual-fourier/figures/filter-patch.tex) | Not used | The filter comparison exercise changes |
| Time-frequency signatures | Not used | [`sound-shapes.tex`](docs/manual-spectre/figures/sound-shapes.tex) | The sound interpretation guide changes |

Each manual's `sections/controls.tex` teaches the panel controls, and
`sections/control-values.tex` holds exact ranges and menu catalogs. Practical
experiments each have their own section file. Lead control descriptions with
what the musician sees and when to use the control. Define
technical terms at first use. Prefer an explanatory plot or annotated detail
to a diagram that merely repeats prose inside boxes. Keep the starting patch,
explicit settings, steps, expected observations, and interpretation together.
Explain what an experiment cannot establish as well as what it reveals. Check defaults and
preset values against the implementation, distinguish capture/analysis
changes from display changes, and state the sample rate for numerical timing
examples. Use schematic illustrations for relationships and identify ideal
or qualitative examples in their captions. New teaching sections should use
unnumbered subsections so they do not disrupt the panel's control numbering.
Rebuild to let `latexmk` resolve contents, bookmarks, and page references.

The window plots use calculated periodic-window responses, not Rack captures.
Their normalized data in `docs/latex/figures/window-response-data.tex` comes from
[`generate_window_figure.py`](scripts/generate_window_figure.py). To regenerate
from the repository root, run `python3 scripts/generate_window_figure.py`
(standard library only), then rebuild both manuals. Keep its coefficients
aligned with `src/dsp/window.hpp`. Edit the layout in the shared
`window-tradeoffs.tex`, not the generated coordinates. Ordinary PDF builds use
the checked-in data and still require no Python runtime. The color detail
reuses `PanelColorControl` from `panel-drawing.tex`; check both the detail and
the full Spectre panel after changing that primitive. The history figure
uses the same panel source with a teaching overlay; verify its scan-line
semantics and the ordinary panel reference after editing either.

The PNG and similarly named `.tex` file serve different purposes. The manual
includes both explicitly. Keep the old `Module.svg`, `Module.pdf`, and
`img/PanelLayout.pdf` assets retired; standalone vector exports belong in
the ignored output directory described below. For drawing-only edits, change
the TikZ source and run the ordinary manual build; no screenshot refresh or
Rack environment is needed.

**Revise the panel drawings.** The shared
[`panel-drawing.tex`](docs/latex/figures/panel-drawing.tex) defines the ink colors,
line weights, hardware primitives, callout badges, and legend typography.
Each module's `panel-layout.tex` provides the geometry and numbered key.
Coordinates use Rack pixels with a downward y axis; check them against
[`PanelLayout`](src/rack_extensions/panel.hpp) and the widget constructors in
[`SpectrumAnalyzer.cpp`](src/SpectrumAnalyzer.cpp) and
[`Spectrogram.cpp`](src/Spectrogram.cpp). Text-control positions are defined
in those constructors, not in the shared geometry header.

Keep fine hardware outlines quieter than the callouts. Use blue for Fourier
and purple for Spectre; numbers and labels must convey the same information
without color. Keep leaders outside the panel where possible, avoid crossings,
and preserve legibility at the actual size used on the manual page. Match
the numbered key to the control subsections after any insertion or reorder.
The screen axes and Spectre's palette swatches are explanatory marks, not
an additional representation of measured data or a specific palette.

**Export for web and print.** From the repository root, with Python 3
(standard library only), the manual's TeX packages plus `standalone`, and
Poppler's `pdftocairo` installed, run:

```shell
make -C docs/latex/figures
```

This runs [`export_panel_drawings.py`](scripts/export_panel_drawings.py) and
writes tightly cropped PDFs and SVGs to `.build/manual-figures/`:

| Filename Pattern | Use |
| --- | --- |
| `{fourier,spectre}-guide.{pdf,svg}` | Annotated panel plus numbered key |
| `{fourier,spectre}-panel.{pdf,svg}` | Panel illustration without callouts or key |
| `{fourier,spectre}-guide-mono.{pdf,svg}` | Grayscale annotated guide |
| `{fourier,spectre}-panel-mono.{pdf,svg}` | Grayscale panel illustration |

The braces describe filenames, not an argument to the exporter. To export
one module, use `make -C docs/latex/figures MODULE=spectre`. `PYTHON`, `PDFLATEX`,
and `PDFTOCAIRO` may be overridden with absolute executable paths. Build
logs are retained beside the exports. The SVGs contain vector paths,
including outlined text, so they need no external fonts or raster images;
edit the TikZ sources and regenerate rather than editing the outlined text.
Use the PDFs for print placement and the SVGs for web or vector editors.
Existing [visual-asset license terms](LICENSE.md) still apply.

Review the color and grayscale guides, the bare panels, and both manual
pages after changing shared primitives. Ensure every callout appears once
in its key, points to the intended control, and stays readable at print size.
Ordinary manual builds use the TikZ sources directly and do not require
Python or Poppler. The export command does not refresh screenshots or alter
the manuals. Keep generated PDFs, SVGs, and logs untracked; regenerate them
from the same source revision when preparing web or print material.

**Refresh the screenshot.** First [configure the Rack build environment](#configure-the-rack-sdk)
and ensure its `res/` assets are available, as required by
[native panel inspection](#choosing-validation). Run in a graphical
desktop session with working OpenGL. Python 3 with Pillow is also required;
`python3 -c 'from PIL import Image'` checks the default interpreter. The PDF
build additionally needs the TeX tools described above. From the repository
root, run the pair for the affected module in order:

```shell
make -C docs/manual-fourier screenshot
make -C docs/manual-fourier
```

```shell
make -C docs/manual-spectre screenshot
make -C docs/manual-spectre
```

The screenshot target rebuilds the inspector as needed and overwrites
the selected manual's `img/PanelLayout.png` only after panel inspection
succeeds. The second command builds its `.build/manual.pdf`. Run them
sequentially so the PDF includes the newly generated image. Each screenshot
target renders both modules but exports only the requested one.

If Pillow is missing, install it in your chosen Python environment. For
example, for Fourier on macOS or Linux, from the repository root:

```shell
python3 -m venv .build/manual-figures-venv
.build/manual-figures-venv/bin/python -m pip install Pillow
make -C docs/manual-fourier screenshot PYTHON="$PWD/.build/manual-figures-venv/bin/python"
make -C docs/manual-fourier
```

Use an absolute path for `PYTHON`, since Make runs the exporter from the
manual directory. The recursive build inherits `RACK_DIR` from the environment
or command line. A missing `plugin.mk` means that the Rack path needs fixing;
an installed Rack application by itself is not a build SDK. If window creation
fails or hangs in a headless or sandboxed session, rerun from a desktop session
with native window access. Preserve the checked-in PNG if capture cannot run;
an old `.ppm` is not evidence that the current panel was rendered. An
`unexpected inspector canvas` error requires checking the crop geometry,
not bypassing the exporter's validation.

The shared [`export_manual_screenshot.py`](scripts/export_manual_screenshot.py)
selects a crop with `--module fourier` or `--module spectre`. Both come from
the light-theme, unzoomed live scenario `panel-live-0.ppm` generated by
[`inspect_panels.cpp`](test/rack/inspect_panels.cpp). The fixture processes
100,000 samples at 48 kHz using default analysis controls and 5 V peak
sinusoids: 250, 500, 1000, and 2000 Hz on Fourier's four inputs, and 1000 Hz
on Spectre's input. Spectre shows partially filled history. Fourier's default
Slope makes its four equal-voltage inputs appear at different plotted levels.

The export preserves native pixel density without resizing or repainting.
Fourier is 720 by 380 logical pixels; Spectre is 525 by 380. At two-pixel
density the PNGs are 1440 by 760 and 1050 by 760, respectively. Antialiasing
and fonts can vary with platform and density; this is a repeatable capture
procedure, not a byte-identical cross-platform image. The original
`export-spectre-screenshot.py` command remains a compatibility entry point
that delegates to the shared exporter.

**Review the result.** Inspect the PNG for complete panel edges, legible
labels, the expected theme, and the fixed signals' traces or partial history.
Inspect the rendered cover and every changed diagram page for clipping, overlaps,
and page breaks. Poppler's `pdfinfo` and `pdftoppm` can help; from the
repository root, after the manual build (use `manual-spectre` for Spectre):

```shell
pdfinfo docs/manual-fourier/.build/manual.pdf
mkdir -p .build/manual-review/fourier
pdftoppm -scale-to 1200 -png docs/manual-fourier/.build/manual.pdf .build/manual-review/fourier/page
git diff --check
git status --short
```

Review the generated page PNGs and the source diff before committing. Record
the OS, Rack version, pixel dimensions, refresh/build results, and visual
checks in the change description or owning spec. Native rendering verifies
the fixture; it does not replace an interactive Rack session when one is
required for a behavior change. Commit the reviewed screenshot and edited
source files, leaving captures, page renders, and compiled PDFs untracked.

Keep the capture geometry in
[`export_manual_screenshot.py`](scripts/export_manual_screenshot.py)
aligned with [`inspect_panels.cpp`](test/rack/inspect_panels.cpp) if its canvas
or module placement changes. The refresh command is explicit: ordinary
manual builds and PDF CI use the reviewed PNG and need no Rack installation,
desktop session, or Pillow. Keep all other capture output in `.build/`.

### Publication Builds

Build the self-contained white paper with `latexmk` and its TeX packages:

```shell
make -C docs/whitepaper
```

This writes `docs/whitepaper/.build/paper.pdf` without running experiments.

The [manuals and white paper workflow](.github/workflows/manuals.yml) builds
all three PDFs on relevant pull requests and pushes to `main`: changes to
either manual directory, the shared `docs/latex/` directory,
the white paper's source or Makefile, `plugin.json`,
or the workflow itself. Version tags matching `v*` build PDFs regardless of
path filters. Other branch pushes do not run the workflow. New PR or `main`
updates cancel obsolete PDF builds; tag and release runs are kept separate
so a tag build cannot cancel release uploads.

Ubuntu installs `texlive-latex-extra`, `texlive-fonts-recommended`,
`texlive-science`, `latexmk`, `lmodern`, and `poppler-utils`. CI checks that
all PDFs are nonempty and readable by `pdfinfo`, then saves `Fourier.pdf`,
`Spectre.pdf`, and `Fourier-whitepaper.pdf` in the `publication-pdfs` workflow
artifact for 14 days.

Publishing a GitHub release builds from its tag and attaches all three PDFs.
The manual asset names match `manualUrl` in `plugin.json`. A tag push builds
an artifact but does not create a release or upload release assets. Publish a
regular release as latest for Rack's `/releases/latest/download/` links to
resolve to these manuals; prerelease assets are also uploaded but do not
become the latest release. Hosting the paper on GitHub does not submit it to
arXiv or a journal.

To backfill an existing release, select **Actions > Manuals and white paper
> Run workflow** and enter its exact tag in `tag`. The workflow must first be
on the default branch. Sources and Makefiles come from that tag; older tags
retain their original build behavior, and tags predating the paper upload
only the two manuals. The release must already exist and allow asset changes.
Reruns replace matching assets without changing release notes or other assets.
Only the upload job has `contents: write`; builds use read-only repository
permissions and require no additional secrets. GitHub's built-in token does
not trigger a release workflow when another workflow creates the release with
that token; use the manual trigger in that case.

Keep generated binaries, object files, PDFs, and build folders out of source
changes. Bare `make` builds the Rack plugin; tests and benchmarks require
the explicit targets above.

## Submit A Pull Request

Preserve file-level attribution and the source and artwork terms in
[LICENSE.md](LICENSE.md).

1.  Update affected user documentation, presets, and resources alongside
    behavior changes. Keep unrelated formatting and dependency updates out
    of the diff.
2.  Review the changes and check whitespace from the repository root:

    ```shell
    git diff --check
    git diff
    git status --short
    ```

3.  Commit the intended files, push your branch to your fork, and open a
    pull request against `main`. Complete the
    [pull request template](.github/PULL_REQUEST_TEMPLATE.md) with the problem,
    resulting behavior, relevant issue, and validation commands and results.
4.  Include your OS, compiler, and Rack/SDK versions when relevant. For manual
    checks, record sample rate, settings, and observations; include screenshots
    for visual changes. State any checks you could not run and why.

[issues]: https://github.com/Kautenja/ArhythmeticUnits-Fourier/issues
[testing]: #development-and-testing
[architecture]: #architecture
[cpp-style]: docs/style-guides/cpp.md
[markdown-style]: docs/style-guides/markdown.md
[rack-building]: https://vcvrack.com/manual/Building
[rack-tutorial]: https://vcvrack.com/manual/PluginDevelopmentTutorial

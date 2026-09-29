# Development And Testing

Run the commands below from the repository root. The two build systems
serve different purposes: SCons verifies standalone DSP, while Make builds
the VCV Rack plugin.

## Dependencies

For standalone tests, use Python with SCons, a C++11-capable compiler
available as `g++`, and the repository's Catch2 headers. `SConstruct` currently
sets `CXX='g++'`; do not assume an environment `CXX` override changes it.
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

## Continuous Integration

The [DSP tests workflow](../../.github/workflows/dsp-tests.yml) runs
`scons test` on pull requests and pushes to `main`, including merges. Pushes
to other branches do not trigger a separate run. It checks out dependencies
recursively and verifies the Catch2 header is present. Its three jobs use
Ubuntu 24.04 with GCC, macOS 14 with Apple Clang, and Windows 2022 with
MSYS2 UCRT64 GCC. Windows uses MSYS2's SCons and Python to preserve POSIX
paths and GNU build tools. Each job runs the same `scons test` command.

On Linux, SCons disables Catch2 2.13.3's optional POSIX signal handler
because it requires a constant `MINSIGSTKSZ`, which modern glibc no longer
provides. Assertions and process failures still fail the job; only Catch2's
extra signal diagnostics are unavailable.

New updates cancel older runs for the same pull request or branch, and
each job has a 15-minute timeout to limit usage. A failure on one platform
does not cancel the other platforms, so their results remain available.

This workflow covers the standalone suites only. Rack plugin builds,
serialization integration tests, benchmarks, and manual UI checks remain
separate validation steps.

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
wraparound, and stress the mailbox with concurrent publication and reading.
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

The existing command and focused alias are:

```shell
scons benchmark
scons benchmark/dsp/benchmark_fft.cpp
```

Currently `benchmark/dsp/benchmark_fft.cpp` enables Catch2 benchmarking
but contains an empty `TEST_CASE`. A successful run supplies no performance
measurement. Add a meaningful workload before using it to evaluate changes.

For an optimization, compare baseline and candidate with the same compiler,
flags, host, signal, FFT size, hop length, and channel count. Separate setup
from the operation being measured. Record repeated samples, units, relevant
latency/throughput statistics, and variability or confidence intervals.
Report inconclusive measurements honestly. A microbenchmark does not by
itself establish whole-patch engine or display performance.

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
performance. See the [display caching measurements](../../specs/archive/001-display-caching.md)
for the baseline comparison, flags, memory tradeoffs, and validation limits.

## User Manuals And Build Products

The existing multi-file LaTeX manuals use their own Makefiles and require
`pdflatex`, BibTeX, and their referenced packages:

```shell
make -C manual
```

Outputs are `manual/build/Fourier.pdf` and `manual/build/Spectre.pdf`.
The child Makefiles recreate their local build directories and stop on
LaTeX or BibTeX errors. Shell escape is disabled. Inspect rendered pages
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

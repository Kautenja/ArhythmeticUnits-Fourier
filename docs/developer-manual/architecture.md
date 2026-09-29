# Architecture

This article maps Fourier's source and the boundaries to preserve when
changing DSP, Rack modules, or displays.

## Source Map

-   `src/plugin.cpp` registers the two models; `src/plugin.hpp` connects Rack,
    shared helpers, and the plugin instance.
-   `src/SpectrumAnalyzer.cpp` contains the Fourier module, display, and
    widget. It analyzes four input ports using `simd::float_4` lanes.
-   `src/Spectrogram.cpp` contains the Spectre module, spectral image display,
    and widget. It uses scalar processing and a history of spectra.
-   `src/structs.hpp` contains shared display/analysis enums and conversions.
-   `src/dsp/` contains mostly header-only math, filters, triggers, and music
    theory in a flat set of focused headers. Consumers include the headers
    they use directly.
-   `src/rack_extensions/` contains Rack graphics and control helpers.
-   `test/dsp/` mirrors the flat DSP header layout. `test/functions.hpp` and
    `test/ieee754.hpp` provide test helpers.
-   `benchmark/dsp/` holds the benchmark harness; see the testing article
    before treating it as performance evidence.
-   `res/` contains shipped graphics. `design/` holds editable Sketch sources.
-   `manual/Fourier/` and `manual/Spectre/` contain LaTeX user manuals and
    illustrations. Developer guidance lives here in `docs/developer-manual/`.
-   `patches/` and `presets/` provide Rack examples and saved module settings.
-   `notebooks/` contains exploratory DSP and window-function calculations.
    Notebooks support reasoning but do not replace production regression tests.

## Analysis Flow

Rack calls each module's `process(const ProcessArgs&)` for engine samples.
The modules normalize Eurorack input voltages, maintain DC-blocker state,
apply gain, and buffer samples. Windowed samples feed the incremental real
FFT. Once coefficients are ready, frequency and time smoothing prepare
data for the spectrum plot or spectrogram history. FFT work is distributed
through `step(...)` using the hop length.

Fourier prepares raster coordinates for four lanes. Spectre keeps a ring of
spectra and converts them to colored pixels in its display code. Rack widget
callbacks draw using NanoVG. Both modules declare zero audio outputs; their
observable results are analysis displays and persisted controls.

Read the actual processing functions before changing run/freeze semantics:
the two modules do not currently gate their processing identically. Also
check `onReset`, `onSampleRateChange`, `dataToJson`, and `dataFromJson` for
state transitions affected by a change.

## Ownership And Threading

DSP headers must remain usable by the standalone test build without Rack.
Generic templates may be instantiated with Rack SIMD values by module code;
the generic header should not need to include Rack to support that use.

Keep analysis on the engine side and NanoVG calls on the display side.
Document who owns mutable buffers, who reads them, and when a reader can
observe an update. Avoid new unsynchronized engine/UI sharing or locks that
could block the engine. Do not assume existing shared vectors and readiness
flags make concurrent access safe.

The current implementation includes allocation during reconfiguration:
for example, Fourier's `process_window()` resizes buffers from `process()`.
This is an existing limitation, not a pattern to extend. Any change to
buffer preparation must account for Rack callback threading and buffer
lifetime before moving work between threads.

## Compatibility

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

## Build Boundaries

The root `Makefile` compiles `src/*.cpp` into the Rack plugin using the
selected Rack tree's `plugin.mk`. Nested `.cpp` files are not automatically
included by that wildcard.

`SConstruct` builds standalone DSP tests and benchmarks and discovers
`.cpp` files recursively under `src/dsp`, `test`, and `benchmark`. It does
not build the Rack modules or exercise their SIMD instantiations and UI.
See [Development And Testing](development-and-testing.md) for commands.

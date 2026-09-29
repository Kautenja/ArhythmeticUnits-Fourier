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
    testing article for workloads and interpretation.
-   `res/` contains shipped graphics. `design/` holds editable Sketch sources.
-   `manual/Fourier/` and `manual/Spectre/` contain LaTeX user manuals and
    illustrations. Developer guidance lives here in `docs/developer-manual/`.
-   `patches/` and `presets/` provide Rack examples and saved module settings.

## Analysis Flow

Rack calls each module's `process(const ProcessArgs&)` for engine samples.
The modules normalize Eurorack voltages, maintain double-precision DC-blocker
state per input, and apply gain. The higher-precision filter state prevents
roundoff from accumulating as a DC offset in short repeating signals; FFT and
display storage remain float (four SIMD lanes in Fourier).
`SpectrumAnalysis<T>` retains input and distributes windowing/packing,
butterflies, real-spectrum reconstruction, magnitude prefix sums,
frequency/time smoothing, and the output callback over one exact hop.

For M=N/2, K=M+1, B=(M/2)log2(M), a frame contains W=M+B+2K work units.
Sample s executes `floor((s+1)W/H)-floor(sW/H)` units. A quotient/remainder
accumulator implements that schedule without per-sample quota division.
Frames end at input indices jH and publish at jH+H-1, starting with zero
padding. This intentionally replaces the earlier restart-on-FFT-completion
cadence. Settings latch at each frame start; mid-frame changes apply next hop.
The original `OnTheFlyFFT/RFFT` APIs remain available for other DSP users.
The [technical report](../../whitepaper/fourier.tex) derives the work bound,
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

## Ownership And Threading

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
`.cpp` files recursively under `src/dsp`, `test`, and `benchmark/dsp`.
Rack benchmark and test sources are excluded and built separately by Make.
SCons does not build the Rack modules or exercise their SIMD instantiations
and UI.
See [Development And Testing](development-and-testing.md) for commands.

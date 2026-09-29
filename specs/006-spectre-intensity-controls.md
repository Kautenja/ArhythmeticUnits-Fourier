# Spectre Intensity Controls

Add independent display Floor and Ceiling controls and a calibrated color
legend to Spectre, using the free panel strip between its single input/gain
and Run button. Make quiet spectral detail inspectable without recapturing
audio at a different input gain.

Status: PLANNED

Created: September 29, 2026

## Goal And Current Behavior

The [Spectre display](../src/Spectrogram.cpp) currently maps slope-weighted
linear magnitudes to a color map using `N_FFT / 2` as its divisor. Values
above the color range saturate. Input Gain changes newly acquired spectra;
it cannot adjust recorded history while frozen. The UI already retains raw
columns and recolors them when display settings change.

Provide a dB intensity mode with independent endpoints and recoloring of
existing history. Preserve the old appearance for existing patches through
an explicit legacy mode. This spec is independent of
[Fourier trace inspection](005-fourier-trace-inspection.md).

## Behavior Examples

-   A new Spectre starts with Floor = -90 dB and Ceiling = 0 dB. A -45 dB
    weighted spectral value maps to the midpoint of the selected color map.
-   With Run off, changing Floor from -90 to -60 dB immediately recolors
    retained history; no new input or FFT frame is needed.
-   Raising Ceiling to +12 dB reveals distinctions between positive-dB
    regions that previously shared the top color. It does not change input
    gain or the underlying spectral values.
-   Opening an older patch selects `Legacy linear` and reproduces its
    existing colors. Choosing `Decibels` enables the new panel controls.

## Functional Requirements

### FR-1: Panel Placement And Parameter Behavior

- [ ] Add two compact physical knobs labeled `FLOOR` and `CEIL`, each with
    a visible numeric dB value and Rack tooltip. Stack them in Spectre's
    unused left strip below its existing input gain and above Run.
- [ ] Use the existing [shared panel geometry](../src/rack_extensions/panel.hpp).
    Initial knob centers are `(23, 140)` and `(23, 215)` Rack pixels.
    Fit new labels, values, hit areas, and knobs inside `x = 6..40`,
    `y = 105..250`; reserve `y = 260..305` for the color legend. Refine
    spacing after rendering, keeping all additions in this strip.
- [ ] Preserve the 35 HP, 525-by-380-pixel module size, display rectangle,
    input and gain positions, existing bottom controls, and Run center
    `(23, 346)`. Keep its label, screws, and branding unobstructed. Do not
    change Fourier's panel when adding Spectre-specific geometry.
- [ ] Append `PARAM_COLOR_FLOOR` and `PARAM_COLOR_CEILING` after all existing
    parameter IDs. Floor ranges from -120 to -1 dB, default -90 dB;
    Ceiling ranges from 0 to +24 dB, default 0 dB. These deliberately
    disjoint ranges guarantee at least 1 dB of span without moving the
    other knob or introducing coupled undo actions.
- [ ] Use continuous 1-decimal-place display and ordinary Rack knob
    dragging, fine adjustment, typed entry, reset, and undo/redo behavior.
    Disable parameter randomization, consistent with existing controls.
    Sanitize non-finite values to defaults and clamp finite out-of-range
    values before display calculations, including externally set values.
- [ ] In legacy mode, visibly dim the new knobs/values and explain in their
    tooltips that they affect Decibels mode only. Preserve their settings;
    adjusting them must not silently switch modes or recolor legacy output.

### FR-2: Intensity Mapping And Reference

- [ ] Add undoable context-menu `Intensity scale` choices `Decibels` and
    `Legacy linear`. New instances and module reset use Decibels mode.
    Keep `Color Map` as the independent existing palette choice.
- [ ] In Decibels mode, for an interpolated magnitude `m`, use
    `a = abs(m) / (N_FFT / 2)` and `d = 20 * log10(a) + slope_db`.
    Map color position as `clamp((d - floor) / (ceiling - floor), 0, 1)`.
    Interpolate linear magnitudes before converting to dB. Feed this
    normalized position into every existing color map without changing
    palette tables.
- [ ] Compute `slope_db = slope * log2(f / 1000)` at the physical frequency
    of the sampled fractional bin for the new mode, on either frequency
    scale. At DC, use zero slope weighting. This intentionally removes the
    old image-row-based weighting error only in Decibels mode. Preserve
    the old weighting and pixel calculations in Legacy linear mode.
- [ ] Zero magnitude maps to the lowest color without a logarithm-domain
    error. Positive infinity saturates at the highest color; NaN/invalid
    data maps to the lowest color and reads `--`. Do not send non-finite
    color coordinates into palette indexing. Check behavior under the
    plugin's actual floating-point compiler flags.
- [ ] Keep the normalization reference explicit: the analyzer divides
    input voltage by 5 V, applies input gain and window coherent-gain
    correction, and publishes unnormalized magnitudes. An isolated,
    bin-centered 5 V peak sinusoid at unity gain, with AC coupling off,
    smoothing off and zero slope, measures approximately 0 dB. This is
    a spectral amplitude reference, not dBFS or broadband RMS. Preserve
    existing DC/Nyquist conventions; do not silently apply endpoint factors.
- [ ] In Decibels mode, align the hover readout with this exact divisor.
    Replace the current approximate `20 * log10(m) - 60` with the exact
    conversion for that mode (about a -0.206 dB correction at N=2048).
    Preserve the existing unweighted-bin meaning and identify it as `Raw`;
    additionally show `Color` for the weighted, interpolated value used by
    the hovered pixel. Silence reads `-inf dB`. Document the difference.
    Legacy mode retains its historical readout and mapping.

### FR-3: Legend And Display Lifecycle

- [ ] Draw a compact color gradient and endpoint values in the reserved
    left strip, using the selected palette. Show floor, ceiling, and `dB`
    in Decibels mode. Label the legend `LIN` with normalized endpoints
    `0` and `1` in legacy mode; never present a false linear-dB legend.
- [ ] Communicate that values at/below Floor or at/above Ceiling saturate
    to endpoint colors. Identify slope weighting in the legend tooltip;
    numeric inspector values must not be clamped to the legend endpoints.
- [ ] Changes to either endpoint or intensity mode invalidate all cached
    spectral pixels and the legend, including while frozen. Palette and
    slope changes retain their corresponding invalidation. Changes must
    not mutate retained magnitudes, restart capture, or advance the scan.
- [ ] Reuse existing history and texture ownership. Unchanged draws must
    not upload an image. Cropping/resizing must still reuse spectral pixels;
    view mapping alone must not recompute the intensity data. Context loss,
    failed texture creation, widget recreation, and reset must remain safe.
- [ ] Render readable labels and values in both themes at 100 percent zoom,
    with no overlap/clipping at 75 and 50 percent zoom. A null-module browser
    preview shows the new defaults and a valid legend without dereferencing
    engine state or creating history actions.

### FR-4: Compatibility And Ownership

- [ ] Serialize intensity mode as custom JSON `intensity_scale`, accepting
    strings `decibels` and `legacy_linear`. Missing or invalid values on
    load select Legacy linear, including loading an old preset into an
    existing new-mode module. New parameters serialize through Rack.
- [ ] Missing endpoint parameters on load receive -90 and 0 dB defaults,
    even when loading into a previously edited module. Define and test this
    at the full Rack deserialization seam, not only `dataFromJson()`.
    Complete new state round-trips both endpoints and the selected mode.
- [ ] Keep existing plugin/module slugs, enum values and parameter/port/light
    identities, input normalization, gain default, smoothing, FFT length,
    hop cadence, and Run/freeze semantics. No new input or output is added.
- [ ] Route menu changes through the existing synchronized undo helper.
    Keep color conversion, legend generation, and all recoloring on the UI
    thread. The audio path must not read the new display parameters, allocate
    storage for them, or perform any new per-sample work.
- [ ] Existing factory preset files without new fields retain their current
    appearance. Do not bulk-migrate them. Add one explicitly labeled dB
    inspection preset demonstrating the new controls, with exact new fields.
    Update preset enumeration/round-trip tests accordingly.

## Non-Goals

Automatic gain/range, time-axis controls, new color palettes, spectral export,
new ports, display resizing, FFT changes, a true logarithmic frequency-axis
redesign, and changes to Fourier are outside this spec. This does not remove
existing lifecycle costs or establish hard real-time safety.

## Acceptance And Validation

- [ ] Test independent mapping examples: -90/-45/0 dB at a -90..0 dB range
    map to 0/0.5/1; values outside the range saturate. Check silence,
    non-finite values, parameter bounds, every palette, and zero/nonzero
    slopes at DC, 500 Hz, 1 kHz, 2 kHz, and Nyquist on both frequency scales.
- [ ] Extend [amplitude tests](../test/rack/test_module_amplitudes.cpp) for
    the stated sine reference and exact hover conversion, including gain,
    DC/Nyquist, and 44.1/48/96 kHz. Keep independently derived expectations
    and existing numerical tolerances. Test fractional-bin color samples
    separately from stored-bin Raw values.
- [ ] Extend [display tests](../test/rack/test_display_lifecycle.cpp) for
    frozen recoloring, all new invalidation inputs, unchanged-draw upload
    counts, history immutability, legend consistency and texture lifecycle.
    Retain independent legacy pixel fixtures proving old output unchanged.
- [ ] Extend [serialization tests](../test/rack/test_serialization.cpp) for
    old patches/presets, missing/malformed mode and endpoints, repeated
    loads into edited modules, new-state round trips, reset and undo/redo.
    Verify stable existing numeric IDs explicitly.
- [ ] Extend [panel inspection](../test/rack/inspect_panels.cpp) for new
    knob/legend bounds, both modes, long/extreme values, themes, zoom and
    browser preview. Render and visually inspect the resulting images.
- [ ] Update the Spectre manual with knob positions, units, reference,
    frozen recoloring, saturation, mode migration and the new preset.
    Include the exact slope/readout differences between the two modes.
- [ ] In a live Rack session, compare an old saved patch and a new instance,
    manipulate both controls during running/frozen capture, verify ordinary
    knob interactions and undo/redo, reload a patch, and change sample rate.
    Record OS, Rack version, settings, and screenshots. Verify Fourier's
    panel remains unchanged.

Run from the repository root with the dependencies and `RACK_DIR` setup in
[Development And Testing](../CONTRIBUTING.md#development-and-testing):

```shell
make test
make test-rack
make -j4 all
make inspect-panels
make -C docs/manual-spectre
git diff --check
```

`inspect-panels` requires a graphical desktop. Inspect its generated images
and rendered manual PDF. Record plugin compilation, headless tests, and the
manual Rack session as separate results. Benchmark any claimed performance
change with repeated comparable workloads; upload-count assertions alone
prove caching behavior, not a speedup.

## Planning Evidence And Completion

September 29, 2026: reviewed module parameters, raw history ownership, color
conversion, hover normalization, panel coordinates, manuals and Rack tests.
This is a specification only; implementation acceptance remains open.

Planning validation: a Python path/anchor and structure check passed for both
new specs (16 relative links total), and validation commands were checked
against the repository and local Rack Make targets. Staged whitespace/diff
checks passed. No DSP tests, Rack build, manual build, or interactive Rack
session was run for this documentation-only change.

Record implementation decisions, actual validation results, manual checks,
and limitations here. Once all acceptance criteria are verified, mark this
spec `Status: COMPLETE`, move it to `specs/archive/`, and update links.

# Spectre Intensity Controls

Add a single vertical color-range control with draggable Floor and Ceiling
limits to Spectre, using the free panel strip between its single input/gain
and Run button. Make quiet spectral detail inspectable without recapturing
audio at a different input gain.

Status: IMPLEMENTED - live interaction validation pending

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
    existing colors. Clicking `Enable dB` below the bar enables its range handles.

## Functional Requirements

### FR-1: Panel Placement And Parameter Behavior

- [x] Use one vertical color bar with two draggable limits, a lower-left
    Floor handle and an upper-right Ceiling handle. Show their values below
    and above the bar. Integrate the palette legend into the control.
- [x] Use the existing [shared panel geometry](../src/rack_extensions/panel.hpp).
    Fit the entire control inside `x = 6..40`, `y = 105..305`, preserving
    the input/gain and Run regions. Both handles share a fixed -120..+24 dB
    axis and occupy opposite sides to remain reachable when close together.
- [x] Preserve the 35 HP, 525-by-380-pixel module size, display rectangle,
    input and gain positions, existing bottom controls, and Run center
    `(23, 346)`. Keep its label, screws, and branding unobstructed. Do not
    change Fourier's panel when adding Spectre-specific geometry.
- [x] Append `PARAM_COLOR_FLOOR` and `PARAM_COLOR_CEILING` after all existing
    parameter IDs. Floor ranges from -120 to -1 dB, default -90 dB;
    Ceiling ranges from 0 to +24 dB, default 0 dB. These deliberately
    disjoint ranges guarantee at least 1 dB of span without moving the
    other handle or introducing coupled undo actions.
- [x] Use continuous 1-decimal-place display, vertical handle dragging,
    fine adjustment, Rack typed entry/reset menus, and one undo action per
    drag. Display-only parameters update immediately without audio smoothing.
    Disable parameter randomization, consistent with existing controls.
    Sanitize non-finite values to defaults and clamp finite out-of-range
    values before display calculations, including externally set values.
- [x] In legacy mode, hide the inactive handles, show a `LIN` legend, and
    provide an explicit `Enable dB` action directly below the bar. Activating
    it switches modes through synchronized undo. Loading old patches and
    presets still preserves their original appearance and saved endpoints.

### FR-2: Intensity Mapping And Reference

- [x] Add undoable context-menu `Intensity scale` choices `Decibels` and
    `Legacy linear`. New instances and module reset use Decibels mode.
    Keep `Color Map` as the independent existing palette choice.
- [x] In Decibels mode, for an interpolated magnitude `m`, use
    `a = abs(m) / (N_FFT / 2)` and `d = 20 * log10(a) + slope_db`.
    Map color position as `clamp((d - floor) / (ceiling - floor), 0, 1)`.
    Interpolate linear magnitudes before converting to dB. Feed this
    normalized position into every existing color map without changing
    palette tables.
- [x] Compute `slope_db = slope * log2(f / 1000)` at the physical frequency
    of the sampled fractional bin for the new mode, on either frequency
    scale. At DC, use zero slope weighting. This intentionally removes the
    old image-row-based weighting error only in Decibels mode. Preserve
    the old weighting and pixel calculations in Legacy linear mode.
- [x] Zero magnitude maps to the lowest color without a logarithm-domain
    error. Positive infinity saturates at the highest color; NaN/invalid
    data maps to the lowest color and reads `--`. Do not send non-finite
    color coordinates into palette indexing. Check behavior under the
    plugin's actual floating-point compiler flags.
- [x] Keep the normalization reference explicit: the analyzer divides
    input voltage by 5 V, applies input gain and window coherent-gain
    correction, and publishes unnormalized magnitudes. An isolated,
    bin-centered 5 V peak sinusoid at unity gain, with AC coupling off,
    smoothing off and zero slope, measures approximately 0 dB. This is
    a spectral amplitude reference, not dBFS or broadband RMS. Preserve
    existing DC/Nyquist conventions; do not silently apply endpoint factors.
- [x] In Decibels mode, align the hover readout with this exact divisor.
    Replace the current approximate `20 * log10(m) - 60` with the exact
    conversion for that mode (about a -0.206 dB correction at N=2048).
    Preserve the existing unweighted-bin meaning and identify it as `Raw`;
    additionally show `Color` for the weighted, interpolated value used by
    the hovered pixel. Silence reads `-inf dB`. Document the difference.
    Legacy mode retains its historical readout and mapping.

### FR-3: Legend And Display Lifecycle

- [x] Draw the selected palette inside the vertical range control, with
    endpoint values above and below it. Show floor, ceiling, and `dB`
    in Decibels mode. Label the legend `LIN` with normalized endpoints
    `0` and `1` in legacy mode; never present a false linear-dB legend.
- [x] Communicate that values at/below Floor or at/above Ceiling saturate
    to endpoint colors. Identify slope weighting in the legend tooltip;
    numeric inspector values must not be clamped to the legend endpoints.
- [x] Changes to either endpoint or intensity mode invalidate all cached
    spectral pixels and the legend, including while frozen. Palette and
    slope changes retain their corresponding invalidation. Changes must
    not mutate retained magnitudes, restart capture, or advance the scan.
- [x] Reuse existing history and texture ownership. Unchanged draws must
    not upload an image. Cropping/resizing must still reuse spectral pixels;
    view mapping alone must not recompute the intensity data. Context loss,
    failed texture creation, widget recreation, and reset must remain safe.
- [x] Render readable labels and values in both themes at 100 percent zoom,
    with no overlap/clipping at 75 and 50 percent zoom. A null-module browser
    preview shows the new defaults and a valid legend without dereferencing
    engine state or creating history actions.

### FR-4: Compatibility And Ownership

- [x] Serialize intensity mode as custom JSON `intensity_scale`, accepting
    strings `decibels` and `legacy_linear`. Missing or invalid values on
    load select Legacy linear, including loading an old preset into an
    existing new-mode module. New parameters serialize through Rack.
- [x] Missing endpoint parameters on load receive -90 and 0 dB defaults,
    even when loading into a previously edited module. Define and test this
    at the full Rack deserialization seam, not only `dataFromJson()`.
    Complete new state round-trips both endpoints and the selected mode.
- [x] Keep existing plugin/module slugs, enum values and parameter/port/light
    identities, input normalization, gain default, smoothing, FFT length,
    hop cadence, and Run/freeze semantics. No new input or output is added.
- [x] Route menu changes through the existing synchronized undo helper.
    Keep color conversion, legend generation, and all recoloring on the UI
    thread. The audio path must not read the new display parameters, allocate
    storage for them, or perform any new per-sample work.
- [x] Existing factory preset files without new fields retain their current
    appearance. Do not bulk-migrate them. Add one explicitly labeled dB
    inspection preset demonstrating the new controls, with exact new fields.
    Update preset enumeration/round-trip tests accordingly.

## Non-Goals

Automatic gain/range, time-axis controls, new color palettes, spectral export,
new ports, display resizing, FFT changes, a true logarithmic frequency-axis
redesign, and changes to Fourier are outside this spec. This does not remove
existing lifecycle costs or establish hard real-time safety.

## Acceptance And Validation

- [x] Test independent mapping examples: -90/-45/0 dB at a -90..0 dB range
    map to 0/0.5/1; values outside the range saturate. Check silence,
    non-finite values, parameter bounds, every palette, and zero/nonzero
    slopes at DC, 500 Hz, 1 kHz, 2 kHz, and Nyquist on both frequency scales.
- [x] Extend [amplitude tests](../test/rack/test_module_amplitudes.cpp) for
    the stated sine reference and exact hover conversion, including gain,
    DC/Nyquist, and 44.1/48/96 kHz. Keep independently derived expectations
    and existing numerical tolerances. Test fractional-bin color samples
    separately from stored-bin Raw values.
- [x] Extend [display tests](../test/rack/test_display_lifecycle.cpp) for
    frozen recoloring, all new invalidation inputs, unchanged-draw upload
    counts, history immutability, legend consistency and texture lifecycle.
    Retain independent legacy pixel fixtures proving old output unchanged.
- [x] Extend [serialization tests](../test/rack/test_serialization.cpp) for
    old patches/presets, missing/malformed mode and endpoints, repeated
    loads into edited modules, new-state round trips, reset and undo/redo.
    Verify stable existing numeric IDs explicitly.
- [x] Extend [panel inspection](../test/rack/inspect_panels.cpp) for new
    handle/control bounds, both modes, long/extreme values, themes, zoom and
    browser preview. Render and visually inspect the resulting images.
- [x] Update the Spectre manual with handle positions, units, reference,
    frozen recoloring, saturation, mode migration and the new preset.
    Include the exact slope/readout differences between the two modes.
- [ ] In a live Rack session, compare an old saved patch and a new instance,
    manipulate both handles during running/frozen capture, verify ordinary
    handle interactions and undo/redo, reload a patch, and change sample rate.
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

### Implementation Evidence

September 29, 2026: implemented the two appended Rack parameters (IDs 9 and
10), UI-only dB mapping, physical-frequency slope weighting, Raw/Color
inspection, live legend, synchronized undoable mode selection, and legacy
migration. The two control centers remain at the proposed coordinates.
The existing audio processing functions and Fourier panel are unchanged.

Endpoint validation uses IEEE-754 bit classification so unsafe floating-point
optimization cannot discard non-finite checks. Finite saved JSON doubles are
clamped before narrowing. Rack's full `Module::fromJson()` seam supplies
missing endpoint defaults and legacy mode on repeated old-preset loads.
Rack bypasses `ParamQuantity::fromJson()` internally, so the two endpoint
entries are explicitly reapplied through their validating quantities.

The legend draws small vector strips directly and owns no texture. Legacy
mode excludes endpoint values from the image cache key, so adjusting dimmed
controls preserves both pixels and upload counts. Color inspection reports
the image row's interpolated magnitude before NanoVG texture filtering; Raw
continues to report a stored unweighted bin. The old five factory presets
are untouched; `DecibelInspection.vcvm` explicitly selects Decibels with
-90/+12 dB endpoints. The Spectre manual now includes a current native panel
image, formulas, migration behavior, and the new preset.

Validation on macOS 26.6.2 ARM64 with Apple Clang and the local Rack 2.6.0
source-tree library, from the repository root:

-   `make test`: passed all 13 standalone suites.
-   `make -j4 test-rack`: passed all five existing headless Rack suites.
    After the final serialization/hover refinements,
    `make -j4 all test-serialization test-display-lifecycle test-module-amplitudes`
    passed again: 5,827 assertions / 8 serialization cases; 89,922 assertions /
    13 display cases; 5,495,783 assertions / 6 amplitude cases. The unchanged
    coordinate and SIMD suites previously passed 98,316 and 3,964,514
    assertions respectively. Tests retain the plugin's `-O3` and
    `-funsafe-math-optimizations` flags. No numerical tolerances in existing
    tests were weakened.
-   `make -j4 all`: passed the C++11 macOS ARM64 plugin build. Existing Rack
    SDK deprecation warnings remain. Other plugin platforms were not built.
-   `make inspect-panels`: passed 60 native OpenGL render scenarios after
    granting desktop access. The sandbox-only renderer could not initialize
    desktop services and was stopped. Inspected light/dark, dB/legacy,
    default/extreme values, 100/75/50 percent zoom, and null-module previews.
    Images are under `.build/test/rack/panel-{live,preview}-*.ppm`; the review
    contact sheet is `.build/test/rack/intensity-contact.png`. Knob/label and
    legend bounds have executable assertions; Fourier artwork is unchanged.
-   `make -C docs/manual-spectre`: passed; the 12-page PDF was rendered with
    Poppler and visually inspected, including the new panel illustration,
    intensity controls, reference/readouts, and saved-settings pages.
-   `git diff --check`: passed. Relative spec links and referenced source,
    tests, preset, and manual artwork paths were checked.

Live validation used VCV Rack Pro 2.6.3 on the same Mac. Installed the built
plugin with `make install` and loaded a temporary test patch containing a
Fundamental VCO at approximately 1 kHz, an old-format Spectre, a Decibels
Spectre, and Fourier. Both Spectres used unity input gain, Flattop, no
smoothing, zero slope, AC coupling off, and a 0..20 kHz view. Screenshots in
this chat show the legacy dimmed controls/0..1 LIN legend alongside the
active -90..0 dB controls and additional quiet harmonic detail. Fourier's
panel remained intact.

The live interaction acceptance remains open: desktop-control calls timed
out intermittently, and coordinate clicks/drags did not reliably reach
Rack's custom controls. Running/frozen knob gestures, typed entry, fine
adjustment, live undo/redo, and live sample-rate changes were therefore not
verified. Automated freeze/recolor, state round trips, reset, menu undo/redo,
and sample-rate/lifecycle tests passed; these are not a substitute for the
remaining manual checks. The original unsaved Rack patch was preserved at
`/tmp/Before-spec-006-check.vcv` and restored after the test. The temporary
validation patch is `/tmp/fourier006-live.vcv`. The final validated package
is installed for Rack to pick up on its next restart; the live sample rate
was not confirmed.

Keep this spec active until those interaction checks are verified, then
mark COMPLETE and archive it. No speedup or hard real-time claim is made.
Existing lifecycle allocation/reset costs remain as documented in the
contributor guide. No release, commit, or push was performed.

### Vertical Control Revision

The user found the two knobs confusing and expected one vertical control.
Replaced the knobs and separate legend with a single color bar using two
opposing draggable limits. The bar shows the selected intensity range on a
fixed -120..+24 dB axis, with saturation outside the handles. The parameter
IDs, defaults, serialization, and numerical mapping are unchanged.

The earlier inactive-knob behavior in legacy patches is now explicit:
handles disappear, the bar reads LIN, and an Enable dB button activates the
range through synchronized undo. No automatic migration changes old colors.
The new handles use immediate parameter updates, Rack's typed-entry and
reset menus, fine adjustment, and one history entry per completed drag.
This supersedes the earlier knob/legend geometry and dimmed-knob descriptions
in the initial implementation evidence above.

Revision validation on the same macOS ARM64 environment:

-   `make -j4 all test-serialization test-display-lifecycle`: passed the
    Rack plugin build, 5,862 assertions / 10 serialization cases, and
    89,924 assertions / 13 display cases. These cover immediate handle
    changes, endpoint independence, fine adjustment, typed values, reset,
    one undo entry per drag, legacy activation, and frozen recoloring.
-   `make inspect-panels`: passed all 60 native render scenarios, including
    presses routed through the real module widget tree and actual drag
    callbacks followed by undo. Inspected light/dark, legacy/dB, extreme
    endpoints, and 100/75/50 percent zoom. Disabled strip-edge antialiasing
    to eliminate gradient seams at fractional zoom. Fourier is unchanged.
-   `make -C docs/manual-spectre`: rebuilt the 12-page manual with the new
    panel image and handle instructions; rendered pages were inspected.

`git diff --check` passed. `make install` installed the revised package
for Rack to load on its next restart. No commit, push, or release was made.

A full interactive Rack session remains pending. The native event tests
and automated running/frozen checks verify the implementation seams but
are not recorded as manual pointer gestures in the Rack application.
The spec remains active until the remaining live acceptance checks pass.

# Spectre Intensity Controls

Add a single vertical color-range control with draggable Floor and Ceiling
limits to Spectre, using the free panel strip between its single input/gain
and Run button. Make quiet spectral detail inspectable without recapturing
audio at a different input gain.

Status: IMPLEMENTED - live interaction validation pending

Created: September 29, 2026

## Goal

Give Spectre a usable vertical color-range surface with first-class Linear
and Decibels modes, direct palette/scale selection, and immediate recoloring
of retained history. Preserve saved patch identities and old linear pixels
at default endpoints. This spec is independent of
[Fourier trace inspection](005-fourier-trace-inspection.md).

## Behavior Examples

-   New instances use Decibels at -90..0 dB. A -45 dB value uses the palette
    midpoint; a -60 dB component uses one third of the range.
-   The ceiling can move to -20 dB to inspect quiet peaks. Endpoints stop
    before crossing; editing one limit never moves the other.
-   Linear uses amplitude endpoints of 0..100 percent by default. A -60 dB
    component uses 0.1 percent of that palette, explaining why it looks
    cleaner than the default Decibels view without filtering any data.
-   Switch mode or palette from the color screen. Each mode remembers its
    own endpoints; frozen history recolors immediately.
-   Old patches select Linear at 0..100 percent, reproducing their original
    colors. Both handles work immediately; there is no activation button.

## Functional Requirements

### FR-1: Panel And Interaction

- [x] Use a rounded black color screen at `(6, 106)`, size `63 x 206`,
    within a widened left control strip. Preserve the 35 HP module size.
    Move Spectre's plot to `(75, 15)` with size `435 x 350` and evenly
    space its seven bottom controls. Fourier's geometry stays unchanged.
- [x] Center input, a larger gain knob, color screen, and Run at x=37.5.
    Input and gain centers are about 41 pixels (13.9 mm) apart. Provide
    22-by-20-pixel handle hit targets on opposite sides of a 120-pixel bar.
    This improves physical-style clearance; it is not a hardware prototype.
- [x] Put palette and intensity-scale readouts with dropdown arrows at the
    top of the screen. Left-click opens each list directly. Remove the
    redundant context-menu palette list; retain the intensity-scale menu.
- [x] Support vertical dragging, fine adjustment, typed values, reset, and
    one undo action per drag. Disable smoothing and randomization. Clamp
    edits at the other limit without changing the sibling parameter.

### FR-2: Ranges And Mapping

- [x] Preserve IDs 0..10, including dB Floor/Ceiling at 9/10. Both use the
    shared -120..+24 dB axis with at least 0.1 dB separation. Defaults stay
    -90/0 dB. A ceiling below zero and a positive floor are supported.
- [x] Append Linear Floor/Ceiling as IDs 11/12: amplitude 0..2, displayed
    as 0..200 percent, with a minimum span of 0.001 (0.1 percentage point).
    Defaults are 0/1. Keep the two modes' ranges independently remembered.
- [x] Decibels uses interpolated linear magnitude, normalized by N/2,
    converted with 20 log10, plus physical-frequency slope weighting.
    DC remains unweighted. Palette position is the normalized position
    between floor and ceiling, clamped to 0..1.
- [x] Linear retains the original magnitude interpolation and image-row
    slope weighting for pixel compatibility, then normalizes amplitude
    between its endpoints. Its 0..1 range exactly matches old pixels.
    Document the historical slope difference on logarithmic frequency axes.
- [x] Keep invalid palette coordinates safe: zero and NaN use the bottom
    color; infinity uses the top. Validate with plugin optimization flags.
- [x] Preserve the existing Raw/Color dB inspection reference and the
    historical Linear readout. No new signal energy, hidden gating,
    smoothing, or engine processing is introduced by display controls.

### FR-3: Persistence And Lifecycle

- [x] Preserve saved JSON `decibels` and `legacy_linear` strings. The latter
    is a compatibility wire value; the UI now calls the mode Linear.
    Missing/invalid mode loads Linear. New instances/reset use Decibels.
- [x] Full loads reset missing endpoints to each mode's defaults. Validate
    finite bounds before narrowing, then repair crossed saved pairs by
    lowering the floor to one minimum span below the ceiling. Load order
    must not affect the result. Non-finite external values use defaults.
- [x] Use Rack's synchronized undo helper for both panel dropdowns and
    context-menu scale selection. Patches preserve mode, palette, and
    both ranges without changing module slugs, port/light IDs or DSP.
- [x] Only active endpoints affect image cache invalidation. Changed
    endpoints, scale, palette, or slope recolor retained history, including
    while frozen, without recapture or moving the scan line. Unchanged
    draws and crop/resize operations reuse pixels. Preserve texture and
    mailbox ownership, context-loss recovery, and null-module previews.
- [x] Keep the five original presets unchanged and the DecibelInspection
    preset supported. Missing appended Linear parameters use defaults.
    Align the manual, panel artwork, and spec with the revised interface.

## Non-Goals

Automatic range/gain, noise filtering, new palettes/ports, time-axis controls,
FFT changes, a true logarithmic frequency-axis redesign, and changes to
Fourier are outside this spec. No hard real-time or performance claim is made.

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

### Supported Scales And Panel Revision

Committed the preceding implementation as `37715a7` before this revision.
The user requested ceilings below zero, first-class Linear/Decibels modes,
more physical control spacing, a black color screen, and direct palette and
scale selection. The requirements above supersede the earlier geometry,
disjoint ranges, hidden Linear handles, and Enable dB button descriptions.

Kept the dB defaults rather than silently filtering weak data. A deterministic
5 V tone plus a -60 dB secondary tone confirms that the same stored spectrum
maps its weak component to 0.001 in Linear and one third in Decibels at
-90..0 dB; raising the floor to -50 dB hides it. The manual explains this
visibility change and how to obtain a quieter background.

Validation on macOS ARM64 with the local Rack 2.6.0 source-tree library:

-   `make -j4 all test-serialization test-display-lifecycle test-module-amplitudes`
    passed. After the final reset and drag regressions, `make test-serialization`
    passed 5,890 assertions / 12 cases; display lifecycle passed 89,929
    assertions / 13 cases; amplitude passed 5,495,792 assertions / 7 cases.
    Checks include negative ceiling gestures, typed/clamped endpoints,
    minimum spans, reversed/malformed JSON, reset, old presets, mode/palette
    undo, independent ranges, active/inactive cache invalidation, and the
    known weak-tone comparison. Existing numerical tolerances are unchanged.
-   `make -j4 all` passed the C++11 Rack plugin build. Existing SDK
    deprecation warnings remain. No other platform build was performed.
-   `make inspect-panels` passed 60 native OpenGL scenarios. It routes
    presses through actual module widgets, drags both modes' handles,
    opens both dropdowns, selects real menu items, and verifies undo.
    Inspected both themes, modes, extremes, 100/75/50 percent zoom and
    browser previews. The review sheets are
    `.build/test/rack/color-screen-review.png` and `color-screen-zooms.png`.
-   `make -C docs/manual-spectre` built the 12-page PDF. Poppler-rendered
    pages were inspected, including the new panel illustration and controls.
-   `git diff --check` and relative spec link checks passed.
-   `make install` installed the updated package for Rack's next restart.

This follow-up remains uncommitted; the requested pre-revision checkpoint
is `37715a7`. Unrelated analyzer/benchmark and saved-patch work is preserved.
No push or release was performed. A full manual Rack interaction session
remains pending; native automated event/render checks are not claimed as
hands-on use. Keep the spec active until that acceptance check is verified.

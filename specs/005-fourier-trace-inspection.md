# Fourier Trace Inspection

Make Fourier's cursor measure the four displayed spectra, with optional
nearest-peak snapping and a pinned frequency marker for repeatable inspection.

Status: PLANNED

Created: September 29, 2026

## Goal And Current Behavior

The [Fourier display](../src/SpectrumAnalyzer.cpp) currently reports the
cursor's frequency, note, cents, and vertical-axis value. The amplitude is
not sampled from a trace. Its engine publishes complete, mapped curve
snapshots through a single-producer/single-consumer mailbox.

Replace the vertical-axis readout with four measured trace levels. Preserve
the current analysis schedule, input routing, amplitude normalization,
capture behavior, module footprint, and saved parameter identities.
This spec is independent of
[Spectre intensity controls](archive/006-spectre-intensity-controls.md).

## Behavior Examples

-   With two different input levels at the same frequency, hovering that
    frequency shows both levels. Moving vertically leaves the measured
    values unchanged when peak snapping is off.
-   Selecting trace 2 and enabling peak snapping finds its nearest visible
    local peak. Frequency, note, and cents describe that selected FFT bin;
    all four level readouts use the same bin.
-   Clicking pins the selected frequency. Moving away or leaving the plot
    retains the marker and readouts while incoming spectra update their
    values. Clicking again replaces the pinned frequency.
-   A pinned frequency survives a widget recreation in the same session.
    Loading a saved patch restores the marker setting, but acquires fresh
    spectra; it does not restore an old measurement or audio buffer.

## Functional Requirements

### FR-1: Measured Values And Units

- [ ] Select the bin nearest the pointer's horizontal position in the
    currently displayed curve snapshot. Resolve an exact tie to the lower
    bin. Do not derive measurements from the pointer's vertical position,
    clipped pixels, or Bezier interpolation.
- [ ] Show the selected bin's physical frequency, `k * sample_rate / N`,
    and its note/cents using the existing tuning convention. Identify the
    measurement as a bin value; do not imply sub-bin pitch estimation.
- [ ] Show four labeled values, `1` through `4`, with the existing channel
    colors as an additional cue. Report dB on either logarithmic magnitude
    scale and percent on the linear scale. Use one decimal place for levels;
    preserve useful frequency and cents precision from existing formatters.
- [ ] Preserve the historical amplitude reference: for published magnitude
    `m` and `K = N / 2 + 1`, unweighted level is `20 * log10(m / K)` dB.
    Input gain, AC coupling, window normalization, and smoothing have already
    affected `m`. Do not introduce broadband RMS or voltage claims.
- [ ] The main readout measures the displayed, slope-weighted trace. Use the
    same latched slope multiplier as the curve. Label the readout `Weighted`
    when slope is nonzero; expose the unweighted dB value in its tooltip.
    Linear percent follows the existing curve's linear-axis convention,
    including its historical +12 dB versus 400 percent approximation.
- [ ] Zero magnitude displays `-inf dB` or `0.0%`. Invalid/non-finite data
    displays `--` and is ineligible for peak detection. Levels outside the
    visible vertical range still show their measured value without clipping.
    Before the first completed snapshot, show `--` for every channel.

The existing [coordinate mapping](../src/rack_extensions/spectrum_coordinates.hpp)
uses `k / K` for horizontal placement, rather than `k / (N / 2)`, and its
Logarithmic mode uses a square-root transform. This feature must not silently
replace either mapping. Locate the marker at the selected bin's stored plot
position, but report the bin's physical frequency from its frame metadata.
Document the small existing axis-position discrepancy and test it explicitly.
Correcting the frequency-axis mapping is separate work.

### FR-2: Peak Snapping

- [ ] Add an undoable context-menu `Inspector` group with `Snap to peak`
    (default off) and `Peak trace` choices 1 through 4 (default 1).
- [ ] Search only finite, positive local maxima of the selected trace whose
    plotted positions lie within the visible horizontal bounds. Compare
    slope-weighted linear magnitudes, independent of magnitude-axis scale.
- [ ] Define a plateau as one peak at its lowest bin index when the adjacent
    bins outside the plateau are lower. DC and Nyquist qualify using their
    single neighbor; cropping must not manufacture a peak at a crop edge.
    An entirely flat or silent spectrum has no peaks.
- [ ] Choose the peak nearest the pointer in plotted horizontal distance;
    ties choose the lower-frequency peak. With no candidate, fall back to
    ordinary nearest-bin inspection and show no peak indicator.
- [ ] Mark the selected trace at its measured level and draw a vertical
    guide shared by all four readings. A snapped marker visibly indicates
    `Peak`. Changing the peak trace must not change channel routing or gain.

### FR-3: Pinning And Interaction

- [ ] A left click inside the plot pins the currently selected frequency in
    Hz. Pinning stores a frequency, not a bin number or a peak-tracking rule.
    Future frames choose the nearest physical bin to that fixed frequency.
    With a pin active, hover movement does not move the inspector.
- [ ] Add undoable `Clear pinned frequency` to the Inspector menu. Escape
    while the plot owns keyboard focus also clears the pin. A new plot click
    replaces it using a fresh selection at the click position, including
    peak snapping if enabled. Right-click opens Rack's module menu.
- [ ] If the pinned frequency falls outside the visible bounds or current
    Nyquist limit, retain it but hide the marker and show `Outside range`.
    Restoring the range restores inspection. Reset clears the pin and returns
    inspector preferences to their defaults.
- [ ] Without a pin, retain the existing hover confinement to the plot
    rectangle. Controls, labels, module dragging outside the plot, and Rack
    shortcuts must keep working. Browser previews with a null module must
    render safely and must not create undo actions or capture focus.
- [ ] Fit the four readouts and frequency/note information inside Fourier's
    existing display, using a compact inset or reserved readout band. Do not
    move existing knobs, ports, or the Run button or increase the 48 HP width.
    Keep text readable in both themes at 100 percent zoom; check 75 and
    50 percent zoom for overlap and clipping.

### FR-4: Snapshot Ownership And Persistence

- [ ] Extend the existing mailbox payload with preallocated per-bin linear
    magnitudes and frame metadata needed by the inspector: active N, sample
    rate, latched display mapping/slope, and a publication revision. Curves,
    measurements, and metadata must come from the same completed frame.
    Make axes and inspector use coherent mapping while newer controls await
    publication; never interpret an old curve using a new FFT length.
- [ ] Populate magnitude slots during the existing scheduled bin callback.
    Do not add an engine-side full-frame copy, allocation, lock, peak scan,
    string formatting, or another mailbox consumer. Cache peak candidates
    on the UI side per publication and inspector settings.
- [ ] Keep stable, module-owned inspector settings separate from UI-local
    hover state. Route setting changes through Rack's synchronized,
    undoable setting path; do not add unsynchronized engine/UI mutations.
- [ ] Add custom JSON fields `inspector_snap_to_peak` (boolean),
    `inspector_peak_trace` (integer 0 through 3), and
    `inspector_pinned_hz` (nonnegative finite number or null). Missing or
    malformed fields use defaults. Preserve valid out-of-range pins for
    later sample-rate/range restoration. Do not serialize magnitudes,
    cursor position, candidates, or publication revisions.
- [ ] Preserve all existing parameter, input, output, and light IDs, model
    slugs, presets' analysis settings, and current Run/freeze behavior.
    Run off can still publish analysis of retained input; pinning does not
    create a true frozen spectrum. Document this distinction.

## Non-Goals

Reference capture, peak hold/decay, polyphonic voice selection, CV inputs,
frequency-axis redesign, sub-bin interpolation, tuning-reference controls,
FFT changes, amplitude recalibration, and export are outside this spec.
There is no claim that this work makes the complete module real-time safe;
existing lifecycle allocation and other documented limitations remain.

## Acceptance And Validation

- [ ] Extend [spectrum tests](../test/rack/test_spectrum_points.cpp) with
    independently computed bin selections, levels, plateaus, ties, cropping,
    endpoints, silence, and invalid values. Cover both frequency scales,
    all magnitude scales, FFT lengths 128/2048/16384, nonzero slopes, and
    44.1/48/96 kHz. Prove physical-bin readout versus legacy plot placement.
- [ ] Extend [amplitude tests](../test/rack/test_module_amplitudes.cpp) with
    analytical tones on all four lanes, different gains, DC and Nyquist,
    and established normalization tolerances. Pointer Y and Bezier mode
    must not affect measurements. Do not calculate expected values by
    calling the production inspector helper.
- [ ] Cover publication coherence across mid-frame settings, reset and
    sample-rate changes, retained snapshot ownership, and allocation-free
    processing/live controls. Peak work must stay outside `process()`.
- [ ] Extend [serialization tests](../test/rack/test_serialization.cpp) for
    defaults, invalid JSON, pin/prefs round trips, reset, and undo/redo.
    Extend [display tests](../test/rack/test_display_lifecycle.cpp) for hover,
    pin replacement/clear, out-of-range restoration and widget recreation.
- [ ] Update both manuals where they compare cursor behavior, and add
    screenshots/instructions for Fourier inspection. Retain existing patch
    and preset behavior. No release version bump is part of this spec.
- [ ] Manually verify four live inputs, silent inputs, frozen capture,
    mouse/keyboard actions, both themes, zoom, module browser, and an older
    patch. Record OS, Rack version, sample rate, settings and screenshots.

Run from the repository root with the dependencies and `RACK_DIR` setup in
[Development And Testing](../CONTRIBUTING.md#development-and-testing):

```shell
make test
make test-rack
make -j4 all
make inspect-panels
make -C docs/manual-fourier
make -C docs/manual-spectre
git diff --check
```

`inspect-panels` requires a graphical desktop; extend its scenarios for the
inspector and inspect its generated images. Inspect rendered manual PDFs.
Headless tests and the inspector executable do not replace a manual Rack
session. Any performance claim additionally requires comparable repeated
before/after workloads under the contributor guide's benchmark protocol.

## Planning Evidence And Completion

September 29, 2026: reviewed module processing, curve mapping, mailbox use,
hover rendering, manuals, panel geometry, and existing Rack tests. This is
a specification only; implementation acceptance remains open.

Planning validation: a Python path/anchor and structure check passed for both
new specs (16 relative links total), and validation commands were checked
against the repository and local Rack Make targets. Staged whitespace/diff
checks passed. No DSP tests, Rack build, manual build, or interactive Rack
session was run for this documentation-only change.

Record implementation decisions, actual validation results, manual checks,
and limitations here. Once all acceptance criteria are verified, mark this
spec `Status: COMPLETE`, move it to `specs/archive/`, and update links.

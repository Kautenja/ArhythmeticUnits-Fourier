# Display Caching

Status: COMPLETE

## Goal

Avoid repeated display work in Fourier and Spectre without changing analysis,
patch identities, display units, or interaction. Baseline: `02dafea`.

## Behavior And Requirements

- Cache backgrounds and axes in Rack framebuffers for both modules, and cache
  label strings/positions for live glyph rendering.
  Frequency bounds, scale, sample rate, size, zoom, and context changes must
  invalidate the relevant cache. Hover overlays remain live.
- Spectre converts and uploads an image only after a new spectrum, reset,
  pixel-affecting setting change, or texture recreation. Frequency cropping
  and widget resizing reuse the image.
- Publish Spectre columns through preallocated SPSC triple buffers. Engine
  publication must not allocate, lock, wait, or call graphics APIs. The UI
  owns the history it draws and can catch up after missed frames.
- Preserve lifecycle cleanup, failed-allocation retry, and frozen history.

## Non-Goals

No DSP scheduling or numerical changes, new controls, saved JSON changes,
Fourier waveform-buffer redesign, or whole-patch performance claims.

## Acceptance And Validation

- `make -j4` builds the Rack plugin.
- `make test-display-lifecycle test-serialization` covers image invalidation,
  lifecycle behavior, reset, frozen state, and saved settings.
- Exercise mailbox ownership under concurrent producer/consumer workloads.
- Measure baseline and candidate with the same Rack library, compiler flags,
  signal, sample rate, FFT/history dimensions, frame count, and repeated runs.
  Report display preparation separately from audio processing and distinguish
  headless measurements from GPU/window performance.
- Inspect diffs with `git diff --check`; record manual UI checks or their absence.

## Completion Evidence

Completed 2026-09-28. No parameter IDs, ports, JSON meanings, analysis windows,
FFT scheduling, or amplitude/frequency units changed.

### Decisions And Costs

- Spectre publishes DC through Nyquist magnitudes through one SPSC triple
  buffer per history column. Atomic exchange transfers slot ownership; a
  consumer-held slot cannot be overwritten. A recreated widget can also read
  the last consumer snapshot, preserving history without new audio frames.
- The UI owns its coefficient copy and dirty-column set. It caches frequency
  interpolation positions and slope gains. Only dirty columns are recolored;
  NanoVG receives a full image upload only when pixels changed or the context
  needs a new texture. Reset publishes all cleared columns.
- Static grids/backgrounds use Rack framebuffers. Text strings and coordinates
  are retained with the artwork, but glyphs draw live. Native rendering found
  missing magnitude labels after a scale change when text itself was baked
  into a framebuffer; live glyph rendering eliminated that failure.
- Snapshot/mailbox storage costs approximately 10 MiB of additional CPU memory
  per live Spectre. Null-module browser previews do not allocate that history.
  Framebuffers add DPI/zoom-dependent GPU memory (about 2.5 MiB for Spectre and
  3.5 MiB for Fourier at the tested 2x pixel ratio).
- Fourier waveform caching was not added: its existing engine/UI buffer sharing
  needs a separate ownership redesign. The static cache does not resolve that
  limitation. Existing reconfiguration allocations and remaining control-state
  sharing are not claimed to be fully real-time safe.

### Comparable Measurements

Baseline source: `02dafea`. Both binaries used the same local Rack `v2.6.0`
library, arm64 macOS 26.6.2 host, and Apple Clang 21.0.0. Flags from Rack's
Makefiles included C++11, `-O3`, `-funsafe-math-optimizations`, and
`-march=armv8-a+fp+simd`. Each version has 14 batches per workload (two runs,
seven repeats per run). The [raw measurements](001-display-caching.csv) retain
every batch. Setup and initial image creation were outside the timed draws.

Signal: 1 kHz sine, 5 V peak, 48 kHz, 8192 warmup samples, FFT 2048, hop 1024,
512 history columns, default window/smoothing/scale/color settings. Each
workload measured 120 frames. Running supplied 800 samples/frame; frozen had
no new samples; settings alternated slope between 4.5 and 3 dB/octave. The
renderer copies uploaded bytes but does not call a GPU driver.

| Workload | Before median (range), us/draw | After median (range), us/draw | Uploads before / after |
| --- | --- | --- | --- |
| frozen | 6496.374 (6366.979-7432.837) | 0.802 (0.766-2.803) | 121 / 1 |
| running | 6286.128 (5939.349-7101.089) | 53.987 (50.916-83.701) | 121 / 95 |
| settings | 6283.608 (5727.198-7349.053) | 5244.688 (3838.876-6703.377) | 121 / 120 |

Upload counts include the warmup image. In the settings workload the first
slope value matches the warmup, so the candidate skips that redundant update.

An engine-only workload separately processed 3,932,160 samples per repeat with
no intervening drawing. Median engine cost was 0.030971 us/sample before
(range 0.030635-0.046811) and 0.033885 afterward
(range 0.032504-0.074201). The median increased by about 9%; the wide, overlapping ranges make
the exact overhead uncertain. This is not evidence of an engine speedup. Frozen and running display preparation improved substantially; the
full-rebuild ranges overlap, so that case has no strong speedup claim. No GPU
frame-rate, Fourier frame-time, or whole-patch speedup is claimed.

Reproduction from the repository root (temporary paths are examples):

```shell
mkdir -p /tmp/fourier-display-baseline
git archive 02dafea src | tar -x -C /tmp/fourier-display-baseline
make -B build/benchmark/rack/display 'DISPLAY_BENCHMARK_FLAGS=-DFOURIER_SPECTROGRAM_SOURCE=\"/tmp/fourier-display-baseline/src/Spectrogram.cpp\"'
cp build/benchmark/rack/display /tmp/fourier-display-baseline/benchmark
make -B build/benchmark/rack/display
DYLD_LIBRARY_PATH=../.. LD_LIBRARY_PATH=../.. /tmp/fourier-display-baseline/benchmark
DYLD_LIBRARY_PATH=../.. LD_LIBRARY_PATH=../.. build/benchmark/rack/display
```

Run each executable twice while no builds or other validation workloads are
running. `make benchmark-display` runs just the candidate.

### Validation Results

- `make -j4`: passed with existing SDK deprecation warnings.
- `make test-display-lifecycle`: 95 assertions in 8 cases passed. Coverage
  includes pixel equality with the original full-image calculation, both
  frequency scales, crop/resize reuse, slope/color/sample-rate invalidation,
  reset, a full history wrap, context and widget recreation, failed texture
  creation, static cache keys, cached label layout, and concurrent mailbox use.
- `make test-serialization`: 5253 assertions in 6 cases passed.
- `make inspect-displays`: passed on native OpenGL at 2x pixel ratio. Reviewed
  log and linear display images and the 0.8x zoom image. No GL errors; both
  framebuffer handles remained valid. Images immediately before and after
  context recreation were byte-identical. This is a native Rack rendering
  harness, not a complete interactive Rack/DAW session.
- `git diff --check`: passed. Standalone DSP tests were not rerun because no
  reusable DSP algorithms changed.

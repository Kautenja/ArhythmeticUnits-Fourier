# Complete Modules And The Rack Engine

Phase 4 of [spec 014](../../../../specs/014-benchmark-reliability-and-scheduling-study.md)
prepares complete-module comparisons. Performance collection is user-owned.
The ordinary `fourier` and `spectre` backends retain their historical controlled
settings. Explicit v3 `fourier-default` and `spectre-default` use constructor
settings: Flattop, no smoothing, Fourier N=2048 and nominal 30 ms (H=1440 at
48 kHz), Spectre N=2048/H=1024. Every run records actual float panel values,
quantized hop, window, AC coupling, active ports, and voices per active port.
Fourier always computes four lanes; voices are summed into each port.

## Engine Boundary

`paper --engine PROFILE.json` calls the real Rack `Engine::stepBlock`, with
actual workers and barriers. The source inspected for this experiment is Rack
`8c33d966d329e4a6e354593b2b5f9ac2df5a03bd`. The offline preparation must retain
that revision, relevant source, and linked-library identities. Private Rack
APIs make this a pinned research harness, not a portable plugin API.

Each engine node owns a complete module. Its outer `process` supplies the
prepared voltages and invokes the module's processing path. This extra wrapper
and input injection are included for all contenders. A benchmark-only metadata
macro adds endpoint/publication coordinates to snapshots and a sequence counter;
it is absent from the shipped plugin. Compare measurements with that overhead
in view. The native controls retain unused production analyzer/display storage
in their control shell, so they are not a matched memory-footprint ablation.

Complete native modules use the production input normalization, DC blockers,
gain and display equations with native scalar Spectre or native four-channel
Fourier transforms. Preparation, smoothing, all coordinates/history values,
lights and mailbox publication are included. All slots are allocated before
measurement and remain alive through reset. Provider planning is outside the
measured blocks. FFTW's opaque allocator cost remains unknown.

Source inspection establishes that `stepBlock` acquires a block mutex and a
shared lifecycle lock, resets the calling FPU, and synchronizes workers on each
sample. Workers reset their FPU at launch and inherit the creating thread's
scheduler. On ARM64 the reset enables flush-to-zero and round-to-nearest. The
execution sidecar reads the calling thread's actual FPU/scheduler/QoS; worker
policy is source-derived, not a per-worker runtime observation. There is no
ideal division by thread count, audio-device callback, or underrun telemetry.

One empty engine sample launches workers before graph setup and settling.
Warmup is rounded up to complete blocks; its extent and measured origin are
explicit. Staggered nodes receive i*H/count prefill calls before warmup. A
separate 0-analyzer graph measures overhead without automatically subtracting
it. Background cases use the same 0/16/64 DC-filter cascade for every contender.

## Output And Consumer Contracts

`engine_host.validate` checks full block intervals, aggregate process CPU time,
lifecycle intervals, all-node replay coverage, publications and consumer rows.
Hop peaks are maxima of full engine blocks intersecting each frame's endpoint
to publication interval, including other modules' work. They are not isolated
per-analyzer CPU times. CPU totals include worker and consumer activity during
the measurement loop; parked-thread join and final drain are outside it.

The single consumer polls at 30 or 60 Hz, with optional skipped poll periods.
It only accesses mailbox snapshots. It reads no live controls or Spectre hop
index. It traverses values twice while holding each snapshot and checks the
sequence/content hash remains stable. This headless workload includes value
inspection and hashing, not rendering. Storage is reserved before settling.

Consumed age is bounded by completed-block watermarks on either side of the
poll, with one additional block on the upper bound. The final drain is labeled
separately and excluded from cadence-age estimates. Skips count measured
publications never consumed, not hypothetical monitor refreshes. Every observed
sequence/endpoint is checked against untimed replay. The replay independently
checks every bin, DC recurrence, EMA and Fourier coordinate mapping; native
replanning need not reproduce bitwise rounding. Hash stability verifies held
snapshot consistency, not bitwise equality between different FFTW plans.

## Lifecycle And Verification

Reset, freeze/resume, sample-rate (48 to 96 kHz), window, band and geometry
changes form separate continuous passes without the timed consumer. Changes
occur after `stepBlock` returns. Reset uses Rack's exclusive lifecycle lock;
sample-rate uses Rack's locked setter. Other controls are changed while workers
wait at the block boundary. These passes measure serialized lifecycle duration,
including lock acquisition; they do not manufacture or isolate contention from
an interactive UI thread. Reset retains panel quantities and resets internal
state/menu options, unlike resetting every panel knob through Rack's UI.

Both modules stop input conditioning when frozen. Fourier continues analysis
and publication of retained history; Spectre pauses analysis too. Window,
band and geometry controls latch at frame starts. Sample-rate changes cancel
pending work; the native benchmark controls also rebuild their plan/storage.
Those allocations belong to lifecycle diagnostics, not steady processing.
`paper-audit --engine-resources PROFILE.json` records serialized C++ allocation
counts with one engine thread and no clocks. Native allocations are unknown;
this audit cannot establish full real-time safety.

The example in `profiles/engine/fourier-default.json` is a prepared workload,
not evidence. From the repository root, these commands perform no benchmark:

```shell
make benchmark-paper-build PAPER_VDSP=1 PAPER_FFTW_PREFIX=.build/deps/fftw
DYLD_LIBRARY_PATH=../.. .build/benchmark/rack/paper --engine-verify docs/whitepaper/benchmarks/profiles/engine/fourier-default.json
python3 -m unittest discover -s docs/whitepaper/benchmarks/tests -p test_engine_host.py
make test-mailbox INSTRUMENT=tsan
```

The Python fixtures compile a separate executable with synthetic clocks. They
exercise actual engine graphs, default/extreme controls, native complete
modules, held snapshots through serialized reset, and recording errors. They
cannot qualify as performance evidence. The real user launcher must supply a
retained `PAPER_HOST_EXECUTION_PATH`, caffeinate guard and readiness checks.

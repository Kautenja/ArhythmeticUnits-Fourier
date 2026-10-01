# Complete Analysis Pipeline Experiment

This archived experiment extended Fourier's resumable-work idea from butterflies to
input preparation, real-spectrum reconstruction, magnitude smoothing, and
snapshot publication. It provides two bounded-work schedules with different
latency tradeoffs. It did not change the shipping Rack modules or manuscript
version 1. The removed specifications in Git history record production
integration, validation, and completion evidence.

## Historical Sources

The prototype has been superseded by `src/dsp/spectrum_analysis.hpp` and its
production regression suite. Its original source, comparison modes, driver,
and tests are retained in [source.tar.gz](source.tar.gz) solely to reproduce
this paper campaign. They are not part of the plugin or current test build.
See [the campaign notes](README.md) for reproduction and provenance checks.

## Mathematical Construction

Let the real frame length be N, the hop H, the packed complex length M=N/2,
and the number of positive bins K=M+1. The complex FFT requires
B=(M/2) log2(M) butterflies. The dependency chain has four stages:

| Stage | Units W | Work In One Unit |
| --- | ---: | --- |
| Prepare | M | Read two retained samples, apply their window weights, store a packed complex value at its bit-reversed index |
| Transform | B | Execute one radix-2 butterfly |
| Reconstruct | K | Reconstruct one positive bin, compute its magnitude, extend the prefix sum |
| Finish | K | Read a smoothing interval, update the bin's temporal EMA, write the producer-owned snapshot |

Windowing, packing, and permutation are fused. Reconstruction, magnitudes,
and prefix sums are fused. The fourth stage combines frequency smoothing,
time smoothing, and output writing. The final publication is one atomic
ownership exchange, with no frame-sized copy.

For W units over H sample calls, assign call s, for 0 <= s < H,

```text
q_W(s) = floor((s+1) W / H) - floor(s W / H).
```

Telescoping gives `sum(q_W)=W`. Each quota is either `floor(W/H)` or
`ceil(W/H)`. Thus every stage ends exactly at the hop boundary, even when W
is not divisible by H or H exceeds W. There is no restart-on-early-completion
behavior. Integer arithmetic avoids floating-point rounding of quotas; a
64-bit product covers all validated N and H combinations.

### Serial Schedule: Preserve Latency

Concatenate the four stages into one dependency-ordered sequence of
`W_total=M+B+2K` units. Spend `q_W_total(s)` units on each sample, crossing
stage boundaries when needed. Every frame finishes within its original hop.
Its per-sample bound is `ceil(W_total/H)` units plus constant overhead.

This is the direct extension of the existing butterfly budget to the whole
analysis graph. It uses one frame workspace. Its limitation is that a
reconstruction unit, a butterfly, and a copy/window unit have unequal costs,
so equal unit counts do not imply uniform elapsed time through the hop.

### Overlapped Schedule: Mix Stages On Every Sample

Give each stage its own hop, applying its quota simultaneously to successive
frames. Four slots rotate through the stages; only indices rotate, not data.
After filling the pipeline, all four types of work occur throughout every
hop. Its per-call bound is

```text
ceil(M/H) + ceil(B/H) + 2 ceil(K/H)
```

plus constant input/scheduling/publication overhead. At N=4096, H=1024,
this is at most 19 units: 2 preparation units, 11 butterflies, 3
reconstruction units, and 3 finish units. Most calls execute 17 units;
reconstruction/finish each have one extra unit on the final call. The serial
schedule has an 18-unit bound here, spent on successive stages.

| Hop Interval | Prepare | Transform | Reconstruct | Finish/Publish |
| --- | --- | --- | --- | --- |
| 0 | Frame 0 | - | - | - |
| 1 | Frame 1 | Frame 0 | - | - |
| 2 | Frame 2 | Frame 1 | Frame 0 | - |
| 3 | Frame 3 | Frame 2 | Frame 1 | Frame 0 |
| 4 | Frame 4 | Frame 3 | Frame 2 | Frame 1 |

For input frames ending at sample indices `jH`, the serial schedule publishes
at `jH+H-1`; the overlapped schedule publishes at `jH+4H-1`. Both publish
exactly every H calls. Overlap therefore adds `3H` samples of latency. At
48 kHz this is 64 ms for H=1024 and 256 ms for H=4096. Spectrum age relative
to the frame's temporal center adds another `(N-1)/2` samples to either
endpoint-age figure. UI scheduling can add further age.

### Retain Input Without A Snapshot Copy

Both candidates insert one sample into a ring of N+H values. At each frame
boundary, record the oldest sample's ring index. The next H-1 insertions
cannot overwrite any sample in that frame: its oldest value is overwritten
only after H+1 further insertions. Preparation can consequently read the
retained frame incrementally, including with a very large hop. Zero-filled
initial storage implements negative-time zero padding. No contiguous
N-element copy is needed at a boundary.

### Numerical And Ownership Contracts

The experimental butterfly and reconstruction formulas match the current
scalar RFFT. Only positive bins are reconstructed; real-input symmetry makes
negative bins unnecessary for the published magnitude spectrum. This is an
output specialization, not a new FFT factorization. Each positive smoothing
interval lies in DC through Nyquist, so its prefix-sum arithmetic is the same
as the positive part of the original full-spectrum pass.

Octave bounds are computed during preparation using the original frequency
formulas, including the Nyquist-edge adjustment. With smoothing disabled,
use the magnitude directly; subtracting adjacent float prefix sums would
unnecessarily lose precision in small bins. The normalized window is supplied
by the caller. Temporal smoothing is maintained in producer-owned state,
independent of which snapshots the consumer reads or drops.

All processing storage and all three publication slots are prepared before
processing. The existing [DisplayMailbox](../../../../../../src/rack_extensions/display_mailbox.hpp)
provides single-producer/single-consumer acquire/release ownership exchange.
A producer fills its slot across the finishing stage, then publishes it.
A consumer can hold a snapshot until its next successful consume; producer
progress cannot mutate that held snapshot. A stalled UI drops intermediate
frames without blocking the producer. Publication requires lock-free unsigned
atomics, enforced by the mailbox's compile-time check.

Configuration is immutable: N is a power of two from 4 through 16384,
1 <= H <= 65536, 0 <= alpha <= 1, 0 <= octave <= 1, and sample rate lies
between 1 and 1,000,000 Hz. Window values must be finite. Input samples are
assumed finite. Reset or reconfiguration means constructing a new instance
while both producer and consumer are stopped. This is deliberately not a
live Rack configuration solution.

On a 64-bit host with 4-byte floats, approximate vector payload storage is
`66N+4H+80` bytes for overlap and `42N+4H+44` bytes for serial. The original
adapter uses approximately `52N+16` bytes. These omit object/vector metadata,
allocator overhead, and the caller's configuration and signal buffers.
At N=16384, H=4096 these are about 1.047 MiB, 0.672 MiB, and 0.813 MiB,
respectively. Extra overlapped workspaces have a measurable memory cost.

## Controlled Comparison

| Mode | Arithmetic | Scheduling | Endpoint Age At Publication |
| --- | --- | --- | ---: |
| `legacy` | Current full RFFT, original smoothing/EMA, positive-output copy | Frame starts forced to exact H; butterflies distributed; remaining phases burst | H-1 |
| `stage_burst` | Candidate positive-spectrum arithmetic and precomputed bounds | All four stage workloads run at each hop's final sample | 4H-1 |
| `serial` | Same candidate arithmetic | Balanced total quota on one dependency chain | H-1 |
| `pipeline` | Same candidate arithmetic | Balanced per-stage quotas across four frames | 4H-1 |

The `stage_burst` versus `pipeline` comparison isolates placement for the
same arithmetic, storage, cadence, and latency. Comparing either candidate
to `legacy` also includes removal of redundant negative-bin work, copies,
and repeated bound calculations; do not attribute that entire difference
to scheduling. `serial` versus `legacy` keeps the same publication latency.
The original modules restart on completion rather than a strict H cadence,
so this adapter measures a controlled workload, not their exact execution.

Timing uses scalar float, normalized periodic Hann, alpha=0.8, and smoothing
off or one-third octave at 48 kHz. Sizes are 1024, 4096, and 16384, with hops
N/4 and N/2. Every mode receives the same deterministic noise sequence.
Twenty warmup hops fill the pipeline; each pass then measures 64 hops.
Twelve repetitions rotate the four mode positions evenly.

Separate fresh-instance passes measure:

-   `block=0`: elapsed time per whole hop, with no per-sample timer reads;
    `hop_ns` estimates aggregate processing cost.
-   `block=1`: every sample call, retaining its median, p99, and maximum
    per repetition. Timer overhead is included and can dominate small calls.
-   `block=16,64,256`: consecutive simulated blocks measured with one timer
    pair per block. These are synchronous loop batches, not audio-device
    callbacks, and include no competing DSP.

Output values are consumed after each timing region so output computation
remains observable. This consumer is synchronous for timing; concurrent
ownership is tested separately. The archived CSV contains per-repetition
summaries, not every individual sample/block observation. Min/max ranges in
the summary describe variation across repetitions, not confidence intervals.
The isolated baseline phase pass uses nine repeats, 16 warmup frames and
64 measured frames, and includes timer overhead.

## Interpretation And Limits

The one-hop schedule is the first candidate for future integration because
it removes whole-frame processing bursts without increasing the fixed-hop
baseline's analysis latency. Overlap is an alternative when mixing different
stage costs on each sample matters enough to justify three additional hops
and larger working storage. The subsequent production change selects the serial schedule.

Operation-count bounds are deterministic; elapsed-time bounds are not.
Cache effects, divisions, square roots, atomics, compiler choices, OS
preemption, and unequal task costs remain. H=1 necessarily concentrates a
frame's work in one sample. The experiment does not prove a novel scheduling
algorithm or a hard real-time guarantee.

The scope ends at a published magnitude snapshot. Rack's input routing,
SIMD lanes, dynamic controls, reset/sample-rate callbacks, coordinate/color
mapping, spectrogram history management, and rendering remain outside it.
At the time of this experiment, shipping Fourier still shared mutable curve
buffers and reconfiguration allocated on the engine thread. Those historical
limitations should not be read as a description of the later integration. Device callbacks,
multiple analyzers, background load, optimized external FFTs, and additional
architectures remain necessary before making plugin-level performance claims.

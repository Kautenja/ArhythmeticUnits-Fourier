# Parameter-Transition Evidence

The `interactive-v1` workload measures ordinary analyzer control changes in
[the existing callback harness](../../../benchmark/paper/protocol.hpp).
Its versioned manifest, lifecycle checker, and table derivation live in
[transitions.py](transitions.py). The C++ adapters and separate correctness
replay live in [transitions.hpp](../../../benchmark/paper/transitions.hpp).
This is research code; neither Rack module includes these adapters.

## Supported Controls And Policies

The production scalar `core-float` and `core-double` cases call the existing
`SpectrumAnalysis.configure()` API at its actual frame boundary. That API
rejects mid-frame calls; it does not enqueue, cancel, or coalesce requests.
The benchmark host retains the latest full requested configuration until the
next boundary. A newer pending request replaces the previous pending request.
An active frame always completes. The event trace distinguishes replacement,
application, publication, and requests still pending at the observation end.

`pffft-hybrid-float` and `pffft-scheduled-batch-float` provide a matched pair of
**benchmark transition controls**, extending the fixed-configuration
`ScheduledAnalysis` task pipeline. They share the same window, magnitude,
band, EMA, and output work, and prepared native PFFFT plans. The immediate
control executes the ordered tasks at the frame endpoint; the hybrid
balances the `N + 1 + 2K` tasks over the hop. The opaque native FFT is one
indivisible task. Their transition engine policies are explicitly recorded
as `prepared-native-balanced` or `prepared-native-immediate`. These are not
claims that PFFFT exposes a streaming reconfiguration API, nor measurements
of the ordinary fixed-size `ExternalAnalysis` adapter.

Other registered adapters, including the ordinary native batch adapters,
legacy analyzers, independent-channel/SIMD cases, and Rack modules, are
unsupported by this transition suite and rejected. Their fixed workloads
remain available in the ordinary campaign matrix.

Both initial and target native plans are prepared before timing; maximum
input and working buffers are also prepared. The scalar analyzer similarly
prepares maximum capacities. Length application clears input availability
and EMA logically, without clearing or reallocating the physical ring. The
first new frame therefore contains its endpoint input and zero padding.
Hop, window, frequency smoothing, and temporal smoothing changes retain
input and EMA history. Disabling temporal smoothing stores the current
magnitude, so re-enabling it uses current history. Native band intervals use
binary64 index arithmetic; production scalar intervals use binary32. The
reference explicitly follows each profile.

Requests are processed before input capture. Actual configuration calls,
request replacement, cache rebuilding, and all output stores occur inside
the timed callback. No-op requests still invoke configuration. The adapter's
small host request bookkeeping is included in both change and no-change
runs. The allocation probe traverses the entire event sequence; it counts
C++ allocations separately from timing. Opaque allocator and stack bytes
remain unknown. The trace identifies both actual timed native plans and the
prepared lengths, rather than reporting only the initial plan.

Only a complete spectrum is published. The last complete old spectrum may
remain visible while the next frame is pending; partially overwritten work
buffers are not independent publications. These benchmarks have no display
consumer, actual audio device, or wall-clock UI latency measurement.

## Versioned Request Sequence

`N` and `H` below are the initial length and hop. The target length is
`min(8N, 16384)`. The suite accepts power-of-two initial lengths from 128 to
2048 and hops of at least eight samples. Its declared horizon is the actual
callback count times callback size, at least `26H`; callback rounding is
preserved. Sample zero is startup, with zero-padded input before it.

| Request Sample | Desired Change | Placement For The Change Sequence |
| --- | --- | --- |
| `2H` | Increase FFT length | Frame boundary |
| `4H + 1` | Restore initial FFT length | Early processing |
| `6H + floor(H/2)` | Increase hop to `2H` | Middle processing |
| `11H - 2` | Change Hann to Blackman-Harris | Late processing |
| `13H - 1` | Enable one-third-octave and temporal smoothing | Immediately before publication |
| `15H` | Request identical settings | No-op at a boundary |
| `17H + 1` | Request Hann | Pending request |
| `17H + 2` | Replace pending request and disable temporal smoothing | Replacement before application |
| `21H` | Decrease hop to `floor(H/2)` | Frame boundary |
| `23H + 1` | Disable frequency smoothing | Retained-history change |
| `horizon - 1` | Request boxcar | Explicit finite-horizon outcome |

For odd hops and callback-rounded horizons the last request may have a
different application outcome. The independent lifecycle model derives it;
it does not force a missing response or delay an immediate result. The
no-change control requests the initial settings at exactly the same input
coordinates, retaining request and configuration overhead. Its boundaries
remain at the initial hop; phase labels above describe the change sequence.

## Correctness And Reporting

The untimed replay independently predicts every request, pending replacement,
boundary application, frame endpoint, generation, history-reset coordinate,
and publication. Every bin of every publication is checked. The reference
uses direct long-double DFTs at `N <= 256` and the independently implemented
recursive binary64 FFT above that size, followed by direct band summation
and an EMA state that never reads actual outputs. Input and float window
bytes match the measured precision contract. The trace records the platform
mantissa widths and uses FR-11's `spectrum-norms-v1` numerical policy.

Each publication retains its instance/channel identity, generation, input
endpoint, publication sample, history origin, bin count, absolute error,
reference magnitude, relative L2/Linf errors, and legacy pointwise diagnostics.
The Python checker separately regenerates the complete event and publication
sequence, checks raw CSV ages and counts, and validates numerical diagnostics
and prepared-plan identities. Corrupt generations, endpoints, missing bins,
missing events/publications, and non-finite or mixed spectra have negative
fixtures. A pending request at the observation horizon is a recorded outcome,
not silently classified as failure or infinite latency.

Response tables derive request-to-application and request-to-first-publication
latencies in samples and milliseconds. These are algorithmic response times.
Compare providers only for matched requested settings and input origins;
batch publications are never delayed to imitate a scheduled provider.

Cost tables select callback intervals overlapping the window from one initial
hop before each request through four initial hops after it, clipped to the
observation horizon. Windows intentionally overlap. Every selected callback
index links back to the retained raw CSV; mean, nearest-rank p99, observed
maximum, and timer p99 are reported per process. Small smoke windows prove
implementation, not tail stability. Preserve process/session variation and
the no-change controls for subsequent comparative work. None of these values
are WCET bounds, device underruns, or UI latency.

## Short Validation

From the repository root, the deterministic fixture needs Python 3 and a
C++11 compiler; it does not need Rack. It compiles with optimized benchmark
math flags and exercises production float/double, matched scheduling, history
changes, cancellation/replacement identity, and negative mutations:

```shell
PYTHONPATH=docs/whitepaper/benchmarks python3 -m unittest test_transitions
```

The bounded native smoke requires the ordinary prepared Rack build dependencies.
It covers eight small change/control configurations and two length changes
from 2048 to 16384, with one repeat and minimum observation windows:

```shell
python3 docs/whitepaper/benchmarks/run.py .build/transition-smoke \
    --config docs/whitepaper/benchmarks/configs/transition-smoke.json \
    --variant rack --phase smoke --repeats 1 --hops 2 --warm-hops 0
python3 docs/whitepaper/benchmarks/check.py .build/transition-smoke
```

Use a fresh output directory. Full pilot/confirmation experiments belong to
the separately launched FR-13 workflow; these fixtures do not establish new
performance claims.

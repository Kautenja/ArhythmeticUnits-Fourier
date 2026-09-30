# Scheduling Experiment Readiness

These decisions belong to phase 5 of
[spec 014](../../../../specs/014-benchmark-reliability-and-scheduling-study.md).
They prepare comparisons without selecting an optimization or collecting times.

## Ready Comparisons

The matched core batch/distributed pair changes work placement while preserving
arithmetic and storage. Native batch/hybrid pairs use contiguous native kernels
with the same input, output, smoothing and retained storage. These controls
remain the first comparisons before introducing more kernels.

For a native hybrid v3 workload, `experimental_policy` additionally accepts:

| Policy | Completion Horizon H_c | Endpoint Cadence |
| --- | --- | --- |
| `existing` | H | H |
| `native-horizon-half-v1` | ceil(H/2) | H |
| `native-horizon-quarter-v1` | ceil(H/4) | H |
| Native **batch** backend, `existing` policy | 1 | H |

The horizon is at least one. A frame ending at jH publishes at jH+H_c-1.
Calls after publication spend zero analysis credits but continue capturing
inputs until the next endpoint. No frame starts early, and temporal alpha still
uses H. The N+H ring is deliberately retained for every candidate: the oldest
frame sample stays valid throughout its completion horizon. This permits
matched storage comparisons rather than confounding timing with ring sizing.

The policies apply to native analysis backends in both precisions, including
true four-channel native plans. They are rejected for other backends, complete
module profiles and native batch identities. Horizon changes within an existing
instance are unsupported; explicit destruction/reconstruction cancels pending
work and rebuilds caches before a new schedule. Tests change H and H_c at such a
cancellation boundary. No mutable quota denominator is exposed to callers.

Untimed checks cover non-divisible hops, H=1, H>N, ring wraparound, zero-credit
idle calls, stage-crossing quotas, the four supported v3 windows, independent
lanes and numerical agreement with immediate native execution. Reconstruction
after cancellation is checked against the independent all-bin oracle. Synthetic
clock fixtures verify C++/Python delay, stage trace, publication counts and
per-channel endpoint contracts. Existing production tests cover sparse/dense
scheduling and mid-cache reset.

## Explicit Candidate Deferrals

| Candidate | Decision And Reason | Consequence For Claims |
| --- | --- | --- |
| Native sub-FFT leaves | Deferred. A correct contender needs a separate factorization, twiddle/permutation/reconstruction implementation and an explicit copy/scratch contract. Reusing an opaque full-size FFT does not implement bounded leaves. Establish the efficient native/horizon baselines first; no leaf-size results are planned in this package. | Cannot recommend a native leaf size or claim the finest granularity is best. |
| Stage-weight sweep | Deferred. Production output weights 1/2 and dirty-cache weights are existing choices, with earlier M1 Pro tuning disclosed in spec 010. A new sweep must separate arithmetic from scheduling credits and cover real sinks; current unequal-cost units are not a calibrated time model. | Cannot claim optimal weights or interpret one credit as a fixed duration. |
| Ring-index variant | Deferred. The core already uses modulo while native retained-ring segments split at wrap. Changing it together with native kernels would confound attribution; first retain the existing endpoints and whole-pipeline cost comparison. | No isolated modulo-versus-branch speedup claim. |
| Butterfly segmentation | Deferred. Existing sparse/dense kernels are retained in the matched core pair. Another segmentation threshold needs an independently versioned comparison after pilot diagnosis. | No newly tuned dispatch threshold claim. |
| Magnitude/reconstruction variant | Deferred. Native layouts and reconstruction already differ from the core; further changes need a separate numerical/layout identity and near-zero error audit. | No isolated magnitude-kernel attribution. |
| Coordinate-mapping variant | Deferred. Complete module sinks use the production cache/mapping, with independent inverse-coordinate checks. A new mapping implementation could change display behavior and would blur the first host comparison. | No claim that display mapping has been optimized. |

These are deliberate scope decisions allowed by phase 5's readiness gate,
not completed implementations or evidence that the candidates would lose.
Follow-up weights/leaves should predeclare tuning cases N=2048/4096,H=1024 and
held-out cases N=16384,H=240/509/1024, dirty/steady states and scalar/four-channel
sinks before collecting their own comparisons. They are not silently added to
this initial package.

## Credits And Time

The current scheduler bounds credits, not wall time. For dirty N=4,H=16 and
output weight one, W=15 gives a maximum quota of one credit per call. Window
preparation executes a four-credit operation on its first credit; the next
credits are accounting. Thus `c0 + kappa*ceil(W/H)` is not justified by treating
kappa as a per-credit operation cost. Opaque native calls are also indivisible.
A future predictive model must include operation boundaries, configuration and
dispatch, explicit primitive-cost assumptions and held-out validation. Observed
p99 is not WCET, including when no deadline miss is observed.

Validation from the repository root:

```shell
python3 -m unittest discover -s docs/whitepaper/benchmarks/tests -p test_native.py
make test/dsp/test_spectrum_analysis
```

The first command uses untimed numerical checks and synthetic observation
clocks. The tests do not collect a scheduling performance comparison.

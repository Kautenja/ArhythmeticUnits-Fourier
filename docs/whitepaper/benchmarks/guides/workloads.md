# Explicit Workloads And Scheduling Metrics

Version-3 workload controls separate windowing, octave smoothing, temporal
smoothing, input fixtures, and active module ports. They extend the existing
runner; they do not change historical v1/v2 workload meanings or paper assets.
The new metrics describe aggregate callback work and its placement within hops.
They do not measure a Rack audio device or screen refresh.

## Prepare A Small Profile

From the repository root, with Python 3 installed, resolve the supplied
[controls profile](../profiles/controls.json) without building or measuring:

```shell
python3 docs/whitepaper/benchmarks/bench.py plan --profile controls --variant rack --output .build/controls-plan.json
python3 docs/whitepaper/benchmarks/bench.py plan --profile controls --variant macos --output .build/controls-macos-plan.json
```

Schema-3 profiles may omit `preset`, which selects only their explicit
`workloads` list. No preset or transition sweep is appended. The example
resolves five workloads for `rack` and six for `macos`, with the vDSP omission
recorded in the Rack plan. A plan declares capabilities; it does not install
dependencies or prove the local binary contains them. Schema-2 profiles keep
their existing preset behavior.

Every profile still declares `seed`, `repeats`, `hops`, `frames`, and
`warm_hops`. The example's two short process repetitions are wiring/exploration
settings, not adequate confirmation. Freezing remains a separate, pilot-backed
step with at least three operator-prepared confirmation sessions. Development
repetitions share one process; publication repetitions launch fresh processes.
Neither is an independent host session merely because its label differs.

## Workload Contract

An explicit workload sets `workload_schema: 3` and keeps the old `smooth` field
at zero. The profile resolver fills the following defaults before hashing,
planning, freezing, or launching. C++ protocol v3 takes the complete control
object after the version token; native development `--config` likewise expects
all v3 fields in each explicit configuration, alongside the base workload.

| Field | Default | Meaning And Supported Values |
| --- | --- | --- |
| `window` | `hann` | `hann`, `boxcar`, or `blackman-harris`; periodic, with existing coherent-gain normalization |
| `octave` | `0` | Independent frequency smoothing width, 0 through 2.5 octaves |
| `temporal_mode` | `alpha` | `alpha` or `module-seconds` |
| `temporal_value` | `0` | Direct alpha in [0,1), or module-style seconds in [0,10]; alpha must remain below one after binary32 conversion |
| `fixture` | `mixed` | One of the signal fixtures below |
| `fixture_seed` | `305419896` | Unsigned 32-bit deterministic input seed, separate from process-order `seed` |
| `decay_samples` | `4096` | Absolute signal-to-silence cutoff, 1 through 1,000,000,000 samples |
| `active_ports` | `1` | Connected module ports, or all channels for a core analysis bank |
| `execution_regime` | `continuous` | `continuous` or callback-only `paced`; must match campaign execution policy |
| `experimental_policy` | `existing` | Only implemented adapters; future experimental policies fail explicitly |

Requested values and effective binary32 octave/alpha values appear in the
resolved C++/Python contract. Full configurations determine hashes, freeze
membership, report grouping, and metadata; `results.csv` includes each explicit
control. Changing even the fixture seed changes the workload identity.

`module-seconds` preserves the existing panel convention:
`alpha = exp(-10 * (H / sample_rate) / seconds)`, with zero disabling temporal
smoothing. It is not a conventional exponential time constant. Module cases
require this mode, public time settings from 0 through 2.5 seconds, and octave
widths 0, 1/3, 1, or 2. `voices` retains its existing per-connected-port meaning;
voices are summed, not independent analyzers. Fourier permits one to four
connected ports; Spectre has one. Unsupported module N/H settings still fail.

V3 currently covers stationary analysis, module, and driver workloads. Dynamic
transitions, the old correlated SIMD control, inverse jobs, and filtering chains
retain their older protocols. Independent scalar/SIMD banks require
`active_ports: 4`. Ordinary scalar cases require one. Non-module partial channel
banks fail instead of measuring a different task.

## Signals And Numerical Audits

All inputs are prepared binary32 samples with absolute value at most one,
shared across backend precisions. Repeating fixtures retain 65,536 samples.
The noise generator uses a specified 32-bit linear congruential sequence;
source identity also preserves the exact trigonometric/window implementation.
No random generation or string-based fixture selection enters measured sample
work. Independent lanes vary their tones, offsets, and seeded noise.

| Fixture | Signal |
| --- | --- |
| `mixed` | Two tones plus seeded noise |
| `silence` | Exact zero |
| `decay` | Mixed signal until `decay_samples`, then permanent silence |
| `impulse` | Unit sample at absolute sample zero, then permanent silence |
| `dc`, `nyquist` | Constant 0.5 or alternating +/-0.5 |
| `off-bin` | Amplitude 0.5 sinusoid at bin 7.25 |
| `weak` | The same sinusoid at amplitude 1e-9 |
| `noise` | Seeded values in [-0.5,0.5) |
| `independent` | Distinct inputs for four-channel banks or active Fourier ports |

Impulse and decay include warmup in their absolute sample origin. Use startup
and zero warmup to observe the initial response; use an explicit cutoff beyond
warmup for a measured signal-to-silence transition. They never restart when the
retained input sequence wraps. Weak input is within the normal floating-point
range; it is not a claim of subnormal preservation. Decaying states can reach
subnormal values, whose treatment depends on the recorded FPU policy.

Independent all-bin replay retains the existing `spectrum-norms-v1` budgets:
3e-4 for float and 1e-10 for double, with exact zero-reference requirements and
legacy pointwise violations retained. Normwise acceptance does not establish
accuracy of every arbitrarily weak bin. Tests cover 108 window/signal/smoothing
combinations and matched independent-channel inputs. Headless module checks
validate controls and output coordinates/history; the full module numerical
oracle remains Phase 4 work. Fourier's logarithmic silence coordinate is
negative infinity; NaN, positive infinity, and non-finite x coordinates fail.

## Scheduling Outputs

Reports containing v3 workloads add six checked tables:

-   `scheduling-processes.csv`: individual aggregate process observations,
    counts, compute mean/median/p99/max, peak/mean, sample count, summed compute,
    simulated duration, and elapsed measurement span when recorded.
-   `scheduling-analyzers.csv`: complete-hop peak distributions and callback
    phase imbalance per analyzer and process, with partial/shared counts.
-   `scheduling-hops.csv`: each endpoint, interval, original callback indices,
    maximum whole-callback duration, shared membership, and partial-edge flag.
-   `scheduling-phases.csv`: distributions grouped by callback start phase
    modulo H; these are not internal FFT-stage timings.
-   `scheduling-budgets.csv`: actual compute exceedance counts for 25%, 50%, and
    100% of D/sample_rate. These are allocation scenarios, not device deadlines.
-   `scheduling-sessions.csv`: cost means, medians, and ranges across process
    points within each session. No callback pooling or confidence interval.

Hop intervals are [endpoint, endpoint+H), using replay endpoints and the
declared stationary lattice for unobserved edges. Offset/staggered instances,
non-divisible H/D, D>H, and partial edges retain their coordinates. A callback
intersecting multiple hops contributes its full duration to each, with shared
membership recorded. No sub-callback cost is invented. Analyzer-indexed groups
still contain aggregate block work and cannot attribute compute to that analyzer.

`evidence.json` also retains paced wake lateness, conditioning, release-to-finish
distributions, actual synthetic-release deadline exceedances, and ordered
throughput chunks. Publication ages come from untimed replay. Consumer/device
metrics and per-analyzer attributed compute are explicitly unavailable.
Historical throughput records have no invented chunk or hop observations.

Report selection/export re-derives these tables from checked raw records;
rehashing an edited table cannot make it valid. Bundle extraction repeats the
same checks. Retained historical raw observations under
[`tests/fixtures/scheduling`](../tests/fixtures/scheduling/README.md) guard the
old headline summaries, peaks, phase imbalance, slow process, and inverse
exceedance counts. They are regression inputs, not new measurements.

Spec 014 remains preparation-only. The user will launch measurements offline
under the required `caffeinate`/power guard after the later packaging phases.
No performance campaign is required to validate these controls and reports.

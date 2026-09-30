# Extend A Study Without Changing Its Baseline

Workloads are JSON objects with explicit units and independent variables.
The schema-2 profile chooses a maintained factor preset and optional extra
workloads. The resolver overlays each workload on `run.BASE`: N=2048,
H=1024 samples, B=64 samples, one instance, aligned, 48 kHz, steady state,
no load/smoothing/cache pressure, callback offset zero. `pass_name` chooses
callback, throughput, or a supported transform pass. Transition cases use
`interactive-v1`; they require a single startup scalar instance and zero
warmup. [transitions.md](transitions.md) defines the entire event contract.

## Add A Workload

Copy `profiles/smoke.json` to `.build/my-study.json`, retaining its schema and
counts, then add this member to the JSON object:

```json
"workloads": [
  {"backend":"core-float","n":256,"hop":53,"block":32,"smooth":1}
]
```

This fragment is a member, not a complete JSON document. Resolve before running:

```shell
python3 docs/whitepaper/benchmarks/bench.py plan --profile .build/my-study.json --variant rack --output .build/my-study-plan.json
python3 docs/whitepaper/benchmarks/bench.py run --profile .build/my-study.json --variant rack --output .build/my-study-smoke
```

Inspect every resolved row and provider omission in the plan. Invalid sizes,
hops, precision, instance/voice counts, unavailable variants, duplicate rows
and unsupported transitions fail. For a future real study, extend a pilot
profile and acquire pilot evidence before freezing. A smoke file remains smoke
regardless of directory or session name. Custom profiles are retained as resolved
launch input; campaigns archive the shared resolver and source identities.

## Add A Backend

1.  Implement its canonical adapter in `benchmark/paper`, preserving GPL and
    upstream attribution. Follow a neighboring provider's transform sign,
    packing, normalization, output-store, planning and memory contracts.
2.  Add explicit capabilities to `lib/backends.json`. Update C++ dispatch and
    the provider build option in `mk/rack.mk`; the generated registry must
    match Python exactly. Unsupported cases stay explicit, not silent fallbacks.
3.  Add independent all-output numerical fixtures in `test/paper` and register
    them in `benchmarks/tests`. Check silence, impulse, DC/Nyquist, off-bin
    tones, noise, weak signals and non-finite/truncated/misordered outputs.
    Existing spectrum-norms-v1 thresholds are 3e-4 float and 1e-10 double;
    exact silence must remain exact. Do not tune thresholds to fit a backend.
4.  Add matched smoke workloads, declared plan/storage policies, setup and
    separate allocation probes. Keep the native opaque FFT charged as one
    indivisible operation. Reference code runs only outside timed/resource
    execution. Configuration transitions need their own engine/lifecycle model.
5.  Run Python discovery, `setup --build`, the focused smoke and report checks.
    Compare exact matched contracts; preserve different time origins and
    precision/channel layouts as separate strata.

CSV callback durations are nanoseconds; ages are input samples and converted
milliseconds. Throughput reports ns per engine sample, transforms ns per
transform. C++ heap bytes exclude unknown native/stack storage. Tables carry
explicit unavailable fields, observation counts and process/session ranges.
Relative L2/Linf are dimensionless; absolute errors are unnormalized FFT
magnitude for analysis. No observed maximum is a WCET bound, and simulated
budget exceedances are not device underruns.

## Compare A Future Revision

Keep the original freeze, campaigns, reports and selection unchanged. Create a
new profile/pilot/freeze under another output directory. Source, dependency,
compiler, provider and policy differences create separate evidence strata;
confirmation export rejects mixed freezes. Review matching workload contracts
side by side in their checked CSVs, including accuracy, resources and response
latency, before making a new claim. A faster transform does not imply lower
callback tails or newer spectra. Use the separately guarded retirement workflow
only after the old evidence has no retained use.

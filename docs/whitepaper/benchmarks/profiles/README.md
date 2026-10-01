# Study Profiles

These are the active study configurations. Each short schema-2 JSON file
names a versioned factor preset, enables the separately reported transition
suite, and declares seed, repetitions, measured hops/frames, and warmup.

| Profile | Purpose |
| --- | --- |
| `smoke` | One short process per case; numerical, transition and integration checks |
| `pilot` | N=2048/4096/16384, H=1024, B=16/64/256; float analysis, inverse/filtering and transforms; transition controls |
| `extensions` | Small/odd hops, rate, multiplicity, phase, startup/live/cache/load, independent channels, double precision and transitions |
| `controls` | Schema-3 example: five Rack cases plus optional vDSP, with explicit independent controls; no appended preset |

[lib/campaigns.py](../lib/campaigns.py) defines the named factor sweeps in one
place. [lib/profiles.py](../lib/profiles.py) resolves options, capabilities,
transition horizons and the bounded smoke matrix. `plan --output plan.json`
prints counts and writes every resolved configuration/contract. The expanded
plan is generated evidence, not another hand-maintained configuration file.

```shell
python3 docs/whitepaper/benchmarks/bench.py plan --profile pilot --variant macos --output .build/pilot-plan.json
```

Command-line `--repeats`, `--hops`, `--frames`, `--warm-hops` and `--seed`
override a profile for pilot exploration. A confirmation freeze stores those
options plus the exact expanded matrix and pilot identities; `run --freeze`
uses it instead of reinterpreting a changed profile. Version-2 profiles are
not confirmation evidence. Old expanded JSON configurations and the former
confirmation plan remain in [history/configs/](../history/configs/); none is
silently promoted to the new study.

For a small extension, copy one profile outside this directory and add a
`workloads` list of partial configurations. Each item overlays the documented
base workload. Unsupported settings and duplicates fail during planning.
Do not mutate a frozen profile to compare a new revision. See the
[worked example](../guides/extending.md).

For a small standalone selection, use schema 3 with no `preset` and a nonempty
`workloads` list. It appends neither a preset nor transition cases. Set
`workload_schema: 3` on rows using the new controls; the profile's schema and
the workload's protocol version are distinct. See
[explicit workloads and scheduling metrics](../guides/workloads.md) for field
defaults, fixture origins, validation limits, and planning-only commands.

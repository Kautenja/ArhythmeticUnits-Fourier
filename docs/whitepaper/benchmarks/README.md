# Benchmark Workflow

For the prepared M1 Pro pilots, use the [offline study guide](guides/offline-study.md).
It provides preparation/check commands and one command per user-run session.
No performance collection is part of preparation.

Start here to reproduce or extend the whitepaper's measurements. All commands
run from the repository root. `bench.py` is the public entry point; it uses the
existing [C++ measurement suite](../../../benchmark/paper/README.md).

```shell
python3 docs/whitepaper/benchmarks/bench.py --help
python3 docs/whitepaper/benchmarks/bench.py setup --variant rack
python3 docs/whitepaper/benchmarks/bench.py inventory --variant rack
python3 docs/whitepaper/benchmarks/bench.py plan --profile smoke --variant rack
python3 docs/whitepaper/benchmarks/bench.py run --profile smoke --variant rack --output .build/study-smoke
python3 docs/whitepaper/benchmarks/bench.py report .build/study-smoke --phase smoke --output .build/study-smoke-report --no-plots
```

The smoke profile is deliberately small: 52 Rack-only processes or 80 macOS
processes, including numerical coverage and parameter transitions. It checks
implementation, not performance claims. The pilot and extension profiles are
larger and should run on a prepared, otherwise idle host. Nothing in the paper
build launches a benchmark or replaces its results.

## Where Things Live

| Location | Purpose |
| --- | --- |
| [bench.py](bench.py) | Setup, plan, run, freeze, status, check, report, export and bundle commands |
| [guides/baselines.md](guides/baselines.md) | Native analysis, matched scheduling pairs, and stage diagnostics |
| [profiles/](profiles/README.md) | Maintained study profiles and a small explicit-controls example |
| [guides/workflow.md](guides/workflow.md) | Copyable launch, review, freeze and publication handoff |
| [guides/extending.md](guides/extending.md) | Add a workload/backend and compare a future revision |
| [guides/workloads.md](guides/workloads.md) | Explicit v3 controls, input fixtures, and checked scheduling metrics |
| [guides/](guides/) | Detailed measurement contracts and provider notes |
| [lib/](lib/) | Private runner, validation, derivation and workflow implementation |
| [tests/](tests/) | Python tests and small C++ verifier drivers |
| [Archived inputs](../../latex/deprecated/whitepaper/benchmarks/history/README.md) | Older configurations and prototype reproduction tooling |
| [Current study evidence](../data/study-014/README.md) | Compact study 014 measurements and provenance |
| [Comparison results](../../latex/deprecated/whitepaper/data/comparison-012/README.md) | Spec 012 replacement measurements and analysis/writing handoff |
| [../tools/](../tools/) | Manuscript source expansion and consistency checks, not benchmarks |

New runs, resolved plans, freezes, reports, selections and bundles belong in
an explicit output root, normally `.build/`. Keep that directory when cleaning
up: the repository's broad `make clean` removes `.build`. Spec 012's verified
local bundles are additionally preserved outside that root, as documented in
the results handoff. Use new names for
reruns; completed and partial evidence are never overwritten or resumed in
place. See [failure and restart](guides/workflow.md#observe-and-restart).

## Choose A Provider Variant

| Variant | Providers | Requirements |
| --- | --- | --- |
| `rack` | Fourier and Rack/PFFFT | Rack 2 SDK, C++ compiler, Make, Python 3 |
| `portable` | Above plus serial float/double FFTW | Pinned FFTW dependency; still requires Rack SDK |
| `macos` | Above plus Apple Accelerate/vDSP | macOS and its system Accelerate framework |

macOS ARM64 is the tested implementation host. Linux, Windows, and x86-64
variants have explicit capability contracts but still need their own execution
validation before platform claims. The portable variant does not mean a
standalone SDK-free benchmark. Provider licenses and storage limits are in
[fftw.md](guides/fftw.md), [pffft.md](guides/pffft.md), and
[vdsp.md](guides/vdsp.md).

For plots, activate a Python virtual environment and install the pinned
[report requirements](report-requirements.txt). Tables and integrity checks use
the standard library. LaTeX is needed only for document compilation. The full
prerequisites, output schema, and launch sequence are in the
[workflow guide](guides/workflow.md).

## Validate A Change

```shell
python3 -m unittest discover -s docs/whitepaper/benchmarks/tests -p 'test_*.py'
make -C docs/whitepaper check
```

Tests use bounded synthetic fixtures and short C++ verifiers, never the full
paper campaign. The scalar/PFFFT checks need a Rack SDK; optional-provider and
plot tests report their skips. A fresh `run --profile smoke` also builds and
verifies the actual native executables, resources, numerical replay and reports.

Old `benchmarks/*.py` commands now live in `benchmarks/lib/`; old JSON files
live in `docs/latex/deprecated/whitepaper/benchmarks/history/configs/`.
Historical source archives retain their original paths and remain readable. No forwarding-script layer is needed:
new work uses `bench.py`; exact historical reproduction uses its archived
source. Existing [fast C++ development commands](guides/DEVELOPMENT.md) remain
separate from publication campaigns.

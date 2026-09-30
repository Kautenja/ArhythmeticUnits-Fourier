# Reproduce And Publish A Study

Run these commands from the repository root. The workflow separates measured
campaigns, checked reports, reviewed selections, generated paper assets and
portable audit bundles. It does not publish externally. Replacement measurements
are **NOT RUN** at this handoff; evidence retirement remains **DEFERRED**.

## Prepare The Host

Use Python 3.10 or later, Make, a C++ compiler and a Rack 2 SDK with headers,
`plugin.mk`, PFFFT and the matching Rack library. Follow the repository
[contributor setup](../../../../CONTRIBUTING.md#configure-the-rack-sdk).
Set `RACK_DIR` when the SDK is not the default `../..`. The CLI also accepts
`--rack-dir`, `--cxx` and `--fftw-prefix` explicitly. No author-specific path
is required. The plotting pin is separate from the standard-library checker:

```shell
python3 -m venv .build/paper-report-env
. .build/paper-report-env/bin/activate
python3 -m pip install -r docs/whitepaper/benchmarks/report-requirements.txt
python3 docs/whitepaper/benchmarks/bench.py dependency fftw --jobs 2
python3 docs/whitepaper/benchmarks/bench.py setup --variant macos --build
```

Skip FFTW installation and use `--variant rack` for the base suite. Use
`portable` for FFTW without vDSP. The FFTW builder verifies the pinned 3.3.10
archive hash and records independent serial float/double builds and exact
provenance; [its guide](fftw.md) documents flags and licensing. Fourier's GPL
license remains in force. Rack SDK binaries and Apple's system framework are
not redistributed by the evidence bundle. PFFFT source/plan identity and its
license are recorded separately. An unavailable requested provider is an error.

Use an otherwise idle host for measured pilot/confirmation sessions. Record
actual power mode, thermal conditions, affinity policy and competing activity.
Fresh subprocesses are repeated measurements, not independent host sessions.
The commands below never loop over fictitious session labels.

## Plan And Smoke

```shell
python3 docs/whitepaper/benchmarks/bench.py plan --profile smoke --variant macos --output .build/smoke-plan.json
python3 docs/whitepaper/benchmarks/bench.py run --profile smoke --variant macos --output .build/study-smoke
python3 docs/whitepaper/benchmarks/bench.py check .build/study-smoke
python3 docs/whitepaper/benchmarks/bench.py report .build/study-smoke --phase smoke --output .build/smoke-report
```

Success markers are `Verified ... runs` and `Wrote ... checked workloads`.
Build and preflight precede serial measurement; setup, reference replay,
progress, formatting and artifact hashing remain outside measured intervals.
The default smoke has 52 Rack / 66 portable / 80 macOS processes. No driver,
module, inverse or transform family is silently substituted for another.

A campaign holds `metadata.json`, source/dependency archives, exact executable
identities, inventory, build/verification logs, resource probes and per-run
CSV/numerical/runtime/transition files. Its metadata identifies all checksummed
artifacts. The launch input is retained beside it as `<output>.input.json`.
A report holds units-bearing CSV/Markdown, SVG/PNG figures, `evidence.json`
and checksums in `manifest.json`. Reports have relative campaign paths and
can be regenerated after moving the whole study directory.

## Run Pilots And Review

Replace the example host/session notes with truthful descriptions. Pilot scale
is configurable; it is deliberately larger than smoke and may take time:

```shell
python3 docs/whitepaper/benchmarks/bench.py plan --profile pilot --variant macos --output .build/pilot-plan.json
python3 docs/whitepaper/benchmarks/bench.py run --profile pilot --variant macos --output .build/pilot-primary --host m1-pro --session pilot-01 --notes 'Actual power, thermal, affinity and host activity notes'
python3 docs/whitepaper/benchmarks/bench.py run --profile extensions --variant macos --output .build/pilot-extensions --host m1-pro --session pilot-extensions-01 --notes 'Actual session notes'
python3 docs/whitepaper/benchmarks/bench.py estimate .build/pilot-primary .build/pilot-extensions
python3 docs/whitepaper/benchmarks/bench.py report .build/pilot-primary --phase pilot --output .build/pilot-report
```

`estimate` reports measured process/build wall time, raw bytes/process and
observation counts. Scale only matching workloads and duration; reference replay,
planning and tails may scale nonlinearly. Budget extra disk for raw data,
archives, reports and bundles. The numbers are planning diagnostics, not speedups.

Review process/session variation, timer controls, observation duration/counts,
rare callback tails, numerical coverage and resource limitations. Select final
lengths/repetitions from that evidence. Keep the complete declared matrix,
including batch wins. Any reduction needs a recorded rationale; a confirmation
workload absent from the pilots is rejected. A second actual pilot session may
be needed to estimate session variation. The extension profile is a separate
study/freeze if it needs different observation counts.

## Freeze And Confirm

These commands use the profile's default counts as an executable starting
point. Override counts only after inspecting the pilots and record why in
`--rationale`; defaults are not asserted to be sufficient for stable tails.

```shell
python3 docs/whitepaper/benchmarks/bench.py freeze .build/pilot-primary --profile pilot --variant macos --output .build/primary-freeze.json --rationale 'Record observed pilot variation, timer resolution, tail counts, inclusion decisions and chosen duration/repetitions here'
python3 docs/whitepaper/benchmarks/bench.py run --freeze .build/primary-freeze.json --variant macos --output .build/confirm-01 --host m1-pro --session confirm-01 --notes 'Actual first independent session conditions'
```

Later, in separately prepared host sessions, run the same freeze with new
output names and truthful `confirm-02` and `confirm-03` labels. Do not launch
three runs together and call them independent sessions. At least three are
required for confirmation reporting; pilot evidence may justify more. The
freeze records exact source/dependency/compiler identities, numerical policies,
workloads, seed, counts and rationale. Changed code or dependencies fail before
measurement. A different host/compiler/revision needs its own pilot/freeze and
report stratum; do not pool it into this study. No cross-architecture claim
follows from the tested macOS ARM64 implementation.

## Observe And Restart

From a second terminal, while the serial run is active:

```shell
python3 docs/whitepaper/benchmarks/bench.py status .build/confirm-01 --logs --follow
python3 docs/whitepaper/benchmarks/bench.py runtime .build/confirm-01
```

Progress shows current workload/repetition, completed/failed counts and log
tails. Runtime diagnostics become complete when the runner finishes; partial
metadata and logs remain useful before that. `Ctrl-C` returns 130 and keeps
the partial directory; validation/runtime failures return nonzero and retain
failure identity. Success returns zero. An external forced kill may leave
`incomplete` metadata; it is never accepted as complete evidence.

There is deliberately no in-place resume. Investigate the retained stderr or
build log, correct the cause, and restart the same command under a new output
name. Keep the failed directory as an attempt record. If sources or dependencies
changed, create a new pilot/freeze instead of editing the old freeze. Never edit
metadata to relabel partial work as complete.

## Report, Select And Export

```shell
python3 docs/whitepaper/benchmarks/bench.py check .build/confirm-01 .build/confirm-02 .build/confirm-03
python3 docs/whitepaper/benchmarks/bench.py report .build/confirm-01 .build/confirm-02 .build/confirm-03 --phase confirmation --output .build/confirmation-report
python3 docs/whitepaper/benchmarks/bench.py select .build/confirmation-report --output .build/paper-selection.json
python3 docs/whitepaper/benchmarks/bench.py export .build/paper-selection.json --output docs/whitepaper/generated/comparison
python3 docs/whitepaper/benchmarks/bench.py check-export docs/whitepaper/generated/comparison
make -C docs/whitepaper check
make -C docs/whitepaper
```

`select` creates a schema-1 selection of checked rows and figures. Review it
before export: it names exact config/stratum hashes, statistic, units, macro,
include names, figure workload membership, source hashes and report identity.
The initial selection includes all stationary mean costs and report figures;
remove irrelevant figures deliberately, retaining their full evidence report.
Other supported row statistics are numerical errors, callback p99/max, heap
bytes and spectrum/publication/delivery ages. Unavailable quantities are errors,
not zeros. Transition figures and CSVs retain event identities and errors.

Export produces `numbers.tex`, `results.tex`, `figures.tex`, selected PNGs and a
receipt mapping every value to its source. Add the desired literal
`\input{generated/comparison/results.tex}` etc. during FR-14 editorial integration.
Export never rewrites prose or automatically inserts provisional numbers into
the manuscript. The table retains a workload hash, statistic, units and session counts;
the receipt/report retain process/session ranges and exact raw identities.
Use the figure captions and limitations when writing claims. Paper `check`
validates any included generated receipt for freshness; ordinary builds neither
measure nor fetch evidence. Generated output cannot overwrite an existing export.

For a **fixture document only**, use smoke `select --fixture` and
`export --fixture --output .build/fixture-paper`. Compile its `fixture.tex`
with `pdflatex -no-shell-escape` inside that directory. The optional production
`preview.tex` is compiled from the manuscript root so its include paths match
the paper. Large selections paginate their figures in bounded groups. Fixture export is
forbidden anywhere in the manuscript tree. Smoke/pilot, incomplete, mixed,
stale or unfrozen evidence cannot populate production paper destinations.

## Package And Regenerate Elsewhere

```shell
python3 docs/whitepaper/benchmarks/bench.py bundle .build/confirm-01 .build/confirm-02 .build/confirm-03 --selection .build/paper-selection.json --output .build/comparison-evidence.tar.gz
python3 docs/whitepaper/benchmarks/bench.py unpack .build/comparison-evidence.tar.gz --output .build/relocated-evidence
```

The bundle includes raw campaigns, logs, first-party source archives, policies,
exact dependency identities, tooling, selection/report when supplied, licenses,
commands and a checksummed inventory. SDK/native binaries and dependency
archives are omitted explicitly; their recorded hashes remain. Validation of
an audit bundle cannot claim the omitted bytes were inspected. System frameworks
must be obtained on the target platform. Run the bundled README commands from
the extracted directory to regenerate tables/figures without any original
absolute path. Source archives support a fresh timing rerun with separately
installed dependencies; timings are hardware-dependent, unlike deterministic
statistical derivation. Do not expect byte-identical plots across different
plot/font-library versions; the plotting environment is recorded.

## Retirement Is Separate

```shell
python3 docs/whitepaper/benchmarks/bench.py retire-plan .build/obsolete-comparison --output .build/retirement-plan.json
python3 docs/whitepaper/benchmarks/bench.py retire .build/retirement-plan.json --replacement .build/comparison-evidence.tar.gz --publication docs/whitepaper/generated/comparison --receipt .build/retirement-receipt.json
```

The first command inventories exact identities and tracked references. The
second previews eligibility only; `--execute` is a separate explicit operation.
It requires a validated independent replacement bundle, frozen confirmation
sessions and a fresh non-fixture publication receipt. Changed candidates and
unresolved manuscript/spec/test references block removal. Only explicit old
pilot/confirmation directories beneath `.build` are eligible. Historical
manuscript data, optimization evidence, dependencies, source inputs and arbitrary
reports are protected. Retained report/spec references must be resolved during
FR-14; record removed/retained identities in spec 004 using the receipt. Nothing
is retired by setup, run, report, export or this implementation handoff.

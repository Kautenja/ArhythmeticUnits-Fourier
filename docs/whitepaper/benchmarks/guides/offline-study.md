# Offline Spec 014 Pilot Collection

This package prepares two independent pilot sessions on the local M1 Pro. It
collects stronger baselines, scheduling horizons, complete modules and actual
Rack-engine workloads. It produces raw evidence for later analysis; it does not
select winners, freeze confirmations, create plots or change the manuscript.

## Prepare While Development Tools Are Available

From `/Users/christiankauten/Documents/Projects/Rack/plugins/Fourier`:

```shell
make benchmark-study-prepare
make benchmark-study-check
```

Preparation requires the locally pinned Rack source/library and serial float
and double FFTW libraries. Defaults are `../..` and `.build/deps/fftw`; override
`STUDY_RACK_DIR`, `STUDY_FFTW_PREFIX` and `STUDY_CXX` if necessary. Missing inputs
fail with their paths. The existing dependency workflow can prepare them while
online; neither preparation nor launch silently downloads anything. The prepared
package is `.build/study-014/prepared`. Preparation refuses an existing directory;
use a fresh `STUDY_PACKAGE` path for a new preparation and supply that same setting
to check/run. Keep prior packages and attempted results for comparison.

Preparation builds both executables, runs correctness preflight, checks all
contracts and untimed allocation probes, and replays every engine profile with
independent numerical references. Resource times are explicitly null under
`untimed-v1`; no clock calibration or performance process is launched. Opaque
provider planning remains part of setup. A sealed manifest retains compiler
commands, source/dependency bytes and archives, exact profiles, software/host
identity and executable hashes. Source membership and artifact mutations fail
before a user-run session. Changes require fresh preparation. `make clean`
removes the prepared package but never the default result directories.

## Launch Each Session Separately

Connect AC, disable Low Power Mode and keep the lid open. Disconnect Wi-Fi,
Ethernet and other network links. Turn Bluetooth off. Stop all agents and close
unnecessary applications, including Codex. Use the built-in keyboard/trackpad or
wired controls. In a standalone terminal, launch one session and leave the laptop
alone:

```shell
cd /Users/christiankauten/Documents/Projects/Rack/plugins/Fourier
make benchmark-study-run SESSION=pilot-01
```

The launcher prints the checklist and asks for `READY`, recording your declaration
separately from observable power/process state. Prepare the host independently
for the second session and run:

```shell
make benchmark-study-run SESSION=pilot-02
```

Do not run both simultaneously. A host-wide file lock rejects concurrent study
launches. Each command checks the immutable local package, automatically starts
`/usr/bin/caffeinate -is`, stages all groups, checks AC/Low Power Mode/assertions,
then waits 180 seconds before dispatch. Each fresh child additionally settles
for one second after its own setup. There are no build prerequisites, dependency
fetches or network calls in launch. The launcher does not disable radios, kill
services or change power settings. Caffeinate prevents the specified sleep
states; it does not pin CPU frequency or eliminate thermal throttling/OS work.

## What To Expect And Return

Each session contains 194 cells and 388 fresh processes, in fixed groups with
predeclared within-group order seeds. All results are retained, including slow
observations. The plan contains 959.147 seconds of nominal paced audio work,
388 seconds of process settling and 180 seconds of session settling. These are
planning quantities, **not a measured runtime estimate**; they omit provider
planning, warmup, numerical replay, I/O, continuous workloads, overruns and
teardown. Tail-sample limitations and quantile ranks accompany every process.

Default outputs are outside the repository:

```text
~/Fourier-benchmarks/spec014/pilot-01/
~/Fourier-benchmarks/spec014/pilot-01.handback.tar.gz
~/Fourier-benchmarks/spec014/pilot-01.handback.tar.gz.sha256
```

The second session uses `pilot-02` in those names. The launcher prints absolute
paths and local progress outside measured intervals. `session.json` records
status and the active/completed groups. A `COMPLETE`, `FAILED` or `INTERRUPTED`
marker distinguishes the outcome. Press Ctrl-C once to stop; allow cleanup and
packaging to finish. SIGTERM also requests cleanup. A forced kill or power loss
cannot guarantee packaging; retain the entire directory in that case. Existing
sessions are never overwritten, resumed or retried automatically. A failed
attempt needs review and a newly declared session/preparation before recollection.

Return each handback archive and SHA256 file after reopening agents. Keep the
full local directories too. Archives include raw rows, execution sidecars,
publication/numerical audits, failed-attempt logs, source archives, manifest,
checksums and README. SDK/provider archives and executables are omitted, with
checksums and explicit per-group omission records retained. Synthetic fixtures
are confined to tests and cannot qualify as measured pilot/confirmation evidence.

The long decay stress has an explicit flagged FTZ tail; see
[the numerical contract](engine.md#tiny-magnitudes-and-silence-decay). Those
records constrain accuracy claims and are never silently discarded. Native
leaves, new stage weights and separate kernel experiments remain reasoned
[deferrals](scheduling-experiments.md). Confirmation runs and paper integration
require a later review of the returned real pilots.

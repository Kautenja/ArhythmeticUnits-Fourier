# Fourier Agent Instructions

Fourier is the Arhythmetic Units plugin for VCV Rack 2. It contains the
Fourier spectrum analyzer and Spectre spectrogram visualizer, supported by
reusable C++ DSP headers. Preserve accurate analysis, responsive displays,
and compatibility with users' saved patches.

## Start Here

Read this file and the relevant developer-manual articles before editing:

-   [Architecture](docs/developer-manual/architecture.md): source map,
    processing flow, Rack boundaries, and compatibility.
-   [Development And Testing](docs/developer-manual/development-and-testing.md):
    dependencies, build commands, tests, benchmarks, and manual checks.
-   [C++ Style Guide](docs/developer-manual/style-guide-cpp.md): required for
    C++ source, headers, tests, and benchmarks.
-   [Markdown Style Guide](docs/developer-manual/style-guide-markdown.md):
    required for documentation changes.

These instructions and the manual are self-contained. Their starting point
was the `free-j` project's agent guidance and C++ guide, which was itself
inspired by Fourier. That project is not a build dependency or a source of
Fourier product requirements.

## Working In This Repository

-   Inspect `git status --short` and relevant diffs before editing. Preserve
    existing user work, including changes in files needed for the task.
-   Implement one coherent requested change at a time. Use the current chat
    for ordinary work; do not introduce agent-loop wrappers or infrastructure
    merely to complete a feature.
-   Read nearby production code and tests before choosing an implementation.
    Keep changes focused; avoid unrelated formatting or dependency upgrades.
-   Keep reusable DSP independent of Rack and UI types. Put host integration
    in module code or `src/rack_extensions/`.
-   Do not renumber existing Rack parameter, port, or light IDs, rename
    module slugs, or change saved JSON meanings without an intentional
    compatibility plan and verification with existing patches.
-   Keep source, `plugin.json`, resources, presets, and user manuals aligned
    when behavior or module interfaces change.
-   Preserve file-level attribution and the repository's existing licenses.
    Do not copy the reference project's MIT header over Fourier's GPL source.
    See [LICENSE.md](LICENSE.md) for source and visual-asset terms.
-   Do not edit vendored dependencies or generated build products as a way
    to fix first-party code. Update dependency revisions only within scope.
-   Commit only when requested or explicitly included in the task. Push,
    publish, and release only when requested. Keep credentials out of tracked
    files.

## Correctness And Real-Time Behavior

Both modules run analysis on Rack's audio engine thread even though they
have no audio outputs. Their processing cost can affect the entire patch.
Keep per-sample work bounded and avoid new allocation, blocking, I/O,
logging, or drawing in that path. Document any existing limitation touched
by a change rather than claiming the current implementation is fully
real-time safe.

Preserve FFT scheduling, window normalization, frequency and amplitude
units, channel independence, and reset/sample-rate behavior. Review the
engine-to-display data handoff whenever changing shared buffers; a flag
alone is not a synchronization contract.

Behavior changes need executable evidence at the appropriate seam. Prefer
deterministic Catch2 regression tests for DSP, then Rack build and manual
checks for integration or UI behavior. Reproduce bugs before fixing them
when practical. Do not weaken assertions or hide failures to make a task
appear complete. Documentation-only edits need link, path, command, and
diff checks rather than a mandatory full C++ build.

Performance claims require comparable before/after workloads, compiler
settings, repeated measurements, and a practically meaningful effect.
Record uncertainty; a passing test or empty benchmark is not evidence of
a speedup.

## Planning And Completion

Small fixes can be planned in the chat. For substantial work that needs a
durable specification, use `specs/NNN-feature-name.md` and include the goal,
behavior examples, requirements, non-goals, testable acceptance criteria,
and exact validation commands. Create specs when useful or requested, not
as a prerequisite for every edit.

Keep completion evidence in the owning spec when one exists: date,
decisions, commands and results, manual checks, and limitations. Mark a
verified spec `Status: COMPLETE` and move it to `specs/archive/`, updating
links. Mark intentionally dropped work `Status: ABANDONED` before archiving.
Do not create duplicate completion diaries or attempt counters.

Finish with a concise summary of what changed, validation actually run,
and any unresolved failures or skipped checks. Distinguish successful DSP
tests from a successful Rack build and from a manual Rack session.

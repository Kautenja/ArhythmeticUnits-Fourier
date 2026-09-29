# Fourier Agent Instructions

Fourier is the Arhythmetic Units plugin for VCV Rack 2. It contains the
Fourier spectrum analyzer and Spectre spectrogram visualizer, supported by
reusable C++ DSP headers. Preserve accurate analysis, responsive displays,
and compatibility with users' saved patches.

## Start Here

Read this file and the relevant contributor and style guidance before editing:

-   [Architecture](CONTRIBUTING.md#architecture): source map,
    processing flow, Rack boundaries, and compatibility.
-   [Development And Testing](CONTRIBUTING.md#development-and-testing):
    dependencies, build commands, tests, benchmarks, and manual checks.
-   [C++ Style Guide](docs/style-guides/cpp.md): required for
    C++ source, headers, tests, and benchmarks.
-   [Markdown Style Guide](docs/style-guides/markdown.md):
    required for documentation changes.
-   [Spectre Manual Figures](CONTRIBUTING.md#spectre-manual-figures):
    required when changing manual figures or refreshing the module screenshot.

These instructions and the contributor guide are self-contained. Their
starting point was the `free-j` project's agent guidance and C++ guide,
which was itself inspired by Fourier. That project is not a build dependency
or a source of Fourier product requirements.

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

## Manual Figures

Follow [Spectre Manual Figures](CONTRIBUTING.md#spectre-manual-figures) for
the source map, prerequisites, refresh commands, and visual checks. Refresh
Spectre's single cover screenshot when its visible panel or controls change,
using `make -C docs/manual-spectre screenshot` from the repository root.
Keep explanatory figures as abstract TikZ diagrams; edit their `.tex`
sources instead of adding more app screenshots or recreating the UI.

Review the generated PNG and rebuilt manual before including an update.
The reviewed screenshot is a tracked source asset; intermediate captures and
compiled manuals belong in ignored build directories. If native rendering
is unavailable, preserve the existing screenshot and report that refresh as
unverified. Do not replace it with a mockup or stale capture. Fourier has not
yet migrated to this figure workflow; do not expand a Spectre task into that
migration without a request.

## VCV Library Releases

Use these permanent links when preparing a release:

-   [Fourier's VCV Library thread, #826](https://github.com/VCVRack/library/issues/826)
    is the update channel for `ArhythmeticUnits-Fourier`. Reuse this thread;
    do not create a new issue for each version.
-   [Fourier's library listing](https://library.vcvrack.com/ArhythmeticUnits-Fourier)
    shows the distributed plugin. The library's
    [source revision](https://github.com/VCVRack/library/tree/v2/repos/ArhythmeticUnits-Fourier)
    records the commit selected by its maintainers.
-   [GitHub releases](https://github.com/Kautenja/ArhythmeticUnits-Fourier/releases)
    host the manuals used by `plugin.json`.
-   [VCV's update instructions](https://github.com/VCVRack/library#pushing-an-update)
    require a new manifest version and an exact commit hash. Maintainers
    reopen the thread after an update comment and close it when the build
    is updated. Posting a comment does not itself publish a library build.
-   Prior examples: [Fourier 2.1.1](https://github.com/VCVRack/library/issues/826#issuecomment-2719964276),
    [PotatoChips](https://github.com/VCVRack/library/issues/652), and
    [RackNES](https://github.com/VCVRack/library/issues/650). Follow the existing
    short format: version, full commit hash or commit link, and a brief change
    summary when useful.

For an explicitly authorized release, work from the repository root:

1.  Increment `plugin.json`'s version and align `CHANGELOG.md` and the manual
    versions. Preserve plugin and module slugs. Follow the
    [manifest version rules](https://vcvrack.com/manual/Manifest#version).
2.  Run the applicable checks in
    [Development And Testing](CONTRIBUTING.md#development-and-testing),
    including DSP tests, a Rack plugin build, and affected manual Rack checks.
    Record which platforms were actually checked. VCV's
    [plugin toolchain](https://github.com/VCVRack/rack-plugin-toolchain)
    supports cross-platform build validation; standalone DSP CI alone does
    not validate the Rack plugin.
3.  Commit and push the approved release changes and a matching `vX.Y.Z`
    tag. Verify that the tag resolves to the intended commit and that its
    `plugin.json` contains version `X.Y.Z`. Do not move a published tag to
    accommodate a fix; prepare a new version instead.
4.  Publish the GitHub release and wait for the
    [manuals workflow](.github/workflows/manuals.yml) to attach `Fourier.pdf`
    and `Spectre.pdf`. Check both downloads. A regular latest release is
    needed for the manifest's `/releases/latest/download/` manual links.
    GitHub publication and VCV Library submission are separate steps.
5.  Read the latest comments in #826 to avoid duplicate requests. Prepare a
    comment using the tagged commit, not the current branch tip. Replace
    `vX.Y.Z` below with the actual release tag. This block only writes a
    local draft and requires Git, `jq`, and an existing local tag:

    ```shell
    release_tag=vX.Y.Z
    release_commit="$(git rev-parse "refs/tags/$release_tag^{commit}")" &&
    release_version="$(git show "$release_commit:plugin.json" | jq -er '.version')" &&
    test "$release_tag" = "v$release_version" &&
    printf 'Updated to %s\n\nCommit: https://github.com/Kautenja/ArhythmeticUnits-Fourier/commit/%s\nRelease: https://github.com/Kautenja/ArhythmeticUnits-Fourier/releases/tag/%s\n' \
      "$release_version" "$release_commit" "$release_tag" \
      > /tmp/fourier-vcv-library-update.md
    ```

6.  Review the draft and confirm the commit is publicly available. When the
    user has authorized notifying VCV, post it with an authenticated GitHub
    CLI session, or paste it into #826:

    ```shell
    gh issue comment 826 --repo VCVRack/library \
      --body-file /tmp/fourier-vcv-library-update.md
    ```

    Comment on the existing thread even if it is closed. Leave reopening
    and build publication to the VCV maintainers. Report the comment URL
    as a submission receipt; verify the library listing/revision separately
    before claiming the release is available in Rack.

No automatic VCV notification is configured. If requested later, trigger it
after successful stable-release validation and manual uploads, and deduplicate
comments by version and commit. The workflow's built-in
[`GITHUB_TOKEN`](https://docs.github.com/en/actions/concepts/security/github_token)
is restricted to this repository, so it cannot comment in `VCVRack/library`.
A separate credential would be required. GitHub currently documents
[fine-grained token limitations](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens#fine-grained-personal-access-tokens-limitations)
for contributing to public repositories where the user is not a member;
a classic token with `public_repo` scope is one option, subject to the target
organization's policy. Keep any such token in an Actions secret, never in
tracked files or chat. Setting `issues: write` in this repository does not
grant access to VCV's repository.

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

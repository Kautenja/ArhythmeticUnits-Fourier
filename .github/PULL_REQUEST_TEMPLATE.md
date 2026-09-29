# Pull Request

Describe the problem, the resulting behavior, and the evidence for this change.
Use the same review and validation standards for human and agent-assisted work.

<!-- Follow ../CONTRIBUTING.md and the relevant guide in ../docs/style-guides/.
Coding agents must also follow ../AGENTS.md. Describe the final change so a
reviewer can assess it without access to a chat or local session history.
Fill in the relevant sections and remove unused prompts. Mark checks as not
applicable with a reason when appropriate. Only claim checks you ran. -->

## Change

<!-- Explain the problem and what changes for users or contributors. Include
a before/after example when useful. Link an issue with "Fixes #123" only if
this PR resolves it; otherwise use "Related to #123". -->

## Compatibility And Processing

<!-- For module/DSP changes, explain effects on saved patches, parameter/port/
light IDs, slugs, custom JSON, analysis units, channel behavior, and reset or
sample-rate handling. Describe a compatibility plan for intentional changes.
For processing/shared-buffer changes, explain allocation, bounded per-sample
work, and engine/display ownership or synchronization implications. -->

## Validation

<!-- Choose checks using CONTRIBUTING.md's "Choosing Validation" section.
Record exact commands run from the repository root and their results, with
OS/architecture, compiler, and Rack/SDK versions where relevant. Distinguish:
- Standalone DSP tests and relevant deterministic regression coverage.
- Rack plugin build and headless integration tests.
- Manual Rack checks: sample rate, settings, observations, and screenshots
  for visual changes. Include existing-patch loading when persistence changes.
- Documentation-only checks: links, paths, commands, and git diff --check;
  no C++ build is required. Inspect rendered pages for manual PDF changes.
State skipped checks, failures, and limitations with reasons.
Separate observed results from assumptions or proposed checks. If reporting
CI or another contributor's results, link the run or evidence and identify it.
For performance claims, include comparable baseline/candidate workloads,
compiler flags, repeated measurements, and uncertainty. -->

## Checklist

- [ ] I reviewed the diff and followed the relevant contributor/style guidance.
- [ ] I recorded validation results and any skipped or failing checks.
- [ ] I preserved compatibility or documented intentional changes and evidence.
- [ ] I updated affected documentation, manifests, presets, and resources,
      or explained why no updates are needed.

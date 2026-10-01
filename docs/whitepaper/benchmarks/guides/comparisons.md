# External FFT Comparison Plan

The completed implementation record lives in
[Spec 004: External FFT Comparison For The Scheduling Paper](../../../../specs/archive/004-external-fft-comparison.md).
It preserves the contender shortlist, research questions, adapter requirements,
measurement contracts and implementation evidence. Replacement measurements,
portable evidence and retained historical dependencies are recorded in
[spec 012](../../../../specs/archive/012-comparison-evidence-and-paper-integration.md)
and its [results handoff](../../data/comparison-012/README.md).
[Spec 013](../../../../specs/archive/013-comparison-paper.md) records the subsequent
manuscript analysis and publication checks.

Rack/PFFFT, optional FFTW3 and Apple Accelerate/vDSP, and the matched PFFFT
hybrid are implemented as benchmark-only adapters. KISS FFT (FR-7), Garrido
feedforward STFT (FR-8), and windowed sliding/hopping methods (FR-9) are
[explicitly deferred for the current paper](../../../../specs/archive/004-external-fft-comparison.md#optional-contender-decision).
The spec records evidence, claim limits and reopening criteria; these are
scope decisions, not benchmark rankings. FR-10's campaign/report tooling is
implemented. FR-11 scalar numerical auditing, FR-12 parameter-transition
tooling and FR-13 reproducible workflow are complete. FR-13 delivers tested,
documented tools to launch and monitor experiments, validate/archive results,
generate figures and tables, and export numeric includes/assets to the paper.
Spec 012 executed the long campaigns across three prepared measurement blocks
on one macOS ARM64 host. Earlier results retain their historical identities
and dependencies until the replacement and retirement gate permits removal.
Neither tooling completion nor the new results alone permits deleting them.

Use the existing [publication protocol](protocol.md) for current build commands
and measurement semantics. The manuscript integrates selected confirmation results with checked numerical
assets, while preserving the historical campaigns separately in its appendices.

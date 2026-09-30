# External FFT Comparison Plan

The completed implementation record lives in
[Spec 004: External FFT Comparison For The Scheduling Paper](../../../../specs/archive/004-external-fft-comparison.md).
It preserves the contender shortlist, research questions, adapter requirements,
measurement contracts and implementation evidence. Active replacement campaigns,
paper integration and retirement are tracked in
[spec 012](../../../../specs/012-comparison-evidence-and-paper-integration.md),
which inherits former FR-14 without claiming its measurements are complete.

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
It completes with a usable handoff; the user can then run the long campaigns.
Spec 012 still requires validated replacement measurements. Earlier results keep
their historical identities until the replacement and retirement gate passes;
tooling completion alone does not permit deleting them.

Use the existing [publication protocol](protocol.md) for current build commands
and measurement semantics. The manuscript's archived results remain unchanged
until validated comparison campaigns are deliberately integrated.

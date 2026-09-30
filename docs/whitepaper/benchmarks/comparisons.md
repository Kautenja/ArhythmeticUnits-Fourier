# External FFT Comparison Plan

The implementation plan now lives in
[Spec 004: External FFT Comparison For The Scheduling Paper](../../../specs/004-external-fft-comparison.md).
It owns the contender shortlist, research questions, adapter requirements,
measurement stages, acceptance criteria, and completion evidence.

Rack/PFFFT, optional FFTW3 and Apple Accelerate/vDSP, and the matched PFFFT
hybrid are implemented as benchmark-only adapters. KISS FFT (FR-7), Garrido
feedforward STFT (FR-8), and windowed sliding/hopping methods (FR-9) are
[explicitly deferred for the current paper](../../../specs/004-external-fft-comparison.md#optional-contender-decision).
The spec records evidence, claim limits and reopening criteria; these are
scope decisions, not benchmark rankings. FR-10's campaign/report tooling is
next, followed by FR-11 measurement and FR-12 paper integration.

Use the existing [publication protocol](README.md) for current build commands
and measurement semantics. The manuscript's archived results remain unchanged
until validated comparison campaigns are deliberately integrated.

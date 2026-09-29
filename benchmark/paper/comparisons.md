# External FFT Comparison Plan

The implementation plan now lives in
[Spec 004: External FFT Comparison For The Scheduling Paper](../../specs/004-external-fft-comparison.md).
It owns the contender shortlist, research questions, adapter requirements,
measurement stages, acceptance criteria, and completion evidence.

The first targets are Rack/PFFFT, FFTW3, and Apple Accelerate/vDSP. KISS FFT
is a reserve candidate; academic overlap-reuse methods require a separate
feasibility review. All external adapters remain planned. The spec also
includes a hybrid comparison to distinguish the value of suspending the FFT
from distributing preparation and postprocessing.

Use the existing [publication protocol](README.md) for current build commands
and measurement semantics. The manuscript's archived results remain unchanged
until validated comparison campaigns are deliberately integrated.

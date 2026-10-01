# Historical Manuscript Verification

These records describe the historical manuscript builds, not current replacement
benchmark campaigns. Original commands retain their historical paths.

## Verification Of Manuscript Version 2

The initial 19-page revision, including the consolidated implementation appendix,
compiled in the built-in LaTeX editor and with
`make -C whitepaper`, with no unresolved references or box warnings. The
artifact check passes for both campaigns, including the new prototype table
and exact-cadence example. Historical driver extraction, compilation, and
numerical verification also passed. Detailed documentation and production
validation evidence remains in the removed specifications in Git history.

The first literature-review updates retained manuscript version 2 at 20 pages.
They deepen the AES/DAFx scheduling comparison, replace the 2015
sliding-DFT exposition with Lyons and Howard (2021), and add benchmarking
references to distinguish existing evidence from a proposed future evaluation.
The expanded review also includes Park and Ko's hopping DFT (2014), Rafii's
kernel windowing (2018), and Wefers's partitioned-convolution treatment (2015).
It proposes matched overlap-reuse and work-granularity comparisons without
attributing new performance results to those sources.
The built-in LaTeX compiler and `make -C docs/whitepaper` both pass; the final
LaTeX pass has no unresolved references or box warnings. All 20 rendered pages
were visually inspected. `make -C docs/latex/deprecated/whitepaper check` passes with 20 cited
references and unchanged campaign data, and `git diff --check` passes.
No new timing campaign, DSP test run, Rack build, or manual Rack session was
performed for this literature-only update. Source-access limitations are
recorded in [sources.md](../sources.md).

The September 29 additions integrate Prusa and Holighaus (2016) into related
work and the display-latency discussion, and Wilhelm et al. (2008) into the
discussion of execution-time bounds. Further additions cover Bécoulet and
Verguet's depth-first FFT (2021), Garrido's feedforward STFT (2016), and
Eleftheriadis et al.'s partial-overlap STFT (2023), with van der Byl and Inggs
(2016) informing the proposed numerical-error evaluation. PFFFT and Apple
Accelerate/vDSP now join FFTW as explicit targets for external implementation
comparisons, with a linked plan for the adapters and publication outputs.
That revision retained version 2 at 22 pages with 28 cited references.
Both LaTeX builds and the artifact check
pass; the final LaTeX pass has no unresolved references or box warnings.
All 22 rendered pages were visually inspected, and `git diff --check` passes.
These additions preserve the campaign data and introduce no new timing,
DSP, Rack build, or manual Rack validation results.

The selective literature audit adds Borß (2009) on convolution load distribution,
Skare (2024) on audio benchmark invocation, and Balasubramaniam, Ramachandran,
and Timoney (2026) on controlled host contention. Comparisons were checked
against the current code; source-access details, excluded candidates, and venue
judgments are recorded in [sources.md](../sources.md). The manuscript remains
version 2, with 31 cited references and 24 pages in the current build.
`make -C docs/latex/deprecated/whitepaper check` and `make -C docs/whitepaper arxiv` pass.
The extracted single-file export compiles independently and has identical
page text to the project PDF. Both final LaTeX passes have no unresolved
references or box warnings. The rendered pages, new citation destinations,
primary-paper links, bookmarks, and PDF metadata were reviewed, and
`git diff --check` passes. No new benchmarks, DSP tests, Rack build, or manual
Rack session were run for this literature-only change.

## Historical Verification Of Manuscript Version 1

On September 28, 2026, the final 13-page source compiled successfully in the
built-in LaTeX editor and with `make -C whitepaper`. The local final log had
no unresolved references, overfull boxes, or underfull boxes. All pages were
rendered and visually inspected; PDF text extraction confirmed searchable
text and the author/bibliography metadata.

`make -C whitepaper check` passed the reference, link, source/data digest,
numerical table, figure-coordinate, and balanced-schedule checks. The root
`CITATION.cff` was validated against the official CFF 1.2.0 JSON schema.
`scons test/dsp/math/test_fft.cpp` passed 11,995 assertions in 14 test cases,
with the existing unused `Math::Window::names` warning. The report's separate
numerical and timing campaign is documented in `data/README.md`. No Rack
build or manual Rack session was performed for this documentation work.

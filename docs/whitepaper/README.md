# Resumable FFT Scheduling for Real-Time Spectral Analysis

**Resumable FFT Scheduling for Real-Time Spectral Analysis** is a
technical report by Christian Kauten (Arhythmetic Units). It examines the
implementation, scheduling bounds, frame cadence, latency, and empirical
cost of the original resumable FFT and its production successor, which
distributes complete analysis over one exact hop. It presents the work as
an implementation study within the established time-distributed FFT literature.
The supporting software is the Arhythmetic Units Fourier plugin for VCV Rack,
which contains the Fourier spectrum analyzer and Spectre spectrogram modules.

## Read And Build

Open [fourier.tex](fourier.tex) in the built-in LaTeX editor for an editable
source and PDF preview. The manuscript is self-contained: its vector
figures, code listing, tables, and bibliography need no external inputs.

For a local PDF export, install a TeX distribution with `pdflatex`,
`latexmk`, and the packages named in the preamble, then run from the
repository root:

```shell
make -C docs/whitepaper
```

This writes `docs/whitepaper/build/paper.pdf` and removes LaTeX auxiliary
files after a successful build. Failed builds retain their logs for diagnosis.
Experiment results and source archives in `build/` are preserved. Compilation
never runs a benchmark or fetches data. The single-column, 11-point layout
prioritizes readable derivations, code, and numerical results. No shell
escape or BibTeX pass is required.

## Contents

-   [fourier.tex](fourier.tex): Complete manuscript, implementation-reference
    appendix, and shared bibliography. The appendix covers bit reversal,
    maximum-size table reuse, batch transform pseudocode, and smoothing bounds.
-   [sources.md](sources.md): Primary-source verification of references and
    the scope of the literature review.
-   [experiments/evaluate.cpp](experiments/evaluate.cpp): Independent DFT
    checks, schedule checks, and a controlled scalar timing comparison.
-   [experiments/run.py](experiments/run.py): Reproduction driver, source
    hashes, metadata capture, and paired statistical summaries.
-   [data/README.md](data/README.md): Recorded campaign and interpretation.
-   [Complete-pipeline campaign](data/pipeline/README.md): Historical one-hop
    and overlapped scheduling evidence, with reproduction sources archived
    separately from the production implementation.
-   [data/](data/): Archived raw observations and derived statistics.
-   [../../CITATION.cff](../../CITATION.cff) and
    [CITATION.bib](CITATION.bib): The shared report citation for this work.

Run the artifact consistency check from the repository root:

```shell
make -C docs/whitepaper check
```

It verifies citation keys, local Markdown links, both campaigns' archived
source/data hashes, reported numerical summaries, and tables/plot coordinates
against data. Historical source hashes are checked inside the source archives,
so later production changes cannot silently redefine the measured code.
It does not establish LaTeX compilation or replace visual inspection.

## Reproduce The Experiments

For the exact historical sources, use the isolated extraction commands in
[the original campaign](data/README.md#reproduce-the-historical-experiment)
and [the pipeline campaign](data/pipeline/README.md#reproduce-the-historical-experiment).
Their metadata and observations are preserved unchanged.

The following command runs a new comparison using the current reusable FFT
headers; it does not time the modules' new `SpectrumAnalysis` implementation.

A C++11 compiler available as `g++` and Python 3 are sufficient. The driver
honors `CXX`. From the repository root, substitute your host information:

```shell
python3 docs/whitepaper/experiments/run.py --cpu 'YOUR CPU MODEL' --memory-gib 16
```

The hardware fields are declarations and must describe the actual machine.
New results go to `docs/whitepaper/build/evaluation/`; the archived campaign is
not overwritten. Both modes use the same scalar real FFT. The timing
experiment fixes cadence at the requested horizon to isolate work placement,
whereas the original modules restarted when computation completed. The
production successor now maintains exact cadence and schedules packing,
windowing, reconstruction, smoothing, and per-bin output as well. Historical
prototype timings motivated this choice; they are not integrated plugin timings.

The campaign does not measure Rack SIMD, module graphics, thread handoff,
actual device callbacks, or an optimized external FFT library. Observed
maxima are not worst-case execution-time guarantees. Preserve these limits
when reusing a result.

## Current Production Measurements

The [publication benchmark suite](../../benchmark/paper/README.md) measures the
current production core and modules, with raw simulated-callback observations,
matched batch/incremental controls built from this repository's RFFT, analyzer
scaling, spectrum-age audits, and forward/inverse transform phases. Its runner
preserves sources, compiler/SDK metadata, individual observations and numerical
checks for repeated campaigns. Future external backends can share its workload
contract. These new campaigns do not replace the historical evidence or update
the manuscript's tables automatically; comparisons require matched workloads,
independent sessions and explicit interpretation of the documented limits.

## Publication And Discoverability

The current document is manuscript version 2, dated September 28, 2026. It
has not been deposited on arXiv, assigned a DOI, or peer reviewed. Repository
citation metadata uses this manuscript as the single citation for the paper
and software. The [code on GitHub](https://github.com/Kautenja/ArhythmeticUnits-Fourier)
is its supporting artifact: link to it and identify the version or commit
used for reproducibility, without adding a separate software citation.
Citation metadata does not itself ensure Google Scholar indexing.

To prepare the source archive, run from the repository root:

```shell
make -C docs/whitepaper arxiv
```

The output is `docs/whitepaper/build/fourier-arxiv-source.tar.gz`, containing the
self-contained `fourier.tex`. Review the compiled PDF, author details,
claims, and license choice before submitting. Follow the current
[arXiv TeX guidance](https://info.arxiv.org/help/submit_tex.html); submission,
endorsement, and moderation are separate from a successful local build.
Nothing in this workflow publishes the paper automatically.

After a public deposit, update the report URL and identifier consistently in
`CITATION.cff`, `docs/whitepaper/CITATION.bib`, and the project README. Keep
the report title and author spelling stable. Link the public abstract page
and freely accessible, searchable PDF from the project. If a separate publication page
is hosted, provide accurate title, author, publication-date, and PDF metadata
as described in the
[Google Scholar inclusion guidelines](https://scholar.google.com/intl/en/scholar/inclusion.html).
Do not invent an identifier or publication date to populate those fields.
Indexing and citation counts remain external outcomes.

## Maintenance

Update claims and evidence together. Rerunning experiments does not
silently update the manuscript: review the new campaign, retain its metadata,
replace the archived data deliberately, and update the text, tables, plot,
and expected values in the consistency check. The bibliography is embedded
in the LaTeX file so that it remains portable; record any new verification
source in `sources.md`.

The report adds no production DSP behavior. The repository's
[license document](../../LICENSE.md) continues to govern the existing source
and visual assets; this manuscript does not grant new permissions for the
plugin artwork.

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
were visually inspected. `make -C docs/whitepaper check` passes with 20 cited
references and unchanged campaign data, and `git diff --check` passes.
No new timing campaign, DSP test run, Rack build, or manual Rack session was
performed for this literature-only update. Source-access limitations are
recorded in [sources.md](sources.md).

The September 29 additions integrate Prusa and Holighaus (2016) into related
work and the display-latency discussion, and Wilhelm et al. (2008) into the
discussion of execution-time bounds. The manuscript remains version 2 and is
now 21 pages with 22 cited references. Both LaTeX builds and the artifact check
pass; the final LaTeX pass has no unresolved references or box warnings.
All 21 rendered pages were visually inspected, and `git diff --check` passes.
These additions preserve the campaign data and introduce no new timing,
DSP, Rack build, or manual Rack validation results.

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

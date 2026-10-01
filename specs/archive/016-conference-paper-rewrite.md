# Focused Conference Paper Rewrite

Status: COMPLETE

## Goal

Rewrite the Fourier whitepaper as a clear conference-style paper of seven to
eight pages in two columns, with an absolute eight-page limit including the
abstract and references. Explain the original motivation for a resumable FFT
in the sample-processing loop, the complete analysis method, and the measured
tradeoff in bursts, aggregate cost, and publication delay.

## Requirements And Examples

-   Use a conventional introduction, related work, method, implementation,
    evaluation, results, discussion, and conclusion. Follow the RackNES example's
    two-column layout without claiming conformance to a particular venue.
-   Explain the idea before introducing the quota equations. For example, the
    4096-point scalar comparison must clearly distinguish a 7.82-fold reduction
    in typical peak block time from an 18% increase in instrumented cost.
-   Use only retained measurements. Keep unfavorable native and demanding-module
    results and distinguish missed synthetic releases from audio underruns.
-   Preserve useful appendices and the extended version 4 source outside the
    short paper. Keep its references usable through a separate report entry.
-   Keep generated tables traceable to evidence and align titles and citation
    metadata. Keep the normal build and portable export reproducible.

## Non-Goals

No DSP or module behavior changes, fresh performance collection, venue selection,
submission, publication, commit, or push. No invented empirical results or new
claims of FFT mathematics or scheduling priority.

## Acceptance And Validation

- [x] The PDF has seven or eight pages, including abstract and references.
- [x] All pages have been visually reviewed; equations, tables, references,
      metadata, bookmarks, and links are usable.
- [x] Generated evidence, manuscript checks, and source export pass.
- [x] The extracted portable source compiles and matches the project PDF.
- [x] The retained report still builds, with its previous content preserved.
- [x] Citation-title changes in the manuals build successfully.
- [x] Local links and `git diff --check` pass.

From the repository root:

```shell
python3 docs/whitepaper/tools/study_paper.py --check
make -C docs/whitepaper check
make -C docs/whitepaper arxiv
make -C docs/whitepaper TEX_SOURCE=report-v4.tex PDF_NAME=report-v4
make -C docs/manual-fourier
make -C docs/manual-spectre
pdfinfo docs/whitepaper/.build/paper.pdf
pdftoppm -r 100 -png docs/whitepaper/.build/paper.pdf docs/whitepaper/.build/conference-review/page
git diff --check
```

Extract `.build/fourier-arxiv-source.tar.gz` into an empty ignored directory
and run `latexmk -pdf -pdflatex='pdflatex -no-shell-escape %O %S' fourier.tex`
there. Compare extracted PDF text and rendered pages with the project build.

## Completion Evidence

October 1, 2026: completed manuscript version 5, *Scheduling FFT-Based Spectral
Analysis in the Audio Processing Loop*. The result is seven pages including
abstract and fourteen references, in two columns at 10 points with 0.75-inch
margins. It begins with the motivation for a resumable FFT, then explains the
complete analysis schedule and its measured burst/cost/age tradeoff. It uses
one conceptual schedule figure, one stage table, and three compact result
tables. Native wins, demanding-setting failures, numerical decay limits, and
inconsistent missed-release outcomes remain explicit.

The default manuscript includes no appendix. The existing appendix sources
remain intact; `report-v4.tex` selects the original extended manuscript using
preserved versions of the replaced prose, preamble, and bibliography. Expansion
of that source is byte-for-byte equal to the original HEAD manuscript
(SHA-256 `88188658973453b303bf33b6faa3804e86b0f594cfbdb056f007f2b4fef0f09c`).
Its 31 rendered pages and extracted text match an independently compiled
original source. This is preservation of the existing report, not endorsement
of its earlier presentation.

Validation actually run:

-   `make -C docs/whitepaper check` passed: all 776 processes, 194 cells, 388
    session summaries, generated outputs, fourteen references, local links,
    historical data/source hashes, and 32,768 balanced schedules. The new worked
    example and exclusion of appendices are also checked.
-   `python3 -m unittest discover -s docs/whitepaper/tools -p test_study_import.py`
    passed all six existing analysis regression tests.
-   The conference build and `arxiv` target passed. The final LaTeX pass has
    no overfull/underfull boxes or unresolved-reference warnings. The exported
    single-file source compiles to seven pages with identical extracted text
    and identical page pixels at 55 dpi.
-   All seven conference pages were visually inspected at 100/110 dpi.
    Metadata, eight top-level section bookmarks, and 47 link annotations were
    inspected. Tables and equations fit without clipping or overlap.
-   The retained report built successfully. Its existing underfull-box warnings
    are identical to those in the original build; no content or layout changed.
-   Both manuals built after citation-title and separate-supplement wording
    updates. The affected further-reading pages (PDF pages 19 and 21) were
    rendered and visually reviewed; no panel or screenshot change was needed.
-   `git diff --check` and the final local-link check passed. PDFs, export
    archives, render comparisons, and logs are confined to ignored build
    directories or temporary files.

No new performance timings, DSP tests, Rack build, or interactive Rack session
were run: the change is manuscript, artifact-generation, and citation work.
Production code and measured data are unchanged. Venue-specific formatting,
submission, and any public raw-data deposit remain outside this task. No commit,
push, or publication was performed.

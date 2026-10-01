# Complete The Comparison Paper

Status: COMPLETE

Completed: September 30, 2026

## Goal

Finish the technical report using the validated evidence from
[spec 012](012-comparison-evidence-and-paper-integration.md), with a
coherent analysis, reproducible publication assets, and a reviewed PDF and
portable source archive. Keep the existing single-column report format and
historical appendices. This is manuscript completion, not external submission.

## Requirements

-   Integrate matched analysis, isolated transforms, inverse jobs, complete
    chains, and relevant extension/transition results. Distinguish practical
    library comparisons from controlled scheduling comparisons.
-   Generate numerical tables and figures from checked evidence; retain
    selection identities, units, aggregation definitions and provenance.
    Include unfavorable results and descriptive session variation.
-   Align the abstract, contribution, mathematics, method, discussion,
    conclusion, related work and availability statement with the actual
    evidence. Verify citations against primary sources. Do not infer an audio
    deadline guarantee, universal speedup, or independent-host replication.
-   Preserve benchmark data, source snapshots, attribution and historical
    campaign identities. Do not rerun timing campaigns or change production
    DSP. Public raw-data availability must reflect the local-only bundles.
-   Update manuscript version and citation metadata consistently, without
    inventing a DOI, arXiv identifier, publication venue or indexing outcome.
-   Build the manuscript and extracted submission archive, check citations,
    references and data, and visually review every final PDF page.

## Examples

A lower p99 with higher aggregate cost is reported as a tradeoff. A batch
FFT that wins a matched workload remains in the table. Inverse job completion
age is not described as reconstructed audio latency. A missing SIMD scalar
audit is not described as zero error.

## Non-Goals

New experiments, production optimization, Rack integration, public upload,
repository push, and any promise of peer review or Google Scholar indexing.

## Acceptance And Validation

- [x] All quantitative claims trace to retained configurations and data.
- [x] Mathematical and literature review findings are resolved.
- [x] `python3 docs/whitepaper/tools/comparison_paper.py --check` passes.
- [x] `make -C docs/whitepaper check` passes.
- [x] `make -C docs/whitepaper arxiv` succeeds.
- [x] Extracted source builds with `latexmk -pdf
      -pdflatex='pdflatex -no-shell-escape %O %S' fourier.tex`.
- [x] Final PDF pages, metadata, links and bookmarks are reviewed.
- [x] `git diff --check` passes; only intended report/support changes remain.

## Completion Evidence

Completed manuscript version 3 in the existing single-column format. The
38-page report includes the completed confirmation method and interpretation,
separate inverse/overlap-save mathematics, six new comparison tables, a vector
length-trend figure, and revised abstract, contribution, discussion and
conclusion. Existing derivations and historical campaigns remain identified
in their appendices. Citation metadata in both BibTeX entries and CFF now
matches version 3; no public identifier was invented.

The publication-specific generator retains 131 selected configuration
identities and 21 source-input hashes. It rederives every stationary
hierarchical summary from the committed per-process tables, then checks its
nine generated assets byte for byte. The narrative includes unfavorable native
inverse comparisons, session variation, weak-bin diagnostics, allocation-scope
limits, and the small fractions of a nominal callback budget occupied by the
representative p99 measurements. Workload totals count timing passes separately.

Independent mathematical review confirmed the analysis work, quota, cadence,
and ring-capacity derivations against the measured source. It identified the
inverse/chain controls' unscheduled linear buffering and bulk inverse setup;
the new section explicitly excludes them from the production analyzer's
primitive-operation bound. Review also corrected actual warmup, float window
storage in double analysis, smoothing boundary arithmetic, independent-channel
coverage, and distinct analysis/synthesis acceptance policies. All 560
stationary analysis configurations have nine complete process audit records.
Primary-source literature review preserved 31 references, calibrated the
novelty claim, and documented access limits in the literature notes.

### Validation Run

From the repository root:

```shell
python3 docs/whitepaper/tools/comparison_paper.py --check
python3 -m py_compile docs/whitepaper/tools/comparison_paper.py docs/whitepaper/tools/check_paper.py
make -C docs/whitepaper check
make -C docs/whitepaper arxiv
mkdir -p .build/paper-013-arxiv
tar -xzf docs/whitepaper/.build/fourier-arxiv-source.tar.gz -C .build/paper-013-arxiv
cd .build/paper-013-arxiv
latexmk -pdf -pdflatex='pdflatex -no-shell-escape %O %S' -interaction=nonstopmode -halt-on-error fourier.tex
```

All commands passed. The publication checks cover the generated results,
31 citations, labels and local paths, original source/data hashes and plot
coordinates, 32768 balanced schedules, and the historical prototype table and
clean/dirty scheduling examples. The extracted archive contains a single
standalone `fourier.tex` and requires no benchmark data or network access to
compile. The final project and extracted builds have no overfull/underfull
boxes, unresolved references, or compiler warnings. Their extracted text
matches on all 38 pages.

Poppler rendered every final page; all pages were visually inspected, with
an independent detailed review of the comparison figure and tables. Fixed
long-path overflow, clarified caption units/settings, kept result floats in
their owning section, and removed orphaned closing paragraphs. PDF title and
author metadata match citation metadata; 237 named destinations resolve, with
35 root outline entries and 178 link annotations. Local review images and logs
are under `.build/paper-013-review/`; the build log is
`.build/paper-013-build.log`.

The native LaTeX editor was opened, but its single-file compiler could not
resolve this project's `preamble.tex`. The established project compiler and
portable extracted build verified the document instead. No source was removed
to accommodate that preview limitation.

All 809 original handoff/export files passed their unchanged SHA-256 manifest.
No raw campaign, historical dataset, measured source archive, production DSP,
Rack module, or benchmark C++ source changed. No timing measurements, DSP
suite, Rack plugin build, or manual Rack session were run for this manuscript
change. `git diff --check` passed.

### Delivery And Remaining Dissemination

The local, ignored outputs are `docs/whitepaper/.build/paper.pdf` and
`docs/whitepaper/.build/fourier-arxiv-source.tar.gz`. Reviewed output hashes:

| Artifact | SHA-256 |
| --- | --- |
| PDF | `fef562c8ca0e8fde1d6d4061519b64c5c3388efe341bbf88cda9b3a6ab383bac` |
| Portable source archive | `76fa817c7946df67b5e319dd7372101f7b28bc0f6ecfc09be91316f5dcf18b57` |

The paper and its compact evidence are ready for author review and a separate
publication/deposition step. Raw audit bundles remain local and untracked;
a fresh clone cannot rederive raw-observation statistics without them. No
arXiv submission, public raw-data deposit, DOI registration, repository push,
or Google Scholar indexing was performed or claimed.

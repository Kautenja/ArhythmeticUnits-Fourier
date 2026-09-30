# Resumable FFT Scheduling for Real-Time Spectral Analysis

A technical report by Christian Kauten (Arhythmetic Units), studying resumable
FFT execution, complete analysis scheduling, frame cadence and practical costs.
Manuscript version 3 incorporates the September 30, 2026 comparison campaign,
including periodic inverse jobs and complete overlap-save filtering controls.
The supporting software is the Fourier/Spectre VCV Rack plugin. The manuscript
is an implementation study within the time-distributed FFT literature; it is
not yet deposited on arXiv, assigned a DOI, or peer reviewed.

## Read And Build

[fourier.tex](fourier.tex) defines manuscript order. Prose lives in `sections/`
and `appendices/`; complete figures/tables live in `figures/` and `tables/`.
The paper has its own [preamble](preamble.tex) and single-column style. It shares
only the PDF build lifecycle with the manuals, not their typography.

From the repository root, with `latexmk`, `pdflatex` and the packages named in
the preamble:

```shell
make -C docs/whitepaper
make -C docs/whitepaper check
make -C docs/whitepaper arxiv
```

The outputs are `.build/paper.pdf` and `.build/fourier-arxiv-source.tar.gz`
inside this directory. Compilation never runs benchmarks, fetches results or
rewrites measurements. `check` verifies citations, links, historical evidence,
numerical tables and included generated assets. Successful PDF builds clean
auxiliary files; failed builds keep logs. `clean` preserves experiment outputs.

The arXiv target expands literal manuscript inputs into one TeX file and packs
any explicitly referenced local image assets.
Extract into an empty directory and compile with
`latexmk -pdf -pdflatex='pdflatex -no-shell-escape %O %S' fourier.tex`.
Review the PDF, claims, author information and license before any submission.
The archive command does not submit or publish anything.

## Project Map

| Location | Responsibility |
| --- | --- |
| [fourier.tex](fourier.tex), [preamble.tex](preamble.tex) | Manuscript order and typography |
| [sections/](sections/), [appendices/](appendices/) | Scientific prose and implementation reference |
| [figures/](figures/), [tables/](tables/) | Editorial figures and tables with labels/captions |
| [bibliography.tex](bibliography.tex), [sources.md](sources.md) | References and literature verification |
| [benchmarks/](benchmarks/README.md) | One maintained workflow for measurements and derived evidence |
| [data/](data/README.md) | Immutable manuscript campaigns and historical research |
| [tools/](tools/README.md) | Manuscript checks and source expansion |
| `.build/` | Local PDF, fixture and reproduction outputs |
| `generated/paper-comparison/` | Selected, checked tables, macros and vector plots used by the paper |
| Other `generated/` directories | Immutable full-grid diagnostic exports from the measurement handoff |

## Measurements And Reproduction

Start with the [benchmark workflow](benchmarks/README.md), then follow its
[launch handoff](benchmarks/guides/workflow.md). It covers setup, the small smoke
profile, pilots, frozen confirmation sessions, progress/failure inspection,
reports, selections, paper export and portable evidence bundles. The C++
measurement code remains in [benchmark/paper](../../benchmark/paper/README.md).
Neither the benchmark runner nor report generation changes the Rack modules.

The completed comparison measurements are separate from the two historical
manuscript campaigns: [original FFT evaluation](data/README.md) and
[complete-pipeline study](data/pipeline/README.md). Their raw bytes, metadata
and source archives remain intact. Use their isolated extraction commands for
exact reproduction. Historical optimization investigations are retained under
[data/research/](data/research/). Their one-off programs are archived research
artifacts, not competing maintained workflow entry points.

The [archived comparison implementation](../../specs/archive/004-external-fft-comparison.md)
records the completed tooling. [Spec 012](../../specs/archive/012-comparison-evidence-and-paper-integration.md)
records the replacement campaign and
[results handoff](data/comparison-012/README.md), covering forward, inverse and
complete-chain work. [Spec 013](../../specs/archive/013-comparison-paper.md) records
the manuscript integration and publication checks. The paper compares total
cost, callback bursts and algorithmic age; it does not claim a universal
speedup. New evidence must
pass provenance, numerical coverage and session checks before explicit export.
Hardware-dependent timing replication is different from regenerating statistics
from retained raw data. [Historical verification notes](data/verification-history.md)
record earlier manuscript builds and their original commands.

## Maintain And Cite

Keep prose in the existing section/appendix files and preserve labels. Use
standalone literal `\input{path.tex}` lines relative to this directory so the
checker and source exporter see the same manuscript. Do not introduce shell
escape, automatic measurements or dynamic data downloads into the paper build.
For source-only TeX reorganizations, compare before/after rendered pages, text,
metadata and references and compile the extracted export.

Paper export writes separate table/macro/figure includes and a provenance
receipt. The publication-specific
[comparison generator](tools/comparison_paper.py) derives the selected tables,
vector plots and prose numbers from the immutable committed tables. It verifies
the input hashes and per-session aggregation without requiring local raw
bundles. Run it without arguments to regenerate, or with `--check` to compare
against committed assets; `make check` includes this check. The original
diagnostic exports and historical results remain unchanged.

The compact evidence and exact measured-source archive are committed. Individual
callback observations remain in local, untracked bundles; a fresh clone cannot
rederive statistics without acquiring those bundles. Their locations and hashes
are in the [handoff](data/comparison-012/README.md). Public raw-data deposition
is still a dissemination step, and no such deposit is claimed.

[CITATION.bib](CITATION.bib) and the repository [CITATION.cff](../../CITATION.cff)
identify the manuscript as the shared report/software citation. After a public
deposit, update its real identifier and URL consistently in both and the project
README. Do not invent a DOI or publication date. Public searchable PDFs,
accurate title/author metadata and stable links help discovery, but Scholar
indexing and citation counts remain external outcomes.

For a public deposit, follow the [arXiv TeX guidance](https://info.arxiv.org/help/submit_tex.html)
and the chosen license terms. [Scholar inclusion guidance](https://scholar.google.com/intl/en/scholar/inclusion.html)
describes discovery requirements; a source archive or citation file alone does
not establish indexing.

The repository [license](../../LICENSE.md) governs its code and artwork.
Benchmark provider and redistribution limits are documented with the workflow.

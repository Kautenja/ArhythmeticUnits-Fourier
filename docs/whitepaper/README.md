# Resumable FFT Scheduling for Real-Time Spectral Analysis

A technical report by Christian Kauten (Arhythmetic Units), studying resumable
FFT execution, complete analysis scheduling, frame cadence and practical costs.
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
| `generated/` | Explicitly exported, checked replacement assets when FR-14 integrates them |

## Measurements And Reproduction

Start with the [benchmark workflow](benchmarks/README.md), then follow its
[launch handoff](benchmarks/guides/workflow.md). It covers setup, the small smoke
profile, pilots, frozen confirmation sessions, progress/failure inspection,
reports, selections, paper export and portable evidence bundles. The C++
measurement code remains in [benchmark/paper](../../benchmark/paper/README.md).
Neither the benchmark runner nor report generation changes the Rack modules.

Current replacement measurements are separate from the two historical
manuscript campaigns: [original FFT evaluation](data/README.md) and
[complete-pipeline study](data/pipeline/README.md). Their raw bytes, metadata
and source archives remain intact. Use their isolated extraction commands for
exact reproduction. Historical optimization investigations are retained under
[data/research/](data/research/). Their one-off programs are archived research
artifacts, not competing maintained workflow entry points.

The [comparison specification](../../specs/004-external-fft-comparison.md)
tracks implementation, user-run measurements and final paper integration
separately. A smoke report proves tooling, not a speedup. New evidence must
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
receipt. Editorial insertion and interpretation belong to FR-14; export never
rewrites prose or historical results. Included generated receipts are checked
for freshness. Retain source selections and evidence bundles with published
claims; do not transcribe numeric results manually.

[CITATION.bib](CITATION.bib) and the repository [CITATION.cff](../../CITATION.cff)
identify the manuscript as the shared report/software citation. After a public
deposit, update its real identifier and URL consistently in both and the project
README. Do not invent a DOI or publication date. Public searchable PDFs,
accurate title/author metadata and stable links help discovery, but Scholar
indexing and citation counts remain external outcomes.

The repository [license](../../LICENSE.md) governs its code and artwork.
Benchmark provider and redistribution limits are documented with the workflow.

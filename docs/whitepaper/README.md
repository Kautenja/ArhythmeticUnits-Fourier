# Scheduling FFT-Based Spectral Analysis In The Audio Processing Loop

A seven-page, two-column conference-style preprint by Christian Kauten
(Arhythmetic Units). The argument is that distributing FFT-based analysis across
sample calls reduces processing bursts in exchange for overhead and later
results. The paper explains the resumable transform, the complete analysis
schedule, and selected comparisons from the retained M1 Pro study.
Manuscript version 5 is dated October 1, 2026. It has not been peer reviewed by
a venue, deposited on arXiv, or assigned a DOI.

## Read And Build

[fourier.tex](fourier.tex) defines the paper's reading order. Prose lives in
`sections/`; editorial figures and tables live in their matching directories.
The [preamble](preamble.tex) uses a two-column, 10-point letter layout based on
[the RackNES example](https://github.com/Kautenja/RackNES/tree/master/whitepaper).
This is a conference-length draft, not a specific venue's submission template.
The abstract and references are included in its seven pages; appendices are
excluded. The requested limit is eight pages in total.

From the repository root, with `latexmk`, `pdflatex`, Python 3 and the preamble
packages available:

```shell
python3 docs/whitepaper/tools/study_paper.py --check
make -C docs/whitepaper
make -C docs/whitepaper check
make -C docs/whitepaper arxiv
```

Outputs are `.build/paper.pdf` and `.build/fourier-arxiv-source.tar.gz` inside
this directory. These targets never collect timings or modify measurements.
The check validates references, links, generated results, and historical
artifacts. Successful PDF builds clean auxiliary files; failed builds retain
logs. `clean` preserves experiment data.

Extract the source archive into an empty directory and compile it with
`latexmk -pdf -pdflatex='pdflatex -no-shell-escape %O %S' fourier.tex`.
The exporter expands only the paper's literal inputs into one portable source.
It does not include the extended report, publish, or submit anything.

## Extended Material

The original appendices remain in [appendices/](appendices/README.md). They
contain FFT derivations, scheduling proofs, implementation details, historical
experiments, and broader related work. They do not enter the conference PDF.

[report-v4.tex](report-v4.tex) preserves the previous extended manuscript,
*Whole-Pipeline Scheduling for Real-Time Spectral Analysis*, including its
original reading order and appendices. The prose replaced by version 5 and its
original typography and bibliography are in
[appendices/report-v4/](appendices/report-v4/). Unchanged derivations and earlier
sections remain shared. The retained report is historical context, not an
additional part of the conference submission.

To build it independently, from the repository root:

```shell
make -C docs/whitepaper TEX_SOURCE=report-v4.tex PDF_NAME=report-v4
```

The result is `.build/report-v4.pdf`. The default build and portable export
continue to select only the seven-page paper.

## Evidence And Source Map

| Location | Responsibility |
| --- | --- |
| [fourier.tex](fourier.tex), [preamble.tex](preamble.tex) | Conference reading order and typography |
| [sections/](sections/) | Main argument; the entry point selects active files |
| [figures/](figures/), [tables/](tables/) | Editorial illustrations and tables |
| [data/study-014/](data/study-014/README.md) | Measured source, compact evidence, identities and rederivation commands |
| [generated/paper-study/](generated/paper-study/) | Checked tables/macros and all 776 process summaries |
| [report-v4.tex](report-v4.tex), [appendices/](appendices/README.md) | Retained extended report and supporting sources |
| [tools/](tools/README.md) | Evidence generation, artifact checks and portable export |
| [benchmarks/](benchmarks/README.md) | Separate user-run collection workflow |
| [bibliography.tex](bibliography.tex), [sources.md](sources.md) | Selected references and literature provenance |
| [reviews/](reviews/) | Historical reviews of the extended report |
| `.build/` | Ignored PDFs, exports and visual-review products |

The study is descriptive: two sessions on one host, binary, and date. Typical
hop peaks, instrumented block cost, publication age, and late synthetic releases
are distinct measures. The findings do not establish audio-device reliability
or an optimal FFT granularity. The [evidence guide](data/study-014/README.md)
records aggregation, provenance, and limits. Original raw sessions remain local;
no public raw-data deposit is claimed.

Earlier [FFT](data/README.md), [pipeline](data/pipeline/README.md), and
[spec 012](data/comparison-012/README.md) artifacts remain for provenance, outside
the current performance argument. Version 5 adds compact table selections from
the same validated evidence. It does not recollect or filter the measurements.

## Maintain And Cite

Keep literal standalone `\input{path.tex}` lines relative to this directory.
Generate tables by editing [study_paper.py](tools/study_paper.py), running it,
and then checking with `--check`; do not edit generated exports. Every selected
row retains its configuration and process identities in the full-grid CSVs.

[CITATION.bib](CITATION.bib) and [CITATION.cff](../../CITATION.cff) identify
*Scheduling FFT-Based Spectral Analysis in the Audio Processing Loop*,
manuscript version 5. The [rewrite specification](../../specs/archive/016-conference-paper-rewrite.md)
records validation. A public deposit must use an actual identifier and URL.
Source and artwork terms remain in [LICENSING.md](../../LICENSING.md).

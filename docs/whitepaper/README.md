# Whole-Pipeline Scheduling for Real-Time Spectral Analysis

A technical report by Christian Kauten (Arhythmetic Units) about complete
analysis scheduling, exact-hop publication and the tradeoffs between work
placement, native kernels and host execution. Manuscript version 4 uses the
user-collected spec 014 study: two separately prepared pilot sessions and 776
fresh processes on an Apple M1 Pro. It is not yet deposited on arXiv, assigned a
DOI or peer reviewed by a venue. Independent agent reviews and their
[revision response](reviews/revision-4-response.md) are retained separately.

## Read And Build

[fourier.tex](fourier.tex) defines manuscript order. Prose lives in `sections/`
and `appendices/`; complete figures/tables live in their matching directories.
The paper has its own [preamble](preamble.tex) and single-column report format.
It shares only the PDF lifecycle with the manuals. Venue selection, formatting,
length and anonymity remain a later submission task.

From the repository root, with `latexmk`, `pdflatex` and the preamble packages:

```shell
python3 docs/whitepaper/tools/study_paper.py --check
make -C docs/whitepaper
make -C docs/whitepaper check
make -C docs/whitepaper arxiv
```

Outputs are `.build/paper.pdf` and `.build/fourier-arxiv-source.tar.gz` inside
this directory. Compilation/checks never collect timings, download data or
rewrite measurements. The check validates citations/links, current generated
assets and separately retained historical evidence. Successful PDF builds clean
auxiliary files; failed builds retain logs. `clean` preserves experiment data.

Extract the portable source archive into an empty directory and compile with
`latexmk -pdf -pdflatex='pdflatex -no-shell-escape %O %S' fourier.tex`.
The exporter expands literal inputs and includes explicitly referenced assets.
It does not publish or submit the manuscript.

## Evidence And Source Map

| Location | Responsibility |
| --- | --- |
| [fourier.tex](fourier.tex), [preamble.tex](preamble.tex) | Reading order and typography |
| [sections/](sections/), [appendices/](appendices/) | Current argument and supporting derivations |
| [figures/](figures/), [tables/](tables/) | Editorial figures and tables |
| [data/study-014/](data/study-014/README.md) | Current compact evidence, original identities and rederivation commands |
| [generated/paper-study/](generated/paper-study/) | Checked current tables/macros/plots and all 776 process summaries |
| [tools/](tools/README.md) | Read-only import, statistical generation, checks and portable export |
| [benchmarks/](benchmarks/README.md) | Benchmark preparation and user-run collection workflow |
| [reviews/](reviews/) | Independent reviews and revision response |
| [bibliography.tex](bibliography.tex), [sources.md](sources.md) | References and literature verification |
| `.build/` | Local PDF/export and visual-review products |

The [evidence guide](data/study-014/README.md) explains exact provenance,
aggregation and limits. Hop peaks, instrumented block cost, process CPU, logical
age and missed synthetic releases remain distinct. The study is descriptive:
one host, binary and date, with two session replicates and residual OS activity.
It does not establish audio-device reliability or an optimal FFT granularity.

The new manuscript replaces the old performance argument. Earlier
[FFT](data/README.md), [pipeline](data/pipeline/README.md), and
[spec 012](data/comparison-012/README.md) artifacts stay intact for provenance;
their timing sections and generated plots are outside the active manuscript.
The [archived preparation spec](../../specs/archive/014-benchmark-reliability-and-scheduling-study.md)
records the measured implementation. New analysis does not mutate original raw
sessions. Their handbacks remain local; no public raw-data deposit is claimed.

## Maintain And Cite

Keep literal standalone `\input{path.tex}` lines relative to this directory so
artifact checking and portable export see the same content. Use
`python3 docs/whitepaper/tools/study_paper.py` to regenerate current assets,
then `--check` to verify them. The generator reads compact evidence; the explicit
importer requires original sessions to rederive process summaries. Neither runs
benchmarks. [Historical verification notes](data/verification-history.md) retain
old commands and outcomes without assigning them to the current revision.

[CITATION.bib](CITATION.bib) and [CITATION.cff](../../CITATION.cff) identify
manuscript version 4 consistently. A public deposit must use an actual identifier
and URL; do not invent a DOI or acceptance. The [repository license](../../LICENSING.md)
governs source and artwork; provider redistribution limits remain in the
benchmark workflow. Review claims, authorship, license and the target venue's
current requirements before any submission.

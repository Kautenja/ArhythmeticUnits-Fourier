# Whitepaper Housekeeping

Status: COMPLETE

## Goal

Make `docs/whitepaper` the home of the accepted seven-page conference paper,
with the extended report and historical material in
`docs/latex/deprecated/whitepaper`. This is a structural refactor: preserve
manuscript text, numerical values, labels, and rendered pages.

## Requirements

-   Keep the active paper self-contained in `fourier.tex`, following RackNES:
    prose, typography, figures, tables, and bibliography in one canonical file.
    Typesetting and source packaging must work without Python or generation.
    Current evidence and maintained tools remain separate from typesetting.
-   Archive the extended report, unused sources, older data and experiment
    helpers together, retaining their provenance and a working report build.
-   Separate current-paper validation from historical validation.
-   Keep active tooling to paper checking, asset generation, and evidence
    import, with regressions in `tools/tests/`. The check must derive values
    in memory without reading or writing generated files. Explicit generation
    writes tables, macros, and process summaries into ignored build output.
    Build, check, and portable export must work from clean output.
-   Preserve existing deletions of generated outputs and historical reviews.
    Do not recover those files into tracked source. Retain the measured evidence.
-   Update documentation and source references to moved paths. Do not change
    historical hashes, raw evidence, or archived source payloads.

## Non-Goals

No manuscript rewrite, new measurements, DSP or Rack behavior changes, dependency
updates, or paper publication. Commit, push, and a pull request into `main` were
explicitly requested after the completed refactor.

## Acceptance And Validation

- [x] Active and archived manuscripts render identically to their baselines.
- [x] Clean paper build, check, and portable export pass; exported source builds.
- [x] Historical checks and the extended-report build pass independently.
- [x] Changed publication-tool tests and the benchmark Python suite pass.
- [x] Existing data/archive bytes and pre-existing deletions are preserved.
- [x] Local links, path references, and `git diff --check` pass.

Commands from the repository root:

```shell
make -C docs/whitepaper clean
make -C docs/whitepaper PYTHON=/nonexistent
make -C docs/whitepaper source PYTHON=/nonexistent
make -C docs/whitepaper check
make -C docs/latex/deprecated/whitepaper check
make -C docs/latex/deprecated/whitepaper
python3 -m unittest discover -s docs/whitepaper/tools/tests
python3 -m unittest discover -s docs/latex/deprecated/whitepaper/tools/tests
python3 -m unittest discover -s docs/whitepaper/benchmarks/tests
make check-build
git diff --check
```

Compile the exported source independently, then compare PDF text and rendered
page pixels. Keep baseline snapshots and QA output in
`.build/whitepaper-refactor/`, outside the source directories.

## Completion Evidence

Completed October 1, 2026 on macOS ARM64.

-   Moved extended report sources, appendices, older experiment evidence,
    configurations, and historical tools to
    `docs/latex/deprecated/whitepaper`. Preserved attribution and licenses.
-   Consolidated the active manuscript into `fourier.tex`, using the RackNES
    reference
    (<https://github.com/Kautenja/RackNES/tree/master/whitepaper>). Removed the
    redundant active section, figure, table, preamble, and bibliography files.
    All non-comment lines remain identical to the previously expanded source;
    no prose, values, labels, or typography changed. The native LaTeX editor
    compiled the standalone source successfully.
-   Reduced active tools to `check_paper.py`, `study_paper.py`, and the unchanged
    `study_import.py`, plus their tests and README. Consolidated study access
    into generation and link validation into the checker. Removed the obsolete
    cache-management layer; archived the legacy split-source exporter with its
    regression. The archived generator is now `report_assets.py`.
-   Shared validated study access and formatting between the two generators.
    The active generator writes only its three result tables, macros,
    full-grid CSVs, and provenance under `.build/paper-study/`. Historical
    generation and validation belong to the archive. New benchmark publication
    exports use `docs/whitepaper/.build/exports/`. The paper check passed with
    `.build/paper-study/` absent and did not recreate it. All generated TeX and
    full-grid CSV payloads matched the pre-simplification bytes. The canonical
    paper, PDF, importer, and measured evidence retained their SHA-256 hashes.
-   Clean conference build and source packaging passed with
    `PYTHON=/nonexistent`, demonstrating that neither depends on the evidence
    tools. The separate artifact check passed. The portable archive contains
    exactly the canonical `fourier.tex`; it compiled independently. The archived
    report and historical checks passed, including all 107 retained spec 012 handoff
    identities. Historical comparison generation and read-only checks passed.
-   Conference PDF: seven pages. Archived report: 31 pages. Exported paper:
    seven pages. Every page matched its pre-refactor baseline in extracted
    text, page dimensions, annotation count, and rendered pixels at 72 dpi.
    Repeated the seven-page paper and export comparisons after single-file
    consolidation; every rendered pixel and extracted text still matched.
    PDF titles also matched. Inspected the rendered first and final conference
    pages; no visual change. Build warnings about references resolved on rerun.
-   Final tool regressions: 11 active tests, one archived-export test, and all
    seven affected benchmark-workflow tests passed. The new checks reject
    edited measurements and changed, missing, or duplicate paper result blocks.
    Cache-only regressions were retired with the cache layer. The full benchmark
    suite passed during the preceding path refactor (126 tests, one optional
    plotting-environment skip); its export test now lives in the archive.
    Build-system regressions: all five tests passed with `make check-build`.
-   All 169 pre-existing tracked non-Markdown data/source payloads retained
    their hashes. All six local raw campaign bundles retained their recorded
    sizes and SHA-256 identities. Active and archived derived contents matched
    the previously committed outputs byte-for-byte, excluding provenance
    records that now identify the refactored generators.
-   Preserved the 736 pre-existing deletions of generated assets and review
    files. Needed assets regenerate into ignored directories. Historical review
    links point to the completed rewrite's Git snapshot. Frozen spec 012
    READMEs and manifests retain their original bytes and recorded old paths;
    the archive guide explains relocation and the removed bulk exports.
-   Maintained local Markdown links and `git diff --check` passed. Updated
    contributor and agent guidance and publication CI for the new layout.

No new measurements or manual Rack session were needed. No production C++ or
Rack behavior changed. External paper submission and publication remain out of
scope.

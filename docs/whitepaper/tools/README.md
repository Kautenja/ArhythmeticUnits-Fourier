# Manuscript Tools

These tools inspect or package the manuscript; they never collect benchmarks.
Use `make -C docs/whitepaper check` or `arxiv` from the repository root.

-   `manuscript.py` expands the literal TeX input tree and writes portable source.
-   `check_paper.py` checks references, links, historical data, derived tables
    and freshness of explicitly included generated evidence.
-   `check_pipeline.py` validates the historical pipeline archive independently
    of the current DSP implementation.

Measurement, validation, reporting and evidence packaging use the separate
[benchmark workflow](../benchmarks/README.md). Historical prototype drivers live
in [benchmarks/history/](../benchmarks/history/README.md), and their exact measured
versions remain inside the manuscript source archives.

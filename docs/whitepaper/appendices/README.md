# Supporting Material Outside The Conference Paper

These sources retain derivations, algorithms, implementation details, historical
experiments, and extended discussion. They are excluded from
[fourier.tex](../fourier.tex) and its portable export. The main paper is a
self-contained seven-page narrative, including abstract and references.

## Retained Report

[report-v4.tex](../report-v4.tex) is the previous extended manuscript with its
original appendix sequence. The version 4 main-text sources replaced during
the rewrite, together with its preamble and bibliography, are preserved in
[report-v4/](report-v4/). This keeps their cross-references usable in the retained
report without requiring them in the short paper.

From the repository root, build that report independently:

```shell
make -C docs/whitepaper TEX_SOURCE=report-v4.tex PDF_NAME=report-v4
```

Its output is `docs/whitepaper/.build/report-v4.pdf`. The report records the
previous argument and is not a supplement automatically attached to a conference
submission. Any venue-specific supplement must follow that venue's rules.

## Source Map

-   [Transform derivations](transform.tex): Butterfly arithmetic, preservation
    of operation order, resumable traversal, and real-spectrum reconstruction.
-   [Original scheduling](original-scheduling.tex) and
    [pipeline cost](original-pipeline.tex): Completion proof, cadence examples,
    smoothing bias, and residual boundary work in the earlier implementation.
-   [One-hop details](one-hop.tex) and
    [implementation reference](implementation.tex): Dispatch listing, weighted
    quota examples, input lifetime, table reuse, smoothing, caches and lifecycle.
-   [Original experimental method](experimental-method.tex),
    [FFT results](original-results.tex), and
    [prototype evaluation](prototype-evaluation.tex): Historical campaigns with
    their original tables and limitations. These also remain outside report v4's
    active reading order.
-   [Extended related work](related-work.tex) and
    [evaluation agenda](evaluation-agenda.tex): Broader comparisons and proposed
    experiments, with the original citations and scope limits.
-   [Reproducibility](reproducibility.tex): Extended report's provenance and
    reproduction notes. The [historical availability source](availability.tex)
    is retained separately.

Preserve labels in these sources. Input paths remain literal and relative to
the whitepaper directory. The [paper guide](../README.md) explains the active
manuscript, build checks, and evidence generation.

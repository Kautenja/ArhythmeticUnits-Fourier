# Supporting Material Outside The Conference Paper

These sources preserve derivations, algorithms, implementation details,
historical experiments, and extended discussion outside the
[conference paper](../../../../whitepaper/README.md).

## Retained Report

The [extended report](../fourier.tex) includes its original appendix sequence.
Main-text sources, typography, and bibliography live alongside this directory
in the [archive](../README.md). From the repository root, build it with:

```shell
make -C docs/latex/deprecated/whitepaper
```

The result is `docs/latex/deprecated/whitepaper/.build/report-v4.pdf`.

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
the archive directory. The [archive guide](../README.md) explains the build,
checks, and evidence locations.

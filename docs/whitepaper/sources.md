# Conference Paper Sources

The inline bibliography in [fourier.tex](fourier.tex) contains the fourteen
works cited in the conference paper.
`make -C docs/whitepaper check` verifies that every reference is cited and every
citation resolves.

The [study evidence guide](data/study-014/README.md) documents the measurements,
source identities, aggregation rules, and limitations behind the numerical
results. Derived tables and full-grid CSVs belong in `.build/paper-study/`;
reproduce them with `make -C docs/whitepaper generate` from the repository root.

The broader [literature and provenance notes](../latex/deprecated/whitepaper/sources.md)
remain with the extended report. They include sources and implementation
background that are outside the conference paper's focused argument.

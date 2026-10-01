# Paper Tools

Three commands support the standalone paper. None collects measurements, and
none is needed to typeset `fourier.tex`. Run them from the repository root.

| Command | Purpose |
| --- | --- |
| `make -C docs/whitepaper check` | Check the paper directly against retained evidence, without writing generated assets |
| `make -C docs/whitepaper generate` | Write derived tables, macros, full-grid CSVs, and provenance to `.build/paper-study/` |
| `python3 docs/whitepaper/tools/study_import.py SESSION_ROOT --output NEW_DIRECTORY` | Validate original sessions and rederive compact evidence; see the [evidence guide](../data/study-014/README.md) for actual paths |

`check_paper.py` owns manuscript, citation, metadata, link, and worked-example
checks. It compares the four marked study blocks with freshly derived values
in memory. Existing generated files are not inputs to the check.

`study_paper.py` owns evidence hash verification, aggregation, formatting, and
asset generation. It also supplies the shared study calculations used by the
archived report. `study_import.py` retains its original bytes because its hash
is part of the evidence receipt.

Regression tests live in `tests/`:

```shell
python3 -m unittest discover -s docs/whitepaper/tools/tests
```

The older split-source exporter lives with the
[archived report tools](../../latex/deprecated/whitepaper/tools/README.md).
Measurement collection uses the separate
[benchmark workflow](../benchmarks/README.md).

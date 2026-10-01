# Historical Manuscript Tools

From the repository root, run
`make -C docs/latex/deprecated/whitepaper check` to validate the retained
historical evidence without collecting new measurements.

-   `check_history.py` checks source and data hashes, numerical results,
    historical tables, scheduling examples, and compact comparison evidence.
-   `check_pipeline.py` independently validates the prototype pipeline archive.
-   `comparison_paper.py` rederives the older spec 012 tables, macros, and vector
    figures. Bare invocation writes `.build/paper-comparison/` in this archive;
    `--check` requires those outputs and verifies their exact contents.
-   `report_assets.py` generates the extended report's study 014 tables, macros,
    vector figures, and full-grid CSVs into `.build/paper-study/`. It shares
    study calculations with the active paper's `study_paper.py`. The report
    build regenerates these assets; `--check` verifies them without writing.
-   `manuscript.py` expands the archived report's literal TeX inputs for checks
    and legacy source export. Its regression lives in `tests/`.

Run the archived exporter regression from the repository root:

```shell
python3 -m unittest discover -s docs/latex/deprecated/whitepaper/tools/tests
```

The [active paper tools](../../../../whitepaper/tools/README.md) contain only
current validation, generation, and evidence import. Exact historical measured
tool versions remain inside the retained source archives.

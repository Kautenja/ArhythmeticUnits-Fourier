# Local Portable Evidence

The six verified audit bundles are stored here outside `.build`, so the
repository's ordinary `make clean` does not remove this evidence. Archive bytes
are local and deliberately ignored by Git; the parent handoff manifest records
their exact sizes and SHA-256 hashes. A clone alone does not contain raw data.
Keep or copy these files when moving the study to another workspace. No external
data deposit has been made.

-   `primary.tar.gz`, `extensions.tar.gz`, `short-hop.tar.gz`,
    `single-sample.tar.gz`, `transitions.tar.gz`: three frozen confirmation
    sessions per bundle, complete raw observations, checked reports,
    selections and portable derivation tools.
-   `pilots.tar.gz`: both successful pilots supporting the five freezes.

The parent `design/`, `host/`, `index.json` and `verification/` records supplement
these bundles with preparation, inclusion rationale and handoff validation.
Native executables, Rack SDK bytes and dependency archives are omitted under
the explicit bundle policy. Reproducing statistical derivation does not require
those bytes; running new timings does.

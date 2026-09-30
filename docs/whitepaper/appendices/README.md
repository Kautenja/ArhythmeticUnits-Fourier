# Supporting Appendices

These sources preserve the derivations, algorithms, implementation details,
and extended discussion behind the compact main narrative. All appendices are
included by [fourier.tex](../fourier.tex) and its standalone source export.

## Main Text Boundary

The main text follows the problem, closest prior work, frozen-frame signal
model, legacy motivation, and complete one-hop design before the experimental
methods and results. It retains the dependency order, total work and quota
equations, exact publication bound, publication ages, retained-input capacity,
and output ownership contract. The main evaluation describes the completed confirmation study, including
inverse jobs and overlap-save filtering controls. Historical methods and results
remain in the appendices, separate from the current production comparisons.

References precede a page break into the appendices, making the main narrative
boundary visible in the PDF. The main text summarizes the supporting arguments
and links to their full derivations here.

## Source Map

-   [Transform derivations](transform.tex): Butterfly arithmetic,
    arithmetic-order proof, resumable step listing, and real reconstruction.
-   [Original scheduling](original-scheduling.tex) and
    [pipeline cost](original-pipeline.tex): Completion proof, cadence table,
    timeline, smoothing bias, and residual boundary work.
-   [One-hop details](one-hop.tex) and
    [implementation reference](implementation.tex): Dispatch listing, quota
    examples and proof, ring lifetime, table reuse, smoothing, caches,
    and lifecycle behavior.
-   [Original experimental method](experimental-method.tex),
    [FFT results](original-results.tex), and
    [prototype evaluation](prototype-evaluation.tex): Complete historical
    campaigns, including their original tables, figures, and limitations.
-   [Extended related work](related-work.tex) and
    [evaluation agenda](evaluation-agenda.tex): Detailed comparisons and
    proposed experiments, with their original citations and limitations.
-   [Reproducibility](reproducibility.tex) and
    [availability](availability.tex): Artifact locations, reproduction
    commands, and citation and licensing information.

Keep existing labels when moving supporting material, and keep every input
literal and relative to the whitepaper directory. The
[whitepaper guide](../README.md) provides build and artifact-check commands.

## Conference Versions

This organization prepares for a conference version; it does not establish
page-limit compliance. The
[DAFx 2026 call](https://dafx26.mit.edu/call-for-papers/) allowed eight pages
maximum and required its template. The current report retains its
single-column layout. A submission must use the chosen edition's template and
rules for references, appendices, supplementary material, and anonymity;
appendices must not be assumed exempt from its page limit.

The separate sources let a later venue-specific submission select what belongs
in its permitted supplement while preserving the full report. Algorithm bodies,
proofs, and worked mathematics remain available for implementation and reference.

# Manuscript Version 4 Review Response

This response maps the independent
[September 30 reviews](2026-09-30-Opus-5-5-M/meta-review.md) to the full-paper
revision and the [new evidence](../data/study-014/README.md). The user's current
priority is strengthening the full paper before choosing a venue. This is not
an assertion of acceptance or conference-format compliance.

## Revised Argument

The paper now centers on whole-pipeline scheduling, exact-hop cadence and the
measured choice of execution granularity. The new 776-process study replaces
the old performance argument. It separates matched placement, native-layout
kernels, completion horizon, complete modules and actual Rack engine behavior.
The title is **Whole-Pipeline Scheduling for Real-Time Spectral Analysis**.

The clearest practical contrast is four aligned Fourier modules on one Rack
thread: production, vDSP batch and vDSP hybrid have typical hop peaks of
201, 890 and 423 microseconds, respectively, against a 1,333 microsecond block
budget. The cost and publication-age penalties appear alongside those values.
Native hybrids can win on both cost and peak in controlled shapes. The paper
also reports cases where lower peaks do not mean fewer late releases.

## Concern-To-Evidence Map

| Concern | Revision And Evidence | Remaining Limit |
| --- | --- | --- |
| C1: format, anonymity and register | Removed historical timing campaigns and inverse/chain results from the active reading path; reorganized current claims around the new study. | Full technical report retained by user preference. Select venue, shorten, template and anonymize before submission. |
| C2: burst reduction too small to matter | Actual Rack graphs and demanding complete-module settings; budget fractions in abstract/results; aligned/staggered comparisons and release-miss counts. | No consistent reliability ranking, no calibrated saturation threshold, no audio device. |
| C3: novelty/positioning | Contribution delta table; whole analysis tail, cadence, lifetime and ownership emphasized; Gardner, Garcia and Wefers/Vorlaender references verified and added. | No claim to invent transform suspension or cooperative chains. |
| C4: weak native glue/multichannel controls | Native-layout batch/hybrid pairs, contiguous preparation, batched vDSP/FFTW four-channel plans; matched controls and full process scatter. | PFFFT ordered output remains; native leaf subdivision and further provider tuning are unmeasured. |
| C5: phase-aliased p99 and hidden exceedances | Full-hop peak distributions from original sample coordinates; p99/maxima remain secondary; compute and release misses separated. | Short pilots do not resolve rare tails. Callbacks/hops are not independent replicas. |
| C6: credit vs time bound, tuned weights | Conditional cost bound with the boundary carry term required by first-credit execution; weight-tuning disclosure; band-cache limitation. | No measured upper bound on operation cost, new weight sensitivity or stage-cost calibration. |
| C7: inverse/chain distraction | Removed from current experiment and results; retained historical artifacts separately; future synthesis requires its own contract. | No new inverse or long-filter performance claim. |
| C8: measured core differs from shipped modules | Shipped-default module paths, four-lane processing, polyphony/control variations, largest FFT and 5 ms hop; actual engine controls. | Hardest paced module shapes have no matched native module controls. |
| C9: unpaced/FPU/host-state confounds | Separate paced module/engine groups, absolute releases, raw wake/start/finish, Rack FPU policy and flagged decay tail; quiet-host declarations and power/sleep evidence. | No matched cross-over for all boundaries, core-placement calibration or thread-policy sweep; residual OS activity remains. |
| Spectrum age treated as fixed | Native full/half/quarter horizons and direct consumer-age bounds. | No perceptual threshold or repaint measurement; butterfly horizon unchanged. |
| Obvious alternatives omitted | Staggering measured; worker/UI choices tied to delivery requirements in introduction/discussion. | No asynchronous capture/queue/worker measurements. |
| Reproducibility | Original handbacks validated, exact measured sources and compact process/session records retained, checked deterministic generator, full-grid CSVs. | Public raw-data deposition still needed; no new public upload authorized. |
| Test-count and numeric precision issues | Removed stale historical counts from active manuscript; report rounded values, counts and exact evidence scope. | Numerical tests do not establish a speedup, perceptual quality or broad real-time safety. |

## Interpretation Changes That Matter

The original runner's engine hop diagnostic stopped at publication. For an
immediate batch path that is shorter than a full hop. The manuscript importer
instead uses the entire endpoint-to-next-endpoint interval for both immediate
and delayed paths. This is a symmetric re-analysis of retained raw observations,
with partial-edge and shared-block rules, not a timing change.

The empty four-thread Rack engine consumes about three cores' worth of CPU
during paced collection. That is consistent with spinning workers and makes
one-thread/four-thread results a combined scheduling/activity-regime contrast.
The paper therefore treats the barrier explanation as a hypothesis supported
by source structure, not a causally isolated result.

Both new sessions were separately prepared but occurred on the same local day.
They remain a descriptive pilot study. They are not relabeled as three-session
confirmations or assigned confidence intervals using thousands of callbacks as
independent samples. The operator's careful setup is recorded alongside the
remaining service activity, without inventing a cause for any outlier.

## Highest-Value Work Before Broad Submission Claims

1.  Freeze the main complete-host contrasts and collect additional independent
    days/processes, with process calibration and matched continuous/paced
    boundaries. Use the pilots to set duration before running them.
2.  Add paced native complete-module controls at the extreme configuration and
    a background sweep that actually brackets saturation. Preserve all attempts.
3.  Characterize the pinned engine's idle-worker behavior and compare real
    audio callbacks, reporting block compute, wake delay, CPU and device
    underruns separately. Add a second platform if making portability claims.
4.  Test native sub-FFT leaves and weight sensitivity before claiming that the
    current suspension granularity is optimal. Compare asynchronous analysis
    only with equivalent capture, history and ownership semantics.

The full revision can support a concrete scheduling/headroom contribution now.
A claim of improved audio reliability or generally optimal granularity would
require the additional evidence above.

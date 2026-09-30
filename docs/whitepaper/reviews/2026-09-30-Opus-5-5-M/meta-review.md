# Meta-Review

**Paper:** "Resumable FFT Scheduling for Real-Time Spectral Analysis" (`docs/whitepaper/fourier.tex`, manuscript version 3, 38-page PDF)
**Target venue:** DAFx
**Inputs:** three independent reviews in this folder:

| ID | File | Perspective | Recommendation | Confidence |
|---|---|---|---|---|
| R1 | `reviewer-1-dsp-theory.md` | Academic DSP / FFT / partitioned convolution; skeptical of novelty | Weak Reject | 4/5 |
| R2 | `reviewer-2-performance-methodology.md` | Real-time systems and benchmarking methodology | Weak Reject | 4/5 |
| R3 | `reviewer-3-practitioner-clarity.md` | Industry plugin developer; readability and practical use | Weak Reject | 4/5 |

## 1. Meta-recommendation

**Reject in current form; encourage resubmission.** The three reviewers reached Weak Reject independently. None of them found a correctness error in the scheduler, its derivations, or the published numbers. Every reviewer also described a concrete path to Weak Accept or Accept.

The rejection rests on three independent problems:

1. **Format.** The manuscript is not a DAFx paper yet.
2. **Baselines.** The headline comparison uses a baseline that performance-literate reviewers will reject.
3. **Significance.** The evaluation never shows a regime where the reduced burst matters.

The first is a writing task. The second and third need new measurements, and those measurements may change the headline numbers the rewrite is built on. **Do the measurements before rewriting the abstract.**

### Score summary

| Criterion | R1 | R2 | R3 | Consensus |
|---|---|---|---|---|
| Originality | 2 | 2 | 2 | 2 |
| Technical soundness | 3 | 3 | 3 | 3 |
| Significance (R3: practical significance) | 2 | 2 | 2 | 2 |
| Clarity | 2 | 3 | 2 | 2 |
| Reproducibility | 4 | 4 | 4 | 4 |
| Experimental rigor | — | 3 (bookkeeping 5, external validity 2) | — | 3 |
| Relevance / fit to DAFx | 3 | — | 3 | 3 (topic fits; format does not) |

## 2. What the reviewers agree is strong

Every reviewer called out these points. Keep them intact through the revision.

- **Correctness of the contract.** R1 rederived the quota and D-window bound, the retention bound, the publication time and the real-FFT reconstruction, and all match `src/dsp/spectrum_analysis.hpp`. R3 independently verified W, the worked credit counts, latching, cancellation and publication timing.
- **Numbers reproduce exactly.** R2 recomputed every headline macro from `data/comparison-012/tables/primary/process-timings.csv`, and all match. A hierarchical bootstrap gives narrow intervals: cost ratio 2.27 [2.14, 2.47], p99 ratio 2.78 [2.72, 2.80]. Within-host sampling noise is not the problem.
- **Real-time hygiene.** R2 found no allocation, locking, logging or syscalls on the steady-state analyzer path. Both R2 and R3 judged the display mailbox a correct SPSC triple buffer, and R3 ran it clean under ThreadSanitizer.
- **Honesty.** All three praise the unfavorable results the paper reports: aggregate cost, the inverse and chain workloads, staggered instances, and live-cache changes. They also praise the explicit p99 ≠ max ≠ WCET caveats.
- **Matched controls.** The PFFFT scheduled-batch vs hybrid pair, which separates *where* work runs from *which* FFT kernel is used, is singled out as better than typical "ours vs FFTW" designs. R1 and R3 both suggest the hybrid result is the most useful practical finding.
- **Artifact engineering.** Frozen designs, generated macros with hashes, check mode, and all-bin numerical audits are all praised.
- **The legacy diagnosis is a good hook.** The completion-driven cadence error (for example, a 9% fast frame rate at N=4096, H=2048), the EMA time-constant bias, and the O(N) boundary passes are all things analyzer authors should hear.

## 3. Consensus concerns (raised by all three)

### C1. Venue fit: length, format and anonymity

- **Length and format.** The PDF is 38 single-column pages: about 18 pages of body, 3 of references and 17 of appendices. DAFx allows 8 two-column pages in the DAFx template.
- **Anonymity.** Review is double-blind, and the manuscript identifies the author in at least five places: author block, email, "Technical report, manuscript version 3", repository URL and commit hash, module names, and `CITATION` text.
- **Codex sentence.** `sections/evaluation-method.tex:78-79` names Codex as a running confound. All three flag it, as an anonymity leak and a measurement confound.
- **Register.** R1 and R3 both count heavy hedging (about 82–99 negations in about 7,000 body words). R3 lists campaign-logistics sentences that belong in an artifact README.

R1 and R3 each supply an 8-page outline and a cut list; they largely agree (see §6).

**Venue note:** the reviewers checked the **DAFx26** call (8 pages max, template v3, `blind` option). DAFx26 has already taken place, so confirm the DAFx27 call before porting to the template.

### C2. Significance: the burst being removed is negligible at the evaluated settings

At the headline point, p99 falls from 9.50 to 3.42 µs in a 1333 µs callback: from 0.71% to 0.26% of the budget. That improvement costs 2.27× the aggregate CPU and 21.3 ms of extra spectrum age.

- R1 shows that the worst native p99 anywhere in the primary grid is 12.1% of budget, at N=16384 and D=16.
- The paper admits this in the Discussion (`sections/engineering-limits.tex:12-17`), not in the Results or the abstract. R3 says readers will feel misled when they reach it.

Every reviewer asks for **one experiment showing a regime where the burst matters**. The candidates differ by reviewer but are compatible:

- **R1:** a utilization sweep with deadline-exceedance rate as the outcome, and/or a low-power second host.
- **R2:** a paced callback mode, a headless Rack `Engine::stepBlock` harness, and an x86 replication.
- **R3:** a per-sample barrier harness or real Rack session, and many aligned instances at realistic block sizes.

**The strongest new argument came out of the reviews, not the paper.** R2 and R3 both checked Rack v2's `src/engine/Engine.cpp`:

- Rack calls `process()` once per frame.
- With more than one engine thread, workers synchronize at a barrier *every sample*.
- A batch FFT in one module's `process()` therefore stalls every thread for that sample. The per-sample period is 20.8 µs at 48 kHz, so a 9.5 µs burst is 46% of one sample period.
- Distributed work, by contrast, load-balances across threads.

This may be the paper's real reason to exist. It is currently absent: `grep -i barrier` over the sources finds nothing. Confirm it against the Rack SDK revision you build with.

### C3. Novelty and positioning

The paper concedes that time-distributed FFTs (Hurchalla 2010), cooperative chain scheduling with native leaf FFTs (Battenberg & Avižienis 2011) and FFT subdivision (Wefers 2015) precede it, and then positions itself by disclaimer.

- **R1** asks for a *delta table* against that prior work and names three defensible novelties:
  - the analysis-specific tail stages scheduled inside one bounded sequence;
  - exact-hop publication decoupled from completion, with the cadence and EMA bias analysis;
  - the placement-vs-kernel experimental design and the hybrid finding.
- **R3** frames the same point as a practitioner takeaway: "schedule the *whole* pipeline, not just the FFT".
- **R2** does not dispute the originality score.

All three want Gardner 1995 cited; R1 also wants García 2002 and Wefers & Vorländer 2011.

## 4. Concerns raised by two reviewers (strong signal)

### C4. The "optimized native" baselines use scalar glue code *(R2, R3; R1 on multichannel)*

This is the most damaging technical finding, and it is independently confirmed.

**What the adapters do.** `benchmark/paper/external.hpp:44-68` and `vdsp.hpp:119-133` wrap one library FFT call in five scalar O(N) passes:
- a ring unwrap with an integer `%` per element;
- a scalar deinterleave (instead of `vDSP_ctoz`);
- a scalar rescale;
- `std::abs(std::complex)` for magnitudes, which is `hypot` in libc++;
- a second copy loop.

**R2's experiment** (on an Apple M5 Max, not the paper's M1 Pro, so compare ratios only):

| Adapter | ns/sample | Callback p99 |
|---|---|---|
| Paper-style vDSP | 8.8–10.2 | 6.9–7.8 µs |
| Idiomatic vDSP (`vDSP_vmul`, `ctoz`, `fft_zrip`, `zvabs`) | 2.8–3.4 | 2.29–2.83 µs |
| Production core | 20.7–20.9 | 2.29–2.33 µs |

About 88% of the paper-style vDSP FFT callback is glue.

**Consequences.**
- The 2.78× p99 advantage **largely disappears** in the hot, unpaced regime.
- The cost penalty grows from 2.27× to **about 6–7×**.
- The paper's own data corroborate this: PFFFT scheduled-batch reaches 32.5 µs p99 while PFFFT native reaches 9.6 µs for the same kernel.

**Multichannel.** R1, R2 and R3 all note that the four-channel native baselines in Table 7 are four aligned, unbatched adapters. There are no batched (`vDSP_fftm_zrip`, FFTW `plan_many`) or staggered variants. That weakens the SIMD result, which R3 thinks should headline the abstract.

**R3 on fairness.** The core also uses `abs(std::complex)` (`spectrum_analysis.hpp:181`), so both sides pay for `hypot`.

### C5. p99 over all callbacks is a phase-aliased metric *(R1, R2; R3 touches on it)*

Whether p99 "sees" a once-per-hop burst depends on H/D:
- D=64, H=1024: 6.25% of callbacks contain the burst.
- D=16: 1.56%.
- D=1: p99 misses the burst entirely, as the paper itself documents.

R1 and R2 both recommend the **per-hop peak callback** (median and p95/p99 across hops) as the primary burst metric. It is a re-analysis of existing raw data, not a rerun. R2 confirmed that at the headline cell the per-hop peak median tracks p99 closely, so the headline survives, but only by coincidence of H/D.

**Maxima.** R1 found that the core's max exceeds vDSP's in 7 of 9 primary cells (e.g. 2775 vs 141 µs) and calls it under-reported. R2 supplies the likely mechanism: the core is busy about 2.3× longer, so it has proportionally more exposure to preemption and interrupts. Report both the pattern and the mechanism.

**Exceedance counts.** R2 found the harness already computes budget-exceedance counts that the paper never reports: 53,076 exceedances across 55 processes. They include deterministic every-hop misses by the scalar batch overlap-save and inverse controls at N=16384, D=16. R2 calls these "the strongest argument for distribution in the paper, and they are hidden."

### C6. The credit bound is not a time bound, and the weights were tuned on the evaluation host *(R1, R2; R3 asks how they were chosen)*

- **Tuning disclosure.** `specs/archive/010-production-cache-scheduling.md` shows p=4 and o=2 were chosen after measurements on the same M1 Pro. R1 found that o=1 raised Fourier-module p99 from 17.3 to 23.5 µs. The paper does not disclose this.
- **Unweighted band-cache rebuild.** It runs inside output units without extra weight (`spectrum_analysis.hpp:201`), while the window rebuild gets weight 4. All three reviewers flag this. R1 links it to the live-cache case where the hybrid beats the core.
- **Imbalance within a hop.** R2's per-phase profile shows the core's callbacks vary 2.2× across phases within a hop. The p99 is set by the reconstruction and magnitude phases, so a time-balanced schedule would be about 35% lower.
- **R1's constructive framing.** Add a time bound under a cost model, `c_0 + κ⌈DW/H⌉` with κ = maxᵢ ĉᵢ/wᵢ, which turns weight choice into a design rule (wᵢ ∝ ĉᵢ). Then **validate the credit model against the data**. The core runs at 2.0–3.9 ns per credit at p99, with p99/mean ≈ 1.2–1.5×, against about 9.7× for vDSP. R1 calls this the single most convincing addition available for a DSP audience.

### C7. The inverse and overlap-save section weakens the paper *(R1, R3)*

The section is about 3 pages plus Table 5. The chains:
- do not satisfy the paper's own bound;
- use complex signals and complex transforms (R1: audio is real);
- use toy FIR fixtures;
- lose to native on both cost and p99.

- **R1:** cut it; mention synthesis as future work in two sentences.
- **R3:** reframe it as a single ablation showing that butterfly-only scheduling is insufficient, keeping 3 rows for the inverse job only.

Either works; see §6.

### C8. What is benchmarked is not what ships *(R2, R3)*

- **Not the module.** The measured "production core" is `SpectrumAnalysis<float>` with a trivial sink and o=1. The shipped Fourier module uses `SpectrumAnalysis<simd::float_4, 2>` plus coordinate mapping, DC blockers and triggers. The `fourier` and `spectre` module-boundary backends already exist in `benchmark/paper/modules.hpp` and `backends.json` but were never run (R2).
- **Not the defaults.** The headline configuration (scalar, N=4096, H=1024, Hann, o=1) matches neither module's defaults. Fourier defaults to N=2048, a 30 ms hop (1440 samples), Flattop and four lanes. Spectre defaults to scalar, N=2048, H=1024 (R3).
- **Hardest regime unevaluated.** Fourier's hardest shipped setting (N=16384 with a 5 ms hop) is not evaluated (R3).

### C9. Aspects of the test regime that the paper doesn't state *(R2, with R3 on FPU mode)*

- **Unpaced callbacks.** Callbacks run back to back on a hot CPU. R2 paced them at 1.3 ms intervals, as a real 64-sample host would, and every implementation got 4–5× slower, even under a real-time thread policy. **In that paced regime the core beats idiomatic vDSP by about 1.5× in p99.** So the realistic regime may *help* the paper.
- **Slow processes.** About 1% of processes run about 3× slow, consistent with E-core placement. One such process (390 vs about 128 ns/sample) accounts for the entire hybrid N=16384 "variation" described in `comparison-results.tex:43-47`, and inflates that Figure 1 point by 22%.
- **Host state.** Host snapshots show `sandboxd` at 90% CPU and Codex/ChatGPT renderers at 8–12% during sessions. Session 03 has 34 processes with millisecond-scale stalls, against 5 and 10 in the other sessions.
- **FPU mode.** No flush-to-zero/denormals-are-zero mode is set in the harness, while Rack resets FPU flags every block (R3).

## 5. Points raised by one reviewer that the author should not miss

| Source | Point | Why it matters |
|---|---|---|
| R1 M6.2 | **Missing four-step / sub-FFT arm.** Schedule √N-sized native sub-FFTs (e.g. 32×64 for M=2048) with the same quota. | By R1's credit-model estimate it could beat the butterfly-level core on *both* cost and p99. It is the natural answer to the paper's own "placement vs kernel" question. It could become the paper's best result, or its most damaging missing baseline if a reviewer thinks of it first. |
| R1 M8 | **Spectrum age is a design choice, not a constant.** Starting frame r's work before its endpoint could cut the minimum delay to about 0.35·H at the same peak quota. | Either add a lower-bound proposition (genuine theory content) or state that H−1 follows from select-then-compute. |
| R3 M7.3 | **Completion horizon H_c ≤ H.** A one-line quota change gives a continuous burst-vs-latency knob. | Turns a binary choice into a tradeoff curve that could be the paper's central figure. Compatible with R1 M8. |
| R3 M1 | **Confront the obvious alternatives:** UI-thread FFT, a worker thread with a FIFO, and phase staggering. Table 6 shows 16 *staggered* vDSP instances beat the core (10.3 vs 28.7 µs). | Every DAFx practitioner-reviewer will ask. Gap-free exact-hop history for Spectre is probably the real justification; say so. |
| R3 M7 | **Perceptual context and a decision rule.** Relate +21 ms age and 85 ms control-response latency to display frames and AV-sync thresholds (ITU-R BT.1359, *verify*). | Replaces the circular "choose the core if you value shorter callbacks". |
| R3 M9 | **CPU-meter optics.** Rack's module CPU meter shows average cost, so a 2.27× (or 6–7×) higher average makes the module look expensive to users. | Adoption risk; one sentence. |
| R3 §5 | **Code findings:** Fourier analyzes all four lanes with unpatched inputs; Spectre's per-column mailboxes use about 6.3 MB per instance; `onReset()` publishes safely only because of the host lock (document it). | Minor but concrete. The first two are product improvements as well as paper notes. |
| R3 §5.14 | **Stale test counts.** The paper says 1,934,764 assertions in 7 cases; HEAD gives 2,180,346 in 13. TSan is not listed. | Easy fix; a reviewer who reruns tests will notice. |
| R1 minor 11 | **Distributed work is cheaper than batch** for both matched pairs (hybrid 31.4 vs 36.6; legacy incremental 43.5 vs 46.7 ns/sample). | Counterintuitive; needs one sentence of explanation or reviewers will doubt the throughput pass. |
| R2 M9 | **No pre-stated hypotheses.** The "confirmation" label implies them. | Relabel as a frozen-design descriptive study, or state a retroactive primary outcome, and add a sign-consistency count over cells. |
| R2 minor 1 | **Timer resolution.** It is 41.67 ns (24 MHz), so 3.417 µs is 82 ticks. | Round to 2–3 significant figures (R3 agrees). |
| R1 minor 1 | **Symbol overloading:** L, D, W, p, o, i/j are each used for two things. | R1 gives renames; coordinate with R3's terminology table (§6). |

## 6. Where the reviewers disagree, and my resolution

| Topic | Positions | Resolution |
|---|---|---|
| **Formalism** | R1 wants more: five propositions, including a time bound and a chains-on-chains optimality remark. R3 wants less notation and a single work bound with a worked number. | Keep R1's Propositions 1 (two-sided quota bound), 2 (time bound under a cost model) and 4 (retention) as compact statements with 2–3 line proofs. Put R3's worked numbers (W=17,410; ≤18 units per sample; ≤1,089 per 64-sample block) right after them. Mention the chains-on-chains optimum in one sentence. Delete the trivial arithmetic-order proposition. |
| **Inverse / overlap-save** | R1: cut. R3: keep as a 3-row ablation. | Take R3's option: the ablation directly supports the "whole pipeline, not butterflies" thesis at almost no page cost. Move the chain, its delay algebra and Table 5 to the supplement. |
| **Terminology** | R1 renames symbols (W→Ω, p→w_pk, o→w_out, D→B_cb). R3 renames concepts ("work unit", "one-hop scheduler", "peak block cost", "added latency", "spectrum age"). | Take R3's concept names and R1's symbol disambiguation, and add one notation table. Avoid Ω; W is fine once the twiddle factor is renamed. |
| **Headline configuration** | R3: lead with Fourier's shipped defaults (4 lanes, N=2048, 1440-sample hop). Others keep N=4096, H=1024, D=64. | Keep N=4096/D=64 as the controlled comparison, since it is the only cell with complete matched controls. Add a shipped-defaults row from the module-boundary backend (C8). |
| **Primary burst metric** | R1 and R2: per-hop peak callback. R3: cost of blocks containing a frame endpoint vs blocks that do not. | Use per-hop peak as primary, since R2 showed it is easy to compute, with p99 as secondary. R3's metric is the same idea for batch implementations and can go in the text. |
| **What proves significance** | R1: utilization sweep or low-power host. R2: pacing, Rack engine harness, x86. R3: barrier harness or Rack session. | Priority: (1) paced mode, cheap and possibly favorable; (2) headless multi-threaded Rack `stepBlock` harness, which answers the barrier hypothesis that all three would accept; (3) second platform if time allows. The utilization sweep can run inside the Rack harness (filler-module count as the load knob). |
| **Title** | R1: "Bounded Cooperative Scheduling of Windowed Spectral Analysis on the Audio Thread: Placement versus Kernel Efficiency". R3: "Whole-Pipeline Scheduling of Spectral Analysis on the Audio Thread". | Decide after the new experiments; the title should name the result that survives. Both reviewers agree "Resumable FFT" undersells the whole-pipeline point and oversells the FFT. |
| **Tone of the abstract** | R3 supplies a full rewrite that leads with a 3× reduction. R1 and R2 insist the abstract carry the budget fraction (0.26% vs 0.71%). | Both are right. Lead with the mechanism and the tradeoff, *and* state the budget fraction, or better, the regime where it matters once C2 is measured. Do not reuse R3's draft numbers until the C4 baselines are rerun. |

## 7. Consolidated revision plan

The order matters. **Phase A can change the headline numbers**, so do it before any prose rewrite. IDs in brackets point back to each review's checklist.

### Phase A: evidence that could change the story (do first)

1. **Idiomatic native adapters** [R2 MF1, R3 #12, R1 R6]. Add vectorized vDSP, PFFFT (unordered) and FFTW adapters as *new* backends; keep the current ones labeled "scalar-glue adapter". Add batched and staggered four-channel native rows. Rerun the primary N ∈ {2048, 4096, 16384} × D ∈ {16, 64, 256} cells, 3 sessions × 3 processes.
2. **Paced callback mode and core-placement control** [R2 MF3, MF4].
   - Pace callbacks with absolute-deadline waits at D/fs, optionally touching a working set between callbacks.
   - Run under default QoS and a time-constraint policy.
   - Record a per-process calibration spin and flag processes that ran slow.
   - Rerun Table 4's row set.
3. **Module-boundary runs** [R2 MF2, R3 #16]. Run the existing `fourier` and `spectre` backends at the shipped defaults, plus Fourier at N=16384 with a 5 ms hop.
4. **Significance experiment** [R1 R5, R2 SF2, R3 #13]. Build a headless Rack engine harness: `threadCount` ∈ {1, 4}, filler modules plus 1/4/16 analyzers, paced `stepBlock(64)`. Report block wall time, per-hop peak, and exceedances at fractional budgets.
5. **Re-analysis of existing raw data (no reruns)** [R1 R3/R4, R2 MF5, SF4]:
   - per-hop peak distributions;
   - p99.9 and maxima with the exposure explanation;
   - exceedance counts, including the deterministic D=16 batch misses;
   - a per-phase profile of the core;
   - credit-model validation (ns/credit, p99/mean ratios);
   - correct the hybrid N=16384 text and use per-process points in Figure 1.
6. **Optional, high value:** a four-step sub-FFT scheduled arm [R1 R10] and/or an H_c completion-horizon sweep [R3 #22]. Either could supply the paper's strongest result.

After Phase A, **decide what the paper's main claim is.** Plausible outcomes:
- **(a)** The core wins in paced and multi-threaded Rack regimes: lead with the barrier story.
- **(b)** The hybrid or four-step arm dominates: re-center on "placement vs kernel granularity", as R1 suggests.
- **(c)** Idiomatic batch wins everywhere measured: reframe as a short paper on whole-pipeline accounting and exact cadence, with an honest decision rule.

### Phase B: rewrite for DAFx (after the claim is settled)

7. **Port and anonymize** [R1 R1, R3 #1–3].
   - Port to the current DAFx template with the `blind` option.
   - Remove the author block, "Technical report, manuscript version 3", the repository URL, the commit hash and module names.
   - Delete or rephrase the Codex sentence (R1 R9).
   - Move all appendices to an anonymized supplement.
8. **Restructure to 8 pages** using the R1 and R3 outlines, which largely agree:
   - Intro: 0.5–0.7 page;
   - Related work with a delta table: 0.45–0.7;
   - **Host model** (Rack per-sample `process()`, barriers, budgets; why not UI/worker thread or staggering): 0.6;
   - Scheduler (propositions, worked numbers, **timeline figure**, dispatch listing): 1.4–1.6;
   - Method: 0.8;
   - Results (per-hop peak, tradeoff/Pareto figure, where-it-matters figure, SIMD, one butterfly-only ablation): 2.3;
   - Discussion with **decision rule**, perception and a single threats-to-validity paragraph: 0.5–0.7;
   - Conclusion: 0.2;
   - References: 0.6.

   Cut the historical evidence section, the transitions and numerics tables, the storage probe and the amplitude-calibration paragraph to the supplement.
9. **Abstract and contributions** [R3 #4–5, R1 R7, R2 MF7]:
   - a bulleted contributions list;
   - one results sentence in the intro;
   - the budget fraction or regime in the abstract;
   - drop "we derive … ownership".
10. **Disclose weight tuning**, and add a small weight-sensitivity table or cost-derived weights. Weight the band-cache rebuild or disclose it [R1 R8].
11. **Terminology and notation cleanup** per §6; consolidate hedges into one threats-to-validity paragraph [R3 #10–11, R1 R13].
12. **Figures:**
    - schedule timeline (R3 M6.1);
    - cost vs peak Pareto plot (R3 M6.2);
    - decision map or exceedance vs load (R3 M6.3 / R1 M3);
    - credit-model validation (R1 M4c);
    - optional per-stage cost breakdown (R3 M2).

    Target about 3 figures and 3 tables.
13. **Related work additions** (all marked *verify* by the reviewers; check each before citing):
    - FFT convolution: Gardner 1995, García 2002, Wefers & Vorländer 2011, Stockham 1966;
    - four-step and split-radix FFTs: Bailey 1990, Van Loan 1992, Duhamel & Hollmann 1984;
    - benchmarking methodology: Georges et al. 2007, Hoefler & Belli 2015;
    - real-time audio programming: Bencina 2011;
    - the Rack `Engine.cpp` source.

    R1 could not verify `balasubramaniam2026`; check it.

### Phase C: housekeeping

14. Update the test counts (2,180,346 assertions / 13 cases) and add TSan [R3 #19].
15. Round µs values to 2–3 significant figures; state the 41.67 ns timer tick [R2 NH1, R3 #18].
16. Explain why distributed execution is cheaper than batch in both matched pairs [R1 R12].
17. State compiler flags vs Rack's plugin build, the FPU mode, and the thread and QoS used [R1 minor 16, R2 minor 13, R3 §5.12].
18. Deposit the raw bundles with a DOI; name the Rack SDK version and hash [R2 SF6, R3 #21].
19. Convert the bibliography to BibTeX in the DAFx style; cite current vDSP docs [R3 #20].

## 8. Caveats on the reviews themselves

- **R2's timing experiments ran on an Apple M5 Max**, not the paper's M1 Pro. Treat the idiomatic-baseline and pacing results as strong hypotheses to reproduce on the M1 Pro, not as final numbers. The direction is corroborated by the paper's own PFFFT native vs scheduled-batch gap.
- **R1 did not run anything**; its cross-checks are static readings of the code, the CSVs and the specs. The credit-model figures (ns/credit, p99/mean) are computed from committed tables and should be regenerated by the tooling.
- **Rack engine claims** come from Rack v2's `Engine.cpp` on GitHub, including the per-frame barrier, `onSampleRateChange` under the engine mutex, `resetFpuFlags` per block, and the default thread count. Verify them against the exact SDK revision before building an argument on them.
- **Citations suggested by the reviewers** are flagged *verify* where uncertain. Check every one before adding it.
- **Reviewer scripts** for R2 and R3 were written to the session scratchpad (`r2-work/`, `r3-work/`) and may be deleted when the session's temporary directory is cleaned. The reviews describe each experiment precisely enough to rebuild it in `benchmark/paper/`.

## 9. Bottom line for the author

The engineering and the artifact are stronger than most DAFx submissions. The paper currently argues a result that its own Discussion undercuts, measured against a baseline reviewers will call a straw man, in a format DAFx cannot accept.

The reviewers' own experiments suggest a better paper is available: realistic pacing and Rack's per-sample thread barrier may favor whole-pipeline scheduling more than the current hot-loop numbers do. Run Phase A first and let the results choose the headline. Then write the 8-page paper around that headline, the mechanism figure and a decision rule. If Phase A comes back favorable, all three reviewers indicated they would move to Weak Accept or Accept.

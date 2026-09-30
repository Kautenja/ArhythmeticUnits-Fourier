# DAFx Review: Reviewer 1 (FFT and STFT theory, partitioned convolution)

**Paper:** "Resumable FFT Scheduling for Real-Time Spectral Analysis" (manuscript v3, `docs/whitepaper/fourier.tex`; compiled PDF is 38 pages: body pp. 1–18, references pp. 19–20, appendices pp. 21–38)

**Reviewer profile:** Senior DSP researcher and long-time DAFx PC member. I work on FFT algorithms (Cooley–Tukey variants, split-radix, real-FFT packing), STFT calibration, and uniformly and non-uniformly partitioned and time-distributed convolution.
**Confidence:** 4/5. I know the theory and the related work well. I checked the artifact myself: the code, the CSV data, and the specs.

## 1. Recommendation and scores

**Overall: Weak Reject for DAFx in its current form.** A focused 8-page rewrite would reach Borderline or Weak Accept, provided it (a) formalizes the contract properly, (b) adds one missing baseline, and (c) shows a regime where the burst reduction matters.

| Criterion | Score (1–5) | One-line justification |
|---|---|---|
| Originality | 2 | The paper itself concedes that time-distributing the FFT and the whole chain is prior art (Hurchalla 2010; Battenberg & Avižienis 2011; Wefers 2015). What remains is an implementation contract for one analyzer. |
| Technical soundness | 3 | The derivations I checked are correct (Eqs. 3–5, 8–10, retention bound, reconstruction). But the "contract" bounds credits, not time. Weights were tuned on the same host that was used for evaluation. The native multichannel baselines are not the strongest available ones. |
| Significance to DAFx audience | 2 | In the headline case, the p99 reduction is 9.5 → 3.4 µs in a 1333 µs budget (0.71% → 0.26%), bought with 2.27× aggregate cost and +21 ms spectrum age. No measured regime comes close to a deadline. |
| Clarity | 2 | The paper reads as an audit or technical report: 82 uses of "not" in ~7,000 body words, and results are split across historical campaigns. Notation is overloaded (L, D, W, p, o, i/j). |
| Reproducibility | 4 | Unusually strong provenance, frozen designs, and all-bin numerical audits. Raw bundles are local only (App. K), and the manuscript is de-anonymizing. |
| Relevance to DAFx | 3 | Real-time spectral analysis inside an audio host is on-topic. The paper would fit better as a short paper or as a stronger long paper with a device-level study. |

**Format fit:** the DAFx26 CfP allows **8 pages maximum**, in the DAFx two-column LaTeX template, with double-blind review. The body alone is ~7,050 words, with 9 tables and 1 figure, in 11-pt single-column format on 18 pages. Appendices add another ~7,400 words. Roughly **55–60% of the body must be cut**, and every appendix must be removed or moved to anonymized supplementary material (see §8, R1).

---

## 2. Summary in my own words

The author maintains a VCV Rack plugin with two visualizers: a spectrum analyzer (four independent SIMD channels) and a spectrogram (scalar). Both run analysis on the audio engine thread. The original code spread radix-2 butterflies over sample calls with quota `q = ceil(B/H)`. That had two defects:
1. The frame clock was completion-driven, so the actual hop was `L = ceil(B/q) ≠ H`. For example, 1878 instead of 2048 for N=4096, about 9% fast.
2. Packing, bit reversal, real-spectrum reconstruction, and display passes stayed as O(N) bursts at the frame boundaries.

The replacement ("production core", `src/dsp/spectrum_analysis.hpp`) linearizes the whole analysis into one ordered sequence of W credits:
- fused window + pack + bit-reverse pairs (weight p = 4 when the window cache is dirty, 1 when clean)
- B = (M/2)·log2 M radix-2 butterflies
- K = M+1 positive-bin reconstructions with a magnitude prefix sum
- K output units (weight o = 2 for the four-channel coordinate mapping)

It spends them with the Bresenham-style balanced quota `q_s = floor(sW/H) − floor((s−1)W/H)`. Each frame is published exactly H−1 calls after its endpoint through an SPSC triple buffer. Input retention needs a ring of `N_max + H_max`.

The evaluation is a single-host (M1 Pro) confirmation campaign: 1,299 workloads, 3 sessions × 3 processes. It compares the core against a legacy batch/incremental pair, a PFFFT "scheduled batch vs. hybrid" pair (native FFT kept indivisible, surrounding stages scheduled), and native PFFFT/FFTW/vDSP.
- **Headline:** callback p99 falls 2.78× (9.50 → 3.42 µs), while aggregate cost rises 2.27× (15.38 → 34.98 ns/sample) and publication is delayed by 21.3 ms.
- **Inverse jobs and overlap-save chains:** full complex transforms, with an explicitly unbounded O(N) buffering step. They are included as counterexamples: native batch inverse jobs beat the incremental control on both cost and p99.

**Does the abstract misrepresent anything?** Not factually, but it misleads by omission in three ways:
- It presents the 3.4 vs. 9.5 µs p99 as the headline without saying that both are under 1% of the 1333 µs callback budget (`sections/engineering-limits.tex:12-17`). A DAFx reader will read "p99 reduced 2.8×" as practically meaningful. The paper's own discussion says it is not.
- "we derive … ownership requirements" (`sections/abstract.tex:8-9`) overstates. The ownership part is a description of a standard triple buffer, not a derivation.
- "including inverse jobs and complete overlap-save chains" suggests the contract extends to them. It explicitly does not (`sections/inverse-and-chain.tex:7-10, 77-81`).

---

## 3. Strengths

1. **Honest scoping.** The paper states that transform suspension and chain scheduling are not new (`sections/related-work.tex:17-18, 36-37`). It reports unfavorable results: native inverse jobs dominate, the hybrid beats the core under live-cache changes, and staggered native analyses beat the core at 16 instances. Few submissions are this candid.
2. **Correct and well-motivated diagnosis of the legacy defect.** The distinction between completing within H and completing at H is right. The cadence derivation checks out: N=4096, H=2048 gives B=11264, q=6, L=1878, H/L=1.0905; Table 10 matches. The smoothing time-constant bias `τ_eff = τL/H` (App. B.2) is a real, practical bug that analyzer designers overlook.
3. **Complete-pipeline accounting.** Fusing window, pack, and bit reversal into per-pair units (`spectrum_analysis.hpp:107-116`), per-bin reconstruction (`:166-183`), and prefix-sum smoothing that respects the "all prefix before any range" dependency is the right engineering answer to the O(N) boundary problem. I verified Eq. (14) against `reconstruct()`: `X[k] = ½(A + D − jW^k(A−D))` is implemented exactly.
4. **Matched controls.** The PFFFT scheduled-batch/hybrid pair (`benchmark/paper/hybrid.hpp`) separates work placement from kernel choice. This is methodologically better than typical "our scheduler vs. FFTW" comparisons, and the hybrid result is arguably the most useful finding for practitioners.
5. **Numerical auditing.** All published bins in 11.7 M float and 1.0 M double spectra were checked against an independent binary64 reference. Norm budgets were frozen before confirmation.
6. **Measurement hygiene.** The paper explicitly says p99 can miss bursts, that maxima are not WCET, and that empty-timer intervals are not subtracted. It cites Kalibera & Jones and Mytkowicz et al.

---

## 4. Major issues (prioritized)

### M1. The contribution is too thin relative to prior art, and positioning is argued by disclaimer, not by a delta

**Where:** `sections/related-work.tex:3-45` (PDF pp. 2–3); `sections/introduction.tex:25-27, 40-46`; `appendices/related-work.tex:24-59`.

**Problem.** The paper concedes that time-distributed FFTs (Hurchalla 2010), whole-chain cooperative scheduling with optimized leaf FFTs (Battenberg & Avižienis 2011), manual preemption and FFT subdivision (Wefers 2015), and TD-FFT/IFFT (Liu et al. 2017) all precede it. What remains is "an implementation contract and an empirical study" (`related-work.tex:32-33`). The contract's elements are:
- a Bresenham quota, which the paper itself calls "elementary" (`production-successor.tex:41-42`; `original-scheduling.tex:91`)
- a ring buffer of size N+H
- a triple buffer
- frame-latched settings

Each is textbook. A DAFx PC member will ask: what can a reader do after this paper that they could not do after Battenberg & Avižienis §4 plus Wefers Ch. 6? The paper never states that delta in one sentence.

**Why it matters:** this is the most likely rejection reason: "incremental engineering report on a known technique."

**Fix.** Rewrite the positioning as a **delta table**. Rows: Hurchalla 2010, Battenberg & Avižienis 2011, Wefers 2015, Liu et al. 2017, Prusa & Holighaus 2016, this paper. Columns:
1. workload (convolution / analysis)
2. which stages are scheduled (FFT / product / IFFT / pack / reconstruct / post-processing)
3. granularity (butterfly / stage / sub-FFT / partition)
4. clock (completion-driven / exact hop)
5. added latency
6. input-retention analysis given?
7. per-block bound stated?
8. measured against optimized batch?

Then claim only the cells that are new. From my reading, the defensible novelties are:
- **(i)** the analysis-specific tail stages (reconstruction, fractional-octave prefix smoothing, EMA, display mapping) scheduled inside the same bounded sequence;
- **(ii)** exact-hop publication decoupled from arithmetic completion, and the cadence/EMA-bias analysis of the completion-driven alternative;
- **(iii)** the matched placement-vs-kernel experimental design, and the empirical finding that a hybrid (indivisible native FFT, scheduled surroundings) captures most of the burst reduction at native-ish cost.

Item (iii) is, in my view, the strongest message for DAFx. Consider re-centering the paper on it, e.g. "How fine must cooperative spectral analysis be? Placement vs. kernel efficiency in engine-thread analyzers."

Also add the missing classics and a closer comparator (see M8). Reviewers from the convolution community will expect Gardner 1995 and García 2002 in any paper that discusses spreading FFT work against latency.

### M2. The "contract" bounds credits, not time; the formalism is incomplete where it matters and redundant where it does not

**Where:** `sections/production-successor.tex:11-58` (PDF pp. 4–5, Eqs. 3–4); `appendices/one-hop.tex:29-57`; `appendices/transform.tex:30-45`; `tables/production-work-units.tex`.

**Problem.** The central result, "at most ⌈W/H⌉ credits per call" plus "any D consecutive calls consume ≤ ⌈DW/H⌉", is correct. I rederived it: with the accumulator reset at `phase==0` (`spectrum_analysis.hpp:300`), cumulative credits at global call t are `rW + floor(sW/H) = floor(tW/H)` for constant W. So telescoping across frame boundaries is valid. But a credit is not a unit of time.
- The weights p ∈ {1,4} and o ∈ {1,2} are **ad hoc**. `specs/archive/010-production-cache-scheduling.md:77-84` shows o=2 was chosen after a measured 36% p99 regression in the Fourier module on this same M1 host. The paper does not disclose that the weights were tuned on the evaluation host (see M5).
- The **band-cache rebuild is unweighted**. `prepare_band()` runs inside `output_bin()` when `bands_dirty` (`spectrum_analysis.hpp:201`) with no extra credit. The window rebuild does get weight 4. This is consistent with the one case where the core loses to the hybrid (Table 6, "Live cache/settings": 17.12 vs. 13.92 µs).

Meanwhile, the only formal propositions are:
- (a) arithmetic-order preservation (App. A, Prop. 1), which is the determinism of a sequential program and needs no proof environment in a DAFx paper;
- (b) the completion bound of the *legacy* scheduler (App. B, Prop. 2);
- (c) a one-line quota proposition (App. D).

The results a DSP reader actually needs are not stated as theorems.

**Fix.** Replace Sec. 5.1 with a compact formal block (about ¾ column):
- **Proposition 1 (Quota).** For W > 0 and H ≥ 1, the rule in Eq. (4) satisfies:
  - (i) credits sum to W and the last credit falls on call H;
  - (ii) `max_s q_s = ⌈W/H⌉`, which is minimal among all schedules completing W unit credits in H calls (pigeonhole);
  - (iii) for any D consecutive calls, within or across frames with constant W, the credits lie in `[⌊DW/H⌋, ⌈DW/H⌉]`.

  State the two-sided version; the lower bound is what explains the flat p99.
- **Proposition 2 (Time bound under a cost model).** Let operation class i have cost `c_i ≤ ĉ_i` and weight `w_i`, with `κ = max_i ĉ_i / w_i`. Then any call's scheduled compute time is at most `c_0 + κ⌈W/H⌉`, and any D-sample block's is at most `D·c_0 + κ⌈DW/H⌉`. This makes explicit that weights should be chosen to equalize `ĉ_i / w_i`. It turns the ad hoc weights into a design rule: choose `w_i ∝ ĉ_i`.
- **Proposition 3 (Optimal weighted chain partition).** With known per-operation costs, minimizing the maximum per-call cost of a fixed-order chain split into H contiguous segments is the chains-on-chains partitioning problem. It is solvable exactly in polynomial time (Bokhari 1988; Pinar & Aykanat 2004, *verify* exact citations). This gives the reader the optimum against which the credit heuristic can be compared. Even one computed optimum per configuration, using measured per-class costs, would quantify how far the heuristic is from optimal.
- **Lemma 4 (Retention).** This is already correct in App. D. Move the two-line proof into the body. Note that capacity `N+H−1` is sufficient (max lag N+H−2 < C). The implementation's `N_max+H_max` is conservative by one sample, which is harmless but should be stated precisely.
- **Proposition 5 (Age).** Eq. (5) as is.

Delete App. A Prop. 1 and App. B Prop. 2, or reduce each to one sentence.

### M3. Significance: the measured burst reduction is practically irrelevant on the measured host, and no experiment shows a regime where it matters

**Where:** abstract; `sections/comparison-results.tex:13-24`; `sections/engineering-limits.tex:12-17` (PDF p. 17); `data/comparison-012/tables/primary/results.csv`.

**Evidence.** From `results.csv` (single-instance float analysis, primary grid), the worst native p99 as a fraction of the callback budget is:
- vDSP: 12.1% at N=16384, D=16 (40.3 µs of 333 µs)
- 3.1% at D=64
- ≤ 0.8% at D=256

The hybrid's p99 is only 5.38 µs at N=4096, D=64. That callback contains the entire indivisible PFFFT forward transform, so an optimized N=4096 real FFT is a burst of only a few µs on this machine. The paper admits this (`engineering-limits.tex:15-17`: "the measured reduction does not show that the native baseline was near its deadline"). But then the evaluation has not shown the method solves a problem.

**Why it matters:** DAFx reviewers weigh practical impact. A 2.27× cost increase and +21 ms display latency, in exchange for reducing a burst from 0.7% to 0.26% of budget, is a net loss unless a regime exists where the burst matters.

**Fix: pick one or both.**
1. **Utilization sweep (schedulability experiment).** Add background DSP load calibrated so total mean utilization is U ∈ {50, 70, 80, 90, 95}% of the D/fs budget, with the analyzer count or N chosen so the native frame burst is 5–30% of the budget. Report the **deadline-exceedance rate** (fraction of callbacks > D/fs) and the **max per hop** for core / hybrid / native, aligned and staggered. The central plot is exceedance rate vs. U. This directly tests the paper's claim and reuses the existing harness: "Background load 64" already exists (`extensions.tex`).
2. **Low-power target.** Repeat the primary cell on a Raspberry Pi 4/5 or similar Cortex-A (Rack runs on ARM64 Linux) or an x86 laptop at low clock. There, an N=16384 native FFT burst is plausibly a much larger fraction of a D=16–64 budget. One host is also a stated limitation (`engineering-limits.tex:57-60`). A second architecture addresses both issues.

If neither is feasible, the paper should be reframed as a short paper about the placement-vs-kernel tradeoff, and the abstract's headline should include the budget fraction.

### M4. The metric (p99 of all callbacks) is structurally fragile; use a hop-conditional burst statistic and validate the credit model against it

**Where:** `sections/evaluation-method.tex:123-152`; `sections/comparison-results.tex:62-69, 122-129`; Tables 4–7.

**Problem.** Whether p99 "sees" a once-per-frame burst depends on the ratio D/H relative to 1%:
- D=64, H=1024: 6.25% of callbacks contain the release, so p99 reflects the burst.
- D=16: 1.56%, borderline.
- D=1: 0.1%, so p99 misses it entirely. The paper had to explain this away twice (App. G; Sec. 8.3 single-sample).

The metric is therefore not comparable across the (N, H, D) grid, and the figure's p99 vs. N curves mix regimes. Meanwhile the **maximum** often ranks the core *worse*. In the primary CSV, the core's observed max exceeds vDSP's in 7 of 9 (N, D) cells. Examples:
- N=2048, D=256: 716 µs vs. 31 µs
- N=16384, D=256: 2775 µs vs. 141 µs
- N=2048, D=64: 632 µs vs. 117 µs

The paper mentions only one of these (`comparison-results.tex:62-64`) and attributes it to the OS. A skeptical reader will ask why interference would systematically hit the core more.

**Fix.**
- **(a)** Define the primary burst metric as the **per-hop peak callback** `P_r = max_{callbacks in hop r} t_cb`. Report its median and p95/p99 over hops. This is invariant to D/H and directly measures what the scheduler controls. The harness already records every callback, so this is a re-analysis, not a re-run.
- **(b)** Report p99.9 and a count of callbacks > 10× median alongside the max. Test whether the core's larger maxima are systematic, e.g. per-process max distributions over 9 processes × 3 sessions, with a rank test that is purely descriptive.
- **(c)** **Validate the credit model.** I computed it from your data. With `W(N) = M + B + 2K` and credits per block `⌈DW/H⌉`, core p99 over the 9 primary cells is 2.0–3.9 ns per credit. For N=4096, D=64: 1089 credits, p99 3.417 µs, so 3.14 ns/credit. Mean cost per credit from the throughput pass is 2.06 ns for N=4096 (34.98·1024/17410), 1.65 for N=16384, and 2.56 for N=2048. So **p99/mean ≈ 1.2–1.5× throughout**, while vDSP's p99/mean callback ratio at N=4096, D=64 is ≈ 9.7× (9.5 µs vs. 15.38·64 ns = 0.98 µs). This peak-to-mean ratio is the clean, dimensionless statement of the paper's result. Add a figure or table: predicted `κ⌈DW/H⌉` vs. measured per-hop peak, across N and D. It would turn the "contract" from an assertion into a validated model. It is the single most convincing addition the paper could make for a DSP audience.

### M5. Tuning and evaluation leakage; the paper does not disclose design iteration on the evaluation host

**Where:** `sections/production-successor.tex:12-16` (weights introduced without rationale); `specs/archive/010-production-cache-scheduling.md:62-84, 112-118`; `specs/archive/008-*`, `009-*`.

**Problem.** The production weights and the dense-dispatch threshold (`spectrum_analysis.hpp:309-311`: "Retain the simple loop when calls do at most two units") were chosen from benchmark campaigns on the same M1 Pro, same compiler, and overlapping workloads as the confirmation study. That is legitimate development. But the confirmation is then not an out-of-sample test of those choices, and the paper does not disclose it. Reviewers who find it will treat it as a validity threat.

**Fix.** Add one paragraph in Sec. 5 or 7:
- "Weights p=4 and o=2 and the sparse-dispatch threshold were selected in development campaigns on the same host (N=2048/H=1024 module matrix)."
- State the observed effect: "o=1 increased Fourier-module p99 from 17.3 to 23.5 µs."
- Either (a) show a small weight-sensitivity table (p ∈ {1,2,4,8}, o ∈ {1,2}) on held-out (N, H) cells, or (b) derive weights from measured per-class costs (Prop. 2 in M2) and show they land near the chosen values.

### M6. Baselines: the strongest competing designs are missing or weakened

**Where:** `generated/paper-comparison/tables/channels.tex`; `benchmark/paper/channels.hpp:32-40`; `benchmark/paper/vdsp.hpp:30,42`; `sections/comparison-results.tex:131-138`; `appendices/evaluation-agenda.tex:62-70`.

1. **Multichannel natives are unbatched and aligned.** The "Four vDSP/FFTW/PFFFT analyses" rows construct four independent adapters (`ScalarChannels`, `channels.hpp:32-38`) that all release on the same sample. They do not use batched APIs such as vDSP's multiple-signal FFTs (`vDSP_fftm_zrip`) or FFTW's `fftwf_plan_many_dft_r2c`. They are not staggered across the hop either, even though Table 6 shows staggering alone cuts native p99 from 86 to 10 µs at 16 instances. The claim that the four-lane core has lower cost *and* p99 than native banks (`comparison-results.tex:132-136`) is the paper's most interesting positive result. It is currently made against a weak baseline.

   **Fix:** add "4 native, staggered by H/4" and, where available, "4 native batched" rows. Keep the claim only if it survives.
2. **No coarse-grained optimized-kernel decomposition.** Butterfly-level suspension is the finest and slowest granularity. The classical alternative (Battenberg & Avižienis's DIF split with FFTW leaves; the four-step/six-step factorization, cf. Bailey 1990, Van Loan 1992) runs `N = N1·N2` as N2 native sub-FFTs of length N1, then twiddles, then N1 native sub-FFTs of length N2. This gives scheduling units of ~√N-point native FFTs at near-native throughput. By M4(c), a scheduler at vDSP's per-credit efficiency would put p99 near the vDSP *mean* callback (~1 µs at N=4096, D=64), below the core's 3.4 µs, at roughly half the core's cost. The paper lists this as future work (`evaluation-agenda.tex:62-67`). For a paper whose core question is "placement vs. kernel," it is **the** missing arm.

   **Fix:** implement a four-step hybrid with vDSP or PFFFT complex sub-transforms (for N=4096 real: M=2048 = 32×64). Schedule it with the same quota and report it in Table 4 and Fig. 1. If it dominates, the paper's message becomes "suspend at sub-FFT granularity, not butterflies." That is a stronger and more useful DAFx result than the current one.
3. **The scalar core is radix-2 with no intra-transform SIMD.** The comparison "resumable scalar radix-2 vs. NEON-vectorized libraries" conflates suspension with a kernel that is ~2× slower per frame, as your own isolated IFFT shows: 65.19 vs. 7.17 µs, a 9× gap. At least discuss radix-4/split-radix (about 25% fewer real multiplies than radix-2, cf. Duhamel & Hollmann 1984; Sorensen et al. 1986, *verify*). Resumability does not depend on radix; a radix-4 traversal suspends just as easily.

### M7. Section 6 (inverse jobs and overlap-save chains) weakens the paper and should be cut or redone on real signals

**Where:** `sections/inverse-and-chain.tex` (PDF pp. 6–7, Eqs. 6–10); Table 5 (p. 14); `sections/comparison-results.tex:71-99`; `sections/engineering-limits.tex:28-37`.

**Problems.**
- **(a)** By the paper's own statement, the chains do not satisfy the paper's contract: an O(N) inverse-buffer operation is charged as one unit, and ring-to-frame copy plus buffer/permutation passes run outside the quota (`inverse-and-chain.tex:77-81`).
- **(b)** They process a **complex** input stream with full N-point complex transforms (`:45-56`; `Wchain = 2Bc(N)+2N+H+1`). Audio is real. A DAFx convolution reviewer would expect real-input overlap-save through the same packed real FFT, followed by a Hermitian real IFFT. That roughly halves the transform work and reuses Sec. 5's machinery.
- **(c)** The fixtures (identity and a 3-tap FIR) are, as you say, not applications.
- **(d)** The balanced chain doubles I/O delay to 2H−2 (Eq. 10). That is the well-known one-block penalty of spreading a block's FFT over the next block. It is exactly the penalty that Gardner's and García's non-uniform partitioning, and Hurchalla's time-distributed FFT, were designed to avoid. So the section invites a direct unfavorable comparison with convolution literature the paper does not engage.

The net result, "native batch beats our incremental inverse on both axes," is a negative result about an unfinished design.

**Fix: pick one.**
- **(i) Recommended for DAFx:** cut Sec. 6, Table 5, and Sec. 10.1's third paragraph. Replace them with two sentences in the Discussion noting that the contract does not yet cover synthesis, and why (bulk buffering). Put the rest in supplementary material.
- **(ii)** Complete it: schedule buffering and conjugation per element, use real-signal overlap-save with the packed real FFT and a real IFFT, use a realistic FIR (e.g. 512–4096 taps, with N=2L), and compare against uniformly partitioned convolution (Wefers & Vorländer 2011) at matched latency. That is a different paper.

Do **not** keep the section in its current form.

### M8. Latency (age) is treated as an inherent cost; it is a design choice with an analyzable frontier

**Where:** `sections/production-successor.tex:60-75` (Eq. 5); `sections/comparison-results.tex:51-60`; `appendices/prototype-evaluation.tex:28-36`.

**Problem.** The paper adopts "select the frame at t_r, then spread all W credits over the next H calls," so `d = H−1` is baked in. But with hop H < N, N−H of frame r's samples are known one hop early. Only the operations that depend on the newest samples must run after t_r. In a radix-2 traversal, the butterflies downstream of a single input form a binary tree of ~M−1 butterflies across log2 M stages, plus reconstruction and output (~(1+o)K). For N=4096 that post-endpoint work is roughly 2K + (1+o)K ≈ 6K credits out of W ≈ 17.4K. So with the same per-call quota, the **minimum publication delay is on the order of 0.35·H rather than H−1**, if work on frame r starts before its endpoint. That requires overlapping two frames' states: more memory, and windowing must be applied per sample as it arrives, which your fused per-pair packing already permits. This is exactly the "acquisition-aware" direction of Lo & Lee and Hurchalla, which the paper dismisses in one sentence (`appendices/related-work.tex:19-22`).

**Why it matters:** "+21 ms age" is the method's second main cost. If it is avoidable, the evaluation overstates the tradeoff. If it is not, a lower bound would be a genuine theoretical contribution.

**Fix.** Either:
- **(a)** add a proposition giving the minimum post-endpoint work `W_post(N)` for the chosen traversal and ordering, and hence the delay lower bound `d_min = ⌈W_post · H / W⌉` at unchanged peak quota. Then present age vs. peak-quota as a Pareto frontier with the current design at one end and batch at the other.
- **(b)** at minimum, state explicitly that `d = H−1` is a consequence of "select-then-compute" and cite acquisition-aware TD-FFTs as the way to reduce it.

The four-hop-overlap prototype (App. H) went in the opposite direction (+3H), which suggests this axis was not explored.

### M9. Structure, length, anonymity: the manuscript is not submittable to DAFx as is

**Where:** `fourier.tex:7-8` (author, affiliation, e-mail, "Technical report, manuscript version 3"); `preamble.tex:26-27` (header "Technical report"); `appendices/reproducibility.tex:4, 27-31` (GitHub URL, commit hash); `sections/introduction.tex:38-40` (named products); `sections/evaluation-method.tex:78-79` ("Ordinary macOS services and Codex remained possible confounds").

**Problems.**
- The manuscript is 38 pages versus an 8-page limit.
- Double-blind review is violated in at least five places.
- There are three evidence layers (historical FFT campaign, pipeline prototype, confirmation) with seven appendix sections about superseded code (App. B, C, F, G, H).
- Heavy defensive hedging: 82 occurrences of "not" in the body.
- The Codex sentence tells reviewers an AI coding agent was running during measurements. Beyond anonymity, it is a measurement confound a reviewer will seize on.

**Fix:** see the R1 cut plan in §8. Remove the Codex sentence. Either re-run the primary cell with the machine quiesced, or rephrase as "interactive user-level processes (editor/agent tooling) were not stopped" and quantify with an idle-control session.

---

## 5. Minor issues

**Notation and math**
1. **Symbol overloading.**
   - `L`: legacy completion count (Sec. 4, App. B) and FIR length (Sec. 6.2).
   - `D`: callback size (Sec. 3) and `\overline{Z[M-k]}` (Eq. 14, `appendices/transform.tex:68`).
   - `W`: credits (Eq. 3) and twiddle `W_N` (Eqs. 11, 14).
   - `p`: window weight (Sec. 5.1) and `N=2^p` (App. A.1, E).
   - `o`: output weight (Eq. 3) and odd term `o` (Eq. 11).
   - `j`: imaginary unit and block index `b_r[j]` (Eq. 8).
   - `i`: imaginary unit in `appendices/implementation.tex:46` and Listing 3, but `j` elsewhere.

   Suggested renames: callback size `D → B_cb` or `P`; FIR length `L → L_g`; credit total `W → Ω` or `C_f`; window weight `p → w_pk`; output weight `o → w_out`. Use `j` everywhere.
2. Eq. (3): define `B` once as `B_r(N)` and use a single symbol. Sec. 3 introduces `B = B_r(N)`, Sec. 5.1 redefines `B=(M/2)log2 M`, and Sec. 6 uses `B_c(N)`.
3. Eq. (4) is 1-indexed, but the header comment (`spectrum_analysis.hpp:51`) is 0-indexed. That is fine, but say "s = 1, …, H counts calls since frame selection, including the selecting call."
4. Retention (`one-hop.tex:65-71`): `C = N+H−1` suffices; say "C ≥ N+H−1; we use N_max+H_max."
5. Sec. 3, amplitude calibration (`signal-model.tex:25-35`): cite Harris 1978 in the body, and add ENBW/processing-gain context (Heinzel, Rüdiger & Schilling 2002, *verify*) if PSD is mentioned. "Tabulated coherent-gain factors … should not be assumed to equal the exact finite-length sum" is a weakness to fix (compute `Σw/N` at cache rebuild; it is O(N) and already inside scheduled units), not a caveat to disclose. Alternatively, report the maximal deviation for the 15 windows at supported N.
6. "Magnitude means, not power means" (`production-successor.tex:87-88`, repeated in `one-hop.tex:96-97`, `original-pipeline.tex:39-41`): state once. For an analyzer, justify the choice. Fractional-octave smoothing of magnitudes biases toward peaks differently than power smoothing (cf. Hatziantoniou & Mourjopoulos 2000, JAES, *verify*).
7. The O(log N) claim for `⌈W/H⌉` holds under `H = ρN`. The primary grid instead fixes H=1024 while N varies, so `⌈W/H⌉ = Θ(N log N)` there. Fig. 1's caption partly says this; make it explicit in Sec. 5.

**Terminology and presentation**
8. The title says "Real-Time" and "Resumable FFT," but the paper says the contribution is neither a new FFT nor a real-time guarantee. Suggested: "Bounded Cooperative Scheduling of Windowed Spectral Analysis on the Audio Thread: Placement versus Kernel Efficiency."
9. "Sample call" vs. "callback" vs. "block" vs. "hop": define once in a small figure (timeline with t_r, H, D, publication). The existing Fig. 2 (legacy timeline) could be redrawn for the production schedule and moved to the body.
10. Table 4: add columns for **mean callback** (cost × D) and **p99/mean**. Put budget % in the caption.
11. Table 4: "PFFFT hybrid" cost (31.42) is *lower* than "PFFFT scheduled batch" (36.63), and "Legacy incremental" (43.51) is lower than "Legacy batch" (46.73). Distributing identical work reduces aggregate cost by 7–14%. This is counterintuitive (cache/prefetch? branch prediction? timer placement?) and needs a sentence of explanation or an ablation, or reviewers will doubt the throughput pass.
12. Table 6 mixes N=2048 defaults with Table 4's N=4096 without a visual cue. Add "N=2048" to the table title.
13. Fig. 1: log–log with only three x points. Consider normalizing y by `N log N` (cost) and by credits per block (p99) to show model agreement (M4c).
14. Table 8 (transitions) and Table 9 (numerical): move to supplement. Summarize numerics in one sentence: "all 12.7 M audited spectra within frozen budgets; max relative ℓ2 4.8e−7 (float), 6.0e−15 (double)."
15. Sec. 7.3, "1,299 workloads" and "11,691 processes": these totals are impressive but irrelevant to the argument. The reader needs the design of the cells actually shown.
16. `-funsafe-math-optimizations` in the confirmation versus no fast-math in the original campaign (`appendices/experimental-method.tex:8`): state whether Rack's plugin build uses the same flags. Otherwise the benchmarked binary is not the shipped one.
17. Legacy `step(H)` computes `ceilf(total/float(H))` (`src/dsp/fft.hpp:311, 451`). This is disclosed in App. B, but it belongs with the legacy material that should be cut.
18. `appendices/implementation.tex:98-102`: when the upper edge clamps to Nyquist, the band width is preserved as a ratio, so the smoothed value near Nyquist averages a wider, asymmetric band. Fine for display, but say it in one clause if kept.
19. `balasubramaniam2026` (DAFx26, "Real-time neural audio on Apple Silicon…"): I could not verify this reference. Double-check authors, title, and pages.

**Citations to add (all to be verified by the authors)**
- W. G. Gardner, "Efficient convolution without input-output delay," *J. Audio Eng. Soc.*, 43(3):127–136, 1995.
- G. García, "Optimal filter partition for efficient convolution with short input/output delay," AES 113th Convention, 2002.
- F. Wefers and M. Vorländer, "Optimal filter partitions for real-time FIR filtering using uniformly-partitioned FFT-based convolution in the frequency-domain," DAFx-11, 2011.
- T. G. Stockham Jr., "High-speed convolution and correlation," AFIPS Spring Joint Computer Conf., 1966.
- D. H. Bailey, "FFTs in external or hierarchical memory," *J. Supercomputing*, 4(1):23–35, 1990 (four-step FFT).
- C. Van Loan, *Computational Frameworks for the Fast Fourier Transform*, SIAM, 1992.
- P. Duhamel and H. Hollmann, "'Split radix' FFT algorithm," *Electronics Letters*, 20(1):14–16, 1984.
- S. H. Bokhari, "Partitioning problems in parallel, pipeline, and distributed computing," *IEEE Trans. Computers*, 37(1):48–57, 1988; A. Pinar and C. Aykanat, "Fast optimal load balancing algorithms for 1D partitioning," *J. Parallel Distrib. Comput.*, 64(8):974–996, 2004.
- For the "work-based pacing" analogy, which is exactly your credit quota: H. G. Baker, "List processing in real time on a serial computer," *CACM*, 21(4):280–294, 1978; D. F. Bacon, P. Cheng, V. T. Rajan, "A real-time garbage collector with low overhead and consistent utilization," POPL 2003. The work-based vs. time-based pacing distinction there maps directly onto your credit vs. time discussion.
- Optionally, cyclic-executive / frame-based real-time scheduling (e.g., Baker & Shaw 1989, *verify*) as the systems framing of "slice work to fit frames."

---

## 6. Code and data cross-check

| Paper claim | Location | Artifact check | Verdict |
|---|---|---|---|
| `W = pM + B + (1+o)K = (N/4)log2(N/2) + (p+1+o)N/2 + 1 + o` | Eq. 3 | `work_per_frame()` `spectrum_analysis.hpp:253-255`; butterfly loop `:280` gives (N/4)·log2(N/2) | Correct |
| Worked counts W = 8194 / 11266 / 9219 / 12291 and quota 8/9, 11/12 | `one-hop.tex:40-46` | Recomputed | Correct |
| Balanced quota via accumulator; reset at frame start | Eq. 4, Listing 2 | `:85-90, 300, 304-306` | Correct; D-window bound holds across frames for constant W |
| Weight-p units execute at the first credit | Table 1 | `:316` (`cursor % 4 == 0`), `:337-339` | Correct |
| Band-cache rebuild included in scheduled units | Sec. 5.1, `:52-53` | `prepare_band` inside `output_bin` `:201`, **unweighted** | Included but unweighted; disclose (M2) |
| Publication at `t_r + H − 1`; frame covers newest N samples incl. `t_r` | Eq. 5 | `:297-303, 372-379` | Correct |
| Ring `N_max + H_max`; no whole-frame copy | Sec. 5.2 | constructor `:219` | Correct, conservative by 1 |
| Real reconstruction Eq. 14 | App. A.2 | `reconstruct()` `:166-183` | Correct |
| O(log N) per-call constant work (plan index) | Sec. 5.1 | `configure()` loops `:280-282`, called only at `phase==0` | Correct; `configure` also does two `std::pow` calls (constant) |
| Weights are principled | Sec. 5.1 (implicit) | `specs/archive/010-…:77-84` | **Empirically tuned on the eval host**; undisclosed (M5) |
| Four native analyses baseline | Table 7 | `channels.hpp:32-38`: four independent, aligned, unbatched adapters | **Weak baseline** (M6.1) |
| Headline numbers (34.98, 15.38, 3.417, 9.50, ratios 2.27/2.78, 21.31 / 42.66 / 63.97 ms, 0.26 / 0.71%) | abstract, Sec. 8, 10 | `macros.tex`; recomputed from formulas | Consistent |
| Maxima "not attributed to scheduler" | `comparison-results.tex:62-67` | `primary/results.csv`: core max > vDSP max in 7/9 cells (e.g. 2775 vs 141 µs) | **Under-reported pattern** (M4b) |
| Credit model explains p99 | not claimed | 2.0–3.9 ns/credit across 9 cells; p99/mean ≈ 1.2–1.5 | **Supports the paper; should be added** (M4c) |
| Hybrid puts the whole native FFT in one unit | Table 2 | `hybrid.hpp:18-20, 64-65` | Correct; hybrid p99 5.38 µs implies native FFT burst of a few µs (M3) |

I did not rebuild or rerun anything. All checks are static readings of the code and the committed CSVs.

---

## 7. Questions for the authors (rebuttal)

1. In one sentence, what does a practitioner gain from this paper that Battenberg & Avižienis (2011) §4 plus Wefers (2015) do not already give them?
2. Can you show any configuration on any host where the native batch analyzer's per-hop peak exceeds, say, 25% of the callback budget while the core's does not?
3. Why do the core's observed maxima exceed vDSP's in 7 of 9 primary cells (e.g., 2775 vs. 141 µs at N=16384, D=256)? Are these clustered in particular sessions or processes?
4. Why is distributed execution cheaper in aggregate than batch for both matched pairs (hybrid 31.4 vs. scheduled batch 36.6; legacy incremental 43.5 vs. legacy batch 46.7 ns/sample)?
5. How were p=4 and o=2 chosen, and how sensitive are the results to them on (N, H) cells not used for tuning? Why is the band-cache rebuild unweighted?
6. Would a four-step decomposition with native sub-FFTs (e.g., 32×64 for M=2048), scheduled by your quota, not dominate the butterfly-level core on both cost and p99?
7. Do the four-channel native baselines change when staggered by H/4 or when using batched multi-signal APIs?
8. Is the H−1 publication delay necessary? What is the minimum delay at the same peak quota if frame r's work begins before its endpoint?
9. Why do the chains use complex input and complex N-point transforms instead of real-signal overlap-save with the packed real FFT?
10. Are the confirmation compiler flags identical to the shipped Rack plugin build?

---

## 8. Prioritized revision checklist (executable against the .tex sources)

### Must-fix for acceptance

- **R1. Restructure to DAFx 8 pages, two-column, anonymized.**
  - Switch `fourier.tex` to the DAFx26 template (`DAFx26_LaTex_v3`) with the blind option.
  - Delete the author block (`fourier.tex:7`), the date/"Technical report" line (`:8`), and header text (`preamble.tex:25-28`).
  - Anonymize product names in `sections/introduction.tex:38-40` ("a modular-synthesis plugin with a four-channel spectrum analyzer and a scalar spectrogram").
  - Remove the repository URL and commit hash (`appendices/reproducibility.tex`) and replace them with "anonymized artifact link."
  - Target structure and budget (two-column pages):
    1. Intro, 0.5 p
    2. Related work plus delta table (M1), 0.7 p
    3. Model and problem statement, with a merged timeline figure, 0.6 p
    4. Scheduler: dependency sequence, Props 1–5 (M2), retention, age, 1.4 p
    5. Method, 0.8 p
    6. Results: Table 4 extended, per-hop-peak and model-validation figure, trimmed Table 6, corrected channels table, utilization/second-host experiment, 2.3 p
    7. Discussion and limits, 0.5 p
    8. Conclusion, 0.2 p
    9. References, 0.6 p
  - **Cut from the body:** `sections/legacy-motivation.tex` (reduce to 3 sentences in the Intro, keeping the 9.1% cadence and EMA-bias example); `sections/historical-evidence.tex` (drop); `sections/inverse-and-chain.tex` and Table 5 (drop, M7); the transitions table and text (`comparison-results.tex:142-154`); the storage-probe paragraph (`:178-186`); `evaluation-method.tex:84-121` (compress to 4 sentences plus one small table).
  - **Drop all appendices** from the submission. Move App. D, E, K to anonymized supplementary material. Delete App. A (Prop. 1), B, C, F, G, H, I, J, or put them in the supplement.
- **R2. Replace Sec. 5.1's prose with Propositions 1–5 (M2)**, including the two-sided D-window bound, the cost-model time bound `c_0 + κ⌈DW/H⌉` with `κ = max_i ĉ_i/w_i`, and the chains-on-chains optimality remark. Delete the trivial arithmetic-order proposition (`appendices/transform.tex:30-45`).
- **R3. Add the credit-model validation (M4c):** a table or figure of measured per-hop peak and p99 against `⌈DW/H⌉` across the 9 primary cells, plus the p99/mean callback ratio for core, hybrid, and native. Data are already in `data/comparison-012/tables/primary/results.csv` (per-hop peak needs the local bundles).
- **R4. Switch the primary burst metric to the per-hop peak callback (M4a).** Keep p99 as secondary. Report p99.9 and the maxima pattern (M4b). Update Table 4, Fig. 1, and the text in `comparison-results.tex:13-69`.
- **R5. Demonstrate significance (M3).** Add a utilization-sweep exceedance experiment and/or a second, low-power host. Update the abstract to state budget fractions, e.g. "…p99 falls from 0.71% to 0.26% of the 1.33 ms callback interval…".
- **R6. Fix the multichannel baselines (M6.1):** add staggered and batched native rows to Table 7, or weaken the claim at `comparison-results.tex:132-136`.
- **R7. Rewrite the positioning with a delta table (M1)** and add Gardner 1995, García 2002, Wefers & Vorländer 2011, and the four-step FFT references. Remove "we derive … ownership" from the abstract (`abstract.tex:8-9`).
- **R8. Disclose weight tuning (M5)** and add a weight-sensitivity table or a cost-derived weight justification. Weight the band-cache rebuild, or disclose that it is unweighted.
- **R9. Remove or rephrase the "Codex" confound sentence** (`evaluation-method.tex:78-79`), ideally with an idle-control session.

### Should-fix

- **R10.** Add the four-step/sub-FFT hybrid arm (M6.2). If infeasible, move it to the first item of future work and soften conclusions about butterfly-level granularity.
- **R11.** Address the age frontier (M8): add the post-endpoint work lower bound, or explicitly state that `H−1` follows from select-then-compute.
- **R12.** Explain why distributed execution is cheaper than batch for both matched pairs (minor 11).
- **R13.** Clean up notation (minor 1–4, 7) consistently across `sections/*.tex`.
- **R14.** Cite Harris in Sec. 3. Compute the exact coherent gain at cache rebuild, or quantify the tabulated-gain deviation (minor 5).
- **R15.** Change the title (minor 8).

### Nice-to-have

- **R16.** Radix-4/split-radix resumable traversal, or at least a discussion (M6.3).
- **R17.** Real-signal overlap-save with a realistic FIR against uniformly partitioned convolution, as a separate future paper (M7-ii).
- **R18.** Frame the quota as work-based pacing and cite Baker 1978 / Bacon et al. 2003.
- **R19.** Publicly deposit the raw bundles (Zenodo), with an anonymized link during review.

---

## 9. What would move my score up

- **Weak Accept:** a tight 8-page anonymized paper that (i) states the delta over Battenberg & Avižienis and Hurchalla precisely, (ii) gives the quota and time bounds as propositions and **validates the credit model against per-hop peaks**, (iii) drops the inverse/chain section, and (iv) shows at least one regime (utilization sweep or low-power host) where native bursts threaten the deadline and the core or hybrid measurably reduces exceedances.
- **Accept:** additionally, the four-step native sub-FFT arm, which answers "what granularity should cooperative analysis use?", and fair multichannel baselines.

A publication-age lower bound (M8) would add genuine theoretical content that the paper currently lacks.

---

*Verification note:* DAFx26 limits (8 pages maximum, DAFx LaTeX template v3, double-blind) are from the [DAFx26 Call for Papers](https://dafx26.mit.edu/call-for-papers/) and [Authors page](https://dafx26.mit.edu/authors/). Hurchalla's AES 129 paper number 8257 was confirmed via the [AES e-library](https://secure.aes.org/forum/pubs/conventions/?elib=15679).

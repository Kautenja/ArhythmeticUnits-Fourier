# Reviewer 3: Practitioner and Clarity Review

**Paper:** "Resumable FFT Scheduling for Real-Time Spectral Analysis" (`docs/whitepaper/fourier.tex`, compiled PDF `.build/paper.pdf`, 38 pages)
**Target venue:** DAFx (full paper, 8 pages, DAFx two-column template, double-blind)

**Reviewer persona:** An industry audio software engineer with 15 years of shipping VST3/AU/CLAP plugins (JUCE) and some VCV Rack modules, including analyzers, meters, and spectral effects. Regular DAFx and AES reviewer. I care most about whether practitioners will use the work and whether the paper reads well.

**Confidence:** 4/5. I know real-time audio engineering, FFT libraries, and the Rack engine well. For this review I read the whole PDF (text and rendered pages), all `.tex` sources, the shipped scheduler, both modules, the mailbox, and the native benchmark adapters. I also built and ran the standalone DSP and mailbox tests in a scratch copy, including under ThreadSanitizer.

**Overall recommendation:** **Weak Reject** in the current form, with a clear path to **Accept**. The engineering is careful and unusually honest, and the artifact is excellent. The submission, however, is a 38-page technical report and not an 8-page DAFx paper. It does not show that the problem it solves occurs at the settings it evaluates. Its native baselines are not what a practitioner would write. It also buries its most useful result.

| Criterion | Score (1–5) | One-line reason |
|---|---|---|
| Originality | 2 | Time-distributed FFT and cooperative chains are well established, as the authors say. What is new is the explicit whole-pipeline contract and an honest comparison against optimized libraries. |
| Technical soundness | 3 | The scheduler and proofs are correct, and I verified them against the code. The measurement protocol is careful. The native "optimized" baselines use scalar glue code, and there is no host or Rack measurement. |
| Practical significance | 2 | At the headline setting the burst being removed is 0.71% of the callback budget. The regimes where the approach matters (multi-threaded Rack, many aligned instances, small blocks) are not shown convincingly. |
| Clarity / presentation | 2 | Dense hedging, a lab-notebook register, an invented vocabulary used inconsistently, 9 tables and only one data figure in the main body, and no diagram of the mechanism. |
| Reproducibility | 4 | An exemplary artifact (generators, hashes, frozen manifests, check mode). The raw observation bundles are local and not deposited. |
| Fit to DAFx | 3 | The topic fits well: DAFx has published Battenberg & Avižienis, Borß, and Průša & Holighaus. The format, length, and anonymity do not fit at all. |

---

## 1. Summary (in my words)

The authors ship two VCV Rack visualizers: a four-channel spectrum analyzer ("Fourier") and a spectrogram ("Spectre"). Both run a windowed real FFT on Rack's audio engine thread, which calls `process()` once per sample.

**The original design and its problems.** The original design made only the FFT butterflies resumable (q butterflies per sample). This left two problems:

- **Wrong frame rate.** The frame period was the completion time L, not the requested hop H. It could be about 9% fast, which also biased temporal smoothing.
- **Remaining O(N) bursts.** Packing, bit reversal, real-spectrum reconstruction, and display passes still ran in bulk at frame boundaries, and some of them allocated.

**The new design.** The analysis is now one dependency-ordered sequence of W weighted "credits": pack/window, then butterflies, then reconstruct/prefix-sum, then per-bin smoothing and output. A balanced integer quota spreads these credits exactly over H sample calls:

q_s = ⌊sW/H⌋ − ⌊(s−1)W/H⌋

As a result:

- Every call does at most ⌈W/H⌉ credits.
- A frame always publishes at t_r+H−1.
- Input is retained in a ring of capacity N_max+H_max.
- Settings latch at frame start, and reset or a sample-rate change cancels the frame in progress.
- Completed spectra reach the UI through an SPSC triple-buffer mailbox.

**The measurements.** On one M1 Pro, in a synchronous harness (not Rack, no audio device), the authors compare the production core with FFTW, PFFFT, vDSP, a PFFFT "hybrid", and matched controls. For scalar analysis at N=4096, H=1024, D=64, 48 kHz:

- **Burst:** the core's simulated-callback p99 is 3.4 µs versus 9.5 µs for vDSP.
- **Cost:** the core's aggregate cost is 2.27× higher (35.0 vs 15.4 ns/sample).
- **Age:** publication is one hop (21.3 ms) later.

The four-lane SIMD core is cheaper *and* less bursty than four separate native analyses. Periodic inverse jobs and overlap-save chains keep bulk stages, and native libraries win there.

**The takeaway I think the paper should have:**

> Scheduling the **whole** analysis pipeline, not just the FFT, spreads its work evenly over a single hop. This cuts the worst callback about 3× in exchange for about 2× total CPU and one hop of extra display latency. Channel-parallel SIMD recovers most of that CPU cost. It is worth it when simultaneous frame bursts constrain you: per-sample-synchronized multi-threaded engines such as Rack, many phase-aligned analyzers, or small blocks. When phases can be staggered, or work can move off the audio thread, batch FFTs remain cheaper.

The current abstract says, in effect: "Results support a conditional cost–burst–age tradeoff, not a universal speedup or an audio deadline guarantee." That is a disclaimer, not a takeaway.

## 2. Strengths

1. **The problem is real and well identified.** The authors show that the "resumable FFT" folklore is incomplete. Boundary work (packing, reconstruction, display passes) remains an O(N) burst (`sections/legacy-motivation.tex:13-20`). Completion-driven restarts also silently change the frame rate by up to 11% (`tables/legacy-cadence.tex`). Both are concrete, instructive hooks that every analyzer author should hear.
2. **The contract is exact and verified.** I checked `src/dsp/spectrum_analysis.hpp` against Section 5 and Appendix D:
   - W = pM+B+(1+o)K, including the worked values 8194, 11266, and 9219.
   - The quotient/remainder accumulator gives ⌊sW/H⌋ credits after s calls.
   - Publication falls on call t_r+H−1.
   - The ring has capacity N_max+H_max.
   - Configuration latches at frame start, and reset and length changes clear state logically.

   All match (details in §6).
3. **Negative results are included.** The paper reports cases where the approach loses:
   - aggregate cost;
   - the inverse and chain workloads;
   - N=128/H=37;
   - 16 staggered instances;
   - the live-cache case, where the hybrid wins.

   This is rare and valuable.
4. **The matched controls are well designed.** The PFFFT scheduled-batch versus hybrid pair isolates where work runs from which FFT kernel is used. The hybrid (native FFT kept indivisible, surrounding stages scheduled) is a practically important intermediate design.
5. **The ownership and handoff design is correct.** The triple-slot mailbox (`src/rack_extensions/display_mailbox.hpp`) is a correct lock-free latest-value exchange. I ran `test/threads/test_display_mailbox.cpp` under ThreadSanitizer (`make test-mailbox INSTRUMENT=tsan`) in a scratch copy, and it is clean.
6. **Reproducibility engineering is exemplary.** It includes a publication generator with check mode, source hashes, frozen manifests, all-bin numerical audits, and explicit statements of what is excluded from timing.
7. **The four-lane SIMD result is practically useful** (Table 7). At 47.9 ns/sample and 4.6 µs p99, the SIMD core beats four vDSP analyses (57.3 ns/sample, 43.9 µs) on *both* cost and burst.

## 3. Major issues (prioritized)

### M1. The paper does not show that the problem it solves exists at the evaluated settings, and it never confronts the obvious alternatives with host-relevant numbers.

**Location:**
- The key admission is buried at `sections/engineering-limits.tex:12-17` (PDF p. 17, §10.1): *"the production and vDSP p99 values occupy only 0.26% and 0.71% of that interval"*.
- Intro alternatives: `sections/introduction.tex:11-17`.
- Worker threads are dismissed as "unmeasured" at `sections/related-work.tex:43` and `appendices/evaluation-agenda.tex:8-19`.

**Why it matters.** My first reaction as a practitioner is: "A 4096-point real FFT is a few microseconds in vDSP. Call it once per hop and move on." The paper's own data agree:

- The batch burst is 9.5 µs in a 1333 µs callback.
- Even 16 phase-aligned vDSP instances reach only 86 µs, which is 6.5% of a 64-sample budget (Table 6).

Because the budget fraction is buried in the Discussion, a DAFx reader finishes the Results believing a 2.78× p99 reduction matters. It does not at these settings. Readers will feel misled when they reach §10.

**The strongest argument for the design is absent: Rack's execution model.** In Rack 2, `Engine::stepBlock` loops over frames and calls `Engine_stepFrame` once per sample. With more than one engine thread, workers pull modules first-come-first-served and synchronize at a barrier **every sample** (`engineBarrier`/`workerBarrier` in `src/engine/Engine.cpp`). The consequence:

- A 9.5 µs batch FFT inside one module's `process()` holds every other engine thread at that sample's barrier.
- The burst therefore adds its *full* duration to wall time and cannot be load-balanced.
- The per-sample period at 48 kHz is only 20.8 µs.
- A distributed analyzer's ~35 ns/sample is load-balanced with the other modules. With T threads its effective wall-time contribution is roughly cost/T.
- With T=4, the 2.27× cost penalty could become an effective *win* in block wall time.

This hypothesis is Rack-specific, testable, and much more compelling than the p99 of a single-threaded synchronous loop. The paper never mentions threads, barriers, block size, or `process()` granularity. `grep -i barrier` over the sources returns nothing.

**The practical alternatives are also not confronted.** A reviewer at DAFx will raise each of these:

- **UI-thread FFT.** The engine pushes samples into a ring (ns/sample). The widget's `step()` or `draw()` takes the last N samples and runs vDSP/PFFFT at display rate. Many Rack scopes and analyzers work this way, with zero FFT work on the engine. Its real drawbacks are specific to this paper's modules, and the paper should argue them:
  - hop and temporal averaging become tied to a variable frame rate;
  - Spectre needs a gap-free column history even when the UI stalls or the module is not drawn;
  - reading the ring across threads needs a seqlock or snapshot protocol.

  These are good reasons, but the paper does not state them.
- **Worker thread plus lock-free FIFO.** Its costs in Rack include:
  - per-instance thread oversubscription alongside Rack's spinning engine workers;
  - wake-up signaling from the audio thread, since condition variables are not real-time safe, which forces polling;
  - Rack's per-module CPU meter not accounting for the work.

  Battenberg & Avižienis found preemptive execution *better* in their setting (`appendices/related-work.tex:40-43`). That result needs a direct answer, not deferral to "future work".
- **Phase staggering.** The paper's own Table 6 shows 16 staggered vDSP analyses at a p99 of 10.33 µs, against 28.71 µs for the core. When phases can be spread, batch beats the core on *both* axes. In Rack, instances created at patch load or after a sample-rate change all reset on the same sample and are therefore *aligned*. A batch analyzer could randomize or stagger its own initial phase with one line of code. Inside one Fourier module, the four channels could be staggered by H/4.

**Fix (concrete):**
1. Add a short "Host execution model" subsection (about 0.4 DAFx page) before the method. It should cover:
   - Rack's per-sample `process()`;
   - the block size set by the audio device;
   - the engine thread count, which defaults to 1 (verify against the current Rack version);
   - the per-frame barrier;
   - `system::resetFpuFlags()` per block.

   Then state the budget arithmetic: per-sample period 20.8 µs; per-callback budget D/f_s for D ∈ {32, 64, 128, 256, 512, 1024}.
2. Put the budget fractions **in the Results**, not the Discussion. Report "peak callback cost as % of D/f_s" next to every p99.
3. Add one experiment that shows where the approach matters. Either is acceptable:
   - **(a) A barrier harness.** T threads step a shared set of M synthetic modules per sample with a spin barrier, one or more of which is an analyzer. Report block wall time for batch versus distributed as T and instance count vary.
   - **(b) A real Rack session.** 1, 4, and 16 Fourier-like instances; thread counts 1, 2, 4, 8; block sizes 64 and 256. Report Rack's engine time or overload counter.

   Either one gives the paper its reason to exist.
4. Add a paragraph titled "Why not a worker or UI thread?" that argues the specific drawbacks above. Add a paragraph on staggering that admits it is simpler when frame phases can be controlled.
5. End §10 with an explicit **decision rule** (see M7).

### M2. The "optimized native" baselines use scalar, non-idiomatic glue code, and the per-stage cost breakdown is missing.

**Location:**
- `benchmark/paper/external.hpp:44-66` (`ExternalAnalysis::process`)
- `benchmark/paper/vdsp.hpp` (`forward_real_positive`)
- Paper: `sections/evaluation-method.tex:10-18,43-52`; Table 4 (`generated/paper-comparison/tables/analysis.tex`)

**Evidence.** The native analysis path wraps a native FFT in hand-written scalar loops:

- windowing from the ring with a per-element modulo: `frame[i] = ring[(head+i)%ring.size()]*window[i]`, where the integer division blocks vectorization;
- a scalar even/odd de-interleave loop (vDSP provides `vDSP_ctoz`);
- a scalar ×0.5 rescale loop (this could be folded into the window gain);
- `std::abs(std::complex)` per bin, which in libc++ is `hypot` and is slow;
- scalar prefix and output loops.

A practitioner would instead write:

- a doubled or two-span ring;
- `vDSP_vmul` for the window;
- `vDSP_ctoz`, then `vDSP_fft_zrip`;
- `vDSP_zvmags` or `vDSP_zvabs` for magnitudes, working in power or dB where possible;
- scaling folded into the window.

With that pipeline, both the native per-frame cost and its burst could fall substantially. The paper's own isolated numbers support this: at N=4096, vDSP IFFT is 7.17 µs versus 65.19 µs for the first-party scalar IFFT. The analysis burst of 9.5 µs is therefore likely dominated by glue rather than the FFT.

The PFFFT pair points the same way: the scheduled batch reaches 32.5 µs p99 and 36.6 ns/sample, against 9.6 µs and 15.7 ns/sample for PFFFT native. Surrounding stages implemented one bin per unit cost more than the FFT itself. The headline "2.78× p99 reduction" could shrink substantially against an idiomatic baseline.

(I note that the core also uses `abs(std::complex)` at `spectrum_analysis.hpp:181`, so both sides pay for `hypot`. Both could use `sqrt(re²+im²)`, or skip the square root entirely for dB display.)

**Why it matters.** A practitioner will read "vDSP native" as "the best I could do with vDSP". It is instead "vDSP FFT plus the authors' scalar glue". The conclusion that native batch is 2.27× cheaper but 2.78× burstier depends on this glue.

**Fix:**
1. Add a "best-effort idiomatic" vDSP adapter (and PFFFT with its ordered/unordered variant) that uses vectorized windowing, packing, magnitude, and scaling. Report it alongside the current canonical adapter.
2. Add a **per-stage cost breakdown**, a stacked bar per implementation at N=4096 covering capture/window/pack, FFT, reconstruct/magnitude, and smoothing/output. It shows where the 2.27× goes: dispatch overhead, the scalar kernel, or scalar glue. This is also the most useful figure for anyone implementing the design.
3. For multichannel, add batched native baselines (`fftw_plan_many_dft_r2c`, or vDSP's multiple-signal FFT variants, e.g. `vDSP_fftm_zrip`; verify the exact API) and a per-channel staggered variant. Without these, the SIMD result in Table 7 compares four independent, aligned, scalar-glued native analyses against a lane-parallel core. That is a real result, but not the strongest competitor.

### M3. The abstract, introduction, and contributions do not deliver a crisp message, and the most useful result is buried.

**Location:**
- `sections/abstract.tex:1-23` (PDF p. 1)
- `sections/introduction.tex:29-56`
- `sections/conclusion.tex:1-16`

**Evidence.**

- **Abstract.** It spends a third of its length on campaign logistics ("1299 workloads with three process repeats in each of three prepared same-host sessions"). It leads with the scalar case, where the approach loses on cost and wins on a burst of less than 1% of budget. It ends with two disclaimers ("not a universal speedup or an audio deadline guarantee"; "a reproducible implementation study within established time-distributed FFT research"). It never mentions:
  - the SIMD win;
  - the exact-cadence fix;
  - the one-sentence rule for when to use the approach.
- **Introduction.** No results or numbers appear. The contributions are a run-on sentence (`introduction.tex:40-46`). The roadmap discusses "historical campaigns … not attributed to the current weighted scheduler" (`:51-54`), which is an internal bookkeeping concern.
- **Conclusion.** It restates hedges and gives no guidance.

**Fix.** Replace the abstract with something like the following (numbers taken from the current macros; update them after M2):

> Spectrum analyzers in audio hosts are cheap on average but bursty: each hop, one audio callback absorbs a full FFT plus windowing, packing, reconstruction and display preparation. Making only the FFT butterflies resumable, a common remedy, leaves these O(N) boundary passes intact and can silently change the frame rate. We schedule the *entire* analysis pipeline as one dependency-ordered sequence of weighted operations and spread it over exactly one hop with a balanced integer quota. Each engine sample then performs a bounded amount of work, frames publish at an exact cadence, and completed spectra reach the UI through a lock-free triple buffer. We give the work bound, input-retention and ownership requirements, and a GPL implementation used in two VCV Rack visualizers. Against FFTW, PFFFT and Apple vDSP on an Apple M1 Pro, the scheduler reduces the worst audio-callback cost about 3× (3.4 vs 9.5 µs p99 at N=4096, 64-sample blocks) at about 2× total CPU and one hop (21 ms) of extra spectrum age; processing four channels in SIMD lanes makes it both cheaper and less bursty than four library FFTs. Staggering frame phases or moving analysis off the audio thread remains preferable when possible; we give a decision rule for choosing among these designs.

Replace the contributions paragraph with a bulleted list:

> Contributions: (1) a whole-pipeline, exact-cadence, bounded-work scheduling contract for windowed spectral analysis, with retention and ownership requirements (Sec. 4); (2) a matched comparison separating *where* work runs from *which* FFT is used, against three optimized libraries (Sec. 6); (3) evidence of when the approach pays off (aligned instances, small blocks, per-sample-synchronized engines, SIMD channels) and when it does not (staggerable batch, inverse and filtering chains), distilled into a decision rule (Sec. 7).

Add one results sentence with numbers to the introduction. Make the conclusion the decision rule.

### M4. Length, format, and anonymity are incompatible with DAFx.

**Location:**
- `fourier.tex:3-8` (11pt article, 1-inch margins, author and email, "Technical report, manuscript version 3")
- `preamble.tex:22-30` (fancyhdr "Technical report")
- `appendices/availability.tex:5-6` (CITATION, "manuscript version 3")
- `appendices/reproducibility.tex:3-4` (repository URL)

**Evidence.** The PDF has 38 pages:

- main body ≈ 18 single-column pages (about 7,000 words, 9 tables, 1 data figure);
- bibliography ≈ 3 pages;
- 12 appendices ≈ 17 pages.

DAFx26 requires at most 8 pages in the DAFx LaTeX template and double-blind review with no author names or affiliations. The following all identify the author: the author name and email, "Arhythmetic Units", the module names "Fourier" and "Spectre", the GitHub URL, `CITATION.cff`, and the self-description "technical report". "Codex" is also named as a confound (`sections/evaluation-method.tex:78-79`), which identifies the tooling and is opaque to readers.

**Fix.** Use the DAFx26 template with the `blind` option. Replace the repository URL with "anonymized for review; supplementary archive provided". Refer to "a four-channel analyzer module and a spectrogram module". Put all appendices, the full method, and the historical campaigns in an anonymized supplementary PDF or arXiv version.

**Proposed DAFx outline** (existing file, then new section; budget in DAFx two-column pages):

| New section | Source material | Budget |
|---|---|---|
| 1 Introduction (hook: burst plus wrong frame clock; contributions; one-line result) | `introduction.tex` rewritten; `legacy-motivation.tex:3-9` (cadence example) | 0.7 |
| 2 Related work (compact, adds Gardner 1995, real-time audio programming refs) | `related-work.tex` condensed by about 40%; drop `tables/related-approaches.tex` | 0.45 |
| 3 Host model and problem statement (Rack per-sample `process()`, block D, threads/barrier, budgets; frame, hop and age definitions) | NEW + `signal-model.tex:3-23` (drop amplitude-calibration paragraph `:25-35`) | 0.6 |
| 4 Whole-pipeline one-hop scheduling (why the whole pipeline: `legacy-motivation.tex:13-20`; stages Table 1; quota Eq.; proposition plus D-sample bound; retention; mailbox; **new Fig. 1 timeline**; 12-line dispatch listing from `appendices/one-hop.tex:14-27`) | `production-successor.tex` + `one-hop.tex` proof (3 lines) | 1.6 |
| 5 Method (implementations and controls table; host/compiler in 4 lines; workloads; metrics: cost, peak-callback % budget, p99, max, age) | `evaluation-method.tex` cut about 65%; `tables/comparison-controls.tex` kept | 0.8 |
| 6 Results: 6.1 tradeoff (**new Fig. 2 Pareto**, compact Table 4); 6.2 where it matters (**new Fig. 3** budget/instances/threads; staggering); 6.3 SIMD channels (Table 7 compact); 6.4 ablation: butterfly-only vs whole pipeline (legacy incremental + inverse/chain, 1 paragraph + 3-row table) | `comparison-results.tex` cut about 55% | 2.3 |
| 7 Discussion: when to use it (decision rule, perceptual latency, limits in one paragraph) | `engineering-limits.tex` cut about 60% | 0.7 |
| 8 Conclusion | rewritten | 0.2 |
| References (about 25 entries) | `bibliography.tex` | 0.6 |
| **Total** | | **≈7.95** |

**Cut list with estimated savings** (current single-column pages; one DAFx page ≈ 1.8 current pages):

| Cut | Current pages saved |
|---|---|
| §9 Historical Evidence (`historical-evidence.tex`) → drop; one sentence in §4 | 0.3 |
| §6 Inverse & chain (`inverse-and-chain.tex`, 117 lines) + Table 5 + §8.2 → one ablation paragraph + 3 rows | ≈2.9 |
| §7 Method (192 lines) + Table 3 campaign design → 0.8 DAFx page; provenance to supplement | ≈2.2 |
| §8.3 transitions + §8.4 numerical/resource + Tables 8, 9 → 3 sentences + supplement | ≈1.6 |
| `signal-model.tex:25-35` calibration paragraph | 0.3 |
| Hedges and scope sentences throughout (about 99 negations such as "not/neither/nor/cannot" in about 7,000 words; see M5) | ≈1.5 |
| All 12 appendices → supplementary PDF | 17 |

### M5. Lab-notebook register, invented terminology, and relentless hedging make the paper hard to read.

**Evidence.**

**Lab-notebook register.**
- "manuscript version 3" (`fourier.tex:8`)
- "the September 30 confirmation campaign" (`comparison-results.tex:10`)
- "revision 12ac3236e332 plus the recorded runner-only metadata fix" (`evaluation-method.tex:60-61`)
- "A failed preliminary launch was rejected by the provenance gate" (`:85-86`)
- "Only 16 exactly duplicate transition configurations shared by the pilot plans were removed" (`:90-91`)
- "on the same laptop and night" (`:77`)
- "Portable bundles rederived all eight report tables" (`:187`)
- "excluding Finder metadata" (`appendices/reproducibility.tex:28`)

These belong in an artifact README, not a conference paper.

**Terminology inventory.** In most cases several names are used for one concept. Pick one and define it once:

| Concept | Terms used | Suggest |
|---|---|---|
| unit of scheduled work | credit, scheduling credit, work credit, unit, scheduled unit, step | **work unit** (weighted) |
| the new design | production core, core, production analyzer, complete analysis implementation, one-hop, successor (in appendices) | **one-hop scheduler (OHS)** |
| the old design | legacy, original, historical, butterfly-only | **butterfly-only scheduling** |
| the study | confirmation campaign/study/experiment, comparison campaign, *replacement campaign* (`appendices/transform.tex:98`, `appendices/related-work.tex:146`; never defined) | "our evaluation" |
| host granularity | sample call, callback, host callback, simulated callback, block | **sample call** and **block of D samples** |
| latency | age, delay, publication delay, sample-index age, spectrum-center age, endpoint-to-publication age, release-to-publication, input-to-delivery | **added latency** (H−1 samples) and **spectrum age** (center) |
| peakiness | burst, concentration, callback concentration, callback p99 | **peak block cost** |
| tradeoff | "cost–burst–age tradeoff" (abstract only), "cost, burst, storage, and age tradeoff" (Table 2) | define once in §1 or drop |
| guarantees | bounded-operation contract, whole-pipeline quota bound, work bound, logical credit bound | **work bound** |

Add a 5-row notation table (N, H, D, W, f_s) and use nothing else.

**Hedging.** Nearly every paragraph ends with a "this is not X" sentence. Examples:

- "This is a practical implementation comparison, not the isolated cost of suspending a common FFT kernel." (`comparison-results.tex:23-24`)
- "these curves are not an experiment with constant overlap ratio" (`:44-45`)
- "The contribution is a reproducible implementation study within established time-distributed FFT research." (`abstract.tex:21-22`)
- "Neither transform suspension nor extending it to an inverse chain is a new principle of this report." (`related-work.tex:17-18`)

Each is defensible alone. Together they make the reader do the work of extracting a claim. **Fix:** consolidate every scope caveat into one "Threats to validity" paragraph in §7. State each result positively once, with its condition. Delete per-sentence disclaimers.

### M6. There is no figure explaining the mechanism, and the figures and tables do not carry the argument.

**Location:** The main body has one data figure (Fig. 1, `generated/paper-comparison/figures/length.tex`, PDF p. 13) and 9 tables. The only schematic (`figures/legacy-timeline.tex`) is in Appendix B and depicts the *old* design.

**Why it matters.** The core idea is visual: a frame's work, which used to be a spike, becomes a flat strip across one hop. DAFx's audience of engineers and artists will grasp it from a picture in 5 seconds and from Section 5 in 5 minutes.

**Proposed figures:**

1. **Fig. 1: Schedule timeline** (full width, TikZ).
   - X axis: sample calls across 2.5 hops.
   - Top lane: input capture, with the ring-retained window for frame r shaded from t_r−N+1 to t_r.
   - Middle lane (batch): a tall spike at t_r labeled pack+FFT+reconstruct+output.
   - Bottom lane (one-hop): a flat strip from t_r to t_r+H−1, color-segmented by stage in proportion to their credits (pack pM, butterflies B, reconstruct K, output oK). A frame-r+1 strip starts right after.
   - Publication arrows at t_r (batch) and t_r+H−1 (one-hop), with added latency annotated "H−1 samples = 21.3 ms".
   - Block boundaries every D samples as light ticks, with the per-block credit bound ⌈DW/H⌉ annotated. For N=4096, H=1024, D=64: W=17,410, ≤18 units per sample, ≤1,089 per block.
2. **Fig. 2: Tradeoff plot.** Aggregate cost (ns/sample) on x, peak block cost on y (log). One marker per implementation at N=4096, with marker fill encoding added latency (0 vs H−1). This shows the Pareto structure at a glance: native in the low-cost/high-burst corner, core and hybrid in high-cost/low-burst, SIMD dominating the four-channel natives. It replaces most of Table 4 and all of Fig. 1's cost panel.
3. **Fig. 3: Decision map.** Peak block cost as % of block budget D/f_s against D (32…1024), with lines for batch vDSP (best-effort, M2) and one-hop at N ∈ {2048, 16384}. Include panels or line styles for 1, 4, and 16 aligned instances, plus staggered batch. Add a shaded "headroom threshold" band (e.g., 10%). If the barrier experiment is done (M1), add a panel of wall time against thread count.
4. **Fig. 4 (optional): Per-stage cost stacked bars** (M2, fix 2).

Condense Table 6 (16 rows × 3 columns of p99 with ranges) into Fig. 3 or into a ratio column (core/vDSP) with at most 8 rows. In Table 4, report p99 with at most 3 significant figures. The M1 Pro `steady_clock` tick is about 41.7 ns (24 MHz), so "3.417 µs" in the abstract and text implies false precision; Table 4 already prints 3.42.

### M7. There is no practical guidance, and the latency cost for a visual display is not put in perceptual context.

**Location:** `sections/engineering-limits.tex:3-17`; `sections/comparison-results.tex:51-60`.

**Evidence.** The paper says a visualizer "that can accept the additional age and places high value on shorter callbacks has a reason to choose the core". That is circular. Readers need a decision rule and a sense of whether 21 ms matters.

**Perceptual context the paper should supply:**

- Spectrum-center age rises from 42.7 to 64.0 ms at N=4096, H=1024.
- One 60 Hz display frame is 16.7 ms. Compositor and vsync add 1–3 frames, and the audio output buffer adds D/f_s or more.
- For audio leading video, which is the case for a spectrum display, broadcast audio–video sync guidance puts detectability at roughly 45 ms and acceptability at roughly 90 ms (ITU-R BT.1359-1; **verify**).
- The added hop can therefore push total audio-to-visual offset past the detectability threshold, while a batch analyzer might remain below it.
- Conversely, user-selectable temporal averaging (0–2.5 s in both modules) often dominates perceived lag.
- Spectre's column rate at H=1024 is 46.9 Hz, below 60 Hz, so no display frame is ever starved.
- The transition result deserves explicit discussion. The core's knob-to-first-response latency is 85.3 ms, against 42.6 ms for scheduled batch (Table 8). About 85 ms is at the edge of feeling sluggish for direct-manipulation UI.

**Fix:**
1. Add a paragraph that converts each age into display frames and relates it to these thresholds, with appropriate hedging and citations.
2. Add the **decision rule**, for example:

> Use one-hop whole-pipeline scheduling when (i) analysis must run on the audio/engine thread (no worker or UI-thread analysis possible, or exact-hop history required as in a spectrogram), **and** (ii) the batch frame cost times the number of analyzers whose frames can coincide in one block exceeds your per-block headroom, or the host synchronizes threads per sample (Rack with more than one engine thread), **and** (iii) one extra hop of latency, H/f_s, is acceptable for the display. If frame phases can be staggered (instances × D ≤ H), prefer staggered batch FFTs: they are cheaper and, in our data, less bursty. If several channels share one hop, process them in SIMD lanes.

3. **Constructive design suggestion.** Decouple the *completion horizon* H_c ≤ H from the hop by spreading W over H_c calls and publishing at t_r+H_c−1. This is a one-line change to the quota (use H_c in place of H). It turns a binary choice (batch, H_c=1; one-hop, H_c=H) into a continuous burst-versus-latency knob. For example, H_c=H/4 adds 5.3 ms instead of 21.3 ms at about 4× the per-call units, which is still far below a batch spike. Evaluating two or three H_c values would make the tradeoff curve the paper's central figure.

### M8. The inverse and overlap-save material dilutes the paper and is framed as disclaimers rather than as a lesson.

**Location:** §6 (`sections/inverse-and-chain.tex`, 117 lines, PDF pp. 6–8); §8.2; Table 5; `engineering-limits.tex:28-37`.

**Evidence.**

- Neither module uses these paths (`inverse-and-chain.tex:5-7`).
- The "incremental controls" keep O(N) buffering, an inverse-buffer operation charged as one unit, and ring-to-frame copies outside the quota (`:77-81`).
- They cost about 6× native (Table 5) and add H−1 samples of delay.
- The three-tap FIR through a 4096-point FFT is explicitly "not a claim that FFT convolution is economical".

This occupies about 3 pages and a large table to show that *partial* scheduling does not work. That is exactly the thesis of §4.

**Fix.** Reframe the material as a single ablation supporting the main claim: "Scheduling only the butterflies is insufficient: the legacy incremental analyzer (Table 4) and the butterfly-only inverse (Table 5) retain O(N) boundary work and do not match whole-pipeline peaks." Keep 3 rows (batch control, butterfly-only control, best native) for the inverse job only. Move the overlap-save chain, its delay algebra (Eq. 9), and its checks to the supplement. Alternatively, commit fully: implement a whole-pipeline-scheduled inverse chain and evaluate it as a real application (e.g., partitioned convolution with a realistic IR). That would be a different, larger paper.

### M9. External validity: there is no real host, no x86, and no measurement of how users will perceive the cost.

**Location:** `sections/evaluation-method.tex:51-52,54-81`; `sections/engineering-limits.tex:57-65,74-80`.

**Evidence.** All data come from a synchronous harness on one M1 Pro, over "one night". There are:

- no Rack session;
- no audio device;
- no concurrent UI;
- no x86 host.

Rack's user base is largely Windows and Linux x86-64 (**verify**; VCV does not publish exact figures). FFTW, PFFFT, and first-party SIMD relative costs differ substantially between NEON and AVX2.

**Adoption risk the paper misses.** Rack's per-module CPU meter shows *average* time per sample. A 2.27× higher average makes the module look more than twice as expensive to users, who pick modules by that meter. The design trades a metric users see for one they do not.

**Fix:**
1. Repeat the primary grid (N ∈ {2048, 4096, 16384}, D ∈ {64, 256}) on one x86-64 host (Windows or Linux) with FFTW and PFFFT.
2. Measure in Rack itself: CPU meter readings and engine overload behavior for 1/4/16 instances at 1 and 4 threads, or run the barrier harness from M1.
3. Discuss the CPU-meter optics in §7.
4. Report the shipped defaults as the headline configuration (see §6.6 below).

### M10. Code availability and reuse for practitioners.

**Location:** `appendices/availability.tex`; `appendices/reproducibility.tex:45-57`.

**Evidence.**

- The raw bundles are "local, untracked" and "have not been deposited".
- The code is GPL-3.0-or-later, which prevents reuse in the proprietary plugins that make up most of the commercial market.
- The core algorithm (about 30 lines) appears only as a partial listing in an appendix (`appendices/one-hop.tex:14-27`).

**Fix:**
1. Deposit the raw bundles on Zenodo (or similar) with a DOI before camera-ready.
2. Put a complete ~15-line dispatch listing in the main text so readers can reimplement it clean-room.
3. Optionally, consider a permissive license for the header-only scheduler (`src/dsp/spectrum_analysis.hpp`) to maximize impact. That is the authors' choice, but say explicitly what reuse the license permits.

## 4. Minor issues (section by section)

**Front matter (`fourier.tex`, `preamble.tex`)**
- `fourier.tex:6`: Title. "Resumable FFT Scheduling" undersells the whole-pipeline point and oversells the FFT. Consider "Whole-Pipeline Scheduling of Spectral Analysis on the Audio Thread" or "Spreading Spectrum-Analyzer Work Over One Hop: Bursts, Cost, and Latency".
- `fourier.tex:8`: Remove "Technical report, manuscript version 3". `preamble.tex` fancyhdr "Technical report": remove it; the DAFx template sets headers.
- Keywords (`abstract.tex:24-25`): add "VCV Rack", "audio visualization", "lock-free"; drop "inverse transform" and "overlap-save filtering" if M8 is adopted.

**Abstract (`sections/abstract.tex`)**
- `:3` "This report": use "This paper".
- `:7-9` "publishes a complete spectrum at an exact hop; we derive input-retention, sample-age, and ownership requirements": jargon-dense before any definition.
- `:10` "matched scheduling controls" is undefined in the abstract.
- `:11-13` campaign logistics: delete.
- `:15` "simulated callback p99 is 3.417 µs": use 3.4 µs (timer tick is ≈41.7 ns).

**Introduction (`sections/introduction.tex`)**
- `:2-8`: Good opening, but give a number: "a 4096-point analysis frame costs ≈X µs, in one of every 16 blocks".
- `:19-24`: The two legacy problems are the best hook in the paper. Move them up and give the 9.1% cadence example right here.
- `:29` "work-credit bound" is used before definition.
- `:38-40`: Say which module uses what: "Spectre" is scalar, N=2048, H=N/2 fixed (`src/Spectrogram.cpp:94,387-389`).
- `:48-56`: Roadmap. Delete the historical-campaign sentences.

**Related work (`sections/related-work.tex`)**
- `:17-18`: Replace the defensive novelty disclaimer with a positive positioning sentence.
- Missing, likely expected by DAFx reviewers:
  - W. G. Gardner, "Efficient convolution without input-output delay," *J. Audio Eng. Soc.* 43(3), 1995: the canonical load-balanced FFT-convolution reference in this community.
  - A real-time audio programming reference for the no-locks/no-allocation rules and lock-free handoff, e.g. R. Bencina, "Real-time audio programming 101: time waits for nothing," 2011 (blog; **verify**), or T. Doumler's talks (**verify**).
  - A triple-buffer or SPSC reference.
  - J. O. Smith, *Spectral Audio Signal Processing* (STFT conventions).
  - G. Heinzel et al., "Spectrum and spectral density estimation by the DFT…," 2002 (coherent gain, ENBW; relevant to `signal-model.tex:25-35`; **verify**).
- `:20-23`: Průša & Holighaus is the closest application precedent. Say in one sentence why their worker-thread model was not adopted.

**Signal model (`sections/signal-model.tex`)**
- `:3-6`: The sample-call versus callback distinction is good. Keep it as the only definition and use it consistently.
- `:25-35`: The amplitude-calibration paragraph is irrelevant to scheduling. Cut it to one sentence or move it to the supplement. It also mentions the "original FFT campaign" and "pipeline campaign" before they are introduced (`:33-35`).
- `:39-40`: B is defined here as B_r(N) and redefined in `production-successor.tex:12`. Define it once.

**Motivation (`sections/legacy-motivation.tex`)**
- `:6-9`: Good. Add "(Table 10)" inline, or bring the four-row cadence table into the main text. It is small and persuasive.
- `:15` "some of those paths allocate": say which, in one clause (packing temporary vector, prefix array; `appendices/original-pipeline.tex:18-23`).

**One-hop scheduling (`sections/production-successor.tex`)**
- `:12-18`: The weights p=4 and o=2 look arbitrary. Explain how they were chosen (measured relative cost? rule of thumb?) and whether they matter (sensitivity).
- `:36-40`: "Weighted operations execute at their first credit; later credits can be bookkeeping only". This means weighting *idles* the rest of the weighted span. State the consequence plainly: dirty-window frames run the window rebuild at ¼ density and the remaining stages at higher density.
- `:46-50`: The D-sample bound ⌈DW/H⌉ is the practitioner-relevant quantity. Promote it to the proposition and give the worked number (W=17,410; ≤1,089 units per 64-sample block at N=4096, H=1024).
- `:73`: The main text references Eq. (17) in Appendix B. Main text must not depend on an appendix equation in a DAFx paper.
- `:92-97`: "increasing the supported hop in a sample-rate callback can allocate". Good that it is stated. Add that Rack calls `onSampleRateChange` while holding the engine lock (Rack 2 `Engine::setSampleRate`), so it is not on the audio path but can stall the engine briefly.
- `:99-107`: Mention the ThreadSanitizer test (it passes; I ran it). Mention Spectre's per-column mailbox memory (see §6.7).

**Inverse and chain (`sections/inverse-and-chain.tex`)**: see M8. If kept:
- `:62-66`: explain why an identity filter is informative at all;
- Eq. (9): the balanced chain's 2H−2 delay is a significant practical cost and should be stated in ms.

**Method (`sections/evaluation-method.tex`)**
- `:56-58`: `-O3 -funsafe-math-optimizations -march=armv8-a+fp+simd`. Say explicitly that these match Rack's plugin build flags (they appear to; **verify** against Rack's `compile.mk`). That is the justification.
- `:73-81`: Replace "Ordinary macOS services and Codex remained possible confounds" with "background processes including an interactive development tool". Also anonymize.
- `:93-112`: Move Table 3 to the supplement.
- `:123-135`: Add a metric that is conditional on a frame event: median cost of blocks *containing* a frame endpoint versus those that do not. It is more interpretable than p99, whose ability to capture the burst depends on D/H (a problem the paper itself documents at `comparison-results.tex:122-129`).
- `:145-152`: The aggregation rule (mean of process means per session, equal weight; median of session medians for p99) is fine. Two lines suffice in the main text.

**Results (`sections/comparison-results.tex`)**
- `:3-11`: The opening paragraph re-hedges. Start instead with the result sentence and Fig. 2.
- Table 4 (PDF p. 12):
  - Add a "Delay (ms)" column or convert.
  - Add "Peak % of D/f_s".
  - Reduce ranges to ± or drop them into the caption.
  - The caption is 6 lines; halve it.
- `:40-47` and Fig. 1:
  - Only 3 x-points on a log-log plot with a 2-column legend over the left panel.
  - Error bars are invisible except for the hybrid at 16384.
  - Replace with Fig. 2 and 3 (M6), or at minimum put one shared legend below both panels and use linear y with a broken axis.
- `:62-69`: Core maximum 631.92 µs at N=2048 is about 185× its p99 and larger than any batch maximum. Say what it was: first-touch page fault, preemption, or timer migration. With a single observation it should not appear in the text without an explanation, or it invites suspicion.
- `:101-110`, Table 6 (PDF p. 15):
  - "Callback offset 1/15/63" rows are identical to the reference and could be one sentence.
  - The "16 staggered instances" row (core worse than vDSP by 2.8×) is one of the most important practical findings and gets half a sentence (`:106-109`). Promote it (M1).
- `:122-129`: The single-sample p99 = timer p99 = 0.042 µs is one tick of the 24 MHz counter. Say so; it explains the "equality".
- `:131-138`, Table 7: Put this result in the abstract. Caption "Cost (ns/engine sample)" versus "ns/sample" elsewhere: make it uniform.
- `:142-154`, Table 8: Explain *why* production response is 2× scheduled batch (wait for the frame boundary, up to H, plus one hop of processing). Then relate 85 ms to UI responsiveness (M7).
- `:156-168`, Table 9:
  - "Diagnostics 370,800" float violations of a "retained legacy pointwise rule" will alarm readers and is unexplained in the main text. Either explain it in one sentence (weak bins near the float noise floor) or drop the column.
  - "11,658,240 audited spectra" is coverage, not evidence strength. One sentence suffices.
- `:178-186`: Heap numbers (556.5 KiB versus 136.6 KiB). Useful. Explain that the core's is dominated by max-size (N_max, H_max) preallocation, and give the bytes for shipped sizes.

**Historical evidence (`sections/historical-evidence.tex`)**: Delete (M4). One sentence in §4 is enough: "An earlier prototype that overlapped stages across four hops added 3H of latency without reducing peaks further; we therefore restrict work to one hop."

**Discussion (`sections/engineering-limits.tex`)**
- `:12-17`: Move to Results (M1).
- `:19-26`: The hybrid paragraph is valuable. The hybrid (native FFT kept whole, other stages spread) captures most of the peak reduction at lower cost. Many practitioners will prefer it because it keeps their preferred FFT library. Recommend it explicitly in the decision rule and give its numbers (5.4 µs p99, 31.4 ns/sample).
- `:47-50`: The p99 "less than once per hundred calls" pitfall is already in §8.3. Deduplicate.
- `:79-80` "The complete paper can support an implementation decision under its recorded conditions": replace with the decision rule.

**Conclusion (`sections/conclusion.tex`)**: Rewrite as 3 sentences: result, when to use it, what is next (host/thread measurement, H_c knob, idiomatic baselines).

**Tables and figures (general)**
- Nine tables and one data figure is unbalanced for DAFx. Target 3 figures + 3 tables.
- Table 1 (`tables/production-work-units.tex`) "Each pair occupies p credits: read two retained samples, prepare/apply window weights and store the permuted pair at the first credit; skip the remaining credit." Simplify to "Pack & window a sample pair (weight p)".
- Table 2 (`tables/comparison-controls.tex`) is useful; keep it but shorten the cells.
- Units: use µs and ms consistently. Delay in samples in tables versus ms in text is inconsistent.
- Number precision: 3 significant figures maximum given the 41.7 ns timer tick.

**Bibliography (`bibliography.tex`)**
- Use the DAFx template's BibTeX style rather than a hand-written `thebibliography` with mixed "doi:" hyperlinks, "Accessed" dates, and "Planner API" sub-URLs.
- `balasubramaniam2026` (DAFx26): **verify** the page numbers and URL. It is a very recent citation.
- Also **verify**: `skare2024` pages 452–458, `liu2017` volume and pages, `eleftheriadis2023` DOI, `lo1998`/`lo1999` details.
- `vdsp`: cites a *legacy archive* page ("library/archive/…vDSP_Programming_Guide"). Cite the current Accelerate/vDSP documentation.
- `rack`: cite the Rack engine source (`src/engine/Engine.cpp`, v2 branch) for the threading and barrier model once M1 is added; the Plugin API guide alone does not document it.
- `pffft`: "J. Pommier and contributors" is fine; add the upstream (bitbucket jpommier/pffft) alongside the VCV fork.
- `wilhelm2008`, `kalibera2013`, `mytkowicz2009`: fine, but three methodology citations for a 3-session single-host study reads as over-defended. Keep one or two.

**Appendices (if kept in a supplement)**
- `appendices/transform.tex:98` and `appendices/related-work.tex:146`: "replacement campaign" is undefined.
- `appendices/prototype-evaluation.tex:46-50`: "1,934,764 standalone assertions in seven cases". At HEAD, `test_spectrum_analysis` reports **2,180,346 assertions in 13 test cases** (I ran `make test-dsp` in a scratch copy). State the revision the count refers to, or regenerate the count.
- `appendices/prototype-evaluation.tex:51-52`: Add "ThreadSanitizer (mailbox)". The Makefile supports `INSTRUMENT=tsan`, and it passes.
- `appendices/original-scheduling.tex:11-13`: The legacy ceiling is computed via single-precision float division, a latent bug for large B/H. Worth one sentence in the motivation as a third legacy hazard.
- `appendices/one-hop.tex:99-103`: The Fourier hop control is in seconds (5–300 ms, default 30 ms; `src/SpectrumAnalyzer.cpp:160,329-333`). Say so in the main text: the shipped hop range is 240–14,400 samples at 48 kHz, not 1024.
- `appendices/availability.tex:5` "No public data or arXiv deposit is claimed": fix with a deposit (M10).

## 5. Code-versus-paper consistency findings

I verified these directly in the source. Items 1–5 are consistent; 6–12 are gaps, hazards, or undocumented behavior.

1. **Work formula and quota: consistent.**
   - `spectrum_analysis.hpp:86-90,253-255`: `preparation_units = (window_dirty?4:1)*(N/2)`, `W = prep + butterflies + (1+o)(N/2+1)`, `quota_base = W/H`, `quota_remainder = W%H`.
   - Accumulator at `:304-306`. Worked values 8194/11266/9219 reproduce.
   - For the headline configuration I compute W=17,410 (≤18 units per sample; ≤1,089 per 64-sample block). For Fourier's four-lane path (o=2): W=19,459 clean and 25,603 dirty.
2. **Publication timing: consistent.** `process()` returns true when `++phase == hop` (`:372`). The frame is selected on the phase-0 call, after capturing that sample (`:292-303`), so publication falls on t_r+H−1.
3. **Retention: consistent.** The ring is `input(maximum_length + maximum_hop)` (`:219`). Fourier grows it via `reserve_hop(ceil(0.3·f_s))` in `onSampleRateChange` (`SpectrumAnalyzer.cpp:235`), which may allocate, as the paper states. Spectre constructs with `H_max = N/2` and never calls `reserve_hop`; its hop is fixed at N/2 (`Spectrogram.cpp:94,387-389`), so this is safe.
4. **Latching and cancellation: consistent.**
   - `configure()` rejects mid-frame calls (`:262`).
   - A length change clears `available` and the EMA (`:270-271`).
   - `reset()` zeroes the phase and marks caches dirty (`:239-246`).
   - Both modules call `reset()` on sample-rate change. Rack 2 invokes `onSampleRateChange` while holding the engine mutex (confirmed in `Engine.cpp`), so there is no race with `process()`.
5. **Freeze semantics: consistent** with `appendices/one-hop.tex:128-130`. Fourier passes `capture=is_running` and zero input (`SpectrumAnalyzer.cpp:477,513-516`), so it keeps re-analyzing the same window at full cost. Spectre skips `process_coefficients` entirely (`Spectrogram.cpp:541-543`). Its partial frame resumes after unfreeze, containing pre-pause samples, and a timestamp discontinuity follows. The paper mentions this only in passing.
6. **Undocumented cost: the band-cache rebuild.** When `bands_dirty`, `prepare_band(k)` (two float divisions, two floors) runs inside output units with **no extra weight** (`:201`). Only the window rebuild receives p=4. Table 1 does not list this. It is minor, but the paper claims the weights account for cache rebuilds (`production-successor.tex:52`).
7. **Idle cost.** Fourier always runs all four SIMD lanes, even with unpatched inputs (`process_input_signal` builds a `float_4` regardless). Spectre likewise analyzes zeros. An idle analyzer therefore costs the full rate. A practitioner will ask why there is no bypass for disconnected inputs, and it affects the CPU-meter optics (M9).
8. **The shipped defaults differ from the headline configuration.**
   - Fourier defaults: N=2048 (`configParam(PARAM_WINDOW_LENGTH,…,11)`), hop 30 ms = 1,440 samples at 48 kHz, **Flattop** window, four SIMD lanes, output weight 2 (`SpectrumAnalyzer.cpp:149,154,160,102`).
   - Spectre: scalar, N=2048, H=1024, Flattop.
   - The paper's headline is scalar, N=4096, H=1024, Hann, o=1, which matches neither module.

   The table the paper should lead with is the four-lane configuration at Fourier's defaults.
9. **Hardest shipped regime not evaluated.** Fourier permits N=16,384 with a 5 ms hop (240 samples at 48 kHz): W=86,019, ≤359 units per sample, ≤22,939 per 64-sample block. That is the regime where both designs are most stressed, but the "shorter hop" extension uses only N=2048, H=257.
10. **Memory: Spectre's mailbox array.** `DisplayMailbox<DisplayColumn>[N_STFT=512]`, each holding 3 × (1025 floats + uint64), is about 6.3 MB per instance (`Spectrogram.cpp:80-83,98-99`), plus a 2.1 MB UI copy. It is correct but heavy. A single SPSC ring of columns would need about one third of that. Worth a sentence, since the paper discusses storage (`comparison-results.tex:178-186`).
11. **`onReset()` publishes from a non-engine context.** Both modules call `publish()` in `onReset()`, which also runs from the constructor. `DisplayMailbox::producer` is a plain `unsigned`. This is safe only because Rack serializes `onReset` with `process()` under the engine lock (and construction precedes both threads). Document this "single producer at a time, serialized by host lock" assumption in the mailbox header and the paper, since the paper stresses that "a flag alone is not a synchronization contract".
12. **Denormals and FPU mode.** Rack calls `system::resetFpuFlags()` per block (Engine.cpp). The benchmark harness sets no FTZ/DAZ mode (a search of `benchmark/paper/` for denormal/FTZ/flush finds nothing). With noise fixtures this is harmless. However, silence after signal, with EMA smoothing (α up to ≈1) and the double-precision DC blocker (`SpectrumAnalyzer.cpp:483`), decays through the subnormal range. That workload is untested in the harness and could differ between harness and Rack. State the FPU mode used, and **verify** whether Rack sets FPCR.FZ on ARM64.
13. **Benchmark glue code** (`benchmark/paper/external.hpp:53-60`; `vdsp.hpp` `forward_real_positive`): the per-element modulo, scalar de-interleave, scalar rescale, and `std::abs`→`hypot` described in M2.
14. **Tests.** Standalone DSP tests pass at HEAD (I ran `make test-dsp test-mailbox` in a scratch copy): 2,180,346 assertions and 13 cases for the analyzer, against the paper's 1,934,764 and 7. The mailbox passes under TSan. I did not run the Rack-linked suites or the timing campaign.

## 6. Questions for the authors

1. Rack's engine synchronizes worker threads with a barrier every sample. Have you measured batch versus one-hop analysis with Rack's engine thread count above 1? Is the effective wall-time penalty of the 2.27× cost reduced, or reversed, by load balancing?
2. Why not compute the FFT on the UI thread from a lock-free snapshot of the last N samples, as many Rack visualizers do? Is the exact-hop, gap-free history (especially for Spectre's columns and Fourier's temporal averaging) the deciding requirement? If so, please say so in the paper.
3. With an idiomatic vDSP pipeline (vectorized window/pack/magnitude, scaling folded into the window), what are the native cost and p99 at N=4096, D=64? Does the 2.78× p99 gap survive?
4. What is the per-stage cost split (pack, FFT, reconstruct, output) for the core and for PFFFT native? Why does "PFFFT scheduled batch" reach a p99 of 32.5 µs when PFFFT native reaches 9.6 µs, for the "same" task?
5. How were the weights p=4 and o=2 chosen? How sensitive is the peak to them?
6. Did you consider a completion horizon H_c < H to reduce the added latency (M7.3)?
7. Could the Fourier module stagger its four channels, or randomize each instance's initial frame phase? How do instance phases behave after patch load in Rack? My understanding is that they are aligned, because all modules reset on the same engine sample.
8. What caused the core's 631.92 µs maximum at N=2048? Was it at process start (first touch) or mid-run?
9. What fraction of Rack users run on ARM64 versus x86-64? Do you expect the ranking to hold with AVX2 FFTW/PFFFT?
10. Does anything in the pipeline, or in the double-precision DC blocker, produce subnormals on silence after signal? Which FPU mode applies in Rack on ARM64?
11. Why does Fourier analyze all four lanes when some inputs are unpatched?
12. Will the raw bundles be deposited with a DOI? Would you consider a permissive license for `spectrum_analysis.hpp` and `display_mailbox.hpp`?
13. Is the 85 ms control-response latency noticeable when turning the window or hop knobs? Have you received user reports?

## 7. Revision checklist (executable against the `.tex` sources)

All paths are relative to `docs/whitepaper/` unless noted. Generated files under `generated/paper-comparison/` must be changed through `tools/comparison_paper.py`, not edited by hand.

### Must-fix
1. **[Format/anon]** Port to the DAFx26 LaTeX template with `blind` enabled. In `fourier.tex`:
   - remove `\author`, the date, and "Technical report, manuscript version 3";
   - replace `preamble.tex` geometry/fancyhdr with template defaults;
   - move every `\input{appendices/...}` into a new `supplement.tex` compiled separately.
2. **[Anon]** Replace the repository URL (`appendices/reproducibility.tex:3-4`) and the CITATION text (`appendices/availability.tex`) with "anonymized for review". Replace the module names "Fourier" and "Spectre" in the main text with "the four-channel analyzer" and "the spectrogram". Delete "and Codex" at `sections/evaluation-method.tex:78-79`.
3. **[Length]** Apply the cut list in M4. Delete `\input{sections/historical-evidence.tex}` from `fourier.tex`. Reduce `sections/inverse-and-chain.tex` to one ablation paragraph inside Results (M8). Move `tables/`-level Table 3 (`evaluation-method.tex:93-112`) and Tables 8 and 9 to the supplement. Target ≤8 DAFx pages including references.
4. **[Abstract]** Replace `sections/abstract.tex:2-22` with the M3 draft, with numbers via the existing macros (`\PaperCmpCoreTail` rounded to 2–3 significant figures, `\PaperCmpVdspTail`, `\PaperCmpCostRatio`, `\PaperCmpDelayMs`). Add one SIMD sentence using the Table 7 values.
5. **[Contributions]** Replace `sections/introduction.tex:38-56` with the bulleted contributions (M3) and a one-sentence result. Remove the historical-campaign roadmap sentences.
6. **[Host model]** Add a new section file `sections/host-model.tex` covering:
   - Rack's per-sample `process()`, block D, and engine threads with the per-frame barrier (cite the Rack Engine.cpp source);
   - the budget arithmetic (20.8 µs per sample; D/f_s per block);
   - why worker and UI-thread analysis were not chosen (M1).

   Insert it after `related-work`.
7. **[Budget up front]** Move `sections/engineering-limits.tex:12-17` into Results §6.1. Add a "peak % of D/f_s" column to the analysis table generator.
8. **[Mechanism figure]** Add `figures/one-hop-timeline.tex` (TikZ, spec in M6 item 1) and reference it from §4.
9. **[Decision rule]** Add the decision-rule paragraph (M7.2) to the end of `sections/engineering-limits.tex` and restate it in `sections/conclusion.tex`.
10. **[Terminology]** Apply the M5 table globally. Use one term per concept, add a notation table (N, H, D, W, f_s, K), and remove "replacement campaign". Suggested sed targets: "credit(s)" → "work unit(s)"; "production core"/"core" → "one-hop scheduler"; "confirmation campaign" → "our evaluation".
11. **[Hedging]** Collect all scope caveats into one "Threats to validity" paragraph in §7 and delete per-sentence disclaimers in `comparison-results.tex` (e.g. `:23-24,44-45,96-99`), `related-work.tex:17-18`, and `abstract.tex:19-22`.

### Should-fix
12. **[Baselines]** Add an idiomatic vDSP (and PFFFT) analysis adapter in `benchmark/paper/` (vectorized window/pack/magnitude, scaling folded into the window, doubled ring) and a per-stage breakdown. Rerun the primary N=4096, D ∈ {64, 256} cells, and update Table 4 and Fig. 2 (M2).
13. **[Where it matters]** Add Fig. 3 (decision map) from the existing extension data (aligned and staggered instances, D, N). If feasible, add the barrier-harness or Rack-session experiment (M1.3).
14. **[Tradeoff figure]** Replace `generated/paper-comparison/figures/length.tex` in the main text with a cost-versus-peak Pareto plot (M6 item 2). Keep the length trend in the supplement.
15. **[Perception]** Add the latency-perception paragraph (M7.1) with a citation to ITU-R BT.1359-1 (**verify**), and discuss the 85 ms control-response latency in Table 8.
16. **[Shipped defaults]** Add a row or table for the shipped defaults: four lanes, N=2048, hop 1,440 samples, Flattop, o=2. Also run the Fourier extreme N=16384 with a 5 ms hop.
17. **[Staggering]** Promote the "16 staggered instances" result in `comparison-results.tex:106-109` to its own paragraph and include it in the decision rule.
18. **[Precision]** Round all µs values to at most 3 significant figures in text and macros (the timer tick is 41.7 ns). Note that the single-sample p99 of 0.042 µs is one tick.
19. **[Code consistency]**
    - `appendices/prototype-evaluation.tex:46-50`: update the test counts (2,180,346 assertions and 13 cases at HEAD) or pin the revision.
    - Add TSan to the listed sanitizers.
    - Mention the band-rebuild cost in Table 1's caption.
    - Mention Spectre's 6.3 MB per-instance mailbox storage.
20. **[Bibliography]** Convert to BibTeX with the DAFx style. Add Gardner 1995, a real-time audio programming reference, Smith SASP, and the Rack engine source. Update the vDSP citation to current docs. Verify the entries marked above.
21. **[Deposit]** Deposit the raw bundles with a DOI and update `appendices/availability.tex` and `appendices/reproducibility.tex:45-57`.

### Nice-to-have
22. Evaluate a completion horizon H_c ∈ {H/4, H/2, H} (a one-line quota change in `src/dsp/spectrum_analysis.hpp:88-89`, benchmark only). Plot the resulting burst-versus-latency curve.
23. Run the primary grid on one x86-64 host.
24. Measure in Rack: CPU meter and engine overload for 1/4/16 instances at 1 and 4 threads.
25. Add batched native multichannel baselines (FFTW `plan_many`, vDSP multiple-signal FFT; verify the API) and per-channel phase staggering.
26. Explain or drop the "Diagnostics 370,800" column in Table 9, and explain the 631.92 µs maximum.
27. Add a one-paragraph "Implementation notes for practitioners" covering:
    - skip analysis when inputs are unpatched;
    - randomize the initial frame phase across instances;
    - compute dB from power without a square root;
    - where the hybrid (library FFT plus scheduled stages) is the pragmatic choice.

## 8. What would move my score up

- **To Borderline/Weak Accept:** Reformat to 8 anonymized pages with the proposed outline, add the new abstract, contributions, decision rule, mechanism figure, and host-model section, report budget fractions in Results, and cut the historical and chain material. This is mostly writing, and the evidence already supports it.
- **To Accept:** Additionally:
  - an idiomatic native baseline plus a per-stage breakdown showing where the ~2× cost goes (M2);
  - one experiment showing a regime where the peak matters, either multi-threaded per-sample barriers or many aligned instances at realistic Rack block sizes (M1);
  - honest positioning of staggered batch and the PFFFT hybrid as practical alternatives.
- **To Strong Accept:** A completion-horizon knob that turns the result into a tunable burst-versus-latency curve, measured in a real Rack session on both ARM64 and x86-64, with the raw data deposited. That would be a paper plugin developers cite and copy.

---

*Sources consulted beyond the repository:*
- [DAFx26 Call for Papers](https://dafx26.mit.edu/call-for-papers): 8-page maximum, LaTeX template, double-blind with a `blind` option.
- [VCV Rack v2 `Engine.cpp`](https://raw.githubusercontent.com/VCVRack/Rack/v2/src/engine/Engine.cpp): per-frame `Engine_stepFrame`, worker/engine barriers, `system::resetFpuFlags()`, `onSampleRateChange` called under the engine mutex.

# Reviewer 2: Performance Methodology and Real-Time Validity

**Paper:** "Resumable FFT Scheduling for Real-Time Spectral Analysis" (technical report, manuscript version 3, 38 pp.)
**Target venue:** DAFx

**Reviewer persona:** I work on real-time systems and performance engineering (computer architecture, benchmarking methodology, embedded/real-time audio, WCET). I have reviewed for RTAS and ISPASS as well as DAFx and AES. By default I distrust performance numbers until the methodology behind them holds up.

**Confidence:** 4/5. I read all main-body sources, the relevant appendices, the benchmark harness (`benchmark/paper/*`), the production DSP (`src/dsp/spectrum_analysis.hpp`, `src/SpectrumAnalyzer.cpp`, `src/rack_extensions/display_mailbox.hpp`), the committed tables and the raw per-callback observations for the headline workload. I also recomputed the headline numbers and ran small replication experiments on my own machine. I could not render the PDF figures as images. I read the PDF text through PDFKit, so figure-layout comments rely on the TikZ sources.

**Overall recommendation: Weak Reject.** The fixes are clear and mostly cheap. If the must-fix items below are addressed, I expect to move to Weak Accept or Accept.

| Criterion | Score (1–5) |
|---|---|
| Originality | 2 |
| Technical soundness | 3 |
| Experimental rigor | 3 (bookkeeping 5, external validity 2) |
| Significance | 2 |
| Clarity | 3 |
| Reproducibility | 4 |

---

## 1. Summary (in my words)

The paper describes the spectrum-analysis core of two VCV Rack modules. The core splits a whole windowed-FFT analysis frame (window/pack, radix-2 butterflies, real-FFT reconstruction, optional fractional-octave smoothing, EMA, output) into dependency-ordered unit operations. A balanced integer quota spreads these units evenly over the H samples of one hop. The design guarantees:

- at most ⌈W/H⌉ credits per sample call;
- publication at exactly H−1 samples after the frame endpoint;
- a bounded input-retention ring;
- a triple-buffer mailbox for handing results to the UI.

The evaluation is a large, frozen, three-session campaign on one Apple M1 Pro: 1299 workloads and 11,691 processes. It compares the scalar core against:

- matched first-party batch and incremental controls;
- a PFFFT "hybrid" that schedules everything except an indivisible native FFT;
- "native" batch analyzers built on FFTW, PFFFT and vDSP.

The headline workload is N=4096, H=1024, D=64, 48 kHz. There, the core's simulated-callback p99 is 3.42 µs against 9.50 µs for vDSP (2.78× lower). Aggregate cost is 2.27× higher (34.98 vs 15.38 ns/sample), and the spectrum is published 21.3 ms later. The authors explicitly frame this as a conditional cost/burst/age tradeoff rather than a speedup or a deadline guarantee. Inverse jobs and overlap-save chains serve as counterexamples where optimized batch execution wins.

## 2. Strengths

1. **Unusually disciplined bookkeeping and provenance.**
   - Frozen designs (`data/comparison-012/design/freezes/`) and a documented pilot-then-confirm workflow. Pilot sizing is in `design/pilot-review.json`.
   - Every macro in the manuscript is generated from committed CSVs with a SHA-256 receipt (`generated/paper-comparison/receipt.json`).
   - I recomputed every headline macro from `data/comparison-012/tables/primary/process-timings.csv` and all of them match exactly (§6.1). This is better than most published performance work I review.
2. **Honest scoping.** The paper repeatedly separates:
   - the logical credit bound from a duration bound (`sections/production-successor.tex:46-58`);
   - p99 from maxima from WCET (`sections/evaluation-method.tex:131-135`);
   - a simulated budget exceedance from an audio underrun (`:131-132`).

   It also reports that native inverse jobs beat the incremental control on both cost and p99 (`sections/comparison-results.tex:73-79`). The sentence that p99 occupies only 0.26% vs 0.71% of the 1333 µs interval (`sections/engineering-limits.tex:12-17`) is exactly the caveat I would have demanded.
3. **Matched controls separate placement from kernel choice.** The scheduled-batch/hybrid pair and the legacy batch/incremental pair are the right way to attribute "where the work runs" separately from "which FFT".
4. **Clean timing hygiene in the harness** (`benchmark/paper/protocol.hpp`):
   - raw observations are preallocated (`rows.reserve`, `:153`);
   - CSV serialization happens after timing;
   - the correctness replay is separate from the timed pass (`:219-260`);
   - empty-timer calibration is retained without subtraction (`:170-176`);
   - cache pollution runs outside the timed region (`:202-204`);
   - background DSP is interleaved per sample (`:188-194`), which is closer to how Rack actually runs than a block-contiguous loop.
5. **Sound engine-to-UI handoff.** `DisplayMailbox` (`src/rack_extensions/display_mailbox.hpp:31-60`) is a textbook SPSC triple buffer with `acq_rel` exchanges and a `DIRTY` bit. The producer never waits or allocates, and the consumer's slot is never overwritten. I found no allocation, locking, logging or syscalls on the steady-state sample path of `SpectrumAnalysis::process` (`src/dsp/spectrum_analysis.hpp:299-380`). The one exception is the `reserve_hop` growth in `onSampleRateChange`, which the paper discloses (`sections/production-successor.tex:95-96`).
6. **Numerical auditing of every published bin.** Every published bin of every audited instance is checked against an independent binary64 reference, and the acceptance budgets were frozen before confirmation.

## 3. Major issues (prioritized)

### M1. The "native" baselines are dominated by unoptimized scalar glue, not by the FFT libraries. With an idiomatic vDSP adapter the headline p99 advantage largely disappears and the cost penalty roughly triples.

**Location.**
- Abstract `sections/abstract.tex:14-18`.
- `sections/comparison-results.tex:18-24`.
- Table 4 (PDF p. 12, `generated/paper-comparison/tables/analysis.tex`).
- Adapters: `benchmark/paper/external.hpp:44-68` (`ExternalAnalysis::process`) and `benchmark/paper/vdsp.hpp:119-133` (`forward_real_positive`).

**Problem.** The "vDSP native" analyzer does five scalar O(N) passes around a single `vDSP_fft_zrip` call:
1. A ring unwrap with an integer modulo per element, fused with windowing (`frame[i] = ring[(head+i)%ring.size()]*window[i]`, `external.hpp:55`).
2. A scalar even/odd deinterleave instead of `vDSP_ctoz` (`vdsp.hpp:122-125`).
3. A scalar scale-and-repack into `std::complex` (`vdsp.hpp:129-131`).
4. A scalar `std::abs` magnitude loop (`external.hpp:58-61`).
5. A second output-copy loop (`external.hpp:62-67`).

The PFFFT and FFTW adapters share the `ExternalAnalysis` wrapper. The adapters are correct, but "native vDSP" in the paper effectively means "vDSP FFT plus hand-written scalar glue". This matters because the paper's headline number is the duration of the one callback that contains this glue.

**Evidence (my experiment).**
- Scripts: `r2-work/vdsp_opt.cpp` and `r2-work/mini.cpp` in my scratch directory.
- Build: `clang++ -std=c++17 -O3 -funsafe-math-optimizations -march=armv8-a+fp+simd … -framework Accelerate`, the paper's first-party flags.
- Setup: I replicated the paper's vDSP analysis adapter exactly ("paper-style"). I also wrote an idiomatic variant: two-segment `vDSP_vmul` for ring unwrap + window, `vDSP_ctoz`, `vDSP_fft_zrip`, `vDSP_zvabs`, `vDSP_vsmul`. Its outputs agree with the paper-style adapter to 1.6×10⁻⁷ relative. Both were driven by the real `SpectrumAnalysis<float>` from `src/dsp` under the paper's protocol (N=4096, H=1024, D=64, 64+⌈N/H⌉ warmup hops, 2048 hops, steady_clock).
- **Host caveat:** my machine is an Apple M5 Max (macOS 26.6.2, same Apple Clang 21.0.0), not the paper's M1 Pro. Only ratios should be compared.

| Implementation (M5 Max, unpaced, hot cache) | ns/sample | p99 per callback (µs) | median per-hop peak (µs) |
|---|---|---|---|
| Production core (`SpectrumAnalysis<float>`, trivial sink) | 20.7–20.9 | 2.29–2.33 | 1.88–1.92 |
| vDSP, paper-style adapter | 8.8–10.2 | 6.9–7.8 | 6.2–7.6 |
| vDSP, idiomatic adapter | 2.8–3.4 | 2.29–2.83 | 2.2–2.7 |
| `vDSP_fft_zrip` alone, N=4096 (tight loop) | – | 1.13 (p99), 0.92 median | – |

On my host, the paper-style vDSP/core p99 ratio is about 3.0–3.4, which reproduces the paper's 2.78 in direction. The idiomatic vDSP adapter has essentially the same callback p99 as the core at roughly one sixth to one seventh of the aggregate cost. About 88% of the paper-style vDSP "FFT callback" is glue: 7.7 µs total vs 0.92 µs for the transform itself, isolated microbenchmark on M5 Max.

With 8 MiB of cache pollution between callbacks, idiomatic vDSP rises to about 3.4 µs p99 while the core stays near 2.4 µs. So some advantage survives in cache-cold regimes (see also M3).

The paper's own data point the same way:
- The PFFFT hybrid's FFT-bearing callback costs about 5.3 µs (per-phase medians from raw `workload-0151`: 1.75 µs baseline, 5.33 µs in the FFT phase). That is only about 3.5 µs for PFFFT including its adapter packing.
- Native PFFFT analysis's single callback is 9.6 µs. The difference is the glue.

**Why it matters.** The abstract's central quantitative claim (p99 3.42 vs 9.50 µs, factor 2.78) is a comparison against a baseline a performance-literate reader would not write. That is a textbook straw-man risk even though the adapters are well documented. The cost-ratio claim (2.27×) is also too kind to the core; against an idiomatic baseline it is closer to 6–7×.

**Fix.**
1. Add an "idiomatic native" adapter per library to `benchmark/paper/external.hpp` and `vdsp.hpp`, as a new backend id (for example `vdsp-analysis-vec-float`) rather than replacing the existing one. Keep the current adapter labelled "portable scalar-glue adapter".
   - vDSP: `vDSP_vmul` ×2 for unwrap+window, `vDSP_ctoz`, `vDSP_fft_zrip`, `vDSP_zvabs`, scale folded into the window gain.
   - PFFFT: unordered `pffft_transform` (not `_ordered`) plus a NEON magnitude loop.
   - FFTW: r2c from a contiguous windowed buffer using a two-segment unwrap without `%`.
2. Rerun the primary N∈{2048, 4096, 16384}, D=64 rows at confirmation scale (3 sessions × 3 processes). This is minutes of machine time.
3. Restate the abstract and `comparison-results.tex:18-24` in terms of the stronger baseline, or report both baselines side by side with the idiomatic one as the primary comparator.
4. In `evaluation-method.tex:50-51`, replace "they are not advertised as bare provider-kernel calls" with an explicit statement of which glue loops are scalar in each adapter.

### M2. The benchmark measures the DSP class with a trivial sink, not the production module path. The module-boundary backends already exist in the harness but were not run.

**Location.** "Production core/analyzer" wording throughout: `sections/abstract.tex:15`, `sections/comparison-results.tex:18`, `sections/production-successor.tex:52-53` ("The per-call bound includes … bounded module output callbacks").

**Problem.** The measured adapter `Paper::Core` (`benchmark/paper/fourier.hpp:21-46`) instantiates `SpectrumAnalysis<T>` with the default `OutputWeight=1`, and its `emit` is a single store. The shipped Fourier module (`src/SpectrumAnalyzer.cpp:102, 513-521`) instead uses:
- `SpectrumAnalysis<simd::float_4, 2>`;
- a per-bin `coordinate_cache.map(bin, value)` that writes display coordinates for four lanes;
- per-sample DC blockers, trigger processing and a light divider (`:466-540`).

`benchmark/paper/modules.hpp` and `docs/whitepaper/benchmarks/lib/backends.json:311-333` already define `fourier` and `spectre` module-boundary backends ("module-display-coordinate-mapping"). No `fourier`/`spectre` backend appears in any confirmation `results.csv`; I checked all five groups. The unequal-unit-cost problem (M6) is likely worse with output weight 2 and the coordinate mapping, which is exactly the stage the paper says is "bounded".

**Why it matters.** The title and abstract are about real-time spectral analysis in a shipped product. The only product-level evidence would come from the module boundary, and it is absent. "Production core" suggests readers are seeing the module's cost.

**Fix.**
1. Run the existing `fourier` (4-lane, N=4096, H chosen to be representable, D=64) and `spectre` (N=2048, H=1024) module backends in the primary protocol: 3 sessions × 3 processes, throughput and callback passes.
2. Report them in Table 4/Table 7 as a "module boundary" block.
3. Until then, rename "production core" to "production analysis class (`SpectrumAnalysis<float>`, unit output sink)" in the abstract, Table 4, Table 7 and `comparison-results.tex`.
4. Weaken `production-successor.tex:52-53` to say the output callback cost is bounded per unit only if the callback is constant-time. Also state that it was not timed in the confirmation campaign.

### M3. The harness measures a hot, unpaced, 100%-duty-cycle regime that no audio host produces. On my machine, paced callbacks are 4–5× slower, and the rankings partly change.

**Location.** `sections/evaluation-method.tex:123-135`, `:51-52`; `benchmark/paper/protocol.hpp:198-209`. The callback pass runs the D-sample callbacks back to back with no idle time between them.

**Problem.** A 64-sample callback at 48 kHz arrives every 1333 µs. The analyzer occupies only 0.2–0.7% of that interval, so between callbacks the core idles or runs other modules. Its caches, branch predictors and DVFS state then differ from a tight loop. The paper never mentions pacing, DVFS, or heterogeneous cores; I grepped all sections and appendices for "E-core", "QoS", "DVFS", "paced", "duty" and "frequency" in that sense.

**Evidence (my experiment, M5 Max; script `r2-work/paced.cpp`, `r2-work/paced_rt.cpp`).** I used the same three implementations as in M1 and inserted `usleep(1300)` between callbacks (256 hops), with no cache pollution. I ran it under `QOS_CLASS_USER_INTERACTIVE`, and a second time under `THREAD_TIME_CONSTRAINT_POLICY` (period 1.333 ms, computation 0.2 ms), which is closer to how CoreAudio runs I/O threads.

| Regime | core p99 | vDSP paper-style p99 | vDSP idiomatic p99 | core ns/sample |
|---|---|---|---|---|
| Unpaced (paper protocol) | 2.3 µs | 7.8 µs | 2.3–2.8 µs | 20.8 |
| Paced, user-interactive QoS | 10.4–11.2 µs | 32.2 µs | 15.5–15.8 µs | 85–97 |
| Paced, time-constraint policy | 10.5 µs | 29.3 µs | 16.5 µs | 95 |
| Unpaced, background QoS (E-cores) | 9.7 µs | 26.6 µs | 10.7 µs | 89 |

Absolute durations inflate 4–5× when callbacks are paced, even under a real-time policy. The cause could be frequency ramp-down, cache/TLB decay, or core migration; I did not isolate it. In the paced regime the core beats even the idiomatic vDSP adapter by about 1.5× in p99. In the unpaced regime they tie.

So the paper's absolute microsecond values are specific to a regime a host does not produce, and the relative conclusion depends on regime and baseline quality. This cuts both ways: it may strengthen the paper's case under realistic pacing. The paper currently has no evidence for either regime beyond the hot one.

**Fix.**
1. Add a paced callback mode to `protocol.hpp`. Sleep or `mach_wait_until` to the next D/fs deadline (absolute-time schedule, not relative sleep), optionally touching a configurable "other modules" working set of 0 / 1 / 8 MiB.
2. Run the primary D=64, N=4096 rows paced at 48 kHz (32768 callbacks ≈ 44 s per process; 256 hops would be enough).
3. Run each regime under both default QoS and `THREAD_TIME_CONSTRAINT_POLICY`.
4. Report whether conclusions hold across regimes in a small table beside Table 4.
5. State explicitly in `evaluation-method.tex` that callbacks are unpaced, and what that implies for DVFS and cache state.

### M4. No control over or diagnosis of heterogeneous-core placement. About 1% of processes run about 3× slower, which the paper interprets as implementation behaviour.

**Location.**
- `sections/comparison-results.tex:43-47` ("The hybrid's large throughput variation at the longest length is retained: its session means span 128.15–215.10 ns … A single favorable run would misrepresent this observation.").
- Figure 1 (PDF p. 13; `generated/paper-comparison/figures/length.tex:10`).
- `sections/evaluation-method.tex:78-81`.

**Evidence.**
- *Outlier scan* (`r2-work/outliers.py`). I compared each throughput process with the median of its 9 same-configuration processes. In the primary group, 6 of 837 processes are more than 1.5× slower and 5 are more than 2.5× slower. They cluster at about 3×: 3.03, 2.96, 3.04 and 3.60, plus one 8.46.
  - The hybrid N=16384 unsmoothed "variation" is entirely one process: session 01, repeat 0 at 390.32 ns/sample against 126.4–128.9 for the other eight. (390.32+128.57+126.40)/3 = 215.10, exactly `\PaperCmpHybridLargeHigh`. The median of the nine is 128.88 against the plotted 157.39, so the plot overstates it by 22%.
  - The same about-3× signature appears in callback passes: PFFFT scheduled batch at N=4096, session 02 repeat 1 has p50 1.083 vs 0.333 µs and p99 98.3 vs 32.5 µs; PFFFT native at N=4096 has p50 1.125 vs 0.375 µs.
- *E-core behaviour on my machine* (M5 Max). Background QoS forces E-cores and slows every implementation 3.5–4.5× (table in M3). An about-3× cluster on M1 Pro (Icestorm at about 2.06 GHz vs Firestorm at about 3.2 GHz, with a narrower pipeline) is consistent with the whole process landing on an E-core or a low-frequency state. I cannot prove the mechanism from the retained data because no per-process core type or cycle count was recorded.
- *Host state.* The snapshots also show the host was not quiet. `host/pre-confirm-01.json` reports load average 1.96 and `sandboxd` at 90% CPU. `host/post-confirm-03.json` reports load 3.52 with WindowServer 11.7%, "Codex (Renderer)" 11.6% and ChatGPT 8.4%.
- *Session 03 stalls.* Session 03 has 34 callback processes with a ms-scale stall (max > 1 ms and > 5× p99), vs 5 and 10 in sessions 01 and 02. The maxima reach 81.2 ms (`pffft-ols-identity-float`) and 35.4 ms (`core-double`, N=16384).

**Why it matters.** The paper attributes this "variation" to the hybrid. More generally, mean-of-means aggregation with 3 processes per session is not robust to a 3× contamination, and a practitioner reading Figure 1 would draw the wrong conclusion about the hybrid at N=16384.

**Fix.**
1. Run benchmark processes with `pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0)`, or preferably a time-constraint policy, to match audio threads.
2. Record per-process core type or frequency evidence. Options: `proc_pidinfo`/`thread_info` CPU time plus wall time; `kperf`/`kpc` cycle counters where available; or a 5 ms calibrated spin loop before and after measurement as a frequency proxy.
3. Add a pre-registered, mechanism-based exclusion or flag rule (for example: "calibration spin more than 1.5× the host median marks the process as E-core/low-frequency"). Report flagged processes rather than silently dropping them.
4. Report the median of processes alongside the mean of session means, and redraw Figure 1 with per-process dots (strip plot) instead of min–max bars.
5. Rewrite `comparison-results.tex:43-47` to attribute the range to a single slow process and state the diagnosis.

### M5. p99 per callback is a phase-aliased statistic. The paper should report a phase-invariant tail metric and deadline-miss rates at realistic budgets.

**Location.** `sections/evaluation-method.tex:128-135`; Tables 4–7; `sections/comparison-results.tex:122-129`.

**Problem.** With H/D = 16 callbacks per hop, a batch implementation's FFT lands in 6.25% of callbacks. Its p99 is therefore about the 84th percentile of FFT-bearing callbacks, while the core's p99 is a true 1% tail of a near-periodic workload. At D=1, H/D = 1024 and p99 misses the FFT completely, which the paper calls a "percentile failure" (`:122-129`). At D=16, H/D = 64, just above the p99 threshold. So the metric's meaning changes with H/D, which is a design parameter.

**Evidence.** I recomputed from raw callbacks: `r2-work/tail.py` on primary bundle workloads 0144–0151 (N=4096, H=1024, D=64, unsmoothed), 9 processes each.
- The per-hop peak callback (max over the 16 callbacks of each hop, 2048 hops per process) has a median that tracks p99 closely: core 3.33–3.46 vs p99 3.38–3.50; vDSP 9.33–10.29 vs 9.42–10.79. The metric is therefore sensible here, but only by coincidence of H/D.
- The p99.9 and p99.99 tails are dominated by interference, not by the algorithm. Core p99.99 ranges 4.2–30.6 µs across processes and vDSP 10.2–22.4 µs.
- Callbacks over 20 µs across 9 processes number 24 for the core, 10 for vDSP, 21 for the hybrid and 13 for FFTW. The core runs about 2.3× longer in total, so it has proportionally more exposure to preemption. That explains why Table 4's core maximum (70.12 µs) is more than twice vDSP's (33.29 µs). The paper notes maxima rank differently (`comparison-results.tex:62-69`) but does not give this mechanism: higher aggregate busy time means more interrupts landing inside timed callbacks.
- The harness already computes `observed_compute_budget_exceedances` per process, but the paper never reports it. Primary group totals: 53,076 exceedances in 55 processes. Almost all are deterministic: scalar batch overlap-save and inverse controls at N=16384, D=16 exceed the 333 µs budget on every hop (18,432 each for `ols-identity-batch` and `ols-fir-batch`). There are also stochastic ms stalls, including `core-float` N=16384 D=16 at 18.6 ms. These are the most "real-time relevant" numbers in the dataset and they are omitted.

**Fix.**
1. Define and report the per-hop peak callback distribution (median and p99 across hops) as the primary concentration metric. It is invariant to H/D phase aliasing and directly interpretable as "worst callback per publication".
2. Add ECDF plots (log x-axis) of callback duration for Table 4's eight implementations, pooled across the 9 processes but colored by process, with the budget line. Make this a new generated figure.
3. Report deadline-exceedance counts per million callbacks at fractional budgets of 1%, 5% and 10% of D/fs, to emulate a patch where the analyzer gets a small share. Use D ∈ {16, 64, 256}, and give Clopper–Pearson intervals for zero-count cells.
4. Report the deterministic batch-control exceedances at D=16 explicitly. They are the strongest argument for distribution in the paper, and they are hidden.
5. Explain the maxima mechanism (exposure proportional to busy time) in `comparison-results.tex:62-69`.

### M6. The balanced quota balances credits, not time. The core's own callbacks vary 2.2× by phase, which caps its p99 benefit.

**Location.** `sections/production-successor.tex:31-58`; `appendices/evaluation-agenda.tex:62-70`.

**Evidence** (`r2-work/tail.py`; per-phase median over 2048 hops of core `workload-0144`, session 01 repeat 0). Callback medians by phase within the hop (µs):

`[2.62, 2.54, 2.12, 2.00, 2.08, 2.29, 2.29, 2.04, 1.96, 1.92, 1.92, 1.92, 3.00, 3.33, 1.75, 1.50]`

Max/min is 3.33/1.50 = 2.2 and max/mean is about 1.5. The p99 (3.42 µs) is set by phases 12–13, which are the reconstruction and magnitude (`abs` → sqrt) stages. A perfectly time-balanced schedule with the same total work would have a peak near 2.2 µs, 35% lower than the reported p99. The paper acknowledges "units have unequal costs", but it does not quantify this or discuss that the headline p99 is set by the worst stage.

**Fix.**
1. Add a per-phase callback profile figure for the headline configuration. The raw data already contain the sample index, so this is a generator change only.
2. Report peak/mean callback ratio as a "balance efficiency" metric for the core, hybrid and legacy incremental.
3. Optionally, evaluate one calibrated weight vector (for example reconstruct=2, output=1) as an ablation. It needs no hardware-specific claim, only a sensitivity result.

### M7. External validity: one host, one OS, synchronous harness, no Rack engine. The Rack engine's per-frame execution model makes the relevant granularity different from what was measured.

**Location.** `sections/evaluation-method.tex:51-58`; `sections/engineering-limits.tex:57-65, 74-80`; `appendices/evaluation-agenda.tex:34-51`.

**Problem.**
1. **Platform.** All data come from one Apple M1 Pro on macOS 26.6.2 during one night. Most VCV Rack users are on Windows/x86-64 and many on Linux (please give a source or drop the implication). In the same paper, x86 AVX changes PFFFT/FFTW relative cost, and the core has no x86-specific path.
2. **Rack's engine is per-frame, not per-block.** I checked Rack v2's `src/engine/Engine.cpp` (fetched from GitHub; please verify against the exact SDK revision). Rack calls every module's `process()` once per frame. When `threadCount > 1`, worker threads synchronize at an engine barrier and a worker barrier on every frame. So in a multi-threaded patch, a batch analyzer that does its whole FFT in one `process()` call stalls that frame's critical path while every other worker spins. The block-level p99 then understates the batch penalty, and the paper's single-sample (D=1) group, where "p99 fails", is the more relevant granularity.
   - This likely strengthens the paper's case.
   - It is not modelled or discussed. `sections/signal-model.tex` and `evaluation-method.tex` talk about "callbacks of D samples" as if the analyzer's samples were contiguous.
3. **No in-host evidence.** There is no in-host measurement of Rack's engine thread (for example Rack's own per-module CPU meter or a patched `Engine::stepBlock` timer), and no device underrun data.

**Fix (cheapest first).**
1. *Text only.* Add a paragraph to `sections/signal-model.tex` or `evaluation-method.tex` explaining Rack's per-frame interleaving and barrier model. State that the benchmark's contiguous D-sample callback corresponds to single-threaded Rack, and that the D=1 per-frame peak is the relevant quantity for multi-threaded Rack.
2. *Cheap experiment (about 1 day).* Build a headless Rack engine harness: link Rack's `Engine`, instantiate N_mod ∈ {1, 16, 64} of a trivial module plus one Fourier or vDSP-analyzer module, set `threadCount` ∈ {1, 4}, call `stepBlock(64)` from a time-constraint thread at a paced 1.333 ms, and time each `stepBlock`. Report per-block p99, per-hop peak, and exceedances at 10% budget. This directly answers "does redistribution matter in Rack".
3. *Second platform (about half a day of machine time).* Rerun the primary D=64 N ∈ {2048, 4096, 16384} rows on one x86-64 Linux machine: `performance` governor, `isolcpus` or `taskset`, `SCHED_FIFO`, FFTW+PFFFT only, 3 sessions × 3 processes. The harness is already portable except for vDSP.
4. *Device level (optional).* Run a Rack patch with 16 Fourier instances at 64-sample buffers and log CoreAudio overload notifications (`kAudioDeviceProcessorOverload`). Report overloads per hour for the core vs a patched build using the idiomatic vDSP analyzer.

### M8. Venue fit: length and format.

The compiled PDF is 38 pages (PDFKit page count), single-column 11 pt. The main body runs to about page 19 before the references start. DAFx papers use the two-column DAFx template with a strict page limit; historically this is 8 pages, but please check the current call. The paper cannot be reviewed at DAFx in its current form. I raise it here because the triage affects which evidence stays: the historical campaigns (Sections 9 and appendices B–H, PDF pp. 18–35) and the transition suite should move to a companion artifact or technical report. The DAFx paper should carry:
- M1's stronger baseline;
- M5's phase-invariant tail metric;
- M3/M7's paced and Rack-engine evidence.

### M9. "Confirmation" without pre-stated hypotheses or decision rules; many comparisons without multiplicity control.

**Location.** `sections/evaluation-method.tex:84-91`; `sections/comparison-results.tex:3-11`.

**Problem.** The design was frozen (good), but I found no pre-stated hypotheses, primary outcome, effect-size threshold or decision rule. I checked `design/*.json`, `specs/archive/012-*.md` and `specs/013-comparison-paper.md`; the only pre-set criteria are the numerical acceptance budgets. The headline configuration (N=4096, D=64) is justified post hoc as "the middle length, not the one with the largest effect" (`comparison-results.tex:14-15`). That is reasonable, but the whole study has 1299 workloads and the prose picks examples from Table 6 in both directions. The historical campaign reported bootstrap intervals (`appendices/experimental-method.tex:68-74`); the confirmation campaign reports only observed ranges.

**Fix.**
1. Either call the study "descriptive/exploratory with frozen design", or state the primary outcome retroactively and honestly. For example: "sign of Δp99 and Δcost for core vs each native backend at the 9 primary (N, D) cells".
2. Report the sign-consistency count across all primary cells, for example "core p99 < vDSP p99 in k/9 cells, cost higher in 9/9". The generator already has every cell, so this is simple arithmetic.
3. Give hierarchical bootstrap intervals for the headline ratios (sessions, then processes within sessions). My computation (`r2-work/boot.py`, 20,000 resamples):
   - cost ratio core/vDSP = 2.27, 95% interval [2.14, 2.47];
   - p99 ratio vDSP/core = 2.78, 95% interval [2.72, 2.80].

   These intervals are narrow. The within-host statistical uncertainty is not the problem; the systematic threats in M1–M4 and M7 are. Saying so explicitly would strengthen the paper.

## 4. Minor issues

1. **Timer resolution should be stated as a mechanism.** `steady_clock` on macOS/arm64 ticks at 24 MHz (41.67 ns). The raw values are quantized to 41/42 ns steps, and 63% of empty-timer reads are 0 (my histogram on `workload-0144`: 0 ns ×645, 42 ×256, 41 ×123). Say this in `evaluation-method.tex:125-133` rather than only "sub-tick observations". The headline p99 values are integer tick counts: 3.417 µs = 82 ticks, 9.500 µs = 228 ticks. Session ranges such as [9.42, 9.50] are ±2 ticks and should be presented that way.
2. **Throughput pass length.** The timed throughput interval is only about 73 ms per process (`runtime.json` `measurement` = 73.38 ms for `workload-0144`), while the whole process runs 670 ms, mostly correctness replay. DVFS ramp and scheduler placement at process start can matter at that timescale. State the timed duration per process, and consider 3–5 consecutive timed throughput chunks per process to detect within-process drift.
3. **Bimodal process costs.** vDSP N=4096 throughput per-process costs are bimodal: {14.2, 14.3, 14.2, 14.2, 14.2} and {16.1, 16.4, 16.5, 16.1, 16.5}. That is plausibly a layout/alignment effect (Mytkowicz et al., cited). Mention it; with 3 processes per session, the session mean depends on which mode each process lands in.
4. **The PFFFT scheduled-batch control is expensive by construction.**
   - It dispatches `unit()` per element with an if-chain and `%` per sample (`benchmark/paper/hybrid.hpp:57-76`), costing 36.6 ns/sample vs 15.7 for PFFFT native with the same kernel.
   - Its "surrounding stages" cost about 21 µs per frame, which native PFFFT does in about 6 µs.
   - The hybrid-vs-scheduled-batch comparison therefore isolates placement of an inflated surrounding workload. Say so in Table 2's caption and `comparison-results.tex:28-33`.
5. **FFTW/PFFFT configuration.**
   - FFTW double has no NEON on ARM (`build_fftw.py:82` correctly omits it); say so in the text, since the double-precision rows in Table 7 compare against a scalar FFTW.
   - PFFFT uses the ordered transform. For magnitude-only analysis, the unordered transform with an order-agnostic magnitude pass is the idiomatic choice.
   - FFTW `MEASURE` without `PATIENT`/wisdom is fine, but state that PATIENT was not tried.
6. **Table 7's four-channel comparison** compares one 4-lane SIMD core against four independent scalar-glue native analyzers. A 4-channel vDSP analyzer using strided `vDSP_ctoz` or batched FFT setups (`vDSP_fftm_zrip`) is the matched native competitor. At minimum, weaken "aggregate cost is also below the measured banks of native float analyses" (`comparison-results.tex:133-135`) to name the scalar glue.
7. **Figure 1 uses min–max bars over session means** (`length.tex`). Replace them with per-process points. Log-y on cost hides the 22% hybrid contamination described in M4.
8. **Abstract caveat.** The abstract should carry the absolute context from `engineering-limits.tex:12-17` ("both p99s are below 1% of the 1.33 ms callback interval"). A reader of the abstract alone will over-interpret "2.78×".
9. **Maxima on a busy host.** `comparison-results.tex:62-66` cites the core's 631.92 µs maximum at N=2048 "despite a small session-median p99". That single process (session 01, repeat 2) also has p99 10.7 µs vs 1.92 µs for its siblings, a whole-process disturbance. Tie it to the host snapshots (`host/pre-confirm-01.json`: `sandboxd` 90% CPU) rather than leaving it unexplained.
10. **`reserve_hop` allocation.** `src/SpectrumAnalyzer.cpp:235` calls `analysis.reserve_hop(...)` in `onSampleRateChange`, which can allocate (`spectrum_analysis.hpp:231-236`). Please verify on which thread Rack 2 invokes `onSampleRateChange` (I believe the engine thread during `setSampleRate`) and state it. This is disclosed, but a sentence with the thread identity would close the loop.
11. **Mailbox assert.** `display_mailbox.hpp:33` asserts `ATOMIC_INT_LOCK_FREE == 2` for a `std::atomic<unsigned>`. It is correct (the macro covers unsigned int), but `static_assert(std::atomic<unsigned>::is_always_lock_free)` is more direct.
12. **Transition table** (Table 8). "Change max" pools request-window overlaps. Report per-event exceedances of the stationary per-hop peak instead of a single maximum over nine processes.
13. **"Engine thread" vs "audio callback thread".** In Rack 2 the engine is driven from the audio device's callback thread, which is a real-time thread under CoreAudio (verify for each driver). The harness runs on a default-QoS thread. State this mismatch explicitly (see M3/M4).
14. **Missing methodology references** (all standard; please verify bibliographic details):
    - Georges, Buytaert and Eeckhout, "Statistically Rigorous Java Performance Evaluation", OOPSLA 2007.
    - Hoefler and Belli, "Scientific Benchmarking of Parallel Computing Systems", SC 2015.
    - Chen and Revels, "Robust benchmarking in noisy environments", arXiv:1608.04295, 2016.
    - Barrett et al., "Virtual Machine Warmup Blows Hot and Cold", OOPSLA 2017, on warmup and steady-state assumptions.
    - For audio real-time practice: R. Bencina, "Real-time audio programming 101: time waits for nothing" (2011, blog; verify URL), and Apple's documentation on audio workgroups and real-time threads on Apple silicon (verify title/URL).
    - For partitioned-convolution scheduling: Gardner, "Efficient convolution without input-output delay", JAES 1995.

## 5. Artifact audit

### 5.1 Recomputations

All from committed CSVs, read-only; the scripts are in my scratch directory `r2-work/`. `recompute.py` filters `data/comparison-012/tables/primary/process-timings.csv` to n=4096, hop=1024, block=64, smooth=0, count=1, voices=1. It takes means of processes per session, the equal-weighted mean of session means for cost, and the median of per-session process-p99 medians.

| Macro / table cell | Paper | Recomputed | Status |
|---|---|---|---|
| `\PaperCmpCoreCost` | 34.98 [34.75, 35.15] | 34.98 [34.75, 35.15] | ✓ |
| `\PaperCmpVdspCost` | 15.38 [14.20, 16.24] | 15.38 [14.20, 16.24] | ✓ |
| `\PaperCmpCoreTail` | 3.417 | 3.417 [3.375, 3.459] | ✓ |
| `\PaperCmpVdspTail` | 9.500 | 9.500 [9.417, 9.500] | ✓ |
| Legacy batch / incremental cost, p99 | 46.73 / 43.51; 42.92 / 8.46 | identical | ✓ |
| PFFFT native / FFTW native (Table 4) | 15.67, 9.62 / 16.32, 11.75 | identical | ✓ |
| Core max (Table 4) | 70.12 | 70.125 (session 03, repeat 1) | ✓ |
| `\PaperCmpCostRatio`, `\PaperCmpTailRatio` | 2.27, 2.78 | 2.2749, 2.7802 | ✓ |
| `\PaperCmpHybridLargeLow/High` | 128.15 / 215.10 | 128.15 / 215.10; the high value is driven by one 3.03× process | ✓ value, ✗ interpretation (M4) |
| `\PaperCmpDelayMs`, `\PaperCmpCoreAgeMs`, `\PaperCmpBatchAgeMs` | 21.31, 63.97, 42.66 | 1023/48000, (2047.5+1023)/48000, 2047.5/48000 | ✓ |
| `\PaperCmpCoreBudgetPercent`, `\PaperCmpVdspBudgetPercent` | 0.26, 0.71 | 3.417/1333.33, 9.5/1333.33 | ✓ |

**Raw-callback checks.** I extracted primary bundle workloads 0144–0151 and 0160–0167 (`bundles/primary.tar.gz` → `r2-work/raw/`). The 9 processes per workload reproduce the committed per-process p99 and maximum values. Each process has 32,768 callbacks, 1024 timer rows and 2048 publication rows, as documented.

### 5.2 Discrepancies and gaps

- **Interpretation.** The hybrid N=16384 "variation" (`comparison-results.tex:43-47`) is one slow process (M4).
- **Missing module measurement.** Module-boundary backends are defined but unmeasured (M2).
- **Omitted exceedance data.** The harness computes `observed_compute_budget_exceedances` and `compute_budget_exceedance_fraction`, but the paper does not report them (M5).
- **Host logs.** The host snapshots show non-trivial concurrent load in every session. The paper says "ordinary macOS services and Codex remained possible confounds" (`evaluation-method.tex:78-79`); the snapshots name `sandboxd` at 90% and ChatGPT/Codex renderers at about 8–12% CPU. Quote them.
- **Raw data not deposited.** The raw per-callback bundles (about 1.5 GB) are local and untracked (`appendices/reproducibility.tex:45-57`). A third party can check the tables but cannot rederive any tail statistic, per-hop peak or phase profile. For an artifact-evaluation badge they must be deposited (for example on Zenodo with a DOI).
- **Rebuilding the executables.** Native executables and Rack SDK bytes are omitted by design. A third party needs the exact Rack SDK version to rebuild; name it with its version and hash in the paper, not only in bundle metadata.

### 5.3 Could a third party reproduce the results?

- **Tables and macros from committed data:** yes. `tools/comparison_paper.py --check` is well designed.
- **Statistics from raw observations:** no, not without the undeposited bundles.
- **Timings on a new host:** mostly yes, given the Rack SDK and the documented `bench.py run --freeze … --variant macos`. The absence of QoS/pacing control means results will depend on the machine state (M3/M4).

### 5.4 Claims that check out

- Every headline macro.
- The engine-to-UI ownership design.
- No steady-state allocation or locking on the analyzer path.
- Callback p99 is stable across processes and sessions within about ±2 timer ticks for the headline cells.
- Within-host sampling uncertainty on headline ratios is small (hierarchical bootstrap in M9).

## 6. Questions for the authors

1. Did any confirmation process run on an E-core? Can you instrument per-process frequency or core type (M4)? Would you accept a pre-registered flagging rule?
2. Why were the `fourier` and `spectre` module-boundary backends not included in the confirmation grid? Do you have pilot data for them?
3. Were the scalar glue loops in `ExternalAnalysis` and `VdspBackend` a deliberate portability choice? Would you object to adding idiomatic vectorized adapters as additional contenders (M1)?
4. What is the per-phase callback profile of the core with `OutputWeight=2` and the display coordinate mapping (Fourier module)? Does the peak phase move to the output stage?
5. In Rack 2 with `threadCount > 1`, do you agree that the per-frame critical path (D=1 granularity) is what a batch analyzer inflates? Would you add the headless multi-threaded `Engine::stepBlock` experiment (M7, item 2)?
6. Under paced execution, do your M1 Pro results show the same 4–5× inflation I see on an M5 Max? If so, which regime do you consider representative of an actual Rack session?
7. Which host activity is documented for session 03, which contains most of the ms-scale stalls?
8. What is the rationale for equal session weighting of process means rather than medians, given 3 processes per session?

## 7. Revision checklist (executable against the sources and tooling)

### Must-fix

- [ ] **MF1 (M1): add idiomatic native analysis adapters**, keeping the current ones.
  - vDSP: in `benchmark/paper/vdsp.hpp`, add `forward_magnitude_vec` using `vDSP_vmul` ×2 for unwrap+window, `vDSP_ctoz`, `vDSP_fft_zrip`, `vDSP_zvabs` and scale folded into the window gain.
  - PFFFT: unordered transform plus a vectorized magnitude pass.
  - FFTW: two-segment memcpy unwrap without `%`, r2c.
  - Wire them through `benchmark/paper/external.hpp` (new `ExternalAnalysisVec`) and register new backend ids in `docs/whitepaper/benchmarks/lib/backends.json`.
  - Run the primary cells N ∈ {2048, 4096, 16384}, D ∈ {16, 64, 256}, H=1024 at 3 sessions × 3 processes.
  - Regenerate with `tools/comparison_paper.py`. Update Table 4, Figure 1, the abstract and `sections/comparison-results.tex:18-24`.
- [ ] **MF2 (M2): measure the shipped module.** Add the existing `fourier` and `spectre` module-boundary backends to the primary run. Add a "module boundary" block to Table 4 or Table 7.
  - Until then, replace "production core"/"production analyzer" with "production analysis class (unit output sink)" in `sections/abstract.tex:15`, `sections/comparison-results.tex:18`, `generated/…/tables/analysis.tex` row labels (edit via the generator) and `sections/production-successor.tex:52-53`.
- [ ] **MF3 (M3): add a paced callback mode** to `benchmark/paper/protocol.hpp:198-209`. Use an absolute-deadline wait (`mach_wait_until` / `clock_nanosleep TIMER_ABSTIME`) at D/fs and an optional between-callback working-set touch in MiB. Run the Table 4 row set paced under the default policy and a time-constraint policy. Add a regime-comparison table and a sentence in `sections/evaluation-method.tex:123-135` stating that the existing results are unpaced.
- [ ] **MF4 (M4): control or diagnose core placement.**
  - Set the thread QoS or policy in the benchmark driver (`benchmark/paper/benchmark.cpp` main).
  - Record a pre- and post-measurement calibration spin per process in `runtime.json`.
  - Flag processes more than 1.5× the host-median spin.
  - Rewrite `sections/comparison-results.tex:43-47` to attribute the hybrid N=16384 range to one process (session 01, repeat 0, 390.32 ns/sample).
  - Redraw Figure 1 with per-process points.
- [ ] **MF5 (M5): report phase-invariant and deadline metrics.** In `tools/comparison_paper.py` and the report library (`docs/whitepaper/benchmarks/lib/report.py` / `observations.py`), compute:
  - (a) the per-hop peak callback (median, p99 across hops);
  - (b) exceedance counts per 10⁶ callbacks at 1%, 5% and 10% of D/fs, with Clopper–Pearson 95% intervals;
  - (c) an ECDF figure for the Table 4 implementations.

  Report the deterministic D=16 batch-control exceedances. Add the maxima-exposure explanation to `sections/comparison-results.tex:62-69`.
- [ ] **MF6 (M8): conform to the DAFx template and page limit** (verify the current limit). Move Sections 9, the historical appendices (B, C, F, G, H), the transition suite and most of Table 6 to a companion technical report or artifact, and cite it.
- [ ] **MF7 (abstract): add the absolute-budget caveat** to `sections/abstract.tex` ("both p99 values are below 1% of the 1.33 ms callback interval").

### Should-fix

- [ ] **SF1 (M7): explain Rack's per-frame model.** Add a paragraph on per-frame `process()` interleaving and per-frame worker barriers (Rack v2 `src/engine/Engine.cpp`; cite the exact revision) to `sections/signal-model.tex` or `sections/evaluation-method.tex`. Explain why D=1 per-frame peaks matter for multi-threaded Rack.
- [ ] **SF2 (M7): headless Rack-engine experiment.** Paced `stepBlock(64)` on a time-constraint thread; `threadCount` ∈ {1, 4}; 1/16/64 filler modules plus one analyzer (core vs idiomatic vDSP). Report per-block p99, per-hop peak and exceedances.
- [ ] **SF3 (M7): one x86-64 Linux replication** of the primary D=64 cells (FFTW, PFFFT, core, hybrid) under the `performance` governor and `SCHED_FIFO` with pinned cores.
- [ ] **SF4 (M6): per-phase callback profile** figure for the core, hybrid and legacy incremental at the headline configuration; report peak/mean ratio. Optionally one reweighted-quota ablation.
- [ ] **SF5 (M9): honest framing.** Relabel the study "frozen-design descriptive comparison" or state a retroactive primary outcome. Add a sign-consistency summary over all primary cells and hierarchical bootstrap intervals for headline ratios. Update `sections/evaluation-method.tex:145-152`.
- [ ] **SF6: deposit raw bundles** (DOI) and name the Rack SDK version and hash in `appendices/reproducibility.tex`.
- [ ] **SF7: quote host snapshot load** (load averages, top processes) per session in `sections/evaluation-method.tex:73-81`, and relate session 03 to its higher stall count (34 vs 5/10 processes with ms stalls).

### Nice-to-have

- [ ] **NH1:** state the timer as 24 MHz / 41.67 ns ticks and present p99 values and ranges in ticks where they are small (minor 1).
- [ ] **NH2:** split the throughput pass into 3–5 consecutive timed chunks per process to expose within-process drift (minor 2).
- [ ] **NH3:** note bimodal per-process vDSP costs and randomize the environment and stack offset per process, following Mytkowicz et al. (minor 3).
- [ ] **NH4:** disclose scheduled-batch control overhead relative to native PFFFT in the Table 2 caption (minor 4).
- [ ] **NH5:** a 4-channel vDSP analyzer with batched FFTs (`vDSP_fftm_zrip`) for Table 7 (minor 6).
- [ ] **NH6:** add the methodology references in minor 14, after verification.
- [ ] **NH7:** replace `ATOMIC_INT_LOCK_FREE` with `std::atomic<unsigned>::is_always_lock_free` in `src/rack_extensions/display_mailbox.hpp:33`.

## 8. What would move my score up

- **Weak Reject → Weak Accept** requires MF1 (idiomatic baselines), MF4 (placement control with a corrected Figure 1 and hybrid text), MF5 (per-hop peak plus exceedances) and MF7, with the conclusions restated honestly under the stronger baselines, whichever way they fall.
- **Weak Accept → Accept** additionally requires MF3 (paced regime) and either SF2 (headless Rack engine) or MF2 (module boundary). Together these connect the "real-time spectral analysis" title to evidence from a host-like execution model.
- My prior after my own smoke tests is that the paper's qualitative story may survive, and may even look better, under realistic pacing and multi-threaded Rack. But the current headline number rests on a weak baseline and an unrealistic regime.
- **Accept → Strong Accept** would need a second architecture (SF3) and device-level overload counts, which I do not consider necessary for DAFx.

---

### Appendix: reviewer scripts

All in `/private/tmp/claude-501/-Users-ckauten-Desktop-fourier/d943b738-493a-4473-895a-cfe5b2a683ce/scratchpad/r2-work/`. Nothing was written to the repository.

- `recompute.py`: recomputes the headline macros from `process-timings.csv`.
- `perproc.py`, `outliers.py`: per-process listings and the slow-process scan (ratio to the configuration median).
- `tail.py`: raw-callback tails, per-hop peaks, per-phase means. Reads `raw/`, extracted from `bundles/primary.tar.gz` for workloads 0144–0151 and 0160–0167.
- `boot.py`: hierarchical bootstrap of the headline ratios.
- `vdsp_opt.cpp`: isolated paper-style vs idiomatic vDSP analysis microbenchmark.
- `mini.cpp`, `paced.cpp`, `paced_rt.cpp`: minimal replication harness around `src/dsp/spectrum_analysis.hpp` (copied via `git archive HEAD src/dsp`). Runs unpaced, cache-polluted, background-QoS, paced-QoS and paced time-constraint regimes.

All of these ran on an Apple M5 Max, not the paper's M1 Pro. Compare ratios only.

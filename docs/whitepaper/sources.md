# Reference Verification

This record tracks primary-source checks for [bibliography.tex](bibliography.tex),
including the September 29, 2026 gap audit. It is a focused literature review
covering the implementation's mathematical basis, directly related scheduling
work, overlap-reuse alternatives, and host integration. It is not an
exhaustive bibliometric survey.

| Citation Key | Verification Source | Use In The Report |
| --- | --- | --- |
| `cooley1965` | [Original paper](https://www.cs.jhu.edu/~misha/ReadingSeminar/Papers/Cooley65.pdf), [IBM author record](https://research.ibm.com/publications/an-algorithm-for-the-machine-calculation-of-complex-fourier-series) | Radix-2 factorization; no claim of inventing the FFT |
| `sorensen1987` | [Illinois author publication record](https://experts.illinois.edu/en/publications/real-valued-fast-fourier-transform-algorithms/) | Real-input FFT literature; authors, pages, and DOI |
| `allen1977` | [Illinois author publication record](https://experts.illinois.edu/en/publications/a-unified-approach-to-short-time-fourier-analysis-and-synthesis/) | STFT framing and normalization context |
| `harris1978` | [IEEE publisher record](https://ieeexplore.ieee.org/document/1455106/) | Window properties and spectral-analysis interpretation |
| `lo1998` | [Publisher abstract](https://www.sciencedirect.com/science/article/abs/pii/S0165168498001522) | Split-radix computation during acquisition |
| `lo1999` | [University repository](https://ir.lib.nycu.edu.tw/items/bc91194b-a045-493b-869c-d02a40c5ef94), [original paper](https://ir.lib.nycu.edu.tw/bitstream/11536/31665/1/000082760300005.pdf) | Online spectral analysis and time distribution |
| `lomoving1999` | [Publisher abstract](https://www.sciencedirect.com/science/article/abs/pii/S0165168499000985) | Combined scheduling and overlap reuse |
| `borss2009` | [DAFx proceedings paper](https://www.dafx.de/paper-archive/2009/papers/paper_48.pdf), [archive record](https://www.dafx.de/paper-archive/details/TUYATRAM6blZcG8lR_NbmQ) | Sections 3.1-3.2, paper pp. 4-5: distributing partition work across cycles and staggering filters; motivates relative-phase checks, not an analyzer performance claim |
| `hurchalla2010` | [AES abstract and paper record](https://secure.aes.org/forum/pubs/conventions/?elib=15679) | Processor/memory load distribution across input blocks in one thread for low-latency convolution; abstract only, paper 8257, 129th convention |
| `battenberg2011` | [Author-hosted full paper](https://ericbattenberg.com/pdf/partconvDAFx2011.pdf), [author publication page](https://ericbattenberg.com/publication/partconv/) | Section 4 distributes forward transforms, spectral multiplication, and inverse transforms across callbacks; section 6 compares performance and implementation effort |
| `wefers2015` | [University record](https://publications.rwth-aachen.de/record/466561), [full dissertation](https://publications.rwth-aachen.de/record/466561/files/466561.pdf) | Section 6.8, especially printed pp. 182-183: manual preemption and short-transform subdivision; motivates a granularity comparison, not a performance ranking for this analyzer |
| `liu2017` | [Rutgers author bibliography](https://zoulab.engr.rutgers.edu/journal_article), [university publication record](https://www.researchwithrutgers.org/en/publications/optimal-time-distributed-fast-fourier-transform-application-to-on/) | Time-distributed FFT/IFFT in online control; volume 41, pages 114-124 |
| `prusa2016` | [DAFx proceedings paper](https://www.dafx.de/paper-archive/2016/dafxpapers/01-DAFx-16_paper_20-PN.pdf) | Section 4.2: worker-thread spectral visualization, lock-free input buffering, polling jitter, and computation/repaint delay |
| `jacobsen2003` | [Jacobsen's later exposition and references](https://www.dsprelated.com/showarticle/776.php) | Sliding DFT distinction; IEEE Signal Processing Magazine 20(2), 74-80 |
| `lyons2021` | [University author record](https://digital.library.adelaide.edu.au/items/14d4a218-cff8-47ca-aeae-eb543ab04b4f/full), [accepted manuscript](https://digital.library.adelaide.edu.au/dspace/bitstream/2440/133370/3/hdl_133370.pdf) | Abstract and bibliographic metadata: guaranteed-stable sliding networks and frequency-flexible analysis; no reproduced equations or performance ranking |
| `richardson2019` | [Author-deposited preprint and journal metadata](https://arxiv.org/abs/1707.08213) | Tree reuse across windows; published as Algorithm 991 in ACM TOMS 45(1), article 12 |
| `garrido2016` | [University-hosted postprint](https://www.diva-portal.org/smash/get/diva2:1014928/FULLTEXT01.pdf) | Sections III-VI: feedforward butterfly reuse, retained buffers, real-input symmetry, and limits of transferring hardware/MATLAB results to Rack |
| `park2014` | [Author publication list](https://home.sejong.ac.kr/~cspark/2.html), [author-uploaded manuscript](https://www.researchgate.net/publication/260522366_The_Hopping_Discrete_Fourier_Transform_sp_TipsTricks) | Sections 1 and 3-4: reuse across multi-sample hops; separates arithmetic reuse from within-hop work placement |
| `rafii2018` | [Author-hosted published paper](https://zafarrafii.com/Documents/Journals/Rafii%20-%20Sliding%20Discrete%20Fourier%20Transform%20with%20Kernel%20Windowing%20-%202018.pdf) | Printed pp. 88-90: window kernels, short Hann/Blackman kernels, and approximation when sparsifying general kernels; motivates matched windowing and accuracy checks |
| `eleftheriadis2023` | [University publication record](https://pure.qub.ac.uk/en/publications/energy-efficient-short-time-fourier-transform-for-partial-window-/), [accepted manuscript](https://pureadmin.qub.ac.uk/ws/portalfiles/portal/487442168/main.pdf) | Partial-overlap frequency decomposition and frequency-domain Hann windowing; fixed-point ASIC evidence does not establish CPU performance |
| `becoulet2021` | [Author-uploaded preprint](https://www.researchgate.net/publication/348040518_A_Depth-First_Iterative_Algorithm_for_the_Conjugate_Pair_Fast_Fourier_Transform), [published-paper record](https://www.researchgate.net/publication/349414360_A_Depth-First_Iterative_Algorithm_for_the_Conjugate_Pair_Fast_Fourier_Transform), [DOI](https://doi.org/10.1109/TSP.2021.3060279) | Iterative depth-first conjugate-pair FFT; constant auxiliary indexing space, not constant total FFT storage; motivates a separate resumable adaptation |
| `frigo2005` | [Authors' full paper](https://fftw.org/fftw-paper-ieee.pdf), [author publication record](https://fftw.org/~athena/abstracts/abstract8.html) | Optimized FFT implementations as an unmeasured comparison class |
| `pffft` | [VCV-maintained PFFFT repository](https://github.com/VCVRack/pffft), [Rack FFT wrapper](https://github.com/VCVRack/Rack/blob/v2/include/dsp/fft.hpp) | Practical Rack baseline; real-transform wrapper and ordered/unordered output; software citation, with the actual measured revision and SIMD configuration to be pinned per campaign |
| `vdsp` | [Apple vDSP Programming Guide](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/vDSP_Programming_Guide/UsingFourierTransforms/UsingFourierTransforms.html), [FFT setup API](https://developer.apple.com/documentation/accelerate/vdsp_create_fftsetup) | Platform baseline for macOS; reusable setup, real-transform packing and scaling; official API documentation rather than a research paper |
| `vanderbyl2016` | [Publisher abstract and section summaries](https://www.sciencedirect.com/science/article/abs/pii/S1051200416000142), [DOI](https://doi.org/10.1016/j.dsp.2016.01.008) | Floating- and fixed-point error across sliding DFT structures; motivates proposed numerical checks without reproducing uninspected rankings |
| `skare2024` | [DAFx proceedings paper](https://www.dafx.de/paper-archive/2024/papers/DAFx24_paper_56.pdf) | Sections 2.2 and 2.8, printed pp. 453-455: throughput versus buffer-processing tails and batch versus periodic invocation; actual DAW contention remains future work in that study |
| `balasubramaniam2026` | [DAFx proceedings paper](https://www.dafx.de/paper-archive/2026/papers/DAFx26_paper_16.pdf) | Sections 3.4-3.5 and 5.1, printed pp. 130-131 and 134: isolated versus controlled host-load measurements, distinct timing/underrun outcomes, and virtual-driver limitations; methodology only |
| `mytkowicz2009` | [Authors' publication page](https://sape.inf.usi.ch/publications/asplos09.html), [full paper](https://cs.uwaterloo.ca/~brecht/courses/Perf-Eval-Shared/readings/producing-wrong-data-asplos-2009.pdf) | Sections 1-3 and 7: layout/environment measurement bias and setup randomization; limits of the existing alternating-order experiment and guidance for future evaluation |
| `kalibera2013` | [University author record](https://kar.kent.ac.uk/33611/), [corrected author manuscript](https://kar.kent.ac.uk/33611/45/p63-kaliber.pdf) | Sections 4, 8, and 9: repetition levels, pilot experiments, and effect-size intervals; proposed future methodology, not a claim about the archived campaigns |
| `wilhelm2008` | [University-hosted published paper](https://www.es.mdh.se/pdf_publications/1258.pdf), [DOI](https://doi.org/10.1145/1347375.1347389) | Section 1 and Figure 1: observed extrema, actual worst-case execution time, and safe bounds; processor-state complications |
| `rack` | [Official plugin API guide](https://vcvrack.com/manual/PluginGuide) | Engine and SIMD context; actual behavior also checked in repository code |

## Source Access And Metadata

Some publisher pages require browser verification or a subscription. For
Hurchalla and Liu et al., the comparison is deliberately limited to the
scope established by their abstracts/records, supplemented by the open
Battenberg paper for its own implementation. The report does not attribute
uninspected algorithm details or performance results to inaccessible texts.
The Lyons and Howard comparison uses the university record and indexed
manuscript abstract; the repository's migrated PDF endpoint did not provide
readable full text during this revision. Its guarantees are attributed to
the proposed networks, not to all sliding DFT implementations.

Park and Ko's author-uploaded text is marked as a March 2015 draft; the
bibliography uses
the journal publication's 2014 date. Rafii's published article is November
2018, despite the IEEE society summary being dated January 2019. Wefers's
dissertation was defended in 2014 and published in 2015. The proposed
overlap-reuse and work-granularity experiments are this report's future work.
Bécoulet and Verguet's inspected preprint is dated 2020; the bibliography uses
the 2021 journal publication. The van der Byl and Inggs discussion is limited
to the indexed publisher abstract and section summaries; the full article was
not accessible. The suggested suspension, matched-window, and long-stream
accuracy experiments are this report's proposals.

The three latest additions were checked against full official proceedings
PDFs. Borß's paper uses local page numbers; no proceedings-wide range or DOI
was verified. Skare's PDF title is "General-Purpose GPU Audio Benchmark
Framework," although the archive uses an abbreviated title. The 2026 PDF
credits Balasubramaniam, Ramachandran, and Timoney; its archive landing page
omits Ramachandran. The bibliography follows the PDF. That study uses a single
Apple M3 and BlackHole virtual driver; it does not establish physical-interface
performance or rank our FFT backends.

PFFFT and Apple Accelerate/vDSP are named execution targets alongside FFTW.
The [external comparison plan](benchmarks/comparisons.md) tracks
adapter status, workload matching, and eligibility of measurement campaigns.
The local Rack wrapper was also inspected; it calls PFFFT's real transform
and exposes both output orders. This establishes availability in the inspected
SDK, not timing evidence. Adapter and measurement status must be read from the
current code and owning plan, rather than inferred from this bibliography.
No external library's published speed claims are imported into this report.

## Search Scope And Selection

This pass combined searches for time-distributed/resumable FFTs, cooperative
convolution, sliding/hopping STFTs, real-time spectral visualization, and audio
callback benchmarking with citation checks around the closest existing work.
It covered the DAFx archive through 2026, AES records, and primary author,
university, and publisher sources in the wider DSP literature. Search results
were screened for a specific claim the paper needs, not a target citation count
or a venue's share of the bibliography. This is a focused gap audit, not a
systematic review or evidence that no other relevant work exists.

The existing 28 references already cover the main mathematical and scheduling
alternatives. Three additions fill distinct gaps: earlier plugin load
distribution, audio-specific benchmark invocation, and controlled host
contention. The first belongs in related work; the latter two support the
proposed evaluation where their methods are relevant. Generic background claims
and the paper's own derivations do not need citation clusters.

Nearby work screened out of the manuscript includes:

-   [Donat-Bouillud, Giavitto, and Jacquemard (DAFx19)](https://www.dafx.de/paper-archive/2019/DAFx2019_paper_51.pdf):
    changes audio-graph sampling rates to trade quality for cost. Our configured
    analysis is retained; existing scheduling and timing references suffice.
-   [Russo (DAFx09)](https://www.dafx.de/paper-archive/2009/papers/paper_43.pdf):
    interactive sliding-DFT tools add little to the existing recursive-transform
    references and Prusa/Holighaus visualization comparison.
-   [Bradford, ffitch, and Dobson (DAFx08)](https://www.dafx.de/paper-archive/2008/papers/dafx08_63.pdf):
    constant-Q sliding analysis would matter if the representation changed.
    Fourier currently smooths fixed-window FFT magnitudes.
-   [Skare (DAFx20)](https://www.dafx.de/paper-archive/2020/proceedings/papers/DAFx2020_paper_73.pdf):
    GPU execution patterns are less directly useful than the 2024 measurement
    paper for a CPU scheduling study.

## Implementation And Evidence Boundary

The comparison was checked against repository revision `92306d3`, especially
[SpectrumAnalysis](../../src/dsp/spectrum_analysis.hpp), the
[Fourier](../../src/SpectrumAnalyzer.cpp) and
[Spectre](../../src/Spectrogram.cpp) integration, and the
[display mailbox](../../src/rack_extensions/display_mailbox.hpp).
The code is authoritative: it schedules a selected window through weighted
preparation, butterflies, reconstruction and output, and publishes only after
the exact hop completes. It neither reuses transforms across frames nor runs
analysis on a worker. Intermediate published snapshots may be superseded before
the display consumes them.
This supports a specific scheduling/ownership contribution, not invention of
time distribution or complete-chain scheduling.

The report's proofs, timing observations, and implementation-specific
findings come from the inspected source and the accompanying experiments.
They are not borrowed performance claims from the cited papers. Equations
for the real transform are derived from the stated DFT convention.
Historical campaigns remain historical. This literature pass runs no new
benchmark and does not promote superseded external comparisons into current
evidence; the replacement gates belong to
[Spec 004](../../specs/004-external-fft-comparison.md).

## Venue Implications

These are fit judgments based on the research question and inspected venue
scope, not acceptance predictions. The cited calls are scope/format precedents;
they are not a claim that submissions remain open or that the next edition
uses identical rules.

| Venue | Fit For This Work | Submission Emphasis |
| --- | --- | --- |
| [DAFx](https://dafx26.mit.edu/call-for-papers/) | Strongest direct fit: signal analysis and audio software design, with close scheduling, visualization, and benchmarking precedents already cited | Complete-analysis scheduling; callback tails versus total cost and spectrum age; reproducible software and host evidence |
| [AES full paper](https://aes.org/wp-content/uploads/2025/11/JAES_V73_11_PG793_CfP_Convention_v2.pdf) | Strong engineering alternative; the inspected Category 1 route reviews complete manuscripts | Practical operating limits, implementation, and measured integration; distinguish this route from abstract-reviewed submissions |
| [SMC](https://smc26.mbz.hr/en/submissions/call-for-papers) | Plausible for its signal-processing and musical-software scope | Show how the implementation serves musical workflows; avoid claiming a user benefit without evaluation |
| [WASPAA](https://waspaa.com/wordpress/wp-content/uploads/2025/02/WASPAA-2025-CfP-4.pdf) / [EUSIPCO](https://eusipco2026.org/submissions/) | Broader DSP options if the result transfers beyond this plugin | A compact, generalizable tradeoff supported by matched optimized baselines and host measurements; the inspected formats allow much less space than this report |

DAFx's connection follows from the technical literature, not from adding
unrelated effects papers. The 2026 neural-audio study supplies a measurement
precedent, not an ML contribution for Fourier. A conference version should
lead with the current code and verified evidence, retaining historical
derivations and extended discussion in this supporting report. More citations
cannot substitute for the pending numerical, transition, and host evaluation.

## Publication Guidance

The repository's publication instructions use the official
[arXiv TeX submission guidance](https://info.arxiv.org/help/submit_tex.html)
and [Google Scholar inclusion guidelines](https://scholar.google.com/intl/en/scholar/inclusion.html).
These operational sources are linked from the report README rather than
added to its scientific bibliography. Citation metadata is a discovery aid;
it does not create a public deposit or guarantee indexing.

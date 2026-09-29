# Reference Verification

The bibliography in [fourier.tex](fourier.tex) was checked and expanded on
September 29, 2026 against the primary sources below.
This is a focused literature review
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
| `vanderbyl2016` | [Publisher abstract and section summaries](https://www.sciencedirect.com/science/article/abs/pii/S1051200416000142), [DOI](https://doi.org/10.1016/j.dsp.2016.01.008) | Floating- and fixed-point error across sliding DFT structures; motivates proposed numerical checks without reproducing uninspected rankings |
| `mytkowicz2009` | [Authors' publication page](https://sape.inf.usi.ch/publications/asplos09.html), [full paper](https://cs.uwaterloo.ca/~brecht/courses/Perf-Eval-Shared/readings/producing-wrong-data-asplos-2009.pdf) | Sections 1-3 and 7: layout/environment measurement bias and setup randomization; limits of the existing alternating-order experiment and guidance for future evaluation |
| `kalibera2013` | [University author record](https://kar.kent.ac.uk/33611/), [corrected author manuscript](https://kar.kent.ac.uk/33611/45/p63-kaliber.pdf) | Sections 4, 8, and 9: repetition levels, pilot experiments, and effect-size intervals; proposed future methodology, not a claim about the archived campaigns |
| `wilhelm2008` | [University-hosted published paper](https://www.es.mdh.se/pdf_publications/1258.pdf), [DOI](https://doi.org/10.1145/1347375.1347389) | Section 1 and Figure 1: observed extrema, actual worst-case execution time, and safe bounds; processor-state complications |
| `rack` | [Official plugin API guide](https://vcvrack.com/manual/PluginGuide) | Engine and SIMD context; actual behavior also checked in repository code |

Some publisher pages require browser verification or a subscription. For
Hurchalla and Liu et al., the comparison is deliberately limited to the
scope established by their abstracts/records, supplemented by the open
Battenberg paper for its own implementation. The report does not attribute
uninspected algorithm details or performance results to inaccessible texts.
The Lyons and Howard comparison uses the university record and indexed
manuscript abstract; the repository's migrated PDF endpoint did not provide
readable full text during this revision. Its guarantees are attributed to
the proposed networks, not to all sliding DFT implementations.

The revision replaces the 2015 sliding-DFT exposition with Lyons and Howard's
peer-reviewed treatment, while retaining the original 2003 reference.
The two benchmarking references motivate explicit limits and a future study;
no new experiment was run or existing observation reinterpreted as evidence
of cross-platform robustness.

The subsequent expansion adds Park and Ko, Rafii, and Wefers. Park and Ko's
author-uploaded text is marked as a March 2015 draft; the bibliography uses
the journal publication's 2014 date. Rafii's published article is November
2018, despite the IEEE society summary being dated January 2019. Wefers's
dissertation was defended in 2014 and published in 2015. The proposed
overlap-reuse and work-granularity experiments are this report's future work;
none of these sources supplies measurements of Fourier's implementation.

The September 29 additions use Prusa and Holighaus for an application-level
threading comparison and Wilhelm et al. for timing-bound terminology. The
report distinguishes engine publication from screen updates, and work-count
bounds from execution-time guarantees. Neither addition supplies new
measurements or a performance ranking for Fourier.

The further additions cover depth-first FFT traversal (Bécoulet and Verguet),
feedforward overlap reuse (Garrido), partial-overlap Hann analysis
(Eleftheriadis et al.), and numerical-error methodology (van der Byl and Inggs).
Bécoulet and Verguet's inspected preprint is dated 2020; the bibliography uses
the 2021 journal publication. The van der Byl and Inggs discussion is limited
to the indexed publisher abstract and section summaries; the full article was
not accessible. The suggested suspension, matched-window, and long-stream
accuracy experiments are this report's proposals, not measurements reported
by those sources or completed experiments in this repository.

The report's proofs, timing observations, and implementation-specific
findings come from the inspected source and the accompanying experiments.
They are not borrowed performance claims from the cited papers. Equations
for the real transform are derived from the stated DFT convention.

## Publication Guidance

The repository's publication instructions use the official
[arXiv TeX submission guidance](https://info.arxiv.org/help/submit_tex.html)
and [Google Scholar inclusion guidelines](https://scholar.google.com/intl/en/scholar/inclusion.html).
These operational sources are linked from the report README rather than
added to its scientific bibliography. Citation metadata is a discovery aid;
it does not create a public deposit or guarantee indexing.

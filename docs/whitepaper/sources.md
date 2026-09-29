# Reference Verification

The bibliography in [fourier.tex](fourier.tex) was checked on September 28,
2026 against the primary sources below. This is a focused literature review
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
| `hurchalla2010` | [AES abstract and paper record](https://secure.aes.org/forum/pubs/conventions/?elib=15679) | Time-distributed FFT for convolution; paper 8257, 129th convention |
| `battenberg2011` | [Author-hosted full paper](https://ericbattenberg.com/pdf/partconvDAFx2011.pdf), [author publication page](https://ericbattenberg.com/publication/partconv/) | Cooperative versus preemptive processing; sections 4 and 6 examined |
| `liu2017` | [Rutgers author bibliography](https://zoulab.engr.rutgers.edu/journal_article), [university publication record](https://www.researchwithrutgers.org/en/publications/optimal-time-distributed-fast-fourier-transform-application-to-on/) | Time-distributed FFT/IFFT in online control; volume 41, pages 114-124 |
| `jacobsen2003` | [Jacobsen's later exposition and references](https://www.dsprelated.com/showarticle/776.php) | Sliding DFT distinction; IEEE Signal Processing Magazine 20(2), 74-80 |
| `jacobsen2015` | [Author's exposition](https://www.dsprelated.com/showarticle/776.php) | Recursive bin updates and relation to the FFT |
| `richardson2019` | [Author-deposited preprint and journal metadata](https://arxiv.org/abs/1707.08213) | Tree reuse across windows; published as Algorithm 991 in ACM TOMS 45(1), article 12 |
| `frigo2005` | [Authors' full paper](https://fftw.org/fftw-paper-ieee.pdf), [author publication record](https://fftw.org/~athena/abstracts/abstract8.html) | Optimized FFT implementations as an unmeasured comparison class |
| `rack` | [Official plugin API guide](https://vcvrack.com/manual/PluginGuide) | Engine and SIMD context; actual behavior also checked in repository code |

Some publisher pages require browser verification or a subscription. For
Hurchalla and Liu et al., the comparison is deliberately limited to the
scope established by their abstracts/records, supplemented by the open
Battenberg paper for its own implementation. The report does not attribute
uninspected algorithm details or performance results to inaccessible texts.

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

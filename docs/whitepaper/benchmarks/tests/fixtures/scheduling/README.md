# Retained Scheduling Regression Inputs

These five gzip files preserve complete raw CSV bytes from spec 012's existing
measurement archives. They are regression evidence, not newly collected data.
`provenance.json` records the original campaign metadata/hash, bundle/member,
raw SHA-256, workload contract, and checked process summary. Bundle paths are
relative to [`data/comparison-012`](../../../../data/comparison-012/README.md).
Compression uses a zero gzip timestamp; decompression reproduces the original
raw hash. Tests need these compact fixtures and the tracked metadata, not the
large optional bundles.

| Fixture | Retained Process |
| --- | --- |
| `core` | Core float N=4096/H=1024/D=64 callback, confirmation session 02 repeat 00 |
| `vdsp` | Existing vDSP float adapter at the same settings and session/repeat |
| `inverse` | Incremental inverse float at those settings, separate inverse-job boundary |
| `slow-hybrid` | N=16384 hybrid throughput process at 390.3232 ns/sample, session 01 repeat 00 |
| `inverse-misses` | Batch inverse N=2048/H=1024/D=1 process with 64 compute-budget exceedances, session 02 repeat 00 |

The raw regressions check original counts, costs, observed maxima, hop peaks,
phase imbalance, and exceedances. A separate regression recomputes the headline
3.42/9.50 us p99 and 2.27 cost ratio from all nine retained process summaries
per workload. Historical timestamps/chunks that were not recorded remain
unavailable. Inverse exceedances do not establish analysis-module or device
deadline misses. No slow process is removed or assigned an unobserved cause.

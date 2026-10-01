# Apple Accelerate/vDSP Benchmark Adapter

[vdsp.hpp](../../../../benchmark/paper/vdsp.hpp) wraps the macOS system framework for research benchmarks.
It is compiled only when `PAPER_HAVE_VDSP` and `__APPLE__` are defined. The
ordinary Fourier/Spectre plugin does not include this adapter or acquire an
Accelerate dependency through it. There is no portable fallback masquerading
as vDSP on another platform.

## Transform And Storage Contract

Float and double use their corresponding native vDSP APIs. Supported lengths
are powers of two from 128 through 16384. Each instance creates one exact-size
radix-2 setup with `vDSP_create_fftsetup` or `vDSP_create_fftsetupD`, reuses it
for every execution, and destroys it with the corresponding destroy function.
The setup serves both complex directions for identity/filtering workloads.

Real input uses `vDSP_fft_zrip` or `vDSP_fft_zripD`. The adapter explicitly
copies even/odd samples into split storage, rescales the native result by
one half, and separates the packed DC/Nyquist entries. `forward_real` writes
all N natural-order bins, including the conjugate half;
`forward_real_positive` writes only N/2+1 bins for analysis. Complex input uses
`vDSP_fft_zip` or `vDSP_fft_zipD` with explicit split packing and interleaved
output stores. Forward output is unnormalized; inverse output includes 1/N
normalization. Apple documents these layouts and scaling in its
[vDSP programming guide][packing]. All copying, scaling, and required output
stores occur inside the execution methods and must remain inside the measured
workload.

The real-only `analysis` and `rfft` operations retain N scalar buffer elements;
complex operations retain 2N. The same buffers serve as execution scratch.
`info_json()` records their actual vector capacities in bytes, one setup,
setup exponent, packing policy, precision, loaded framework image path,
OS product/build versions, and compiler SDK/deployment macros. Those macros
identify compile-time availability levels, not the full installed SDK version;
the campaign's build provenance must retain the actual SDK selection too.

The system framework is proprietary platform software supplied by Apple;
the adapter itself follows this repository's GPL-3.0-or-later source terms.
No Apple implementation is vendored or independently rebuildable here. A
framework image can reside in the dyld shared cache even when `dladdr` supplies
its install path; that path alone is not a hashable standalone binary. Metadata
therefore does not invent a provider version or binary hash. Apple's public
setup API does not expose setup size, internal scratch, or native allocator
activity. These remain explicitly unknown. The separate C++ allocation audit
observes adapter allocations but cannot prove absence of native allocation or
bound the duration of an indivisible platform FFT call.

## Independent Validation

From the repository root on macOS with Apple Command Line Tools/Xcode and
Python 3:

```shell
python3 -m unittest discover -s docs/whitepaper/benchmarks/tests -p test_vdsp.py
c++ -std=c++11 -O2 -Wall -Wextra -pedantic -DPAPER_HAVE_VDSP -DPAPER_ALLOCATION_AUDIT test/paper/verify_vdsp.cpp -framework Accelerate -o /tmp/fourier-verify-vdsp
/tmp/fourier-verify-vdsp
```

The verifier applies the shared direct-DFT fixtures at N=128 independently to
real forward, complex forward, and complex inverse transforms in both
precisions. Analytical shifted impulses and non-Hermitian inverse spectra
exercise every output at every supported length. A sentinel after the positive
half checks the analysis output boundary. Repeated prepared executions are
checked for observed C++ allocations. Invalid lengths and operations must fail.
These are numerical/capability checks, not performance measurements or claims
of hard real-time safety. The disabled-header regression runs on every
platform without importing Accelerate; the numerical test explicitly skips
hosts other than macOS.

Both Python tests and the standalone verifier passed on 2026-09-29 on Apple
Silicon, macOS 26.6.2 build 25G83, with Xcode's macOS 26.5 SDK. No interactive
Rack session or publication timing campaign was run for this provider check.
The owning specification records integrated streaming and artifact evidence.

[packing]: https://developer.apple.com/library/archive/documentation/Performance/Conceptual/vDSP_Programming_Guide/UsingFourierTransforms/UsingFourierTransforms.html

# Rack/PFFFT Benchmark Adapter

`pffft.hpp` is research code. It uses the selected Rack SDK's existing
`rack::dsp::RealFFT` and `rack::dsp::ComplexFFT` wrappers; it does not change
Fourier, Spectre, the plugin dependency graph, or their transform choices.
The owning work and completion evidence are in
[spec 004](../../../specs/004-external-fft-comparison.md).

## Scope And Canonical Layout

The adapter uses single-precision floats and accepts powers of two from
128 through 16384. This intentionally matches the comparison suite. The
inspected native implementation accepts additional lengths with factors
2, 3, and 5, subject to SIMD divisibility constraints. Those lengths are
outside this experiment. Rack documents multiples of 32 for real transforms
and 16 for complex transforms; those divisibility conditions alone do not
guarantee support for arbitrary prime factors.

Each adapter constructs one exact-size native plan and two aligned transfer
buffers before measurement. Real analysis and RFFT workloads need a real
plan. Complex forward, inverse, periodic inverse jobs, and identity/FIR
chains share one reusable complex plan. No extra plan is prepared for an
unused transform family.

Rack's ordered real output packs DC and Nyquist into its first two floats,
followed by interleaved real/imaginary components of bins 1 through N/2-1.
`forward_real_positive` writes K=N/2+1 canonical complex bins for analyzer
work. `forward_real` also reconstructs negative-frequency conjugates and
writes all N complex bins, matching the existing isolated RFFT boundary.
This extra reconstruction is part of that benchmark's execution cost.
Neither path assumes `std::complex<float>` storage can be passed directly
to the native library: input copies and output conversion are explicit.

Ordered complex transforms pack and unpack all N complex values. The
native inverse is unnormalized. `inverse_complex` multiplies each returned
component by 1/N while writing the canonical output; normalization and all
stores are part of execution. Canonical complex input and output may alias.
An adapter rejects a method requiring a transform family that its operation
did not prepare.

## Inspected Implementation And Provenance

The initial implementation inspection used the following clean source
revisions, available locally in the selected Rack tree:

-   Rack: `8c33d966d329e4a6e354593b2b5f9ac2df5a03bd`.
-   PFFFT: `74d7261be17cf659d5930d4830609406bd7553e3`.
-   PFFFT upstream in that tree: Julien Pommier's
    [PFFFT repository](https://bitbucket.org/jpommier/pffft/).

The adapter uses this older single-precision API. Features of the current
[VCV PFFFT fork](https://github.com/VCVRack/pffft), including its newer build
configuration and optional precision variants, are not evidence about this
linked implementation.

The inspected local files had these SHA-256 identities:

| File Relative To Rack | SHA-256 |
| --- | --- |
| `include/dsp/fft.hpp` | `49ca9221619ee8d148337f3fb517ced75147ac60382b14f9a69623a1c98996c3` |
| `dep/include/pffft.h` | `32a44c944e8f8f8d693286aebcb564d14183ead4ecbf28ca53dabd47a97aafc7` |
| `dep/pffft/pffft.c` | `c8fa9044fbc62ebaa05b76cc9a273b1d76bc310c23672c6dfd856b90075ea64c` |
| `libRack.dylib` | `0ee091bb5065326c1e5a101d38ac2c3de00f962fd049294f50f11582b53a820e` |

The runner must retain the actual selected headers, library, and available
implementation source in each campaign. A checkout revision does not prove
that an existing binary was built from that checkout. These initial hashes
identify what was inspected and tested; future campaigns carry their own
identities and must not inherit this source inspection unconditionally.

No native library source is copied into this repository. Rack's license and
the PFFFT/FFTPACK UCAR redistribution terms remain attached to their upstream
files and retained dependency artifacts. This first-party adapter follows
[Fourier's source license](../../../LICENSE.md).

## Scratch And Persistent Storage

Both Rack wrappers pass a null work pointer to `pffft_transform_ordered`.
The inspected PFFFT implementation allocates its temporary array on the
stack in that case. The real transform requires N floats of scratch and
the complex transform requires 2N floats: respectively 64 KiB and 128 KiB
at N=16384. These are scratch payload sizes, not a measurement of the entire
call stack. Compiler alignment, native call frames, and local temporaries
add overhead.

The adapter owns two native aligned buffers: 2N floats total for a real plan
and 4N floats for a complex plan. PFFFT also retains a twiddle payload of N
floats for real transforms or 2N floats for complex transforms, plus an
opaque setup structure. Its inspected aligned allocator requests 64 extra
bytes per allocation. Allocator bookkeeping and size-class rounding are
additional costs. Metadata reports aligned buffer payloads and
source-derived twiddle/scratch payloads separately; the opaque plan byte
count and total allocator/stack usage remain unknown.

No heap allocation appears in the inspected native transform path. Its
setup and aligned buffers use C allocation, which the framework's C++
`new`/`delete` audit does not intercept. Zero C++ execution allocations
therefore cannot independently prove native allocation behavior. Native
source inspection and binary identity accompany that observation. No direct
PFFFT variant with caller-managed scratch is introduced here: that would be
a separate experiment answering the stack-versus-persistent-scratch question.

## Provider Verification

From the repository root, with the normal Rack source-tree layout or
`RACK_DIR` set to a compatible SDK:

```shell
python3 -m unittest discover -s benchmark/paper -p test_pffft.py -v
```

This compiles a C++11 verifier and links the actual selected Rack library.
It skips explicitly if Rack headers are unavailable. It does not require an
audio device or create a Rack module. The independent references cover:

-   The shared silence, shifted impulse, DC, Nyquist, off-bin tone, and dense
    complex fixtures, using a direct DFT at N=128 for real forward, complex
    forward, and complex inverse transforms.
-   Every power of two from 128 through 16384: all-output complex impulse,
    non-Hermitian analytic inverse, and real DC/Nyquist/tone checks.
-   Dense non-Hermitian inverse input against selected direct-DFT samples,
    positive-spectrum output bounds, canonical buffer aliasing, unsupported
    sizes/operations, and incorrectly prepared transform families.
-   Parseable provider metadata and correct real/complex payload accounting.

The provider suite passed on 2026-09-29 using arm64 macOS 26.6.2 (25G83),
Apple Clang 21.0.0, and the library identified above. This is correctness
evidence, not a performance campaign or a manual Rack session. Shared
analyzer, inverse-job, chain, callback, throughput, and artifact validation
belong to the integrated campaign recorded in spec 004.

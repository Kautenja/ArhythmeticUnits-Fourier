# FFTW Benchmark Provider

[fftw.hpp](../../../benchmark/paper/fftw.hpp) adapts the optional serial FFTW C API to the paper benchmark's
canonical transform boundaries. It is research code, not a Fourier/Spectre
backend. Ordinary Rack plugin and standalone DSP builds do not need FFTW.

## Dependency And Build

From the repository root, with Python 3, a C compiler, GNU-compatible Make,
and network access:

```shell
python3 docs/whitepaper/benchmarks/build_fftw.py --jobs 2
python3 -m unittest discover -s docs/whitepaper/benchmarks -p 'test_fftw.py' -v
```

The helper downloads the pinned upstream [FFTW 3.3.10 source archive][source],
checks SHA-256
`56c932549852cddcfafdab3820b0200c7742675be92179e59e6215b340e26467`,
and builds separate float and double static libraries. It installs only in
`.build/deps/fftw`; no system installation or vendoring is required. The
archive and build logs remain in `.build/deps/fftw-source`. Preserve required
evidence elsewhere before `make clean`, which removes `.build`.

Every invocation extracts the verified archive into a new retained work
directory and uses empty per-precision build directories. Previous sources
and objects are never reused or deleted. A manifest of the extracted relative
paths, file permissions and contents is hashed before building and checked
again afterward; provenance records its digest, source root, source archive,
and each precision's configuration and build-log paths and hashes.

Both builds use `-O3 -fPIC`, disable Fortran, threads, OpenMP and shared
libraries, and link only the serial API. ARM64 uses NEON codelets for float;
**FFTW 3.3.10 does not support double NEON codelets**, so this build's double
codelets are scalar. x86-64 builds enable SSE2, AVX and AVX2 codelets, with
FFTW's runtime CPU selection. Other architectures use the scalar build.
The script records exact configure commands, compiler identity, per-precision
SIMD options, generated configuration hashes, header and library hashes in
`.build/deps/fftw/provenance.json`. This identifies the tested build, not a
claim about the best possible FFTW configuration.

Benchmark compilation requires `-DPAPER_HAVE_FFTW`,
`-I.build/deps/fftw/include`, and the explicit archive paths
`.build/deps/fftw/lib/libfftw3f.a` and
`.build/deps/fftw/lib/libfftw3.a`. An alternative prefix must contain both
precision libraries and `include/fftw3.h`; its provenance must be captured
independently. The provider's standalone test honors `FFTW_PREFIX` and `CXX`.

## Transform And Planning Contracts

The provider creates only plans required by its operation: real forward for
analysis/rFFT, complex forward for FFT, complex backward for inverse jobs,
and forward/backward pairs for identity/FIR processing. Forward transforms
are unnormalized. Complex backward output includes every real and imaginary
sample and divides by N. FFTW's native r2c output has N/2+1 natural-order
complex bins. Analysis consumes those bins directly; the complete real
transform boundary reconstructs all N bins by conjugate symmetry.
These conventions follow FFTW's [array layout][layout] and [definitions][math].

Plans bind aligned native arrays allocated by `fftw_malloc` or `fftwf_malloc`.
Every execution copies input into those arrays, executes the plan, and copies
canonical output back; conversion and scaling therefore count toward the
operation's cost. No assumption about `std::complex`'s native layout or
caller alignment is needed. Input/output aliases are supported by the full
complex methods because input is copied before any caller output is stored.

The primary policy is `FFTW_MEASURE`, one thread, fresh benchmark processes,
and no imported wisdom. The constructor explicitly forgets previous wisdom.
Since [measurement planning can overwrite inputs][flags], each execution
copies its complete input after planning. The low-level constructor also
accepts `FFTW_ESTIMATE` for separately identified sensitivity experiments;
it must not be pooled with MEASURE results. Other planning flags are rejected.

`info_json()` exports the linked precision library's version, compiler and
codelet optimization strings, per-instance plan text, and the precision's
process-global wisdom at metadata capture. It runs outside execution timing.
Wisdom can change when another instance is planned; the recorded plan text
is the authoritative description of this instance. Plans and aligned arrays
are released at destruction. The wrapper does not call global `fftw_cleanup`
because that would invalidate other live plans; global planner state can
remain until process exit, as described by [FFTW's plan API][plans].

## Storage And Allocation Limits

Native wrapper array requests are exact: `(2N+2)*sizeof(T)` for real forward
at the even sizes in this benchmark and `4N*sizeof(T)` for complex transforms,
including paired forward/backward plans sharing the same arrays. These counts
exclude the outer benchmark's input, output and processing state. FFTW's
opaque plan storage, allocator overhead, internal execution scratch and stack
usage are not returned by its public API and are explicitly unknown.

The wrapper itself performs no allocation in its successful transform calls.
That does **not** establish allocation-free native FFTW execution. Inspection
of the pinned source shows `dft/buffered.c` allocating and freeing scratch in
`apply()`, and `kernel/ifftw.h`'s buffer macros using heap allocation beyond
their stack threshold. Whether these paths execute depends on the selected
plan. The benchmark's C++ new/delete audit does not intercept FFTW's native C
allocation. Metadata records this limitation and retains the selected plans;
native allocation counts remain null rather than an unmeasured zero.

## Independent Checks

The provider test uses the shared six-fixture direct DFT suite at N=128 for
float and double real forward, complex forward and complex inverse. It also
checks every bin at N=2048 and N=16384 against a separate recursive complex
transform and selected bins against direct sums, using seeded non-Hermitian
input where appropriate. It checks input preservation, positive-only output
bounds, alias-safe complex operation, invalid configuration rejection and
MEASURE/ESTIMATE metadata separation. The reference uses `long double` and
reports its actual mantissa width; on the tested Apple ARM64 host it is not
wider than double. Streaming latency, smoothing, startup and frequency-domain
processing are covered by the shared outer-adapter checks.

These are correctness and evidence-plumbing checks. They do not establish
performance rankings or replace a clean measurement campaign.

[source]: https://www.fftw.org/fftw-3.3.10.tar.gz
[layout]: https://www.fftw.org/fftw3_doc/Real_002ddata-DFT-Array-Format.html
[math]: https://www.fftw.org/fftw3_doc/What-FFTW-Really-Computes.html
[flags]: https://www.fftw.org/fftw3_doc/Planner-Flags.html
[plans]: https://www.fftw.org/fftw3_doc/Using-Plans.html

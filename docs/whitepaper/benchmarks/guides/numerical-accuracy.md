# Analysis Numerical Acceptance

`spectrum-norms-v1` defines the benchmark-only processed-spectrum acceptance
policy introduced after the first historical FR-11 measurement pilot. It
changes validation and reporting, not measured FFT providers, pipeline
arithmetic, or Rack modules.
It is an engineering error budget, not a floating-point error theorem.

## Motivation And Scope

The previous test required each magnitude error to be at most
`tau * max(1, abs(reference_bin))`. Its unit floor was independent of input
level and transform length, even though these spectra use an unnormalized
DFT. Cancellation can make an individual bin small while roundoff depends on
the whole input. Consequently this rule could reject a well-resolved spectrum
near a weak bin yet accept deletion of an entire sufficiently quiet spectrum.
The long pilot exposed both reference precision and acceptance-policy issues.

The repaired oracle computes large transforms in binary64 after forming the
same rounded input/window products as the measured precision; small sizes use
direct DFT sums. Its output follows the same positive-bin, band and EMA
contract. A native verification suite checks selected large oracle bins against
direct sums. Binary64 is not arbitrary precision; `long double` is also
binary64 on the measured Apple Silicon host. Native independent analytical,
layout, normalization, inverse and direct-FIR checks remain in place.

[benchFFT's accuracy methodology](https://www.fftw.org/accuracy/method.html)
compares transforms using vector-relative errors, including L2 and Linf.
Its [accuracy commentary](https://www.fftw.org/accuracy/comments.html) discusses
FFT roundoff growth and the importance of accurate twiddles. Those sources
motivate the norm conventions here; they do not supply our tolerances or prove
bounds for this processed magnitude/band/EMA pipeline. In particular, the
complex DFT's unitary properties do not transfer to the nonlinear magnitude
and smoothing stages.

## Per-Spectrum Contract

For each published channel independently, let `a` be all actual output bins
and `r` their reference values. Compute:

-   `relative_l2 = sqrt(sum((a-r)^2)) / sqrt(sum(r^2))`.
-   `relative_linf = max(abs(a-r)) / max(abs(r))`.

Both must be no larger than the already specified analysis budget: `3e-4`
for float and `1e-10` for double. These values were not increased to fit the
observed failures. Analysis retains its own budget rather than borrowing the
tighter float isolated-transform budget, because it includes prefix sums,
band averaging and EMA. All values must be finite. A zero reference requires
exactly zero output; the reported relative errors are then zero. No absolute
unit floor is used for quiet nonzero spectra. Norms use binary64 `hypot`
accumulation outside timing.

The denominator belongs to the same frame and channel. A previous loud frame,
another analyzer, or another SIMD lane cannot mask failure. An entire campaign's
maximum reference magnitude is never used as the acceptance denominator.
Publication/output counts, cadence, actual provider instances and native
transform normalization checks remain separate mandatory conditions.

This is a deliberate change from a pointwise acceptance contract to a
spectrum-level contract. It provides no uniform relative-accuracy guarantee
for every weak bin. Applications requiring that guarantee need a separate
application-specific test; they cannot infer it from a normwise pass.

## Retained Diagnostics And Compatibility

Every new audited analysis run retains the policy identifier and tolerance,
number of checked spectra/bins/zero spectra, largest per-spectrum relative L2
and Linf errors, original absolute error/reference maxima, and the former
pointwise test's violation count and largest scaled error. The worst pointwise
case retains endpoint, channel, bin, actual and reference values. JSON retains
these details; generated CSV tables expose norm maxima and pointwise counts.
Counts summed across process replays describe repeated checks, not independent
numerical samples. Pointwise violations do not disappear when a spectrum passes.

The runner stamps `analysis_accuracy_policy` in metadata. The checker requires
that policy and complete diagnostics when the archived sources contain the new
implementation. It rejects missing, non-finite, inconsistent, truncated or
out-of-budget evidence. Earlier archives keep their earlier meaning and are
not upgraded by the current checker. Reports separate changed source/build
identities; the incomplete and provisional pilots are not pooled with the
restored full pilot.

The scalar core/legacy adapters retain their documented preflight-only
numerical evidence. This policy is used by external and scheduled analysis,
and by all independently audited four-channel banks, including the core SIMD
bank. It establishes no unmeasured first-party accuracy advantage.

## Executable Evidence

From the repository root, with Python and the ordinary C++ compiler:

```shell
python3 -m unittest discover -s docs/whitepaper/benchmarks/tests -p 'test_*.py'
```

`verify_analysis_accuracy.cpp` rejects wrong scaling/layout, dropped weak
tones, NaN, nonzero silence and loss of a very quiet spectrum. It checks scale
invariance and prevents cross-frame/channel masking. Python mutations check
policy, counts, norm limits and retained pointwise diagnostics.

`verify_analysis_streams.cpp` checks 220000-sample pilot streams and
strong/weak-tone-plus-noise streams followed by silence. It covers
N=128/2048/4096/16384, smoothing off/on, amplitudes 1 and 1e-6, independent
selected-bin direct sums, and every supported precision. The ordinary suite
runs its PFFFT cases when Rack is present. On macOS with the documented FFTW
prefix and generated registry, run all native cases as follows:

```shell
mkdir -p .build/paper-numerical-policy
python3 docs/whitepaper/benchmarks/lib/generate_registry.py .build/paper-numerical-policy/registry.generated.hpp --features="fftw vdsp"
c++ -std=c++11 -O3 -funsafe-math-optimizations -DPAPER_HAVE_FFTW -DPAPER_HAVE_VDSP -I.build/paper-numerical-policy -I../../include -I../../dep/include -I.build/deps/fftw/include test/paper/verify_analysis_streams.cpp -L../.. -lRack .build/deps/fftw/lib/libfftw3f.a .build/deps/fftw/lib/libfftw3.a -framework Accelerate -o .build/paper-numerical-policy/verify-streams
DYLD_LIBRARY_PATH=../.. .build/paper-numerical-policy/verify-streams > .build/paper-numerical-policy/streams.jsonl
```

This is an untimed correctness experiment. Host-specific results and full
pilot revalidation are recorded in [spec 012](../../../../specs/archive/012-comparison-evidence-and-paper-integration.md).

# C++ Style Guide

This guide adapts `free-j/docs/developer-manual/style-guide-cpp.md` back to
Fourier, whose DSP headers inspired that guide. It preserves the compact,
documented DSP style while using Fourier's actual language baseline,
namespaces, build structure, and licensing.

## Project Defaults

-   Use C++11 for reusable DSP and standalone tests, as specified by
    `SConstruct`. Do not introduce a newer language requirement incidentally.
-   Use `.hpp` for C++ headers and `.cpp` for implementation and test files.
    Keep templates in headers. Most current DSP is intentionally header-only.
-   Use existing domain namespaces: `Math`, `Filter`, `Trigger`, and
    `MusicTheory`, with nested namespaces where appropriate. Keep Rack types
    and host dependencies out of reusable DSP headers.
-   Use conventional include guards. For new headers, follow the descriptive
    `ARHYTHMETIC_UNITS_FOURIER_..._HPP_` pattern; preserve existing guards in
    unrelated code.
-   Preserve existing copyright and license notices. New production source
    should follow the GPL-3.0-or-later notice in neighboring `src/` files,
    using the appropriate year and attribution. Tests and imported code may
    carry different existing notices; do not replace them mechanically.
-   Do not import Quadra-specific C ABI structs, Swift bridging, C++17,
    Catch2 v3, or `djkernel::dsp` conventions from the reference guide.

## File Shape

Prefer one clear domain per file: a filter, transform family, trigger,
buffer, or window family. Umbrella headers should collect child headers
rather than acquire behavior. Keep Rack registration, module processing,
and rendering recognizable even where they share a module `.cpp` file.

Split by responsibility, ownership, or test seam when that improves
navigation. File length is a design smell, not a mandatory limit. Large
formula catalogs and cohesive module implementations do not need arbitrary
splits. Avoid catch-all helpers and unrelated additions to existing files.

For a compact processor, prefer documented private state followed by public
construction, reset, configuration, accessors, and processing. Use `struct`
for simple value types and the existing DSP idiom; use `class` when it makes
encapsulation clearer. Match the surrounding layout when editing a type.

## Naming And Formatting

-   Types use `PascalCase`, such as `DCBlocker` and `OnTheFlyRFFT`.
-   Variables and stored state generally use `snake_case`, such as
    `sample_rate`, `last_input`, and `hop_index`.
-   Keep method families consistent. Filters and triggers use names such as
    `setTransitionWidth` and `getDivision`; FFT and module helpers also use
    `is_done_computing` and `process_coefficients`. Match the domain being
    edited rather than renaming established APIs.
-   Constants and Rack ID enumerators commonly use `UPPER_SNAKE_CASE`.
    Preserve serialized enum values and existing scoped enum conventions.
-   Make units and ranges explicit in names or nearby documentation: Hz,
    samples, seconds, volts, linear amplitude, power, and decibels must not
    be interchangeable. Short mathematical names such as `n`, `N`, and
    `omega` are appropriate beside the formula they implement.
-   Indent statements with four spaces and put opening braces on the
    declaration/control line. Match existing access-label indentation
    (` public:` and ` private:` in many DSP headers).
-   Close namespaces and guards with identifying comments, matching nearby
    files. Wrap long expressions and signatures at meaningful boundaries.
-   Prefer early returns and named intermediate calculations. Compact guard
    returns are fine; use braces where branches have multiple operations or
    ambiguity would obscure state changes.
-   Use `const` for values that should not change, explicit casts for numeric
    boundaries, and `explicit` for single-argument constructors when implicit
    conversion is not intended.

Include what a file uses, with standard and local headers grouped clearly.
Short trailing comments naming imported symbols are useful in DSP headers.
Avoid transitive include dependencies and new `using namespace` directives
in headers. The existing Rack umbrella is not a precedent for namespace
pollution in reusable math.

## Documentation

Use a short file-purpose comment and concise `///` Doxygen comments for
types, functions, and stored state. Use `@brief`, `@param`, `@returns`, and
`@details` to document nontrivial contracts. Documentation depth depends on
behavior, not access control: private helpers and state need their units,
invariants, lifetime, and safety constraints explained too.

For mathematical code, put formulas near the implementation using the
existing Doxygen math syntax. Explain normalization, coefficient layout,
valid lengths, indexing, failure behavior, and whether returned references
remain valid after the next call or resize. Comments should explain why
the code is correct, not narrate obvious assignments.

For example, a filter contract can describe the recurrence directly:

```cpp
/// @brief Process one sample using the DC blocker.
/// @param input the current input sample
/// @returns a reference to the internal output, updated on the next call
/// @details
/// \f$y[n] = g(x[n] - x[n - 1]) + p y[n - 1]\f$,
/// where \f$g = (1 + p) / 2\f$ corrects the gain at Nyquist.
inline const T& process(const T& input);
```

Document declarations in the header; add implementation comments for local
formulas or invariants rather than duplicating API prose. Keep documentation
changes scoped to the ownership unit being changed.

## DSP APIs And Numeric Behavior

Favor small processors with explicit state, reset/configuration methods,
and deterministic `process`, `buffer`, `step`, or `compute` operations.
Preserve the incremental FFT API and its scheduling contract. Separate
preparation and storage sizing from repeated processing where possible.

Templates are appropriate for generic math and the existing scalar/SIMD
sharing. Do not assume that a scalar-only standard-library operation also
works with `simd::float_4`. Use typed constants and verify the instantiated
Rack build when changing shared templates. Generic scalar parameters may
follow the existing `const T&` convention; concrete arithmetic helpers can
pass small scalar values by value.

Use storage and integer widths appropriate to the actual range. Preserve
`size_t` for container sizes and avoid accidental signed/unsigned narrowing.
Use wider counters when the required lifetime demands them, without imposing
a blanket integer-width migration. Choose floating-point precision based
on error and stability needs and test the supported types.

Validate external values before indexing, allocating, taking logarithms,
or dividing. Define behavior for silence, zero sizes, invalid lengths, and
non-finite values where applicable. Do not silently change transform
normalization, clipping, or frequency limits under the guise of cleanup.
Keep failures and exceptions from escaping Rack processing callbacks.

## Real-Time And Display Boundaries

Do not add blocking locks, waits, file I/O, logging, drawing, or allocation
to repeated engine processing. Prefer preallocated buffers and bounded work.
Document preparation costs and ownership changes explicitly. Existing
reconfiguration allocations are described in [Architecture](architecture.md)
and should not be generalized into new hot-path behavior.

Display code may prepare pixels and issue NanoVG calls, but must not mutate
engine-owned analysis storage without a defined handoff. Explain threading
and lifetime for buffers crossing that boundary.

## Tests And Review

Use the pinned Catch2 v2 harness. Follow
[Development And Testing](development-and-testing.md) for executable targets,
signal fixtures, tolerances, Rack checks, and benchmark evidence.

Before completing a change, check that its files have clear ownership,
math and units are documented, generic DSP remains independent of Rack,
persisted identities remain compatible, and relevant tests exercise the
changed behavior. Avoid broad style churn in unrelated code.

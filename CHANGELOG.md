# Change Log

## 2.1.1 (2025-03-TODO)

-   Fix `Math::freq_to_string` to cover edge cases along unit bounds
-   Update `SpectrumAnalyzer` and `Spectrogram` to use `Math::freq_to_string`
    in all cases of frequency string serialization
-   Remove `FlagMenuItem` in favor of built-in `createBoolPtrMenuItem`
-   Update `Spectrogram` to use `createIndexPtrSubmenuItem` instead of custom
    defined menu options for window function selection
-   Replace `configParam<TriggerParamQuantity>(...)` with `configButton(...)`
-   Replace `NumOptions` with name vector `.size()` call, which is more direct
    and readable
-   Simplify JSON code for the plugin

## 2.1.0 (2025-03-10)

-   Support for built-in VCV dark light/dark panels
    -   Deprecates internal management of theme in favor of built-in features
    -   Dark panels may be selected using `View > Use dark panels if available`

## 2.0.2 (2025-03-10)

-   Update manual for _Fourier_ and _Spectre_
-   Resolve error from macOS build chain (`stream = {}` -> refactored functions)
-   Resolve error from Windows build chain (`static constexpr` in displays
    refactored to instance level `const`.)
-   Resolve runtime error from Windows machines on initialization of _Fourier_
    -   Update `SpectrogramAnalyzer::draw_coefficients` to pass coefficient
        buffers by _value_ as opposed to by _reference_ to prevent race
        conditions
-   Resolve runtime error from Windows machines on initialization of _Spectre_
    -   Update `Spectrogram::draw_coefficients` to render pixels on the _heap_
        instead of the _stack_. I.e. replace `uint8_t pixels[size]` with
        `std::vector<uint8_t> pixels; pixels.resize(size);`.

## 2.0.1 (2025-03-08)

-   Resolve compiler error on Windows machines

## 2.0.0 (2025-03-07)

-   Initial release

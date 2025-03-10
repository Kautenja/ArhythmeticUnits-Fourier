# Change Log

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

// Functions for calculating window coefficients.
//
// Copyright 2025 Arhythmetic Units
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_WINDOW_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_WINDOW_HPP_

#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include "constants.hpp"
#include "functions.hpp"

/// @brief Basic mathematical functions.
namespace Math {

/// @brief Windowing functions.
namespace Window {

// ---------------------------------------------------------------------------
// MARK: Non-Parametric Windows
// ---------------------------------------------------------------------------

/// @brief Calculate the value of a Boxcar (i.e., rectangular) window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @details
/// The rectangular window is calculated as:
///
/// \f$w[n] = 1.0\f$
///
template<typename T>
inline T boxcar(const T& n, const T& N, const bool& is_symmetric = true) {
    // NOTE: boxcar (rectangular) window is an identity type of function;
    // however, it must still conform to the window function interface. To
    // suppress unused parameter warnings from the compiler use the following
    // statements to "use" each of the input variables. This solution works
    // on all compilers and is the least ugly looking syntax.
    (void)(n); (void)(N); (void)(is_symmetric);
    return 1;
}

// MARK: B-Spline Windows

/// @brief Calculate the value of a Bartlett (i.e., triangular) window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Bartlett window is calculated as:
///
/// \f$
/// w[n] = \frac{2}{N - \mathrm{1}_{symmetric}} *
/// \Bigg(
///   \frac{N - \mathrm{1}_{symmetric}}{2}
///   - \bigg|n - \frac{N - \mathrm{1}_{symmetric}}{2}\bigg|
/// \Bigg)
/// \f$
///
template<typename T>
inline T bartlett(const T& n, const T& N, const bool& is_symmetric = true) {
    return (T(2) / (N - is_symmetric)) * (((N - is_symmetric) / T(2)) - abs(n - (N - is_symmetric) / T(2)));
}

/// @brief Calculate the value of a Parzen window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Parzen window is calculated as:
///
/// \f$
/// w[n] = \begin{cases}
///   1 - 6n^2 + 6|n|^3 & |n| < \frac{1}{2}          \\
///   2 (1 - |n|)^3     & \frac{1}{2} \le |n| \le 1  \\
///   0                 & \text{otherwise}
/// \end{cases}
/// \f$
///
template<typename T>
inline T parzen(const T& n, const T& N, const bool& is_symmetric = true) {
    // Shift and scale the domain of n from [0, N) to [-1, 1]
    const T x = T(2) * n / (N - is_symmetric) - T(1);
    if (abs(x) >= T(0.5)) return T(2) * Math::cubed(T(1) - abs(x));
    return T(1) - T(6) * Math::squared(x) + T(6) * Math::cubed(abs(x));
}

// MARK: Polynomial Windows

/// @brief Calculate the value of a Welch window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Welch window is calculated as:
///
/// \f$
/// 1 - \bigg(\frac{k - \frac{N-1-\mathrm{1}_{symmetric}}{2}}{\frac{N+1-\mathrm{1}_{symmetric}}{2}}\bigg)^2
/// \f$
///
template<typename T>
inline T welch(const T& n, const T& N, const bool& is_symmetric = true) {
    return T(1) - Math::squared(
        (n - (N - T(1) - is_symmetric) / T(2)) /
            ((N + T(1) - is_symmetric) / T(2))
    );
}

// MARK: Cosine/Sine/Sinc Windows

/// @brief Calculate the value of a cosine window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The cosine window is calculated as:
///
/// \f$
/// w[n] = \sin\bigg(\frac{\pi (n + 0.5)}{N + \mathrm{1}_{\overline{symmetric}}}\bigg)
/// \f$
///
/// which is equivalently expressed as:
///
/// \f$
/// w[n] = -\cos\bigg(\frac{\pi (n + \frac{N}{2} + 0.5)}{N + \mathrm{1}_{\overline{symmetric}}}\bigg)
/// \f$
///
template<typename T>
inline T cosine(const T& n, const T& N, const bool& is_symmetric = true) {
    return sin(Math::pi<T>() * (n + T(0.5)) / (N + !is_symmetric));
}

/// @brief Calculate the value of a Bohman window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Bohman window is calculated as:
///
/// \f$
/// w[n] =
/// \bigg(1 - \bigg|\frac{2n}{N - \mathrm{1}_{symmetric}} - 1\bigg|\bigg)
/// \cos\bigg(\pi\bigg|\frac{2n}{N - \mathrm{1}_{symmetric}} - 1\bigg|\bigg)
/// + \frac{1}{\pi}\sin\bigg(\pi\bigg|\frac{2n}{N - \mathrm{1}_{symmetric}} - 1\bigg|\bigg)
/// \f$
///
template<typename T>
inline T bohman(const T& n, const T& N, const bool& is_symmetric = true) {
    const auto x = abs(T(2) * n / (N - is_symmetric) - T(1));
    return (T(1) - x) * cos(Math::pi<T>() * x) + (T(1) / Math::pi<T>()) * sin(Math::pi<T>() * x);
}

/// @brief Calculate the value of a Lanczos window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Lanczos window is calculated as:
///
/// \f$
/// w[n] = \begin{cases}
/// \frac{
///   \sin\bigg(\pi\big(\frac{2n}{N-\mathrm{1}_{symmetric}}-1\big)\bigg)
///   }{
///   \pi\big(\frac{2n}{N-\mathrm{1}_{symmetric}}-1\big)
/// } & n \ne \frac{N-\mathrm{1}_{symmetric}}{2} \\
/// 1 & \text{otherwise}
/// \end{cases}
/// \f$
///
template<typename T>
inline T lanczos(const T& n, const T& N, const bool& is_symmetric = true) {
    if (n == floor((N - is_symmetric)/T(2))) return 1;
    T x = Math::pi<T>() * (T(2) * n / (N - is_symmetric) - T(1));
    return sin(x) / (x + std::numeric_limits<T>::min());
}

// MARK: Cosine-Sum Windows

/// @brief Calculate the value of a Hann window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Hann window is calculated as:
///
/// \f$w[n] = 0.5 - 0.5\cos\Big(\frac{2 \pi n}{N - \mathrm{1}_{symmetric}}\Big)\f$
///
template<typename T>
inline T hann(const T& n, const T& N, const bool& is_symmetric = true) {
    return T(0.50) - T(0.50) * cos(T(2) * Math::pi<T>() * n / (N - is_symmetric));
}

/// @brief Calculate the value of a Bartlett-Hann window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Bartlett-Hann window is calculated as:
///
/// \f$
/// w[n] = 0.62
/// − 0.48\bigg|\frac{n}{N-\mathrm{1}_{symmetric}}-0.5\bigg|
/// − 0.38\cos\bigg(\frac{2\pi n}{N - \mathrm{1}_{symmetric}}\bigg)
/// \f$
///
template<typename T>
inline T barthann(const T& n, const T& N, const bool& is_symmetric = true) {
    return T(0.62) - T(0.48) * abs(n / (N - is_symmetric) - T(0.5))
        - T(0.38) * cos(T(2) * Math::pi<T>() * n / (N - is_symmetric));
}

/// @brief Calculate the value of a Hamming window.
///
/// @tparam T the type of data to calculate the coefficients as
/// @param n the coefficient index of the filter
/// @param N the length of the array, i.e., the order of the filter plus 1
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Hamming window is calculated as:
///
/// \f$w[n] = 0.54 - 0.46\cos\Big(\frac{2 \pi n}{N - \mathrm{1}_{symmetric}}\Big)\f$
///
template<typename T>
inline T hamming(const T& n, const T& N, const bool& is_symmetric = true) {
    return T(0.54) - T(0.46) * cos(T(2) * Math::pi<T>() * n / (N - is_symmetric));
}

/// @brief Calculate the value of a Blackman window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Blackman window is calculated as:
///
/// \f$
/// w[n] = 0.42
/// - 0.5\cos\Big(\frac{2 \pi n}{N - \mathrm{1}_{symmetric}}\Big)
/// + 0.08\cos\Big(\frac{4 \pi n}{N - \mathrm{1}_{symmetric}}\Big)
/// \f$
///
template<typename T>
inline T blackman(const T& n, const T& N, const bool& is_symmetric = true) {
    return T(0.42) - T(0.50) * cos(T(2) * Math::pi<T>() * n / (N - is_symmetric)) + T(0.08) * cos(T(4) * Math::pi<T>() * n / (N - is_symmetric));
}

/// @brief Calculate the value of a Blackman-Harris window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Blackman-Harris window is calculated as:
///
/// \f$
/// w[n] = 0.35875
/// - 0.48829 \cos\bigg(\frac{2 \pi n}{N - \mathrm{1}_{symmetric}}\bigg)
/// + 0.14128 \cos\bigg(\frac{4 \pi n}{N - \mathrm{1}_{symmetric}}\bigg)
/// - 0.01168 \cos\bigg(\frac{6 \pi n}{N - \mathrm{1}_{symmetric}}\bigg)
/// \f$
///
template<typename T>
inline T blackmanharris(const T& n, const T& N, const bool& is_symmetric = true) {
    return T(0.35875)
        - T(0.48829) * cos(T(2) * Math::pi<T>() * n / (N - is_symmetric))
        + T(0.14128) * cos(T(4) * Math::pi<T>() * n / (N - is_symmetric))
        - T(0.01168) * cos(T(6) * Math::pi<T>() * n / (N - is_symmetric));
}

/// @brief Calculate the value of a Blackman-BlackmanNuttall window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Blackman-BlackmanNuttall window is calculated as:
///
/// \f$
/// w[n] = 0.3635819
/// - 0.4891775 \cos\bigg(\frac{2 \pi n}{N - \mathrm{1}_{symmetric}}\bigg)
/// + 0.1365995 \cos\bigg(\frac{4 \pi n}{N - \mathrm{1}_{symmetric}}\bigg)
/// - 0.0106411 \cos\bigg(\frac{6 \pi n}{N - \mathrm{1}_{symmetric}}\bigg)
/// \f$
///
template<typename T>
inline T blackmannuttall(const T& n, const T& N, const bool& is_symmetric = true) {
    return T(0.3635819)
        - T(0.4891775) * cos(T(2) * Math::pi<T>() * n / (N - is_symmetric))
        + T(0.1365995) * cos(T(4) * Math::pi<T>() * n / (N - is_symmetric))
        - T(0.0106411) * cos(T(6) * Math::pi<T>() * n / (N - is_symmetric));
}

/// @brief Calculate the value of a Kaiser-Bessel window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Kaiser-Bessel window is calculated as:
///
/// \f$
/// w[n] = 0.402
/// − 0.498 \cos\bigg(\frac{2 \pi n}{N - \mathrm{1}_{symmetric}}\bigg)
/// + 0.098 \cos\bigg(\frac{4 \pi n}{N - \mathrm{1}_{symmetric}}\bigg)
/// − 0.001 \cos\bigg(\frac{6 \pi n}{N - \mathrm{1}_{symmetric}}\bigg)
/// \f$
///
template<typename T>
inline T kaiserbessel(const T& n, const T& N, const bool& is_symmetric = true) {
    return T(0.402)
        - T(0.498) * cos(T(2) * Math::pi<T>() * n / (N - is_symmetric))
        + T(0.098) * cos(T(4) * Math::pi<T>() * n / (N - is_symmetric))
        - T(0.001) * cos(T(6) * Math::pi<T>() * n / (N - is_symmetric));
}

/// @brief Calculate the value of a Flattop window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @details
/// The Flattop window is calculated as:
///
/// \f$
/// w[n] = 0.21557895
/// −0.416631580\cos\bigg(\frac{2\pi n}{N-\mathrm{1}_{symmetric}}\bigg)
/// +0.277263158\cos\bigg(\frac{4\pi n}{N-\mathrm{1}_{symmetric}}\bigg)
/// −0.083578947\cos\bigg(\frac{6\pi n}{N-\mathrm{1}_{symmetric}}\bigg)
/// +0.006947368\cos\bigg(\frac{8\pi n}{N-\mathrm{1}_{symmetric}}\bigg)
/// \f$
///
/// > Flat top windows are used for taking accurate measurements of signal
/// > amplitude in the frequency domain, with minimal scalloping error from the
/// > center of a frequency bin to its edges, compared to others. This is a
/// > 5th-order cosine window, with the 5 terms optimized to make the main lobe
/// > maximally flat.
///
/// "Digital Signal Processing for Measurement Systems", D'Antona, Gabriele,
/// and A. Ferrero, Springer Media, 2006, p. 70
///
template<typename T>
inline T flattop(const T& n, const T& N, const bool& is_symmetric = true) {
    return T(0.21557895)
        - T(0.416631580) * cos(T(2) * Math::pi<T>() * n / (N - is_symmetric))
        + T(0.277263158) * cos(T(4) * Math::pi<T>() * n / (N - is_symmetric))
        - T(0.083578947) * cos(T(6) * Math::pi<T>() * n / (N - is_symmetric))
        + T(0.006947368) * cos(T(8) * Math::pi<T>() * n / (N - is_symmetric));
}

/// @brief Window function types.
enum class Function {
    Boxcar = 0,
    Bartlett,
    BartlettHann,
    Parzen,
    Welch,
    Cosine,
    Bohman,
    Lanczos,
    Hann,
    Hamming,
    Blackman,
    BlackmanHarris,
    BlackmanNuttall,
    KaiserBessel,
    Flattop,
};

/// @brief Return a vector of window function names.
static const std::vector<std::string>& function_names() {
    static const std::vector<std::string> names = {
        "Boxcar",
        "Bartlett",
        "BartlettHann",
        "Parzen",
        "Welch",
        "Cosine",
        "Bohman",
        "Lanczos",
        "Hann",
        "Hamming",
        "Blackman",
        "BlackmanHarris",
        "BlackmanNuttall",
        "KaiserBessel",
        "Flattop"
    };
    return names;
}

/// @brief Calculate the value of a standard window.
///
/// @tparam T the type of data to calculate the window value
/// @param function the windowing function to use
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
///
template<typename T>
inline T window(Function window_, const T& n, const T& N, const bool& is_symmetric = true) {
    switch (window_) {
    case Function::Boxcar:          return boxcar<T>(n, N, is_symmetric);
    case Function::Bartlett:        return bartlett<T>(n, N, is_symmetric);
    case Function::BartlettHann:    return barthann<T>(n, N, is_symmetric);
    case Function::Parzen:          return parzen<T>(n, N, is_symmetric);
    case Function::Welch:           return welch<T>(n, N, is_symmetric);
    case Function::Cosine:          return cosine<T>(n, N, is_symmetric);
    case Function::Bohman:          return bohman<T>(n, N, is_symmetric);
    case Function::Lanczos:         return lanczos<T>(n, N, is_symmetric);
    case Function::Hann:            return hann<T>(n, N, is_symmetric);
    case Function::Hamming:         return hamming<T>(n, N, is_symmetric);
    case Function::Blackman:        return blackman<T>(n, N, is_symmetric);
    case Function::BlackmanHarris:  return blackmanharris<T>(n, N, is_symmetric);
    case Function::BlackmanNuttall: return blackmannuttall<T>(n, N, is_symmetric);
    case Function::KaiserBessel:    return kaiserbessel<T>(n, N, is_symmetric);
    case Function::Flattop:         return flattop<T>(n, N, is_symmetric);
    default: throw std::runtime_error("Received invalid window " + std::to_string(static_cast<int>(window_)));
    }
}

/// @brief Return the coherent gain for the given windowing function.
///
/// @tparam window the window function to get the coherent gain of
/// @returns the coherent for the given window in decibels
/// @details
/// The coherent gain should be applied to windowed DFTs to account for pass
/// band attenuation of the filter. i.e., H[s] / (getCoherentGain(...) * N).
/// The coherent gain is calculated as the integral of the window. I.e., we
/// can simply take the expectation of the window as N approaches infinity. In
/// practice the coherent gain is calculated as the average amplitude of the
/// window with length N = 1024.
///
template<Function window>
float coherent_gain();
template<> inline constexpr float coherent_gain<Function::Boxcar>()          { return 1.000000; }
template<> inline constexpr float coherent_gain<Function::Bartlett>()        { return 0.500000; }
template<> inline constexpr float coherent_gain<Function::BartlettHann>()    { return 0.500000; }
template<> inline constexpr float coherent_gain<Function::Parzen>()          { return 0.375000; }
template<> inline constexpr float coherent_gain<Function::Welch>()           { return 0.667317; }
template<> inline constexpr float coherent_gain<Function::Cosine>()          { return 0.637240; }
template<> inline constexpr float coherent_gain<Function::Bohman>()          { return 0.405285; }
template<> inline constexpr float coherent_gain<Function::Lanczos>()         { return 0.589490; }
template<> inline constexpr float coherent_gain<Function::Hann>()            { return 0.500000; }
template<> inline constexpr float coherent_gain<Function::Hamming>()         { return 0.540000; }
template<> inline constexpr float coherent_gain<Function::Blackman>()        { return 0.420000; }
template<> inline constexpr float coherent_gain<Function::BlackmanHarris>()  { return 0.358750; }
template<> inline constexpr float coherent_gain<Function::BlackmanNuttall>() { return 0.363582; }
template<> inline constexpr float coherent_gain<Function::KaiserBessel>()    { return 0.402000; }
template<> inline constexpr float coherent_gain<Function::Flattop>()         { return 0.215579; }

/// @brief Return the coherent gain for the given windowing function.
///
/// @param window the window function to get the coherent gain of
/// @returns the coherent for the given window in decibels
/// @details
/// The coherent gain should be applied to windowed DFTs to account for pass
/// band attenuation of the filter. i.e., H[s] / (getCoherentGain(...) * N)
/// Reference:
/// https://dsp.stackexchange.com/questions/27277/why-does-the-hamming-window-attenuate-the-fft
/// https://www.recordingblogs.com/wiki/coherent-gain
///
inline float coherent_gain(const Function& window) {
    switch (window) {
    case Function::Boxcar:          return coherent_gain<Function::Boxcar>();
    case Function::Bartlett:        return coherent_gain<Function::Bartlett>();
    case Function::BartlettHann:    return coherent_gain<Function::BartlettHann>();
    case Function::Parzen:          return coherent_gain<Function::Parzen>();
    case Function::Welch:           return coherent_gain<Function::Welch>();
    case Function::Cosine:          return coherent_gain<Function::Cosine>();
    case Function::Bohman:          return coherent_gain<Function::Bohman>();
    case Function::Lanczos:         return coherent_gain<Function::Lanczos>();
    case Function::Hann:            return coherent_gain<Function::Hann>();
    case Function::Hamming:         return coherent_gain<Function::Hamming>();
    case Function::Blackman:        return coherent_gain<Function::Blackman>();
    case Function::BlackmanHarris:  return coherent_gain<Function::BlackmanHarris>();
    case Function::BlackmanNuttall: return coherent_gain<Function::BlackmanNuttall>();
    case Function::KaiserBessel:    return coherent_gain<Function::KaiserBessel>();
    case Function::Flattop:         return coherent_gain<Function::Flattop>();
    default: throw std::runtime_error("Received invalid window " + std::to_string(static_cast<int>(window)));
    }
}

/// @brief Return the side-lobe amplitude for the given windowing function.
///
/// @tparam window the window function to get the side-lobe amplitude of
/// @returns the side-lobe amplitude for the given window in decibels
///
template<Function window>
float side_lobe_amplitude();
template<> inline constexpr float side_lobe_amplitude<Function::Boxcar>()          { return -13.2; }
template<> inline constexpr float side_lobe_amplitude<Function::Bartlett>()        { return -26.4; }
template<> inline constexpr float side_lobe_amplitude<Function::BartlettHann>()    { return -35.7; }
template<> inline constexpr float side_lobe_amplitude<Function::Parzen>()          { return -53.0; }
template<> inline constexpr float side_lobe_amplitude<Function::Welch>()           { return -21.2; }
template<> inline constexpr float side_lobe_amplitude<Function::Cosine>()          { return -22.8; }
template<> inline constexpr float side_lobe_amplitude<Function::Bohman>()          { return -46.0; }
template<> inline constexpr float side_lobe_amplitude<Function::Lanczos>()         { return -26.3; }
template<> inline constexpr float side_lobe_amplitude<Function::Hann>()            { return -31.5; }
template<> inline constexpr float side_lobe_amplitude<Function::Hamming>()         { return -41.7; }
template<> inline constexpr float side_lobe_amplitude<Function::Blackman>()        { return -58.1; }
template<> inline constexpr float side_lobe_amplitude<Function::BlackmanHarris>()  { return -91.8; }
template<> inline constexpr float side_lobe_amplitude<Function::BlackmanNuttall>() { return -88.7; }
template<> inline constexpr float side_lobe_amplitude<Function::KaiserBessel>()    { return -65.4; }
template<> inline constexpr float side_lobe_amplitude<Function::Flattop>()         { return -83.0; }

/// @brief Return the side-lobe amplitude for the given windowing function.
///
/// @param window the window function to get the side-lobe amplitude of
/// @returns the side-lobe amplitude for the given window in decibels
///
inline float side_lobe_amplitude(const Function& window) {
    switch (window) {
    case Function::Boxcar:          return side_lobe_amplitude<Function::Boxcar>();
    case Function::Bartlett:        return side_lobe_amplitude<Function::Bartlett>();
    case Function::BartlettHann:    return side_lobe_amplitude<Function::BartlettHann>();
    case Function::Parzen:          return side_lobe_amplitude<Function::Parzen>();
    case Function::Welch:           return side_lobe_amplitude<Function::Welch>();
    case Function::Cosine:          return side_lobe_amplitude<Function::Cosine>();
    case Function::Bohman:          return side_lobe_amplitude<Function::Bohman>();
    case Function::Lanczos:         return side_lobe_amplitude<Function::Lanczos>();
    case Function::Hann:            return side_lobe_amplitude<Function::Hann>();
    case Function::Hamming:         return side_lobe_amplitude<Function::Hamming>();
    case Function::Blackman:        return side_lobe_amplitude<Function::Blackman>();
    case Function::BlackmanHarris:  return side_lobe_amplitude<Function::BlackmanHarris>();
    case Function::BlackmanNuttall: return side_lobe_amplitude<Function::BlackmanNuttall>();
    case Function::KaiserBessel:    return side_lobe_amplitude<Function::KaiserBessel>();
    case Function::Flattop:         return side_lobe_amplitude<Function::Flattop>();
    default: throw std::runtime_error("Received invalid window " + std::to_string(static_cast<int>(window)));
    }
}

/// @brief Return the stop-band attenuation for the given windowing function.
///
/// @tparam window the window function to get the stop-band attenuation of
/// @returns the stop-band attenuation for the given window in decibels
///
template<Function window>
float stopband_attenuation();
template<> inline constexpr float stopband_attenuation<Function::Boxcar>()          { return -21; }
template<> inline constexpr float stopband_attenuation<Function::Bartlett>()        { return -25; }
template<> inline constexpr float stopband_attenuation<Function::BartlettHann>()    { return -42; }
template<> inline constexpr float stopband_attenuation<Function::Parzen>()          { return -31; }
template<> inline constexpr float stopband_attenuation<Function::Welch>()           { return -31; }
template<> inline constexpr float stopband_attenuation<Function::Cosine>()          { return -33; }
template<> inline constexpr float stopband_attenuation<Function::Bohman>()          { return -28; }
template<> inline constexpr float stopband_attenuation<Function::Lanczos>()         { return -28; }
template<> inline constexpr float stopband_attenuation<Function::Hann>()            { return -44; }
template<> inline constexpr float stopband_attenuation<Function::Hamming>()         { return -53; }
template<> inline constexpr float stopband_attenuation<Function::Blackman>()        { return -74; }
template<> inline constexpr float stopband_attenuation<Function::BlackmanHarris>()  { return -92; }
template<> inline constexpr float stopband_attenuation<Function::BlackmanNuttall>() { return -93; }
template<> inline constexpr float stopband_attenuation<Function::KaiserBessel>()    { return -60; }
template<> inline constexpr float stopband_attenuation<Function::Flattop>()         { return -99; }

/// @brief Return the stop-band attenuation for the given windowing function.
///
/// @param window the window function to get the stop-band attenuation of
/// @returns the stop-band attenuation for the given window in decibels
///
inline float stopband_attenuation(const Function& window) {
    switch (window) {
    case Function::Boxcar:          return stopband_attenuation<Function::Boxcar>();
    case Function::Bartlett:        return stopband_attenuation<Function::Bartlett>();
    case Function::BartlettHann:    return stopband_attenuation<Function::BartlettHann>();
    case Function::Parzen:          return stopband_attenuation<Function::Parzen>();
    case Function::Welch:           return stopband_attenuation<Function::Welch>();
    case Function::Cosine:          return stopband_attenuation<Function::Cosine>();
    case Function::Bohman:          return stopband_attenuation<Function::Bohman>();
    case Function::Lanczos:         return stopband_attenuation<Function::Lanczos>();
    case Function::Hann:            return stopband_attenuation<Function::Hann>();
    case Function::Hamming:         return stopband_attenuation<Function::Hamming>();
    case Function::Blackman:        return stopband_attenuation<Function::Blackman>();
    case Function::BlackmanHarris:  return stopband_attenuation<Function::BlackmanHarris>();
    case Function::BlackmanNuttall: return stopband_attenuation<Function::BlackmanNuttall>();
    case Function::KaiserBessel:    return stopband_attenuation<Function::KaiserBessel>();
    case Function::Flattop:         return stopband_attenuation<Function::Flattop>();
    default: throw std::runtime_error("Received invalid window " + std::to_string(static_cast<int>(window)));
    }
}

/// @brief Return the transition band width for the given window.
///
/// @tparam window the window function to calculate the transition band of
/// @returns the transition band width for the window.
///
template<Function window>
float transition_width();
template<> inline constexpr float transition_width<Function::Boxcar>()          { return 0.9; }
template<> inline constexpr float transition_width<Function::Bartlett>()        { return 1.8; }
template<> inline constexpr float transition_width<Function::BartlettHann>()    { return 3.2; }  // slightly wider than Hann
template<> inline constexpr float transition_width<Function::Parzen>()          { return 4.0; }  // broader transition for smoother roll‐off
template<> inline constexpr float transition_width<Function::Welch>()           { return 3.3; }  // comparable to Hamming
template<> inline constexpr float transition_width<Function::Cosine>()          { return 3.1; }  // similar to Hann
template<> inline constexpr float transition_width<Function::Bohman>()          { return 3.3; }  // nearly the same as Welch/Hamming
template<> inline constexpr float transition_width<Function::Lanczos>()         { return 3.3; }  // for a typical Lanczos parameter (e.g. a = 3)
template<> inline constexpr float transition_width<Function::Hann>()            { return 3.1; }
template<> inline constexpr float transition_width<Function::Hamming>()         { return 3.3; }
template<> inline constexpr float transition_width<Function::Blackman>()        { return 5.5; }
template<> inline constexpr float transition_width<Function::BlackmanHarris>()  { return 6.3; }  // wider main lobe for extra sidelobe suppression
template<> inline constexpr float transition_width<Function::BlackmanNuttall>() { return 6.4; }
template<> inline constexpr float transition_width<Function::KaiserBessel>()    { return 3.6; }  // value depends on the chosen beta (here, ~-60 dB design)
template<> inline constexpr float transition_width<Function::Flattop>()         { return 7.5; }  // very wide to ensure amplitude flatnes

/// @brief Return the transition band width for the given window.
///
/// @param N the number of samples in the impulse response
/// @param window the window function to calculate the transition band of
///
/// @details
/// The transition width is calculated as:
///
/// \f$\Delta f = \f$`TRANSITION_WIDTHS[i]`\f$/ N\f$
///
/// Filter order can be calculated by rearranging the equation:
///
/// \f$N = \f$`TRANSITION_WIDTHS[i]`\f$/ \Delta f\f$
///
template<typename T>
inline T transition_width(const T& N, const Function& window) {
    switch (window) {
    case Function::Boxcar:          return transition_width<Function::Boxcar>() / N;
    case Function::Bartlett:        return transition_width<Function::Bartlett>() / N;
    case Function::BartlettHann:    return transition_width<Function::BartlettHann>() / N;
    case Function::Parzen:          return transition_width<Function::Parzen>() / N;
    case Function::Welch:           return transition_width<Function::Welch>() / N;
    case Function::Cosine:          return transition_width<Function::Cosine>() / N;
    case Function::Bohman:          return transition_width<Function::Bohman>() / N;
    case Function::Lanczos:         return transition_width<Function::Lanczos>() / N;
    case Function::Hann:            return transition_width<Function::Hann>() / N;
    case Function::Hamming:         return transition_width<Function::Hamming>() / N;
    case Function::Blackman:        return transition_width<Function::Blackman>() / N;
    case Function::BlackmanHarris:  return transition_width<Function::BlackmanHarris>() / N;
    case Function::BlackmanNuttall: return transition_width<Function::BlackmanNuttall>() / N;
    case Function::KaiserBessel:    return transition_width<Function::KaiserBessel>() / N;
    case Function::Flattop:         return transition_width<Function::Flattop>() / N;
    default: throw std::runtime_error("Received invalid window " + std::to_string(static_cast<int>(window)));
    }
}

/// @brief A structure for holding and updating samples of a cached window.
template<typename T>
struct CachedWindow {
 private:
    /// The current window function being represented.
    Function function;
    /// The samples of the window function being represented.
    std::vector<T> samples;
    /// Whether to use a symmetric window.
    bool is_symmetric;
    /// Whether the window function should be gained or not.
    bool is_gained;

    /// @brief Compute the window for the current set of parameters.
    inline void compute_window() {
        const float gain = is_gained ? T(1) / coherent_gain(function) : T(1);
        for (size_t n = 0; n < samples.size(); n++)
            samples[n] = gain * window<T>(function, n, samples.size(), is_symmetric);
    }

 public:
    /// @brief Initialize a new pre-computed window.
    CachedWindow() : function(Function::Boxcar), samples(1), is_symmetric(true), is_gained(false) {
        compute_window();
    }

    /// @brief Initialize a new pre-computed window.
    /// @param function_ The window function to represent.
    /// @param N The length of the window function.
    /// @param is_symmetric_ Whether the window function is symmetric.
    /// @param is_gained_ Whether the window function should be gained or not.
    explicit CachedWindow(
        const Function& function_,
        const size_t& N,
        const bool& is_symmetric_ = true,
        const bool& is_gained_ = false
    ) : function(function_), samples(N), is_symmetric(is_symmetric_), is_gained(is_gained_) {
        compute_window();
    }

    /// @brief Update the window to given size and function.
    /// @param function The window function to represent.
    /// @param N The length of the window function.
    /// @param is_symmetric Whether the window function is symmetric.
    /// @param is_gained Whether the window function should be gained or not.
    /// @details
    /// If the window function is already of the specified type, this
    /// function returns without re-calculating the parameters.
    inline void set_window(
        const Function& function,
        const size_t& N,
        const bool& is_symmetric = true,
        const bool& is_gained = false
    ) {
        // If the function and length have not changed, then there is no need
        // to re-calculate the values. Silently fall back to a NoOp.
        if (
            function == this->function &&
            N == samples.size() &&
            is_symmetric == this->is_symmetric &&
            is_gained == this->is_gained
        ) return;
        // Resize buffer to the new size, set the function, and compute samples.
        this->function = function;
        samples.resize(N);
        this->is_symmetric = is_symmetric;
        this->is_gained = is_gained;
        compute_window();
    }

    /// @brief Return the window function sample at the given index.
    /// @param index The index of the window function sample to return.
    /// @returns The window function sample at the given index.
    inline float operator[](const size_t& index) { return samples[index]; }

    /// @brief Return the current window function.
    inline const Function& get_function() const { return function; }

    /// @brief Return the sample buffer.
    inline std::vector<float>& get_samples() { return samples; }

    /// @brief Return true for symmetric window, false for asymmetric.
    inline const bool& get_is_symmetric() const { return is_symmetric; }

    /// @brief Return true for gained window, false for un-gained.
    inline const bool& get_is_gained() const { return is_gained; }
};

// ---------------------------------------------------------------------------
// MARK: Parametric Windows
// ---------------------------------------------------------------------------

/// @brief Calculate the value of an exponential window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @param alpha the shape parameter that controls the sharpness of the window.
/// @details
/// The exponential window is calculated as:
///
/// \f$
/// w[n] = e^{-\alpha|n-\frac{N - \mathrm{1}_{symmetric}}{2}|}
/// \f$
///
template<typename T>
inline T exponential(const T& n, const T& N, const bool& is_symmetric = true, const T& alpha = 0.5) {
    const T M = (N - is_symmetric) / T(2);
    return exp(-alpha * abs(n - M) / M);
}

/// @brief Calculate the value of a Hann-Poisson window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @param alpha the shape parameter that controls the sharpness of the window.
/// @details
/// The Hann-Poisson window is calculated as the product of a Hann window and
/// an exponential window, i.e.,:
///
/// \f$
/// w[n] = 0.5
/// \bigg(1 - \cos\big(\frac{\pi n}{M}\big)\bigg)
/// e^{-\alpha \frac{|n - M|}{M}} |
/// M = \frac{N - \mathrm{1}_{symmetric}}{2}
/// \f$
///
template<typename T>
inline T hannpoisson(const T& n, const T& N, const bool& is_symmetric = true, const T& alpha = 0.5) {
    const T M = (N - is_symmetric) / T(2);
    return T(0.5) * (T(1) - cos(Math::pi<T>() * n / M)) * exp(-alpha * abs(n - M) / M);
}

/// @brief Calculate the value of a Gaussian window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @param std the standard deviation of the Gaussian distribution.
/// @details
/// The Gaussian window is calculated as:
///
/// \f$
/// w[n] = e^{-0.5 \big(\frac{n - M}{\sigma M}\big)^2} \bigg| M = \frac{N - \mathrm{1}_{symmetric}}{2}
/// \f$
///
template<typename T>
inline T gaussian(const T& n, const T& N, const bool& is_symmetric = true, const T& std = 0.25) {
    const T M = (N - is_symmetric) / T(2);
    return exp(-T(0.5) * Math::squared((n - M)/(std * M)));
}

/// @brief Calculate the value of a Tukey window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param is_symmetric True to return the symmetric form for filter design,
/// False to return the periodic form for spectral analysis.
/// @param alpha the shape parameter that controls the sharpness of the window.
/// @details
/// The Tukey window is calculated as:
///
/// \f$
/// w[n] = \begin{cases}
/// 0.5\bigg(1+\cos\big(\pi\frac{|k−M|−\alpha M}{(1-\alpha)M}\big)\bigg) & |k-M| \ge \alpha M \\
/// 1                                                                    & |k−M| < \alpha M
/// \end{cases}
/// \f$
///
template<typename T>
inline T tukey(const T& n, const T& N, const bool& is_symmetric = true, const T& alpha = 0.5) {
    const T M = (N - is_symmetric) / T(2);
    if (abs(n - M) < alpha * M) return 1;
    return T(0.5) * (T(1) + cos(Math::pi<T>() * (abs(n - M) - alpha * M) / ((T(1) - alpha) * M)));
}

/// @brief Kaiser window design.
namespace Kaiser {

/// @brief Compute the zeroth-order modified Bessel function of the first kind.
/// @tparam T the numeric type used in the computation
/// @param x the function argument
/// @return the computed I₀(x)
template<typename T>
inline T bessel(const T& x) {
    T sum = T(1);
    T term = T(1);
    // y = (x/2)^2
    T y = x * x / T(4);
    int k = 1;
    // iterate until the term becomes negligible
    while (term > T(1e-6) * sum) {
        term *= y / (T(k) * T(k));
        sum += term;
        ++k;
    }
    return sum;
}

/// @brief Compute the order of a Kaiser window.
///
/// @tparam T the type of data to use for calculating the order
/// @param a the stop-band ripple in decibels, i.e., \f$\alpha_s\f$
/// @param f the transition width, i.e., \f$\Delta f\f$
/// @return the order for the given Kaiser filter parameters
///
template<typename T>
inline size_t order(const T& a, const T& f) {
    return ceilf((a - T(7.95)) / (T(14.36) * f));
}

/// @brief Compute the \f$\beta\f$ parameter for a Kaiser window.
///
/// @tparam T the type of data to use for calculating the parameter
/// @param a the stop-band ripple in decibels, i.e., \f$\alpha_s\f$
/// @return the \f$\beta\f$ parameter computed from the stop-band ripple
///
template<typename T>
inline T beta(const T& a) {
    if (a > T(50))
        return T(0.1102) * (a - T(8.7));
    if (a >= T(21))
        return T(0.5842) * pow(a - T(21), T(0.4)) + T(0.07886) * (a - T(21));
    return 0;
}

/// @brief Calculate the value of a Kaiser window.
///
/// @tparam T the type of data to calculate the window value
/// @param n the coefficient index of the filter
/// @param N the order of the filter (number of coefficients - 1)
/// @param B the beta parameter for the Kaiser window, i.e., \f$\beta\f$
/// @details
/// The parameter B can be calculated from the stop-band ripple using the
/// function `Kaiser::beta`
///
template<typename T>
inline T window(const T& n, const T& N, const T& B) {
    // calculate alpha, bearing in mind that the filter order is N - 1
    static const T a = (N - T(1)) / T(1);
    return bessel(B * pow(T(1) - pow((n - a) / a, T(2)), T(0.5))) / bessel(B);
}

}  // namespace Kaiser

}  // namespace Window

}  // namespace Math

#endif  // ARHYTHMETIC_UNITS_FOURIER_DSP_MATH_WINDOW_HPP_

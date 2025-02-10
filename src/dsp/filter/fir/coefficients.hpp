// An n'th order FIR filter.
// Copyright 2020 Christian Kauten
//
// Author: Christian Kauten (kautenja@auburn.edu)
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

#ifndef DSP_FILTER_FIR_COEFFICIENTS
#define DSP_FILTER_FIR_COEFFICIENTS

#include "../../math.hpp"

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief FIR filter designs.
namespace FIR {

/// @brief Return a sample of a FIR low-pass filter impulse response.
///
/// @param n the index in the impulse response
/// @param N the number of samples in the impulse response
/// @param cutoff the cut-off frequency of the low-pass filter
/// @returns an impulse of the low-pass filter with given design
/// @details
/// The response of the ideal low-pass filter is a step function in the
/// frequency domain where \f$\omega_c\f$ determines the frequency where the
/// step goes low:
///
/// \f$H_{\mathrm{LP}}(e^{j\omega}) = \begin{cases} 1 & \gets e^{j\omega} < e^{j\omega_c} \\ 0 & \gets \text{otherwise} \end{cases}\f$
///
/// All frequency content \f$\omega\f$ below the cut-off frequency
/// \f$\omega_c\f$ has a gain of \f$1\f$ and all \f$\omega\f$ equal to or above
/// the cut-off frequency have a gain of \f$0\f$. Using the Inverse Fourier
/// transform, the frequency response can be transformed into an impulse
/// response. The impulse response of a low-pass FIR filter is a symmetric
/// shifted sinc function defined as:
///
/// \f$h_{\mathrm{LP}}[n] = \mathcal{F}^{-1}\{H_{\mathrm{LP}}(e^{j\omega})\} = \frac{\sin(\omega_c \phi)}{\phi}\f$
///
/// where \f$\omega_c\f$ is the cut-off frequency \f$\in (0, 1)\f$ relative to
/// the Nyquist frequency (i.e., \f$f_s/2\f$), \f$\phi\f$ is the phase of the
/// sinc:
///
/// \f$\phi = \pi * \big(n - \frac{m}{2}\big)\f$
///
/// and \f$m\f$ is the point of symmetry for the shifted sinc function.
///
/// \f$m = \begin{cases}N & \gets \text{odd }N \\ N - 1 & \gets \text{even }N\end{cases}\f$
///
template<typename T>
inline T lowpass(const int& n, const int& N, const T& cutoff) {
    // Calculate the midpoint of the shifted sinc function. For odd impulse
    // responses, this equates to N and for even impulse responses it is N - 1.
    const int midpoint = N - (1 - (N % 2));
    // Calculate the phase for the numerator and denominator of the sinc.
    const T phase = Math::pi<T>() * (n - midpoint / T(2));
    // If the phase close to zero (within numerical precision), return 1
    if (Math::IEEE754::epsilon_equal<T>(phase, 0)) return 1;
    // Calculate the value of the sinc filter based on the given cut-off
    return std::sin(cutoff * phase) / phase;
}

/// @brief Return a sample of a FIR high-pass filter impulse response.
///
/// @param n the index in the impulse response
/// @param N the number of samples in the impulse response
/// @param cutoff the cut-off frequency of the low-pass filter
/// @returns an impulse of the high-pass filter with given design
/// @details
/// The response of the ideal high-pass filter is a step function in the
/// frequency domain where \f$\omega_c\f$ determines the frequency where the
/// step goes high (i.e., the reciprocal of the ideal low-pass filter):
///
/// \f$H_{\mathrm{HP}}(e^{j\omega}) = \begin{cases} 1 & \gets e^{j\omega} > e^{j\omega_c} \\ 0 & \gets \text{otherwise} \end{cases} = 1 - H_{\mathrm{HP}}(e^{j\omega})\f$
///
/// The impulse response can be obtained through the inverse Fourier Transform:
///
/// \f$h_{\mathrm{HP}}[n] = \mathcal{F}^{-1}\{H_{\mathrm{HP}}(e^{j\omega})\} = \mathcal{F}^{-1}\{1 - H_{\mathrm{LP}}(e^{j\omega})\} = \delta[n] - h_{\mathrm{LP}}[n]\f$
///
/// Practically, this solution does not work when coded as below. The stop-band
/// is hard-locked to -48dB for some reason?
///
/// ```
/// (n == ((N-1) / 2)) - lowpass(n, N, 1 - cutoff)
/// ```
///
/// In practice, the high-pass is implemented by subtracting the low-pass filter
/// filter with given cut-off frequency from the low-pass filter with wide-open
/// cut-off frequency (i.e., a pure sinc function):
///
/// \f$h_{\mathrm{HP}}[n] = \frac{\sin(\phi)}{\phi} - \frac{\sin(\omega_c \phi)}{\phi}\f$
///
/// where \f$\omega_c\f$ is the cut-off frequency \f$\in (0, 1)\f$ relative to
/// the Nyquist frequency (i.e., \f$f_s/2\f$), \f$\phi\f$ is the phase of the
/// sinc:
///
/// \f$\phi = \pi * \big(n - \frac{m}{2}\big)\f$
///
/// and \f$m\f$ is the point of symmetry for the shifted sinc function.
///
/// \f$m = \begin{cases}N & \gets \text{odd }N \\ N - 1 & \gets \text{even }N\end{cases}\f$
///
/// ### Alternative Solutions
///
/// The high-pass could also be implemented by inverting the sign of every
/// other impulse in a low-pass filter of the same n, N, and frequency. I.e.,
///
/// \f$h_{\mathrm{HP}}[n] = (-1)^{n} \frac{\sin(\omega_c \phi)}{\phi}\f$
///
/// However, this causes a phase response that shifts the individual sinusoidal
/// components in such a way as to create spikey signals with an increasing
/// gain relative to the cut-off frequency:
///
/// \f$A = \frac{\omega_c}{\sqrt{2}}\f$
///
/// It is possible to implement this form of the high-pass, but it is
/// undesirable for most audio tasks due to its inherent trait of altering the
/// shape of the signal. the old code for this faulty solution is:
///
/// ```
/// (1.f - frequency * (1 / sqrt(2))) * (2 * (n % 2) - 1) * lowpass(n, N, frequency)
/// ```
///
template<typename T>
inline T highpass(const int& n, const int& N, const T& cutoff) {
    // Calculate the midpoint of the shifted sinc function. For odd impulse
    // responses, this equates to N and for even impulse responses it is N - 1.
    const int midpoint = N - (1 - (N % 2));
    // Calculate the phase for the numerator and denominator of the sinc.
    const T phase = Math::pi<T>() * (n - midpoint / T(2));
    // If the phase close to zero (within numerical precision), return 1
    if (Math::IEEE754::epsilon_equal<T>(phase, 0)) return 1;
    // Calculate the value of the sinc filter based on the given cut-off
    return (std::sin(phase) - std::sin(cutoff * phase)) / phase;
}

/// @brief Return a sample of a FIR band-pass filter impulse response.
///
/// @param n the index in the impulse response
/// @param N the number of samples in the impulse response
/// @param low the cut-off frequency of the low-band
/// @param high the cut-off frequency of the high-band
/// @returns an impulse of the band-pass filter with given design
/// @details
/// The response of the ideal band-pass filter is a step function in the
/// frequency domain where the gain is 1 for all frequencies \f$\omega\f$
/// between the low cut-off frequency \f$\omega_L\f$ and the high cut-off
/// frequency \f$\omega_H\f$, and 0 elsewhere:
///
/// \f$H_{\mathrm{BP}}(e^{j\omega}) = \begin{cases}
/// 1 & \omega_L \leq \omega \leq \omega_H \\
/// 0 & \text{otherwise}
/// \end{cases}\f$
///
/// The impulse response of the band-pass filter is defined as the difference
/// between two low-pass sinc functions, one with a cut-off frequency of
/// \f$\omega_H\f$ and one with a cut-off frequency of \f$\omega_L\f$:
///
/// \f$h_{\mathrm{BP}}[n] = h_{\mathrm{LP}}[n]_{\omega_H} - h_{\mathrm{LP}}[n]_{\omega_L}\f$
///
/// Using the shifted sinc function, this is equivalent to:
///
/// \f$h_{\mathrm{BP}}[n] = \frac{\sin(\omega_H \phi)}{\phi} - \frac{\sin(\omega_L \phi)}{\phi}\f$
///
/// where \f$\phi = \pi * (n - \frac{m}{2})\f$, and \f$m\f$ is the point of
/// symmetry for the shifted sinc function:
///
/// \f$m = \begin{cases}N & \gets \text{odd }N \\ N - 1 & \gets \text{even }N\end{cases}\f$
///
template<typename T>
inline T bandpass(const int& n, const int& N, const T& low, const T& high) {
    // Calculate the midpoint of the shifted sinc function. For odd impulse
    // responses, this equates to N and for even impulse responses it is N - 1.
    const int midpoint = N - (1 - (N % 2));
    // Calculate the phase for the numerator and denominator of the sinc.
    const T phase = Math::pi<T>() * (n - midpoint / T(2));
    // If the phase close to zero (within numerical precision), return 1
    if (Math::IEEE754::epsilon_equal<T>(phase, 0)) return 1;
    // Calculate the value of the sinc filter based on the given cut-off
    return (std::sin(high * phase) - std::sin(low * phase)) / phase;
}

/// @brief Return a sample of a FIR band-stop filter impulse response.
///
/// @param n the index in the impulse response
/// @param N the number of samples in the impulse response
/// @param low the cut-off frequency of the low-band
/// @param high the cut-off frequency of the high-band
/// @returns an impulse of the band-stop filter with given design
/// @details
/// The response of the ideal band-stop filter is a step function in the
/// frequency domain where the gain is 0 for all frequencies \f$\omega\f$
/// between the low cut-off frequency \f$\omega_L\f$ and the high cut-off
/// frequency \f$\omega_H\f$, and 1 elsewhere:
///
/// \f$H_{\mathrm{BS}}(e^{j\omega}) = \begin{cases}
/// 0 & \omega_L \leq \omega \leq \omega_H \\
/// 1 & \text{otherwise}
/// \end{cases}\f$
///
/// The impulse response of the band-stop filter is defined as the difference
/// between a wide-open sinc function and the band-pass sinc function:
///
/// \f$h_{\mathrm{BS}}[n] = h_{\mathrm{LP}}[n]_{\text{wide}} - (h_{\mathrm{LP}}[n]_{\omega_H} - h_{\mathrm{LP}}[n]_{\omega_L})\f$
///
/// Simplifying:
///
/// \f$h_{\mathrm{BS}}[n] = \frac{\sin(\phi)}{\phi} - \bigg(\frac{\sin(\omega_H \phi)}{\phi} - \frac{\sin(\omega_L \phi)}{\phi}\bigg)\f$
///
/// where \f$\phi = \pi * (n - \frac{m}{2})\f$, and \f$m\f$ is the point of
/// symmetry for the shifted sinc function:
///
/// \f$m = \begin{cases}N & \gets \text{odd }N \\ N - 1 & \gets \text{even }N\end{cases}\f$
///
template<typename T>
inline T bandstop(const int& n, const int& N, const T& low, const T& high) {
    // Calculate the midpoint of the shifted sinc function. For odd impulse
    // responses, this equates to N and for even impulse responses it is N - 1.
    const int midpoint = N - (1 - (N % 2));
    // Calculate the phase for the numerator and denominator of the sinc.
    const T phase = Math::pi<T>() * (n - midpoint / T(2));
    // If the phase close to zero (within numerical precision), return 1
    if (Math::IEEE754::epsilon_equal<T>(phase, 0)) return 1;
    // Calculate the value of the sinc filter based on the given cut-off
    return (std::sin(phase) - (std::sin(high * phase) - std::sin(low * phase))) / phase;
}

}  // namespace FIR

}  // namespace Filter

#endif  // DSP_FILTER_FIR_COEFFICIENTS

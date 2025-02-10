// Basic mathematical functions.
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

#ifndef DSP_MATH_FUNCTIONS_HPP
#define DSP_MATH_FUNCTIONS_HPP

#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <vector>
#include "constants.hpp"

// TODO: various quantized look-up tables
// TODO: fast versions of:
// - [ ] harmonic series (and other geometric series)
// - [ ] exp / log / ln / etc.
// - [ ] polynomials
// - [ ] fast inverse square root

/// @brief Basic mathematical functions.
namespace Math {

/// @brief Clip the given value within the given limits.
///
/// @tparam the type of values to clamp
/// @param x the value to clamp
/// @param lower the lower bound to clamp the value to
/// @param upper the upper bound to clamp the value to
/// @returns the value clamped within \f$[lower, upper]\f$
///
template <typename T>
inline T clip(const T& x, const T& lower, const T& upper) {
    return std::max(lower, std::min(x, upper));
}

/// @brief Return the sign of the given value.
///
/// @tparam the type of the value to return the sign of
/// @param x the floating point number to get the sign of
/// @returns \f$1\f$ if the number is positive, \f$-1\f$ otherwise
///
template<typename T>
inline T sgn(const T& x) {
    return std::signbit(x) ? -1 : 1;
}

/// @brief Return the modulo operation between two values.
///
/// @param a the input to the modulo function
/// @param b the upper bound of the modulo function
/// @returns \f$a \mod b\f$
///
template<typename T>
inline T mod(const T& a, const T& b) {
    return (a % b + b) % b;
}

/// @brief Return `base` raised to the power of 2.
///
/// @param x the base value to raise to the power of 2
/// @returns the evaluation of \f$\texttt{base}^2\f$
///
template<typename T>
inline T squared(const T& x) {
    return x * x;
}

/// @brief Return `base` raised to the power of 3.
///
/// @param x the base value to raise to the power of 3
/// @returns the evaluation of \f$\texttt{base}^3\f$
///
template<typename T>
inline T cubed(const T& x) {
    return x * x * x;
}

/// @brief Return `base` raised to the power of `exponent`.
///
/// @tparam T the type of the base value
/// @param base the base value to raise to the given exponent
/// @param exponent the signed integer exponent to raise the base to
/// @returns the evaluation of \f$\texttt{base}^\texttt{exponent}\f$
/// @details
/// This implementation provides a generic pow function using the
/// _exponentiation by squaring_ technique.
///
template<typename T>
inline T powT2i(T base, int exponent) {
    if (exponent < 0) return powT2i(1 / base, -exponent);
    T result = 1;
    for (;;) {
        if (exponent & 1)
            result *= base;
        exponent >>= 1;
        if (!exponent)
            break;
        base *= base;
    }
    return result;
}

/// @brief Return 2 raised to the power of `exponent` using a shift operation.
///
/// @param exponent the signed integer exponent to raise 2 to
/// @returns the evaluation of \f$2^\texttt{exponent}\f$
/// @details
/// This implementation provides a generic \f$2^\texttt{exponent}\f$ function
/// using the shift operator with 32-bit integers as a starting point. As a
/// result, it has a limited range of \f$[2^{-31}, 2^{31}]\f$; for larger
/// ranges, use `pow2`.
///
inline float exp2shift(const int32_t& exponent) {
    // handle negative values; 0 is handled implicitly by the main branch.
    if (exponent < 0)
        return 1.f / exp2shift(-exponent);
    // exponent will always be positive by this point, and has a range of 31
    // bits in either direction. Throw an error if the exponent exceeds this
    // range.
    if (exponent > 31)
        throw "exponent must be in the range [-31, 31]";
    // cast a constant 1 to an unsigned 32-bit integer and perform the shift
    // operation to calculate 2**exponent
    return static_cast<uint32_t>(1) << exponent;
}

/// @brief Calculate n! using Stirling's Formula.
///
/// @tparam the type of values to compute the factorial of
/// @param n the value to compute the factorial of
/// @returns the approximation of n! following Stirling's formula.
/// @details
/// Stirling's formula is defined as
///
/// \f$
/// n! \approx \sqrt{2\pi n}\big(\frac{n}{e}\big)^n
/// \f$
///
template <typename T>
inline T stirlings_formula(const T& n) {
    return sqrt(2 * pi<T>() * n) * pow(n / e<T>(), n);
}

/// @brief Return the input value converted to decibels.
///
/// @tparam T the type of number to convert to decibels
/// @param x the value to convert to decibel scale
/// @returns the decibel value \f$20 \log{10}(|x|)\f$
/// @details
/// It is assumed that signals exist in the unit domain \f$\in [-1, 1]\f$
///
template<typename T>
inline T amplitude2decibels(const T& x) {
    return T(20) * log10(abs(x));
}

/// @brief Return the input decibel value converted to amplitude.
///
/// @tparam T the type of number to convert to amplitude
/// @param x the decibel value to convert to linear scale
/// @returns the amplitude value \f$10^{\frac{x}{20}}\f$
/// @details
/// It is assumed that signals exist in the unit domain \f$\in [-1, 1]\f$.
///
template<typename T>
inline T decibels2amplitude(const T& x) {
    return pow(T(10), x / T(20));
}

/// @brief Quantize a single-precision float value to the given number of bits.
///
/// @tparam the type of number to quantize
/// @param value the value \f$\in [-1, 1]\f$ to quantize
/// @param bits to number of bits to quantize the value to
/// @returns the value quantized to the given number of bits
/// @details
/// Numbers are quantized by:
///
/// 1. computing the maximal unsigned value of the number system as
///    \f$max \gets 2^{bits} - 1\f$;
/// 2. scaling the input value from \f$[-1, 1]\f$ to \f$[-max, max]\f$;
/// 3. truncating the scaled value to its integral component; and
/// 4. scaling the quantized integer from \f$[-max, max]\f$ back to floating
///    point in \f$[-1, 1]\f$
///
template<typename T>
inline T quantize(const T& value, const int& bits) {
    // Determine the maximal value in the number system, i.e., (2^bits) - 1.
    const T max = (1 << bits) - 1;
    // Convert from floating point in [-1.f, 1.f] to integer in [-max, max] and
    // use floor to discard the floating fractional component. Divide by max to
    // convert the integer in [-max, max] back to quantized T in [-1.f, 1.f]
    return sgn(value) * floor(abs(value) * max) / max;
}

/// @brief Return the interpolation between two points
///
/// @tparam the type of numbers to interpolate between
/// @param initial the first point
/// @param terminal the final point
/// @param alpha the linear interpolation factor between the two points
/// @returns the linear interpolation between initial and terminal points
/// @details
/// The interpolation is calculated as:
///
/// \f$(1-\alpha) x_i + \alpha x_f\f$
///
/// \f$\alpha\f$ determines the fractional unit progress from initial to terminal
///
template<typename T>
inline T interpolate(const T& initial, const T& terminal, const T& alpha) {
    return (T(1) - alpha) * initial + alpha * terminal;
}

/// @brief Return the index of the value with the largest value.
///
/// @tparam T the type of data contained in the array
/// @param arr the array to search for the max value of in
/// @param size the number of elements in the given array
/// @returns the index of the max value in the array
///
template<typename T>
std::size_t argmax(const T* const arr, const std::size_t& size) {
    T max = std::numeric_limits<T>::min();
    std::size_t argmax = 0;
    for (std::size_t i = 0; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
            argmax = i;
        }
    }
    return argmax;
}

/// @brief Return the index of the complex value with the largest absolute value.
///
/// @tparam T the type of data contained in the array
/// @param arr the array to search for the max value of in
/// @param size the number of elements in the given array
/// @returns the index of the max value in the array
///
template<typename T>
std::size_t argmax(const std::complex<T>* const arr, const std::size_t& size) {
    std::complex<T> max = std::numeric_limits<std::complex<T>>::min();
    std::size_t argmax = 0;
    for (std::size_t i = 0; i < size; i++) {
        if (abs(arr[i]) > abs(max)) {
            max = arr[i];
            argmax = i;
        }
    }
    return argmax;
}

/// @brief Return the index of the value with the smallest value.
///
/// @tparam T the type of data contained in the array
/// @param arr the array to search for the min value of in
/// @param size the number of elements in the given array
/// @returns the index of the min value in the array
///
template<typename T>
std::size_t argmin(const T* const arr, const std::size_t& size) {
    T min = std::numeric_limits<T>::max();
    std::size_t argmin = 0;
    for (std::size_t i = 0; i < size; i++) {
        if (arr[i] < min) {
            min = arr[i];
            argmin = i;
        }
    }
    return argmin;
}

/// @brief Iterate from the given starting point to stopping point.
///
/// @tparam T the type of the numbers to iterate over
/// @tparam Function a function that accepts an unsigned value and returns void
/// @param from the starting point
/// @param to the stopping point
/// @param callback the callback function to pass indexes to
/// @details
/// Iterates from `from` to `to` and passes each index `i` to `callback`.
///
template<typename T, typename Function>
void range_map(const T& from, const T& to, const Function& callback) {
    if (to > from) for (T i = from; i < to;   i++) callback(i);
    else           for (T i = to;   i < from; i++) callback(i);
}













template<typename T>
std::vector<T> amplitude2decibels(const std::vector<std::complex<T>>& input) {
    std::vector<T> output(input.size());
    for (std::size_t i = 0; i < input.size(); i++)
        output[i] = amplitude2decibels(input[i].real());
    return output;
}

template<typename T>
std::vector<T> amplitude2decibels(const std::vector<T>& input) {
    std::vector<T> output(input.size());
    for (std::size_t i = 0; i < input.size(); i++)
        output[i] = amplitude2decibels(input[i]);
    return output;
}


/// @brief Generate a fixed length sinusoidal waveform.
///
/// @tparam the data type for representing samples
/// @param frequency the frequency of the sine wave in \f$Hz\f$
/// @param sample_rate the sampling rate in \f$Hz\f$
/// @param num_sample the number of samples to generate
/// @returns a vector of size num_samples with the sampled sinusoid
///
template<typename T>
std::vector<T> generate_sinusoid(T frequency, T sample_rate, std::size_t num_samples) {
    // Create a vector to store the samples in
    std::vector<T> wave(num_samples);
    for (std::size_t i = 0; i < wave.size(); i++)  // Iterate over the samples
        wave[i] = sin(T(2) * Math::pi<T>() * frequency * T(i) / T(sample_rate));
    return wave;
}

/// @brief Find the peak level of the given waveform.
///
/// @tparam the data type for representing samples
/// @param wave the wave to find the peak level of
/// @returns the energy of the loudest sample in decibels
/// @details
/// In most cases for detecting changes in energy, RMS should be used. Test
/// cases designed using peak levels for filters will be sensitive to the
/// Gibbs phenomenon an be difficult to calibrate / confirm analytically.
///
template<typename T>
T get_peak(const std::vector<T>& wave) {
    // Initialize a register with the largest value found send to the minimal
    // value.
    T peak = Math::amplitude2decibels(std::numeric_limits<T>::epsilon());
    for (unsigned i = 0; i < wave.size(); i++) {
        // Calculate the instantaneous gain (i.e., peak) for this sample. This
        // can potentially be negative infinity when `wave[i] == 0`, but this
        // is OK because negative infinity will always be less than `peak`
        // guaranteeing that the function returns at most the minimal peak that
        // can be represented through the `amplitude2decibels` function.
        const T gain = Math::amplitude2decibels(wave[i]);
        // Update the peak gain if the instantaneous gain is higher.
        if (gain > peak) peak = gain;
    }
    return peak;
}

/// @brief Calculate the RMS level of the given waveform.
///
/// @tparam the data type for representing samples
/// @param wave the wave to find the RMS level of
/// @returns the RMS level of the waveform \f$[-120, 0]dB\f$
///
template<typename T>
T get_rms(const std::vector<T>& wave) {
    // Calculate the minimum value for the mean square
    static constexpr T MIN_MEAN_SQUARE = std::numeric_limits<T>::epsilon();
    // Iterate over the samples in the waveform to calculate the mean square
    T mean_square = 0;
    for (std::size_t i = 0; i < wave.size(); i++)
        mean_square += Math::squared(wave[i]);
    mean_square /= wave.size();
    // Map the mean square to decibels using properties of logarithm to avoid
    // taking the square root of the mean square. Bound the mean square to the
    // minimal mean square as defined by the type's epsilon value.
    return T(10) * std::log10(std::max(mean_square, MIN_MEAN_SQUARE));
}

/// @brief Western scale exponentially spaced notes.
enum class Note {C = 0, CSharp, D, DSharp, E, F, FSharp, G, GSharp, A, ASharp, B};

/// @brief Convert a note to its string representation.
/// @param note The note to concert to a string.
/// @returns The string representation of the string note.
inline const char* to_string(const Note& note) {
    switch (note) {
    case Note::C:      return "C";
    case Note::CSharp: return "C#";
    case Note::D:      return "D";
    case Note::DSharp: return "D#";
    case Note::E:      return "E";
    case Note::F:      return "F";
    case Note::FSharp: return "F#";
    case Note::G:      return "G";
    case Note::GSharp: return "G#";
    case Note::A:      return "A";
    case Note::ASharp: return "A#";
    case Note::B:      return "B";
    }
}

/// @brief A structure to hold note information.
struct TunedNote {
    /// The note.
    enum Note note;
    /// Octave number.
    int octave;
    /// Cents deviation.
    float cents;
};

/// @brief Convert frequency to a musical note.
/// @param note The note to populate.
/// @param frequency The frequency to convert into a musical note.
/// @details
/// This function treats A4 = 440Hz as the base frequency for note conversion.
/// Notes are generated with their quantized value as an enumeration `Note`,
/// The associated octave the note belongs to, and the tuning offset measured
/// in cents.
inline int frequency_to_note(TunedNote& note, const float& frequency) {
    static constexpr float base_frequency = 440.f;
    // Handle invalid frequencies
    if (frequency <= 0) return 1;
    // Calculate semitones from reference frequency.
    float n = 12.f * log2f(frequency / base_frequency);
    // Nearest note.
    int nearest_note = static_cast<int>(roundf(n));
    // Calculate the note index and octave.
    int note_index = (nearest_note + 9) % 12;  // Offset to align A4
    if (note_index < 0) note_index += 12;
    note.octave = 4 + (nearest_note + 9) / 12;
    if (nearest_note < -9) note.octave--;
    note.note = static_cast<Note>(note_index);
    // Frequency of the nearest note
    float nearest_frequency = base_frequency * powf(2.f, nearest_note / 12.f);
    // Calculate cents difference
    note.cents = 1200 * log2f(frequency / nearest_frequency);
    return 0;
}

}  // namespace Math

#endif  // DSP_MATH_FUNCTIONS_HPP

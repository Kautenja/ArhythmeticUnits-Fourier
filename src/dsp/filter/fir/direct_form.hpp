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

#ifndef DSP_FILTER_FIR_DIRECT_FORM_HPP
#define DSP_FILTER_FIR_DIRECT_FORM_HPP

#include <vector>
#include "../../math/circular_buffer.hpp"
#include "coefficients.hpp"

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief FIR filter designs.
namespace FIR {

/// @brief An FIR filter.
/// @tparam T the data type to process and use for coefficients
/// @details
/// The impulse response of FIR filters can be one of four types
///
/// | Type  | Impulse Response | Order | Unable to design
/// |:------|:-----------------|:------|:--------|
/// | I     | Symmetric        | Odd   | --
/// | II    | Symmetric        | Even  | HP
/// | III   | Anti-symmetric   | Odd   | HP / LP
/// | IV    | Anti-symmetric   | Even  | LP
///
template<typename T>
struct DirectForm {
 protected:
    /// the delay line for the feed-forward network
    Math::CircularBuffer<T> delay;
    /// the coefficients of the filter
    std::vector<T> coefficients;

 public:
    /// @brief Initialize a new FIR filter.
    ///
    /// @param order the number of coefficients/taps in the filter
    ///
    explicit DirectForm(const std::size_t& order = 1) :
        delay(order),
        coefficients(order) {
        if (order < 1) throw "order must be 1 or more";
    }

    /// @brief Set the number of coefficients and delay line length.
    ///
    /// @param order the number of coefficients / length of the delay line
    ///
    inline void resize(const std::size_t& order) {
        if (order < 1) throw "order must be 1 or more";
        delay.resize(order);
        coefficients.resize(order);
    }

    /// @brief Return the number of coefficients in the FIR filter.
    ///
    /// @returns the number of coefficients / length of the delay line
    ///
    inline std::size_t size() const { return delay.size(); }

    /// @brief Return a reference to the coefficient at given index.
    ///
    /// @param index the index of the coefficient to return a reference to
    /// @details
    /// If the index is not within the range of the filter length, an
    /// exception of type `std::out_of_range` will throw.
    ///
    inline T& coefficient(const std::size_t& pos) {
        return coefficients.at(pos);
    }

    /// @brief Clear the coefficients in the filter.
    inline void clearCoefficients() {
        std::fill(coefficients.begin(), coefficients.end(), 0);
    }

    /// @brief set the low-pass cut-off frequency of the filter to a new value.
    ///
    /// @tparam ImpulseFunction the type of the impulse function
    /// @tparam WindowFunction the type of the windowing function
    /// @tparam Args the parameter pack type for the critical frequency args
    /// @param impulse an impulse train function for generating filter samples
    /// @param window a windowing function to use when assigning coefficients
    /// @param cutoff the cutoff frequency \f$\in [0, 1]\f$ or the low and high
    /// bands \f$\in [0, 1]\f$.
    /// @details
    /// `impulse` should accept three arguments, i.e., `int n` (the filter tap
    /// index), `int N` (filter order), and `float cutoff` the cutoff frequency.
    /// This function returns the impulse for the given sample of the filter of
    /// given length with given cut-off frequency.
    ///
    /// `window` should accept two arguments of type `T`, i.e., `n` (filter tap
    /// index) and `N` (filter order), and return the window function value
    /// based on `n` and `N`.
    ///
    /// `cutoff` can be either a single cut-off frequency for setting high/low
    /// pass filters or any parameter pack of critical frequencies to pass to
    /// the `impulse` function.
    ///
    template<typename ImpuleFunction, typename WindowFunction, typename ... Args>
    inline void setCoefficients(
        const ImpuleFunction& impulse,
        const WindowFunction& window,
        const Args& ... cutoff
    ) {
        // Cache the order of the filter in a calculable form.
        const T N = coefficients.size();
        for (unsigned i = 0; i < coefficients.size(); i++)  // Iterate over taps
            // Get the impulse response for this sample using a low-pass sinc
            // function with given cut-off in [0, 1] where 1 is the highest
            // frequency, i.e., the Nyquist rate (1 / sample rate). Apply a
            // symmetric window function to the sinc function to meet the
            // filter design specifications in terms of stop-band attenuation,
            // side-lobe amplitude, and transition width (relative to the order
            // of the filter).
            coefficients.at(i) = impulse(i, N, cutoff...) * window(i, N, true);
    }

    /// @brief Return a reference to the coefficient at given index.
    ///
    /// @param index the index of the coefficient to return a reference to
    /// @details
    /// If the index is not within the range of the filter length, an
    /// exception of type `std::out_of_range` will throw.
    ///
    inline const T& delayedSample(const std::size_t& pos) {
        return delay.at(pos);
    }

    /// @brief Clear the contents of the delay line.
    inline void reset() { delay.clear(); }

    /// @brief Insert a sample directly into the delay line.
    ///
    /// @param value the value to insert into the delay line
    ///
    inline void process_delay(const T& value = 0) { delay.insert(value); }

    /// @brief Pass a value through the FIR filter and return the output.
    ///
    /// @param value the value to process with the FIR filter
    /// @returns the current output from the FIR filter
    ///
    inline T process(const T& value) {
        // Update the delay line with the current value.
        delay.insert(value);
        // Accumulate the filter output as the inner product of the delay line
        // and the coefficients.
        T accumulator = 0;
        for (std::size_t i = 0; i < size(); i++)
            accumulator += coefficients.at(i) * delay.at(-i);
        return accumulator;
    }
};

}  // namespace FIR

}  // namespace Filter

#endif  // DSP_FILTER_FIR_DIRECT_FORM_HPP

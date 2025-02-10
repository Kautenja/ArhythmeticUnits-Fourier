// An IIR filter in direct form 2 configuration.
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

#ifndef DSP_FILTER_IIR_BIPOLYNOMIAL_DIRECT_FORM_2_HPP
#define DSP_FILTER_IIR_BIPOLYNOMIAL_DIRECT_FORM_2_HPP

#include <vector>
#include "../../../math/circular_buffer.hpp"

/// @brief Structure for digital filtering.
namespace Filter {

/// @brief IIR filter designs.
namespace IIR {

/// @brief Bipolynomial IIR filter designs (i.e., N'th-order).
namespace Bipolynomial {

/// @brief An IIR filter in direct form 2 configuration.
/// @tparam T the data type to process and use for coefficients
/// @details
/// The network structure of the filter looks like:
///
/// ```
/// x[n] -+----------•-- *b0 -->+--> y[n]
///       |          |          |
///       |         z-1         |
///       |          |          |
///       +<- *-a0 --•-- *b1 -->+
///       |          |          |
///       |         z-1         |
///       |          |          |
///       +<- *-a1 --•-- *b2 -->+
///       |         ...         |
/// ```
///
template<typename T>
struct DirectForm2 {
    /// the coefficients of the numerator of the filter, i.e., \f$b\f$
    std::vector<T> b;
    /// the coefficients of the denominator of the filter, i.e., \f$a\f$
    std::vector<T> a;

    /// the central delay line for the network
    Math::CircularBuffer<T> h;

    /// @brief Initialize a new direct form 2 IIR filter.
    ///
    /// @param order the number of taps in the delay line
    ///
    explicit DirectForm2(const std::size_t& order = 1) :
        b(order + 1),
        a(order),
        h(order) { }

    /// @brief Set the order of the filter.
    ///
    /// @param order the number of taps in the delay line
    ///
    inline void setOrder(std::size_t order) {
        if (order < 1) throw "order must be 1 or greater";
        b.resize(order + 1);
        a.resize(order);
        h.resize(order);
    }

    /// @brief Return the order of the filter.
    ///
    /// @returns the number of taps in the delay line
    ///
    inline std::size_t getOrder() const { return h.size(); }

    /// @brief Clear the contents of the delay lines.
    inline void reset() { h.clear(); }

    /// @brief Pass a value through the IIR filter and return the output.
    ///
    /// @param value the value to process with the IIR filter
    /// @returns the current output from the IIR filter
    ///
    inline T process(const T& value) {
        // the result of the feedback network (start with the input value)
        T feedback = value;
        // the result of the feed-forward network
        T feedforward = 0;
        // iterate over the samples in the delay line. Start at index 1 to
        // index the first coefficient
        for (std::size_t i = 0; i < getOrder(); i++) {
            feedback -= a.at(i) * h.at(-i);
            feedforward += b.at(i + 1) * h.at(-i);
        }
        // insert the sum of the feedback network and the input value into the
        // delay line
        h.insert(feedback);
        // return the result of the feed-forward network added to the product
        // of the feedback network and 0th b coefficient
        return feedback * b.at(0) + feedforward;
    }
};

}  // namespace Bipolynomial

}  // namespace IIR

}  // namespace Filter

#endif  // DSP_FILTER_IIR_BIPOLYNOMIAL_DIRECT_FORM_2_HPP

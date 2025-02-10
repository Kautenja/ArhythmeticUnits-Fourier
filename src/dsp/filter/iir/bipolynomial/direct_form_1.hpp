// An IIR filter in direct form 1 configuration.
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

#ifndef DSP_FILTER_IIR_BIPOLYNOMIAL_DIRECT_FORM_1_HPP
#define DSP_FILTER_IIR_BIPOLYNOMIAL_DIRECT_FORM_1_HPP

#include <vector>
#include "../../../math/circular_buffer.hpp"

/// @brief Structure for digital filtering.
namespace Filter {

/// @brief IIR filter designs.
namespace IIR {

/// @brief Bipolynomial IIR filter designs (i.e., N'th-order).
namespace Bipolynomial {

/// @brief An IIR filter in direct form 1 configuration.
/// @tparam T the data type to process and use for coefficients
/// @details
/// The network structure of the filter looks like:
///
/// ```
/// x[n] -•-- *b0 ---+----------•--> y[n]
///       |          |          |
///      z-1         |         z-1
///       |          |          |
///       •-- *b1 -->+<- *-a0 --•
///       |          |          |
///      z-1         |         z-1
///       |          |          |
///       •-- *b2 -->+<- *-a1 --•
///       |         ...         |
/// ```
///
template<typename T>
struct DirectForm1 {
    /// the coefficients of the numerator of the filter, i.e., \f$b\f$
    std::vector<T> b;
    /// the coefficients of the denominator of the filter, i.e., \f$a\f$
    std::vector<T> a;

    /// the delay line for the feed-forward network, i.e., \f$x\f$
    Math::CircularBuffer<T> x;
    /// the delay line for the feedback network, i.e., \f$y\f$
    Math::CircularBuffer<T> y;

    /// @brief Initialize a new direct form 1 IIR filter.
    ///
    /// @param order the number of taps in the delay line
    ///
    explicit DirectForm1(const std::size_t& order = 1) :
        b(order + 1),
        a(order),
        x(order),
        y(order) { }

    /// @brief Set the order of the filter.
    ///
    /// @param order the number of taps in the delay line
    ///
    inline void setOrder(std::size_t order) {
        if (order < 1) throw "order must be 1 or greater";
        x.resize(order);
        y.resize(order);
        b.resize(order + 1);
        a.resize(order);
    }

    /// @brief Return the order of the filter.
    ///
    /// @returns the number of taps in the delay line
    ///
    inline std::size_t getOrder() const { return x.size(); }

    /// @brief Clear the contents of the delay lines.
    inline void reset() { x.clear(); y.clear(); }

    /// @brief Pass a value through the IIR filter and return the output.
    ///
    /// @param value the value to process with the IIR filter
    /// @returns the current output from the IIR filter
    ///
    inline T process(const T& value) {
        // create a single accumulator for the loop to put values into
        T accumulator = b.at(0) * value;
        for (std::size_t i = 0; i < getOrder(); i++) {
            // add the feed-forward branch of the network to the sum
            accumulator += b.at(i + 1) * x.at(-i);
            // subtract the feedback branch of the network from the sum
            accumulator -= a.at(i) * y.at(-i);
        }
        // insert the un-filtered value into the feed-forward network delay line
        x.insert(value);
        // insert the fully filtered signal into the feedback delay line
        y.insert(accumulator);
        // return the accumulator
        return accumulator;
    }
};

}  // namespace Bipolynomial

}  // namespace IIR

}  // namespace Filter

#endif  // DSP_FILTER_IIR_BIPOLYNOMIAL_DIRECT_FORM_1_HPP

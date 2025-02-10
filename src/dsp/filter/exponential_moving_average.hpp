// A structure for calculating an exponential moving average/variance.
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

#ifndef DSP_FILTER_EMA_HPP
#define DSP_FILTER_EMA_HPP

#include <algorithm>
#include <cmath>

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief Convert a cut-off frequency to an \f$\alpha\f$ for a moving average.
///
/// @param f the normalized frequency \f$\in [0, PI]\f$
/// @returns the \f$\alpha\f$ parameter for an EMA \f$\in [0, >1]\f$
///
template<typename T>
inline T cutoff_to_alpha(T f) {
    const T cos_f = cos(f);
    return cos_f - 1 + sqrt(cos_f * cos_f - 4 * cos_f + 3);
}

/// @brief Convert the alpha of a moving average to its cut-off frequency.
///
/// @param a the \f$\alpha\f$ parameter of an EMA \f$\in [0, >1]\f$
/// @returns the normalized cut-off frequency \f$\in [0, PI]\f$
///
template<typename T>
inline T alpha_to_cutoff(T a) {
    return acos((a * a + 2 * a - 2) / (2 * a - 2));
}

/// @brief A structure for calculating an exponential moving average.
template<typename T>
struct ExponentialMovingAverage {
 private:
    /// the alpha parameter of the exponential moving average
    T alpha = 0;
    /// the exponential moving average value
    T average = 0;

 public:
    /// @brief Set the \f$\alpha\f$ parameter.
    ///
    /// @param a the new value for the \f$\alpha\f$ parameter
    ///
    inline void setAlpha(const T& a) { alpha = std::max(a, static_cast<T>(0)); }

    /// @brief Return the \f$\alpha\f$ parameter.
    ///
    /// @returns the \f$\alpha\f$ parameter of the moving average
    ///
    inline T getAlpha() const { return alpha; }

    /// @brief Return the average \f$\mu\f$.
    inline T getAverage() const { return average; }

    /// @brief Process a sample.
    ///
    /// @param value the input sample to process
    ///
    inline void process(const T& value) {
        average += alpha * (value - average);
    }
};

/// @brief A structure for calculating an exponential moving variance.
template<typename T>
struct ExponentialMovingVariance {
 private:
    /// the alpha parameter of the exponential moving average
    T alpha = 0;
    /// the exponential moving average value
    T average = 0;
    /// the exponential moving variance value
    T variance = 0;

 public:
    /// @brief Set the \f$\alpha\f$ parameter.
    ///
    /// @param a the new value for the \f$\alpha\f$ parameter
    ///
    inline void setAlpha(const T& a) { alpha = std::max(a, static_cast<T>(0)); }

    /// @brief Return the \f$\alpha\f$ parameter.
    ///
    /// @returns the \f$\alpha\f$ parameter of the moving average
    ///
    inline T getAlpha() const { return alpha; }

    /// @brief Return the average \f$\mu\f$.
    inline T getAverage() const { return average; }

    /// @brief Return the variance \f$\sigma^2\f$.
    inline T getVariance() const { return variance; }

    /// @brief Process a sample.
    ///
    /// @param value the input sample to process
    ///
    inline void process(const T& value) {
        const auto delta = value - average;
        average = average + alpha * delta;
        variance = (1 - alpha) * (variance + alpha * delta * delta);
    }
};

}  // namespace Filter

#endif  // DSP_FILTER_EMA_HPP

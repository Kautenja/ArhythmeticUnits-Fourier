// An FIR filter in lattice structure.
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

#ifndef DSP_FILTER_LATTICE_FIR_HPP
#define DSP_FILTER_LATTICE_FIR_HPP

#include <cstddef>

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief Lattice filter designs.
namespace Lattice {

/// @brief A lattice stage for an FIR filter.
/// @tparam T the data type to process and use for coefficients
template<typename T>
struct StageFIR {
    enum {
        /// the minimal reflection coefficient value that guarantees stability
        B_MIN = -1,
        /// the maximal reflection coefficient value that guarantees stability
        B_MAX = 1
    };

    /// the coefficient of the filter \f$\Gamma\f$
    T b = 0;

    /// the delay line for the feed-forward network \f$h = g * z^{-1}\f$
    T h = 0;

    /// @brief Clear the contents of the delay line.
    inline void reset() { h = 0; }

    /// @brief Pass inputs through the filter
    ///
    /// @param f the input for the \f$f\f$ function of the lattice filter
    /// @param g the input for the \f$g\f$ function of the lattice filter
    ///
    inline void _process(T& f, T& g) {
        // backup g so it can be safely overwritten
        const T g_backup = g;
        // calculate the outputs for both sides of the lattice
        g = h + b * f;
        f = f + b * h;
        // put the g input into the delay line
        h = g_backup;
    }

    /// @brief Reset all the stages of a lattice filter.
    ///
    /// @param stages the array of filter stages to process
    /// @param N the number of stages in the cascade
    ///
    static inline void reset(StageFIR<T>* stages, std::size_t N) {
        for (unsigned i = 0; i < N; i++) stages[i].reset();
    }

    /// @brief Process a value using a cascade of FIR lattice stages.
    ///
    /// @param stages the array of filter stages to process
    /// @param N the number of stages in the cascade
    /// @param k the input for the reflection coefficients \f$b\f$
    ///
    static inline void setCoefficients(StageFIR<T>* stages, std::size_t N, T* k) {
        for (unsigned i = 0; i < N; i++) stages[i].b = k[i];
    }

    /// @brief Process a value using a cascade of FIR lattice stages.
    ///
    /// @param stages the array of filter stages to process
    /// @param N the number of stages in the cascade
    /// @param f the input for the \f$f\f$ function of the lattice filter
    /// @param g the input for the \f$g\f$ function of the lattice filter
    /// @details
    /// `f` and `g` are updated with the \f$f\f$ and \f$g\f$ outputs from the
    /// network
    ///
    static inline void process(StageFIR<T>* stages, std::size_t N, T& f, T& g) {
        for (unsigned i = 0; i < N; i++) stages[i]._process(f, g);
    }
};

}  // namespace Lattice

}  // namespace Filter

#endif  // DSP_FILTER_LATTICE_FIR_HPP

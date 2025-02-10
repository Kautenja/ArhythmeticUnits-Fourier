// An IIR filter in lattice structure.
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

#ifndef DSP_FILTER_LATTICE_IIR_HPP
#define DSP_FILTER_LATTICE_IIR_HPP

#include <cstddef>

/// @brief Structures for digital filtering.
namespace Filter {

/// @brief Lattice filter designs.
namespace Lattice {

/// @brief A lattice stage for an IIR filter.
/// @tparam T the data type to process and use for coefficients
template<typename T>
struct StageIIR {
    enum {
        /// the minimal reflection coefficient value that guarantees stability
        B_MIN = -1,
        /// the maximal reflection coefficient value that guarantees stability
        B_MAX = 1
    };

    /// the coefficient of the all-pole lattice filter \f$\Gamma\f$
    T b = 0;

    /// the tapped delay line coefficient of the filter \f$c\f$
    T c = 0;

    /// the delay line for the feedback network \f$y\f$
    T g = 0;

    /// @brief Clear the contents of the delay line.
    inline void reset() {
        g = 0;
    }

    /// @brief Pass inputs through the filter
    ///
    /// @param f the input for the \f$f\f$ function of the lattice filter
    ///
    inline void _process(T& f) {
        f = f + -b * g;
        g = g +  b * f;
    }

    /// @brief Reset all the stages of a lattice filter.
    ///
    /// @param stages the array of filter stages to process
    /// @param N the number of stages in the cascade
    ///
    static inline void reset(StageIIR<T>* stages, std::size_t N) {
        for (unsigned i = 0; i < N; i++) stages[i].reset();
    }

    /// @brief Process a value using a cascade of FIR lattice stages.
    ///
    /// @param stages the array of filter stages to process
    /// @param N the number of stages in the cascade
    /// @param k the input for the IIR coefficients \f$b\f$
    ///
    static inline void setCoefficientsPoles(StageIIR<T>* stages, std::size_t N, T* k) {
        for (unsigned i = 0; i < N; i++) stages[i].b = k[i];
    }

    /// @brief Process a value using a cascade of FIR lattice stages.
    ///
    /// @param stages the array of filter stages to process
    /// @param N the number of stages in the cascade
    /// @param k the input for the FIR coefficients \f$c\f$
    ///
    static inline void setCoefficientsZeros(StageIIR<T>* stages, std::size_t N, T* v) {
        for (unsigned i = 0; i < N; i++) stages[i].c = v[i];
    }

    /// @brief Process a value using a cascade of FIR lattice stages.
    ///
    /// @param stages the array of filter stages to process
    /// @param N the number of stages in the cascade
    /// @param f the input for the \f$f\f$ function of the lattice filter
    /// @param g the initial tapped delay line coefficient \f$c_0\f$
    /// @details
    /// `f` and `g` are updated with the \f$f\f$ and \f$g\f$ outputs from the
    /// network
    ///
    static inline void process(StageIIR<T>* stages, std::size_t N, T& f, T& g) {
        // first stage (guaranteed length of 1), process `f` and `g`
        stages[0]._process(f);
        // set `y` output from the `g` delay line and `c` coefficient.
        T y = stages[0].c * stages[0].g;
        // backup the `w` output because stages[0].g will change
        const T w = stages[0].g;
        if (N > 1) {  // more than one stage
            for (std::size_t i = 1; i < N; i++) {
                // process and set the `g` of last stage to the current stage
                stages[i]._process(f);
                stages[i - 1].g = stages[i].g;
                // update the delay line accumulator from current `g` and `c`
                y += stages[i].c * stages[i].g;
            }
        }
        // last stage, set the `g` delay line value to the output `f` value
        stages[N - 1].g = f;
        // update the `f` output based on the `c0` that is passed through `g`
        f = g * f + y;
        // set `g` to the `w` output from the network
        g = w;
    }
};

}  // namespace Lattice

}  // namespace Filter

#endif  // DSP_FILTER_LATTICE_IIR_HPP

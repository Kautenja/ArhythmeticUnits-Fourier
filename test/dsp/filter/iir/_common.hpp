// Constant values for testing IIR filters.
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

#ifndef TEST_FILTER_BIQUAD_COMMON_HPP
#define TEST_FILTER_BIQUAD_COMMON_HPP

/// @brief Process samples and compare the output to an expected vector.
///
/// @tparam the type of the filter and the target vector
/// @param filter the filter to process samples with
/// @param input the input samples to pass through the filter
/// @param output the expected outputs from the filter for the given inputs
///
template<typename F, typename T>
inline void process_compare(F& filter,
    const float input[30],
    const T target[30]
) {
    for (unsigned i = 0; i < 30; i++) {
        auto sample = filter.process(input[i]);
        REQUIRE(sample == Approx(target[i]));
    }
}

#endif  // TEST_FILTER_BIQUAD_COMMON_HPP

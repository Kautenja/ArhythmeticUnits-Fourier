// A global header file for the filter package.
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

#ifndef DSP_FILTER_HPP
#define DSP_FILTER_HPP

// TODO: "Schur-Cohn stability" (see below)
// TODO: linked list lattice structure as a delay line thing for granular?
// TODO: "An important property of the lattice filter is that the roots of
// A,(z) will lie inside the unit circle if and only if the reflection
// coefficients are bounded by I in magnitude:This property is the basis for
// the Schur-Cohn stability test for digital filters."

// == IIR ==
// TODO: butterworth
// TODO: chebyshev (both types)
// TODO: elliptical
// TODO: Parks McClellan
// TODO: Kerwin-Huelsman-Newcomb
// TODO: Delyiannis-Friend

// == FIR ==
// TODO: DFT design (frequency response sampling design)
// TODO: basic order 1 and order 2 FIR filter structures for cascade
// TODO: structures for Type 1, Type 2, Type 3, and Type 4 FIR filters

// == cascade ==
// TODO: Steiner-Parker filter (MicroBrute)
// TODO: CEM3320 (Oberheim, Prophet)
// TODO: SEM Filter (now in MicroFreak)
// TODO: wasp filter

#include "filter/compression_filter.hpp"
#include "filter/exponential_moving_average.hpp"
#include "filter/fir.hpp"
#include "filter/iir.hpp"
#include "filter/moving_average.hpp"
#include "filter/root_mean_square.hpp"
#include "filter/vactrol.hpp"
#include "filter/lattice.hpp"

#endif  // DSP_FILTER_HPP

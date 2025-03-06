// The Arhythmetic Units "Fourier" VCVRack plugin.
//
// Copyright 2024 Arhythmetic Units
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_PLUGIN_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PLUGIN_HPP_

#include "rack.hpp"
using namespace rack;

/// the global instance of the VCV Rack plug-in.
extern Plugin* plugin_instance;

#include "./json.hpp"
#include "./rack_extensions/param_quantity.hpp"
#include "./rack_extensions/menu_item.hpp"
#include "./rack_extensions/text_knob.hpp"
#include "./rack_extensions/graphics.hpp"
#include "./rack_extensions/theme.hpp"

/// The "Fourier" Spectrogram analyzer module.
extern Model *modelSpectrogram;
extern Model *modelSpectrumAnalyzer;

#endif  // ARHYTHMETIC_UNITS_FOURIER_PLUGIN_HPP_

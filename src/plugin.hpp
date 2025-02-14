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

/// @brief A menu item for changing boolean parameters.
struct FlagMenuItem : rack::MenuItem {
    /// @brief The flag to update.
    bool* flag = nullptr;

    /// @brief Respond to the menu item being selected.
    inline void onAction(const rack::event::Action& e) override {
        (*flag) = !(*flag);
    }
};

using namespace rack;

/// the global instance of the VCV Rack plug-in.
extern rack::Plugin* plugin_instance;

#include "./json.hpp"
#include "./param_quantity.hpp"
#include "./theme.hpp"

/// The "Fourier" Spectrogram analyzer module.
extern rack::Model *modelSpectrogram;
extern rack::Model *modelSpectrumAnalyzer;

#endif  // ARHYTHMETIC_UNITS_FOURIER_PLUGIN_HPP_

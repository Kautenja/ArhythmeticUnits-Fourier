// Undoable changes to serialized module settings.
//
// Copyright 2026 Arhythmetic Units
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

#ifndef ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_SETTINGS_HISTORY_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_SETTINGS_HISTORY_HPP_

#include <memory>
#include <string>
#include "rack.hpp"

namespace Fourier {

/// @brief Change a custom JSON setting and record one Rack undo action.
/// @param module The registered module whose setting is being changed.
/// @param name The action label shown in Rack's Edit menu.
/// @param key The existing key in the module's custom JSON data.
/// @param value The new JSON value; ownership is transferred to this function.
/// @details Call only from the UI thread. Engine serialization APIs coordinate
/// with processing; do not mutate engine-owned settings directly from menus.
/// Equal values do not create an action or discard the current redo history.
inline void set_module_setting(rack::engine::Module* module,
                              const std::string& name,
                              const char* key, json_t* value) {
    std::unique_ptr<json_t, decltype(&json_decref)> new_value(value, json_decref);
    std::unique_ptr<rack::history::ModuleChange> action(new rack::history::ModuleChange);
    action->oldModuleJ = nullptr;
    action->newModuleJ = nullptr;
    action->oldModuleJ = APP->engine->moduleToJson(module);
    auto old_data = json_object_get(action->oldModuleJ, "data");
    if (json_equal(json_object_get(old_data, key), new_value.get())) return;
    action->newModuleJ = json_deep_copy(action->oldModuleJ);
    json_object_set(json_object_get(action->newModuleJ, "data"), key, new_value.get());
    action->name = name;
    action->moduleId = module->id;
    APP->engine->moduleFromJson(module, action->newModuleJ);
    APP->history->push(action.release());
}

}  // namespace Fourier

#endif  // ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_SETTINGS_HISTORY_HPP_

// Strict JSON boundary for explicit workload controls; no JSON in sample work.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_WORKLOAD_JSON_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_WORKLOAD_JSON_HPP_
#include <set>
#include <jansson.h>
#include "protocol.hpp"
namespace Paper {
inline const std::set<std::string>& control_fields() {
    static const std::set<std::string> fields = {"workload_schema", "window", "octave", "temporal_mode",
        "temporal_value", "fixture", "fixture_seed", "decay_samples", "active_ports", "execution_regime", "experimental_policy"};
    return fields;
}
inline void parse_workload_controls(Config& c, json_t* value, bool controls_only = true) {
    require(json_is_object(value), "Expected workload controls object");
    for (const auto& field : control_fields()) require(json_object_get(value, field.c_str()), "Missing workload control");
    const char* key; json_t* item;
    json_object_foreach(value, key, item) {
        const std::string name(key);
        if (!control_fields().count(name)) { require(!controls_only, "Unknown workload control"); continue; }
        if (name == "window" || name == "temporal_mode" || name == "fixture" ||
                name == "execution_regime" || name == "experimental_policy") {
            require(json_is_string(item), "Expected string workload control");
            const std::string text(json_string_value(item));
            if (name == "window") c.window = text;
            if (name == "temporal_mode") c.temporal_mode = text;
            if (name == "fixture") c.fixture = text;
            if (name == "execution_regime") c.execution_regime = text;
            if (name == "experimental_policy") c.experimental_policy = text;
        } else if (name == "octave" || name == "temporal_value") {
            require(json_is_number(item), "Expected numeric workload control");
            if (name == "octave") c.octave = json_number_value(item);
            else c.temporal_value = json_number_value(item);
        } else {
            require(json_is_integer(item) && json_integer_value(item) >= 0 &&
                json_integer_value(item) <= UINT32_MAX, "Invalid integer workload control");
            const size_t number = size_t(json_integer_value(item));
            if (name == "workload_schema") { require(number == 3, "Unknown workload schema"); c.workload_schema = number; }
            if (name == "fixture_seed") c.fixture_seed = number;
            if (name == "decay_samples") c.decay_samples = number;
            if (name == "active_ports") c.active_ports = number;
        }
    }
}
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_WORKLOAD_JSON_HPP_

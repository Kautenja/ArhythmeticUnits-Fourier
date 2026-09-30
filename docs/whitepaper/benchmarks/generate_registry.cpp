// Native build-time registry compiler. Python remains an archival/test reference.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <jansson.h>

namespace {
using Json = std::shared_ptr<json_t>;
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
Json own(json_t* value) { check(value != nullptr, "Invalid JSON"); return Json(value, json_decref); }
std::string encode(json_t* value) {
    char* bytes = json_dumps(value, JSON_ENCODE_ANY | JSON_SORT_KEYS);
    check(bytes != nullptr, "JSON encoding failed");
    const std::string result(bytes); std::free(bytes); return result;
}
std::string text(json_t* value) {
    check(json_is_string(value), "Expected string"); return json_string_value(value);
}
bool member(const std::string& value, const std::string& values) {
    return ("|"+values+"|").find("|"+value+"|") != std::string::npos;
}
}

int main(int argc, char** argv) {
    try {
        check(argc == 4, "Usage: generate-registry registry.json output.hpp 'features'");
        std::set<std::string> features;
        std::istringstream requested(argv[3]); std::string feature;
        while (requested >> feature) {
            check(feature == "fftw" || feature == "vdsp", "Unknown build feature"); features.insert(feature);
        }
        json_error_t error{};
        auto document = own(json_load_file(argv[1], JSON_REJECT_DUPLICATES, &error));
        check(json_integer_value(json_object_get(document.get(), "schema")) == 1, "Unsupported registry schema");
        auto defaults = own(json_deep_copy(json_object_get(document.get(), "defaults")));
        check(json_is_object(defaults.get()), "Missing defaults");
        json_object_set_new(defaults.get(), "id", json_string(""));
        std::vector<std::string> fields;
        std::ostringstream header;
        header << "// Generated from docs/whitepaper/benchmarks/backends.json; do not edit.\nstruct BackendDescriptor {\n";
        const char* key; json_t* value;
        json_object_foreach(defaults.get(), key, value) {
            fields.push_back(key);
            check(json_is_boolean(value) || json_is_integer(value) || json_is_string(value), "Invalid field type");
            header << "    " << (json_is_boolean(value) ? "bool" : json_is_integer(value) ? "size_t" : "const char*")
                << ' ' << key << ";\n";
        }
        header << "};\nstatic const BackendDescriptor backend_registry[] = {\n";
        auto registry = own(json_object());
        json_t* backends = json_object_get(document.get(), "backends");
        check(json_is_array(backends) && json_array_size(backends), "Missing backends");
        size_t index;
        json_array_foreach(backends, index, value) {
            check(json_is_object(value), "Invalid descriptor");
            auto descriptor = own(json_deep_copy(defaults.get()));
            check(json_object_update(descriptor.get(), value) == 0, "Invalid descriptor override");
            check(json_object_size(descriptor.get()) == fields.size(), "Unknown capability field");
            for (const auto& field : fields) {
                json_t* v = json_object_get(descriptor.get(), field.c_str());
                json_t* d = json_object_get(defaults.get(), field.c_str());
                check((json_is_boolean(v) && json_is_boolean(d)) || json_typeof(v) == json_typeof(d), "Invalid capability type");
                if (json_is_string(v)) for (char c : text(v))
                    check(c >= 32 && c <= 126 && c != '\\' && c != '"', "Invalid capability string");
                if (json_is_integer(v)) check(json_integer_value(v) >= 0, "Negative capability bound");
            }
            auto get = [&](const char* field) { return json_object_get(descriptor.get(), field); };
            check(json_integer_value(get("size_min")) >= 1
                && json_integer_value(get("size_max")) >= json_integer_value(get("size_min"))
                && json_integer_value(get("size_multiple")) >= 1 && json_integer_value(get("channels")) >= 1,
                "Invalid capability bounds");
            check(member(text(get("boundary")), "analysis|module|transform|inverse-job|chain|control")
                && member(text(get("schedule")), "balanced|immediate|legacy-budget|transform|none")
                && member(text(get("step_model")), "none|opaque|radix2-real|radix2-complex|radix2-inverse")
                && member(text(get("precision")), "float|double"), "Unknown capability semantics");
            const auto id = text(get("id"));
            check(!id.empty() && !json_object_get(registry.get(), id.c_str()), "Duplicate or empty backend identity");
            const auto feature = text(get("build_feature"));
            if (!feature.empty()) {
                check(feature == "fftw" || feature == "vdsp", "Unknown optional feature");
                const bool enabled = features.count(feature);
                json_object_set_new(descriptor.get(), "available", json_boolean(enabled));
                json_object_set_new(descriptor.get(), "reason", json_string(enabled ? "" :
                    ("Optional benchmark feature is disabled: "+feature).c_str()));
            }
            json_object_set(registry.get(), id.c_str(), descriptor.get());
            header << "    {";
            for (size_t f = 0; f < fields.size(); ++f) {
                if (f) header << ", ";
                header << encode(get(fields[f].c_str()));
            }
            header << "},\n";
        }
        auto literal = own(json_string(encode(registry.get()).c_str()));
        header << "};\nstatic const char* registry_json = " << encode(literal.get()) << ";\n";
        // Make invokes this only when a registry/build input changes. Refresh the
        // timestamp even if the bytes match, so incremental builds stay quiet.
        std::ofstream output(argv[2]); output << header.str(); output.close();
        check(bool(output), "Registry write failed");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}

// Optional coarse runtime lifecycle; JSON validation belongs to the runner tests.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iostream>
#include <stdexcept>
#include <string>
#include "../../benchmark/paper/runtime.hpp"

void require(bool valid, const char* message) {
    if (!valid) throw std::runtime_error(message);
}

int main(int argc, char** argv) {
    using namespace Paper::Runtime;
    try {
        require(argc == 2, "Expected complete, failure or disabled");
        const std::string mode(argv[1]);
        require(mode == "complete" || mode == "failure" || mode == "disabled", "Unknown mode");
        {
            Profile profile(mode != "disabled");
            const bool enabled = mode != "disabled" && std::getenv("PAPER_RUNTIME_PATH")
                && *std::getenv("PAPER_RUNTIME_PATH");
            require(bool(active()) == enabled, "Runtime opt-in differs");
            set(Phase::Setup);
            if (enabled) require(active()->current() == Phase::Setup, "Setup phase missing");
            {
                Scope nested(Phase::TimedWarmup);
                if (enabled) require(active()->current() == Phase::TimedWarmup, "Nested phase missing");
            }
            if (enabled) require(active()->current() == Phase::Setup, "Phase was not restored");
            if (mode == "failure") throw std::runtime_error("Intentional workload failure");
            set(Phase::Measurement);
            set(Phase::Other);
            profile.finish();
            require(!active(), "Completed runtime profile stayed active");
        }
        require(!active(), "Runtime destructor left dangling state");
        std::cout << "Runtime fixture passed\n";
    } catch (const std::exception& error) {
        if (active()) return 2;
        std::cerr << error.what() << '\n';
        return 1;
    }
}

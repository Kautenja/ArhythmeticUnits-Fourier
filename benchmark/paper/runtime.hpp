// Optional coarse process attribution, separate from benchmark observations.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_RUNTIME_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_RUNTIME_HPP_

#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>

namespace Paper {
namespace Runtime {
enum class Phase {
    Setup, TimedSetup, TimedWarmup, TimerCalibration, Measurement,
    PostMeasurementChecks, TimedTeardown, ReplaySetup, ReplayWarmup,
    CorrectnessReplay, ReplayTeardown, CsvOutput, Other, Count
};

class Profile;
inline Profile*& active() {
    static Profile* profile = nullptr;
    return profile;
}

/// @brief Mutually exclusive wall durations covering one successful execute().
/// @details Only coarse boundaries call change(); never call it from a measured
/// callback, transform, or sample loop. The sidecar excludes process startup,
/// argument parsing and its own serialization. Failed executions do not finish.
class Profile {
    using Clock = std::chrono::steady_clock;
    std::string path;
    std::array<double, static_cast<size_t>(Phase::Count)> durations{};
    Clock::time_point start, previous;
    Phase phase = Phase::Other;
    bool enabled = false;

    void accrue(Clock::time_point now) {
        durations[static_cast<size_t>(phase)] +=
            std::chrono::duration<double, std::nano>(now-previous).count();
        previous = now;
    }

 public:
    explicit Profile(bool allowed = true) {
        const char* requested = allowed ? std::getenv("PAPER_RUNTIME_PATH") : nullptr;
        if (!requested || !*requested) return;
        if (active()) throw std::logic_error("Nested paper runtime profile");
        path = requested;
        start = previous = Clock::now();
        enabled = true;
        active() = this;
    }
    ~Profile() { if (active() == this) active() = nullptr; }
    Profile(const Profile&) = delete;
    Profile& operator=(const Profile&) = delete;

    Phase current() const { return phase; }
    void change(Phase next) {
        accrue(Clock::now());
        phase = next;
    }
    /// @brief Write only after all workload checks and output flushes succeeded.
    void finish() {
        if (!enabled) return;
        const auto end = Clock::now();
        accrue(end);
        enabled = false;
        active() = nullptr;
        const char* names[] = {"setup", "timed_setup", "timed_warmup", "timer_calibration",
            "measurement", "post_measurement_checks", "timed_teardown", "replay_setup",
            "replay_warmup", "correctness_replay", "replay_teardown", "csv_output", "other"};
        std::ofstream output(path);
        if (!output) throw std::runtime_error("Cannot create paper runtime sidecar");
        output.precision(17);
        output << "{\"schema\":1,\"status\":\"complete\",\"scope\":\"benchmark_execute\",\"unit\":\"ns\",\"total_ns\":"
            << std::chrono::duration<double, std::nano>(end-start).count() << ",\"phases_ns\":{";
        for (size_t i = 0; i < durations.size(); ++i) {
            if (i) output << ',';
            output << '"' << names[i] << "\":" << durations[i];
        }
        output << "}}\n";
        output.close();
        if (!output) throw std::runtime_error("Cannot write paper runtime sidecar");
    }
};

inline void set(Phase phase) { if (active()) active()->change(phase); }

/// @brief Restore a surrounding coarse phase after a nested untimed operation.
class Scope {
    Phase previous;
 public:
    explicit Scope(Phase phase) : previous(active() ? active()->current() : Phase::Other) { set(phase); }
    ~Scope() { set(previous); }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};
}  // namespace Runtime
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_RUNTIME_HPP_

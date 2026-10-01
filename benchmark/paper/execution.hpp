// Versioned measurement scheduling; independent of DSP and Rack types.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_EXECUTION_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_EXECUTION_HPP_

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include "measurement_clock.hpp"
#ifndef _WIN32
#include <pthread.h>
#include <signal.h>
#include <time.h>
#endif
#ifdef __SSE__
#include <xmmintrin.h>
#endif

namespace Paper {
namespace Execution {
inline void check(bool valid, const char* message) {
    if (!valid) throw std::runtime_error(message);
}
inline std::string environment(const char* name, const char* fallback = "") {
    const char* value = std::getenv(name);
    return value ? value : fallback;
}
inline size_t number(const char* name, size_t fallback, size_t maximum) {
    const auto value = environment(name);
    if (value.empty()) return fallback;
    char* end = nullptr;
    const auto result = std::strtoull(value.c_str(), &end, 10);
    check(value[0] != '-' && end && !*end && result <= maximum,
        "Invalid execution-policy integer");
    return static_cast<size_t>(result);
}

struct Policy {
    std::string regime = "continuous";
    size_t chunks = 8, settle_ms = 1000;
    void validate(const std::string& pass) const {
        check(regime == "continuous" || regime == "paced",
            "Execution regime unavailable (Rack engine requires its own harness)");
        check(regime != "paced" || pass == "callback", "Pacing requires callback observations");
        check(chunks >= 2 && chunks <= 4096 && settle_ms > 0 && settle_ms <= 3600000,
            "Execution policy outside bounds");
    }
    static Policy configured(const std::string& pass) {
        check(environment("PAPER_THREAD_POLICY", "inherit") == "inherit" &&
            environment("PAPER_FPU_POLICY", "inherit") == "inherit",
            "Requested thread/FPU policy is unsupported; only inherit is implemented");
        Policy value;
        value.regime = environment("PAPER_EXECUTION_REGIME", "continuous");
        value.chunks = number("PAPER_THROUGHPUT_CHUNKS", 8, 4096);
        value.settle_ms = number("PAPER_PROCESS_SETTLE_MS", 1000, 3600000);
        value.validate(pass);
        return value;
    }
};

inline std::string& output_override() { static std::string path; return path; }
/// @brief Give each shared-process development repetition a distinct sidecar.
class OutputPath {
    std::string previous;
 public:
    explicit OutputPath(const std::string& path) : previous(output_override()) { output_override() = path; }
    ~OutputPath() { output_override() = previous; }
    OutputPath(const OutputPath&) = delete;
    OutputPath& operator=(const OutputPath&) = delete;
};

/// @brief Real monotonic clock, replaceable by a deterministic test clock.
struct SystemClock {
    int64_t now() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            PaperMeasurement::Clock::now().time_since_epoch()).count();
    }
    void sleep_until(int64_t ns) const {
#ifdef PAPER_FIXTURE_CLOCK
        PaperMeasurement::Clock::advance_to(std::chrono::nanoseconds(ns));
#else
        std::this_thread::sleep_until(std::chrono::steady_clock::time_point(
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::nanoseconds(ns))));
#endif
    }
};

/// @brief Development preflight runs in the child, so its session gate does too.
inline std::pair<int64_t, int64_t> session_settle(size_t seconds) {
    check(seconds >= 180 && seconds <= 3600, "Session settling must be 180 to 3600 seconds");
    SystemClock clock;
    const auto start = clock.now();
    const auto target = start+static_cast<int64_t>(seconds)*1000000000;
    clock.sleep_until(target);
    const auto finish = clock.now();
    check(finish >= target && finish <= target+60000000000LL,
        "Session settling exceeded its scheduling allowance");
    return {start, finish};
}

/// @brief Compute absolute releases from sample coordinates, without drift.
inline int64_t offset_ns(size_t samples, double rate) {
    check(std::isfinite(rate) && rate > 0, "Invalid sample rate");
    const long double value = static_cast<long double>(samples)*1000000000.L/rate;
    check(value >= 0 &&
        value < static_cast<long double>(std::numeric_limits<int64_t>::max()/2),
        "Sample-time coordinate overflow");
    return static_cast<int64_t>(std::llround(value));
}

struct Observation {
    size_t sample, samples;
    int64_t release, wake, start, finish, deadline;
};

/// @brief One ordered pass. Overdue callbacks catch up; none are dropped/rebased.
/// @details Conditioning is outside compute duration but inside release-to-finish
/// latency. The caller preallocates observations. Clock/work seams allow tests to
/// exercise timing logic without obtaining any hardware performance measurement.
template<typename Clock, typename Work, typename Condition>
void measure(const Policy& policy, const std::string& pass, size_t callbacks,
        size_t block, double rate, Clock& clock, Work work, Condition condition,
        std::vector<Observation>& rows) {
    policy.validate(pass);
    check(callbacks && block && callbacks <= std::numeric_limits<size_t>::max()/block,
        "Invalid measurement extent");
    const size_t chunks = pass == "throughput" ? std::min(callbacks, policy.chunks) : callbacks;
    check(rows.empty() && rows.capacity() >= chunks, "Observations must be preallocated");
    const int64_t epoch = clock.now();
    check(epoch >= 0 && offset_ns(callbacks*block, rate) <=
        std::numeric_limits<int64_t>::max()-epoch, "Absolute release overflow");
    size_t cursor = 0;
    for (size_t i = 0; i < chunks; ++i) {
        const size_t count = pass == "throughput" ?
            (callbacks/chunks + (i < callbacks%chunks))*block : block;
        const int64_t release = offset_ns(cursor, rate);
        if (policy.regime == "paced") clock.sleep_until(epoch+release);
        const auto wake = clock.now()-epoch;
        condition();
        const auto start = clock.now()-epoch;
        work(count);
        const auto finish = clock.now()-epoch;
        check(wake >= 0 && start >= wake && finish >= start, "Non-monotonic measurement clock");
        rows.push_back({cursor, count, policy.regime == "paced" ? release : -1,
            wake, start, finish, policy.regime == "paced" ? offset_ns(cursor+count, rate) : -1});
        cursor += count;
    }
    check(cursor == callbacks*block, "Lost logical samples");
}

/// @brief Read control state on the measuring thread; never infer CPU placement.
struct ThreadState {
    uint64_t fpu = 0;
    std::string fpu_kind = "unavailable";
    int scheduler = -1, priority = -1, qos = -1;
    static ThreadState read() {
        ThreadState value;
#if defined(__aarch64__)
        asm volatile("mrs %0, fpcr" : "=r"(value.fpu));
        value.fpu_kind = "arm64-fpcr";
#elif defined(__SSE__)
        value.fpu = _mm_getcsr() & ~uint64_t(0x3f); // Exclude sticky exception flags.
        value.fpu_kind = "x86-mxcsr-control";
#endif
#ifndef _WIN32
        sched_param parameter{};
        check(pthread_getschedparam(pthread_self(), &value.scheduler, &parameter) == 0,
            "Cannot read effective thread scheduler");
        value.priority = parameter.sched_priority;
#endif
#ifdef __APPLE__
        qos_class_t qos;
        check(pthread_get_qos_class_np(pthread_self(), &qos, nullptr) == 0,
            "Cannot read effective thread QoS");
        value.qos = static_cast<int>(qos);
#endif
        return value;
    }
    void write(std::ostream& output) const {
        output << "{\"fpu_kind\":\"" << fpu_kind << "\",\"fpu_control\":" << fpu
            << ",\"scheduler\":" << scheduler << ",\"priority\":" << priority
            << ",\"qos\":" << qos << '}';
    }
};

class Session;
inline Session*& active() { static Session* value = nullptr; return value; }

/// @brief Policy and process sidecar for one timed execute(), never a DSP object.
class Session {
    std::string path;
    ThreadState before;
    std::vector<Observation> observations;
    std::vector<int64_t> pre_timer, post_timer;
    bool settled = false;
    bool completed = false;
    int64_t settle_start = 0, settle_finish = 0;
    int sleep_owner = 0;

    void calibration(std::vector<int64_t>& target) {
        SystemClock clock;
        for (size_t i = 0; i < 1024; ++i) {
            const auto start = clock.now();
            asm volatile("" : : : "memory");
            target.push_back(clock.now()-start);
        }
    }
    void protection() const {
#if defined(__APPLE__) && !defined(PAPER_FIXTURE_CLOCK)
        check(sleep_owner > 0 && kill(sleep_owner, 0) == 0,
            "Missing caffeinate guard; use the protected benchmark launcher");
#endif
    }
    void write_observations(std::ostream& output) const {
        output << '[';
        for (size_t i = 0; i < observations.size(); ++i) {
            if (i) output << ',';
            const auto& r = observations[i];
            output << "{\"sample\":" << r.sample << ",\"samples\":" << r.samples
                << ",\"release_ns\":" << r.release << ",\"wake_ns\":" << r.wake << ",\"start_ns\":" << r.start
                << ",\"finish_ns\":" << r.finish << ",\"deadline_ns\":" << r.deadline << '}';
        }
        output << ']';
    }

 public:
    Policy policy;
    explicit Session(const std::string& pass) {
        check(!active(), "Nested execution session");
        policy = Policy::configured(pass);
        path = output_override().empty() ? environment("PAPER_EXECUTION_PATH") : output_override();
        sleep_owner = static_cast<int>(number("PAPER_SLEEP_OWNER", 0, 1u << 30));
        protection();
        before = ThreadState::read();
        pre_timer.reserve(1024); post_timer.reserve(1024);
        active() = this;
    }
    ~Session() {
        if (active() == this) active() = nullptr;
        // Preserve completed intervals on failed replay/guard/processing paths.
        // This is outside sample work; never replace the original exception.
        if (!completed && !path.empty()) {
            try {
                std::ofstream output(path);
                output << "{\"schema\":1,\"status\":\"failed\",\"regime\":\"" << policy.regime << '"';
#ifdef PAPER_FIXTURE_CLOCK
                output << ",\"fixture\":true";
#endif
                output << ",\"observations\":";
                write_observations(output);
                output << "}\n";
            } catch (...) {} // A full disk may also prevent this best-effort diagnostic.
        }
    }
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    /// @brief After plan creation/preparation, before declared warmup/calibration.
    void settle() {
        check(!settled, "Repeated per-process settling");
        protection();
        SystemClock clock;
        settle_start = clock.now();
        clock.sleep_until(settle_start+static_cast<int64_t>(policy.settle_ms)*1000000);
        settle_finish = clock.now();
        const auto duration = settle_finish-settle_start;
        check(duration >= static_cast<int64_t>(policy.settle_ms)*1000000 &&
            duration <= static_cast<int64_t>(policy.settle_ms)*1000000+60000000000LL,
            "Process settling exceeded its scheduling allowance");
        settled = true;
        calibration(pre_timer);
    }
    std::vector<Observation>& reserve(size_t count) {
        observations.reserve(count);
        return observations;
    }
    /// @brief Post-calibration precedes correctness replay; no probes in callbacks.
    void measured() { calibration(post_timer); protection(); }
    void finish() {
        check(settled && pre_timer.size() == 1024 && post_timer.size() == 1024,
            "Execution did not complete its declared measurement boundaries");
        protection();
        const auto after = ThreadState::read();
        check(before.fpu == after.fpu && before.fpu_kind == after.fpu_kind,
            "FPU control changed during execute");
        if (path.empty()) { completed = true; return; }
        std::ofstream output(path);
        check(bool(output), "Cannot create execution sidecar");
        output << "{\"schema\":1,\"status\":\"complete\",\"regime\":\"" << policy.regime
            << "\",\"thread_policy\":\"inherit\",\"fpu_policy\":\"inherit\","
            << "\"thread_scope\":\"calling-thread; opaque provider workers unavailable\",\"threads\":[{";
        output << "\"before\":"; before.write(output);
        output << ",\"after\":"; after.write(output);
        int64_t resolution_ns = 0;
#ifndef _WIN32
        timespec resolution{};
        check(clock_getres(CLOCK_MONOTONIC, &resolution) == 0, "Cannot read clock resolution");
        resolution_ns = resolution.tv_sec*1000000000LL+resolution.tv_nsec;
#else
        resolution_ns = std::max<int64_t>(1, 1000000000LL*
            std::chrono::steady_clock::period::num/std::chrono::steady_clock::period::den);
#endif
        output << "}],\"clock_resolution_ns\":" << resolution_ns
            << ",\"sleep_owner\":" << sleep_owner
            << ",\"settle_start_ns\":" << settle_start << ",\"settle_finish_ns\":" << settle_finish
            << ",\"process_settle_ms\":" << policy.settle_ms << ",\"throughput_chunks\":" << policy.chunks;
#ifdef PAPER_FIXTURE_CLOCK
        output << ",\"fixture\":true";
#endif
        for (const auto& group : {std::make_pair("pre_timer_ns", &pre_timer),
                                  std::make_pair("post_timer_ns", &post_timer)}) {
            output << ",\"" << group.first << "\":[";
            for (size_t i = 0; i < group.second->size(); ++i) {
                if (i) output << ',';
                output << (*group.second)[i];
            }
            output << ']';
        }
        output << ",\"observations\":";
        write_observations(output);
        output << "}\n";
        output.close();
        check(bool(output), "Cannot write execution sidecar");
        completed = true;
    }
};

inline void settle() { if (active()) active()->settle(); }
inline void measured() { if (active()) active()->measured(); }
}  // namespace Execution
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_EXECUTION_HPP_

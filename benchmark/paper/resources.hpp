// Separate setup/storage audit; never instrument the publication timing binary.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_RESOURCES_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_RESOURCES_HPP_
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>

namespace PaperResources {
struct Counts { size_t allocations = 0, allocated = 0, live = 0, peak = 0; };
static Counts counts;
static bool active = false;
#ifdef PAPER_ALLOCATION_AUDIT
struct Entry { void* pointer = nullptr; size_t bytes = 0; bool used = false; };
static Entry entries[131072];
inline size_t slot(void* pointer) { return (reinterpret_cast<uintptr_t>(pointer)>>4)%131072; }
inline void record(void* pointer, size_t bytes) {
    if (!active) return;
    size_t index = slot(pointer);
    for (size_t i = 0; i < 131072; ++i, index = (index+1)%131072) {
        auto& item = entries[index];
        if (!item.pointer) {
            item.pointer = pointer; item.bytes = bytes; item.used = true;
            ++counts.allocations; counts.allocated += bytes; counts.live += bytes;
            if (counts.live > counts.peak) counts.peak = counts.live;
            return;
        }
    }
    std::abort(); // Fail rather than silently truncate an allocation audit.
}
inline void release(void* pointer) {
    if (!pointer) return;
    size_t index = slot(pointer);
    for (size_t i = 0; i < 131072; ++i, index = (index+1)%131072) {
        auto& item = entries[index];
        if (!item.used) return;
        if (item.pointer == pointer) { counts.live -= item.bytes; item.pointer = nullptr; return; }
    }
}
#endif
inline void reset_phase() { counts.allocations = counts.allocated = 0; counts.peak = counts.live; }
inline void number(size_t value) {
#ifdef PAPER_ALLOCATION_AUDIT
    std::cout << value;
#else
    (void)value;
    std::cout << "null";
#endif
}
inline void phase(double ns, const Counts& snapshot) {
    std::cout << "{\"ns\":" << ns << ",\"allocations\":"; number(snapshot.allocations);
    std::cout << ",\"allocated_bytes\":"; number(snapshot.allocated);
    std::cout << ",\"live_bytes\":"; number(snapshot.live);
    std::cout << ",\"peak_bytes\":"; number(snapshot.peak);
    std::cout << '}';
}

/// @brief Factory owns one object; run includes buffering and required output work.
template<typename Object, typename Factory, typename Run>
void inspect(Factory factory, Run run, size_t operations) {
    using Clock = std::chrono::steady_clock;
    auto elapsed = [](Clock::time_point a, Clock::time_point b) {
        return std::chrono::duration<double, std::nano>(b-a).count();
    };
    reset_phase(); active = true;
    const auto start = Clock::now();
    Object* object = factory();
    const auto prepared = Clock::now();
    const auto setup = counts;
    reset_phase();
    run(*object);
    const auto executed = Clock::now();
    const auto execution = counts;
    reset_phase();
    delete object;
    const auto destroyed = Clock::now();
    const auto teardown = counts;
    active = false;
    std::cout.precision(17);
    std::cout << "{\"schema\":1,\"instrumented\":";
#ifdef PAPER_ALLOCATION_AUDIT
    std::cout << "true";
#else
    std::cout << "false";
#endif
    std::cout << ",\"allocation_scope\":\"C++ new/delete requested bytes only\","
        << "\"native_allocation_bytes\":null,\"stack_scratch_bytes\":null,"
        << "\"unknown_reason\":\"Native allocators, stack and allocator overhead are not intercepted\","
        << "\"object_bytes\":" << sizeof(Object) << ",\"operations\":" << operations << ",\"setup\":";
    phase(elapsed(start, prepared), setup);
    std::cout << ",\"execution\":"; phase(elapsed(prepared, executed), execution);
    std::cout << ",\"destruction\":"; phase(elapsed(executed, destroyed), teardown);
    std::cout << ",\"heap_scratch_peak_over_setup_bytes\":";
    number(execution.peak > setup.live ? execution.peak-setup.live : 0);
    std::cout << "}\n";
}
}  // namespace PaperResources

#ifdef PAPER_ALLOCATION_AUDIT
// The instrumented executable is single-threaded and separate from timed runs.
// Returning malloc's original pointer preserves compatibility with native frees.
void* operator new(std::size_t bytes) {
    if (void* pointer = std::malloc(bytes ? bytes : 1)) { PaperResources::record(pointer, bytes); return pointer; }
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* pointer) noexcept { PaperResources::release(pointer); std::free(pointer); }
void operator delete[](void* pointer) noexcept { ::operator delete(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { ::operator delete(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { ::operator delete(pointer); }
#endif
#endif

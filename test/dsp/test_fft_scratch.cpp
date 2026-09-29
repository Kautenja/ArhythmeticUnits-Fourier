// Allocation and reuse regressions for real FFT scratch storage.
//
// Copyright (c) 2020 Christian Kauten
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <cstdlib>
#include <new>
#include <vector>
#include "dsp/fft.hpp"
#include "catch_amalgamated.hpp"

namespace {
/// Count allocations only during DSP operations, excluding Catch2 assertions.
bool count_allocations = false;
std::size_t allocation_count = 0;

/// Restore allocation tracking even if a DSP operation throws.
struct AllocationScope {
    AllocationScope() {
        allocation_count = 0;
        count_allocations = true;
    }
    ~AllocationScope() { count_allocations = false; }
};
}  // namespace

// This standalone executable intercepts the allocation forms used by
// std::vector without changing the allocator in production DSP types.
void* operator new(std::size_t size) {
    if (count_allocations) ++allocation_count;
    if (void* memory = std::malloc(size ? size : 1)) return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
// C++14 permits sized deallocation; keep it paired with the malloc-backed new.
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

TEST_CASE("RFFT reuses scratch storage after construction and resizing", "[rfft]") {
    Fourier::OnTheFlyRFFT<float> fft(128);
    // Exercise initial storage, growth, then shrinkage at supported module sizes.
    for (const std::size_t n : {128u, 16384u, 256u}) {
        if (fft.size() != n) fft.resize(n);
        std::vector<float> input(n, 0.f);
        std::vector<float> window(n, 0.5f);
        for (const float amplitude : {2.f, 0.f, 4.f}) {
            input[0] = amplitude;
            {
                AllocationScope scope;
                fft.buffer(input.data(), window);
                while (!fft.is_done_computing()) fft.step(256);
            }
            CHECK(allocation_count == 0);
            // A windowed impulse has the same real coefficient in every bin;
            // silence between impulses catches stale packed samples.
            for (const auto& coefficient : fft.coefficients) {
                REQUIRE(coefficient.real() == Catch::Approx(amplitude * 0.5f));
                REQUIRE(coefficient.imag() == Catch::Approx(0.f).margin(1e-6f));
            }
            {
                AllocationScope scope;
                fft.smooth(48000.f, 1.f / 3.f);
            }
            CHECK(allocation_count == 0);
            for (std::size_t k = 0; k < n; ++k) {
                const float expected = k <= n / 2 ? amplitude * 0.5f : 0.f;
                REQUIRE(fft.coefficients[k].real() == Catch::Approx(expected));
                REQUIRE(fft.coefficients[k].imag() == 0.f);
            }
        }
    }
}

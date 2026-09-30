// Rack/PFFFT canonical adapters, used only by the research benchmark.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_PROVIDERS_PFFFT_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_PROVIDERS_PFFFT_HPP_

#include <complex>
#include <cstddef>
#include <memory>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <dsp/fft.hpp>

namespace Paper {

/// @brief Float ordered Rack FFTs with explicitly charged packing and scaling.
/// @details This adapter supports powers of two from 128 to 16384. Real
/// transforms use Rack's packed DC/Nyquist layout internally; callers receive
/// natural-order complex bins. Complex inverse transforms include 1/N scaling.
/// Plans and aligned transfer buffers are allocated only during construction.
/// Rack passes null scratch to PFFFT, which uses transform-sized stack scratch.
class PffftBackend {
    struct AlignedDelete {
        void operator()(float* pointer) const { pffft_aligned_free(pointer); }
    };
    using Buffer = std::unique_ptr<float, AlignedDelete>;
    const size_t n;
    const bool real;
    const size_t buffer_floats;
    std::unique_ptr<rack::dsp::RealFFT> real_plan;
    std::unique_ptr<rack::dsp::ComplexFFT> complex_plan;
    Buffer input;
    Buffer output;

    static size_t valid_size(size_t size) {
        if (size < 128 || size > 16384 || (size & (size-1)))
            throw std::invalid_argument("PFFFT benchmark requires power-of-two N in [128,16384]");
        return size;
    }

    static bool real_operation(const std::string& operation) {
        if (operation == "rfft" || operation == "analysis") return true;
        if (operation == "fft" || operation == "ifft" || operation == "inverse"
                || operation == "identity" || operation == "fir") return false;
        throw std::invalid_argument("Unsupported PFFFT operation");
    }

    static Buffer allocate(size_t count) {
        Buffer buffer(static_cast<float*>(pffft_aligned_malloc(count*sizeof(float))));
        if (!buffer) throw std::bad_alloc();
        return buffer;
    }

    void complex(const std::complex<float>* source, std::complex<float>* destination,
            bool inverse) {
        if (!complex_plan) throw std::logic_error("PFFFT complex plan not prepared");
        for (size_t i = 0; i < n; ++i) {
            input.get()[2*i] = source[i].real();
            input.get()[2*i+1] = source[i].imag();
        }
        if (inverse) complex_plan->ifft(input.get(), output.get());
        else complex_plan->fft(input.get(), output.get());
        const float scale = inverse ? 1.f/static_cast<float>(n) : 1.f;
        for (size_t i = 0; i < n; ++i)
            destination[i] = {output.get()[2*i]*scale, output.get()[2*i+1]*scale};
    }

 public:
    PffftBackend(size_t size, const std::string& operation) :
            n(valid_size(size)), real(real_operation(operation)),
            buffer_floats(real ? n : 2*n), input(allocate(buffer_floats)),
            output(allocate(buffer_floats)) {
        if (real) real_plan.reset(new rack::dsp::RealFFT(n));
        else complex_plan.reset(new rack::dsp::ComplexFFT(n));
    }

    /// @brief Write K=N/2+1 nonnegative-frequency bins, including DC and Nyquist.
    void forward_real_positive(const float* source, std::complex<float>* destination) {
        if (!real_plan) throw std::logic_error("PFFFT real plan not prepared");
        for (size_t i = 0; i < n; ++i) input.get()[i] = source[i];
        real_plan->rfft(input.get(), output.get());
        destination[0] = {output.get()[0], 0};
        destination[n/2] = {output.get()[1], 0};
        for (size_t k = 1; k < n/2; ++k)
            destination[k] = {output.get()[2*k], output.get()[2*k+1]};
    }

    /// @brief Write all N bins; Hermitian reconstruction is included in timing.
    void forward_real(const float* source, std::complex<float>* destination) {
        forward_real_positive(source, destination);
        for (size_t k = n/2+1; k < n; ++k) destination[k] = std::conj(destination[n-k]);
    }

    void forward_complex(const std::complex<float>* source, std::complex<float>* destination) {
        complex(source, destination, false);
    }

    void inverse_complex(const std::complex<float>* source, std::complex<float>* destination) {
        complex(source, destination, true);
    }

    /// @brief Source-derived payload accounting, separate from measured C++ heap audits.
    /// @details Opaque setup/allocator bytes and compiler stack overhead remain
    /// unknown. Source and linked-library identities are retained by the runner.
    std::string info_json() const {
        std::ostringstream out;
        out << "{\"provider\":\"rack-pffft\",\"api\":\"rack::dsp::"
            << (real ? "RealFFT::rfft" : "ComplexFFT::fft/ifft")
            << "\",\"plan_policy\":\"one exact-size reusable ordered plan\""
            << ",\"precision\":\"float\",\"simd_lanes\":" << pffft_simd_size()
            << ",\"native_layout\":\"" << (real ? "DC,Nyquist,Re1,Im1,..." : "interleaved-complex")
            << "\",\"inverse_normalization\":\"adapter 1/N\""
            << ",\"aligned_io_payload_bytes\":" << 2*buffer_floats*sizeof(float)
            << ",\"native_plan_bytes\":null,\"native_heap_scratch_bytes\":0"
            << ",\"source_derived_twiddle_payload_bytes\":" << buffer_floats*sizeof(float)
            << ",\"source_derived_stack_scratch_payload_bytes\":" << buffer_floats*sizeof(float)
            << ",\"native_execution_allocation_policy\":\"stack scratch; no heap allocation in inspected PFFFT transform source\""
            << ",\"unknown_reason\":\"opaque setup size, allocator overhead and total compiler stack usage; source formulas require matching linked implementation\"}";
        return out.str();
    }
};

}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_PROVIDERS_PFFFT_HPP_

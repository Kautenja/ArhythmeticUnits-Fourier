// Analytical amplitude checks through the actual Fourier and Spectre modules.
//
// Copyright 2026 Arhythmetic Units
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include "../../src/SpectrumAnalyzer.cpp"
#include "../../src/Spectrogram.cpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

Plugin* plugin_instance = nullptr;

namespace {

/// Cable voices and their independently specified algebraic sum.
struct Cable {
    std::vector<float> voices;
    float sum;
    float gain;
};

const std::array<Cable, 6> cables{{
    {{1.f}, 1.f, 0.5f},
    {{1.f, -0.5f, 0.25f}, 0.75f, 1.25f},
    {std::vector<float>(16, 1.f), 16.f, 2.f},
    {{}, 0.f, 3.f},
    {{1.f, -1.f}, 0.f, 1.f},
    {{1.f}, 1.f, 0.f}
}};

/// Steady-state |H(exp(jw))| for the modules' 10 Hz DC blockers.
/// p = 1 - 20/fs; H(z) = (1+p)/2 * (1-z^-1)/(1-p*z^-1).
/// Use double precision and no production filter/normalization helpers.
double ac_response(size_t bin, size_t length, double sample_rate) {
    const double p = 1.0 - 20.0 / sample_rate;
    const double sine = std::sin(std::acos(-1.0) * bin / length);
    return (1.0 + p) * sine /
        std::sqrt((1.0-p)*(1.0-p) + 4.0*p*sine*sine);
}

/// Sum of DC, one bin-centered cosine, and an alternating Nyquist signal,
/// in volts. Distinct components on each port expose SIMD lane mixups.
double voltage(size_t sample, size_t length, size_t lane) {
    return 0.25 * (lane+1) + (1.0 + 0.25*lane) *
        std::cos(2.0 * std::acos(-1.0) * (lane+1) * sample / length) +
        (0.5 + 0.125*lane) * (sample % 2 ? -1.0 : 1.0);
}

/// Boxcar DFT magnitudes: N*A for DC/Nyquist, N*A/2 for an interior
/// cosine. All input voltages (including DC) are divided by 5 V before
/// the port gain. No one-sided doubling is applied to the published FFT.
double expected_magnitude(size_t bin, size_t length, size_t lane,
        const Cable& cable, bool ac_coupled, double sample_rate) {
    double amplitude = 0.0;
    if (bin == 0) amplitude = 0.25 * (lane+1);
    if (bin == lane+1) amplitude = (1.0 + 0.25*lane) / 2.0;
    if (bin == length/2) amplitude = 0.5 + 0.125*lane;
    return length * amplitude * std::abs(cable.sum) * cable.gain / 5.0 *
        (ac_coupled ? ac_response(bin, length, sample_rate) : 1.0);
}

/// Absolute error in FFT units, relative to the input's peak bound. The
/// sixteen-voice fixture exceeds nominal voltage levels, so a fixed absolute
/// floor would overconstrain float filter roundoff for that fixture. Allow
/// 20 ppm of the peak bound (about -94 dB), plus relative error at signal bins.
double magnitude_error(size_t length, size_t lane, const Cable& cable) {
    double voice_sum = 0.0;
    for (const float voice : cable.voices) voice_sum += std::abs(voice);
    const double peak = voice_sum * cable.gain / 5.0 *
        (0.25*(lane+1) + 1.0+0.25*lane + 0.5+0.125*lane);
    return 2e-5 * length * std::max(1.0, peak);
}

/// Check Fourier's published linear display ordinates, including its
/// historical K=N/2+1 and +12 dB display normalization, independently.
void check_spectrum(SpectrumAnalyzer& module, size_t length, size_t rotation) {
    const auto& spectrum = module.consume_display_spectrum();
    REQUIRE(spectrum.count == length/2+1);
    const double display_scale = (length/2+1) * std::pow(10.0, 12.0/20.0);
    for (size_t lane = 0; lane < SpectrumAnalyzer::NUM_INPUTS; ++lane) {
        const auto& cable = cables[(rotation+lane) % cables.size()];
        for (size_t bin = 0; bin < spectrum.count; ++bin) {
            CAPTURE(lane, bin);
            const double expected = expected_magnitude(bin, length, lane,
                cable, module.is_ac_coupled, module.get_sample_rate()) / display_scale;
            // Bound float FFT/filter error per input sample, then convert
            // that absolute bound to display units. Relative tolerance covers
            // the rounded filter pole, most visible near its transition band.
            CHECK(spectrum.points[lane][bin].y == Approx(expected)
                .epsilon(2e-4).margin(magnitude_error(length, lane, cable) / display_scale));
        }
    }
}

/// Spectre publishes raw magnitudes, before UI color/slope transformations.
void check_spectrum(Spectrogram& module, size_t length, size_t rotation) {
    const size_t index = (module.get_hop_index() + Spectrogram::N_STFT - 1) %
        Spectrogram::N_STFT;
    const auto* column = module.consume_display_column(index);
    REQUIRE(column != nullptr);
    REQUIRE(column->revision > 0);
    REQUIRE(column->values.size() == length/2+1);
    for (size_t bin = 0; bin < column->values.size(); ++bin) {
        CAPTURE(bin);
        const double expected = expected_magnitude(bin, length, 0,
            cables[rotation], module.is_ac_coupled, module.get_sample_rate());
        CHECK(column->values[bin] == Approx(expected)
            .epsilon(2e-4).margin(magnitude_error(length, 0, cables[rotation])));
    }
}

/// Drive real Rack ports and process(), then inspect only published output.
/// Every cable/gain combination visits every Fourier lane and Spectre's port.
template<typename Module>
void check_amplitudes(Module& module, size_t length) {
    module.set_window_function(Fourier::Window::Function::Boxcar);
    module.set_time_smoothing(0.f);
    module.set_frequency_smoothing(FrequencySmoothing::None);
    rack::engine::Module::ProcessArgs args = {};
    args.sampleRate = module.get_sample_rate();
    args.sampleTime = 1.f / args.sampleRate;
    // Precompute periodic voltages to avoid phase drift in long settling runs.
    std::array<std::vector<float>, Module::NUM_INPUTS> signals;
    for (size_t lane = 0; lane < signals.size(); ++lane) {
        signals[lane].resize(length);
        for (size_t sample = 0; sample < length; ++sample)
            signals[lane][sample] = voltage(sample, length, lane);
    }
    for (size_t rotation = 0; rotation < cables.size(); ++rotation) {
        for (size_t lane = 0; lane < signals.size(); ++lane) {
            const auto& cable = cables[(rotation+lane) % cables.size()];
            // Rack normally sets channels on connection. setChannels() alone
            // intentionally cannot connect a previously disconnected input.
            module.inputs[Module::INPUT_SIGNAL+lane].channels = cable.voices.size();
            module.params[Module::PARAM_INPUT_GAIN+lane].setValue(cable.gain);
        }
        // Also check returning to bypass after the filter has processed audio.
        for (const bool ac_coupled : {false, true, false}) {
            CAPTURE(length, args.sampleRate, rotation, ac_coupled);
            module.is_ac_coupled = ac_coupled;
            const size_t hop = module.get_hop_length();
            // 24 time constants suppress the DC blocker's startup/disconnect
            // tail below float precision. Then fill an entire unpadded window
            // and allow two hops for frame latching and publication.
            const size_t settling = ac_coupled ? std::ceil(24.0*args.sampleRate/20.0) : 0;
            const size_t samples = settling + length + 2*hop;
            for (size_t sample = 0; sample < samples; ++sample) {
                for (size_t lane = 0; lane < signals.size(); ++lane) {
                    const auto& cable = cables[(rotation+lane) % cables.size()];
                    auto& input = module.inputs[Module::INPUT_SIGNAL+lane];
                    // Nonzero stale storage on disconnected ports must not
                    // leak through as a mono input.
                    input.setVoltage(7.f);
                    for (size_t voice = 0; voice < cable.voices.size(); ++voice)
                        input.setVoltage(cable.voices[voice] *
                            signals[lane][sample % length], voice);
                }
                module.process(args);
            }
            check_spectrum(module, length, rotation);
        }
    }
}

}  // namespace

TEST_CASE("Fourier publishes analytical absolute amplitudes through Rack ports") {
    rack::Context context;
    rack::contextSet(&context);
    context.engine = new rack::engine::Engine;
    for (const float rate : {44100.f, 96000.f}) {
        context.engine->setSampleRate(rate);
        for (const size_t length : {128u, 2048u, 16384u}) {
            SpectrumAnalyzer module;
            module.set_window_length(length);
            module.set_hop_length(512);
            module.set_slope(0.f);
            module.set_frequency_scale(FrequencyScale::Linear);
            module.set_magnitude_scale(MagnitudeScale::Linear);
            check_amplitudes(module, length);
        }
    }
}

TEST_CASE("Spectre publishes analytical absolute amplitudes through Rack ports") {
    rack::Context context;
    rack::contextSet(&context);
    context.engine = new rack::engine::Engine;
    for (const float rate : {44100.f, 96000.f}) {
        context.engine->setSampleRate(rate);
        Spectrogram module;
        check_amplitudes(module, Spectrogram::N_FFT);
    }
}

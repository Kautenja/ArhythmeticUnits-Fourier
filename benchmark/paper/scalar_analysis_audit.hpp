// Complete per-instance numerical auditing of scalar analysis replay.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ARHYTHMETIC_UNITS_FOURIER_PAPER_SCALAR_ANALYSIS_AUDIT_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_PAPER_SCALAR_ANALYSIS_AUDIT_HPP_
#include <limits>
#include <map>
#include <sstream>
#include <vector>
#include <memory>
#include "fourier.hpp"
#include "synthesis.hpp"
#include "analysis_reference.hpp"

namespace Paper {
/// @brief Per-instance replay identity; all counters exclude measured setup/warmup.
struct ScalarCoverage {
    size_t instance = 0, first_sample = 0, samples = 0;
    size_t expected_spectra = 0, checked_spectra = 0, checked_bins = 0;
    size_t first_endpoint = 0, last_endpoint = 0;
};

/// @brief Check the expected cadence even on samples with no reported publication.
/// @details Lazy references reconstruct warmup history from the exact common
/// input bytes, while only measured-window publications contribute coverage.
/// There is no call to this object in timed processing or resource probes.
template<typename T>
struct ScalarAnalysisAudit {
    Config config;
    SynthesisAccuracy& accuracy;
    std::vector<ScalarCoverage>& coverage;
    std::map<const void*, size_t> identities;
    std::vector<std::unique_ptr<AnalysisReference<T>>> references;
    ScalarAnalysisAudit(const Config& c, SynthesisAccuracy& a, std::vector<ScalarCoverage>& v)
        : config(c), accuracy(a), coverage(v) {}
    ScalarAnalysisAudit(ScalarAnalysisAudit&&) = default;
    template<typename Adapter>
    void operator()(const Adapter& adapter, const std::vector<float>& input, size_t sample) {
        auto found = identities.find(&adapter);
        if (found == identities.end()) {
            const size_t id = coverage.size();
            identities[&adapter] = id;
            ScalarCoverage item; item.instance = id; item.first_sample = sample;
            coverage.push_back(item);
            references.emplace_back(new AnalysisReference<T>(config));
            found = identities.find(&adapter);
        }
        auto& item = coverage[found->second];
        require(sample == item.first_sample+item.samples, "Duplicate or missing scalar replay sample");
        ++item.samples;
        const size_t delay = backend_contract(config).delay;
        require(adapter.delay() == delay, "Scalar audit delay mismatch");
        const bool expected_publication = sample%config.hop == delay;
        item.expected_spectra += expected_publication;
        require(adapter.published() == expected_publication, "Missing, duplicate or off-phase scalar publication");
        if (!expected_publication) return;
        const size_t endpoint = sample-delay;
        auto& reference = *references[found->second];
        reference.advance(input, endpoint);
        require(adapter.output.size() == reference.expected.size(), "Truncated scalar spectrum");
        accuracy.analysis.compare(reference.expected, [&](size_t k) { return adapter.output[k]; }, endpoint);
        if (!item.checked_spectra) item.first_endpoint = endpoint;
        item.last_endpoint = endpoint;
        ++item.checked_spectra;
        item.checked_bins += reference.expected.size();
        ++accuracy.publications;
        accuracy.checked += reference.expected.size();
        accuracy.maximum_error = accuracy.analysis.maximum_error;
        accuracy.maximum_reference = accuracy.analysis.maximum_reference;
    }
};

/// @brief Versioned coverage alongside the unchanged spectrum-norms-v1 policy.
inline std::string scalar_coverage_json(const Config& c, const SynthesisAccuracy& accuracy,
        const std::vector<ScalarCoverage>& coverage) {
    require(coverage.size() == c.count, "Missing scalar audit instance");
    size_t expected = 0;
    std::ostringstream instances;
    for (size_t i = 0; i < coverage.size(); ++i) {
        const auto& item = coverage[i];
        require(item.samples == c.callbacks*c.block && item.expected_spectra == item.checked_spectra
            && item.checked_bins == item.checked_spectra*(c.n/2+1), "Incomplete scalar audit coverage");
        expected += item.expected_spectra;
        if (i) instances << ',';
        instances << "{\"instance\":" << item.instance << ",\"first_sample\":" << item.first_sample
            << ",\"samples\":" << item.samples << ",\"expected_spectra\":" << item.expected_spectra
            << ",\"checked_spectra\":" << item.checked_spectra << ",\"checked_bins\":" << item.checked_bins
            << ",\"first_endpoint\":" << item.first_endpoint << ",\"last_endpoint\":" << item.last_endpoint << '}';
    }
    std::ostringstream out;
    out << "{\"policy\":\"all-publications-v1\",\"status\":\"full\","
        << "\"reference_precision\":\"binary64 FFT / long-double direct DFT\","
        << "\"reference_mantissa_bits\":" << std::numeric_limits<double>::digits << ','
        << "\"direct_mantissa_bits\":" << std::numeric_limits<long double>::digits << ','
        << "\"interval_arithmetic\":\"binary32\",\"expected_spectra\":" << expected
        << ",\"checked_spectra\":" << accuracy.analysis.vectors
        << ",\"expected_bins\":" << expected*(c.n/2+1) << ",\"checked_bins\":" << accuracy.analysis.values
        << ",\"instances\":[" << instances.str() << "]}";
    return out.str();
}

template<typename T, typename Adapter>
void scalar_analysis_stream(const Config& c) {
    SynthesisAccuracy accuracy;
    std::vector<ScalarCoverage> coverage;
    stream<Adapter>(c, ScalarAnalysisAudit<T>(c, accuracy, coverage));
    if (c.resources) return;
    require(accuracy.checked && accuracy.publications, "Missing scalar numerical audit");
    std::cerr.precision(17);
    std::cerr << "{\"reference\":\"independent recursive binary64 FFT / direct DFT; matched float windows; direct bands and EMA\","
        << "\"max_abs_error\":" << accuracy.maximum_error << ",\"max_reference\":" << accuracy.maximum_reference
        << ",\"checked_samples\":" << accuracy.checked << ",\"publications\":" << accuracy.publications
        << ",\"playback_checked_samples\":0,\"provider_instances\":[],\"analysis\":" << accuracy.analysis.json()
        << ",\"audit\":" << scalar_coverage_json(c, accuracy, coverage) << "}\n";
}
/// @brief Quick preflight for the independent oracle and scalar replay contract.
inline void verify_scalar_analysis() {
    std::vector<std::complex<double>> input(128);
    for (size_t i = 0; i < input.size(); ++i)
        input[i] = {std::sin(i*0.73), std::cos(i*0.37)};
    const auto fft = Reference::independent_fft(input);
    const std::vector<Reference::Complex> precise(input.begin(), input.end());
    for (size_t k = 0; k < input.size(); ++k)
        require(std::abs(Reference::Complex(fft[k])-Reference::coefficient(precise, k, false)) < 1e-10,
            "Independent scalable oracle differs from direct DFT");
    Config c{}; c.backend = "core-float"; c.n = 128; c.hop = 37;
    c.rate = 48000; c.smooth = true; c.state = "live";
    Core<float> adapter(c);
    SynthesisAccuracy accuracy; std::vector<ScalarCoverage> coverage;
    ScalarAnalysisAudit<float> audit(c, accuracy, coverage);
    const auto values = signal();
    for (size_t sample = 0; sample < 8*c.hop; ++sample) {
        adapter.process(values[sample%values.size()]);
        audit(adapter, values, sample);
    }
    require(accuracy.analysis.vectors == 8 && accuracy.checked == 8*(c.n/2+1),
        "Incomplete scalar preflight audit");
}
}  // namespace Paper
#endif  // ARHYTHMETIC_UNITS_FOURIER_PAPER_SCALAR_ANALYSIS_AUDIT_HPP_

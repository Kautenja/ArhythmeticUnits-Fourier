// Benchmark code for the Filter::IIR namespace.
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

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <random>
#include "catch.hpp"
#include "dsp/filter/iir.hpp"

// ---------------------------------------------------------------------------
// MARK: first-order filters
// ---------------------------------------------------------------------------

TEST_CASE("process single precision floating point samples with first-order designs") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<float>(0.0001, 0.1);
    // test the performance of sampling
    BENCHMARK("baseline") {
        distribution(generator);
    };
    auto df1 = Filter::IIR::Bilinear::DirectForm1<float>();
    BENCHMARK("Bilinear::DirectForm1") {
        df1.process(distribution(generator));
    };
    auto df1T = Filter::IIR::Bilinear::DirectForm1Transpose<float>();
    BENCHMARK("Bilinear::DirectForm1Transpose") {
        df1T.process(distribution(generator));
    };
    auto df2 = Filter::IIR::Bilinear::DirectForm2<float>();
    BENCHMARK("Bilinear::DirectForm2") {
        df2.process(distribution(generator));
    };
    auto df2T = Filter::IIR::Bilinear::DirectForm2Transpose<float>();
    BENCHMARK("Bilinear::DirectForm2Transpose") {
        df2T.process(distribution(generator));
    };
    auto df1N = Filter::IIR::Bipolynomial::DirectForm1<float>(1);
    BENCHMARK("Bipolynomial::DirectForm1") {
        df1N.process(distribution(generator));
    };
    auto df2N = Filter::IIR::Bipolynomial::DirectForm2<float>(1);
    BENCHMARK("Bipolynomial::DirectForm2") {
        df2N.process(distribution(generator));
    };
}

TEST_CASE("process double precision floating point samples with first-order designs") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<double>(0.0001, 0.1);
    // test the performance of sampling
    BENCHMARK("baseline") {
        distribution(generator);
    };
    auto df1 = Filter::IIR::Bilinear::DirectForm1<double>();
    BENCHMARK("Bilinear::DirectForm1") {
        df1.process(distribution(generator));
    };
    auto df1T = Filter::IIR::Bilinear::DirectForm1Transpose<double>();
    BENCHMARK("Bilinear::DirectForm1Transpose") {
        df1T.process(distribution(generator));
    };
    auto df2 = Filter::IIR::Bilinear::DirectForm2<double>();
    BENCHMARK("Bilinear::DirectForm2") {
        df2.process(distribution(generator));
    };
    auto df2T = Filter::IIR::Bilinear::DirectForm2Transpose<double>();
    BENCHMARK("Bilinear::DirectForm2Transpose") {
        df2T.process(distribution(generator));
    };
    auto df1N = Filter::IIR::Bipolynomial::DirectForm1<double>(1);
    BENCHMARK("Bipolynomial::DirectForm1") {
        df1N.process(distribution(generator));
    };
    auto df2N = Filter::IIR::Bipolynomial::DirectForm2<double>(1);
    BENCHMARK("Bipolynomial::DirectForm2") {
        df2N.process(distribution(generator));
    };
}

// ---------------------------------------------------------------------------
// MARK: second-order filters
// ---------------------------------------------------------------------------

TEST_CASE("process single precision floating point samples with second-order designs") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<float>(0.0001, 0.1);
    // test the performance of sampling
    BENCHMARK("baseline") {
        distribution(generator);
    };
    auto df1 = Filter::IIR::Biquadratic::DirectForm1<float>();
    BENCHMARK("Biquadratic::DirectForm1") {
        df1.process(distribution(generator));
    };
    auto df1T = Filter::IIR::Biquadratic::DirectForm1Transpose<float>();
    BENCHMARK("Biquadratic::DirectForm1Transpose") {
        df1T.process(distribution(generator));
    };
    auto df2 = Filter::IIR::Biquadratic::DirectForm2<float>();
    BENCHMARK("Biquadratic::DirectForm2") {
        df2.process(distribution(generator));
    };
    auto df2T = Filter::IIR::Biquadratic::DirectForm2Transpose<float>();
    BENCHMARK("Biquadratic::DirectForm2Transpose") {
        df2T.process(distribution(generator));
    };
    auto df1N = Filter::IIR::Bipolynomial::DirectForm1<float>(2);
    BENCHMARK("Bipolynomial::DirectForm1") {
        df1N.process(distribution(generator));
    };
    auto df2N = Filter::IIR::Bipolynomial::DirectForm2<float>(2);
    BENCHMARK("Bipolynomial::DirectForm2") {
        df2N.process(distribution(generator));
    };
}

TEST_CASE("process double precision floating point samples with second-order designs") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<double>(0.0001, 0.1);
    // test the performance of sampling
    BENCHMARK("baseline") {
        distribution(generator);
    };
    auto df1 = Filter::IIR::Biquadratic::DirectForm1<double>();
    BENCHMARK("Biquadratic::DirectForm1") {
        df1.process(distribution(generator));
    };
    auto df1T = Filter::IIR::Biquadratic::DirectForm1Transpose<double>();
    BENCHMARK("Biquadratic::DirectForm1Transpose") {
        df1T.process(distribution(generator));
    };
    auto df2 = Filter::IIR::Biquadratic::DirectForm2<double>();
    BENCHMARK("Biquadratic::DirectForm2") {
        df2.process(distribution(generator));
    };
    auto df2T = Filter::IIR::Biquadratic::DirectForm2Transpose<double>();
    BENCHMARK("Biquadratic::DirectForm2Transpose") {
        df2T.process(distribution(generator));
    };
    auto df1N = Filter::IIR::Bipolynomial::DirectForm1<double>(2);
    BENCHMARK("Bipolynomial::DirectForm1") {
        df1N.process(distribution(generator));
    };
    auto df2N = Filter::IIR::Bipolynomial::DirectForm2<double>(2);
    BENCHMARK("Bipolynomial::DirectForm2") {
        df2N.process(distribution(generator));
    };
}

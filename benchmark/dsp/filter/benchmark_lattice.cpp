// Benchmark code for the Filter::Lattice structures.
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
#include "dsp/filter/lattice.hpp"

TEST_CASE("process samples with lattice filter with 1 stage") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<float>(0.0001, 0.1);
    BENCHMARK("baseline") {
        distribution(generator);
    };
    static constexpr int STAGES = 1;
    {
        typedef Filter::Lattice::StageFIR<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageFIR<float>") {
            float f = distribution(generator);
            float g = f;
            Filter::process(stages, STAGES, f, g);
        };
    }
    {
        typedef Filter::Lattice::StageAllPole<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageAllPole<float>") {
            float f = distribution(generator);
            float g = f;
            Filter::process(stages, STAGES, f, g);
        };
    }
    {
        typedef Filter::Lattice::StageIIR<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
            stages[i].c = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageIIR<float>") {
            float f = distribution(generator);
            float g = 0.5f;
            Filter::process(stages, STAGES, f, g);
        };
    }
}

TEST_CASE("process samples with lattice filter with 2 stage") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<float>(0.0001, 0.1);
    BENCHMARK("baseline") {
        distribution(generator);
    };
    static constexpr int STAGES = 2;
    {
        typedef Filter::Lattice::StageFIR<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageFIR<float>") {
            float f = distribution(generator);
            float g = f;
            Filter::process(stages, STAGES, f, g);
        };
    }
    {
        typedef Filter::Lattice::StageAllPole<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageAllPole<float>") {
            float f = distribution(generator);
            float g = f;
            Filter::process(stages, STAGES, f, g);
        };
    }
    {
        typedef Filter::Lattice::StageIIR<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
            stages[i].c = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageIIR<float>") {
            float f = distribution(generator);
            float g = 0.5f;
            Filter::process(stages, STAGES, f, g);
        };
    }
}

TEST_CASE("process samples with lattice filter with 3 stage") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<float>(0.0001, 0.1);
    BENCHMARK("baseline") {
        distribution(generator);
    };
    static constexpr int STAGES = 3;
    {
        typedef Filter::Lattice::StageFIR<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageFIR<float>") {
            float f = distribution(generator);
            float g = f;
            Filter::process(stages, STAGES, f, g);
        };
    }
    {
        typedef Filter::Lattice::StageAllPole<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageAllPole<float>") {
            float f = distribution(generator);
            float g = f;
            Filter::process(stages, STAGES, f, g);
        };
    }
    {
        typedef Filter::Lattice::StageIIR<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
            stages[i].c = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageIIR<float>") {
            float f = distribution(generator);
            float g = 0.5f;
            Filter::process(stages, STAGES, f, g);
        };
    }
}

TEST_CASE("process samples with lattice filter with 10 stage") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<float>(0.0001, 0.1);
    BENCHMARK("baseline") {
        distribution(generator);
    };
    static constexpr int STAGES = 10;
    {
        typedef Filter::Lattice::StageFIR<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageFIR<float>") {
            float f = distribution(generator);
            float g = f;
            Filter::process(stages, STAGES, f, g);
        };
    }
    {
        typedef Filter::Lattice::StageAllPole<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageAllPole<float>") {
            float f = distribution(generator);
            float g = f;
            Filter::process(stages, STAGES, f, g);
        };
    }
    {
        typedef Filter::Lattice::StageIIR<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
            stages[i].c = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageIIR<float>") {
            float f = distribution(generator);
            float g = 0.5f;
            Filter::process(stages, STAGES, f, g);
        };
    }
}

TEST_CASE("process samples with lattice filter with 100 stage") {
    // a random distribution of sample times to ensure that the loop is not
    // optimized away
    auto generator = std::default_random_engine();
    auto distribution = std::normal_distribution<float>(0.0001, 0.1);
    BENCHMARK("baseline") {
        distribution(generator);
    };
    static constexpr int STAGES = 100;
    {
        typedef Filter::Lattice::StageFIR<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageFIR<float>") {
            float f = distribution(generator);
            float g = f;
            Filter::process(stages, STAGES, f, g);
        };
    }
    {
        typedef Filter::Lattice::StageAllPole<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageAllPole<float>") {
            float f = distribution(generator);
            float g = f;
            Filter::process(stages, STAGES, f, g);
        };
    }
    {
        typedef Filter::Lattice::StageIIR<float> Filter;
        Filter stages[STAGES];
        for (int i = 0; i < STAGES; i++) {
            stages[i].b = 0.5f;
            stages[i].c = 0.5f;
        }
        BENCHMARK("Filter::Lattice::StageIIR<float>") {
            float f = distribution(generator);
            float g = 0.5f;
            Filter::process(stages, STAGES, f, g);
        };
    }
}

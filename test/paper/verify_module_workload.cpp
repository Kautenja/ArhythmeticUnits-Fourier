// Untimed module controls and active-port/voice wiring; no benchmark launch.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../benchmark/paper/modules.hpp"
using namespace Paper;

template<typename Module>
void verify(Config c) {
    validate_backend(c);
    Paper::Context context(c.rate);
    Host<Module> host(c);
    require(host.module.get_window_function() == window_function(c), "Module window control changed");
    require(std::abs(host.module.get_time_smoothing_alpha()-temporal_alpha(c)) < 1e-6,
        "Module time knob convention changed");
    for (size_t port = 0; port < host.module.inputs.size(); ++port)
        require(host.module.inputs[port].channels == (port < c.active_ports ? int(c.voices) : 0),
            "Active ports/voices were ignored");
    const auto input = signal(c);
    for (size_t i = 0; i < 2*c.hop; ++i) host.process(input_sample(c, input, i));
    host.check();
}
int main() {
    Config c{}; c.workload_schema = 3; c.n = 2048; c.hop = 1024; c.block = 64; c.count = 1;
    c.voices = 3; c.rate = 48000; c.state = "startup"; c.alignment = "aligned"; c.pass = "callback";
    c.callbacks = 32; c.window = "blackman-harris"; c.temporal_mode = "module-seconds"; c.temporal_value = .1;
    for (const std::string fixture : {"silence", "noise"}) {
        c.fixture = fixture; c.backend = "fourier"; c.active_ports = 2; c.octave = 1;
        verify<SpectrumAnalyzer>(c);
        c.backend = "spectre"; c.active_ports = 1; c.octave = 0;
        verify<Spectrogram>(c);
    }
    std::cout << "Module controls and silence checks passed without timing\n";
}

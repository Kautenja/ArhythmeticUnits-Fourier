// An spectrogram module.
//
// Copyright 2024 Arhythmetic Units
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <algorithm>
#include <string>
#include <iostream>
#include "./dsp/math.hpp"
#include "./dsp/trigger.hpp"
#include "./plugin.hpp"
#include "./text_knob.hpp"

/// @brief Compute the Fast Fourier Transform (FTT) in-place.
///
/// @tparam N the length of the input sequence.
/// @param input the input vector to compute the FFT of, \f$x[n]\f$
/// @param window a windowing function to use when calculating coefficients.
///
template<int N>
void fft_(
    std::vector<std::complex<float>>& input,
    const Math::Window::Function& window = Math::Window::Function::Boxcar
) {
    static_assert((N & (N - 1)) == 0, "N must be a power of 2");
    std::vector<std::complex<float>> even(N / 2), odds(N / 2);
    for (int i = 0; i < N / 2; ++i) {
        even[i] = Math::Window::window<float>(window, 2*i, N, false) * input[2*i] / Math::Window::coherent_gain(window);
        odds[i] = Math::Window::window<float>(window, 2*i+1, N, false) * input[2*i+1] / Math::Window::coherent_gain(window);
    }
    fft_<N / 2>(even);
    fft_<N / 2>(odds);
    for (int i = 0; i < N / 2; ++i) {
        std::complex<float> w = exp(std::complex<float>(0, -2 * Math::pi<float>() * i / N)) * odds[i];
        input[i] = even[i] + w;
        input[i + N / 2] = even[i] - w;
    }
}

// The base case of a 1-point FFT is a NoOp.
template<>
inline void fft_<1>(std::vector<std::complex<float>>&, const Math::Window::Function&) { }

/// @brief Linearly interpolate between coefficients.
/// @param coeff The coefficients to interpolate between.
/// @param index The floating point index to sample from the coefficients.
/// @returns The linearly interpolated coefficient at the given index.
static inline std::complex<float> interpolate_coefficients(const Math::DFTCoefficients& coeff, float index) {
    int y0 = floorf(index);
    int y1 = ceilf(index);
    float alpha = index - y0;
    return (1 - alpha) * coeff[y0] + alpha * coeff[y1];
}

/// A spectrogram module.
struct Spectrogram : rack::Module {
 public:
    enum {
        N_FFT = 512,
        N_STFT = 1024
    };

    enum ParamIds {
        PARAM_INPUT_GAIN,
        PARAM_RUN,
        PARAM_WINDOW_FUNCTION,
        PARAM_FREQUENCY_SCALE,
        NUM_PARAMS
    };

    enum InputIds {
        INPUT_SIGNAL,
        NUM_INPUTS
    };

    enum OutputIds {
        NUM_OUTPUTS
    };

    enum LightIds {
        LIGHT_RUN,
        NUM_LIGHTS
    };

 private:
    /// The delay line for tracking the input signal x[t]
    Math::ContiguousCircularBuffer<float> delay;

    /// A clock divider for calculating the DFT on a specific period.
    Trigger::Divider dft_divider;

    /// a clock divider for updating the lights every 512 frames
    Trigger::Divider lightDivider;

    /// a Schmitt Trigger for handling presses on the clock button
    Trigger::Threshold<float> runTrigger;

    /// Whether the analyzer is running or not.
    bool is_running = true;

 public:
    /// A buffer for storing the DFT coefficients of x[t-N], ..., x[t]
    Math::STFTCoefficients coefficients;

    /// @brief The frequency scale to render.
    FrequencyScale frequency_scale = FrequencyScale::Logarithmic;

    /// The index of the current STFT hop.
    uint32_t hop_index = 0;

    /// @brief Initialize a new spectrogram.
    Spectrogram() : coefficients(N_STFT) {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        // Setup the input signal port and controls.
        configParam(PARAM_INPUT_GAIN, 0, std::pow(10.f, 24.f / 20.f), std::pow(10.f, 18.f / 20.f), "Input Gain", " dB", -10, 20);
        configInput(INPUT_SIGNAL, "TODO");
        // Configure the run button.
        configParam<TriggerParamQuantity>(PARAM_RUN, 0.f, 1.f, 0.f, "Run");
        getParamQuantity(PARAM_RUN)->description =
            "Enables or disables the analyzer. When disabled,\n"
            "the analyzer stops buffering and processing new audio.";
        // Setup the window function as a custom discrete enumeration.
        configParam<WindowFunctionParamQuantity>(PARAM_WINDOW_FUNCTION, 0, static_cast<size_t>(Math::Window::Function::Flattop), static_cast<size_t>(Math::Window::Function::Flattop), "Window");
        getParamQuantity(PARAM_WINDOW_FUNCTION)->snapEnabled = true;
        getParamQuantity(PARAM_WINDOW_FUNCTION)->description =
            "The window function to apply before the FFT. Windowing\n"
            "helps reduce spectral leakage in the frequency domain.";
        // Setup the discrete frequency scale selector.
        configParam<FrequencyScaleParamQuantity>(PARAM_FREQUENCY_SCALE, 0, 1, 1, "X Scale");
        getParamQuantity(PARAM_FREQUENCY_SCALE)->snapEnabled = true;
        getParamQuantity(PARAM_FREQUENCY_SCALE)->description =
            "The frequency-axis scale on the display. The DFT spaces\n"
            "frequencies linearly but humans hear frequencies along\n"
            "a logarithmic scale.";

        for (std::size_t i = 0; i < coefficients.size(); i++)
            coefficients[i] = Math::DFTCoefficients(N_FFT);
        delay.resize(N_FFT);
        dft_divider.setDivision(N_FFT / 2);
        lightDivider.setDivision(512);
        onReset();
    }

    /// @brief Respond to the module being reset by the host environment.
    inline void onReset() final {
        delay.clear();
        dft_divider.reset();
        lightDivider.reset();
        for (std::size_t i = 0; i < coefficients.size(); i++)
            std::fill(coefficients[i].begin(), coefficients[i].end(), 0.f);
        frequency_scale = FrequencyScale::Logarithmic;
        is_running = true;
    }

    /// @brief Convert the module's state to a JSON object.
    ///
    /// @returns a pointer to a new json_t object with the module's state
    ///
    inline json_t* dataToJson() final {
        json_t* rootJ = json_object();
        JSON::set<int>(rootJ, "frequency_scale", static_cast<int>(frequency_scale));
        JSON::set<bool>(rootJ, "is_running", is_running);
        return rootJ;
    }

    /// @brief Load the module's state from a JSON object.
    ///
    /// @param rootJ a pointer to a json_t with state data for this module
    ///
    inline void dataFromJson(json_t* rootJ) final {
        JSON::get<int>(rootJ,  "frequency_scale", [&](const int& value)  { frequency_scale = static_cast<FrequencyScale>(value); });
        JSON::get<bool>(rootJ, "is_running",      [&](const bool& value) { is_running = value; });
    }

    // -----------------------------------------------------------------------
    // MARK: Parameters
    // -----------------------------------------------------------------------

    // /// @brief Return the current sample rate of the module.
    // /// @returns The sample rate of the module.
    // inline float get_sample_rate() const { return sample_rate; }

    // Window Function

    /// @brief Return the window function.
    /// @returns The window function for computing DFT coefficients.
    inline Math::Window::Function get_window_function() {
        const auto value = params[PARAM_WINDOW_FUNCTION].getValue();
        return static_cast<Math::Window::Function>(value);
    }

    /// @brief Set the window function.
    /// @param value The window function for computing DFT coefficients.
    inline void set_window_function(const Math::Window::Function& value) {
        params[PARAM_WINDOW_FUNCTION].setValue(static_cast<float>(value));
    }

    // Frequency Scale

    /// @brief Return the frequency scale setting.
    /// @returns The frequency scale for rendering the X axis.
    inline FrequencyScale get_frequency_scale() {
        const auto value = params[PARAM_FREQUENCY_SCALE].getValue();
        return static_cast<FrequencyScale>(value);
    }

    /// @brief Set the frequency scale setting.
    /// @param value The frequency scale for rendering the X axis.
    inline void set_frequency_scale(const FrequencyScale& value) {
        params[PARAM_FREQUENCY_SCALE].setValue(static_cast<float>(value));
    }

    // -----------------------------------------------------------------------
    // MARK: Processing
    // -----------------------------------------------------------------------

    /// @brief Process the window length and hop length parameters.
    /// @details
    /// Resizes the delay lines and DFT buffers to the length of the window.
    /// Also sets the DFT divider to the length of the hop.
    inline void process_window() {
        // // Determine the length of the delay lines and associated FFTs.
        // const size_t N = get_window_length();
        // window_function.set_window(get_window_function(), N, false, true);
        // // Iterate over the number of channels to resize buffers.
        // for (size_t i = 0; i < NUM_CHANNELS; i++) {
        //     if (ffts[i].size() != N)
        //         ffts[i].resize(N);
        //     if (delay[i].size() == N) continue;
        //     // Resize and clear the delay lines.
        //     delay[i].resize(N);
        //     delay[i].clear();
        //     // Resize and clear the coefficient buffers.
        //     filtered_coefficients[i].resize(N);
        //     std::fill(filtered_coefficients[i].begin(), filtered_coefficients[i].end(), 0.f);
        //     // Update the rasterized coefficients from the FFT length.
        //     rasterized_coefficients[i].resize(N / 2.f + 1);
        //     for (auto& coeff : rasterized_coefficients[i]) {
        //         coeff.x = 0.f;
        //         coeff.y = 0.f;
        //     }
        //     render_coefficients[i] = rasterized_coefficients[i];
        // }
    }

    /// @brief Process a sample.
    ///
    /// @param args the sample arguments (sample rate, sample time, etc.)
    ///
    void process(const ProcessArgs& args) final {
        // Buffer the input signal.
        delay.insert(Math::Eurorack::fromAC(inputs[INPUT_SIGNAL].getVoltage()));
        // Handle presses on the "run" button
        if (runTrigger.process(params[PARAM_RUN].getValue()))
            is_running = !is_running;
        // Process samples with the DFT
        const float gain = params[PARAM_INPUT_GAIN].getValue();
        if (dft_divider.process() && is_running) {
            for (int i = 0; i < N_FFT; i++) coefficients[hop_index][i] = gain * delay.at(i);
            fft_<N_FFT>(coefficients[hop_index], get_window_function());
            hop_index = (hop_index + 1) % N_STFT;
        }
        // set lights
        if (lightDivider.process()) {
            const auto lightTime = args.sampleTime * lightDivider.getDivision();
            lights[LIGHT_RUN].setSmoothBrightness(is_running, lightTime);
        }
    }
};

/// A widget that displays an image stored in a 32-bit RGBA pixel buffer.
struct SpectralImageDisplay : rack::TransparentWidget {
 private:
    /// The spectrogram module to render data from.
    Spectrogram* module;
    /// a pointer to the image to draw the display to
    int screen = -1;

 public:
    /// The radius of the rounded corners of the screen
    int corner_radius = 5;
    /// The background color of the screen
    NVGcolor background_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    /// The stroke color for the axis lines
    NVGcolor axis_stroke_color = {{{1.0f, 1.0f, 1.0f, 0.2f}}};
    /// The width of the lines to render for axes.
    float axis_stroke_width = 1;
    /// The font color for the axis text.
    NVGcolor axis_font_color = {{{1.0f, 1.0f, 1.0f, 1.0f}}};
    /// The font size for the axis text.
    float axis_font_size = 8;
    /// The color of the line to render.
    NVGcolor coefficients_stroke_color = {{{0.9f, 0.85f, 0.15f, 0.75f}}};
    /// The width of the lines to render for coefficients.
    float coefficients_stroke_width = 1.5;

    explicit SpectralImageDisplay(Spectrogram* module_) :
        TransparentWidget(), module(module_) { }

    /// @brief Draw the Y ticks with a linear scale.
    ///
    /// @param args the arguments for the current draw call
    ///
    void draw_y_ticks_linear(const DrawArgs& args) const {
        const float num_y_ticks = 10;
        // Determine the highest frequency to render (Nyquist frequency)
        // and quantize it to the nearest 1000'th place. This allows the
        // spectra to be linearly cut up in a way that yields friendly
        // frequencies.
        const float nyquist_frequency = 1e3 * roundf(0.001 * APP->engine->getSampleRate() / 2);
        const auto quantized_sample_rate = 2 * nyquist_frequency / APP->engine->getSampleRate();
        for (float i = 1; i < num_y_ticks; i++) {
            // Determine the relative position and re-scale it to the
            // pixel location on-screen.
            const auto position = i / num_y_ticks;
            const auto point_y = rescale(position, 0.f, quantized_sample_rate, box.size.y, 0);
            // Render the tick marker.
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, point_y);
            nvgLineTo(args.vg, box.size.x, point_y);
            nvgStrokeWidth(args.vg, axis_stroke_width);
            nvgStrokeColor(args.vg, axis_stroke_color);
            nvgStroke(args.vg);
            nvgClosePath(args.vg);
            // Render the tick label.
            std::stringstream stream;
            stream << std::fixed << std::setprecision(1) << (nyquist_frequency * position / 1000.f) << "kHz";
            nvgFontSize(args.vg, axis_font_size);
            nvgFillColor(args.vg, axis_font_color);
            nvgTextAlign(args.vg, NVG_ALIGN_BOTTOM | NVG_ALIGN_CENTER);
            nvgText(args.vg, 15, point_y + 4, stream.str().c_str(), NULL);
        }
    }

    /// @brief Draw the Y ticks with an exponential scale.
    ///
    /// @param args the arguments for the current draw call
    ///
    void draw_y_ticks_logarithmic(const DrawArgs& args) const {
        // Determine the highest frequency to render (Nyquist frequency)
        const float nyquist_frequency = APP->engine->getSampleRate() / 2;
        // Iterate over frequencies exponentially (base 10) starting
        // at 100Hz up to the Nyquist frequency (e.g., 100, 1000, ...).
        const auto nyquist_exponnent = floor(log10(nyquist_frequency)) + 1;
        for (float exponent = 2.f; exponent < nyquist_exponnent; exponent++) {
            // Iterate over harmonics of the base frequency, i.e., if
            // we're at base 100Hz, iterate over 200Hz, 300Hz, ...
            const float base_frequency = powf(10.f, exponent);
            for (float offset = 1.f; offset < 10.f; offset++) {
                // Scale base frequency to offset to the n'th harmonic.
                const float frequency = base_frequency * offset;
                if (frequency >= nyquist_frequency) break;
                nvgBeginPath(args.vg);
                // Re-scale the frequency to a pixel location and render.
                const auto position = sqrt(frequency / nyquist_frequency);
                const auto point_y = rescale(position, 0.f, 1.f, box.size.y, 0);
                nvgMoveTo(args.vg, 0, point_y);
                nvgLineTo(args.vg, box.size.x, point_y);
                nvgStrokeWidth(args.vg, axis_stroke_width);
                nvgStrokeColor(args.vg, axis_stroke_color);
                nvgStroke(args.vg);
                nvgClosePath(args.vg);
            }
            // Render a label with the base frequency in kHz.
            std::stringstream stream;
            stream << std::fixed << std::setprecision(1) << (base_frequency / 1000.f) << "kHz";
            nvgFontSize(args.vg, axis_font_size);
            nvgFillColor(args.vg, axis_font_color);
            nvgTextAlign(args.vg, NVG_ALIGN_BOTTOM | NVG_ALIGN_CENTER);
            const auto point_y = rescale(sqrt(base_frequency / nyquist_frequency), 0.f, 1.f, box.size.y, 0);
            nvgText(args.vg, 15, point_y + 4, stream.str().c_str(), NULL);
        }
    }

    /// @brief Draw the spectrogram.
    ///
    /// @param args the arguments for the current draw call
    ///
    void draw_spectrogram(const DrawArgs& args) {
        // Determine the dimensions of the spectral image.
        const int width = module->coefficients.size();
        const int height = module->coefficients[0].size() / 2;
        // Create a pixel buffer from the spectral image in RGBA8888 format.
        uint8_t pixels[height * width * 4];
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float scaled_y = y;
                if (module->frequency_scale == FrequencyScale::Logarithmic)
                    scaled_y = height * Math::squared(scaled_y / height);
                auto coeff = interpolate_coefficients(module->coefficients[x], scaled_y);
                // range from (-inf, 0] to (0, 1] such that 1 is at 0dB.
                auto color = Math::ColorMap::magma(abs(coeff) / height);
                pixels[4 * (width * (height - 1 - y) + x) + 0] = color.r * 255;
                pixels[4 * (width * (height - 1 - y) + x) + 1] = color.g * 255;
                pixels[4 * (width * (height - 1 - y) + x) + 2] = color.b * 255;
                pixels[4 * (width * (height - 1 - y) + x) + 3] = static_cast<uint8_t>(0.7 * 255);
            }
        }

        // create / update the image container
        if (screen == -1)  // check if the screen has been initialized yet
            screen = nvgCreateImageRGBA(args.vg, width, height, 0, pixels);
        else  // update the screen with the pixel data
            nvgUpdateImage(args.vg, screen, pixels);
        // draw the screen
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, corner_radius);
        nvgFillPaint(args.vg, nvgImagePattern(args.vg, 0, 0, box.size.x, box.size.y, 0, screen, 1.0));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, axis_stroke_width);
        nvgStrokeColor(args.vg, axis_stroke_color);
        nvgStroke(args.vg);
        nvgClosePath(args.vg);
        // scan-line
        nvgBeginPath(args.vg);
        float scan_x = module->hop_index / static_cast<float>(Spectrogram::N_STFT);
        nvgMoveTo(args.vg, scan_x * box.size.x, 0);
        nvgLineTo(args.vg, scan_x * box.size.x, box.size.y);
        nvgStrokeWidth(args.vg, axis_stroke_width);
        nvgStrokeColor(args.vg, axis_stroke_color);
        nvgStroke(args.vg);
        nvgClosePath(args.vg);
    }

    /// @brief Draw the display on the main context.
    ///
    /// @param args the arguments for the draw context for this widget
    ///
    void drawLayer(const DrawArgs& args, int layer) override {
        if (module != nullptr && layer == 1) {  // draw regardless of brightness settings.
            // Draw the background.
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, corner_radius);
            nvgFillColor(args.vg, background_color);
            nvgFill(args.vg);
            nvgClosePath(args.vg);
            // draw ticks for the axes of the plot.
            switch (module->frequency_scale) {
            case FrequencyScale::Linear:
                draw_y_ticks_linear(args);
                break;
            case FrequencyScale::Logarithmic:
                draw_y_ticks_logarithmic(args);
                break;
            }
            // Draw the spectrogram.
            draw_spectrogram(args);
        }
        Widget::drawLayer(args, layer);
    }
};

/// The base-name for the Spectrogram panel files.
const char BASENAME[] = "res/Spectrogram";

struct SpectrogramWidget : ThemedWidget<BASENAME> {
    explicit SpectrogramWidget(Spectrogram* module) : ThemedWidget<BASENAME>() {
        setModule(module);
        // Spectrogram display
        SpectralImageDisplay* display = new SpectralImageDisplay(static_cast<Spectrogram*>(module));
        display->setPosition(Vec(RACK_GRID_WIDTH + 55, RACK_GRID_WIDTH));
        display->setSize(Vec(box.size.x - 2 * RACK_GRID_WIDTH - 55, box.size.y - 2 * RACK_GRID_WIDTH));
        addChild(display);
        // Input/output/parameters/lights
        addInput(createInput<PJ301MPort>(Vec(20, box.size.y - RACK_GRID_WIDTH - 35), module, Spectrogram::INPUT_SIGNAL));
        addParam(createParam<Rogan2PWhite>(Vec(18, box.size.y - RACK_GRID_WIDTH - 100), module, Spectrogram::PARAM_INPUT_GAIN));
        addParam(createParamCentered<PB61303>(Vec(35, 45), module, Spectrogram::PARAM_RUN));
        addChild(createLightCentered<PB61303Light<WhiteLight>>(Vec(35, 45), module, Spectrogram::LIGHT_RUN));

        // Screen controls.
        // Window function control with custom angles to match discrete range.
        auto window_function_param = createParam<WindowFunctionTextKnob>(Vec(50 + 0 * 66, 330), module, Spectrogram::PARAM_WINDOW_FUNCTION);
        window_function_param->maxAngle = 2.f * M_PI;
        addParam(window_function_param);
        // Frequency scale control with custom angles to match discrete range.
        auto frequency_scale_param = createParam<FrequencyScaleTextKnob>(Vec(50 + 3 * 66, 330), module, Spectrogram::PARAM_FREQUENCY_SCALE);
        frequency_scale_param->maxAngle = 0.3 * M_PI;
        addParam(frequency_scale_param);

        // Screws
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    }

    /// @brief Append the context menu to the module when right clicked.
    ///
    /// @param menu the menu object to add context items for the module to
    ///
    void appendContextMenu(Menu* menu) override {
        // get a pointer to the module
        Spectrogram* const module = dynamic_cast<Spectrogram*>(this->module);

        // -------------------------------------------------------------------
        // MARK: Frequency Scale
        // -------------------------------------------------------------------

        // labels for the frequency axis scaling options
        static const char* FREQUENCY_SCALE_LABELS[2] = {
            "Linear",
            "Logarithmic"
        };

        /// A menu item for changing the frequency scale.
        struct FrequencyScaleItem : MenuItem {
            /// The module to update.
            Spectrogram* module = nullptr;
            /// The frequency scale option for this menu item.
            FrequencyScale frequency_scale;

            inline void onAction(const event::Action& e) override {
                module->frequency_scale = frequency_scale;
            }
        };
        // add the item to the menu
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Frequency Scale"));
        for (unsigned i = 0; i < 2; i++) {
            auto frequency_scale = static_cast<FrequencyScale>(i);
            const auto check = CHECKMARK(module->frequency_scale == frequency_scale);
            auto item = createMenuItem<FrequencyScaleItem>(FREQUENCY_SCALE_LABELS[i], check);
            item->module = module;
            item->frequency_scale = frequency_scale;
            menu->addChild(item);
        }

        // -------------------------------------------------------------------
        // MARK: Super
        // -------------------------------------------------------------------

        ThemedWidget<BASENAME>::appendContextMenu(menu);
    }
};

Model* modelSpectrogram = createModel<Spectrogram, SpectrogramWidget>("Spectrogram");

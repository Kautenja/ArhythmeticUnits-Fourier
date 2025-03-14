// A spectrogram module.
//
// Copyright 2025 Arhythmetic Units
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

#include <algorithm>  // std::fill
#include <string>     // std::string
#include <limits>     // std::numeric_limits
#include <iomanip>    // std::fixed, std::setprecision
#include "./plugin.hpp"

/// @brief A spectrogram module.
struct Spectrogram : Module {
 public:
    enum {
        N_FFT = 2048,
        N_STFT = 512
    };

    enum ParamIds {
        PARAM_INPUT_GAIN,
        PARAM_RUN,
        PARAM_WINDOW_FUNCTION,
        PARAM_FREQUENCY_SCALE,
        PARAM_TIME_SMOOTHING,
        PARAM_FREQUENCY_SMOOTHING,
        PARAM_LOW_FREQUENCY,
        PARAM_HIGH_FREQUENCY,
        PARAM_SLOPE,
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
    /// The sample rate of the module.
    float sample_rate = 0.f;

    /// DC-blocking filters for AC-coupled mode.
    Filter::DCBlocker<float> dc_blocker;

    /// The delay line for tracking the input signal x[t]
    Math::ContiguousCircularBuffer<float> delay;

    /// The window function for windowing the FFT.
    Math::Window::CachedWindow<float> window_function;

    /// An on-the-fly FFT calculator for each input channel.
    Math::OnTheFlyRFFT<float> fft;

    /// A copy of the low-pass filtered coefficients.
    Math::DFTCoefficients filtered_coefficients;

    /// A buffer for storing the DFT coefficients of x[t-N], ..., x[t]
    Math::STFTCoefficients coefficients;

    /// The index of the current STFT hop.
    uint32_t hop_index = 0;

    /// a clock divider for updating the lights every 512 frames
    Trigger::Divider light_divider;

    /// a Schmitt Trigger for handling presses on the clock button
    Trigger::Threshold<float> run_trigger;

    /// Whether the analyzer is running or not.
    bool is_running = true;

 public:
    /// Whether to apply AC coupling to input signal.
    bool is_ac_coupled = true;

    /// The color map to use when rasterizing STFT coefficients to images.
    Math::ColorMap::Function color_map = Math::ColorMap::Function::Magma;

    /// @brief Initialize a new spectrogram.
    Spectrogram() :
        sample_rate(APP->engine->getSampleRate()),
        delay(N_FFT),
        window_function(Math::Window::Function::Boxcar, N_FFT, false, true),
        fft(N_FFT),
        filtered_coefficients(N_FFT),
        coefficients(N_STFT) {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        // Setup the input signal port and controls.
        configParam(PARAM_INPUT_GAIN, 0, std::pow(10.f, 12.f / 20.f), std::pow(10.f, 6.f / 20.f), "Input Gain", " dB", -10, 20);
        configInput(INPUT_SIGNAL, "TODO");
        // Configure the run button.
        configButton(PARAM_RUN, "Run");
        getParamQuantity(PARAM_RUN)->description =
            "Enables or disables the analyzer. When disabled,\n"
            "the analyzer stops buffering and processing new audio.";
        // Setup the window function as a custom discrete enumeration.
        configSwitch(PARAM_WINDOW_FUNCTION, 0, Math::Window::names().size() - 1, static_cast<size_t>(Math::Window::Function::Flattop), "Window", Math::Window::names());
        getParamQuantity(PARAM_WINDOW_FUNCTION)->description =
            "The window function to apply before the FFT. Windowing\n"
            "helps reduce spectral leakage in the frequency domain.";
        // Setup the discrete frequency scale selector.
        configSwitch(PARAM_FREQUENCY_SCALE, 0, frequency_scale_names().size() - 1, static_cast<size_t>(FrequencyScale::Logarithmic), "Y Scale", frequency_scale_names());
        getParamQuantity(PARAM_FREQUENCY_SCALE)->description =
            "The frequency-axis scale on the display. The DFT spaces\n"
            "frequencies linearly but humans hear frequencies along\n"
            "a logarithmic scale.";
        // Setup time smoothing in seconds with millisecond render scaling.
        configParam(PARAM_TIME_SMOOTHING, 0.f, 2.5f, 0.f, "Average", "ms", 0, 1000);
        getParamQuantity(PARAM_TIME_SMOOTHING)->displayPrecision = 4;
        getParamQuantity(PARAM_TIME_SMOOTHING)->description =
            "The temporal smoothing filter of the STFT. Higher values\n"
            "increase the averaging duration, making the spectrum move\n"
            "more slowly to provide a general impression of signal\n"
            "frequency content.";
        // Setup frequency smoothing as a custom discrete enumeration.
        configSwitch(PARAM_FREQUENCY_SMOOTHING, 0, frequency_smoothing_names().size() - 1, static_cast<size_t>(FrequencySmoothing::None), "Smooth", frequency_smoothing_names());
        getParamQuantity(PARAM_FREQUENCY_SMOOTHING)->description =
            "The fractional-octave smoothing filter of the DFT. For\n"
            "example, 1/6-oct smoothing reduces fine details in the\n"
            "high frequencies.";
        // Setup the low frequency range selector based on the Nyquist rate.
        configParam(PARAM_LOW_FREQUENCY, 0, sample_rate / 2.f, 0, "LO Freq", "Hz");
        getParamQuantity(PARAM_LOW_FREQUENCY)->description =
            "The lower frequency bound for display. Frequencies below\n"
            "this bound are not shown.";
        // Setup the high frequency range selector based on the Nyquist rate.
        configParam(PARAM_HIGH_FREQUENCY, 0, sample_rate / 2.f, sample_rate / 2.f, "HI Freq", "Hz");
        getParamQuantity(PARAM_HIGH_FREQUENCY)->description =
            "The upper frequency bound for display. Frequencies above\n"
            "this bound are not shown.";
        // Setup the slope along a simple range of values. Use a default value
        // of 4.5dB/oct that will be familiar to SPAN users.
        configParam(PARAM_SLOPE, -9, 9, 4.5, "Slope", "dB/oct");
        getParamQuantity(PARAM_SLOPE)->description =
            "The spectrum's slope around 1kHz. Useful for visually\n"
            "compensating the natural roll-off of high frequency energy\n"
            "in musical signals. Typical values are 4.5 and 3.0.";
        // Disable randomization for all parameters.
        for (size_t i = 0; i < NUM_PARAMS; i++)
            getParamQuantity(i)->randomizeEnabled = false;
        // Setup the buffer of coefficients.
        for (std::size_t i = 0; i < coefficients.size(); i++)
            coefficients[i] = Math::DFTCoefficients(N_FFT);
        // Resize the delay line for the number of FFT bins.
        onReset();
    }

    /// @brief Respond to the module being reset by the host environment.
    inline void onReset() final {
        Module::onReset();
        // Resent instance state of the module and menu preferences.
        is_running = true;
        hop_index = 0;
        is_ac_coupled = true;
        color_map = Math::ColorMap::Function::Magma;
        // Clear delay lines and cached coefficients.
        delay.clear();
        for (std::size_t i = 0; i < coefficients.size(); i++)
            std::fill(coefficients[i].begin(), coefficients[i].end(), 0.f);
        std::fill(filtered_coefficients.begin(), filtered_coefficients.end(), 0.f);
        // Act as if the sample rate has changed to reset remaining state.
        onSampleRateChange();
    }

    /// @brief Respond to a change in sample rate from the engine.
    inline void onSampleRateChange() final {
        Module::onSampleRateChange();
        sample_rate = APP->engine->getSampleRate();
        // Set the light divider relative to the sample rate and reset it.
        light_divider.setDivision(512);
        light_divider.reset();
        // Update the low frequency bound and preserve settings.
        const auto low_frequency = get_low_frequency();
        getParamQuantity(PARAM_LOW_FREQUENCY)->maxValue = sample_rate / 2.f;
        set_low_frequency(low_frequency);
        // Update the high frequency bound and preserve settings.
        const auto high_frequency = get_high_frequency();
        auto param_high_frequency = getParamQuantity(PARAM_HIGH_FREQUENCY);
        param_high_frequency->maxValue = param_high_frequency->defaultValue = sample_rate / 2.f;
        set_high_frequency(high_frequency);
        // Set the transition width of DC-blocking filters for AC-coupled mode.
        dc_blocker.setTransitionWidth(10.f, sample_rate);
        dc_blocker.reset();
    }

    // -----------------------------------------------------------------------
    // MARK: Serialization
    // -----------------------------------------------------------------------

    /// @brief Convert the module's state to a JSON object.
    /// @returns a pointer to a new json_t object with the module's state.
    inline json_t* dataToJson() final {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "is_running", json_boolean(is_running));
        json_object_set_new(rootJ, "is_ac_coupled", json_boolean(is_ac_coupled));
        json_object_set_new(rootJ, "color_map", json_integer(static_cast<int>(color_map)));
        return rootJ;
    }

    /// @brief Load the module's state from a JSON object.
    /// @param rootJ a pointer to a json_t with state data for this module.
    inline void dataFromJson(json_t* rootJ) final {
        json_t* opt = nullptr;
        if ((opt = json_object_get(rootJ, "is_running")))
            is_running = json_boolean_value(opt);
        if ((opt = json_object_get(rootJ, "is_ac_coupled")))
            is_ac_coupled = json_boolean_value(opt);
        if ((opt = json_object_get(rootJ, "color_map")))
            color_map = static_cast<Math::ColorMap::Function>(json_integer_value(opt));
    }

    // -----------------------------------------------------------------------
    // MARK: Parameters
    // -----------------------------------------------------------------------

    /// @brief Return the current sample rate of the module.
    /// @returns The sample rate of the module.
    inline const float& get_sample_rate() const { return sample_rate; }

    /// @brief Return the current hop index of the STFT.
    /// @brief The current hop index in [0, STFT - 1]
    inline const uint32_t& get_hop_index() const { return hop_index; }

    /// @brief Return the STFT coefficients.
    /// @returns The current STFT coefficients.
    inline const Math::STFTCoefficients& get_coefficients() const {
        return coefficients;
    }

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

    // Hop Length

    /// @brief Return the hop length of the windowed DFT in samples.
    /// @returns The number of samples to hop between computations of the DFT.
    inline size_t get_hop_length() {
        return N_FFT >> 1;  // N_FFT / 2
        // return sample_rate * (2048 / 2) / 44100;
    }

    // /// @brief Return the hop length of the windowed DFT in samples.
    // /// @returns The number of samples to hop between computations of the DFT.
    // inline size_t get_hop_length(const float& duration = 10.f) const {
    //     return sample_rate * duration / N_STFT;
    // }

    // N STFT

    // /// @brief Return the length of the STFT for a certain length of time.
    // /// @returns The length of the STFT in frames for the input duration.
    // inline size_t get_n_stft(const float& duration = 10.f) {
    //     // return duration / (get_hop_length() / sample_rate);
    //     return sample_rate * duration / get_hop_length();
    // }

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

    // Time/Magnitude Smoothing

    /// @brief Return the time smoothing setting.
    /// @returns The time smoothing setting (measured in seconds.)
    inline float get_time_smoothing() {
        return params[PARAM_TIME_SMOOTHING].getValue();
    }

    /// @brief Set the time smoothing setting.
    /// @param value The time smoothing setting (measured in seconds.)
    inline void set_time_smoothing(const float& value) {
        params[PARAM_TIME_SMOOTHING].setValue(value);
    }

    /// @brief Compute the alpha parameter of the time smoothing filter.
    /// @returns The alpha parameter of an EMA smoothing filter.
    inline float get_time_smoothing_alpha() {
        // Determine the length of the smoothing filter.
        const float smoothing_time = params[PARAM_TIME_SMOOTHING].getValue();
        // If smoothing time is 0 or lower, alpha is always 0.
        if (smoothing_time <= 0.f) return 0.f;
        // Determine the hop-rate, i.e., the refresh rate of the DFT.
        const float hop_time = get_hop_length() / sample_rate;
        // Calculate alpha relative to the hop-rate to keep time normalized.
        return expf(-10.f * hop_time / smoothing_time);
    }

    // Frequency/Magnitude Smoothing

    /// @brief Return the frequency smoothing setting.
    /// @returns The frequency smoothing for rendering the coefficients.
    inline FrequencySmoothing get_frequency_smoothing() {
        const auto value = params[PARAM_FREQUENCY_SMOOTHING].getValue();
        return static_cast<FrequencySmoothing>(value);
    }

    /// @brief Set the frequency smoothing setting.
    /// @param value The frequency smoothing for rendering the coefficients.
    inline void set_frequency_smoothing(const FrequencySmoothing& value) {
        params[PARAM_FREQUENCY_SMOOTHING].setValue(static_cast<float>(value));
    }

    // Low Frequency Bound

    /// @brief Return the lowest frequency to render on the display.
    /// @returns The lowest frequency to render in Hz. If the frequency falls
    /// below the Nyquist rate, then the Nyquist rate is returned.
    inline float get_low_frequency() {
        return fmin(params[PARAM_LOW_FREQUENCY].getValue(), sample_rate / 2.f);
    }

    /// @brief Set the lowest frequency to render on the display.
    /// @param value The lowest frequency to render in Hz. If the value is
    /// above the Nyquist rate, then the value is clipped.
    inline void set_low_frequency(const float& value) {
        params[PARAM_LOW_FREQUENCY].setValue(fmin(value, sample_rate / 2.f));
    }

    // High Frequency Bound

    /// @brief Return the highest frequency to render on the display.
    /// @returns The highest frequency to render in Hz. If the frequency
    /// falls below the Nyquist rate, then the Nyquist rate is returned.
    inline float get_high_frequency() {
        return fmin(params[PARAM_HIGH_FREQUENCY].getValue(), sample_rate / 2.f);
    }

    /// @brief Set the highest frequency to render on the display.
    /// @param value The highest frequency to render in Hz. If the value is
    /// above the Nyquist rate, then the value is clipped.
    inline void set_high_frequency(const float& value) {
        params[PARAM_HIGH_FREQUENCY].setValue(fmin(value, sample_rate / 2.f));
    }

    // Magnitude/Frequency Slope

    /// @brief Return the slope of the Bode plot.
    /// @returns The slope of the Bode plot measured in dB/octave.
    inline float get_slope() {
        return params[PARAM_SLOPE].getValue();
    }

    /// @brief Set the slope of the Bode plot.
    /// @param value The slope of the Bode plot measured in dB/octave.
    inline void set_slope(const float& value) {
        params[PARAM_SLOPE].setValue(value);
    }

    // -----------------------------------------------------------------------
    // MARK: Processing
    // -----------------------------------------------------------------------

    /// @brief Process input signal.
    inline void process_input_signal() {
        // Get the input signal and convert to normalized bipolar [-1, 1].
        auto signal = Math::Eurorack::fromAC(inputs[INPUT_SIGNAL].getVoltageSum());
        // Determine the gain to apply to this channel's input signal.
        const auto gain = params[PARAM_INPUT_GAIN].getValue();
        // Pass signal through the DC blocking filter. Do this regardless
        // of whether we are in AC-coupling mode to ensure when switching
        // between modes there is no graphical delay from the filter
        // accumulating signal data.
        dc_blocker.process(signal);
        // If AC coupling is enabled, replace signal with DC blocker output.
        if (is_ac_coupled) signal = dc_blocker.getValue();
        // Insert the normalized and processed input signal into the delay.
        delay.insert(gain * signal);
    }

    /// @brief Process samples with the DFT.
    inline void process_coefficients() {
        if (fft.is_done_computing()) {
            // Perform octave smoothing. For an N-length FFT, smooth over the
            // first N/2 + 1 coefficients to omit reflected frequencies.
            const auto frequency_smoothing = get_frequency_smoothing();
            if (frequency_smoothing != FrequencySmoothing::None)
                fft.smooth(sample_rate, to_float(frequency_smoothing));
            // Pass the coefficients through a smoothing filter.
            const float alpha = get_time_smoothing_alpha();
            for (size_t n = 0; n < fft.coefficients.size(); n++)
                filtered_coefficients[n] = alpha * std::abs(filtered_coefficients[n]) + (1.f - alpha) * std::abs(fft.coefficients[n]);
            // Update the coefficients and increment the hop index.
            coefficients[hop_index] = filtered_coefficients;
            hop_index = (hop_index + 1) % N_STFT;
            // Add the delay line to the FFT pipeline.
            fft.buffer(delay.contiguous(), window_function.get_samples());
        }
        // Perform the number of FFT steps required at this hop-rate.
        fft.step(get_hop_length());
    }

    /// @brief Process a sample.
    /// @param args the sample arguments (sample rate, sample time, etc.)
    void process(const ProcessArgs& args) final {
        // Update the window function. We need asymmetric windows for FFT
        // analysis and need coherent gain to be integrated into the window.
        window_function.set_window(get_window_function(), N_FFT, false, true);
        // Handle presses to the run button.
        if (run_trigger.process(params[PARAM_RUN].getValue()))
            is_running = !is_running;
        // Process the input signal and compute SFT coefficients as needed.
        if (is_running) {
            process_input_signal();
            process_coefficients();
        }
        // Update the panel lights.
        if (light_divider.process()) {
            const auto light_time = args.sampleTime * light_divider.getDivision();
            lights[LIGHT_RUN].setSmoothBrightness(is_running, light_time);
        }
    }
};

/// A widget that displays an image stored in a 32-bit RGBA pixel buffer.
struct SpectralImageDisplay : TransparentWidget {
 private:
    /// The vertical (top) padding for the plot.
    const size_t pad_top = 20;
    /// The vertical (bottom) padding for the plot.
    const size_t pad_bottom = 50;
    /// The horizontal (left) padding for the plot.
    const size_t pad_left = 40;
    /// The horizontal (right) padding for the plot.
    const size_t pad_right = 15;
    /// The radius of the rounded corners of the screen
    const int corner_radius = 5;
    /// The background color of the screen
    const NVGcolor background_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    /// The stroke color for the axis lines
    const NVGcolor axis_stroke_color = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
    /// The width of the lines to render for axes.
    const float axis_stroke_width = 1;
    /// The font color for the axis text.
    const NVGcolor axis_font_color = {{{1.0f, 1.0f, 1.0f, 1.0f}}};
    /// The font size for the axis text.
    const float axis_font_size = 8;
    /// The stroke color for the cross-hair
    const NVGcolor cross_hair_stroke_color = {{{0.2f, 0.2f, 0.2f, 1.0f}}};

    /// The spectrogram module to render data from.
    Spectrogram* module = nullptr;

    /// the state of the mouse.
    struct {
        /// A state variable determining whether the mouse is above the widget.
        bool is_hovering = false;
        /// whether a drag is currently active
        bool is_pressed = false;
        /// whether the drag operation is being modified
        bool is_modified = false;
        /// the current position of the mouse pointer during the drag
        Vec position = {0, 0};
    } mouse_state;

    /// The pixels being rendered on the display.
    std::vector<uint8_t> pixels;

    /// a pointer to the image to draw the display to
    int screen = -1;

    /// @brief Return the normalized position of the mouse.
    Vec get_mouse_position() {
        Vec position = mouse_state.position;
        // calculate the normalized x,y positions in [0, 1]. Account for
        // padding to ensure relative position corresponds to the plot.
        position.x = (position.x - pad_left) / (box.size.x - pad_left - pad_right);
        position.x = Math::clip(position.x, 0.f, 1.f);
        // y axis increases downward in pixel space, so invert about 1.
        position.y = 1.f - (position.y - pad_top) / (box.size.y - pad_top - pad_bottom);
        position.y = Math::clip(position.y, 0.f, 1.f);
        return position;
    }

    /// @brief Return the minimum frequency to render on the x axis.
    inline float get_low_frequency() {
        if (module == nullptr) return 0;
        return module->get_low_frequency();
    }

    /// @brief Return the maximum frequency to render on the x axis.
    inline float get_high_frequency() {
        if (module == nullptr) return APP->engine->getSampleRate() / 2;
        return module->get_high_frequency();
    }

 public:
    explicit SpectralImageDisplay(Spectrogram* module_) :
        TransparentWidget(), module(module_) { }

    // -----------------------------------------------------------------------
    // MARK: Interactivity
    // -----------------------------------------------------------------------

    /// @brief Respond to the mouse entering the widget.
    void onEnter(const EnterEvent& e) override {
        // Consume the event to prevent it from propagating.
        e.consume(this);
        // Set the hovering state to true.
        mouse_state.is_hovering = true;
    }

    /// @brief Respond to the mouse exiting the widget.
    void onLeave(const LeaveEvent& e) override {
        // Consume the event to prevent it from propagating.
        e.consume(this);
        // Set the hovering state to false.
        mouse_state.is_hovering = false;
    }

    /// @brief Respond to mouse hover events above the widget.
    void onHover(const HoverEvent& e) override {
        // Consume the event to prevent it from propagating.
        e.consume(this);
        // Set the mouse state to the hover position.
        mouse_state.position = e.pos;
    }

    // /// @brief Respond to mouse scroll events while hovering above the widget.
    // void onHoverScroll(const HoverScrollEvent& e) override {
    //     // Consume the event to prevent it from propagating.
    //     e.consume(this);
    // }

    // /// Respond to a button event on this widget.
    // void onButton(const event::Button &e) override {
    //     // Consume the event to prevent it from propagating.
    //     e.consume(this);
    //     // Set the mouse state to the hover position.
    //     mouse_state.position = e.pos;
    //     // setup the drag state.
    //     mouse_state.is_modified = e.mods & GLFW_MOD_CONTROL;
    //     // if the action is a press copy the waveform before updating
    //     mouse_state.is_pressed = e.action == GLFW_PRESS&& e.button == GLFW_MOUSE_BUTTON_LEFT;
    //     // Handle right clicks.
    //     if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT)
    //         dynamic_cast<ModuleWidget*>(parent)->createContextMenu();
    // }

    // /// @brief Respond to drag start event on this widget.
    // void onDragStart(const event::DragStart &e) override {
    //     // // lock the cursor so it does not move in the engine during the edit
    //     // APP->window->cursorLock();
    //     // consume the event to prevent it from propagating
    //     e.consume(this);
    // }

    // /// @brief Respond to drag move event on this widget.
    // void onDragMove(const event::DragMove &e) override {
    //     // consume the event to prevent it from propagating
    //     e.consume(this);
    //     // if the drag operation is not active, return early
    //     if (!mouse_state.is_pressed) return;
    //     // update the drag state based on the change in position from the mouse
    //     mouse_state.position.x += e.mouseDelta.x / APP->scene->rackScroll->zoomWidget->zoom;
    //     mouse_state.position.y += e.mouseDelta.y / APP->scene->rackScroll->zoomWidget->zoom;
    // }

    // /// @brief Respond to drag end event on this widget.
    // void onDragEnd(const event::DragEnd &e) override {
    //     // // unlock the cursor to return it to its normal state
    //     // APP->window->cursorUnlock();
    //     // consume the event to prevent it from propagating
    //     e.consume(this);
    //     if (!mouse_state.is_pressed) return;
    //     // disable the press state.
    //     mouse_state.is_pressed = false;
    // }

    // -----------------------------------------------------------------------
    // MARK: Rendering
    // -----------------------------------------------------------------------

    /// @brief Draw the Y ticks with a linear scale.
    /// @param args the arguments for the current draw call
    void draw_y_ticks_linear(const DrawArgs& args) {
        static constexpr float xticks = 10;
        for (float i = 1; i < xticks; i++) {
            // Determine the relative position and re-scale it to the pixel
            // location on-screen. Since we're drawing a static number of
            // points, the position doesn't change relative to the minimum or
            // maximum frequencies (only the label value will change.)
            float position = i / xticks;
            float point_y = rescale(position, 1.f, 0.f, pad_top, box.size.y - pad_bottom);
            // Render tick marker
            // nvgBeginPath(args.vg);
            // nvgMoveTo(args.vg, pad_left, point_y);
            // nvgLineTo(args.vg, box.size.x - pad_right, point_y);
            // nvgStrokeWidth(args.vg, axis_stroke_width);
            // nvgStrokeColor(args.vg, axis_stroke_color);
            // nvgStroke(args.vg);
            // nvgClosePath(args.vg);
            // Render tick label
            float freq = get_low_frequency() + (get_high_frequency() - get_low_frequency()) * position;
            const auto freq_string = Math::freq_to_string(freq);
            nvgFontSize(args.vg, axis_font_size);
            nvgFillColor(args.vg, axis_font_color);
            nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, pad_left - 3 * axis_stroke_width, point_y, freq_string.c_str(), NULL);
        }
    }

    /// @brief Draw the Y ticks with an exponential scale.
    /// @param args the arguments for the current draw call
    void draw_y_ticks_logarithmic(const DrawArgs& args) {
        // Use the spectrogram image height (number of vertical pixels)
        const int height = module->get_coefficients()[0].size() / 2;
        const float nyquist_rate = module->get_sample_rate() / 2.f;
        // Compute the mapping parameters using the same transformation as draw_spectrogram.
        // These define the portion of the texture that is used for the desired frequency range.
        float texture_y_low  = height * (1 - sqrt(get_low_frequency() / nyquist_rate));
        float texture_y_high = height * (1 - sqrt(get_high_frequency() / nyquist_rate));
        float image_section_height = texture_y_low - texture_y_high;
        float draw_height = box.size.y - pad_top - pad_bottom;
        float scale_y = draw_height / image_section_height;
        // Determine the frequency range in the logarithmic domain.
        const auto min_exponent = log10(fmax(100.f, get_low_frequency()));
        const auto max_exponent = log10(get_high_frequency());
        // Iterate over base frequencies (exponential steps).
        for (float exponent = min_exponent; exponent < max_exponent; exponent++) {
            float base_frequency = powf(10.f, exponent);
            // Compute the texture coordinate for this frequency using the same sqrt mapping.
            float t = height * (1 - sqrt(base_frequency / nyquist_rate));
            // Apply the same translation and scaling as used in draw_spectrogram.
            float point_y = pad_top + (t - texture_y_high) * scale_y;
            // Render the tick label.
            const auto freq_string = Math::freq_to_string(base_frequency);
            nvgFontSize(args.vg, axis_font_size);
            nvgFillColor(args.vg, axis_font_color);
            nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, pad_left - 3 * axis_stroke_width, point_y, freq_string.c_str(), NULL);
        }
    }

    /// @brief Draw the spectrogram.
    /// @param args the arguments for the current draw call
    void draw_spectrogram(const DrawArgs& args) {
        // The reference frequency for the slope compensation.
        static constexpr float reference_frequency = 1000.f;
        const auto slope = module->get_slope();
        // Determine the Nyquist rate from the sample rate.
        const float nyquist_rate = module->get_sample_rate() / 2.f;
        // Determine the dimensions of the spectral image.
        const int width = module->get_coefficients().size();
        const int height = module->get_coefficients()[0].size() / 2;

        // Update the pixel buffer based on the spectrogram dimensions.
        pixels.resize(height * width * 4);
        for (int y = 0; y < height; y++) {
            // Compute the gain based on the octave offset.
            auto gain = log2f((y / static_cast<float>(height)) * nyquist_rate / reference_frequency + std::numeric_limits<float>::epsilon());
            gain = Math::decibels2amplitude(slope * gain);
            for (int x = 0; x < width; x++) {
                float scaled_y = y;
                if (module->get_frequency_scale() == FrequencyScale::Logarithmic)
                    scaled_y = height * Math::squared(scaled_y / height);
                auto coeff = gain * Math::interpolate_coefficients(module->get_coefficients()[x], scaled_y);
                auto color = Math::ColorMap::color_map(module->color_map, abs(coeff) / height);
                int index = 4 * (width * (height - 1 - y) + x);
                pixels[index + 0] = color.r * 255;
                pixels[index + 1] = color.g * 255;
                pixels[index + 2] = color.b * 255;
                pixels[index + 3] = 255;
            }
        }

        // Create or update the image container.
        if (screen == -1)
            screen = nvgCreateImageRGBA(args.vg, width, height, 0, pixels.data());
        else
            nvgUpdateImage(args.vg, screen, pixels.data());

        // Compute the mask rectangle from the padded region.
        const Rect mask = Rect(
            Vec(pad_left, pad_top),
            box.size.minus(Vec(pad_left + pad_right, pad_top + pad_bottom))
        );

        // Compute transformation parameters based on frequency bounds
        float texture_y_low, texture_y_high;
        if (module->get_frequency_scale() == FrequencyScale::Logarithmic) {
            texture_y_low  = height * (1 - sqrt(get_low_frequency() / nyquist_rate));
            texture_y_high = height * (1 - sqrt(get_high_frequency() / nyquist_rate));
        } else {
            texture_y_low  = height * (1 - get_low_frequency() / nyquist_rate);
            texture_y_high = height * (1 - get_high_frequency() / nyquist_rate);
        }
        float image_section_height = texture_y_low - texture_y_high;
        float scale_y = mask.size.y / image_section_height;

        // Draw the spectrogram image within the mask
        nvgSave(args.vg);
        nvgScissor(args.vg, mask.pos.x, mask.pos.y, mask.size.x, mask.size.y);
        nvgSave(args.vg);
        // Translate so that the texture coordinate corresponding to the high frequency maps to mask.pos.y.
        nvgTranslate(args.vg, 0, mask.pos.y - texture_y_high * scale_y);
        // Scale vertically so that the selected frequency band fills the mask.
        nvgScale(args.vg, 1.0f, scale_y);
        nvgBeginPath(args.vg);
        // Draw the spectrogram image using the mask's x position and width.
        nvgRect(args.vg, mask.pos.x, 0, mask.size.x, height);
        nvgFillPaint(args.vg, nvgImagePattern(args.vg, mask.pos.x, 0, mask.size.x, height, 0, screen, 1.0));
        nvgFill(args.vg);
        nvgRestore(args.vg);
        nvgResetScissor(args.vg);
        nvgRestore(args.vg);

        // Draw a scan-line to indicate the current hop index.
        nvgBeginPath(args.vg);
        float scan_x = module->get_hop_index() / static_cast<float>(width);
        nvgMoveTo(args.vg, mask.pos.x + scan_x * mask.size.x, mask.pos.y);
        nvgLineTo(args.vg, mask.pos.x + scan_x * mask.size.x, mask.pos.y + mask.size.y);
        nvgStrokeWidth(args.vg, axis_stroke_width);
        nvgStrokeColor(args.vg, axis_stroke_color);
        nvgStroke(args.vg);
        nvgClosePath(args.vg);
    }

    /// @brief Draw the mouse position cross-hair.
    /// @param args the arguments for the current draw call.
    void draw_cross_hair(const DrawArgs& args) {
        const auto mouse_position = get_mouse_position();
        // Convert normalized mouse y (0 = bottom, 1 = top) to a pixel coordinate.
        float y_pixels = rescale(mouse_position.y, 0, 1, box.size.y - pad_bottom, pad_top);
        // Draw the horizontal cross-hair.
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, pad_left, y_pixels);
        nvgLineTo(args.vg, box.size.x - pad_right, y_pixels);
        nvgStrokeWidth(args.vg, 0.5);
        nvgStrokeColor(args.vg, cross_hair_stroke_color);
        nvgStroke(args.vg);
        nvgClosePath(args.vg);
        // Draw the vertical cross-hair (always a linear mapping).
        float x_position = rescale(mouse_position.x, 0, 1, pad_left, box.size.x - pad_right);
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, x_position, pad_top);
        nvgLineTo(args.vg, x_position, box.size.y - pad_bottom);
        nvgStrokeWidth(args.vg, 0.5);
        nvgStrokeColor(args.vg, cross_hair_stroke_color);
        nvgStroke(args.vg);
        nvgClosePath(args.vg);
    }

    /// @brief Draw the cross-hair information as text.
    /// @param args the arguments for the current draw call.
    void draw_cross_hair_text(const DrawArgs& args) {
        const auto mouse_position = get_mouse_position();
        // Convert the mouse's normalized Y to a pixel coordinate.
        float y_pixels = rescale(mouse_position.y, 0, 1, box.size.y - pad_bottom, pad_top);
        float hover_freq = 0;

        if (module->get_frequency_scale() == FrequencyScale::Logarithmic) {
            // 'texHeight' is the height of the spectrogram texture.
            const int texHeight = module->get_coefficients()[0].size() / 2;
            const float nyquist = module->get_sample_rate() / 2.f;
            // Map the low/high frequency to texture coordinates using the square-root mapping.
            // (Flipping vertically: low frequency is at the bottom, high frequency at the top.)
            float texY_low  = texHeight * sqrt(get_low_frequency() / nyquist);
            float texY_high = texHeight * sqrt(get_high_frequency() / nyquist);
            // Compute the vertical scale factor from texture to screen.
            float scaleY = (box.size.y - pad_top - pad_bottom) / (texY_low - texY_high);
            float t = texY_high + (y_pixels - pad_top) / scaleY;
            hover_freq = nyquist * powf(t / texHeight, 2);
        } else {
            // Linear mapping.
            hover_freq = get_low_frequency() + (get_high_frequency() - get_low_frequency()) * mouse_position.y;
        }

        auto font_path = asset::plugin(plugin_instance, "res/Font/Arial/Bold.ttf");
        const std::shared_ptr<Font> font = APP->window->loadFont(font_path);
        nvgFontSize(args.vg, 9);
        nvgFontFaceId(args.vg, font->handle);
        nvgFillColor(args.vg, {{{0.f / 255.f, 90.f / 255.f, 11.f / 255.f, 1.f}}});
        nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_LEFT);

        // Render the hovered frequency at the top left.
        const auto freq_string = Math::freq_to_string(hover_freq);
        nvgText(args.vg, pad_left + 3, pad_top / 2, freq_string.c_str(), NULL);

        // Optionally, also render musical note information.
        if (hover_freq > 0) {
            MusicTheory::TunedNote note(hover_freq);
            nvgText(args.vg, pad_left + 55, pad_top / 2, note.note_string().c_str(), NULL);
            nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_RIGHT);
            nvgText(args.vg, pad_left + 140, pad_top / 2, note.tuning_string().c_str(), NULL);
        }

        // Render the coefficient magnitude.
        // Map normalized coordinates to coefficient indices.
        int coeff_x = mouse_position.x * (module->get_coefficients().size() - 1);
        int coeff_y = module->get_coefficients()[0].size() * hover_freq / module->get_sample_rate();
        // Retrieve the coefficient, compute its magnitude in dB.
        float coeff_value = abs(module->get_coefficients()[coeff_x][coeff_y]);
        float db = Math::amplitude2decibels(coeff_value) - 60.f;
        // Format and render the decibel value.
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << db << " dB";
        nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_RIGHT);
        nvgText(args.vg, box.size.x - pad_right - 3, pad_top / 2, oss.str().c_str(), NULL);
    }

    /// @brief Draw the display on the main context.
    /// @param args the arguments for the draw context for this widget
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1) {  // draw regardless of brightness settings.
            // Background
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, corner_radius);
            nvgFillColor(args.vg, background_color);
            nvgFill(args.vg);
            nvgStrokeColor(args.vg, axis_stroke_color);
            nvgStroke(args.vg);
            nvgClosePath(args.vg);
            // Spectrogram plot
            if (module != nullptr) {
                // draw ticks for the axes of the plot.
                switch (module->get_frequency_scale()) {
                case FrequencyScale::Linear:
                    draw_y_ticks_linear(args);
                    break;
                case FrequencyScale::Logarithmic:
                    draw_y_ticks_logarithmic(args);
                    break;
                default:
                    throw std::runtime_error("Invalid frequency scale");
                }
                draw_spectrogram(args);
                // Interactive mouse hovering functionality.
                if (mouse_state.is_hovering) {
                    draw_cross_hair(args);
                    draw_cross_hair_text(args);
                }
            }
            // Border
            nvgBeginPath(args.vg);
            nvgRect(args.vg, pad_left, pad_top, box.size.x - pad_left - pad_right, box.size.y - pad_top - pad_bottom);
            nvgStrokeWidth(args.vg, axis_stroke_width);
            nvgStrokeColor(args.vg, axis_stroke_color);
            nvgStroke(args.vg);
            nvgClosePath(args.vg);
        }
        Widget::drawLayer(args, layer);
    }
};

struct SpectrogramWidget : ModuleWidget {
    explicit SpectrogramWidget(Spectrogram* module) : ModuleWidget() {
        setModule(module);
        setPanel(createPanel(
            asset::plugin(plugin_instance, "res/Spectrogram-Light.svg"),
            asset::plugin(plugin_instance, "res/Spectrogram-Dark.svg")
        ));
        // Spectrogram display
        SpectralImageDisplay* display = new SpectralImageDisplay(module);
        display->setPosition(Vec(45, 15));
        display->setSize(Vec(465, 350));
        addChild(display);
        // Inputs
        addInput(createInput<PJ301MPort>(Vec(11, 30), module, Spectrogram::INPUT_SIGNAL));
        addParam(createParam<Trimpot>(Vec(13, 66), module, Spectrogram::PARAM_INPUT_GAIN));
        // Buttons.
        addParam(createParamCentered<PB61303>(Vec(8 + 15, 331 + 15), module, Spectrogram::PARAM_RUN));
        addChild(createLightCentered<PB61303Light<WhiteLight>>(Vec(8 + 15, 331 + 15), module, Spectrogram::LIGHT_RUN));
        // Screen controls.
        // Window function control with custom angles to match discrete range.
        auto window_function_param = createParam<TextKnob>(Vec(50 + 0 * 66, 330), module, Spectrogram::PARAM_WINDOW_FUNCTION);
        window_function_param->label.text = "WINDOW";
        window_function_param->maxAngle = 2.f * M_PI;
        addParam(window_function_param);
        // Frequency scale control with custom angles to match discrete range.
        auto frequency_scale_param = createParam<TextKnob>(Vec(50 + 1 * 66, 330), module, Spectrogram::PARAM_FREQUENCY_SCALE);
        frequency_scale_param->maxAngle = 0.3 * M_PI;
        frequency_scale_param->label.text = "Y SCALE";
        addParam(frequency_scale_param);
        // Time smoothing control.
        auto time_smoothing_param = createParam<TextKnob>(Vec(50 + 2 * 66, 330), module, Spectrogram::PARAM_TIME_SMOOTHING);
        time_smoothing_param->label.text = "AVERAGE";
        addParam(time_smoothing_param);
        // Frequency smoothing control with custom angles to match discrete range.
        auto frequency_smoothing_param = createParam<TextKnob>(Vec(50 + 3 * 66, 330), module, Spectrogram::PARAM_FREQUENCY_SMOOTHING);
        frequency_smoothing_param->label.text = "SMOOTH";
        frequency_smoothing_param->maxAngle = 2.f * M_PI;
        addParam(frequency_smoothing_param);
        // Low and High frequency (frequency range) controls.
        auto low_freq_param = createParam<TextKnob>(Vec(50 + 4 * 66, 330), module, Spectrogram::PARAM_LOW_FREQUENCY);
        low_freq_param->label.text = "LO FREQ";
        addParam(low_freq_param);
        auto high_freq_param = createParam<TextKnob>(Vec(50 + 5 * 66, 330), module, Spectrogram::PARAM_HIGH_FREQUENCY);
        high_freq_param->label.text = "HI FREQ";
        addParam(high_freq_param);
        // Slope (dB/octave @1000Hz) controls.
        auto slope_param = createParam<TextKnob>(Vec(50 + 6 * 66, 330), module, Spectrogram::PARAM_SLOPE);
        slope_param->label.text = "SLOPE";
        addParam(slope_param);
        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    }

    /// @brief Append the context menu to the module when right clicked.
    /// @param menu the menu object to add context items for the module to
    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Render Settings"));
        menu->addChild(createBoolPtrMenuItem("AC-coupled", "", &getModule<Spectrogram>()->is_ac_coupled));
        menu->addChild(createIndexPtrSubmenuItem("Color Map", Math::ColorMap::names(), reinterpret_cast<int*>(&getModule<Spectrogram>()->color_map)));
        ModuleWidget::appendContextMenu(menu);
    }
};

Model* modelSpectrogram = createModel<Spectrogram, SpectrogramWidget>("Spectrogram");

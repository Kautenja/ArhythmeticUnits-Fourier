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
#include "./dsp/dc_blocker.hpp"
#include "./plugin.hpp"
#include "./text_knob.hpp"

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
    Math::OnTheFlyRFFT fft;

    /// A copy of the low-pass filtered coefficients.
    Math::DFTCoefficients filtered_coefficients;

    /// A buffer for storing the DFT coefficients of x[t-N], ..., x[t]
    Math::STFTCoefficients coefficients;

    /// a clock divider for updating the lights every 512 frames
    Trigger::Divider light_divider;

    /// a Schmitt Trigger for handling presses on the clock button
    Trigger::Threshold<float> run_trigger;

    /// Whether the analyzer is running or not.
    bool is_running = true;

    /// The index of the current STFT hop.
    uint32_t hop_index = 0;

 public:
    /// Whether to apply AC coupling to input signal.
    bool is_ac_coupled = true;

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
        configParam<FrequencyScaleParamQuantity>(PARAM_FREQUENCY_SCALE, 0, 1, 1, "Y Scale");
        getParamQuantity(PARAM_FREQUENCY_SCALE)->snapEnabled = true;
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
        configParam<FrequencySmoothingParamQuantity>(PARAM_FREQUENCY_SMOOTHING, 0, static_cast<float>(FrequencySmoothing::NumOptions) - 1, 0, "Smooth");
        getParamQuantity(PARAM_FREQUENCY_SMOOTHING)->snapEnabled = true;
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
        rack::Module::onReset();
        // Reset momentary button trigger states.
        is_running = true;
        // Reset the hop index.
        hop_index = 0;
        // Reset hidden menu options.
        is_ac_coupled = true;
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
        rack::Module::onSampleRateChange();
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
        getParamQuantity(PARAM_HIGH_FREQUENCY)->maxValue = sample_rate / 2.f;
        set_high_frequency(high_frequency);
        // Set the transition width of DC-blocking filters for AC-coupled mode.
        dc_blocker.setTransitionWidth(10.f, sample_rate);
        dc_blocker.reset();
    }

    /// @brief Convert the module's state to a JSON object.
    ///
    /// @returns a pointer to a new json_t object with the module's state
    ///
    inline json_t* dataToJson() final {
        json_t* rootJ = json_object();
        JSON::set<bool>(rootJ, "is_running", is_running);
        JSON::set<bool>(rootJ, "is_ac_coupled", is_ac_coupled);
        return rootJ;
    }

    /// @brief Load the module's state from a JSON object.
    ///
    /// @param rootJ a pointer to a json_t with state data for this module
    ///
    inline void dataFromJson(json_t* rootJ) final {
        JSON::get<bool>(rootJ, "is_running", [&](const bool& value) { is_running = value; });
        JSON::get<bool>(rootJ, "is_ac_coupled", [&](const bool& value) { is_ac_coupled = value; });
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
    inline static constexpr size_t get_hop_length() {
        return N_FFT >> 1;  // N_FFT / 2
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
    /// @details
    /// Applies gain to each input signal and buffers it for DFT computation.
    inline void process_input_signal() {
        // Get the input signal and convert to normalized bipolar [-1, 1].
        const auto signal = Math::Eurorack::fromAC(inputs[INPUT_SIGNAL].getVoltageSum());
        // Determine the gain to apply to this channel's input signal.
        const auto gain = params[PARAM_INPUT_GAIN].getValue();
        // Pass signal through the DC blocking filter. Do this regardless
        // of whether we are in AC-coupling mode to ensure when switching
        // between modes there is no graphical delay from the filter
        // accumulating signal data.
        dc_blocker.process(signal);
        // Insert the normalized and processed input signal into the delay.
        delay.insert(gain * (is_ac_coupled ? dc_blocker.getValue() : signal));
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
    ///
    /// @param args the sample arguments (sample rate, sample time, etc.)
    ///
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
struct SpectralImageDisplay : rack::TransparentWidget {
 private:
    /// The vertical (top) padding for the plot.
    static constexpr size_t pad_top = 20;
    /// The vertical (bottom) padding for the plot.
    static constexpr size_t pad_bottom = 50;
    /// The horizontal (left) padding for the plot.
    static constexpr size_t pad_left = 30;
    /// The horizontal (right) padding for the plot.
    static constexpr size_t pad_right = 5;
    /// The radius of the rounded corners of the screen
    static constexpr int corner_radius = 5;
    /// The background color of the screen
    static constexpr NVGcolor background_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    /// The stroke color for the axis lines
    static constexpr NVGcolor axis_stroke_color = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
    /// The width of the lines to render for axes.
    static constexpr float axis_stroke_width = 1;
    /// The font color for the axis text.
    static constexpr NVGcolor axis_font_color = {{{1.0f, 1.0f, 1.0f, 1.0f}}};
    /// The font size for the axis text.
    static constexpr float axis_font_size = 8;
    /// The stroke color for the cross-hair
    static constexpr NVGcolor cross_hair_stroke_color = {{{0.2f, 0.2f, 0.2f, 1.0f}}};

    /// the font for rendering text on the display
    const std::shared_ptr<Font> font = APP->window->loadFont(
        rack::asset::plugin(plugin_instance, "res/Font/Arial/Bold.ttf")
    );

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
        rack::Vec position = {0, 0};
    } mouse_state;

    /// a pointer to the image to draw the display to
    int screen = -1;

    /// @brief Return the normalized position of the mouse.
    rack::Vec get_mouse_position() {
        rack::Vec position = mouse_state.position;
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

    /// Respond to a button event on this widget.
    void onButton(const rack::event::Button &e) override {
        // Consume the event to prevent it from propagating.
        e.consume(this);
        // Set the mouse state to the hover position.
        mouse_state.position = e.pos;
        // setup the drag state.
        mouse_state.is_modified = e.mods & GLFW_MOD_CONTROL;
        // if the action is a press copy the waveform before updating
        mouse_state.is_pressed = e.action == GLFW_PRESS&& e.button == GLFW_MOUSE_BUTTON_LEFT;
        // Handle right clicks.
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT)
            dynamic_cast<ModuleWidget*>(parent)->createContextMenu();
    }

    /// @brief Respond to drag start event on this widget.
    void onDragStart(const rack::event::DragStart &e) override {
        // // lock the cursor so it does not move in the engine during the edit
        // APP->window->cursorLock();
        // consume the event to prevent it from propagating
        e.consume(this);
    }

    /// @brief Respond to drag move event on this widget.
    void onDragMove(const rack::event::DragMove &e) override {
        // consume the event to prevent it from propagating
        e.consume(this);
        // if the drag operation is not active, return early
        if (!mouse_state.is_pressed) return;
        // update the drag state based on the change in position from the mouse
        mouse_state.position.x += e.mouseDelta.x / APP->scene->rackScroll->zoomWidget->zoom;
        mouse_state.position.y += e.mouseDelta.y / APP->scene->rackScroll->zoomWidget->zoom;
    }

    /// @brief Respond to drag end event on this widget.
    void onDragEnd(const rack::event::DragEnd &e) override {
        // // unlock the cursor to return it to its normal state
        // APP->window->cursorUnlock();
        // consume the event to prevent it from propagating
        e.consume(this);
        if (!mouse_state.is_pressed) return;
        // disable the press state.
        mouse_state.is_pressed = false;
    }

    // -----------------------------------------------------------------------
    // MARK: Rendering
    // -----------------------------------------------------------------------

    /// @brief Draw the Y ticks with a linear scale.
    ///
    /// @param args the arguments for the current draw call
    ///
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
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, pad_left, point_y);
            nvgLineTo(args.vg, box.size.x - pad_right, point_y);
            nvgStrokeWidth(args.vg, axis_stroke_width);
            nvgStrokeColor(args.vg, axis_stroke_color);
            nvgStroke(args.vg);
            nvgClosePath(args.vg);
            // Render tick label
            float freq = get_low_frequency() + (get_high_frequency() - get_low_frequency()) * position;
            std::stringstream stream;
            if (freq < 1000.f)
                stream << std::fixed << std::setprecision(0) << freq << "Hz";
            else
                stream << std::fixed << std::setprecision(1) << freq / 1000.f << "kHz";
            nvgFontSize(args.vg, axis_font_size);
            nvgFillColor(args.vg, axis_font_color);
            nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, pad_left - 2 * axis_stroke_width, point_y, stream.str().c_str(), NULL);
        }
    }

    /// @brief Draw the Y ticks with an exponential scale.
    ///
    /// @param args the arguments for the current draw call
    ///
    void draw_y_ticks_logarithmic(const DrawArgs& args) {
        // Iterate over frequencies exponentially (base 10) starting
        // at at least 100Hz up to the maximum frequency (at most the Nyquist
        // frequency) I.e., follow an exponential series like 100, 1000, etc.
        const auto min_exponent = log10(fmax(100.f, get_low_frequency()));
        const auto max_exponent = log10(get_high_frequency());
        const auto frequency_range = get_high_frequency() - get_low_frequency();
        for (float exponent = min_exponent; exponent < max_exponent; exponent++) {
            // Iterate over harmonics of the base frequency, i.e., if
            // we're at base 100Hz, iterate over 200Hz, 300Hz, ...
            const float base_frequency = powf(10.f, exponent);
            for (float offset = 1.f; offset < 10.f; offset++) {
                // Scale base frequency to offset to the n'th harmonic.
                const float frequency = base_frequency * offset;
                if (frequency >= get_high_frequency()) break;
                nvgBeginPath(args.vg);
                // Re-scale the frequency to a pixel location and render.
                const auto position = sqrt((frequency - get_low_frequency()) / frequency_range);
                nvgMoveTo(args.vg, pad_left, rescale(position, 1.f, 0.f, pad_top, box.size.y - pad_bottom));
                nvgLineTo(args.vg, box.size.x - pad_right, rescale(position, 1.f, 0.f, pad_top, box.size.y - pad_bottom));
                nvgStrokeWidth(args.vg, axis_stroke_width);
                nvgStrokeColor(args.vg, axis_stroke_color);
                nvgStroke(args.vg);
                nvgClosePath(args.vg);
            }
            // Render a label with the base frequency in kHz.
            std::stringstream stream;
            if (base_frequency < 1000.f)
                stream << std::fixed << std::setprecision(0) << base_frequency << "Hz";
            else
                stream << std::fixed << std::setprecision(0) << base_frequency / 1000.f << "kHz";
            nvgFontSize(args.vg, axis_font_size);
            nvgFillColor(args.vg, axis_font_color);
            nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, pad_left - 2 * axis_stroke_width, rescale(sqrt((base_frequency - get_low_frequency()) / frequency_range), 1.f, 0.f, pad_top, box.size.y - pad_bottom), stream.str().c_str(), NULL);
        }
    }

    /// @brief Draw the spectrogram.
    ///
    /// @param args the arguments for the current draw call
    ///
    void draw_spectrogram(const DrawArgs& args) {
        // The reference frequency for the slope compensation.
        static constexpr float reference_frequency = 1000.f;
        const auto slope = module->get_slope();
        // Determine the Nyquist rate from the sample rate.
        const float nyquist_rate = APP->engine->getSampleRate() / 2.f;
        // A small constant for numerical stability.
        static constexpr float epsilon = 1e-6f;
        // Determine the dimensions of the spectral image.
        const int width = module->get_coefficients().size();
        const int height = module->get_coefficients()[0].size() / 2;
        // Create a pixel buffer from the spectral image in RGBA8888 format.
        uint8_t pixels[height * width * 4];
        for (int y = 0; y < height; y++) {
            // Determine the y-scale from the frequency. The slope is provided
            // in decibels/octave, so first determine the octave offset from
            // the current frequency using, e.g., 1000Hz as the reference
            // frequency for the curve. I.e., because octaves are logarithmic,
            // we can simply compute \f$\log2(f_i / f_{reference})\f$ and
            // multiply by the slope. Because we're dealing with y first in
            // terms of amplitude, also convert the decibel scaling to an
            // amplitude gain.
            auto gain = log2f((y / (float) height) * nyquist_rate / reference_frequency + epsilon);
            gain = Math::decibels2amplitude(slope * gain);
            for (int x = 0; x < width; x++) {
                float scaled_y = y;
                if (module->get_frequency_scale() == FrequencyScale::Logarithmic)
                    scaled_y = height * Math::squared(scaled_y / height);
                auto coeff = gain * interpolate_coefficients(module->get_coefficients()[x], scaled_y);
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
        // screen from STFT pixels.
        nvgBeginPath(args.vg);
        nvgRect(args.vg, pad_left, pad_top, box.size.x - pad_left - pad_right, box.size.y - pad_top - pad_bottom);
        nvgFillPaint(args.vg, nvgImagePattern(args.vg, pad_left, pad_top, box.size.x - pad_left - pad_right, box.size.y - pad_top - pad_bottom, 0, screen, 1.0));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, axis_stroke_width);
        nvgStrokeColor(args.vg, axis_stroke_color);
        nvgStroke(args.vg);
        nvgClosePath(args.vg);
        // scan-line from hop index.
        nvgBeginPath(args.vg);
        float scan_x = module->get_hop_index() / static_cast<float>(width);
        nvgMoveTo(args.vg, pad_left + scan_x * (box.size.x - pad_left - pad_right), pad_top);
        nvgLineTo(args.vg, pad_left + scan_x * (box.size.x - pad_left - pad_right), box.size.y - pad_bottom);
        nvgStrokeWidth(args.vg, axis_stroke_width);
        nvgStrokeColor(args.vg, axis_stroke_color);
        nvgStroke(args.vg);
        nvgClosePath(args.vg);
    }

    /// @brief Draw the mouse position cross-hair.
    /// @param args the arguments for the current draw call.
    void draw_cross_hair(const DrawArgs& args) {
        const auto mouse_position = get_mouse_position();
        // Render the cross-hair row.
        const float y_position = rescale(mouse_position.y, 0, 1, box.size.y - pad_bottom, pad_top);
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, pad_left, y_position);
        nvgLineTo(args.vg, box.size.x - pad_right, y_position);
        nvgStrokeWidth(args.vg, 0.5);
        nvgStrokeColor(args.vg, cross_hair_stroke_color);
        nvgStroke(args.vg);
        nvgClosePath(args.vg);
        // Render the cross-hair column.
        const float x_position = rescale(mouse_position.x, 0, 1, pad_left, box.size.x - pad_right);
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
        // Determine the frequency, frequency bin, etc. based on x.
        float hover_freq = 0;
        switch (module->get_frequency_scale()) {
        case FrequencyScale::Linear:
            hover_freq = get_low_frequency() + (get_high_frequency() - get_low_frequency()) * mouse_position.y;
            break;
        case FrequencyScale::Logarithmic:
            hover_freq = (get_high_frequency() - get_low_frequency()) * Math::squared(mouse_position.y) + get_low_frequency();
            break;
        default: break;  // TODO: raise error
        }
        nvgFontSize(args.vg, 9);
        nvgFontFaceId(args.vg, font->handle);
        nvgFillColor(args.vg, {{{0.f / 255.f, 90.f / 255.f, 11.f / 255.f, 1.f}}});
        nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_LEFT);
        // Convert the hovered frequency to a string representation.
        std::ostringstream stream;
        if (hover_freq < 1000.f)
            stream << std::fixed << std::setprecision(2) << hover_freq << "Hz";
        else
            stream << std::fixed << std::setprecision(2) << hover_freq / 1000.f << "kHz";
        // Render hovered frequency above the plot in the top left.
        nvgText(args.vg, pad_left + 3, pad_top / 2, stream.str().c_str(), NULL);
        // Convert the frequency to a note.
        if (hover_freq > 0) {
            Math::TunedNote note;
            Math::frequency_to_note(note, hover_freq);
            // Render the note and octave
            stream = {};
            stream << Math::to_string(note.note) << note.octave;
            nvgText(args.vg, pad_left + 55, pad_top / 2, stream.str().c_str(), NULL);
            // Render the tuning measured in cents.
            stream = {};
            stream << (note.cents >= 0 ? "+" : "");
            stream << std::fixed << std::setprecision(2) << note.cents << " cents";
            nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_RIGHT);
            nvgText(args.vg, pad_left + 140, pad_top / 2, stream.str().c_str(), NULL);
        }
        // TODO:
        // Render the coefficient magnitude.
        // const auto x = static_cast<int>(mouse_position.x * module->get_coefficients().size());
        // const auto y = static_cast<int>(mouse_position.y * module->get_coefficients()[0].size() / 2);
        // const auto magnitude = module->get_coefficients()[x][y];
        // std::cout << Math::amplitude2decibels(magnitude) << std::endl;
        // // // Render the y position.
        // stream = {};
        // // switch (module->get_magnitude_scale()) {
        // // case MagnitudeScale::Linear:
        // //     stream << std::fixed << std::setprecision(2) << mouse_position.y * 4 * 100 << "%";
        // //     break;
        // // case MagnitudeScale::Logarithmic60dB:
        // //     stream << std::fixed << std::setprecision(2) << rescale(mouse_position.y, 0, 1, -60, 12) << "dB";
        // //     break;
        // // case MagnitudeScale::Logarithmic120dB:
        // //     stream << std::fixed << std::setprecision(2) << rescale(mouse_position.y, 0, 1, -120, 12) << "dB";
        // //     break;
        // // default: break;  // TODO: raise error
        // // }
        // nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_RIGHT);
        // nvgText(args.vg, box.size.x - pad_right - 3, pad_top / 2, stream.str().c_str(), NULL);
    }

    /// @brief Draw the display on the main context.
    ///
    /// @param args the arguments for the draw context for this widget
    ///
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1) {  // draw regardless of brightness settings.
            // // Draw the background.
            // nvgBeginPath(args.vg);
            // nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, corner_radius);
            // nvgFillColor(args.vg, background_color);
            // nvgFill(args.vg);
            // nvgClosePath(args.vg);
            // Draw the background.
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, corner_radius);
            nvgFillColor(args.vg, background_color);
            nvgFill(args.vg);
            nvgStrokeColor(args.vg, axis_stroke_color);
            nvgStroke(args.vg);
            nvgClosePath(args.vg);
            // draw ticks for the axes of the plot.
            switch (module == nullptr ? FrequencyScale::Logarithmic : module->get_frequency_scale()) {
            case FrequencyScale::Linear:
                draw_y_ticks_linear(args);
                break;
            case FrequencyScale::Logarithmic:
                draw_y_ticks_logarithmic(args);
                break;
            }
            // Draw the spectrogram.
            if (module != nullptr) {
                draw_spectrogram(args);
                // Interactive mouse hovering functionality.
                if (mouse_state.is_hovering) {
                    draw_cross_hair(args);
                    draw_cross_hair_text(args);
                }
            }
            // border
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

/// The base-name for the Spectrogram panel files.
const char BASENAME[] = "res/Spectrogram";

struct SpectrogramWidget : ThemedWidget<BASENAME> {
    explicit SpectrogramWidget(Spectrogram* module) : ThemedWidget<BASENAME>() {
        setModule(module);
        // Spectrogram display
        SpectralImageDisplay* display = new SpectralImageDisplay(static_cast<Spectrogram*>(module));
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
        auto window_function_param = createParam<WindowFunctionTextKnob>(Vec(50 + 0 * 66, 330), module, Spectrogram::PARAM_WINDOW_FUNCTION);
        window_function_param->maxAngle = 2.f * M_PI;
        addParam(window_function_param);
        // Frequency scale control with custom angles to match discrete range.
        auto frequency_scale_param = createParam<FrequencyScaleTextKnob>(Vec(50 + 1 * 66, 330), module, Spectrogram::PARAM_FREQUENCY_SCALE);
        frequency_scale_param->maxAngle = 0.3 * M_PI;
        addParam(frequency_scale_param);
        // Time smoothing control.
        addParam(createParam<TextKnob>(Vec(50 + 2 * 66, 330), module, Spectrogram::PARAM_TIME_SMOOTHING));
        // Frequency smoothing control with custom angles to match discrete range.
        auto frequency_smoothing_param = createParam<FrequencySmoothingTextKnob>(Vec(50 + 3 * 66, 330), module, Spectrogram::PARAM_FREQUENCY_SMOOTHING);
        frequency_smoothing_param->maxAngle = 2.f * M_PI;
        addParam(frequency_smoothing_param);
        // Low and High frequency (frequency range) controls.
        addParam(createParam<TextKnob>(Vec(50 + 4 * 66, 330), module, Spectrogram::PARAM_LOW_FREQUENCY));
        addParam(createParam<TextKnob>(Vec(50 + 5 * 66, 330), module, Spectrogram::PARAM_HIGH_FREQUENCY));
        // Slope (dB/octave @1000Hz) controls.
        addParam(createParam<TextKnob>(Vec(50 + 6 * 66, 330), module, Spectrogram::PARAM_SLOPE));
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
        menu->addChild(new MenuSeparator);
        // get a pointer to the module
        Spectrogram* const module = dynamic_cast<Spectrogram*>(this->module);
        // Create an option for enabling AC-coupled mode.
        auto ac_coupling_item = createMenuItem<FlagMenuItem>("AC-coupled", CHECKMARK(module->is_ac_coupled));
        ac_coupling_item->flag = &module->is_ac_coupled;
        menu->addChild(ac_coupling_item);
        // Call the super function.
        ThemedWidget<BASENAME>::appendContextMenu(menu);
    }
};

Model* modelSpectrogram = createModel<Spectrogram, SpectrogramWidget>("Spectrogram");

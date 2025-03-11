// A spectrum analyzer module.
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

/// @brief A spectrum analyzer module.
struct SpectrumAnalyzer : Module {
 public:
    /// Architectural constants of the module.
    enum Architecture {
        /// The number of processing lanes on the module
        NUM_CHANNELS = 4
    };

    /// Controllable parameters on the module.
    enum ParamIds {
        ENUMS(PARAM_INPUT_GAIN, NUM_CHANNELS),
        PARAM_RUN,
        PARAM_WINDOW_FUNCTION,
        PARAM_WINDOW_LENGTH,
        PARAM_HOP_LENGTH,
        PARAM_FREQUENCY_SCALE,
        PARAM_MAGNITUDE_SCALE,
        PARAM_TIME_SMOOTHING,
        PARAM_FREQUENCY_SMOOTHING,
        PARAM_LOW_FREQUENCY,
        PARAM_HIGH_FREQUENCY,
        PARAM_SLOPE,
        NUM_PARAMS
    };

    /// Input ports on the module.
    enum InputIds {
        ENUMS(INPUT_SIGNAL, NUM_CHANNELS),
        NUM_INPUTS
    };

    /// Output ports on the module.
    enum OutputIds {
        NUM_OUTPUTS
    };

    /// LED lights on the module.
    enum LightIds {
        LIGHT_RUN,  // Running indicator
        NUM_LIGHTS
    };

 private:
    /// The sample rate of the module.
    float sample_rate = 0.f;

    /// DC-blocking filter for AC-coupled mode.
    Filter::DCBlocker<simd::float_4> dc_blocker;

    /// Delay line for tracking the input signal x[t].
    Math::ContiguousCircularBuffer<simd::float_4> delay;

    /// Sampled function for windowing the FFT.
    Math::Window::CachedWindow<float> window_function;

    /// An on-the-fly FFT calculator for each input channel.
    Math::OnTheFlyRFFT<simd::float_4> fft;

    /// A copy of low-pass filtered DFT coefficients.
    std::vector<std::complex<simd::float_4>> filtered_coeffs;

    /// A buffer of rasterized coefficients with \f$(x, y) \in [0, 1)\f$.
    std::vector<Vec> raster_coeffs[NUM_CHANNELS];

    /// A clock divider for updating the lights at a lower sampling rate.
    Trigger::Divider light_divider;

    /// A trigger for handling presses on the "run" button.
    Trigger::Threshold<float> run_trigger;

    /// A flag determining whether the analyzer is running or not.
    bool is_running = true;

 public:
    /// A buffer of rasterized coefficients with \f$(x, y) \in [0, 1)\f$.
    std::vector<Vec> render_coefficients[NUM_CHANNELS];

    /// Whether to fill the plots.
    bool is_fill_enabled = false;

    /// Whether to use Bezier curves.
    bool is_bezier_enabled = true;

    /// Whether to apply AC coupling to input signal.
    bool is_ac_coupled = true;

    /// @brief Initialize a new spectrum analyzer.
    SpectrumAnalyzer() : sample_rate(APP->engine->getSampleRate()) {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        // Setup input signals and gain parameters.
        static constexpr const char* INPUT_NAMES[NUM_CHANNELS] = {"Red", "Green", "Blue", "Yellow"};
        for (std::size_t i = 0; i < NUM_CHANNELS; i++) {
            configParam(PARAM_INPUT_GAIN + i, 0.f, std::pow(10.f, 12.f / 20.f), 1.f, std::string(INPUT_NAMES[i]) + " Gain", "dB", -10, 20);
            configInput(INPUT_SIGNAL + i, INPUT_NAMES[i]);
        }
        // Configure the run button.
        configButton(PARAM_RUN, "Run");
        getParamQuantity(PARAM_RUN)->description =
            "Enables or disables the analyzer. When disabled,\n"
            "the analyzer stops buffering and processing new audio.";
        // Setup the window function as a custom discrete enumeration.
        configParam<WindowFunctionParamQuantity>(PARAM_WINDOW_FUNCTION, 0, static_cast<size_t>(Math::Window::Function::Flattop), static_cast<size_t>(Math::Window::Function::Flattop), "Window");
        getParamQuantity(PARAM_WINDOW_FUNCTION)->snapEnabled = true;
        getParamQuantity(PARAM_WINDOW_FUNCTION)->description =
            "The window function to apply before the FFT. Windowing\n"
            "helps reduce spectral leakage in the frequency domain.";
        // Setup the window length as powers of 2 from 2^7=128 to 2^14=16384
        configParam(PARAM_WINDOW_LENGTH, 7.f, 14.f, 11.f, "Length", "", 2, 1);
        getParamQuantity(PARAM_WINDOW_LENGTH)->snapEnabled = true;
        getParamQuantity(PARAM_WINDOW_LENGTH)->description =
            "The FFT size as a power of two. Larger sizes provide\n"
            "higher spectral resolution but require more computation.";
        // Setup hop length in seconds with millisecond render scaling.
        configParam(PARAM_HOP_LENGTH, 0.005, 0.300, 0.030, "Hop", "ms", 0, 1000);
        getParamQuantity(PARAM_HOP_LENGTH)->displayPrecision = 3;
        getParamQuantity(PARAM_HOP_LENGTH)->description =
            "The hop size for the time-domain segmentation (STFT.)\n"
            "The analyzer computes a new FFT along this period.";
        // Setup the discrete frequency scale selector.
        configParam<FrequencyScaleParamQuantity>(PARAM_FREQUENCY_SCALE, 0, 1, 1, "X Scale");
        getParamQuantity(PARAM_FREQUENCY_SCALE)->snapEnabled = true;
        getParamQuantity(PARAM_FREQUENCY_SCALE)->description =
            "The frequency-axis scale on the display. The DFT spaces\n"
            "frequencies linearly but humans hear frequencies along\n"
            "a logarithmic scale.";
        // Setup the discrete magnitude scale selector.
        configParam<MagnitudeScaleParamQuantity>(PARAM_MAGNITUDE_SCALE, 0, 2, 1, "Y Scale");
        getParamQuantity(PARAM_MAGNITUDE_SCALE)->snapEnabled = true;
        getParamQuantity(PARAM_MAGNITUDE_SCALE)->description =
            "The magnitude scale on the display. The DFT spaces\n"
            "magnitude linearly but humans hear volume along\n"
            "logarithmic scales.";
            // Linear, Logarithmic (60dB range), or Logarithmic (120dB range).";
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
        // Module state initialization.
        onReset();
    }

    /// @brief Respond to the module being reset by the host environment.
    inline void onReset() final {
        Module::onReset();
        // Reset momentary button trigger states.
        is_running = true;
        // Reset hidden menu options.
        is_fill_enabled = false;
        is_bezier_enabled = true;
        is_ac_coupled = true;
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
    ///
    /// @returns a pointer to a new json_t object with the module's state
    ///
    inline json_t* dataToJson() final {
        json_t* rootJ = json_object();
        JSON::set<bool>(rootJ, "is_running", is_running);
        JSON::set<bool>(rootJ, "is_fill_enabled", is_fill_enabled);
        JSON::set<bool>(rootJ, "is_bezier_enabled", is_bezier_enabled);
        JSON::set<bool>(rootJ, "is_ac_coupled", is_ac_coupled);
        return rootJ;
    }

    /// @brief Load the module's state from a JSON object.
    ///
    /// @param rootJ a pointer to a json_t with state data for this module
    ///
    inline void dataFromJson(json_t* rootJ) final {
        JSON::get<bool>(rootJ, "is_running", [&](const bool& value) {
            is_running = value;
        });
        JSON::get<bool>(rootJ, "is_fill_enabled", [&](const bool& value) {
            is_fill_enabled = value;
        });
        JSON::get<bool>(rootJ, "is_bezier_enabled", [&](const bool& value) {
            is_bezier_enabled = value;
        });
        JSON::get<bool>(rootJ, "is_ac_coupled", [&](const bool& value) {
            is_ac_coupled = value;
        });
    }

    // -----------------------------------------------------------------------
    // MARK: Parameters
    // -----------------------------------------------------------------------

    /// @brief Return the current sample rate of the module.
    /// @returns The sample rate of the module.
    inline float get_sample_rate() const { return sample_rate; }

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

    // Window Length

    /// @brief Return the length of the window.
    /// @returns The length of the window measured in samples.
    inline size_t get_window_length() {
        return 1 << static_cast<size_t>(params[PARAM_WINDOW_LENGTH].getValue());
    }

    /// @brief Set the length of the window.
    /// @param value The length of the window measured in samples. Should be
    /// a power of \f$2\f$, e.g., \f$[1, 2, 4, 8, 16, 32, ...]\f$.
    inline void set_window_length(const size_t& value) {
        params[PARAM_WINDOW_LENGTH].setValue(floorf(log2f(value)));
    }

    // Hop Length

    /// @brief Return the hop length of the windowed DFT in samples.
    /// @returns The number of samples to hop between computations of the DFT.
    inline size_t get_hop_length() {
        return params[PARAM_HOP_LENGTH].getValue() * sample_rate;
    }

    /// @brief Set the hop length of the windowed DFT in samples.
    /// @param value Number of samples to hop between computations of the DFT.
    inline void set_hop_length(const size_t& value) {
        return params[PARAM_HOP_LENGTH].setValue(value / sample_rate);
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

    // Magnitude Scale

    /// @brief Set the magnitude scale setting.
    /// @param value The magnitude scale for rendering the Y axis.
    inline MagnitudeScale get_magnitude_scale() {
        const auto value = params[PARAM_MAGNITUDE_SCALE].getValue();
        return static_cast<MagnitudeScale>(value);
    }

    /// @brief Set the magnitude scale setting.
    /// @param value The magnitude scale for rendering the Y axis.
    inline void set_magnitude_scale(const MagnitudeScale& value) {
        params[PARAM_MAGNITUDE_SCALE].setValue(static_cast<float>(value));
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
        const float hop_time = params[PARAM_HOP_LENGTH].getValue();
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

    /// @brief Process the window length and hop length parameters.
    /// @details
    /// Resizes the delay lines and DFT buffers to the length of the window.
    /// Also sets the DFT divider to the length of the hop.
    inline void process_window() {
        // Determine the length of the delay lines and associated FFTs.
        const size_t N = get_window_length();
        if (delay.size() != N) {
            delay.resize(N);
            delay.clear();
        }
        window_function.set_window(get_window_function(), N, false, true);
        if (fft.size() != N)
            fft.resize(N);
        if (filtered_coeffs.size() != N) {
            filtered_coeffs.resize(N);
            std::fill(filtered_coeffs.begin(), filtered_coeffs.end(), 0);
        }
        // Update the rasterized coefficients from the FFT length.
        for (size_t i = 0; i < NUM_CHANNELS; i++) {
            if (raster_coeffs[i].size() == N / 2.f + 1) continue;
            raster_coeffs[i].resize(N / 2.f + 1);
            std::fill(raster_coeffs[i].begin(), raster_coeffs[i].end(), Vec(0.f, 0.f));
            render_coefficients[i] = raster_coeffs[i];
        }
    }

    /// @brief Process presses to the "run" button.
    /// @details
    /// Processes the run parameter with a trigger and flips the `is_running`
    /// flag when it fires.
    inline void process_run_button() {
        if (run_trigger.process(params[PARAM_RUN].getValue()))
            is_running = !is_running;
    }

    /// @brief Process input signals.
    /// @details
    /// Applies gain to each input signal and buffers it for DFT computation.
    inline void process_input_signal() {
        if (!is_running) return;  // Don't buffer input signals if not running.
        // Buffer signals and gains.
        float signals[NUM_CHANNELS] = {0.f, 0.f, 0.f, 0.f};
        float gains[NUM_CHANNELS] = {1.f, 1.f, 1.f, 1.f};
        for (size_t i = 0; i < NUM_CHANNELS; i++) {
            signals[i] = Math::Eurorack::fromAC(inputs[INPUT_SIGNAL + i].getVoltageSum());
            gains[i] = params[PARAM_INPUT_GAIN + i].getValue();
        }
        simd::float_4 signals_simd(signals[0], signals[1], signals[2], signals[3]);
        simd::float_4 gains_simd(gains[0], gains[1], gains[2], gains[3]);
        // Process the input signals with the DC blocking filters.
        dc_blocker.process(signals_simd);
        // Insert the normalized and processed input signal into the delay.
        delay.insert(gains_simd * (is_ac_coupled ? dc_blocker.getValue() : signals_simd));
    }

    /// @brief Create a point from a spectral coefficient.
    ///
    /// @param index The index of the frequency bin.
    /// @param coefficient The value of the frequency bin.
    /// @param length Half the length of the FFT, i.e., \f$\frac{N}{2}\f$.
    /// @param slope The slope (dB/oct). It allows you to adjust spectrum
    /// analyzer display's slope around the reference frequency. Skewing the
    /// spectrum can be useful because higher frequencies usually have weaker
    /// power in comparison to the lower frequencies. By choosing an
    /// appropriate spectrum slope, you can compensate for this fact and make
    /// the spectrum plot look more convenient and meaningful.
    /// @param reference_frequency The frequency frequency for spectrum
    /// scaling. This is the frequency that the gain curve is centered around
    /// measured in Hertz.
    /// @param maximum_energy The maximum energy level (measured in decibels.)
    /// @param minimum_energy The minimum energy level (measured in decibels.)
    /// The minimum energy only takes effect in logarithmic mode.
    /// @returns A new point containing the encoded spectral coefficient.
    /// @details
    /// The `x` component of the point is either the linear or exponential
    /// frequency value depending on `get_frequency_scale()`. The `y`
    /// component is either linear, exponential with a \f$60dB\f$ bias, or
    /// exponential with a \f$120dB\f$ bias. `x` and `y` are normalized in
    /// \f$[0, 1]\f$. Points outside this range should likely be removed
    /// before plotting. This can occur when the minimum and/or maximum
    /// frequency to render change from their default values.
    ///
    void make_points(const size_t& lane_index) {
        // The reference frequency for the slope compensation.
        static constexpr float reference_frequency = 1000.f;
        // The maximum amplitude for logarithmic mode.
        static constexpr float max_magnitude = 12.f;
        // Determine the Nyquist rate from the sample rate.
        const float nyquist_rate = sample_rate / 2.f;
        // Get parameters from the panel once before iterating over points.
        const auto slope = get_slope();
        const auto low_frequency = get_low_frequency();
        const auto high_frequency = get_high_frequency();
        const auto frequency_scale = get_frequency_scale();
        const auto magnitude_scale = get_magnitude_scale();
        // Determine the non-repeated coefficients.
        const float N = filtered_coeffs.size() / 2.f + 1.f;
        for (size_t n = 0; n < static_cast<size_t>(N); n++) {
            // Set the point to a reference from the rasterized point buffer.
            Vec& point = raster_coeffs[lane_index][n];
            // Set the X point to the normalized linear coefficient offset.
            point.x = n / N;
            // Determine the y-scale from the frequency. The slope is provided
            // in decibels/octave, so first determine the octave offset from
            // the current frequency using, e.g., 1000Hz as the reference
            // frequency for the curve. I.e., because octaves are logarithmic,
            // we can simply compute \f$\log2(f_i / f_{reference})\f$ and
            // multiply by the slope. Because we're dealing with y first in
            // terms of amplitude, also convert the decibel scaling to an
            // amplitude gain.
            auto gain = log2f(point.x * nyquist_rate / reference_frequency + std::numeric_limits<float>::epsilon());
            gain = Math::decibels2amplitude(slope * gain);
            // Normalize X point based on the minimum and maximum frequencies.
            point.x -= low_frequency / nyquist_rate;
            point.x /= (high_frequency - low_frequency) / nyquist_rate;
            // Map the point from linear to logarithmic (Hertz) frequency
            // range. Handle negative points by taking the absolute value
            // before the square root (to avoid NaN) and negating the positive
            // result.
            if (frequency_scale == FrequencyScale::Logarithmic)
                point.x = point.x < 0 ? -sqrtf(fabs(point.x)) : sqrtf(point.x);
            // Set the Y point to the linear coefficient percentage. Apply the
            // gain that was previously calculated from the scaling function.
            const float max_amplitude = Math::decibels2amplitude(max_magnitude);
            point.y = gain * abs(filtered_coeffs[n]).s[lane_index] / (max_amplitude * N);
            // Apply magnitude scaling to the Y point.
            switch (magnitude_scale) {
            case MagnitudeScale::Linear:
                break;
            case MagnitudeScale::Logarithmic60dB:  // Exponential with -60dB bias
                point.y = Math::amplitude2decibels(point.y) / (60.f + max_magnitude) + 1.f;
                break;
            case MagnitudeScale::Logarithmic120dB:  // Exponential with -120dB bias
                point.y = Math::amplitude2decibels(point.y) / (120.f + max_magnitude) + 1.f;
                break;
            default: break;
            }
        }
        render_coefficients[lane_index] = raster_coeffs[lane_index];
    }

    /// @brief Process samples with the DFT.
    inline void process_coefficients() {
        // Determine the alpha parameter of the low-pass smoothing filter.
        const float alpha = get_time_smoothing_alpha();
        // Determine the setting of the frequency smoothing mode.
        const auto frequency_smoothing = get_frequency_smoothing();
        if (fft.is_done_computing()) {
            // Perform octave smoothing. For an N-length FFT, smooth over the
            // first N/2 + 1 coefficients to omit reflected frequencies.
            if (frequency_smoothing != FrequencySmoothing::None)
                fft.smooth(sample_rate, to_float(frequency_smoothing));
            // Pass the coefficients through a smoothing filter.
            for (size_t n = 0; n < fft.coefficients.size(); n++)
                filtered_coeffs[n] = alpha * abs(filtered_coeffs[n]) + (1.0 - alpha) * abs(fft.coefficients[n]);
            make_points(0);
            make_points(1);
            make_points(2);
            make_points(3);
            // Add the delay line to the FFT pipeline.
            fft.buffer(delay.contiguous(), window_function.get_samples());
        }
        // Perform the number of FFT steps required at this hop-rate.
        fft.step(get_hop_length());
    }

    /// @brief Set the lights on the panel.
    ///
    /// @param args the sample arguments (sample rate, sample time, etc.)
    ///
    inline void process_lights(const ProcessArgs& args) {
        if (!light_divider.process()) return;
        const auto light_time = args.sampleTime * light_divider.getDivision();
        lights[LIGHT_RUN].setSmoothBrightness(is_running, light_time);
    }

    /// @brief Process a sample.
    ///
    /// @param args the sample arguments (sample rate, sample time, etc.)
    ///
    void process(const ProcessArgs& args) final {
        process_window();
        process_run_button();
        process_input_signal();
        process_coefficients();
        process_lights(args);
    }
};

/// @brief A display widget for rendering frequency coefficients.
struct SpectrumAnalyzerDisplay : TransparentWidget {
 private:
    /// The vertical (top) padding for the plot.
    const size_t pad_top = 20;
    /// The vertical (bottom) padding for the plot.
    const size_t pad_bottom = 50;
    /// The horizontal (left) padding for the plot.
    const size_t pad_left = 30;
    /// The horizontal (right) padding for the plot.
    const size_t pad_right = 5;
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

    /// the font for rendering text on the display
    const std::shared_ptr<Font> font = APP->window->loadFont(
        asset::plugin(plugin_instance, "res/Font/Arial/Bold.ttf")
    );

    /// The module to render on the display.
    SpectrumAnalyzer* module = nullptr;

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
    /// @brief Initialize a new spectrum analyzer display widget.
    ///
    /// @param module_ the module to render on the display.
    ///
    explicit SpectrumAnalyzerDisplay(SpectrumAnalyzer* module_) :
        TransparentWidget(),
        module(module_) { }

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
    void onButton(const event::Button &e) override {
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
    void onDragStart(const event::DragStart &e) override {
        // // lock the cursor so it does not move in the engine during the edit
        // APP->window->cursorLock();
        // consume the event to prevent it from propagating
        e.consume(this);
    }

    /// @brief Respond to drag move event on this widget.
    void onDragMove(const event::DragMove &e) override {
        // consume the event to prevent it from propagating
        e.consume(this);
        // if the drag operation is not active, return early
        if (!mouse_state.is_pressed) return;
        // update the drag state based on the change in position from the mouse
        mouse_state.position.x += e.mouseDelta.x / APP->scene->rackScroll->zoomWidget->zoom;
        mouse_state.position.y += e.mouseDelta.y / APP->scene->rackScroll->zoomWidget->zoom;
    }

    /// @brief Respond to drag end event on this widget.
    void onDragEnd(const event::DragEnd &e) override {
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

    /// @brief Draw the X ticks with a linear scale.
    ///
    /// @param args the arguments for the current draw call
    ///
    void draw_x_ticks_linear(const DrawArgs& args) {
        static constexpr float xticks = 10;
        for (float i = 1; i < xticks; i++) {
            // Determine the relative position and re-scale it to the pixel
            // location on-screen. Since we're drawing a static number of
            // points, the position doesn't change relative to the minimum or
            // maximum frequencies (only the label value will change.)
            float position = i / xticks;
            float point_x = rescale(position, 0.f, 1.f, pad_left, box.size.x - pad_right);
            // Render tick marker
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, point_x, pad_top);
            nvgLineTo(args.vg, point_x, box.size.y - pad_bottom);
            nvgStrokeWidth(args.vg, axis_stroke_width);
            nvgStrokeColor(args.vg, axis_stroke_color);
            nvgStroke(args.vg);
            nvgClosePath(args.vg);
            // Render tick label
            float freq = get_low_frequency() + (get_high_frequency() - get_low_frequency()) * position;
            const auto freq_string = Math::freq_to_string(freq);
            nvgFontSize(args.vg, axis_font_size);
            nvgFillColor(args.vg, axis_font_color);
            nvgTextAlign(args.vg, NVG_ALIGN_BOTTOM | NVG_ALIGN_CENTER);
            nvgText(args.vg, point_x, box.size.y - pad_bottom + 8, freq_string.c_str(), NULL);
        }
    }

    /// @brief Draw the X ticks with an exponential scale.
    ///
    /// @param args the arguments for the current draw call
    ///
    void draw_x_ticks_logarithmic(const DrawArgs& args) {
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
                nvgMoveTo(args.vg, rescale(position, 0.f, 1.f, pad_left, box.size.x - pad_right), pad_top);
                nvgLineTo(args.vg, rescale(position, 0.f, 1.f, pad_left, box.size.x - pad_right), box.size.y - pad_bottom);
                nvgStrokeWidth(args.vg, axis_stroke_width);
                nvgStrokeColor(args.vg, axis_stroke_color);
                nvgStroke(args.vg);
                nvgClosePath(args.vg);
            }
            // Render a label with the base frequency in kHz.
            const auto freq_string = Math::freq_to_string(base_frequency);
            nvgFontSize(args.vg, axis_font_size);
            nvgFillColor(args.vg, axis_font_color);
            nvgTextAlign(args.vg, NVG_ALIGN_BOTTOM | NVG_ALIGN_CENTER);
            nvgText(args.vg, rescale(sqrt((base_frequency - get_low_frequency()) / frequency_range), 0.f, 1.f, pad_left, box.size.x - pad_right), box.size.y - pad_bottom + 8, freq_string.c_str(), NULL);
        }
    }

    /// @brief Draw the Y ticks with a linear scale.
    ///
    /// @param args the arguments for the current draw call
    ///
    void draw_y_ticks_linear(const DrawArgs& args) {
        // Iterate over the levels from 25%-400% in steps of 25%.
        for (int level_offset = 0; level_offset < 17; level_offset++) {
            const auto level = 25 * level_offset;
            const auto y_position = rescale(0.25 * level / 100.f, 0.f, 1.f, box.size.y - pad_bottom, pad_top);
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, pad_left, y_position);
            nvgLineTo(args.vg, box.size.x - pad_right, y_position);
            nvgStrokeWidth(args.vg, axis_stroke_width);
            nvgStrokeColor(args.vg, axis_stroke_color);
            nvgStroke(args.vg);
            nvgClosePath(args.vg);
            const auto label = std::to_string(level) + "%";
            nvgFontSize(args.vg, axis_font_size);
            nvgFillColor(args.vg, axis_font_color);
            nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, pad_left - 2, y_position, label.c_str(), NULL);
        }
    }

    /// @brief Draw the Y ticks with a logarithmic scale.
    ///
    /// @tparam L the data type for the levels.
    /// @param args The arguments for the current draw call.
    /// @param minimum_level The minimum level to render.
    /// @param maximum_level The maximum level to render.
    /// @param levels The individual magnitude levels to render and label.
    ///
    template<typename L>
    void draw_y_ticks_logarithmic(
        const DrawArgs& args,
        const float& minimum_level,
        const float& maximum_level,
        L levels
    ) {
        for (auto& level : levels) {
            // Compute the magnitude of the shifted and scaled level (such that
            // 12dB is the maximum level to render on-screen).
            const auto magnitude = (level - maximum_level) / (maximum_level - minimum_level) + 1.f;
            const auto y_position = rescale(magnitude, 0.f, 1.f, box.size.y - pad_bottom, pad_top);
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, pad_left, y_position);
            nvgLineTo(args.vg, box.size.x - pad_right, y_position);
            nvgStrokeWidth(args.vg, axis_stroke_width);
            nvgStrokeColor(args.vg, axis_stroke_color);
            nvgStroke(args.vg);
            nvgClosePath(args.vg);
            const auto label = std::to_string(level) + "dB";
            nvgFontSize(args.vg, axis_font_size);
            nvgFillColor(args.vg, axis_font_color);
            nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, pad_left - 2, y_position, label.c_str(), NULL);
        }
    }

    /// @brief Draw DFT coefficients as a stroke.
    /// @param args the arguments for the current draw call
    /// @param coefficients The coefficients to render
    /// @param stroke_width The width of the stroke.
    /// @param stroke_color The color of the stroke.
    /// @param fill_color The color of the fill.
    ///
    void draw_coefficients(
        const DrawArgs& args,
        std::vector<Vec> coefficients,
        const float& stroke_width,
        const NVGcolor& stroke_color={{{0.9f, 0.85f, 0.15f, 0.75f}}},
        const NVGcolor& fill_color={{{0.9f, 0.85f, 0.15f, 0.5f}}}
    ) {
        // Create a rectangle for masking the curve to the visible area.
        const Rect mask = Rect(Vec(pad_left, pad_top), box.size.minus(Vec(pad_left + pad_right, pad_top + pad_bottom)));
        // Create a new path that is masked to the box of the waveform display.
        nvgSave(args.vg);
        nvgBeginPath(args.vg);
        nvgScissor(args.vg, mask.pos.x, mask.pos.y, mask.size.x, mask.size.y);
        // For fill mode, move to a starting point on the bottom left of the
        // mask that is as far out as the stroke of the line (to hide it.)
        nvgMoveTo(args.vg, mask.pos.x - 2 * stroke_width, mask.pos.y + mask.size.y + 2 * stroke_width);
        // Find and render the first visible frequency bin.
        size_t n = 0;
        for (; n < coefficients.size(); n++) {
            Vec point = coefficients[n];
            // Check if we've reached the visible frequency range.
            if (point.x < 0.f) continue;
            // Remap the coefficient point to the window of the mask.
            point.x = rescale(point.x, 0.f, 1.f, mask.pos.x, mask.pos.x + mask.size.x);
            point.y = rescale(point.y, 0.f, 1.f, mask.pos.y + mask.size.y, mask.pos.y);
            // Clip the point to the visible window.
            point.x = Math::clip(point.x, mask.pos.x - 2 * stroke_width, mask.pos.x + mask.size.x + 2 * stroke_width);
            point.y = Math::clip(point.y, mask.pos.y - 2 * stroke_width, mask.pos.y + mask.size.y + 2 * stroke_width);
            // Draw an invisible line from the bottom point to the starting point.
            nvgLineTo(args.vg, mask.pos.x - 2 * stroke_width, point.y);
            // Exit the loop as we have found the first point.
            break;
        }
        // Render the visible frequency bins.
        if (module->is_bezier_enabled) {
            for (; n < coefficients.size() - 2; n++) {
                // Determine whether this is the last point to render.
                bool last_point = (coefficients[n+1].x >= 1.f) || (n == coefficients.size() - 3);
                // Create a neighborhood of points to render.
                Vec points[4] = {coefficients[n-1], coefficients[n-0], coefficients[n+1], coefficients[n+2]};
                for (Vec& point : points) {
                    // Remap the coefficient point to the window of the mask.
                    point.x = rescale(point.x, 0.f, 1.f, mask.pos.x, mask.pos.x + mask.size.x);
                    point.y = rescale(point.y, 0.f, 1.f, mask.pos.y + mask.size.y, mask.pos.y);
                    // Clip the point to the visible window.
                    point.x = Math::clip(point.x, mask.pos.x - 2 * stroke_width, mask.pos.x + mask.size.x + 2 * stroke_width);
                    point.y = Math::clip(point.y, mask.pos.y - 2 * stroke_width, mask.pos.y + mask.size.y + 2 * stroke_width);
                }
                Vec control[2];
                catmull_rom_to_bezier(points, control);
                // Draw a cubic Bezier from p[i] to p[i+1] using control points
                nvgBezierTo(args.vg, control[0].x, control[0].y, control[1].x, control[1].y, points[2].x, points[2].y);
                if (last_point) {
                    nvgLineTo(args.vg, mask.pos.x + mask.size.x + 2 * stroke_width, points[3].y);
                    break;
                }
            }
        } else {
            for (; n < coefficients.size(); n++) {
                Vec point = coefficients[n];
                // Determine whether this is the last point to render.
                bool last_point = (point.x >= 1.f) || (n == coefficients.size() - 1);
                // Remap the coefficient point to the window of the mask.
                point.x = rescale(point.x, 0.f, 1.f, mask.pos.x, mask.pos.x + mask.size.x);
                point.y = rescale(point.y, 0.f, 1.f, mask.pos.y + mask.size.y, mask.pos.y);
                // Clip the point to the visible window.
                point.x = Math::clip(point.x, mask.pos.x - stroke_width, mask.pos.x + mask.size.x + stroke_width);
                point.y = Math::clip(point.y, mask.pos.y - stroke_width, mask.pos.y + mask.size.y + stroke_width);
                // Connection to the next point in the plot.
                nvgLineTo(args.vg, point.x, point.y);
                // Connection to stop point for fill.
                if (last_point) {
                    nvgLineTo(args.vg, mask.pos.x + mask.size.x + 2 * stroke_width, point.y);
                    break;
                }
            }
        }
        // For fill mode, move to a stopping point on the bottom right of the
        // mask that is as far out as the stroke of the line (to hide it.)
        nvgLineTo(args.vg, mask.pos.x + mask.size.x + 2 * stroke_width, mask.pos.y + mask.size.y + 2 * stroke_width);
        nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
        nvgStrokeWidth(args.vg, stroke_width);
        nvgStrokeColor(args.vg, stroke_color);
        nvgStroke(args.vg);
        if (module->is_fill_enabled) {
            nvgFillColor(args.vg, fill_color);
            nvgFill(args.vg);
        }
        nvgResetScissor(args.vg);
        nvgClosePath(args.vg);
        nvgRestore(args.vg);
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

    /// @brief Return the frequency that the mouse is hovering over.
    /// @param mouse_position The position of the mouse on the screen.
    /// @param scale The frequency scale to render the mouse transform with.
    /// @param low_frequency The low frequency bound of the window.
    /// @param high_frequency The high frequency bound of the window.
    static inline float get_hover_freq(
        const Vec& mouse_position,
        const FrequencyScale& scale,
        const float& low_frequency,
        const float& high_frequency
    ) {
        switch (scale) {
        case FrequencyScale::Linear:
            return low_frequency + (high_frequency - low_frequency) * mouse_position.x;
        case FrequencyScale::Logarithmic:
            return (high_frequency - low_frequency) * Math::squared(mouse_position.x) + low_frequency;
        default:
            throw std::runtime_error("Invalid frequency scale " + std::to_string(static_cast<int>(scale)));
        }
    }

    /// @brief Return a string representation of the mouse position.
    /// @param mouse_position The position of the mouse on the screen.
    /// @param scale The magnitude scale to render the mouse transform with.
    static inline std::string mouse_position_to_string(
        const Vec& mouse_position,
        const MagnitudeScale& scale
    ) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2);
        switch (scale) {
        case MagnitudeScale::Linear:
            stream << mouse_position.y * 4 * 100 << "%";
            break;
        case MagnitudeScale::Logarithmic60dB:
            stream << rescale(mouse_position.y, 0, 1, -60, 12) << "dB";
            break;
        case MagnitudeScale::Logarithmic120dB:
            stream << rescale(mouse_position.y, 0, 1, -120, 12) << "dB";
            break;
        default:
            throw std::runtime_error("Invalid magnitude scale " + std::to_string(static_cast<int>(scale)));
        }
        return stream.str();
    }

    /// @brief Draw the cross-hair information as text.
    /// @param args the arguments for the current draw call.
    void draw_cross_hair_text(const DrawArgs& args) {
        const auto mouse_position = get_mouse_position();
        nvgFontSize(args.vg, 9);
        nvgFontFaceId(args.vg, font->handle);
        nvgFillColor(args.vg, {{{0.f / 255.f, 90.f / 255.f, 11.f / 255.f, 1.f}}});
        nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_LEFT);
        // Render hovered frequency above the plot in the top left.
        const float hover_freq = get_hover_freq(
            mouse_position,
            module->get_frequency_scale(),
            get_low_frequency(),
            get_high_frequency()
        );
        const auto hover_freq_string = Math::freq_to_string(hover_freq);
        nvgText(args.vg, pad_left + 3, pad_top / 2, hover_freq_string.c_str(), NULL);
        // Convert the frequency to a note.
        if (hover_freq > 0) {  // Render note, octave, and tuning (in cents.)
            MusicTheory::TunedNote note(hover_freq);
            nvgText(args.vg, pad_left + 55, pad_top / 2, note.note_string().c_str(), NULL);
            nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_RIGHT);
            nvgText(args.vg, pad_left + 140, pad_top / 2, note.tuning_string().c_str(), NULL);
        }
        ;
        // Render the y position.
        const auto mouse_position_string = mouse_position_to_string(mouse_position, module->get_magnitude_scale());
        nvgTextAlign(args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_RIGHT);
        nvgText(args.vg, box.size.x - pad_right - 3, pad_top / 2, mouse_position_string.c_str(), NULL);
    }

    /// @brief Draw the screen.
    ///
    /// @param args the arguments for the current draw call
    ///
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1) {  // Render as a light/display w/o dimming features
            // Draw the background.
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, corner_radius);
            nvgFillColor(args.vg, background_color);
            nvgFill(args.vg);
            nvgStrokeColor(args.vg, axis_stroke_color);
            nvgStroke(args.vg);
            nvgClosePath(args.vg);
            // Draw the frequency (X) axis.
            // - Left border
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, pad_left, pad_top);
            nvgLineTo(args.vg, pad_left, box.size.y - pad_bottom);
            nvgStrokeWidth(args.vg, axis_stroke_width);
            nvgStrokeColor(args.vg, axis_stroke_color);
            nvgStroke(args.vg);
            nvgClosePath(args.vg);
            // - Right border
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, box.size.x - pad_right, pad_top);
            nvgLineTo(args.vg, box.size.x - pad_right, box.size.y - pad_bottom);
            nvgStrokeWidth(args.vg, axis_stroke_width);
            nvgStrokeColor(args.vg, axis_stroke_color);
            nvgStroke(args.vg);
            nvgClosePath(args.vg);
            // - Ticks
            switch (module == nullptr ? FrequencyScale::Logarithmic : module->get_frequency_scale()) {
            case FrequencyScale::Linear:
                draw_x_ticks_linear(args);
                break;
            case FrequencyScale::Logarithmic:
                draw_x_ticks_logarithmic(args);
                break;
            default:
                throw std::runtime_error("Invalid frequency scale " + std::to_string(static_cast<int>(module->get_frequency_scale())));
            }
            // Draw the magnitude (Y) axis.
            switch (module == nullptr ? MagnitudeScale::Logarithmic60dB : module->get_magnitude_scale()) {
            case MagnitudeScale::Linear:
                draw_y_ticks_linear(args);
                break;
            case MagnitudeScale::Logarithmic60dB:
                draw_y_ticks_logarithmic(args, -60.f, 12.f, std::vector<int>{12, 0, -12, -24, -48, -60});
                break;
            case MagnitudeScale::Logarithmic120dB:
                draw_y_ticks_logarithmic(args, -120.f, 12.f, std::vector<int>{12, 0, -12, -24, -48, -60, -96, -120});
                break;
            default:
                throw std::runtime_error("Invalid magnitude scale " + std::to_string(static_cast<int>(module->get_magnitude_scale())));
            }
            if (module != nullptr) {
                draw_coefficients(args, module->render_coefficients[0], 1.5, {{{1.f, 0.f, 0.f, 1.f}}}, {{{1.f, 0.f, 0.f, 0.35f}}});
                draw_coefficients(args, module->render_coefficients[1], 1.5, {{{0.f, 1.f, 0.f, 1.f}}}, {{{0.f, 1.f, 0.f, 0.35f}}});
                draw_coefficients(args, module->render_coefficients[2], 1.5, {{{0.f, 0.f, 1.f, 1.f}}}, {{{0.f, 0.f, 1.f, 0.35f}}});
                draw_coefficients(args, module->render_coefficients[3], 1.5, {{{1.f, 1.f, 0.f, 1.f}}}, {{{1.f, 1.f, 0.f, 0.35f}}});
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
        TransparentWidget::drawLayer(args, layer);
    }
};

/// @brief The widget for the spectrum analyzer module.
struct SpectrumAnalyzerWidget : ModuleWidget {
    /// @brief Create a new spectrum analyzer widget.
    ///
    /// @param module The back-end module to interact with. Can be a nullptr.
    ///
    explicit SpectrumAnalyzerWidget(SpectrumAnalyzer* module = nullptr) : ModuleWidget() {
        setModule(module);
        setPanel(createPanel(
            asset::plugin(plugin_instance, "res/SpectrumAnalyzer-Light.svg"),
            asset::plugin(plugin_instance, "res/SpectrumAnalyzer-Dark.svg")
        ));
        // Input signals, gains, output signals, and meters.
        for (std::size_t i = 0; i < SpectrumAnalyzer::NUM_CHANNELS; i++) {
            addInput(createInput<PJ301MPort>(Vec(11, 30 + 75 * i), module, SpectrumAnalyzer::INPUT_SIGNAL + i));
            addParam(createParam<Trimpot>(Vec(13, 66 + 75 * i), module, SpectrumAnalyzer::PARAM_INPUT_GAIN + i));
        }
        // Buttons.
        addParam(createParamCentered<PB61303>(Vec(8 + 15, 331 + 15), module, SpectrumAnalyzer::PARAM_RUN));
        addChild(createLightCentered<PB61303Light<WhiteLight>>(Vec(8 + 15, 331 + 15), module, SpectrumAnalyzer::LIGHT_RUN));
        // Screen.
        SpectrumAnalyzerDisplay* display = new SpectrumAnalyzerDisplay(module);
        display->setPosition(Vec(45, 15));
        display->setSize(Vec(660, 350));
        addChild(display);
        // Screen controls.
        // Window function control with custom angles to match discrete range.
        auto window_function_param = createParam<WindowFunctionTextKnob>(Vec(50 + 0 * 66, 330), module, SpectrumAnalyzer::PARAM_WINDOW_FUNCTION);
        window_function_param->maxAngle = 2.f * M_PI;
        addParam(window_function_param);
        // Window length control with custom angles to match discrete range.
        auto window_length_param = createParam<WindowLengthTextKnob>(Vec(50 + 1 * 66, 330), module, SpectrumAnalyzer::PARAM_WINDOW_LENGTH);
        window_length_param->maxAngle = 1.2f * M_PI;
        addParam(window_length_param);
        // Hop length control.
        addParam(createParam<TextKnob>(Vec(50 + 2 * 66, 330), module, SpectrumAnalyzer::PARAM_HOP_LENGTH));
        // Frequency scale control with custom angles to match discrete range.
        auto frequency_scale_param = createParam<FrequencyScaleTextKnob>(Vec(50 + 3 * 66, 330), module, SpectrumAnalyzer::PARAM_FREQUENCY_SCALE);
        frequency_scale_param->maxAngle = 0.3 * M_PI;
        addParam(frequency_scale_param);
        // Magnitude scale control with custom angles to match discrete range.
        auto magnitude_scale_param = createParam<MagnitudeScaleTextKnob>(Vec(50 + 4 * 66, 330), module, SpectrumAnalyzer::PARAM_MAGNITUDE_SCALE);
        magnitude_scale_param->maxAngle = 0.6 * M_PI;
        addParam(magnitude_scale_param);
        // Time smoothing control.
        addParam(createParam<TextKnob>(Vec(50 + 5 * 66, 330), module, SpectrumAnalyzer::PARAM_TIME_SMOOTHING));
        // Frequency smoothing control with custom angles to match discrete range.
        auto frequency_smoothing_param = createParam<FrequencySmoothingTextKnob>(Vec(50 + 6 * 66, 330), module, SpectrumAnalyzer::PARAM_FREQUENCY_SMOOTHING);
        frequency_smoothing_param->maxAngle = 2.f * M_PI;
        addParam(frequency_smoothing_param);
        // Low and High frequency (frequency range) controls.
        addParam(createParam<TextKnob>(Vec(50 + 7 * 66, 330), module, SpectrumAnalyzer::PARAM_LOW_FREQUENCY));
        addParam(createParam<TextKnob>(Vec(50 + 8 * 66, 330), module, SpectrumAnalyzer::PARAM_HIGH_FREQUENCY));
        // Slope (dB/octave @1000Hz) controls.
        addParam(createParam<TextKnob>(Vec(50 + 9 * 66, 330), module, SpectrumAnalyzer::PARAM_SLOPE));
        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    }

    /// @brief Append the context menu to the module when right clicked.
    ///
    /// @param menu the menu object to add context items for the module to
    ///
    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Render Settings"));
        menu->addChild(createBoolPtrMenuItem("Filled Display", "", &getModule<SpectrumAnalyzer>()->is_fill_enabled));
        menu->addChild(createBoolPtrMenuItem("Bezier Curve", "", &getModule<SpectrumAnalyzer>()->is_bezier_enabled));
        menu->addChild(createBoolPtrMenuItem("AC-coupled", "", &getModule<SpectrumAnalyzer>()->is_ac_coupled));
        ModuleWidget::appendContextMenu(menu);
    }
};

Model* modelSpectrumAnalyzer = createModel<SpectrumAnalyzer, SpectrumAnalyzerWidget>("SpectrumAnalyzer");

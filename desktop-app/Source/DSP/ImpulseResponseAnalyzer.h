/*
  ==============================================================================

    ImpulseResponseAnalyzer.h

    Impulse Response and Frequency Response measurement using sweep method

  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <vector>
#include <complex>

class ImpulseResponseAnalyzer
{
public:
    //==========================================================================
    enum class MeasurementState
    {
        Idle,
        GeneratingSweep,
        Processing,
        Complete
    };

    struct MeasurementResult
    {
        std::vector<float> impulseResponse;
        std::vector<float> frequencyMagnitude;  // dB
        std::vector<float> frequencyPhase;      // degrees
        std::vector<float> frequencyAxis;       // Hz
        float peakLevel { 0.0f };               // dB
        float rt60 { 0.0f };                    // seconds (reverberation time)
        bool isValid { false };
    };

    //==========================================================================
    ImpulseResponseAnalyzer();
    ~ImpulseResponseAnalyzer() = default;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    //==========================================================================
    // Start measurement
    void startMeasurement();
    void stopMeasurement();

    // Get current state
    MeasurementState getState() const { return state; }
    float getProgress() const;

    // Get results
    MeasurementResult getResult() const { return result; }

    //==========================================================================
    // Audio processing - call from audio callback
    // Returns sweep sample to output, records input
    float processSample(float inputSample);

    //==========================================================================
    // Configuration
    void setSweepDuration(float seconds) { sweepDuration = seconds; }
    void setStartFrequency(float freq) { startFrequency = freq; }
    void setEndFrequency(float freq) { endFrequency = freq; }
    void setSweepAmplitude(float amp) { sweepAmplitude = amp; }

    float getSweepDuration() const { return sweepDuration; }
    float getStartFrequency() const { return startFrequency; }
    float getEndFrequency() const { return endFrequency; }

    //==========================================================================
    // Callback when measurement completes
    std::function<void()> onMeasurementComplete;

private:
    //==========================================================================
    void generateSweep();
    void generateInverseSweep();
    void computeImpulseResponse();
    void computeFrequencyResponse();
    float calculateRT60();

    //==========================================================================
    double sampleRate { 44100.0 };
    float sweepDuration { 3.0f };      // seconds
    float startFrequency { 20.0f };    // Hz
    float endFrequency { 20000.0f };   // Hz
    float sweepAmplitude { 0.5f };

    MeasurementState state { MeasurementState::Idle };

    // Sweep signal
    std::vector<float> sweepSignal;
    std::vector<float> inverseSweep;

    // Recording buffer
    std::vector<float> recordedSignal;

    // Current position
    int currentSample { 0 };
    int totalSamples { 0 };

    // Result
    MeasurementResult result;

    // FFT for frequency response
    static constexpr int fftOrder = 14;  // 16384 samples
    static constexpr int fftSize = 1 << fftOrder;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImpulseResponseAnalyzer)
};

/*
  ==============================================================================

    THDAnalyzer.h

    Total Harmonic Distortion (THD) and Signal-to-Noise Ratio (SNR) analyzer

  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <complex>

class THDAnalyzer
{
public:
    //==========================================================================
    struct MeasurementResult
    {
        float fundamentalFrequency { 0.0f };  // Hz
        float fundamentalAmplitude { 0.0f };  // dB
        float thd { 0.0f };                   // % (Total Harmonic Distortion)
        float thdPlusNoise { 0.0f };          // % (THD+N)
        float snr { 0.0f };                   // dB (Signal-to-Noise Ratio)
        float sinad { 0.0f };                 // dB (Signal-to-Noise and Distortion)
        std::vector<float> harmonicLevels;   // dB for each harmonic (up to 10)
        bool isValid { false };
    };

    //==========================================================================
    THDAnalyzer();
    ~THDAnalyzer() = default;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    //==========================================================================
    // Push samples for analysis
    void pushSample(float sample);

    // Get latest measurement result
    MeasurementResult getResult() const { return latestResult; }

    //==========================================================================
    // Configuration
    void setExpectedFundamental(float freq) { expectedFundamental = freq; }
    void setNumHarmonicsToMeasure(int num) { numHarmonics = juce::jlimit(2, 10, num); }

    float getExpectedFundamental() const { return expectedFundamental; }
    int getNumHarmonicsToMeasure() const { return numHarmonics; }

private:
    //==========================================================================
    void analyze();
    int findFundamentalBin();
    float getBinAmplitude(int bin);
    float getInterpolatedAmplitude(float exactBin);

    //==========================================================================
    static constexpr int fftOrder = 13;  // 8192 samples
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris };

    std::vector<float> inputBuffer;
    std::vector<float> fftData;
    std::vector<float> magnitudeSpectrum;

    int writeIndex { 0 };
    int samplesCollected { 0 };

    double sampleRate { 44100.0 };
    float expectedFundamental { 1000.0f };
    int numHarmonics { 5 };

    MeasurementResult latestResult;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(THDAnalyzer)
};

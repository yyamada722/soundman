/*
  ==============================================================================

    HarmonicsAnalyzer.h

    Harmonic analysis for detecting fundamental and overtones

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>

class HarmonicsAnalyzer
{
public:
    //==========================================================================
    static constexpr int maxHarmonics = 16;  // Maximum number of harmonics to track

    struct Harmonic
    {
        int number { 0 };           // Harmonic number (1 = fundamental)
        float frequency { 0.0f };   // Frequency in Hz
        float amplitude { 0.0f };   // Amplitude (linear)
        float amplitudeDb { -100.0f }; // Amplitude in dB
        float phase { 0.0f };       // Phase in radians
        bool detected { false };    // Whether this harmonic was detected
    };

    struct AnalysisResult
    {
        float fundamentalFrequency { 0.0f };
        float fundamentalAmplitudeDb { -100.0f };
        std::array<Harmonic, maxHarmonics> harmonics;
        int numHarmonicsDetected { 0 };
        float totalHarmonicDistortion { 0.0f };  // THD
        float inharmonicity { 0.0f };            // Deviation from perfect harmonic series
        bool isValid { false };
    };

    //==========================================================================
    HarmonicsAnalyzer();
    ~HarmonicsAnalyzer() = default;

    //==========================================================================
    // Configuration
    void setSampleRate(double rate);
    void setFundamentalFrequency(float freq);  // Set known fundamental (optional)
    void setMinAmplitudeDb(float db) { minAmplitudeDb = db; }
    void setHarmonicSearchWidth(float cents) { harmonicSearchWidthCents = cents; }

    double getSampleRate() const { return sampleRate; }

    //==========================================================================
    // FFT Configuration
    static constexpr int fftOrder = 12;  // 2^12 = 4096 samples
    static constexpr int fftSize = 1 << fftOrder;

    //==========================================================================
    // Processing
    void pushSample(float sample);
    AnalysisResult getLatestAnalysis() const { return latestResult; }

    // Analyze FFT magnitude data directly
    AnalysisResult analyze(const float* fftMagnitudes, int numBins, float fundamentalHint = 0.0f);

    //==========================================================================
    // Utility
    static float calculateTHD(const std::array<Harmonic, maxHarmonics>& harmonics, int numHarmonics);

private:
    //==========================================================================
    void processFFT();
    int findFundamental(const std::vector<float>& magnitudes);
    void findHarmonics(const std::vector<float>& magnitudes, float fundamental, AnalysisResult& result);
    float interpolatePeak(const std::vector<float>& magnitudes, int peakBin);
    float binToFrequency(float bin) const;
    float frequencyToBin(float freq) const;

    //==========================================================================
    double sampleRate { 44100.0 };
    float minAmplitudeDb { -60.0f };
    float harmonicSearchWidthCents { 50.0f };  // Search window around expected harmonic
    float knownFundamental { 0.0f };  // If set, use this instead of detecting

    // FFT processing
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    std::vector<float> fifo;
    std::vector<float> fftData;
    std::vector<float> magnitudes;
    int fifoIndex { 0 };
    bool nextFFTBlockReady { false };

    // Latest analysis result
    AnalysisResult latestResult;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonicsAnalyzer)
};

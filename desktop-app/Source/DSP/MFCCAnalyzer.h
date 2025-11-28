/*
  ==============================================================================

    MFCCAnalyzer.h

    Mel-Frequency Cepstral Coefficients (MFCC) analysis

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>

class MFCCAnalyzer
{
public:
    //==========================================================================
    static constexpr int numMFCCs = 13;       // Number of MFCC coefficients (including C0)
    static constexpr int numMelFilters = 26;  // Number of Mel filter banks

    struct MFCCResult
    {
        std::array<float, numMFCCs> coefficients;
        std::array<float, numMelFilters> melEnergies;
        float totalEnergy { 0.0f };
        bool isValid { false };
    };

    //==========================================================================
    MFCCAnalyzer();
    ~MFCCAnalyzer() = default;

    //==========================================================================
    // Configuration
    void setSampleRate(double rate);
    void setMinFrequency(float freq);
    void setMaxFrequency(float freq);

    double getSampleRate() const { return sampleRate; }
    float getMinFrequency() const { return minFrequency; }
    float getMaxFrequency() const { return maxFrequency; }

    //==========================================================================
    // FFT Configuration
    static constexpr int fftOrder = 11;  // 2^11 = 2048 samples
    static constexpr int fftSize = 1 << fftOrder;

    //==========================================================================
    // Processing
    void pushSample(float sample);
    MFCCResult getLatestResult() const { return latestResult; }

    // Analyze a block of samples directly
    MFCCResult analyze(const float* samples, int numSamples);

    //==========================================================================
    // Utility functions
    static float hzToMel(float hz);
    static float melToHz(float mel);

private:
    //==========================================================================
    void processFFT();
    void initializeMelFilterBank();
    void applyMelFilterBank(const std::vector<float>& powerSpectrum, std::array<float, numMelFilters>& melEnergies);
    void computeDCT(const std::array<float, numMelFilters>& melLogEnergies, std::array<float, numMFCCs>& mfccs);

    //==========================================================================
    double sampleRate { 44100.0 };
    float minFrequency { 20.0f };
    float maxFrequency { 8000.0f };  // Typically 8kHz for speech/music analysis

    // Mel filter bank
    std::vector<std::vector<float>> melFilterBank;
    std::vector<int> filterBankStartBins;
    std::vector<int> filterBankEndBins;
    bool filterBankInitialized { false };

    // FFT processing
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    std::vector<float> fifo;
    std::vector<float> fftData;
    std::vector<float> powerSpectrum;
    int fifoIndex { 0 };
    bool nextFFTBlockReady { false };

    // Pre-computed DCT matrix
    std::array<std::array<float, numMelFilters>, numMFCCs> dctMatrix;
    void initializeDCTMatrix();

    // Latest result
    MFCCResult latestResult;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MFCCAnalyzer)
};

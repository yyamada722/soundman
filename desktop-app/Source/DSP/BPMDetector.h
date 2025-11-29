/*
  ==============================================================================

    BPMDetector.h

    Beat/Tempo detection using onset detection and autocorrelation

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <deque>

class BPMDetector
{
public:
    //==========================================================================
    BPMDetector();
    ~BPMDetector() = default;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    //==========================================================================
    // Process audio samples and detect BPM
    void processBlock(const juce::AudioBuffer<float>& buffer);

    //==========================================================================
    // Get detected BPM (0 if not yet detected)
    float getBPM() const { return currentBPM; }

    // Get confidence level (0.0 - 1.0)
    float getConfidence() const { return confidence; }

    // Get onset strength signal for visualization
    const std::vector<float>& getOnsetStrength() const { return onsetStrength; }

    // Get autocorrelation for visualization
    const std::vector<float>& getAutocorrelation() const { return autocorrelation; }

    // Is a beat detected in the current frame?
    bool isBeatDetected() const { return beatDetected; }

    //==========================================================================
    // Settings
    void setMinBPM(float bpm) { minBPM = juce::jlimit(30.0f, 200.0f, bpm); }
    void setMaxBPM(float bpm) { maxBPM = juce::jlimit(60.0f, 300.0f, bpm); }
    float getMinBPM() const { return minBPM; }
    float getMaxBPM() const { return maxBPM; }

private:
    //==========================================================================
    void computeOnsetStrength(const juce::AudioBuffer<float>& buffer);
    void computeAutocorrelation();
    float findBPMFromAutocorrelation();
    void detectBeat();

    //==========================================================================
    double sampleRate { 44100.0 };
    int blockSize { 512 };

    // FFT for spectral flux computation
    static constexpr int fftOrder = 10;  // 1024 samples
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };

    std::vector<float> fftData;
    std::vector<float> prevSpectrum;
    std::vector<float> currentSpectrum;

    // Onset detection
    std::vector<float> onsetStrength;
    static constexpr int onsetBufferSize = 512;  // ~6 seconds at 44100Hz/512
    int onsetWritePos { 0 };
    float onsetThreshold { 0.0f };

    // Autocorrelation buffer
    std::vector<float> autocorrelation;

    // Beat tracking
    float currentBPM { 0.0f };
    float confidence { 0.0f };
    float minBPM { 60.0f };
    float maxBPM { 200.0f };

    // Beat detection
    bool beatDetected { false };
    float beatThreshold { 1.2f };  // Lower threshold for better sensitivity
    float lastOnsetValue { 0.0f };
    int samplesSinceLastBeat { 0 };

    // Smoothing
    float smoothedBPM { 0.0f };
    static constexpr float bpmSmoothingFactor = 0.3f;  // Faster response

    // Hop size for onset detection
    int hopSize { 512 };
    int sampleCounter { 0 };
    std::vector<float> inputBuffer;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BPMDetector)
};

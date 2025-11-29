/*
  ==============================================================================

    KeyDetector.h

    Musical key detection using chroma features and key profiles

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>

class KeyDetector
{
public:
    //==========================================================================
    // Key enum (12 major + 12 minor = 24 keys)
    enum class Key
    {
        Unknown = -1,
        CMajor = 0, CSharpMajor, DMajor, DSharpMajor, EMajor, FMajor,
        FSharpMajor, GMajor, GSharpMajor, AMajor, ASharpMajor, BMajor,
        CMinor, CSharpMinor, DMinor, DSharpMinor, EMinor, FMinor,
        FSharpMinor, GMinor, GSharpMinor, AMinor, ASharpMinor, BMinor
    };

    //==========================================================================
    KeyDetector();
    ~KeyDetector() = default;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    //==========================================================================
    // Process audio samples
    void processBlock(const juce::AudioBuffer<float>& buffer);

    //==========================================================================
    // Get detected key
    Key getDetectedKey() const { return detectedKey; }

    // Get key name string
    static juce::String getKeyName(Key key);

    // Get confidence (0.0 - 1.0)
    float getConfidence() const { return confidence; }

    // Get chroma features for visualization (12 values, C to B)
    const std::array<float, 12>& getChroma() const { return chromaFeatures; }

    // Get all key correlations for visualization
    const std::array<float, 24>& getKeyCorrelations() const { return keyCorrelations; }

private:
    //==========================================================================
    void computeChroma(const juce::AudioBuffer<float>& buffer);
    void detectKey();
    void initializeKeyProfiles();

    //==========================================================================
    double sampleRate { 44100.0 };
    int blockSize { 512 };

    // FFT for chroma computation
    static constexpr int fftOrder = 11;  // 2048 samples (faster processing)
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };

    std::vector<float> fftData;
    std::vector<float> inputBuffer;
    int inputBufferPos { 0 };

    // Chroma features (C, C#, D, D#, E, F, F#, G, G#, A, A#, B)
    std::array<float, 12> chromaFeatures {};
    std::array<float, 12> accumulatedChroma {};
    int chromaFrameCount { 0 };

    // Key profiles (Krumhansl-Schmuckler)
    std::array<float, 12> majorProfile {};
    std::array<float, 12> minorProfile {};

    // Key correlations (24 keys)
    std::array<float, 24> keyCorrelations {};

    // Detection result
    Key detectedKey { Key::Unknown };
    float confidence { 0.0f };

    // Smoothing
    std::array<float, 24> smoothedCorrelations {};
    static constexpr float smoothingFactor = 0.15f;  // Faster response

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyDetector)
};

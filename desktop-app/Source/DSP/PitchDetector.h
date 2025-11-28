/*
  ==============================================================================

    PitchDetector.h

    Real-time pitch detection using YIN algorithm

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>

class PitchDetector
{
public:
    //==========================================================================
    struct PitchResult
    {
        float frequency { 0.0f };      // Detected frequency in Hz
        float confidence { 0.0f };     // Confidence level (0-1)
        juce::String noteName;         // Musical note name (e.g., "A4")
        int midiNote { -1 };           // MIDI note number
        float cents { 0.0f };          // Cents deviation from nearest note
        bool isPitched { false };      // Whether a valid pitch was detected
    };

    //==========================================================================
    PitchDetector();
    ~PitchDetector() = default;

    //==========================================================================
    // Configuration
    void setSampleRate(double rate);
    void setMinFrequency(float freq) { minFrequency = freq; updateLagRange(); }
    void setMaxFrequency(float freq) { maxFrequency = freq; updateLagRange(); }
    void setThreshold(float thresh) { threshold = thresh; }

    double getSampleRate() const { return sampleRate; }
    float getMinFrequency() const { return minFrequency; }
    float getMaxFrequency() const { return maxFrequency; }
    float getThreshold() const { return threshold; }

    //==========================================================================
    // Processing
    void pushSample(float sample);
    PitchResult getLatestPitch() const { return latestPitch; }

    // Process a block of samples and return pitch
    PitchResult detectPitch(const float* samples, int numSamples);

    //==========================================================================
    // Utility functions
    static juce::String frequencyToNoteName(float frequency);
    static int frequencyToMidiNote(float frequency);
    static float midiNoteToFrequency(int midiNote);
    static float getCentsDeviation(float frequency, int midiNote);

private:
    //==========================================================================
    void updateLagRange();
    float yinDifference(const std::vector<float>& buffer, int tau);
    void cumulativeMeanNormalizedDifference(std::vector<float>& yinBuffer);
    int absoluteThreshold(const std::vector<float>& yinBuffer);
    float parabolicInterpolation(const std::vector<float>& yinBuffer, int tauEstimate);

    //==========================================================================
    double sampleRate { 44100.0 };
    float minFrequency { 50.0f };    // Minimum detectable frequency
    float maxFrequency { 2000.0f };  // Maximum detectable frequency
    float threshold { 0.4f };         // YIN threshold (lower = more selective, higher = more sensitive)

    int minLag { 0 };
    int maxLag { 0 };

    // Circular buffer for incoming samples
    static constexpr int bufferSize = 4096;
    std::vector<float> inputBuffer;
    int writeIndex { 0 };
    int samplesCollected { 0 };

    // YIN working buffer
    std::vector<float> yinBuffer;

    // Latest detected pitch
    PitchResult latestPitch;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchDetector)
};

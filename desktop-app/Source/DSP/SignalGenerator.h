/*
  ==============================================================================

    SignalGenerator.h

    Tone and noise signal generator for testing and measurement

  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <random>

//==============================================================================
/**
    Tone Generator - generates various waveforms
*/
class ToneGenerator
{
public:
    //==========================================================================
    enum class Waveform
    {
        Sine,
        Square,
        Triangle,
        Sawtooth
    };

    //==========================================================================
    ToneGenerator();
    ~ToneGenerator() = default;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    //==========================================================================
    // Parameters
    void setFrequency(float freq);
    void setAmplitude(float amp);  // 0.0 - 1.0
    void setWaveform(Waveform waveform);
    void setEnabled(bool enabled) { isEnabled = enabled; }

    float getFrequency() const { return frequency; }
    float getAmplitude() const { return amplitude; }
    Waveform getWaveform() const { return currentWaveform; }
    bool isGenerating() const { return isEnabled; }

    //==========================================================================
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    float getNextSample();

private:
    //==========================================================================
    float generateSine();
    float generateSquare();
    float generateTriangle();
    float generateSawtooth();

    //==========================================================================
    double sampleRate { 44100.0 };
    float frequency { 440.0f };
    float amplitude { 0.5f };
    Waveform currentWaveform { Waveform::Sine };
    bool isEnabled { false };

    double phase { 0.0 };
    double phaseIncrement { 0.0 };

    void updatePhaseIncrement();

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToneGenerator)
};

//==============================================================================
/**
    Noise Generator - generates various noise types
*/
class NoiseGenerator
{
public:
    //==========================================================================
    enum class NoiseType
    {
        White,
        Pink,
        Brown
    };

    //==========================================================================
    NoiseGenerator();
    ~NoiseGenerator() = default;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    //==========================================================================
    // Parameters
    void setNoiseType(NoiseType type);
    void setAmplitude(float amp);
    void setEnabled(bool enabled) { isEnabled = enabled; }

    NoiseType getNoiseType() const { return currentNoiseType; }
    float getAmplitude() const { return amplitude; }
    bool isGenerating() const { return isEnabled; }

    //==========================================================================
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    float getNextSample();

private:
    //==========================================================================
    float generateWhiteNoise();
    float generatePinkNoise();
    float generateBrownNoise();

    //==========================================================================
    double sampleRate { 44100.0 };
    float amplitude { 0.5f };
    NoiseType currentNoiseType { NoiseType::White };
    bool isEnabled { false };

    // Random number generator
    std::mt19937 rng;
    std::uniform_real_distribution<float> distribution { -1.0f, 1.0f };

    // Pink noise state (Voss-McCartney algorithm)
    static constexpr int pinkNoiseRows = 16;
    float pinkNoiseState[pinkNoiseRows] {};
    int pinkNoiseIndex { 0 };
    float pinkNoiseRunningSum { 0.0f };

    // Brown noise state
    float brownNoiseState { 0.0f };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseGenerator)
};

//==============================================================================
/**
    Sweep Generator - generates frequency sweep signals
*/
class SweepGenerator
{
public:
    //==========================================================================
    enum class SweepType
    {
        Linear,
        Logarithmic
    };

    //==========================================================================
    SweepGenerator();
    ~SweepGenerator() = default;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    //==========================================================================
    // Parameters
    void setStartFrequency(float freq) { startFrequency = freq; }
    void setEndFrequency(float freq) { endFrequency = freq; }
    void setDuration(float seconds) { duration = seconds; calculateSweepRate(); }
    void setSweepType(SweepType type) { sweepType = type; calculateSweepRate(); }
    void setAmplitude(float amp) { amplitude = amp; }
    void setEnabled(bool enabled);

    float getStartFrequency() const { return startFrequency; }
    float getEndFrequency() const { return endFrequency; }
    float getDuration() const { return duration; }
    SweepType getSweepType() const { return sweepType; }
    float getAmplitude() const { return amplitude; }
    bool isGenerating() const { return isEnabled; }
    float getCurrentFrequency() const { return currentFrequency; }
    float getProgress() const;

    //==========================================================================
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    float getNextSample();

    // Callback when sweep completes
    std::function<void()> onSweepComplete;

private:
    //==========================================================================
    void calculateSweepRate();
    void updateCurrentFrequency();

    //==========================================================================
    double sampleRate { 44100.0 };
    float startFrequency { 20.0f };
    float endFrequency { 20000.0f };
    float duration { 10.0f };  // seconds
    float amplitude { 0.5f };
    SweepType sweepType { SweepType::Logarithmic };
    bool isEnabled { false };

    float currentFrequency { 20.0f };
    double phase { 0.0 };
    double currentSample { 0.0 };
    double totalSamples { 0.0 };

    // For logarithmic sweep
    double logStartFreq { 0.0 };
    double logEndFreq { 0.0 };
    double logSweepRate { 0.0 };

    // For linear sweep
    double linearSweepRate { 0.0 };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SweepGenerator)
};

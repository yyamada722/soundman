/*
  ==============================================================================

    AudioFilter.h

    Audio filter processing (Lowpass, Highpass, Bandpass, Notch)

  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_core/juce_core.h>
#include <array>

class AudioFilter
{
public:
    //==========================================================================
    enum class FilterType
    {
        Lowpass,
        Highpass,
        Bandpass,
        Notch,
        LowShelf,
        HighShelf,
        Peak
    };

    //==========================================================================
    AudioFilter();
    ~AudioFilter() = default;

    //==========================================================================
    // Configuration
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    //==========================================================================
    // Filter parameters
    void setFilterType(FilterType type);
    void setFrequency(float freq);
    void setQ(float q);
    void setGain(float gainDb);  // For shelf and peak filters
    void setEnabled(bool enabled) { filterEnabled = enabled; }

    FilterType getFilterType() const { return currentType; }
    float getFrequency() const { return frequency; }
    float getQ() const { return qFactor; }
    float getGain() const { return gainDb; }
    bool isEnabled() const { return filterEnabled; }

    //==========================================================================
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    float processSample(int channel, float sample);

    //==========================================================================
    // Get frequency response for visualization
    float getMagnitudeForFrequency(float freq) const;
    void getMagnitudeForFrequencyArray(const float* frequencies, float* magnitudes, int numPoints) const;

private:
    //==========================================================================
    void updateCoefficients();

    //==========================================================================
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    std::array<Filter, 2> filters;  // Stereo
    typename Coefficients::Ptr coefficients;

    FilterType currentType { FilterType::Lowpass };
    float frequency { 1000.0f };
    float qFactor { 0.707f };  // Butterworth Q
    float gainDb { 0.0f };
    bool filterEnabled { true };

    double sampleRate { 44100.0 };
    bool isPrepared { false };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFilter)
};

//==============================================================================
/**
    3-Band Parametric EQ
*/
class ParametricEQ
{
public:
    //==========================================================================
    struct Band
    {
        float frequency { 1000.0f };
        float gain { 0.0f };       // dB
        float q { 1.0f };
        bool enabled { true };
    };

    //==========================================================================
    ParametricEQ();
    ~ParametricEQ() = default;

    //==========================================================================
    // Configuration
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    //==========================================================================
    // Band parameters
    void setBand(int bandIndex, float frequency, float gainDb, float q);
    void setBandFrequency(int bandIndex, float freq);
    void setBandGain(int bandIndex, float gainDb);
    void setBandQ(int bandIndex, float q);
    void setBandEnabled(int bandIndex, bool enabled);

    Band getBand(int bandIndex) const;
    static constexpr int numBands = 3;

    //==========================================================================
    // Master
    void setEnabled(bool enabled) { eqEnabled = enabled; }
    bool isEnabled() const { return eqEnabled; }

    //==========================================================================
    // Processing
    void process(juce::AudioBuffer<float>& buffer);

    //==========================================================================
    // Get frequency response for visualization
    float getMagnitudeForFrequency(float freq) const;
    void getMagnitudeForFrequencyArray(const float* frequencies, float* magnitudes, int numPoints) const;

private:
    //==========================================================================
    void updateBandCoefficients(int bandIndex);

    //==========================================================================
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    // 3 bands x 2 channels
    std::array<std::array<Filter, 2>, numBands> bandFilters;
    std::array<typename Coefficients::Ptr, numBands> bandCoefficients;
    std::array<Band, numBands> bands;

    bool eqEnabled { true };
    double sampleRate { 44100.0 };
    bool isPrepared { false };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParametricEQ)
};

/*
  ==============================================================================

    AudioFilter.cpp

    Audio filter processing implementation

  ==============================================================================
*/

#include "AudioFilter.h"

//==============================================================================
// AudioFilter Implementation
//==============================================================================

AudioFilter::AudioFilter()
{
    coefficients = Coefficients::makeLowPass(44100.0, 1000.0f);
}

void AudioFilter::prepare(double rate, int samplesPerBlock, int numChannels)
{
    sampleRate = rate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = rate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)numChannels;

    for (auto& filter : filters)
    {
        filter.prepare(spec);
    }

    updateCoefficients();
    isPrepared = true;
}

void AudioFilter::reset()
{
    for (auto& filter : filters)
    {
        filter.reset();
    }
}

void AudioFilter::setFilterType(FilterType type)
{
    if (currentType != type)
    {
        currentType = type;
        updateCoefficients();
    }
}

void AudioFilter::setFrequency(float freq)
{
    freq = juce::jlimit(20.0f, 20000.0f, freq);
    if (std::abs(frequency - freq) > 0.01f)
    {
        frequency = freq;
        updateCoefficients();
    }
}

void AudioFilter::setQ(float q)
{
    q = juce::jlimit(0.1f, 20.0f, q);
    if (std::abs(qFactor - q) > 0.001f)
    {
        qFactor = q;
        updateCoefficients();
    }
}

void AudioFilter::setGain(float gain)
{
    gain = juce::jlimit(-24.0f, 24.0f, gain);
    if (std::abs(gainDb - gain) > 0.01f)
    {
        gainDb = gain;
        updateCoefficients();
    }
}

void AudioFilter::updateCoefficients()
{
    switch (currentType)
    {
        case FilterType::Lowpass:
            coefficients = Coefficients::makeLowPass(sampleRate, frequency, qFactor);
            break;

        case FilterType::Highpass:
            coefficients = Coefficients::makeHighPass(sampleRate, frequency, qFactor);
            break;

        case FilterType::Bandpass:
            coefficients = Coefficients::makeBandPass(sampleRate, frequency, qFactor);
            break;

        case FilterType::Notch:
            coefficients = Coefficients::makeNotch(sampleRate, frequency, qFactor);
            break;

        case FilterType::LowShelf:
            coefficients = Coefficients::makeLowShelf(sampleRate, frequency, qFactor,
                                                       juce::Decibels::decibelsToGain(gainDb));
            break;

        case FilterType::HighShelf:
            coefficients = Coefficients::makeHighShelf(sampleRate, frequency, qFactor,
                                                        juce::Decibels::decibelsToGain(gainDb));
            break;

        case FilterType::Peak:
            coefficients = Coefficients::makePeakFilter(sampleRate, frequency, qFactor,
                                                         juce::Decibels::decibelsToGain(gainDb));
            break;
    }

    for (auto& filter : filters)
    {
        *filter.coefficients = *coefficients;
    }
}

void AudioFilter::process(juce::AudioBuffer<float>& buffer)
{
    if (!filterEnabled || !isPrepared)
        return;

    for (int channel = 0; channel < buffer.getNumChannels() && channel < 2; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = filters[channel].processSample(channelData[sample]);
        }
    }
}

float AudioFilter::processSample(int channel, float sample)
{
    if (!filterEnabled || !isPrepared || channel >= 2)
        return sample;

    return filters[channel].processSample(sample);
}

float AudioFilter::getMagnitudeForFrequency(float freq) const
{
    if (coefficients == nullptr)
        return 1.0f;

    return coefficients->getMagnitudeForFrequency(freq, sampleRate);
}

void AudioFilter::getMagnitudeForFrequencyArray(const float* frequencies, float* magnitudes, int numPoints) const
{
    if (coefficients == nullptr)
    {
        for (int i = 0; i < numPoints; ++i)
            magnitudes[i] = 1.0f;
        return;
    }

    for (int i = 0; i < numPoints; ++i)
    {
        magnitudes[i] = coefficients->getMagnitudeForFrequency((double)frequencies[i], sampleRate);
    }
}

//==============================================================================
// ParametricEQ Implementation
//==============================================================================

ParametricEQ::ParametricEQ()
{
    // Initialize default band frequencies
    bands[0] = { 100.0f, 0.0f, 1.0f, true };   // Low band
    bands[1] = { 1000.0f, 0.0f, 1.0f, true };  // Mid band
    bands[2] = { 8000.0f, 0.0f, 1.0f, true };  // High band

    for (int i = 0; i < numBands; ++i)
    {
        bandCoefficients[i] = Coefficients::makePeakFilter(44100.0, bands[i].frequency,
                                                            bands[i].q, 1.0f);
    }
}

void ParametricEQ::prepare(double rate, int samplesPerBlock, int numChannels)
{
    sampleRate = rate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = rate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)numChannels;

    for (int band = 0; band < numBands; ++band)
    {
        for (auto& filter : bandFilters[band])
        {
            filter.prepare(spec);
        }
        updateBandCoefficients(band);
    }

    isPrepared = true;
}

void ParametricEQ::reset()
{
    for (int band = 0; band < numBands; ++band)
    {
        for (auto& filter : bandFilters[band])
        {
            filter.reset();
        }
    }
}

void ParametricEQ::setBand(int bandIndex, float frequency, float gainDb, float q)
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return;

    bands[bandIndex].frequency = juce::jlimit(20.0f, 20000.0f, frequency);
    bands[bandIndex].gain = juce::jlimit(-24.0f, 24.0f, gainDb);
    bands[bandIndex].q = juce::jlimit(0.1f, 10.0f, q);

    updateBandCoefficients(bandIndex);
}

void ParametricEQ::setBandFrequency(int bandIndex, float freq)
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return;

    bands[bandIndex].frequency = juce::jlimit(20.0f, 20000.0f, freq);
    updateBandCoefficients(bandIndex);
}

void ParametricEQ::setBandGain(int bandIndex, float gain)
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return;

    bands[bandIndex].gain = juce::jlimit(-24.0f, 24.0f, gain);
    updateBandCoefficients(bandIndex);
}

void ParametricEQ::setBandQ(int bandIndex, float q)
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return;

    bands[bandIndex].q = juce::jlimit(0.1f, 10.0f, q);
    updateBandCoefficients(bandIndex);
}

void ParametricEQ::setBandEnabled(int bandIndex, bool enabled)
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return;

    bands[bandIndex].enabled = enabled;
}

ParametricEQ::Band ParametricEQ::getBand(int bandIndex) const
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return Band();

    return bands[bandIndex];
}

void ParametricEQ::updateBandCoefficients(int bandIndex)
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return;

    const auto& band = bands[bandIndex];
    float gainLinear = juce::Decibels::decibelsToGain(band.gain);

    bandCoefficients[bandIndex] = Coefficients::makePeakFilter(
        sampleRate, band.frequency, band.q, gainLinear);

    for (auto& filter : bandFilters[bandIndex])
    {
        *filter.coefficients = *bandCoefficients[bandIndex];
    }
}

void ParametricEQ::process(juce::AudioBuffer<float>& buffer)
{
    if (!eqEnabled || !isPrepared)
        return;

    for (int channel = 0; channel < buffer.getNumChannels() && channel < 2; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float value = channelData[sample];

            for (int band = 0; band < numBands; ++band)
            {
                if (bands[band].enabled)
                {
                    value = bandFilters[band][channel].processSample(value);
                }
            }

            channelData[sample] = value;
        }
    }
}

float ParametricEQ::getMagnitudeForFrequency(float freq) const
{
    float totalMagnitude = 1.0f;

    for (int band = 0; band < numBands; ++band)
    {
        if (bands[band].enabled && bandCoefficients[band] != nullptr)
        {
            totalMagnitude *= bandCoefficients[band]->getMagnitudeForFrequency(freq, sampleRate);
        }
    }

    return totalMagnitude;
}

void ParametricEQ::getMagnitudeForFrequencyArray(const float* frequencies, float* magnitudes, int numPoints) const
{
    for (int i = 0; i < numPoints; ++i)
    {
        magnitudes[i] = getMagnitudeForFrequency(frequencies[i]);
    }
}

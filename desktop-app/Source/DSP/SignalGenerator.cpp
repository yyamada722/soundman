/*
  ==============================================================================

    SignalGenerator.cpp

    Tone and noise signal generator implementation

  ==============================================================================
*/

#include "SignalGenerator.h"

//==============================================================================
// ToneGenerator Implementation
//==============================================================================

ToneGenerator::ToneGenerator()
{
    updatePhaseIncrement();
}

void ToneGenerator::prepare(double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;
    updatePhaseIncrement();
}

void ToneGenerator::reset()
{
    phase = 0.0;
}

void ToneGenerator::setFrequency(float freq)
{
    frequency = juce::jlimit(20.0f, 20000.0f, freq);
    updatePhaseIncrement();
}

void ToneGenerator::setAmplitude(float amp)
{
    amplitude = juce::jlimit(0.0f, 1.0f, amp);
}

void ToneGenerator::setWaveform(Waveform waveform)
{
    currentWaveform = waveform;
}

void ToneGenerator::updatePhaseIncrement()
{
    phaseIncrement = frequency / sampleRate;
}

void ToneGenerator::process(juce::AudioBuffer<float>& buffer)
{
    if (!isEnabled)
        return;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float value = getNextSample();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.addSample(channel, sample, value);
        }
    }
}

float ToneGenerator::getNextSample()
{
    float sample = 0.0f;

    switch (currentWaveform)
    {
        case Waveform::Sine:     sample = generateSine(); break;
        case Waveform::Square:   sample = generateSquare(); break;
        case Waveform::Triangle: sample = generateTriangle(); break;
        case Waveform::Sawtooth: sample = generateSawtooth(); break;
    }

    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return sample * amplitude;
}

float ToneGenerator::generateSine()
{
    return std::sin(phase * juce::MathConstants<double>::twoPi);
}

float ToneGenerator::generateSquare()
{
    return phase < 0.5 ? 1.0f : -1.0f;
}

float ToneGenerator::generateTriangle()
{
    if (phase < 0.25)
        return (float)(phase * 4.0);
    else if (phase < 0.75)
        return (float)(1.0 - (phase - 0.25) * 4.0);
    else
        return (float)(-1.0 + (phase - 0.75) * 4.0);
}

float ToneGenerator::generateSawtooth()
{
    return (float)(2.0 * phase - 1.0);
}

//==============================================================================
// NoiseGenerator Implementation
//==============================================================================

NoiseGenerator::NoiseGenerator()
    : rng(std::random_device{}())
{
    reset();
}

void NoiseGenerator::prepare(double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;
}

void NoiseGenerator::reset()
{
    std::fill(std::begin(pinkNoiseState), std::end(pinkNoiseState), 0.0f);
    pinkNoiseIndex = 0;
    pinkNoiseRunningSum = 0.0f;
    brownNoiseState = 0.0f;
}

void NoiseGenerator::setNoiseType(NoiseType type)
{
    if (currentNoiseType != type)
    {
        currentNoiseType = type;
        reset();
    }
}

void NoiseGenerator::setAmplitude(float amp)
{
    amplitude = juce::jlimit(0.0f, 1.0f, amp);
}

void NoiseGenerator::process(juce::AudioBuffer<float>& buffer)
{
    if (!isEnabled)
        return;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float value = getNextSample();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.addSample(channel, sample, value);
        }
    }
}

float NoiseGenerator::getNextSample()
{
    float sample = 0.0f;

    switch (currentNoiseType)
    {
        case NoiseType::White: sample = generateWhiteNoise(); break;
        case NoiseType::Pink:  sample = generatePinkNoise(); break;
        case NoiseType::Brown: sample = generateBrownNoise(); break;
    }

    return sample * amplitude;
}

float NoiseGenerator::generateWhiteNoise()
{
    return distribution(rng);
}

float NoiseGenerator::generatePinkNoise()
{
    // Voss-McCartney algorithm for pink noise
    float white = distribution(rng);

    // Update one of the rows based on the current index
    int numZeros = 0;
    int n = pinkNoiseIndex;
    while ((n & 1) == 0 && numZeros < pinkNoiseRows - 1)
    {
        numZeros++;
        n >>= 1;
    }

    pinkNoiseRunningSum -= pinkNoiseState[numZeros];
    pinkNoiseState[numZeros] = white;
    pinkNoiseRunningSum += white;

    pinkNoiseIndex = (pinkNoiseIndex + 1) & ((1 << pinkNoiseRows) - 1);

    // Normalize (approximately)
    return (pinkNoiseRunningSum + white) / (float)(pinkNoiseRows + 1) * 3.0f;
}

float NoiseGenerator::generateBrownNoise()
{
    // Brown noise (Brownian motion / random walk)
    float white = distribution(rng);
    brownNoiseState += white * 0.02f;

    // Keep in range with soft limiting
    if (brownNoiseState > 1.0f)
        brownNoiseState = 1.0f;
    else if (brownNoiseState < -1.0f)
        brownNoiseState = -1.0f;

    return brownNoiseState;
}

//==============================================================================
// SweepGenerator Implementation
//==============================================================================

SweepGenerator::SweepGenerator()
{
    calculateSweepRate();
}

void SweepGenerator::prepare(double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;
    calculateSweepRate();
}

void SweepGenerator::reset()
{
    phase = 0.0;
    currentSample = 0.0;
    currentFrequency = startFrequency;
}

void SweepGenerator::setEnabled(bool enabled)
{
    if (enabled && !isEnabled)
    {
        reset();
    }
    isEnabled = enabled;
}

void SweepGenerator::calculateSweepRate()
{
    totalSamples = duration * sampleRate;

    if (sweepType == SweepType::Logarithmic)
    {
        logStartFreq = std::log(startFrequency);
        logEndFreq = std::log(endFrequency);
        logSweepRate = (logEndFreq - logStartFreq) / totalSamples;
    }
    else
    {
        linearSweepRate = (endFrequency - startFrequency) / totalSamples;
    }
}

void SweepGenerator::updateCurrentFrequency()
{
    if (sweepType == SweepType::Logarithmic)
    {
        currentFrequency = (float)std::exp(logStartFreq + logSweepRate * currentSample);
    }
    else
    {
        currentFrequency = startFrequency + (float)(linearSweepRate * currentSample);
    }
}

float SweepGenerator::getProgress() const
{
    if (totalSamples <= 0)
        return 0.0f;
    return (float)(currentSample / totalSamples);
}

void SweepGenerator::process(juce::AudioBuffer<float>& buffer)
{
    if (!isEnabled)
        return;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float value = getNextSample();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.addSample(channel, sample, value);
        }
    }
}

float SweepGenerator::getNextSample()
{
    if (currentSample >= totalSamples)
    {
        if (isEnabled && onSweepComplete)
        {
            isEnabled = false;
            juce::MessageManager::callAsync([this]() {
                if (onSweepComplete)
                    onSweepComplete();
            });
        }
        return 0.0f;
    }

    updateCurrentFrequency();

    // Generate sine wave at current frequency
    float sample = std::sin(phase * juce::MathConstants<double>::twoPi) * amplitude;

    // Update phase
    double phaseIncrement = currentFrequency / sampleRate;
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    currentSample++;

    return sample;
}

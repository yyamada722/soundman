/*
  ==============================================================================

    PitchDetector.cpp

    Real-time pitch detection using YIN algorithm implementation

  ==============================================================================
*/

#include "PitchDetector.h"

//==============================================================================
PitchDetector::PitchDetector()
{
    inputBuffer.resize(bufferSize, 0.0f);
    yinBuffer.resize(bufferSize / 2, 0.0f);
    updateLagRange();
}

//==============================================================================
void PitchDetector::setSampleRate(double rate)
{
    sampleRate = rate;
    updateLagRange();
}

void PitchDetector::updateLagRange()
{
    // Calculate lag range based on frequency range
    // lag = sampleRate / frequency
    minLag = (int)(sampleRate / maxFrequency);
    maxLag = (int)(sampleRate / minFrequency);

    // Clamp to buffer size
    maxLag = juce::jmin(maxLag, bufferSize / 2 - 1);
    minLag = juce::jmax(minLag, 2);
}

//==============================================================================
void PitchDetector::pushSample(float sample)
{
    inputBuffer[writeIndex] = sample;
    writeIndex = (writeIndex + 1) % bufferSize;
    samplesCollected++;

    // Process when we have enough samples (every bufferSize/8 samples for faster response)
    if (samplesCollected >= bufferSize / 8)
    {
        samplesCollected = 0;

        // Create a contiguous buffer for processing
        std::vector<float> processBuffer(bufferSize);
        float maxLevel = 0.0f;
        for (int i = 0; i < bufferSize; ++i)
        {
            processBuffer[i] = inputBuffer[(writeIndex + i) % bufferSize];
            maxLevel = juce::jmax(maxLevel, std::abs(processBuffer[i]));
        }

        // Only process if there's enough signal (very low threshold for sensitivity)
        if (maxLevel > 0.001f)
        {
            latestPitch = detectPitch(processBuffer.data(), bufferSize);
        }
        else
        {
            latestPitch = PitchResult();  // No signal, clear pitch
        }
    }
}

//==============================================================================
PitchDetector::PitchResult PitchDetector::detectPitch(const float* samples, int numSamples)
{
    PitchResult result;

    if (numSamples < maxLag * 2)
        return result;

    // Copy samples to working buffer
    std::vector<float> buffer(samples, samples + numSamples);

    // Step 1: Calculate difference function
    std::fill(yinBuffer.begin(), yinBuffer.end(), 0.0f);

    for (int tau = 1; tau < (int)yinBuffer.size(); ++tau)
    {
        yinBuffer[tau] = yinDifference(buffer, tau);
    }

    // Step 2: Cumulative mean normalized difference
    cumulativeMeanNormalizedDifference(yinBuffer);

    // Step 3: Absolute threshold
    int tauEstimate = absoluteThreshold(yinBuffer);

    if (tauEstimate == -1)
    {
        // No pitch detected
        result.isPitched = false;
        return result;
    }

    // Step 4: Parabolic interpolation for better accuracy
    float betterTau = parabolicInterpolation(yinBuffer, tauEstimate);

    // Calculate frequency
    float frequency = (float)sampleRate / betterTau;

    // Validate frequency range
    if (frequency < minFrequency || frequency > maxFrequency)
    {
        result.isPitched = false;
        return result;
    }

    // Calculate confidence (inverse of YIN value at detected tau)
    float confidence = 1.0f - yinBuffer[tauEstimate];
    confidence = juce::jlimit(0.0f, 1.0f, confidence);

    // Fill result
    result.frequency = frequency;
    result.confidence = confidence;
    result.isPitched = true;
    result.midiNote = frequencyToMidiNote(frequency);
    result.noteName = frequencyToNoteName(frequency);
    result.cents = getCentsDeviation(frequency, result.midiNote);

    return result;
}

//==============================================================================
float PitchDetector::yinDifference(const std::vector<float>& buffer, int tau)
{
    float sum = 0.0f;
    int windowSize = (int)buffer.size() / 2;

    for (int i = 0; i < windowSize; ++i)
    {
        float delta = buffer[i] - buffer[i + tau];
        sum += delta * delta;
    }

    return sum;
}

void PitchDetector::cumulativeMeanNormalizedDifference(std::vector<float>& buffer)
{
    buffer[0] = 1.0f;
    float runningSum = 0.0f;

    for (int tau = 1; tau < (int)buffer.size(); ++tau)
    {
        runningSum += buffer[tau];

        if (runningSum == 0.0f)
            buffer[tau] = 1.0f;
        else
            buffer[tau] = buffer[tau] * tau / runningSum;
    }
}

int PitchDetector::absoluteThreshold(const std::vector<float>& buffer)
{
    // Find the first tau where the CMND is below threshold
    int bestTau = -1;
    float bestValue = threshold;

    for (int tau = minLag; tau < maxLag; ++tau)
    {
        if (buffer[tau] < threshold)
        {
            // Find local minimum
            while (tau + 1 < maxLag && buffer[tau + 1] < buffer[tau])
            {
                ++tau;
            }
            return tau;
        }

        // Track the best (lowest) value even if above threshold
        if (buffer[tau] < bestValue)
        {
            bestValue = buffer[tau];
            bestTau = tau;
        }
    }

    // If no value below threshold but we found a good minimum, use it
    // This helps detect pitch in noisier signals
    if (bestTau > 0 && bestValue < 0.6f)
    {
        // Find the local minimum around bestTau
        int tau = bestTau;
        while (tau + 1 < maxLag && buffer[tau + 1] < buffer[tau])
        {
            ++tau;
        }
        return tau;
    }

    // No pitch detected
    return -1;
}

float PitchDetector::parabolicInterpolation(const std::vector<float>& buffer, int tauEstimate)
{
    if (tauEstimate <= 0 || tauEstimate >= (int)buffer.size() - 1)
        return (float)tauEstimate;

    float s0 = buffer[tauEstimate - 1];
    float s1 = buffer[tauEstimate];
    float s2 = buffer[tauEstimate + 1];

    float adjustment = (s2 - s0) / (2.0f * (2.0f * s1 - s2 - s0));

    if (std::abs(adjustment) > 1.0f)
        adjustment = 0.0f;

    return (float)tauEstimate + adjustment;
}

//==============================================================================
juce::String PitchDetector::frequencyToNoteName(float frequency)
{
    if (frequency <= 0.0f)
        return "---";

    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    int midiNote = frequencyToMidiNote(frequency);

    if (midiNote < 0 || midiNote > 127)
        return "---";

    int noteIndex = midiNote % 12;
    int octave = (midiNote / 12) - 1;

    return juce::String(noteNames[noteIndex]) + juce::String(octave);
}

int PitchDetector::frequencyToMidiNote(float frequency)
{
    if (frequency <= 0.0f)
        return -1;

    // MIDI note 69 = A4 = 440 Hz
    float midiNote = 69.0f + 12.0f * std::log2(frequency / 440.0f);
    return juce::roundToInt(midiNote);
}

float PitchDetector::midiNoteToFrequency(int midiNote)
{
    // MIDI note 69 = A4 = 440 Hz
    return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}

float PitchDetector::getCentsDeviation(float frequency, int midiNote)
{
    if (frequency <= 0.0f || midiNote < 0)
        return 0.0f;

    float exactMidiNote = 69.0f + 12.0f * std::log2(frequency / 440.0f);
    float cents = (exactMidiNote - (float)midiNote) * 100.0f;

    return cents;
}

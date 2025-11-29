/*
  ==============================================================================

    BPMDetector.cpp

    Beat/Tempo detection implementation using onset detection and autocorrelation

  ==============================================================================
*/

#include "BPMDetector.h"
#include <cmath>

//==============================================================================
BPMDetector::BPMDetector()
{
    fftData.resize(fftSize * 2, 0.0f);
    prevSpectrum.resize(fftSize / 2, 0.0f);
    currentSpectrum.resize(fftSize / 2, 0.0f);
    onsetStrength.resize(onsetBufferSize, 0.0f);
    autocorrelation.resize(onsetBufferSize / 2, 0.0f);
    inputBuffer.resize(fftSize, 0.0f);
}

//==============================================================================
void BPMDetector::prepare(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    blockSize = samplesPerBlock;
    hopSize = fftSize / 2;  // 50% overlap
    reset();
}

void BPMDetector::reset()
{
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    std::fill(prevSpectrum.begin(), prevSpectrum.end(), 0.0f);
    std::fill(currentSpectrum.begin(), currentSpectrum.end(), 0.0f);
    std::fill(onsetStrength.begin(), onsetStrength.end(), 0.0f);
    std::fill(autocorrelation.begin(), autocorrelation.end(), 0.0f);
    std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);

    onsetWritePos = 0;
    sampleCounter = 0;
    currentBPM = 0.0f;
    smoothedBPM = 0.0f;
    confidence = 0.0f;
    beatDetected = false;
    lastOnsetValue = 0.0f;
    samplesSinceLastBeat = 0;
}

//==============================================================================
void BPMDetector::processBlock(const juce::AudioBuffer<float>& buffer)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Mix to mono and accumulate into input buffer
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            sample += buffer.getSample(ch, i);
        sample /= numChannels;

        inputBuffer[sampleCounter] = sample;
        sampleCounter++;

        if (sampleCounter >= hopSize)
        {
            computeOnsetStrength(buffer);
            sampleCounter = 0;

            // Shift input buffer
            std::copy(inputBuffer.begin() + hopSize, inputBuffer.end(), inputBuffer.begin());
        }
    }

    samplesSinceLastBeat += numSamples;
    detectBeat();
}

//==============================================================================
void BPMDetector::computeOnsetStrength(const juce::AudioBuffer<float>& /*buffer*/)
{
    // Apply Hann window and prepare FFT data
    for (int i = 0; i < fftSize; ++i)
    {
        float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1)));
        fftData[i] = inputBuffer[i] * window;
    }

    // Zero padding for imaginary part
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);

    // Perform FFT
    fft.performRealOnlyForwardTransform(fftData.data());

    // Compute magnitude spectrum
    for (int i = 0; i < fftSize / 2; ++i)
    {
        float real = fftData[i * 2];
        float imag = fftData[i * 2 + 1];
        currentSpectrum[i] = std::sqrt(real * real + imag * imag);
    }

    // Compute spectral flux (half-wave rectified difference)
    float flux = 0.0f;
    for (int i = 0; i < fftSize / 2; ++i)
    {
        float diff = currentSpectrum[i] - prevSpectrum[i];
        if (diff > 0)
            flux += diff;
    }

    // Normalize
    flux /= (fftSize / 2);

    // Store onset strength
    onsetStrength[onsetWritePos] = flux;
    onsetWritePos = (onsetWritePos + 1) % onsetBufferSize;

    // Copy current to previous
    std::copy(currentSpectrum.begin(), currentSpectrum.end(), prevSpectrum.begin());

    // Compute autocorrelation and BPM every 2 frames for faster response
    static int updateCounter = 0;
    updateCounter++;
    if (updateCounter >= 2)
    {
        updateCounter = 0;
        computeAutocorrelation();
        float detectedBPM = findBPMFromAutocorrelation();

        if (detectedBPM > 0)
        {
            // Smooth the BPM estimate
            if (smoothedBPM == 0.0f)
                smoothedBPM = detectedBPM;
            else
                smoothedBPM = smoothedBPM * (1.0f - bpmSmoothingFactor) + detectedBPM * bpmSmoothingFactor;

            currentBPM = std::round(smoothedBPM);
        }
    }
}

//==============================================================================
void BPMDetector::computeAutocorrelation()
{
    int n = onsetBufferSize;
    int maxLag = n / 2;

    // Compute mean
    float mean = 0.0f;
    for (int i = 0; i < n; ++i)
        mean += onsetStrength[i];
    mean /= n;

    // Compute autocorrelation
    for (int lag = 0; lag < maxLag; ++lag)
    {
        float sum = 0.0f;
        float norm1 = 0.0f;
        float norm2 = 0.0f;

        for (int i = 0; i < n - lag; ++i)
        {
            float v1 = onsetStrength[i] - mean;
            float v2 = onsetStrength[i + lag] - mean;
            sum += v1 * v2;
            norm1 += v1 * v1;
            norm2 += v2 * v2;
        }

        if (norm1 > 0 && norm2 > 0)
            autocorrelation[lag] = sum / std::sqrt(norm1 * norm2);
        else
            autocorrelation[lag] = 0.0f;
    }
}

//==============================================================================
float BPMDetector::findBPMFromAutocorrelation()
{
    // Calculate frame rate (onset frames per second)
    float frameRate = (float)sampleRate / hopSize;

    // Convert BPM range to lag range
    int minLag = (int)(frameRate * 60.0f / maxBPM);
    int maxLag = (int)(frameRate * 60.0f / minBPM);

    minLag = juce::jmax(1, minLag);
    maxLag = juce::jmin((int)autocorrelation.size() - 1, maxLag);

    if (minLag >= maxLag)
        return 0.0f;

    // Find peak in autocorrelation
    float maxCorr = 0.0f;
    int bestLag = 0;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        // Weight towards musically common tempos (around 120 BPM)
        float bpm = frameRate * 60.0f / lag;
        float weight = 1.0f;

        // Prefer tempos near 120 BPM
        float distFrom120 = std::abs(bpm - 120.0f);
        weight = 1.0f / (1.0f + distFrom120 * 0.01f);

        // Also prefer octave-related tempos
        float bpmNormalized = bpm;
        while (bpmNormalized > 160) bpmNormalized /= 2;
        while (bpmNormalized < 80) bpmNormalized *= 2;

        float corr = autocorrelation[lag] * weight;

        if (corr > maxCorr)
        {
            maxCorr = corr;
            bestLag = lag;
        }
    }

    if (bestLag > 0 && maxCorr > 0.1f)
    {
        confidence = juce::jlimit(0.0f, 1.0f, maxCorr);
        return frameRate * 60.0f / bestLag;
    }

    confidence = 0.0f;
    return 0.0f;
}

//==============================================================================
void BPMDetector::detectBeat()
{
    // Get current onset value
    int currentIdx = (onsetWritePos - 1 + onsetBufferSize) % onsetBufferSize;
    float currentOnset = onsetStrength[currentIdx];

    // Compute local average for adaptive threshold
    float localAvg = 0.0f;
    int windowSize = 8;
    for (int i = 0; i < windowSize; ++i)
    {
        int idx = (currentIdx - i + onsetBufferSize) % onsetBufferSize;
        localAvg += onsetStrength[idx];
    }
    localAvg /= windowSize;

    // Beat detected if current onset exceeds threshold
    float threshold = localAvg * beatThreshold;

    // Also require minimum time between beats (based on max BPM)
    float minBeatInterval = sampleRate * 60.0f / maxBPM * 0.8f;

    if (currentOnset > threshold && currentOnset > lastOnsetValue &&
        samplesSinceLastBeat > minBeatInterval)
    {
        beatDetected = true;
        samplesSinceLastBeat = 0;
    }
    else
    {
        beatDetected = false;
    }

    lastOnsetValue = currentOnset;
}

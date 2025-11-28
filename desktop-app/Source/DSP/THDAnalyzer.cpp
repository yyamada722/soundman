/*
  ==============================================================================

    THDAnalyzer.cpp

    THD and SNR analyzer implementation

  ==============================================================================
*/

#include "THDAnalyzer.h"
#include <cmath>

//==============================================================================
THDAnalyzer::THDAnalyzer()
{
    inputBuffer.resize(fftSize, 0.0f);
    fftData.resize(fftSize * 2, 0.0f);
    magnitudeSpectrum.resize(fftSize / 2, 0.0f);
}

void THDAnalyzer::prepare(double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;
    reset();
}

void THDAnalyzer::reset()
{
    std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    std::fill(magnitudeSpectrum.begin(), magnitudeSpectrum.end(), 0.0f);
    writeIndex = 0;
    samplesCollected = 0;
    latestResult = MeasurementResult();
}

void THDAnalyzer::pushSample(float sample)
{
    inputBuffer[writeIndex] = sample;
    writeIndex = (writeIndex + 1) % fftSize;
    samplesCollected++;

    // Analyze when buffer is full
    if (samplesCollected >= fftSize)
    {
        samplesCollected = 0;
        analyze();
    }
}

void THDAnalyzer::analyze()
{
    MeasurementResult result;

    // Copy to FFT buffer in correct order (circular buffer unwrap)
    for (int i = 0; i < fftSize; ++i)
    {
        fftData[i] = inputBuffer[(writeIndex + i) % fftSize];
    }

    // Apply window function
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    // Perform FFT
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Calculate magnitude spectrum
    float totalPower = 0.0f;
    for (int i = 0; i < fftSize / 2; ++i)
    {
        float magnitude = fftData[i];
        magnitudeSpectrum[i] = magnitude;
        totalPower += magnitude * magnitude;
    }

    // Find fundamental frequency
    int fundamentalBin = findFundamentalBin();

    if (fundamentalBin <= 0 || fundamentalBin >= fftSize / 2 - 1)
    {
        latestResult = result;
        return;
    }

    // Calculate exact fundamental frequency using parabolic interpolation
    float peakMag = magnitudeSpectrum[fundamentalBin];
    float prevMag = magnitudeSpectrum[fundamentalBin - 1];
    float nextMag = magnitudeSpectrum[fundamentalBin + 1];

    float delta = 0.5f * (prevMag - nextMag) / (prevMag - 2.0f * peakMag + nextMag);
    float exactBin = fundamentalBin + delta;

    result.fundamentalFrequency = (float)(exactBin * sampleRate / fftSize);
    float fundamentalMag = getInterpolatedAmplitude(exactBin);
    result.fundamentalAmplitude = 20.0f * std::log10(fundamentalMag + 1e-10f);

    // Calculate power of fundamental and harmonics
    float fundamentalPower = fundamentalMag * fundamentalMag;
    float harmonicPower = 0.0f;

    result.harmonicLevels.resize(numHarmonics);
    result.harmonicLevels[0] = result.fundamentalAmplitude;  // First "harmonic" is fundamental

    // Measure each harmonic
    int binWidth = 3;  // Bins to consider around each harmonic
    for (int h = 2; h <= numHarmonics; ++h)
    {
        float harmonicBin = exactBin * h;
        int centerBin = (int)std::round(harmonicBin);

        if (centerBin >= fftSize / 2 - binWidth)
            break;

        // Find peak near expected harmonic frequency
        float maxMag = 0.0f;
        for (int b = centerBin - binWidth; b <= centerBin + binWidth; ++b)
        {
            if (b > 0 && b < fftSize / 2)
            {
                if (magnitudeSpectrum[b] > maxMag)
                    maxMag = magnitudeSpectrum[b];
            }
        }

        float harmonicMagDb = 20.0f * std::log10(maxMag + 1e-10f);
        result.harmonicLevels[h - 1] = harmonicMagDb;
        harmonicPower += maxMag * maxMag;
    }

    // Calculate THD (%)
    if (fundamentalPower > 0)
    {
        result.thd = 100.0f * std::sqrt(harmonicPower / fundamentalPower);
    }

    // Calculate noise power (total - fundamental - harmonics)
    float signalPower = fundamentalPower + harmonicPower;
    float noisePower = totalPower - signalPower;
    if (noisePower < 0) noisePower = 0;

    // Calculate THD+N (%)
    if (fundamentalPower > 0)
    {
        result.thdPlusNoise = 100.0f * std::sqrt((harmonicPower + noisePower) / fundamentalPower);
    }

    // Calculate SNR (dB)
    if (noisePower > 0)
    {
        result.snr = 10.0f * std::log10(fundamentalPower / noisePower);
    }
    else
    {
        result.snr = 120.0f;  // Very high SNR
    }

    // Calculate SINAD (dB) - Signal to Noise and Distortion
    float distortionAndNoise = harmonicPower + noisePower;
    if (distortionAndNoise > 0)
    {
        result.sinad = 10.0f * std::log10(fundamentalPower / distortionAndNoise);
    }
    else
    {
        result.sinad = 120.0f;
    }

    result.isValid = true;
    latestResult = result;
}

int THDAnalyzer::findFundamentalBin()
{
    // If expected fundamental is set, search near it
    int expectedBin = (int)(expectedFundamental * fftSize / sampleRate);
    int searchRange = expectedBin / 2;  // Search +/- 50% of expected

    int startBin = juce::jmax(1, expectedBin - searchRange);
    int endBin = juce::jmin(fftSize / 2 - 1, expectedBin + searchRange);

    // Find the bin with maximum magnitude
    int maxBin = startBin;
    float maxMag = magnitudeSpectrum[startBin];

    for (int i = startBin + 1; i <= endBin; ++i)
    {
        if (magnitudeSpectrum[i] > maxMag)
        {
            maxMag = magnitudeSpectrum[i];
            maxBin = i;
        }
    }

    // Verify it's a peak (has local maximum)
    if (maxBin > 0 && maxBin < fftSize / 2 - 1)
    {
        if (magnitudeSpectrum[maxBin] > magnitudeSpectrum[maxBin - 1] &&
            magnitudeSpectrum[maxBin] > magnitudeSpectrum[maxBin + 1])
        {
            return maxBin;
        }
    }

    return -1;
}

float THDAnalyzer::getBinAmplitude(int bin)
{
    if (bin < 0 || bin >= fftSize / 2)
        return 0.0f;
    return magnitudeSpectrum[bin];
}

float THDAnalyzer::getInterpolatedAmplitude(float exactBin)
{
    int bin = (int)exactBin;
    float frac = exactBin - bin;

    if (bin < 0 || bin >= fftSize / 2 - 1)
        return 0.0f;

    // Simple linear interpolation
    return magnitudeSpectrum[bin] * (1.0f - frac) + magnitudeSpectrum[bin + 1] * frac;
}

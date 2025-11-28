/*
  ==============================================================================

    MFCCAnalyzer.cpp

    MFCC analysis implementation

  ==============================================================================
*/

#include "MFCCAnalyzer.h"
#include <cmath>

//==============================================================================
MFCCAnalyzer::MFCCAnalyzer()
    : forwardFFT(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    fifo.resize(fftSize, 0.0f);
    fftData.resize(2 * fftSize, 0.0f);
    powerSpectrum.resize(fftSize / 2 + 1, 0.0f);

    initializeDCTMatrix();
    initializeMelFilterBank();
}

//==============================================================================
void MFCCAnalyzer::setSampleRate(double rate)
{
    if (std::abs(sampleRate - rate) > 1.0)
    {
        sampleRate = rate;
        filterBankInitialized = false;
        initializeMelFilterBank();
    }
}

void MFCCAnalyzer::setMinFrequency(float freq)
{
    if (std::abs(minFrequency - freq) > 1.0f)
    {
        minFrequency = freq;
        filterBankInitialized = false;
        initializeMelFilterBank();
    }
}

void MFCCAnalyzer::setMaxFrequency(float freq)
{
    if (std::abs(maxFrequency - freq) > 1.0f)
    {
        maxFrequency = freq;
        filterBankInitialized = false;
        initializeMelFilterBank();
    }
}

//==============================================================================
float MFCCAnalyzer::hzToMel(float hz)
{
    // HTK formula
    return 2595.0f * std::log10(1.0f + hz / 700.0f);
}

float MFCCAnalyzer::melToHz(float mel)
{
    return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
}

//==============================================================================
void MFCCAnalyzer::initializeMelFilterBank()
{
    melFilterBank.clear();
    filterBankStartBins.clear();
    filterBankEndBins.clear();

    float melMin = hzToMel(minFrequency);
    float melMax = hzToMel(maxFrequency);

    // Create equally spaced points in Mel scale
    std::vector<float> melPoints(numMelFilters + 2);
    for (int i = 0; i < numMelFilters + 2; ++i)
    {
        melPoints[i] = melMin + (melMax - melMin) * i / (numMelFilters + 1);
    }

    // Convert Mel points to Hz and then to FFT bins
    std::vector<int> binPoints(numMelFilters + 2);
    for (int i = 0; i < numMelFilters + 2; ++i)
    {
        float hz = melToHz(melPoints[i]);
        binPoints[i] = (int)std::floor((fftSize + 1) * hz / sampleRate);
    }

    // Create triangular filters
    int numBins = fftSize / 2 + 1;
    melFilterBank.resize(numMelFilters);
    filterBankStartBins.resize(numMelFilters);
    filterBankEndBins.resize(numMelFilters);

    for (int m = 0; m < numMelFilters; ++m)
    {
        int startBin = binPoints[m];
        int centerBin = binPoints[m + 1];
        int endBin = binPoints[m + 2];

        filterBankStartBins[m] = startBin;
        filterBankEndBins[m] = endBin;

        melFilterBank[m].resize(numBins, 0.0f);

        // Rising edge
        for (int k = startBin; k < centerBin && k < numBins; ++k)
        {
            if (centerBin != startBin)
                melFilterBank[m][k] = (float)(k - startBin) / (float)(centerBin - startBin);
        }

        // Falling edge
        for (int k = centerBin; k <= endBin && k < numBins; ++k)
        {
            if (endBin != centerBin)
                melFilterBank[m][k] = (float)(endBin - k) / (float)(endBin - centerBin);
        }
    }

    filterBankInitialized = true;
}

void MFCCAnalyzer::initializeDCTMatrix()
{
    // Type-II DCT matrix
    for (int i = 0; i < numMFCCs; ++i)
    {
        for (int j = 0; j < numMelFilters; ++j)
        {
            dctMatrix[i][j] = std::cos(juce::MathConstants<float>::pi * i * (j + 0.5f) / numMelFilters);
        }
    }
}

//==============================================================================
void MFCCAnalyzer::pushSample(float sample)
{
    fifo[fifoIndex++] = sample;

    if (fifoIndex >= fftSize)
    {
        fifoIndex = 0;
        processFFT();
    }
}

void MFCCAnalyzer::processFFT()
{
    if (!filterBankInitialized)
        initializeMelFilterBank();

    // Copy to FFT buffer
    std::copy(fifo.begin(), fifo.end(), fftData.begin());
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);

    // Apply window
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    // Perform FFT
    forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

    // Calculate power spectrum
    for (int i = 0; i <= fftSize / 2; ++i)
    {
        float magnitude = fftData[i] / (float)(fftSize / 2);
        powerSpectrum[i] = magnitude * magnitude;
    }

    // Compute MFCCs
    latestResult = analyze(nullptr, 0);  // Use internal power spectrum
}

MFCCAnalyzer::MFCCResult MFCCAnalyzer::analyze(const float* samples, int numSamples)
{
    MFCCResult result;

    if (!filterBankInitialized)
        return result;

    // If samples provided, compute FFT first
    if (samples != nullptr && numSamples > 0)
    {
        int samplesToUse = juce::jmin(numSamples, fftSize);

        std::fill(fftData.begin(), fftData.end(), 0.0f);
        std::copy(samples, samples + samplesToUse, fftData.begin());

        window.multiplyWithWindowingTable(fftData.data(), fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

        for (int i = 0; i <= fftSize / 2; ++i)
        {
            float magnitude = fftData[i] / (float)(fftSize / 2);
            powerSpectrum[i] = magnitude * magnitude;
        }
    }

    // Calculate total energy
    result.totalEnergy = 0.0f;
    for (int i = 0; i <= fftSize / 2; ++i)
    {
        result.totalEnergy += powerSpectrum[i];
    }

    // Check if signal is present
    if (result.totalEnergy < 1e-10f)
        return result;

    // Apply Mel filter bank
    applyMelFilterBank(powerSpectrum, result.melEnergies);

    // Take logarithm
    std::array<float, numMelFilters> logMelEnergies;
    for (int m = 0; m < numMelFilters; ++m)
    {
        // Add small epsilon to avoid log(0)
        logMelEnergies[m] = std::log(result.melEnergies[m] + 1e-10f);
    }

    // Apply DCT
    computeDCT(logMelEnergies, result.coefficients);

    result.isValid = true;
    return result;
}

void MFCCAnalyzer::applyMelFilterBank(const std::vector<float>& spectrum, std::array<float, numMelFilters>& melEnergies)
{
    for (int m = 0; m < numMelFilters; ++m)
    {
        float energy = 0.0f;
        int startBin = filterBankStartBins[m];
        int endBin = juce::jmin(filterBankEndBins[m], (int)spectrum.size() - 1);

        for (int k = startBin; k <= endBin; ++k)
        {
            energy += spectrum[k] * melFilterBank[m][k];
        }

        melEnergies[m] = energy;
    }
}

void MFCCAnalyzer::computeDCT(const std::array<float, numMelFilters>& melLogEnergies, std::array<float, numMFCCs>& mfccs)
{
    for (int i = 0; i < numMFCCs; ++i)
    {
        float sum = 0.0f;
        for (int j = 0; j < numMelFilters; ++j)
        {
            sum += melLogEnergies[j] * dctMatrix[i][j];
        }

        // Normalization
        if (i == 0)
            mfccs[i] = sum * std::sqrt(1.0f / numMelFilters);
        else
            mfccs[i] = sum * std::sqrt(2.0f / numMelFilters);
    }
}

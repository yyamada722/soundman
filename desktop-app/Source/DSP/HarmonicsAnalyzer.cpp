/*
  ==============================================================================

    HarmonicsAnalyzer.cpp

    Harmonic analysis implementation

  ==============================================================================
*/

#include "HarmonicsAnalyzer.h"
#include <cmath>

//==============================================================================
HarmonicsAnalyzer::HarmonicsAnalyzer()
    : forwardFFT(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    fifo.resize(fftSize, 0.0f);
    fftData.resize(2 * fftSize, 0.0f);
    magnitudes.resize(fftSize / 2, 0.0f);
}

//==============================================================================
void HarmonicsAnalyzer::setSampleRate(double rate)
{
    sampleRate = rate;
}

void HarmonicsAnalyzer::setFundamentalFrequency(float freq)
{
    knownFundamental = freq;
}

//==============================================================================
void HarmonicsAnalyzer::pushSample(float sample)
{
    fifo[fifoIndex++] = sample;

    if (fifoIndex >= fftSize)
    {
        fifoIndex = 0;
        processFFT();
    }
}

void HarmonicsAnalyzer::processFFT()
{
    // Copy to FFT buffer
    std::copy(fifo.begin(), fifo.end(), fftData.begin());
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);

    // Apply window
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    // Perform FFT
    forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

    // Calculate magnitudes
    for (int i = 0; i < fftSize / 2; ++i)
    {
        magnitudes[i] = fftData[i] / (float)(fftSize / 2);
    }

    // Analyze
    latestResult = analyze(magnitudes.data(), fftSize / 2, knownFundamental);
}

//==============================================================================
HarmonicsAnalyzer::AnalysisResult HarmonicsAnalyzer::analyze(
    const float* fftMagnitudes, int numBins, float fundamentalHint)
{
    AnalysisResult result;

    // Copy magnitudes to local vector
    std::vector<float> mags(fftMagnitudes, fftMagnitudes + numBins);

    // Find fundamental frequency
    int fundamentalBin;
    if (fundamentalHint > 0.0f)
    {
        // Use hint - search around expected frequency
        float expectedBin = frequencyToBin(fundamentalHint);
        int searchStart = juce::jmax(1, (int)(expectedBin * 0.9f));
        int searchEnd = juce::jmin(numBins - 1, (int)(expectedBin * 1.1f));

        fundamentalBin = searchStart;
        float maxMag = mags[searchStart];
        for (int i = searchStart + 1; i <= searchEnd; ++i)
        {
            if (mags[i] > maxMag)
            {
                maxMag = mags[i];
                fundamentalBin = i;
            }
        }
    }
    else
    {
        fundamentalBin = findFundamental(mags);
    }

    if (fundamentalBin <= 0)
        return result;

    // Interpolate for better frequency resolution
    float exactBin = interpolatePeak(mags, fundamentalBin);
    float fundamental = binToFrequency(exactBin);

    // Check if amplitude is above threshold
    float fundamentalAmplitude = mags[fundamentalBin];
    float fundamentalDb = juce::Decibels::gainToDecibels(fundamentalAmplitude);

    if (fundamentalDb < minAmplitudeDb)
        return result;

    result.fundamentalFrequency = fundamental;
    result.fundamentalAmplitudeDb = fundamentalDb;
    result.isValid = true;

    // Find harmonics
    findHarmonics(mags, fundamental, result);

    // Calculate THD
    result.totalHarmonicDistortion = calculateTHD(result.harmonics, result.numHarmonicsDetected);

    return result;
}

//==============================================================================
int HarmonicsAnalyzer::findFundamental(const std::vector<float>& mags)
{
    // Find the strongest peak in the typical fundamental range (50Hz - 2kHz)
    int minBin = (int)frequencyToBin(50.0f);
    int maxBin = (int)frequencyToBin(2000.0f);

    minBin = juce::jmax(1, minBin);
    maxBin = juce::jmin((int)mags.size() - 1, maxBin);

    int bestBin = minBin;
    float bestMag = mags[minBin];

    for (int i = minBin + 1; i <= maxBin; ++i)
    {
        // Look for local maxima
        if (mags[i] > mags[i - 1] && mags[i] > mags[i + 1] && mags[i] > bestMag)
        {
            // Verify this is a true peak (not just noise)
            float avgNeighbor = (mags[i - 1] + mags[i + 1]) / 2.0f;
            if (mags[i] > avgNeighbor * 1.5f)  // Peak should be significantly higher
            {
                bestMag = mags[i];
                bestBin = i;
            }
        }
    }

    return bestBin;
}

void HarmonicsAnalyzer::findHarmonics(const std::vector<float>& mags, float fundamental, AnalysisResult& result)
{
    float searchWidthRatio = std::pow(2.0f, harmonicSearchWidthCents / 1200.0f);

    for (int h = 1; h <= maxHarmonics; ++h)
    {
        float expectedFreq = fundamental * h;
        float expectedBin = frequencyToBin(expectedFreq);

        // Check if within valid range
        if (expectedBin >= mags.size() - 1)
            break;

        // Search around expected position
        int searchStart = juce::jmax(1, (int)(expectedBin / searchWidthRatio));
        int searchEnd = juce::jmin((int)mags.size() - 2, (int)(expectedBin * searchWidthRatio));

        // Find peak in search window
        int peakBin = searchStart;
        float peakMag = mags[searchStart];

        for (int i = searchStart + 1; i <= searchEnd; ++i)
        {
            if (mags[i] > peakMag)
            {
                peakMag = mags[i];
                peakBin = i;
            }
        }

        // Check if peak is above threshold
        float peakDb = juce::Decibels::gainToDecibels(peakMag);

        Harmonic& harmonic = result.harmonics[h - 1];
        harmonic.number = h;

        if (peakDb >= minAmplitudeDb)
        {
            float exactBin = interpolatePeak(mags, peakBin);
            harmonic.frequency = binToFrequency(exactBin);
            harmonic.amplitude = peakMag;
            harmonic.amplitudeDb = peakDb;
            harmonic.detected = true;
            result.numHarmonicsDetected = h;

            // Calculate inharmonicity (deviation from perfect harmonic)
            float expectedPerfect = fundamental * h;
            float deviation = std::abs(harmonic.frequency - expectedPerfect) / expectedPerfect;
            result.inharmonicity += deviation;
        }
        else
        {
            harmonic.frequency = expectedFreq;
            harmonic.amplitude = 0.0f;
            harmonic.amplitudeDb = -100.0f;
            harmonic.detected = false;
        }
    }

    // Average inharmonicity
    if (result.numHarmonicsDetected > 1)
        result.inharmonicity /= (result.numHarmonicsDetected - 1);
}

float HarmonicsAnalyzer::interpolatePeak(const std::vector<float>& mags, int peakBin)
{
    if (peakBin <= 0 || peakBin >= (int)mags.size() - 1)
        return (float)peakBin;

    // Parabolic interpolation
    float alpha = mags[peakBin - 1];
    float beta = mags[peakBin];
    float gamma = mags[peakBin + 1];

    float p = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);

    return (float)peakBin + p;
}

float HarmonicsAnalyzer::binToFrequency(float bin) const
{
    return bin * (float)sampleRate / (float)fftSize;
}

float HarmonicsAnalyzer::frequencyToBin(float freq) const
{
    return freq * (float)fftSize / (float)sampleRate;
}

//==============================================================================
float HarmonicsAnalyzer::calculateTHD(const std::array<Harmonic, maxHarmonics>& harmonics, int numHarmonics)
{
    if (numHarmonics < 2 || !harmonics[0].detected)
        return 0.0f;

    float fundamentalPower = harmonics[0].amplitude * harmonics[0].amplitude;
    if (fundamentalPower <= 0.0f)
        return 0.0f;

    float harmonicPowerSum = 0.0f;
    for (int i = 1; i < numHarmonics; ++i)
    {
        if (harmonics[i].detected)
        {
            harmonicPowerSum += harmonics[i].amplitude * harmonics[i].amplitude;
        }
    }

    // THD = sqrt(sum of harmonic powers) / fundamental amplitude * 100%
    float thd = std::sqrt(harmonicPowerSum) / harmonics[0].amplitude * 100.0f;

    return thd;
}

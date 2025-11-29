/*
  ==============================================================================

    KeyDetector.cpp

    Musical key detection implementation using chroma features

  ==============================================================================
*/

#include "KeyDetector.h"
#include <cmath>

//==============================================================================
KeyDetector::KeyDetector()
{
    fftData.resize(fftSize * 2, 0.0f);
    inputBuffer.resize(fftSize, 0.0f);
    initializeKeyProfiles();
}

//==============================================================================
void KeyDetector::prepare(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    blockSize = samplesPerBlock;
    reset();
}

void KeyDetector::reset()
{
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
    std::fill(chromaFeatures.begin(), chromaFeatures.end(), 0.0f);
    std::fill(accumulatedChroma.begin(), accumulatedChroma.end(), 0.0f);
    std::fill(keyCorrelations.begin(), keyCorrelations.end(), 0.0f);
    std::fill(smoothedCorrelations.begin(), smoothedCorrelations.end(), 0.0f);

    inputBufferPos = 0;
    chromaFrameCount = 0;
    detectedKey = Key::Unknown;
    confidence = 0.0f;
}

//==============================================================================
void KeyDetector::initializeKeyProfiles()
{
    // Krumhansl-Schmuckler key profiles
    // Major profile (starting from C)
    majorProfile = {
        6.35f,  // C
        2.23f,  // C#
        3.48f,  // D
        2.33f,  // D#
        4.38f,  // E
        4.09f,  // F
        2.52f,  // F#
        5.19f,  // G
        2.39f,  // G#
        3.66f,  // A
        2.29f,  // A#
        2.88f   // B
    };

    // Minor profile (starting from C, natural minor)
    minorProfile = {
        6.33f,  // C
        2.68f,  // C#
        3.52f,  // D
        5.38f,  // D#
        2.60f,  // E
        3.53f,  // F
        2.54f,  // F#
        4.75f,  // G
        3.98f,  // G#
        2.69f,  // A
        3.34f,  // A#
        3.17f   // B
    };
}

//==============================================================================
void KeyDetector::processBlock(const juce::AudioBuffer<float>& buffer)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Mix to mono and accumulate
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            sample += buffer.getSample(ch, i);
        sample /= numChannels;

        inputBuffer[inputBufferPos++] = sample;

        if (inputBufferPos >= fftSize)
        {
            computeChroma(buffer);
            inputBufferPos = 0;
        }
    }
}

//==============================================================================
void KeyDetector::computeChroma(const juce::AudioBuffer<float>& /*buffer*/)
{
    // Apply Hann window
    for (int i = 0; i < fftSize; ++i)
    {
        float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1)));
        fftData[i] = inputBuffer[i] * window;
    }

    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);

    // Perform FFT
    fft.performRealOnlyForwardTransform(fftData.data());

    // Reset chroma for this frame
    std::array<float, 12> frameChroma {};

    // Map FFT bins to chroma
    // We only consider frequencies from ~30Hz to ~4000Hz (roughly A0 to B7)
    float minFreq = 30.0f;
    float maxFreq = 4000.0f;

    int minBin = (int)(minFreq * fftSize / sampleRate);
    int maxBin = (int)(maxFreq * fftSize / sampleRate);

    for (int bin = minBin; bin <= maxBin && bin < fftSize / 2; ++bin)
    {
        // Get magnitude
        float real = fftData[bin * 2];
        float imag = fftData[bin * 2 + 1];
        float magnitude = std::sqrt(real * real + imag * imag);

        // Convert bin to frequency
        float freq = (float)bin * (float)sampleRate / fftSize;

        // Convert frequency to pitch class (0-11)
        // Using A4 = 440Hz as reference
        float noteNum = 12.0f * std::log2(freq / 440.0f) + 69.0f;
        int pitchClass = ((int)std::round(noteNum) % 12 + 12) % 12;

        // Accumulate magnitude in corresponding chroma bin
        // Weight by magnitude squared for more emphasis on prominent tones
        frameChroma[pitchClass] += magnitude * magnitude;
    }

    // Normalize frame chroma
    float maxChroma = 0.0f;
    for (int i = 0; i < 12; ++i)
        maxChroma = std::max(maxChroma, frameChroma[i]);

    if (maxChroma > 0)
    {
        for (int i = 0; i < 12; ++i)
            frameChroma[i] /= maxChroma;
    }

    // Accumulate into long-term chroma
    for (int i = 0; i < 12; ++i)
        accumulatedChroma[i] += frameChroma[i];

    // Update display chroma (faster response)
    for (int i = 0; i < 12; ++i)
    {
        chromaFeatures[i] = chromaFeatures[i] * 0.7f + frameChroma[i] * 0.3f;
    }

    // Detect key every frame for faster response

    // Normalize accumulated chroma
    float sum = 0.0f;
    for (int i = 0; i < 12; ++i)
        sum += accumulatedChroma[i];

    if (sum > 0)
    {
        std::array<float, 12> normalizedChroma;
        for (int i = 0; i < 12; ++i)
            normalizedChroma[i] = accumulatedChroma[i] / sum;

        // Temporarily swap for detection
        auto tempChroma = accumulatedChroma;
        accumulatedChroma = normalizedChroma;
        detectKey();
        accumulatedChroma = tempChroma;
    }

    // Decay accumulated chroma faster
    for (int i = 0; i < 12; ++i)
        accumulatedChroma[i] *= 0.95f;
}

//==============================================================================
void KeyDetector::detectKey()
{
    // Compute correlation with all 24 key profiles
    float maxCorrelation = -1.0f;
    int bestKey = -1;

    for (int key = 0; key < 24; ++key)
    {
        bool isMajor = key < 12;
        int root = key % 12;

        const auto& profile = isMajor ? majorProfile : minorProfile;

        // Compute Pearson correlation
        float sumXY = 0.0f;
        float sumX = 0.0f, sumY = 0.0f;
        float sumX2 = 0.0f, sumY2 = 0.0f;

        for (int i = 0; i < 12; ++i)
        {
            int chromaIdx = (i + root) % 12;
            float x = accumulatedChroma[chromaIdx];
            float y = profile[i];

            sumXY += x * y;
            sumX += x;
            sumY += y;
            sumX2 += x * x;
            sumY2 += y * y;
        }

        float n = 12.0f;
        float numerator = n * sumXY - sumX * sumY;
        float denominator = std::sqrt((n * sumX2 - sumX * sumX) * (n * sumY2 - sumY * sumY));

        float correlation = (denominator > 0) ? (numerator / denominator) : 0.0f;

        // Smooth correlations
        smoothedCorrelations[key] = smoothedCorrelations[key] * (1.0f - smoothingFactor) +
                                    correlation * smoothingFactor;

        keyCorrelations[key] = smoothedCorrelations[key];

        if (smoothedCorrelations[key] > maxCorrelation)
        {
            maxCorrelation = smoothedCorrelations[key];
            bestKey = key;
        }
    }

    // Update detected key if confidence is high enough
    if (maxCorrelation > 0.3f)
    {
        detectedKey = static_cast<Key>(bestKey);
        confidence = juce::jlimit(0.0f, 1.0f, (maxCorrelation + 1.0f) / 2.0f);
    }
    else
    {
        confidence = 0.0f;
    }
}

//==============================================================================
juce::String KeyDetector::getKeyName(Key key)
{
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    if (key == Key::Unknown)
        return "Unknown";

    int keyIndex = static_cast<int>(key);
    bool isMajor = keyIndex < 12;
    int root = keyIndex % 12;

    return juce::String(noteNames[root]) + (isMajor ? " Major" : " Minor");
}

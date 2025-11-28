/*
  ==============================================================================

    ImpulseResponseAnalyzer.cpp

    Impulse Response measurement implementation

  ==============================================================================
*/

#include "ImpulseResponseAnalyzer.h"
#include <cmath>

//==============================================================================
ImpulseResponseAnalyzer::ImpulseResponseAnalyzer()
{
}

void ImpulseResponseAnalyzer::prepare(double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;
    reset();
}

void ImpulseResponseAnalyzer::reset()
{
    state = MeasurementState::Idle;
    currentSample = 0;
    sweepSignal.clear();
    inverseSweep.clear();
    recordedSignal.clear();
    result = MeasurementResult();
}

//==============================================================================
void ImpulseResponseAnalyzer::startMeasurement()
{
    if (state != MeasurementState::Idle)
        return;

    // Generate sweep and inverse sweep
    generateSweep();
    generateInverseSweep();

    // Prepare recording buffer
    recordedSignal.resize(totalSamples + fftSize, 0.0f);

    currentSample = 0;
    state = MeasurementState::GeneratingSweep;
}

void ImpulseResponseAnalyzer::stopMeasurement()
{
    state = MeasurementState::Idle;
    currentSample = 0;
}

float ImpulseResponseAnalyzer::getProgress() const
{
    if (totalSamples <= 0)
        return 0.0f;

    switch (state)
    {
        case MeasurementState::GeneratingSweep:
            return (float)currentSample / (float)totalSamples * 0.9f;
        case MeasurementState::Processing:
            return 0.95f;
        case MeasurementState::Complete:
            return 1.0f;
        default:
            return 0.0f;
    }
}

//==============================================================================
float ImpulseResponseAnalyzer::processSample(float inputSample)
{
    if (state != MeasurementState::GeneratingSweep)
        return 0.0f;

    // Record input
    if (currentSample < (int)recordedSignal.size())
    {
        recordedSignal[currentSample] = inputSample;
    }

    // Output sweep
    float output = 0.0f;
    if (currentSample < (int)sweepSignal.size())
    {
        output = sweepSignal[currentSample];
    }

    currentSample++;

    // Check if sweep is complete
    if (currentSample >= totalSamples)
    {
        state = MeasurementState::Processing;

        // Process in background
        juce::MessageManager::callAsync([this]()
        {
            computeImpulseResponse();
            computeFrequencyResponse();
            result.rt60 = calculateRT60();
            result.isValid = true;
            state = MeasurementState::Complete;

            if (onMeasurementComplete)
                onMeasurementComplete();
        });
    }

    return output;
}

//==============================================================================
void ImpulseResponseAnalyzer::generateSweep()
{
    // Generate exponential (logarithmic) sweep
    totalSamples = (int)(sweepDuration * sampleRate);
    sweepSignal.resize(totalSamples);

    double w1 = 2.0 * juce::MathConstants<double>::pi * startFrequency;
    double w2 = 2.0 * juce::MathConstants<double>::pi * endFrequency;
    double T = sweepDuration;
    double K = T * w1 / std::log(w2 / w1);
    double L = T / std::log(w2 / w1);

    for (int i = 0; i < totalSamples; ++i)
    {
        double t = (double)i / sampleRate;
        double phase = K * (std::exp(t / L) - 1.0);
        sweepSignal[i] = (float)(sweepAmplitude * std::sin(phase));
    }

    // Apply fade in/out to avoid clicks
    int fadeLength = (int)(0.01 * sampleRate);  // 10ms fade
    for (int i = 0; i < fadeLength && i < totalSamples; ++i)
    {
        float fade = (float)i / (float)fadeLength;
        sweepSignal[i] *= fade;
        sweepSignal[totalSamples - 1 - i] *= fade;
    }
}

void ImpulseResponseAnalyzer::generateInverseSweep()
{
    // Inverse sweep is time-reversed and amplitude-modulated
    inverseSweep.resize(totalSamples);

    double w1 = 2.0 * juce::MathConstants<double>::pi * startFrequency;
    double w2 = 2.0 * juce::MathConstants<double>::pi * endFrequency;
    double T = sweepDuration;
    double L = T / std::log(w2 / w1);

    for (int i = 0; i < totalSamples; ++i)
    {
        // Time-reverse
        int reverseIndex = totalSamples - 1 - i;

        // Amplitude modulation (compensate for frequency-dependent energy)
        double t = (double)reverseIndex / sampleRate;
        double envelope = std::exp(-t / L);

        inverseSweep[i] = sweepSignal[reverseIndex] * (float)envelope;
    }

    // Normalize
    float maxVal = 0.0f;
    for (float s : inverseSweep)
        maxVal = std::max(maxVal, std::abs(s));
    if (maxVal > 0.0f)
    {
        float scale = 1.0f / maxVal;
        for (float& s : inverseSweep)
            s *= scale;
    }
}

void ImpulseResponseAnalyzer::computeImpulseResponse()
{
    // Convolve recorded signal with inverse sweep using FFT
    int convLength = (int)recordedSignal.size() + (int)inverseSweep.size() - 1;

    // Find next power of 2
    int fftLen = 1;
    while (fftLen < convLength)
        fftLen *= 2;

    // Prepare FFT
    int order = (int)std::log2(fftLen);
    juce::dsp::FFT fft(order);

    // Prepare buffers
    std::vector<std::complex<float>> recFFT(fftLen);
    std::vector<std::complex<float>> invFFT(fftLen);

    // Copy and zero-pad recorded signal
    for (int i = 0; i < fftLen; ++i)
    {
        recFFT[i] = (i < (int)recordedSignal.size()) ? recordedSignal[i] : 0.0f;
    }

    // Copy and zero-pad inverse sweep
    for (int i = 0; i < fftLen; ++i)
    {
        invFFT[i] = (i < (int)inverseSweep.size()) ? inverseSweep[i] : 0.0f;
    }

    // Perform FFT
    fft.perform(recFFT.data(), recFFT.data(), false);
    fft.perform(invFFT.data(), invFFT.data(), false);

    // Multiply in frequency domain
    for (int i = 0; i < fftLen; ++i)
    {
        recFFT[i] *= invFFT[i];
    }

    // Inverse FFT
    fft.perform(recFFT.data(), recFFT.data(), true);

    // Extract impulse response (take the relevant portion)
    int irLength = std::min(fftLen, (int)(sampleRate * 2));  // Max 2 seconds
    result.impulseResponse.resize(irLength);

    float maxVal = 0.0f;
    for (int i = 0; i < irLength; ++i)
    {
        result.impulseResponse[i] = recFFT[i].real();
        maxVal = std::max(maxVal, std::abs(result.impulseResponse[i]));
    }

    // Normalize
    if (maxVal > 0.0f)
    {
        float scale = 1.0f / maxVal;
        for (float& s : result.impulseResponse)
            s *= scale;
    }

    result.peakLevel = 20.0f * std::log10(maxVal + 1e-10f);
}

void ImpulseResponseAnalyzer::computeFrequencyResponse()
{
    // Use FFT to compute frequency response from impulse response
    juce::dsp::FFT fft(fftOrder);

    std::vector<float> fftData(fftSize * 2, 0.0f);

    // Copy IR to FFT buffer
    int copyLen = std::min((int)result.impulseResponse.size(), fftSize);
    for (int i = 0; i < copyLen; ++i)
    {
        fftData[i] = result.impulseResponse[i];
    }

    // Apply window
    juce::dsp::WindowingFunction<float> window(fftSize, juce::dsp::WindowingFunction<float>::hann);
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    // Perform FFT
    fft.performRealOnlyForwardTransform(fftData.data());

    // Calculate magnitude and phase
    int numBins = fftSize / 2;
    result.frequencyMagnitude.resize(numBins);
    result.frequencyPhase.resize(numBins);
    result.frequencyAxis.resize(numBins);

    for (int i = 0; i < numBins; ++i)
    {
        float real = fftData[i * 2];
        float imag = fftData[i * 2 + 1];

        float magnitude = std::sqrt(real * real + imag * imag);
        result.frequencyMagnitude[i] = 20.0f * std::log10(magnitude + 1e-10f);

        result.frequencyPhase[i] = std::atan2(imag, real) * 180.0f / juce::MathConstants<float>::pi;

        result.frequencyAxis[i] = (float)(i * sampleRate / fftSize);
    }
}

float ImpulseResponseAnalyzer::calculateRT60()
{
    // Calculate RT60 (time for signal to decay by 60dB)
    if (result.impulseResponse.empty())
        return 0.0f;

    // Find peak
    float peakValue = 0.0f;
    int peakIndex = 0;
    for (int i = 0; i < (int)result.impulseResponse.size(); ++i)
    {
        float absVal = std::abs(result.impulseResponse[i]);
        if (absVal > peakValue)
        {
            peakValue = absVal;
            peakIndex = i;
        }
    }

    if (peakValue < 1e-10f)
        return 0.0f;

    // Calculate energy decay curve (Schroeder integration)
    std::vector<float> energyDecay(result.impulseResponse.size());
    float runningSum = 0.0f;

    for (int i = (int)result.impulseResponse.size() - 1; i >= peakIndex; --i)
    {
        runningSum += result.impulseResponse[i] * result.impulseResponse[i];
        energyDecay[i] = runningSum;
    }

    // Convert to dB
    float maxEnergy = energyDecay[peakIndex];
    if (maxEnergy < 1e-10f)
        return 0.0f;

    // Find -60dB point (or extrapolate from -5 to -35dB range)
    float startDb = -5.0f;
    float endDb = -35.0f;  // Use -35dB for T30 estimation
    int startIndex = -1;
    int endIndex = -1;

    for (int i = peakIndex; i < (int)energyDecay.size(); ++i)
    {
        float dB = 10.0f * std::log10(energyDecay[i] / maxEnergy + 1e-10f);

        if (startIndex < 0 && dB <= startDb)
            startIndex = i;

        if (dB <= endDb)
        {
            endIndex = i;
            break;
        }
    }

    if (startIndex < 0 || endIndex < 0 || endIndex <= startIndex)
        return 0.0f;

    // Calculate RT60 by extrapolating from T30
    float t30 = (float)(endIndex - startIndex) / (float)sampleRate;
    float rt60 = t30 * 2.0f;  // Extrapolate to 60dB

    return rt60;
}

/*
  ==============================================================================

    SpectrumDisplay.cpp

    Real-time frequency spectrum analyzer implementation

  ==============================================================================
*/

#include "SpectrumDisplay.h"
#include <cmath>

//==============================================================================
SpectrumDisplay::SpectrumDisplay()
    : forwardFFT(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    // Initialize FFT buffers
    fifo.resize(fftSize, 0.0f);
    fftData.resize(2 * fftSize, 0.0f);
    scopeData.resize(fftSize / 2, 0.0f);

    // Start timer for display updates (30 FPS)
    startTimerHz(30);
}

SpectrumDisplay::~SpectrumDisplay()
{
    stopTimer();
}

//==============================================================================
void SpectrumDisplay::pushNextSampleIntoFifo(float sample)
{
    // If the fifo contains enough data, set a flag to say
    // that the next frame should now be rendered
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            nextFFTBlockReady = true;
        }

        fifoIndex = 0;
    }

    fifo[fifoIndex++] = sample;
}

//==============================================================================
void SpectrumDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    auto bounds = getLocalBounds();
    drawFrame(g);

    auto displayBounds = bounds.reduced(10);
    displayBounds.removeFromLeft(50);   // Space for dB labels
    displayBounds.removeFromBottom(30); // Space for frequency labels

    drawGrid(g, displayBounds);
    drawSpectrum(g, displayBounds);
    drawFrequencyLabels(g, bounds.reduced(10));
    drawDecibelLabels(g, bounds.reduced(10));
}

void SpectrumDisplay::resized()
{
    // Layout handling if needed
}

//==============================================================================
void SpectrumDisplay::timerCallback()
{
    if (nextFFTBlockReady)
    {
        // Apply windowing function
        window.multiplyWithWindowingTable(fftData.data(), fftSize);

        // Perform FFT
        forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

        // Convert to dB and store in scopeData
        for (int i = 0; i < fftSize / 2; ++i)
        {
            float magnitude = fftData[i];
            float db = magnitude > 0.0f ? juce::Decibels::gainToDecibels(magnitude) : minDecibels;
            scopeData[i] = juce::jlimit(minDecibels, maxDecibels, db);
        }

        nextFFTBlockReady = false;
        repaint();
    }
}

//==============================================================================
void SpectrumDisplay::drawFrame(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRect(getLocalBounds(), 1);
}

void SpectrumDisplay::drawSpectrum(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    if (scopeData.empty())
        return;

    // Create path for spectrum
    juce::Path spectrumPath;
    bool firstPoint = true;

    for (int i = 1; i < scopeData.size(); ++i)
    {
        float freq = (float)i * sampleRate / (float)fftSize;

        if (freq < minFrequency || freq > maxFrequency)
            continue;

        float x = getXForFrequency(freq, (float)bounds.getWidth());
        float y = getYForDecibel(scopeData[i], (float)bounds.getHeight());

        if (firstPoint)
        {
            spectrumPath.startNewSubPath(bounds.getX() + x, bounds.getY() + y);
            firstPoint = false;
        }
        else
        {
            spectrumPath.lineTo(bounds.getX() + x, bounds.getY() + y);
        }
    }

    // Draw spectrum line
    g.setColour(juce::Colour(0xff4a9eff));
    g.strokePath(spectrumPath, juce::PathStrokeType(2.0f));

    // Fill under spectrum
    spectrumPath.lineTo((float)bounds.getRight(), (float)bounds.getBottom());
    spectrumPath.lineTo((float)bounds.getX(), (float)bounds.getBottom());
    spectrumPath.closeSubPath();

    g.setColour(juce::Colour(0xff4a9eff).withAlpha(0.2f));
    g.fillPath(spectrumPath);
}

void SpectrumDisplay::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colour(0xff3a3a3a));

    // Frequency grid lines (logarithmic)
    std::vector<float> freqs = { 50.0f, 100.0f, 200.0f, 500.0f,
                                  1000.0f, 2000.0f, 5000.0f, 10000.0f };

    for (float freq : freqs)
    {
        if (freq >= minFrequency && freq <= maxFrequency)
        {
            float x = getXForFrequency(freq, (float)bounds.getWidth());
            g.drawVerticalLine(bounds.getX() + (int)x, (float)bounds.getY(), (float)bounds.getBottom());
        }
    }

    // Decibel grid lines
    for (float db = minDecibels; db <= maxDecibels; db += 10.0f)
    {
        float y = getYForDecibel(db, (float)bounds.getHeight());
        g.drawHorizontalLine(bounds.getY() + (int)y, (float)bounds.getX(), (float)bounds.getRight());
    }
}

void SpectrumDisplay::drawFrequencyLabels(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(10.0f));

    auto labelBounds = bounds;
    labelBounds.removeFromLeft(50);
    auto labelArea = labelBounds.removeFromBottom(30);

    std::vector<std::pair<float, juce::String>> labels = {
        { 50.0f, "50" },
        { 100.0f, "100" },
        { 200.0f, "200" },
        { 500.0f, "500" },
        { 1000.0f, "1k" },
        { 2000.0f, "2k" },
        { 5000.0f, "5k" },
        { 10000.0f, "10k" }
    };

    for (const auto& [freq, label] : labels)
    {
        if (freq >= minFrequency && freq <= maxFrequency)
        {
            float x = getXForFrequency(freq, (float)labelBounds.getWidth());
            g.drawText(label,
                      labelBounds.getX() + (int)x - 20,
                      labelArea.getY(),
                      40, 20,
                      juce::Justification::centred);
        }
    }

    // Draw "Hz" unit
    g.drawText("Hz", labelArea.getRight() - 40, labelArea.getY(), 35, 20,
              juce::Justification::centredRight);
}

void SpectrumDisplay::drawDecibelLabels(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(10.0f));

    auto workingBounds = bounds;
    auto labelArea = workingBounds.removeFromLeft(50);
    labelArea.removeFromBottom(30);

    for (float db = minDecibels; db <= maxDecibels; db += 10.0f)
    {
        float y = getYForDecibel(db, (float)labelArea.getHeight());
        juce::String label = juce::String((int)db) + " dB";
        g.drawText(label,
                  labelArea.getX(),
                  labelArea.getY() + (int)y - 8,
                  45, 16,
                  juce::Justification::centredRight);
    }
}

//==============================================================================
float SpectrumDisplay::getFrequencyForX(float x, float width) const
{
    // Logarithmic frequency scale
    float normalized = x / width;
    float logMin = std::log10(minFrequency);
    float logMax = std::log10(maxFrequency);
    return std::pow(10.0f, logMin + normalized * (logMax - logMin));
}

float SpectrumDisplay::getXForFrequency(float freq, float width) const
{
    // Logarithmic frequency scale
    float logMin = std::log10(minFrequency);
    float logMax = std::log10(maxFrequency);
    float logFreq = std::log10(freq);
    float normalized = (logFreq - logMin) / (logMax - logMin);
    return normalized * width;
}

float SpectrumDisplay::getYForDecibel(float db, float height) const
{
    // Linear dB scale (inverted - 0dB at top, -60dB at bottom)
    float normalized = (maxDecibels - db) / (maxDecibels - minDecibels);
    return normalized * height;
}

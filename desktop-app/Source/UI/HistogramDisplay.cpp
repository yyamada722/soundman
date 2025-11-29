/*
  ==============================================================================

    HistogramDisplay.cpp

    Audio amplitude histogram implementation - Optimized

  ==============================================================================
*/

#include "HistogramDisplay.h"
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
HistogramDisplay::HistogramDisplay()
{
    histogram.resize(NUM_BINS, 0);

    // Precompute bar colors
    barColors.resize(NUM_BINS);
    for (int i = 0; i < NUM_BINS; ++i)
    {
        float db = juce::jmap((float)i, 0.0f, (float)(NUM_BINS - 1), -60.0f, 0.0f);

        if (db > -3.0f)
            barColors[i] = juce::Colour(0xffcc3333);  // Red
        else if (db > -10.0f)
            barColors[i] = juce::Colour(0xffcccc33);  // Yellow
        else if (db > -20.0f)
            barColors[i] = juce::Colour(0xff33cc33);  // Green
        else
            barColors[i] = juce::Colour(0xff3366cc);  // Blue
    }

    startTimer(67);  // ~15 fps (reduced from 30)
}

HistogramDisplay::~HistogramDisplay()
{
    stopTimer();
}

//==============================================================================
void HistogramDisplay::pushSample(float sample)
{
    // Convert to dB and map to bin
    float absSample = std::abs(sample);
    if (absSample < 0.0001f)  // -80 dB threshold
        return;

    float db = juce::Decibels::gainToDecibels(absSample);

    // Map -60dB to 0dB range to bins
    int bin = (int)juce::jmap(db, -60.0f, 0.0f, 0.0f, (float)(NUM_BINS - 1));
    bin = juce::jlimit(0, NUM_BINS - 1, bin);

    juce::ScopedLock lock(histogramLock);
    histogram[bin]++;

    // Track max for scaling
    if (histogram[bin] > maxBinValue)
        maxBinValue = histogram[bin];
}

void HistogramDisplay::clear()
{
    juce::ScopedLock lock(histogramLock);

    for (auto& bin : histogram)
        bin = 0;

    maxBinValue = 1;
}

//==============================================================================
void HistogramDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff0a0a0a));

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);

    auto innerBounds = bounds.reduced(30, 20);

    // Draw grid
    drawGrid(g, innerBounds);

    // Draw histogram
    drawHistogram(g, innerBounds);
}

void HistogramDisplay::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.setFont(juce::Font(9.0f));

    // Vertical grid lines (every 20 dB for simplicity)
    for (int db = -60; db <= 0; db += 20)
    {
        float x = juce::jmap((float)db, -60.0f, 0.0f,
                            (float)bounds.getX(), (float)bounds.getRight());

        g.drawVerticalLine((int)x, (float)bounds.getY(), (float)bounds.getBottom());

        // dB labels
        g.setColour(juce::Colours::grey);
        g.drawText(juce::String(db), (int)x - 15, bounds.getBottom() + 2, 30, 12,
                  juce::Justification::centred);

        g.setColour(juce::Colour(0xff2a2a2a));
    }

    // Axis label
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(11.0f));
    g.drawText("dB", bounds.getX(), bounds.getBottom() + 15,
              bounds.getWidth(), 15, juce::Justification::centred);
}

void HistogramDisplay::drawHistogram(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    juce::ScopedLock lock(histogramLock);

    if (maxBinValue < 1)
        return;

    float binWidth = bounds.getWidth() / (float)NUM_BINS;
    float invMaxBin = 1.0f / (float)maxBinValue;

    for (int i = 0; i < NUM_BINS; ++i)
    {
        if (histogram[i] == 0)
            continue;

        float x = bounds.getX() + i * binWidth;
        float normalizedHeight = (float)histogram[i] * invMaxBin;
        float height = normalizedHeight * bounds.getHeight();

        // Use precomputed color
        g.setColour(barColors[i].withAlpha(0.8f));
        g.fillRect(x, (float)bounds.getBottom() - height, binWidth - 1.0f, height);
    }
}

void HistogramDisplay::resized()
{
    // Nothing to resize
}

void HistogramDisplay::timerCallback()
{
    // Apply decay to histogram
    {
        juce::ScopedLock lock(histogramLock);

        maxBinValue = 1;
        for (auto& bin : histogram)
        {
            bin = static_cast<int>(static_cast<float>(bin) * decay);
            if (bin > maxBinValue)
                maxBinValue = bin;
        }
    }

    repaint();
}

/*
  ==============================================================================

    HistogramDisplay.cpp

    Audio amplitude histogram implementation

  ==============================================================================
*/

#include "HistogramDisplay.h"
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
HistogramDisplay::HistogramDisplay()
{
    histogram.resize(NUM_BINS, 0);
    startTimer(30);  // ~30 fps
}

HistogramDisplay::~HistogramDisplay()
{
    stopTimer();
}

//==============================================================================
void HistogramDisplay::pushSample(float sample)
{
    juce::ScopedLock lock(histogramLock);

    // Convert to dB and map to bin
    float absSample = std::abs(sample);
    if (absSample < 0.0001f)  // -80 dB threshold
        return;

    float db = juce::Decibels::gainToDecibels(absSample);

    // Map -60dB to 0dB range to bins
    int bin = (int)juce::jmap(db, -60.0f, 0.0f, 0.0f, (float)(NUM_BINS - 1));
    bin = juce::jlimit(0, NUM_BINS - 1, bin);

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

    // Draw grid
    drawGrid(g, bounds.reduced(30, 20));

    // Draw histogram
    drawHistogram(g, bounds.reduced(30, 20));
}

void HistogramDisplay::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colour(0xff2a2a2a));

    // Horizontal grid lines (every 10 dB)
    g.setFont(juce::Font(9.0f));
    for (int db = -60; db <= 0; db += 10)
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

    // Vertical grid lines
    int numHorizontalLines = 5;
    for (int i = 0; i <= numHorizontalLines; ++i)
    {
        float y = juce::jmap((float)i, 0.0f, (float)numHorizontalLines,
                            (float)bounds.getBottom(), (float)bounds.getY());

        g.drawHorizontalLine((int)y, (float)bounds.getX(), (float)bounds.getRight());
    }

    // Axis labels
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(11.0f));
    g.drawText("Amplitude (dB)", bounds.getX(), bounds.getBottom() + 15,
              bounds.getWidth(), 15, juce::Justification::centred);
}

void HistogramDisplay::drawHistogram(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    juce::ScopedLock lock(histogramLock);

    if (maxBinValue < 1)
        return;

    float binWidth = bounds.getWidth() / (float)NUM_BINS;

    for (int i = 0; i < NUM_BINS; ++i)
    {
        if (histogram[i] == 0)
            continue;

        float x = bounds.getX() + i * binWidth;
        float normalizedHeight = (float)histogram[i] / (float)maxBinValue;
        float height = normalizedHeight * bounds.getHeight();

        auto barBounds = juce::Rectangle<float>(x, bounds.getBottom() - height, binWidth, height);

        // Color based on dB level
        float db = juce::jmap((float)i, 0.0f, (float)(NUM_BINS - 1), -60.0f, 0.0f);
        juce::Colour color;

        if (db > -3.0f)
            color = juce::Colour(0xffcc3333);  // Red
        else if (db > -10.0f)
            color = juce::Colour(0xffcccc33);  // Yellow
        else if (db > -20.0f)
            color = juce::Colour(0xff33cc33);  // Green
        else
            color = juce::Colour(0xff3366cc);  // Blue

        g.setColour(color.withAlpha(0.8f));
        g.fillRect(barBounds);

        // Outline
        g.setColour(color);
        g.drawRect(barBounds, 0.5f);
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
            bin = (int)((float)bin * decay);
            if (bin > maxBinValue)
                maxBinValue = bin;
        }
    }

    repaint();
}

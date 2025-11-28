/*
  ==============================================================================

    MFCCDisplay.cpp

    MFCC visualization implementation

  ==============================================================================
*/

#include "MFCCDisplay.h"
#include <cmath>

//==============================================================================
MFCCDisplay::MFCCDisplay()
{
    smoothedMFCCs.fill(0.0f);
    smoothedMelEnergies.fill(0.0f);
    startTimerHz(30);
}

MFCCDisplay::~MFCCDisplay()
{
    stopTimer();
}

//==============================================================================
void MFCCDisplay::setMFCCResult(const MFCCAnalyzer::MFCCResult& result)
{
    currentResult = result;

    if (result.isValid)
    {
        // Update smoothed MFCCs
        for (int i = 0; i < MFCCAnalyzer::numMFCCs; ++i)
        {
            smoothedMFCCs[i] = smoothedMFCCs[i] + smoothingFactor * (result.coefficients[i] - smoothedMFCCs[i]);
        }

        // Update smoothed Mel energies
        for (int i = 0; i < MFCCAnalyzer::numMelFilters; ++i)
        {
            float logEnergy = std::log10(result.melEnergies[i] + 1e-10f);
            smoothedMelEnergies[i] = smoothedMelEnergies[i] + smoothingFactor * (logEnergy - smoothedMelEnergies[i]);
        }

        // Add to history
        if (showHistory)
        {
            mfccHistory.push_back(result.coefficients);
            while ((int)mfccHistory.size() > maxHistoryLength)
                mfccHistory.pop_front();
        }
    }
}

void MFCCDisplay::pushSample(float sample)
{
    analyzer.pushSample(sample);
}

void MFCCDisplay::setSampleRate(double rate)
{
    analyzer.setSampleRate(rate);
}

void MFCCDisplay::setHistoryLength(int length)
{
    maxHistoryLength = juce::jmax(10, length);
    while ((int)mfccHistory.size() > maxHistoryLength)
        mfccHistory.pop_front();
}

//==============================================================================
void MFCCDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    drawBackground(g, bounds);

    auto workingBounds = bounds.reduced(10);

    // Info panel at top
    auto infoPanel = workingBounds.removeFromTop(80);
    drawInfoPanel(g, infoPanel);

    workingBounds.removeFromTop(10);

    // Split remaining area
    auto topArea = workingBounds.removeFromTop(workingBounds.getHeight() / 2);
    topArea.removeFromBottom(5);

    // MFCC coefficients bar display
    auto mfccBarArea = topArea.removeFromTop(topArea.getHeight() / 2);
    mfccBarArea.removeFromBottom(5);
    drawMFCCBars(g, mfccBarArea);

    // Mel filter bank energies
    drawMelFilterBankDisplay(g, topArea);

    workingBounds.removeFromTop(10);

    // MFCC history (spectrogram-like)
    if (showHistory)
    {
        drawMFCCHistory(g, workingBounds);
    }
}

void MFCCDisplay::resized()
{
    // Layout handled in paint
}

void MFCCDisplay::timerCallback()
{
    auto result = analyzer.getLatestResult();
    setMFCCResult(result);
    repaint();
}

//==============================================================================
void MFCCDisplay::drawBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRect(bounds, 1);
}

void MFCCDisplay::drawInfoPanel(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    auto infoBounds = bounds.reduced(15, 10);

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("MFCC Analysis", infoBounds.removeFromTop(24), juce::Justification::centredLeft);

    // Status and info
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));

    juce::String statusText = currentResult.isValid ? "Active" : "Waiting for signal...";
    g.drawText("Status: " + statusText, infoBounds.removeFromTop(18), juce::Justification::centredLeft);

    if (currentResult.isValid)
    {
        // Show first few MFCC values
        juce::String mfccText = "C0: " + juce::String(smoothedMFCCs[0], 1)
                              + "  C1: " + juce::String(smoothedMFCCs[1], 1)
                              + "  C2: " + juce::String(smoothedMFCCs[2], 1);
        g.drawText(mfccText, infoBounds.removeFromTop(18), juce::Justification::centredLeft);
    }
}

void MFCCDisplay::drawMFCCBars(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));
    g.drawText("MFCC Coefficients", bounds.getX() + 10, bounds.getY() + 3, 120, 14,
              juce::Justification::centredLeft);

    auto chartBounds = bounds.reduced(10);
    chartBounds.removeFromTop(18);
    chartBounds.removeFromBottom(15);  // Space for labels

    if (!currentResult.isValid)
    {
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(11.0f));
        g.drawText("No data", chartBounds, juce::Justification::centred);
        return;
    }

    // Draw MFCC bars
    float barWidth = (float)chartBounds.getWidth() / MFCCAnalyzer::numMFCCs;
    float gap = barWidth * 0.15f;
    barWidth -= gap;

    float centerY = chartBounds.getCentreY();
    float halfHeight = chartBounds.getHeight() / 2.0f;

    for (int i = 0; i < MFCCAnalyzer::numMFCCs; ++i)
    {
        float x = chartBounds.getX() + i * (barWidth + gap) + gap / 2;
        float normalized = (smoothedMFCCs[i] - mfccMin) / (mfccMax - mfccMin);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);

        // Bar extends from center, positive up, negative down
        float value = smoothedMFCCs[i];
        float normalizedValue = juce::jlimit(-1.0f, 1.0f, value / 30.0f);

        float barHeight = std::abs(normalizedValue) * halfHeight;
        float barY = (normalizedValue >= 0) ? centerY - barHeight : centerY;

        juce::Rectangle<float> barRect(x, barY, barWidth, barHeight);

        // Color based on coefficient index and value
        g.setColour(getMFCCColor(i, normalizedValue));
        g.fillRoundedRectangle(barRect, 2.0f);

        // Label
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(8.0f));
        g.drawText("C" + juce::String(i), (int)x, chartBounds.getBottom() + 2, (int)barWidth, 12,
                  juce::Justification::centred);
    }

    // Draw center line
    g.setColour(juce::Colour(0xff4a4a4a));
    g.drawHorizontalLine((int)centerY, (float)chartBounds.getX(), (float)chartBounds.getRight());
}

void MFCCDisplay::drawMelFilterBankDisplay(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));
    g.drawText("Mel Filter Bank Energies", bounds.getX() + 10, bounds.getY() + 3, 150, 14,
              juce::Justification::centredLeft);

    auto chartBounds = bounds.reduced(10);
    chartBounds.removeFromTop(18);
    chartBounds.removeFromBottom(15);

    if (!currentResult.isValid)
    {
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(11.0f));
        g.drawText("No data", chartBounds, juce::Justification::centred);
        return;
    }

    // Find min/max for normalization
    float minEnergy = -10.0f;
    float maxEnergy = 0.0f;

    // Draw Mel energy bars
    float barWidth = (float)chartBounds.getWidth() / MFCCAnalyzer::numMelFilters;

    for (int i = 0; i < MFCCAnalyzer::numMelFilters; ++i)
    {
        float x = chartBounds.getX() + i * barWidth;
        float normalized = (smoothedMelEnergies[i] - minEnergy) / (maxEnergy - minEnergy);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);

        float barHeight = normalized * chartBounds.getHeight();

        juce::Rectangle<float> barRect(x, chartBounds.getBottom() - barHeight, barWidth - 1, barHeight);

        g.setColour(getMelColor(normalized));
        g.fillRect(barRect);
    }

    // Labels
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(8.0f));
    g.drawText("Low", chartBounds.getX(), chartBounds.getBottom() + 2, 30, 12, juce::Justification::centredLeft);
    g.drawText("High", chartBounds.getRight() - 30, chartBounds.getBottom() + 2, 30, 12, juce::Justification::centredRight);
}

void MFCCDisplay::drawMFCCHistory(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));
    g.drawText("MFCC History", bounds.getX() + 10, bounds.getY() + 3, 100, 14,
              juce::Justification::centredLeft);

    auto chartBounds = bounds.reduced(10);
    chartBounds.removeFromTop(18);
    chartBounds.removeFromLeft(25);  // Space for MFCC labels

    if (mfccHistory.empty())
    {
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(11.0f));
        g.drawText("Collecting data...", chartBounds, juce::Justification::centred);
        return;
    }

    // Draw MFCC labels on the left
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(8.0f));
    float cellHeight = (float)chartBounds.getHeight() / MFCCAnalyzer::numMFCCs;

    for (int i = 0; i < MFCCAnalyzer::numMFCCs; ++i)
    {
        int y = chartBounds.getY() + (int)(i * cellHeight);
        g.drawText("C" + juce::String(i), bounds.getX() + 5, y, 20, (int)cellHeight,
                  juce::Justification::centredRight);
    }

    // Draw MFCC history as a heatmap
    float cellWidth = (float)chartBounds.getWidth() / mfccHistory.size();

    for (size_t t = 0; t < mfccHistory.size(); ++t)
    {
        const auto& mfccs = mfccHistory[t];
        float x = chartBounds.getX() + t * cellWidth;

        for (int c = 0; c < MFCCAnalyzer::numMFCCs; ++c)
        {
            float y = chartBounds.getY() + c * cellHeight;

            // Normalize MFCC value
            float normalized = (mfccs[c] - mfccMin) / (mfccMax - mfccMin);
            normalized = juce::jlimit(0.0f, 1.0f, normalized);

            g.setColour(getMFCCColor(c, normalized * 2.0f - 1.0f));
            g.fillRect(x, y, cellWidth, cellHeight);
        }
    }

    // Time arrow
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(9.0f));
    g.drawText("Time ->", chartBounds.getRight() - 50, chartBounds.getBottom() + 2, 50, 12,
              juce::Justification::centredRight);
}

//==============================================================================
juce::Colour MFCCDisplay::getMFCCColor(int index, float normalizedValue) const
{
    // Create a color based on coefficient index and value
    // Different coefficients get different base hues

    float hue = (float)index / MFCCAnalyzer::numMFCCs;
    float saturation = 0.7f;
    float brightness = 0.3f + 0.7f * std::abs(normalizedValue);

    // Shift hue slightly based on value sign
    if (normalizedValue < 0)
        hue = std::fmod(hue + 0.5f, 1.0f);

    return juce::Colour::fromHSV(hue, saturation, brightness, 1.0f);
}

juce::Colour MFCCDisplay::getMelColor(float normalizedValue) const
{
    // Viridis-like colormap
    float r = 0.267f + normalizedValue * (0.993f - 0.267f);
    float g = 0.004f + normalizedValue * (0.906f - 0.004f);
    float b = 0.329f + normalizedValue * (0.143f - 0.329f);

    // Adjust for better visibility
    if (normalizedValue < 0.3f)
    {
        r = 0.1f + normalizedValue;
        g = 0.1f + normalizedValue * 0.5f;
        b = 0.3f + normalizedValue;
    }

    return juce::Colour::fromFloatRGBA(
        juce::jlimit(0.0f, 1.0f, r),
        juce::jlimit(0.0f, 1.0f, g),
        juce::jlimit(0.0f, 1.0f, b),
        1.0f);
}

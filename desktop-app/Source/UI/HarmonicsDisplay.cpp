/*
  ==============================================================================

    HarmonicsDisplay.cpp

    Harmonics visualization implementation

  ==============================================================================
*/

#include "HarmonicsDisplay.h"
#include <cmath>

//==============================================================================
HarmonicsDisplay::HarmonicsDisplay()
{
    smoothedAmplitudes.fill(-100.0f);
    startTimerHz(30);
}

HarmonicsDisplay::~HarmonicsDisplay()
{
    stopTimer();
}

//==============================================================================
void HarmonicsDisplay::setAnalysisResult(const HarmonicsAnalyzer::AnalysisResult& result)
{
    currentResult = result;

    // Update smoothed values
    for (int i = 0; i < HarmonicsAnalyzer::maxHarmonics; ++i)
    {
        float targetDb = result.harmonics[i].detected ? result.harmonics[i].amplitudeDb : -100.0f;
        smoothedAmplitudes[i] = smoothedAmplitudes[i] + smoothingFactor * (targetDb - smoothedAmplitudes[i]);
    }
}

void HarmonicsDisplay::pushSample(float sample)
{
    analyzer.pushSample(sample);
}

void HarmonicsDisplay::setSampleRate(double rate)
{
    analyzer.setSampleRate(rate);
}

//==============================================================================
void HarmonicsDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    drawBackground(g, bounds);

    auto workingBounds = bounds.reduced(10);

    // Info panel on the left
    auto infoPanel = workingBounds.removeFromLeft(180);
    drawInfoPanel(g, infoPanel);

    workingBounds.removeFromLeft(10);

    // Main display area
    auto displayBounds = workingBounds;

    // Harmonic bars at top
    auto barArea = displayBounds.removeFromTop(displayBounds.getHeight() / 2);
    barArea.removeFromBottom(5);
    drawHarmonicBars(g, barArea);

    displayBounds.removeFromTop(10);

    // Harmonic spectrum at bottom
    drawHarmonicSpectrum(g, displayBounds);
}

void HarmonicsDisplay::resized()
{
    // Layout handled in paint
}

void HarmonicsDisplay::timerCallback()
{
    auto result = analyzer.getLatestAnalysis();
    setAnalysisResult(result);
    repaint();
}

//==============================================================================
void HarmonicsDisplay::drawBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRect(bounds, 1);
}

void HarmonicsDisplay::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    if (!showGrid)
        return;

    g.setColour(juce::Colour(0xff3a3a3a));

    // Horizontal lines (dB levels)
    for (float db = minDb; db <= maxDb; db += 10.0f)
    {
        float y = dbToY(db, (float)bounds.getHeight());
        g.drawHorizontalLine(bounds.getY() + (int)y, (float)bounds.getX(), (float)bounds.getRight());
    }
}

void HarmonicsDisplay::drawHarmonicBars(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    if (!currentResult.isValid)
    {
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(14.0f));
        g.drawText("No harmonic content detected", bounds, juce::Justification::centred);
        return;
    }

    auto chartBounds = bounds.reduced(10);
    chartBounds.removeFromBottom(25);  // Space for labels
    chartBounds.removeFromLeft(30);    // Space for dB labels

    // Draw dB labels
    if (showLabels)
    {
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(9.0f));

        for (float db = minDb; db <= maxDb; db += 20.0f)
        {
            float y = dbToY(db, (float)chartBounds.getHeight());
            g.drawText(juce::String((int)db), bounds.getX() + 5, chartBounds.getY() + (int)y - 6, 25, 12,
                      juce::Justification::centredRight);
        }
    }

    // Draw grid
    drawGrid(g, chartBounds);

    // Draw bars
    int numBars = currentResult.numHarmonicsDetected;
    if (numBars <= 0)
        numBars = 8;  // Show at least 8 bar positions

    float barWidth = (float)chartBounds.getWidth() / (float)(numBars + 1);
    float gap = barWidth * 0.2f;
    barWidth -= gap;

    for (int i = 0; i < numBars && i < HarmonicsAnalyzer::maxHarmonics; ++i)
    {
        float x = chartBounds.getX() + (i + 0.5f) * (barWidth + gap);
        float db = smoothedAmplitudes[i];
        float barHeight = (1.0f - dbToY(db, (float)chartBounds.getHeight()) / (float)chartBounds.getHeight())
                          * chartBounds.getHeight();

        barHeight = juce::jmax(2.0f, barHeight);

        // Draw bar
        juce::Rectangle<float> barRect(x, chartBounds.getBottom() - barHeight, barWidth, barHeight);
        g.setColour(getHarmonicColor(i + 1));
        g.fillRoundedRectangle(barRect, 3.0f);

        // Draw outline
        g.setColour(getHarmonicColor(i + 1).brighter(0.3f));
        g.drawRoundedRectangle(barRect, 3.0f, 1.0f);

        // Draw harmonic number label
        if (showLabels)
        {
            g.setColour(juce::Colours::lightgrey);
            g.setFont(juce::Font(10.0f));
            juce::String label = (i == 0) ? "F" : juce::String(i + 1);
            g.drawText(label, (int)x, chartBounds.getBottom() + 5, (int)barWidth, 15,
                      juce::Justification::centred);
        }
    }

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText("Harmonic Amplitudes (dB)", bounds.getX() + 10, bounds.getY() + 5, 150, 14,
              juce::Justification::centredLeft);
}

void HarmonicsDisplay::drawInfoPanel(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    auto infoBounds = bounds.reduced(10);
    int lineHeight = 22;
    int y = infoBounds.getY();

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("Harmonic Analysis", infoBounds.getX(), y, infoBounds.getWidth(), lineHeight,
              juce::Justification::centredLeft);
    y += lineHeight + 10;

    // Fundamental frequency
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText("Fundamental:", infoBounds.getX(), y, infoBounds.getWidth(), lineHeight,
              juce::Justification::centredLeft);
    y += lineHeight - 4;

    g.setColour(currentResult.isValid ? juce::Colour(0xff4a9eff) : juce::Colours::grey);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    juce::String fundText = currentResult.isValid
        ? juce::String(currentResult.fundamentalFrequency, 1) + " Hz"
        : "--- Hz";
    g.drawText(fundText, infoBounds.getX(), y, infoBounds.getWidth(), lineHeight + 4,
              juce::Justification::centredLeft);
    y += lineHeight + 10;

    // Fundamental amplitude
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText("Level:", infoBounds.getX(), y, infoBounds.getWidth(), lineHeight,
              juce::Justification::centredLeft);
    y += lineHeight - 4;

    g.setColour(currentResult.isValid ? juce::Colours::lightgrey : juce::Colours::grey);
    g.setFont(juce::Font(14.0f));
    juce::String levelText = currentResult.isValid
        ? juce::String(currentResult.fundamentalAmplitudeDb, 1) + " dB"
        : "--- dB";
    g.drawText(levelText, infoBounds.getX(), y, infoBounds.getWidth(), lineHeight,
              juce::Justification::centredLeft);
    y += lineHeight + 10;

    // THD
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText("THD:", infoBounds.getX(), y, infoBounds.getWidth(), lineHeight,
              juce::Justification::centredLeft);
    y += lineHeight - 4;

    juce::Colour thdColor;
    if (!currentResult.isValid)
        thdColor = juce::Colours::grey;
    else if (currentResult.totalHarmonicDistortion < 1.0f)
        thdColor = juce::Colour(0xff00cc00);  // Green
    else if (currentResult.totalHarmonicDistortion < 5.0f)
        thdColor = juce::Colour(0xffcccc00);  // Yellow
    else
        thdColor = juce::Colour(0xffcc6600);  // Orange

    g.setColour(thdColor);
    g.setFont(juce::Font(14.0f));
    juce::String thdText = currentResult.isValid
        ? juce::String(currentResult.totalHarmonicDistortion, 2) + " %"
        : "--- %";
    g.drawText(thdText, infoBounds.getX(), y, infoBounds.getWidth(), lineHeight,
              juce::Justification::centredLeft);
    y += lineHeight + 10;

    // Number of harmonics detected
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText("Harmonics:", infoBounds.getX(), y, infoBounds.getWidth(), lineHeight,
              juce::Justification::centredLeft);
    y += lineHeight - 4;

    g.setColour(currentResult.isValid ? juce::Colours::lightgrey : juce::Colours::grey);
    g.setFont(juce::Font(14.0f));
    juce::String numText = currentResult.isValid
        ? juce::String(currentResult.numHarmonicsDetected)
        : "---";
    g.drawText(numText, infoBounds.getX(), y, infoBounds.getWidth(), lineHeight,
              juce::Justification::centredLeft);
}

void HarmonicsDisplay::drawHarmonicSpectrum(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    auto chartBounds = bounds.reduced(10);

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText("Harmonic Spectrum", bounds.getX() + 10, bounds.getY() + 5, 150, 14,
              juce::Justification::centredLeft);

    chartBounds.removeFromTop(20);

    if (!currentResult.isValid)
    {
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(12.0f));
        g.drawText("Waiting for signal...", chartBounds, juce::Justification::centred);
        return;
    }

    // Draw harmonic lines
    float fundamentalFreq = currentResult.fundamentalFrequency;
    float maxFreq = fundamentalFreq * (HarmonicsAnalyzer::maxHarmonics + 1);

    for (int i = 0; i < currentResult.numHarmonicsDetected && i < HarmonicsAnalyzer::maxHarmonics; ++i)
    {
        const auto& harmonic = currentResult.harmonics[i];
        if (!harmonic.detected)
            continue;

        float x = (harmonic.frequency / maxFreq) * chartBounds.getWidth();
        float height = (1.0f - dbToY(smoothedAmplitudes[i], (float)chartBounds.getHeight())
                        / (float)chartBounds.getHeight()) * chartBounds.getHeight();

        height = juce::jmax(2.0f, height);

        // Draw line
        g.setColour(getHarmonicColor(i + 1));
        g.fillRect(chartBounds.getX() + (int)x - 2, chartBounds.getBottom() - (int)height, 4, (int)height);

        // Draw frequency label for first few harmonics
        if (i < 4 && showLabels)
        {
            g.setColour(juce::Colours::lightgrey);
            g.setFont(juce::Font(9.0f));
            g.drawText(juce::String((int)harmonic.frequency) + "Hz",
                      chartBounds.getX() + (int)x - 25, chartBounds.getBottom() + 2, 50, 12,
                      juce::Justification::centred);
        }
    }
}

//==============================================================================
juce::Colour HarmonicsDisplay::getHarmonicColor(int harmonicNumber) const
{
    // Color progression from fundamental through harmonics
    static const juce::Colour harmonicColors[] = {
        juce::Colour(0xff4a9eff),  // 1 - Fundamental (blue)
        juce::Colour(0xff00cc66),  // 2 - Second (green)
        juce::Colour(0xffffcc00),  // 3 - Third (yellow)
        juce::Colour(0xffff9900),  // 4 - Fourth (orange)
        juce::Colour(0xffff6666),  // 5 - Fifth (red)
        juce::Colour(0xffcc66ff),  // 6 - Sixth (purple)
        juce::Colour(0xff66ccff),  // 7 - Seventh (cyan)
        juce::Colour(0xffff66cc),  // 8 - Eighth (pink)
        juce::Colour(0xff99ff66),  // 9 - Ninth (lime)
        juce::Colour(0xff66ffcc),  // 10 - Tenth (teal)
        juce::Colour(0xffffcc66),  // 11
        juce::Colour(0xffcc99ff),  // 12
        juce::Colour(0xff99ccff),  // 13
        juce::Colour(0xffff99cc),  // 14
        juce::Colour(0xffccff99),  // 15
        juce::Colour(0xff99ffcc),  // 16
    };

    int index = juce::jlimit(0, 15, harmonicNumber - 1);
    return harmonicColors[index];
}

float HarmonicsDisplay::dbToY(float db, float height) const
{
    float normalized = (maxDb - db) / (maxDb - minDb);
    return juce::jlimit(0.0f, 1.0f, normalized) * height;
}

/*
  ==============================================================================

    PhaseMeter.cpp

    Stereo phase correlation meter implementation

  ==============================================================================
*/

#include "PhaseMeter.h"

//==============================================================================
PhaseMeter::PhaseMeter()
{
    startTimerHz(30);  // 30 FPS updates
}

PhaseMeter::~PhaseMeter()
{
    stopTimer();
}

//==============================================================================
void PhaseMeter::setCorrelation(float correlation)
{
    // Clamp to valid range
    correlation = juce::jlimit(-1.0f, 1.0f, correlation);
    currentCorrelation.store(correlation, std::memory_order_relaxed);
}

void PhaseMeter::reset()
{
    currentCorrelation.store(0.0f, std::memory_order_relaxed);
    displayCorrelation = 0.0f;
}

//==============================================================================
void PhaseMeter::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("PHASE CORRELATION", getLocalBounds().removeFromTop(25),
              juce::Justification::centred);

    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(25);

    // Reserve space for numeric value at bottom
    auto numericBounds = bounds.removeFromBottom(30);

    // Draw correlation meter
    drawCorrelationMeter(g, bounds.reduced(5));

    // Draw numeric value
    drawNumericValue(g, numericBounds);
}

void PhaseMeter::resized()
{
    // Layout handled in paint
}

//==============================================================================
void PhaseMeter::timerCallback()
{
    // Get current correlation with smoothing
    float targetCorrelation = currentCorrelation.load(std::memory_order_relaxed);
    displayCorrelation = displayCorrelation * SMOOTHING + targetCorrelation * (1.0f - SMOOTHING);

    repaint();
}

//==============================================================================
void PhaseMeter::drawCorrelationMeter(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Draw background
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(bounds);

    // Draw scale markings
    drawScale(g, bounds);

    // Calculate meter position
    // Correlation: -1.0 (left) to +1.0 (right)
    // Center at 0.0
    float normalized = (displayCorrelation + 1.0f) * 0.5f;  // Convert -1..+1 to 0..1
    int meterX = bounds.getX() + (int)(normalized * bounds.getWidth());

    // Draw danger zones
    // Red zone for negative correlation (phase issues)
    if (displayCorrelation < 0.0f)
    {
        int zeroX = bounds.getX() + bounds.getWidth() / 2;
        auto dangerZone = juce::Rectangle<int>(meterX, bounds.getY(), zeroX - meterX, bounds.getHeight());
        g.setColour(juce::Colours::red.withAlpha(0.3f));
        g.fillRect(dangerZone);
    }

    // Draw center line (zero correlation)
    int centerX = bounds.getX() + bounds.getWidth() / 2;
    g.setColour(juce::Colours::grey);
    g.drawVerticalLine(centerX, (float)bounds.getY(), (float)bounds.getBottom());

    // Draw correlation indicator bar
    g.setColour(getColorForCorrelation(displayCorrelation));
    int barWidth = 4;
    g.fillRect(meterX - barWidth / 2, bounds.getY(), barWidth, bounds.getHeight());

    // Draw border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);
}

void PhaseMeter::drawNumericValue(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(getColorForCorrelation(displayCorrelation));
    g.setFont(juce::Font(16.0f, juce::Font::bold));

    juce::String valueText = juce::String(displayCorrelation, 2);
    g.drawText(valueText, bounds, juce::Justification::centred);

    // Draw status text
    g.setFont(juce::Font(10.0f));
    juce::String statusText;
    if (displayCorrelation > 0.5f)
        statusText = "Good Stereo";
    else if (displayCorrelation > 0.0f)
        statusText = "Acceptable";
    else if (displayCorrelation > -0.5f)
        statusText = "Phase Issues";
    else
        statusText = "Severe Phase Issues";

    auto statusBounds = bounds;
    statusBounds.translate(0, 20);
    g.setColour(juce::Colours::lightgrey);
    g.drawText(statusText, statusBounds, juce::Justification::centred);
}

void PhaseMeter::drawScale(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colour(0xff3a3a3a));
    g.setFont(juce::Font(9.0f));

    // Draw scale markers at -1, -0.5, 0, +0.5, +1
    std::vector<float> markers = { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f };

    for (float marker : markers)
    {
        float normalized = (marker + 1.0f) * 0.5f;
        int x = bounds.getX() + (int)(normalized * bounds.getWidth());

        // Draw tick mark
        g.setColour(juce::Colour(0xff5a5a5a));
        g.drawVerticalLine(x, (float)bounds.getY(), (float)bounds.getY() + 10);
        g.drawVerticalLine(x, (float)bounds.getBottom() - 10, (float)bounds.getBottom());

        // Draw label
        g.setColour(juce::Colours::grey);
        juce::String label = marker == 0.0f ? "0" : juce::String(marker, 1);
        g.drawText(label, x - 15, bounds.getBottom() + 2, 30, 12,
                  juce::Justification::centred);
    }
}

juce::Colour PhaseMeter::getColorForCorrelation(float corr) const
{
    if (corr >= 0.5f)
        return juce::Colours::green;
    else if (corr >= 0.0f)
        return juce::Colours::yellow;
    else if (corr >= -0.5f)
        return juce::Colours::orange;
    else
        return juce::Colours::red;
}

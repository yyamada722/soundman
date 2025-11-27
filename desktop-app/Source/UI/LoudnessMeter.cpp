/*
  ==============================================================================

    LoudnessMeter.cpp

    ITU-R BS.1770-4 compliant loudness meter implementation

  ==============================================================================
*/

#include "LoudnessMeter.h"

//==============================================================================
LoudnessMeter::LoudnessMeter()
{
    startTimerHz(30);  // 30 FPS updates
}

LoudnessMeter::~LoudnessMeter()
{
    stopTimer();
}

//==============================================================================
void LoudnessMeter::setIntegratedLoudness(float lufs)
{
    integratedLoudness.store(lufs, std::memory_order_relaxed);
}

void LoudnessMeter::setShortTermLoudness(float lufs)
{
    shortTermLoudness.store(lufs, std::memory_order_relaxed);
}

void LoudnessMeter::setMomentaryLoudness(float lufs)
{
    momentaryLoudness.store(lufs, std::memory_order_relaxed);
}

void LoudnessMeter::setLoudnessRange(float lra)
{
    loudnessRange.store(lra, std::memory_order_relaxed);
}

void LoudnessMeter::reset()
{
    integratedLoudness.store(-70.0f, std::memory_order_relaxed);
    shortTermLoudness.store(-70.0f, std::memory_order_relaxed);
    momentaryLoudness.store(-70.0f, std::memory_order_relaxed);
    loudnessRange.store(0.0f, std::memory_order_relaxed);

    displayIntegrated = -70.0f;
    displayShortTerm = -70.0f;
    displayMomentary = -70.0f;
    displayLRA = 0.0f;
}

//==============================================================================
void LoudnessMeter::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("LOUDNESS (LUFS)", getLocalBounds().removeFromTop(25),
              juce::Justification::centred);

    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(25);

    // Split into meter area and numeric values
    int numericHeight = 80;
    auto numericBounds = bounds.removeFromBottom(numericHeight);
    auto meterBounds = bounds;

    // Draw meters (Momentary, Short-term, Integrated)
    int meterWidth = (meterBounds.getWidth() - 20) / 3;

    auto momentaryBounds = meterBounds.removeFromLeft(meterWidth);
    meterBounds.removeFromLeft(10);
    auto shortTermBounds = meterBounds.removeFromLeft(meterWidth);
    meterBounds.removeFromLeft(10);
    auto integratedBounds = meterBounds;

    drawMeter(g, momentaryBounds, displayMomentary, "M");
    drawMeter(g, shortTermBounds, displayShortTerm, "S");
    drawMeter(g, integratedBounds, displayIntegrated, "I");

    // Draw numeric values
    drawNumericValues(g, numericBounds);
}

void LoudnessMeter::resized()
{
    // Layout handled in paint
}

void LoudnessMeter::mouseDown(const juce::MouseEvent& /*event*/)
{
    reset();
    repaint();
}

//==============================================================================
void LoudnessMeter::timerCallback()
{
    // Get current values with smoothing
    float targetIntegrated = integratedLoudness.load(std::memory_order_relaxed);
    float targetShortTerm = shortTermLoudness.load(std::memory_order_relaxed);
    float targetMomentary = momentaryLoudness.load(std::memory_order_relaxed);
    float targetLRA = loudnessRange.load(std::memory_order_relaxed);

    displayIntegrated = displayIntegrated * SMOOTHING + targetIntegrated * (1.0f - SMOOTHING);
    displayShortTerm = displayShortTerm * SMOOTHING + targetShortTerm * (1.0f - SMOOTHING);
    displayMomentary = displayMomentary * SMOOTHING + targetMomentary * (1.0f - SMOOTHING);
    displayLRA = displayLRA * SMOOTHING + targetLRA * (1.0f - SMOOTHING);

    repaint();
}

//==============================================================================
void LoudnessMeter::drawMeter(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                               float value, const juce::String& label)
{
    auto workingBounds = bounds;

    // Draw label
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    auto labelBounds = workingBounds.removeFromTop(20);
    g.drawText(label, labelBounds, juce::Justification::centred);

    auto meterBounds = workingBounds.reduced(5);

    // Draw background
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(meterBounds);

    // Draw scale markings (-60 to 0 LUFS range, with -23 LUFS target line)
    g.setColour(juce::Colour(0xff3a3a3a));
    for (int lufs = -60; lufs <= 0; lufs += 6)
    {
        float normalized = (lufs + 60.0f) / 60.0f;  // 0.0 to 1.0
        float y = meterBounds.getY() + (1.0f - normalized) * meterBounds.getHeight();
        g.drawHorizontalLine((int)y, (float)meterBounds.getX(), (float)meterBounds.getRight());
    }

    // Draw target line (-23 LUFS)
    float targetNormalized = (TARGET_LEVEL + 60.0f) / 60.0f;
    float targetY = meterBounds.getY() + (1.0f - targetNormalized) * meterBounds.getHeight();
    g.setColour(juce::Colours::yellow.withAlpha(0.5f));
    g.drawHorizontalLine((int)targetY, (float)meterBounds.getX(), (float)meterBounds.getRight());

    // Draw meter bar
    float normalized = juce::jlimit(0.0f, 1.0f, (value + 60.0f) / 60.0f);
    int fillPixels = (int)(normalized * meterBounds.getHeight());

    if (fillPixels > 0)
    {
        auto fillBounds = meterBounds.removeFromBottom(fillPixels);
        g.setColour(getColorForLoudness(value));
        g.fillRect(fillBounds);
    }

    // Draw border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(workingBounds.reduced(5), 1);
}

void LoudnessMeter::drawNumericValues(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    auto workingBounds = bounds;

    // Draw integrated loudness (main value)
    g.setColour(getColorForLoudness(displayIntegrated));
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    juce::String integratedText = juce::String(displayIntegrated, 1) + " LUFS";
    auto integratedBounds = workingBounds.removeFromTop(30);
    g.drawText(integratedText, integratedBounds, juce::Justification::centred);

    // Draw short-term and momentary
    g.setFont(juce::Font(12.0f));
    auto row = workingBounds.removeFromTop(18);

    g.setColour(juce::Colours::lightgrey);
    g.drawText("Short-term:", row.removeFromLeft(80), juce::Justification::centredLeft);
    g.setColour(getColorForLoudness(displayShortTerm));
    juce::String shortTermText = juce::String(displayShortTerm, 1) + " LUFS";
    g.drawText(shortTermText, row, juce::Justification::centredLeft);

    row = workingBounds.removeFromTop(18);
    g.setColour(juce::Colours::lightgrey);
    g.drawText("Momentary:", row.removeFromLeft(80), juce::Justification::centredLeft);
    g.setColour(getColorForLoudness(displayMomentary));
    juce::String momentaryText = juce::String(displayMomentary, 1) + " LUFS";
    g.drawText(momentaryText, row, juce::Justification::centredLeft);

    // Draw loudness range
    row = workingBounds.removeFromTop(18);
    g.setColour(juce::Colours::lightgrey);
    g.drawText("Range:", row.removeFromLeft(80), juce::Justification::centredLeft);
    g.setColour(juce::Colours::cyan);
    juce::String lraText = juce::String(displayLRA, 1) + " LU";
    g.drawText(lraText, row, juce::Justification::centredLeft);
}

juce::Colour LoudnessMeter::getColorForLoudness(float lufs) const
{
    // Color coding based on EBU R128 standards
    // Target: -23 LUFS ±1 LU (green zone)
    // Warning: -18 LUFS (yellow zone for short-term max)
    // Over: > -13 LUFS (red zone)

    if (lufs > -13.0f)
        return juce::Colours::red;
    else if (lufs > -18.0f)
        return juce::Colours::orange;
    else if (lufs > -24.0f && lufs < -22.0f)
        return juce::Colours::green;  // Target zone
    else if (lufs > -28.0f)
        return juce::Colours::yellow;
    else
        return juce::Colours::grey;
}

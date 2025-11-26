/*
  ==============================================================================

    TruePeakMeter.cpp

    True Peak meter implementation

  ==============================================================================
*/

#include "TruePeakMeter.h"

//==============================================================================
TruePeakMeter::TruePeakMeter()
{
    startTimerHz(30);  // 30 FPS updates
}

TruePeakMeter::~TruePeakMeter()
{
    stopTimer();
}

//==============================================================================
void TruePeakMeter::setTruePeaks(float leftPeak, float rightPeak)
{
    leftTruePeak.store(leftPeak, std::memory_order_relaxed);
    rightTruePeak.store(rightPeak, std::memory_order_relaxed);
}

void TruePeakMeter::resetPeakHold()
{
    leftPeakHold = 0.0f;
    rightPeakHold = 0.0f;
    leftHoldTimer = 0;
    rightHoldTimer = 0;
    leftClipping = false;
    rightClipping = false;
    leftClipTimer = 0;
    rightClipTimer = 0;
}

//==============================================================================
void TruePeakMeter::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("TRUE PEAK", getLocalBounds().removeFromTop(25),
              juce::Justification::centred);

    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(25);

    // Split into two channels
    int channelWidth = (bounds.getWidth() - 10) / 2;
    auto leftBounds = bounds.removeFromLeft(channelWidth);
    bounds.removeFromLeft(10);  // Gap
    auto rightBounds = bounds;

    // Draw channels
    drawChannel(g, leftBounds, leftDisplayPeak, leftPeakHold, "L");
    drawChannel(g, rightBounds, rightDisplayPeak, rightPeakHold, "R");

    // Draw clipping indicators
    if (leftClipping)
    {
        g.setColour(juce::Colours::red);
        g.fillRect(leftBounds.removeFromTop(5));
    }

    if (rightClipping)
    {
        g.setColour(juce::Colours::red);
        g.fillRect(rightBounds.removeFromTop(5));
    }
}

void TruePeakMeter::resized()
{
    // Layout handled in paint
}

void TruePeakMeter::mouseDown(const juce::MouseEvent& /*event*/)
{
    resetPeakHold();
    repaint();
}

//==============================================================================
void TruePeakMeter::timerCallback()
{
    // Get current peak values
    float leftPeak = leftTruePeak.load(std::memory_order_relaxed);
    float rightPeak = rightTruePeak.load(std::memory_order_relaxed);

    // Update display peaks with decay
    if (leftPeak > leftDisplayPeak)
        leftDisplayPeak = leftPeak;
    else
        leftDisplayPeak *= DECAY_RATE;

    if (rightPeak > rightDisplayPeak)
        rightDisplayPeak = rightPeak;
    else
        rightDisplayPeak *= DECAY_RATE;

    // Update peak hold
    if (leftPeak > leftPeakHold)
    {
        leftPeakHold = leftPeak;
        leftHoldTimer = HOLD_TIME_MS / (1000 / 30);  // Convert ms to timer ticks
    }
    else if (leftHoldTimer > 0)
    {
        leftHoldTimer--;
    }
    else
    {
        leftPeakHold *= DECAY_RATE;
    }

    if (rightPeak > rightPeakHold)
    {
        rightPeakHold = rightPeak;
        rightHoldTimer = HOLD_TIME_MS / (1000 / 30);
    }
    else if (rightHoldTimer > 0)
    {
        rightHoldTimer--;
    }
    else
    {
        rightPeakHold *= DECAY_RATE;
    }

    // Check for clipping (> 0 dBTP)
    if (leftPeak > 1.0f)
    {
        leftClipping = true;
        leftClipTimer = CLIP_HOLD_TIME_MS / (1000 / 30);
    }
    else if (leftClipTimer > 0)
    {
        leftClipTimer--;
    }
    else
    {
        leftClipping = false;
    }

    if (rightPeak > 1.0f)
    {
        rightClipping = true;
        rightClipTimer = CLIP_HOLD_TIME_MS / (1000 / 30);
    }
    else if (rightClipTimer > 0)
    {
        rightClipTimer--;
    }
    else
    {
        rightClipping = false;
    }

    repaint();
}

//==============================================================================
void TruePeakMeter::drawChannel(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                                float peakValue, float holdValue, const juce::String& label)
{
    auto workingBounds = bounds;

    // Draw channel label
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    auto labelBounds = workingBounds.removeFromTop(20);
    g.drawText(label, labelBounds, juce::Justification::centred);

    auto meterBounds = workingBounds.reduced(5);

    // Draw background
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(meterBounds);

    // Draw scale markings
    g.setColour(juce::Colour(0xff3a3a3a));
    for (int db = -60; db <= 0; db += 6)
    {
        float y = meterBounds.getY() + (1.0f - (db + 60.0f) / 60.0f) * meterBounds.getHeight();
        g.drawHorizontalLine((int)y, (float)meterBounds.getX(), (float)meterBounds.getRight());
    }

    // Convert to dB
    float peakDb = linearToDb(peakValue);
    float holdDb = linearToDb(holdValue);

    // Draw meter bar
    float fillHeight = juce::jmax(0.0f, juce::jmin(1.0f, (peakDb + 60.0f) / 60.0f));
    int fillPixels = (int)(fillHeight * meterBounds.getHeight());

    if (fillPixels > 0)
    {
        auto fillBounds = meterBounds.removeFromBottom(fillPixels);

        // Gradient from green to yellow to red
        juce::ColourGradient gradient(
            juce::Colours::green,
            (float)fillBounds.getX(), (float)fillBounds.getBottom(),
            juce::Colours::red,
            (float)fillBounds.getX(), (float)fillBounds.getY(),
            false
        );
        gradient.addColour(0.5, juce::Colours::yellow);

        g.setGradientFill(gradient);
        g.fillRect(fillBounds);
    }

    // Draw peak hold line
    if (holdValue > 0.0f)
    {
        float holdHeight = juce::jmax(0.0f, juce::jmin(1.0f, (holdDb + 60.0f) / 60.0f));
        int holdY = meterBounds.getY() + (int)((1.0f - holdHeight) * meterBounds.getHeight());

        g.setColour(juce::Colours::white);
        g.drawHorizontalLine(holdY, (float)meterBounds.getX(), (float)meterBounds.getRight());
    }

    // Draw numeric value
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(11.0f));
    juce::String valueText = juce::String(peakDb, 1) + " dBTP";
    auto valueBounds = workingBounds.removeFromBottom(20);
    g.drawText(valueText, valueBounds, juce::Justification::centred);

    // Draw border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(meterBounds, 1);
}

juce::Colour TruePeakMeter::getColorForLevel(float dbValue) const
{
    if (dbValue > -3.0f)
        return juce::Colours::red;
    else if (dbValue > -6.0f)
        return juce::Colours::yellow;
    else
        return juce::Colours::green;
}

float TruePeakMeter::linearToDb(float linear) const
{
    if (linear <= 0.0f)
        return -60.0f;

    return juce::Decibels::gainToDecibels(linear);
}

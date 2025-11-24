/*
  ==============================================================================

    LevelMeter.cpp

    Real-time audio level meter implementation

  ==============================================================================
*/

#include "LevelMeter.h"

//==============================================================================
LevelMeter::LevelMeter()
{
    // Start timer for UI updates (30 fps)
    startTimer(33);
}

LevelMeter::~LevelMeter()
{
    stopTimer();
}

//==============================================================================
void LevelMeter::setLevels(float leftRMS, float leftPeak, float rightRMS, float rightPeak)
{
    // Thread-safe atomic write from audio thread
    channels[0].rms.store(leftRMS, std::memory_order_relaxed);
    channels[0].peak.store(leftPeak, std::memory_order_relaxed);
    channels[1].rms.store(rightRMS, std::memory_order_relaxed);
    channels[1].peak.store(rightPeak, std::memory_order_relaxed);
}

void LevelMeter::reset()
{
    for (auto& channel : channels)
    {
        channel.rms.store(0.0f, std::memory_order_relaxed);
        channel.peak.store(0.0f, std::memory_order_relaxed);
        channel.peakHold = 0.0f;
        channel.peakHoldTime = 0;
        channel.clipping = false;
    }
    repaint();
}

//==============================================================================
void LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);

    // Draw scale on the left
    auto scaleBounds = bounds.removeFromLeft(40);
    drawScale(g, scaleBounds);

    // Divide remaining space for channels
    auto channelWidth = bounds.getWidth() / 2;

    // Draw left channel
    auto leftBounds = bounds.removeFromLeft(channelWidth).reduced(5, 10);
    drawChannel(g, leftBounds, channels[0], "L");

    // Draw right channel
    auto rightBounds = bounds.reduced(5, 10);
    drawChannel(g, rightBounds, channels[1], "R");
}

void LevelMeter::drawScale(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));

    // Draw dB markers
    std::array<float, 7> markers = { 0.0f, -3.0f, -6.0f, -12.0f, -24.0f, -48.0f, -60.0f };

    for (float db : markers)
    {
        float y = levelToY(db, bounds.getHeight());
        int yPos = bounds.getY() + (int)y;

        // Draw tick mark
        g.drawHorizontalLine(yPos, (float)bounds.getRight() - 5, (float)bounds.getRight());

        // Draw label
        juce::String label = (db == 0.0f) ? "0" : juce::String((int)db);
        g.drawText(label, bounds.getX(), yPos - 6, bounds.getWidth() - 8, 12,
                  juce::Justification::centredRight);
    }
}

void LevelMeter::drawChannel(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                            const ChannelLevel& level, const juce::String& label)
{
    // Get current levels (thread-safe atomic read)
    float rmsLevel = level.rms.load(std::memory_order_relaxed);
    float peakLevel = level.peak.load(std::memory_order_relaxed);

    // Convert to dB
    float rmsDB = juce::Decibels::gainToDecibels(rmsLevel, MIN_DB);
    float peakDB = juce::Decibels::gainToDecibels(peakLevel, MIN_DB);

    // Clamp to range
    rmsDB = juce::jlimit(MIN_DB, MAX_DB, rmsDB);
    peakDB = juce::jlimit(MIN_DB, MAX_DB, peakDB);

    // Background (meter track)
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(bounds);

    // Draw RMS level bar
    float rmsY = levelToY(rmsDB, bounds.getHeight());
    auto rmsRect = bounds.withTop(bounds.getY() + (int)rmsY);

    // Color gradient based on level
    juce::ColourGradient gradient;
    if (rmsDB > -3.0f)
    {
        // Warning zone (yellow to red)
        gradient = juce::ColourGradient::vertical(
            juce::Colour(0xffff4444),  // Red at top
            bounds.getY(),
            juce::Colour(0xffffaa00),  // Orange/yellow
            bounds.getBottom()
        );
    }
    else if (rmsDB > -12.0f)
    {
        // Optimal zone (green)
        gradient = juce::ColourGradient::vertical(
            juce::Colour(0xff44ff44),  // Bright green at top
            bounds.getY(),
            juce::Colour(0xff00aa00),  // Dark green
            bounds.getBottom()
        );
    }
    else
    {
        // Low zone (blue to green)
        gradient = juce::ColourGradient::vertical(
            juce::Colour(0xff44ff44),  // Green at top
            bounds.getY(),
            juce::Colour(0xff0088ff),  // Blue
            bounds.getBottom()
        );
    }

    g.setGradientFill(gradient);
    g.fillRect(rmsRect);

    // Draw peak level line (brighter)
    float peakY = levelToY(peakDB, bounds.getHeight());
    g.setColour(juce::Colours::white);
    g.drawHorizontalLine(bounds.getY() + (int)peakY,
                        (float)bounds.getX(), (float)bounds.getRight());

    // Draw peak hold line
    if (level.peakHold > MIN_DB)
    {
        float holdY = levelToY(level.peakHold, bounds.getHeight());
        g.setColour(level.clipping ? juce::Colour(0xffff0000) : juce::Colour(0xffffff00));
        g.fillRect(bounds.getX(), bounds.getY() + (int)holdY - 1,
                  bounds.getWidth(), 3);
    }

    // Draw clipping indicator at top
    if (level.clipping)
    {
        g.setColour(juce::Colour(0xffff0000));
        g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), 5);
    }

    // Draw channel label at bottom
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    auto labelBounds = juce::Rectangle<int>(bounds.getX(), bounds.getBottom() - 20,
                                             bounds.getWidth(), 20);
    g.drawText(label, labelBounds, juce::Justification::centred);

    // Draw level value at top
    g.setFont(juce::Font(10.0f));
    juce::String levelText = levelToString(peakDB);
    g.drawText(levelText, bounds.getX(), bounds.getY() + 5,
              bounds.getWidth(), 15, juce::Justification::centred);
}

//==============================================================================
float LevelMeter::levelToY(float levelDB, int height) const
{
    // Map dB value to vertical position (inverted, 0dB at top)
    float normalized = (levelDB - MIN_DB) / (MAX_DB - MIN_DB);
    return height * (1.0f - normalized);
}

juce::String LevelMeter::levelToString(float levelDB) const
{
    if (levelDB <= MIN_DB)
        return "-inf";

    return juce::String(levelDB, 1) + " dB";
}

//==============================================================================
void LevelMeter::resized()
{
    // Nothing to do here
}

void LevelMeter::timerCallback()
{
    // Update peak hold for each channel
    for (auto& channel : channels)
    {
        float peakLevel = channel.peak.load(std::memory_order_relaxed);
        float peakDB = juce::Decibels::gainToDecibels(peakLevel, MIN_DB);

        // Update peak hold
        if (peakDB > channel.peakHold)
        {
            channel.peakHold = peakDB;
            channel.peakHoldTime = PEAK_HOLD_FRAMES;
            channel.clipping = (peakDB >= -0.1f);  // Clipping threshold
        }
        else if (channel.peakHoldTime > 0)
        {
            channel.peakHoldTime--;
        }
        else
        {
            // Decay peak hold
            channel.peakHold *= 0.95f;
            if (channel.peakHold < MIN_DB)
            {
                channel.peakHold = MIN_DB;
                channel.clipping = false;
            }
        }
    }

    repaint();
}

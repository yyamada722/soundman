/*
  ==============================================================================

    VectorscopeDisplay.cpp

    Vectorscope (Lissajous) display implementation - Optimized

  ==============================================================================
*/

#include "VectorscopeDisplay.h"

//==============================================================================
VectorscopeDisplay::VectorscopeDisplay()
{
    sampleBuffer.resize(MAX_POINTS);
    startTimer(50);  // 20 fps (reduced from 30)
}

VectorscopeDisplay::~VectorscopeDisplay()
{
    stopTimer();
}

//==============================================================================
void VectorscopeDisplay::pushSample(float leftSample, float rightSample)
{
    juce::ScopedLock lock(bufferLock);

    sampleBuffer[writeIndex].left = leftSample;
    sampleBuffer[writeIndex].right = rightSample;

    writeIndex++;
    if (writeIndex >= MAX_POINTS)
    {
        writeIndex = 0;
        bufferFull = true;
    }

    pathNeedsUpdate = true;
}

void VectorscopeDisplay::clear()
{
    juce::ScopedLock lock(bufferLock);

    for (auto& sample : sampleBuffer)
    {
        sample.left = 0.0f;
        sample.right = 0.0f;
    }

    writeIndex = 0;
    bufferFull = false;
    cachedPath.clear();
    pathNeedsUpdate = true;
}

//==============================================================================
void VectorscopeDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff0a0a0a));

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);

    // Draw grid and labels
    drawGrid(g, bounds.reduced(2));

    // Draw vectorscope
    drawVectorscope(g, bounds.reduced(2));
}

void VectorscopeDisplay::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colour(0xff2a2a2a));

    // Center point
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();

    // Horizontal and vertical center lines
    g.drawHorizontalLine((int)centerY, (float)bounds.getX(), (float)bounds.getRight());
    g.drawVerticalLine((int)centerX, (float)bounds.getY(), (float)bounds.getBottom());

    // Circle grid - just outer circle for performance
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;
    g.drawEllipse(centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    // Labels
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));

    g.drawText("L", bounds.getX() + 5, centerY - 15, 20, 15, juce::Justification::centred);
    g.drawText("R", centerX - 10, bounds.getY() + 5, 20, 15, juce::Justification::centred);
}

void VectorscopeDisplay::drawVectorscope(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    juce::ScopedLock lock(bufferLock);

    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();
    float scale = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.45f;

    // Determine how many points to draw
    int numPoints = bufferFull ? MAX_POINTS : writeIndex;
    if (numPoints < 2)
        return;

    // Rebuild path if needed
    if (pathNeedsUpdate)
    {
        cachedPath.clear();

        bool started = false;
        for (int i = 0; i < numPoints; i += DRAW_STEP)
        {
            const auto& p = sampleBuffer[i];

            // Calculate position (L-R on X axis, L+R on Y axis)
            float x = centerX + (p.left - p.right) * scale;
            float y = centerY - (p.left + p.right) * scale * 0.5f;

            if (!started)
            {
                cachedPath.startNewSubPath(x, y);
                started = true;
            }
            else
            {
                cachedPath.lineTo(x, y);
            }
        }

        pathNeedsUpdate = false;
    }
    else
    {
        // Translate cached path to current center (in case of resize)
        // For now, just use as-is
    }

    // Draw the path with a gradient color based on recent audio activity
    float avgAmplitude = 0.0f;
    int sampleCount = juce::jmin(32, numPoints);
    int startIdx = (writeIndex - sampleCount + MAX_POINTS) % MAX_POINTS;

    for (int i = 0; i < sampleCount; ++i)
    {
        int idx = (startIdx + i) % MAX_POINTS;
        const auto& p = sampleBuffer[idx];
        avgAmplitude += std::abs(p.left) + std::abs(p.right);
    }
    avgAmplitude /= (sampleCount * 2.0f);

    // Color based on amplitude
    juce::Colour pathColor = juce::Colour::fromHSV(0.5f + avgAmplitude * 0.3f, 0.7f, 0.8f, 0.7f);
    g.setColour(pathColor);
    g.strokePath(cachedPath, juce::PathStrokeType(1.5f));

    // Draw a brighter dot at the current position
    if (numPoints > 0)
    {
        int currentIdx = (writeIndex > 0) ? writeIndex - 1 : MAX_POINTS - 1;
        const auto& current = sampleBuffer[currentIdx];

        float x = centerX + (current.left - current.right) * scale;
        float y = centerY - (current.left + current.right) * scale * 0.5f;

        g.setColour(juce::Colours::white);
        g.fillEllipse(x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    }
}

void VectorscopeDisplay::resized()
{
    pathNeedsUpdate = true;
}

void VectorscopeDisplay::timerCallback()
{
    repaint();
}

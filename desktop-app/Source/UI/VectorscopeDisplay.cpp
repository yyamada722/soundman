/*
  ==============================================================================

    VectorscopeDisplay.cpp

    Vectorscope (Lissajous) display implementation

  ==============================================================================
*/

#include "VectorscopeDisplay.h"

//==============================================================================
VectorscopeDisplay::VectorscopeDisplay()
{
    sampleBuffer.resize(MAX_POINTS);
    startTimer(30);  // ~30 fps
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

    // Circle grid (45 degree angles)
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;

    for (int i = 1; i <= 3; ++i)
    {
        float r = radius * i / 3.0f;
        g.drawEllipse(centerX - r, centerY - r, r * 2.0f, r * 2.0f, 1.0f);
    }

    // Diagonal lines (L+R and L-R)
    g.setColour(juce::Colour(0xff1a1a1a));
    float diagLen = radius * 1.2f;

    // +45 degree (mono)
    g.drawLine(centerX - diagLen, centerY + diagLen,
               centerX + diagLen, centerY - diagLen, 1.0f);

    // -45 degree (out of phase)
    g.drawLine(centerX - diagLen, centerY - diagLen,
               centerX + diagLen, centerY + diagLen, 1.0f);

    // Labels
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));

    g.drawText("L", bounds.getX() + 5, centerY - 15, 20, 15, juce::Justification::centred);
    g.drawText("R", centerX - 10, bounds.getY() + 5, 20, 15, juce::Justification::centred);
    g.drawText("Mono", centerX + 5, bounds.getY() + 5, 50, 15, juce::Justification::centredLeft);
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

    // Draw points with fading trail
    for (int i = 0; i < numPoints - 1; ++i)
    {
        const auto& p1 = sampleBuffer[i];
        const auto& p2 = sampleBuffer[i + 1];

        // Calculate position (L-R on X axis, L+R on Y axis)
        // This creates the classic Lissajous figure
        float x1 = centerX + (p1.left - p1.right) * scale;
        float y1 = centerY - (p1.left + p1.right) * scale * 0.5f;

        float x2 = centerX + (p2.left - p2.right) * scale;
        float y2 = centerY - (p2.left + p2.right) * scale * 0.5f;

        // Fade based on position in buffer
        float fade = (float)i / (float)numPoints;
        fade = fade * 0.7f + 0.3f;  // Keep minimum brightness

        // Color gradient based on amplitude
        float amplitude = std::sqrt(p1.left * p1.left + p1.right * p1.right);
        juce::Colour color = juce::Colour::fromHSV(0.5f + amplitude * 0.3f, 0.8f, fade, fade);

        g.setColour(color);
        g.drawLine(x1, y1, x2, y2, 1.5f);
    }

    // Draw a brighter dot at the current position
    if (numPoints > 0)
    {
        int currentIdx = (writeIndex > 0) ? writeIndex - 1 : MAX_POINTS - 1;
        const auto& current = sampleBuffer[currentIdx];

        float x = centerX + (current.left - current.right) * scale;
        float y = centerY - (current.left + current.right) * scale * 0.5f;

        g.setColour(juce::Colours::white);
        g.fillEllipse(x - 2.0f, y - 2.0f, 4.0f, 4.0f);
    }
}

void VectorscopeDisplay::resized()
{
    // Nothing to resize
}

void VectorscopeDisplay::timerCallback()
{
    repaint();
}

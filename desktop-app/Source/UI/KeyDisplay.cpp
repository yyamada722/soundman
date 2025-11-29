/*
  ==============================================================================

    KeyDisplay.cpp

    Musical key detection display implementation

  ==============================================================================
*/

#include "KeyDisplay.h"

//==============================================================================
KeyDisplay::KeyDisplay()
{
    startTimerHz(20);
}

KeyDisplay::~KeyDisplay()
{
    stopTimer();
}

//==============================================================================
void KeyDisplay::prepare(double sampleRate, int samplesPerBlock)
{
    detector.prepare(sampleRate, samplesPerBlock);
}

void KeyDisplay::processBlock(const juce::AudioBuffer<float>& buffer)
{
    detector.processBlock(buffer);
}

//==============================================================================
void KeyDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    auto bounds = getLocalBounds().reduced(10);

    // Split into three sections
    int sectionWidth = bounds.getWidth() / 3;

    auto keyArea = bounds.removeFromLeft(sectionWidth - 5);
    bounds.removeFromLeft(10);
    auto chromaArea = bounds.removeFromLeft(sectionWidth - 5);
    bounds.removeFromLeft(10);
    auto circleArea = bounds;

    drawKeyDisplay(g, keyArea);
    drawChromaDisplay(g, chromaArea);
    drawCircleOfFifths(g, circleArea);
}

void KeyDisplay::resized()
{
    // No controls to layout
}

//==============================================================================
void KeyDisplay::timerCallback()
{
    displayKey = detector.getDetectedKey();
    displayConfidence = detector.getConfidence();
    displayChroma = detector.getChroma();
    displayCorrelations = detector.getKeyCorrelations();

    repaint();
}

//==============================================================================
void KeyDisplay::drawKeyDisplay(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(12.0f));
    g.drawText("Detected Key", bounds.getX() + 10, bounds.getY() + 10, 100, 15, juce::Justification::centredLeft);

    // Key name - large display
    auto keyArea = bounds.reduced(10);
    keyArea.removeFromTop(30);

    juce::String keyName = KeyDetector::getKeyName(displayKey);

    // Color based on major/minor
    if (displayKey != KeyDetector::Key::Unknown)
    {
        int keyIndex = static_cast<int>(displayKey);
        bool isMajor = keyIndex < 12;

        if (isMajor)
            g.setColour(juce::Colour(0xff4a9eff));  // Blue for major
        else
            g.setColour(juce::Colour(0xffff9e4a));  // Orange for minor

        g.setFont(juce::Font(36.0f, juce::Font::bold));
        g.drawText(keyName, keyArea.removeFromTop(50), juce::Justification::centred);

        // Scale type indicator
        g.setFont(juce::Font(14.0f));
        g.setColour(juce::Colours::lightgrey);

        juce::String scaleInfo = isMajor ? "Major Scale" : "Minor Scale";
        g.drawText(scaleInfo, keyArea.removeFromTop(25), juce::Justification::centred);
    }
    else
    {
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(36.0f, juce::Font::bold));
        g.drawText("---", keyArea.removeFromTop(50), juce::Justification::centred);
    }

    // Confidence bar
    keyArea.removeFromTop(20);
    auto confArea = keyArea.removeFromTop(30).reduced(10, 5);

    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));
    g.drawText("Confidence", confArea.removeFromTop(12), juce::Justification::centredLeft);

    auto barBounds = confArea.reduced(0, 2);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRoundedRectangle(barBounds.toFloat(), 3.0f);

    float filledWidth = barBounds.getWidth() * displayConfidence;
    g.setColour(juce::Colour(0xff4a9eff));
    g.fillRoundedRectangle((float)barBounds.getX(), (float)barBounds.getY(),
                           filledWidth, (float)barBounds.getHeight(), 3.0f);

    // Relative/Parallel key info
    if (displayKey != KeyDetector::Key::Unknown)
    {
        int keyIndex = static_cast<int>(displayKey);
        bool isMajor = keyIndex < 12;
        int root = keyIndex % 12;

        // Calculate relative key (major -> relative minor, minor -> relative major)
        int relativeRoot = isMajor ? (root + 9) % 12 : (root + 3) % 12;
        KeyDetector::Key relativeKey = static_cast<KeyDetector::Key>(isMajor ? relativeRoot + 12 : relativeRoot);

        keyArea.removeFromTop(15);
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(11.0f));
        g.drawText("Relative: " + KeyDetector::getKeyName(relativeKey),
                   keyArea.removeFromTop(18), juce::Justification::centred);

        // Parallel key
        KeyDetector::Key parallelKey = static_cast<KeyDetector::Key>(isMajor ? root + 12 : root);
        g.drawText("Parallel: " + KeyDetector::getKeyName(parallelKey),
                   keyArea.removeFromTop(18), juce::Justification::centred);
    }
}

void KeyDisplay::drawChromaDisplay(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(12.0f));
    g.drawText("Chroma Features", bounds.getX() + 10, bounds.getY() + 10, 120, 15, juce::Justification::centredLeft);

    auto chromaArea = bounds.reduced(15, 35);
    chromaArea.removeFromTop(5);

    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    float barWidth = chromaArea.getWidth() / 12.0f;
    float maxHeight = chromaArea.getHeight() - 20;

    // Find max for normalization
    float maxChroma = 0.001f;
    for (int i = 0; i < 12; ++i)
        maxChroma = std::max(maxChroma, displayChroma[i]);

    for (int i = 0; i < 12; ++i)
    {
        float x = chromaArea.getX() + i * barWidth;
        float barHeight = (displayChroma[i] / maxChroma) * maxHeight;
        float y = chromaArea.getBottom() - 15 - barHeight;

        // Determine if this is the root of the detected key
        bool isRoot = false;
        if (displayKey != KeyDetector::Key::Unknown)
        {
            int keyRoot = static_cast<int>(displayKey) % 12;
            isRoot = (i == keyRoot);
        }

        // Bar color
        if (isRoot)
            g.setColour(juce::Colour(0xff4a9eff));
        else
        {
            // Color based on whether it's a black or white key
            bool isBlackKey = (i == 1 || i == 3 || i == 6 || i == 8 || i == 10);
            g.setColour(isBlackKey ? juce::Colour(0xff6a6a6a) : juce::Colour(0xff9a9a9a));
        }

        g.fillRoundedRectangle(x + 2, y, barWidth - 4, barHeight, 2.0f);

        // Note name
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(9.0f));
        g.drawText(noteNames[i], (int)x, chromaArea.getBottom() - 12, (int)barWidth, 12, juce::Justification::centred);
    }
}

void KeyDisplay::drawCircleOfFifths(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(12.0f));
    g.drawText("Circle of Fifths", bounds.getX() + 10, bounds.getY() + 10, 100, 15, juce::Justification::centredLeft);

    auto circleArea = bounds.reduced(15, 35);
    circleArea.removeFromTop(5);

    // Circle parameters
    float centerX = circleArea.getCentreX();
    float centerY = circleArea.getCentreY();
    float radius = juce::jmin(circleArea.getWidth(), circleArea.getHeight()) * 0.4f;
    float innerRadius = radius * 0.6f;

    // Order of keys in circle of fifths (starting from C at top)
    static const int circleOrder[] = { 0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5 };  // C, G, D, A, E, B, F#, C#, G#, D#, A#, F
    static const char* majorNames[] = { "C", "G", "D", "A", "E", "B", "F#", "Db", "Ab", "Eb", "Bb", "F" };
    static const char* minorNames[] = { "Am", "Em", "Bm", "F#m", "C#m", "G#m", "D#m", "Bbm", "Fm", "Cm", "Gm", "Dm" };

    // Draw segments
    for (int i = 0; i < 12; ++i)
    {
        float angle = (i * 30.0f - 90.0f) * juce::MathConstants<float>::pi / 180.0f;
        float nextAngle = ((i + 1) * 30.0f - 90.0f) * juce::MathConstants<float>::pi / 180.0f;

        int keyIdx = circleOrder[i];

        // Determine if this key is detected
        bool isMajorDetected = (displayKey != KeyDetector::Key::Unknown &&
                                static_cast<int>(displayKey) == keyIdx);
        bool isMinorDetected = (displayKey != KeyDetector::Key::Unknown &&
                                static_cast<int>(displayKey) == keyIdx + 12);

        // Outer ring (major keys)
        {
            juce::Path segment;
            segment.addPieSegment(centerX - radius, centerY - radius,
                                  radius * 2, radius * 2,
                                  angle, nextAngle, innerRadius / radius);

            float correlation = displayCorrelations[keyIdx];
            float brightness = 0.2f + correlation * 0.3f;

            if (isMajorDetected)
                g.setColour(juce::Colour(0xff4a9eff));
            else
                g.setColour(juce::Colour::fromHSV(0.6f, 0.3f, brightness, 1.0f));

            g.fillPath(segment);
            g.setColour(juce::Colour(0xff3a3a3a));
            g.strokePath(segment, juce::PathStrokeType(1.0f));
        }

        // Inner ring (minor keys)
        {
            juce::Path segment;
            segment.addPieSegment(centerX - innerRadius, centerY - innerRadius,
                                  innerRadius * 2, innerRadius * 2,
                                  angle, nextAngle, 0.0f);

            float correlation = displayCorrelations[keyIdx + 12];
            float brightness = 0.2f + correlation * 0.3f;

            if (isMinorDetected)
                g.setColour(juce::Colour(0xffff9e4a));
            else
                g.setColour(juce::Colour::fromHSV(0.08f, 0.3f, brightness, 1.0f));

            g.fillPath(segment);
            g.setColour(juce::Colour(0xff3a3a3a));
            g.strokePath(segment, juce::PathStrokeType(1.0f));
        }

        // Labels
        float labelAngle = (i * 30.0f - 90.0f) * juce::MathConstants<float>::pi / 180.0f;
        float midAngle = labelAngle + 15.0f * juce::MathConstants<float>::pi / 180.0f;

        // Major key label (outer)
        float labelRadius = (radius + innerRadius) * 0.5f;
        float labelX = centerX + std::cos(midAngle) * labelRadius;
        float labelY = centerY + std::sin(midAngle) * labelRadius;

        g.setColour(isMajorDetected ? juce::Colours::white : juce::Colours::lightgrey);
        g.setFont(juce::Font(isMajorDetected ? 11.0f : 9.0f, isMajorDetected ? juce::Font::bold : juce::Font::plain));
        g.drawText(majorNames[i], (int)labelX - 15, (int)labelY - 7, 30, 14, juce::Justification::centred);

        // Minor key label (inner)
        labelRadius = innerRadius * 0.5f;
        labelX = centerX + std::cos(midAngle) * labelRadius;
        labelY = centerY + std::sin(midAngle) * labelRadius;

        g.setColour(isMinorDetected ? juce::Colours::white : juce::Colours::grey);
        g.setFont(juce::Font(isMinorDetected ? 10.0f : 8.0f, isMinorDetected ? juce::Font::bold : juce::Font::plain));
        g.drawText(minorNames[i], (int)labelX - 15, (int)labelY - 6, 30, 12, juce::Justification::centred);
    }
}

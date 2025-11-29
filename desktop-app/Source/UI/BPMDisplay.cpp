/*
  ==============================================================================

    BPMDisplay.cpp

    BPM (Beats Per Minute) detection display implementation

  ==============================================================================
*/

#include "BPMDisplay.h"

//==============================================================================
BPMDisplay::BPMDisplay()
{
    // Min BPM slider
    minBPMSlider.setRange(30, 200, 1);
    minBPMSlider.setValue(60);
    minBPMSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    minBPMSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 20);
    minBPMSlider.addListener(this);
    addAndMakeVisible(minBPMSlider);

    minBPMLabel.setFont(juce::Font(11.0f));
    minBPMLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(minBPMLabel);

    // Max BPM slider
    maxBPMSlider.setRange(60, 300, 1);
    maxBPMSlider.setValue(200);
    maxBPMSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    maxBPMSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 20);
    maxBPMSlider.addListener(this);
    addAndMakeVisible(maxBPMSlider);

    maxBPMLabel.setFont(juce::Font(11.0f));
    maxBPMLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(maxBPMLabel);

    // Tap tempo button
    tapTempoButton.onClick = [this]()
    {
        int64_t now = juce::Time::currentTimeMillis();
        tapTimes.push_back(now);

        // Keep only last 8 taps
        while (tapTimes.size() > 8)
            tapTimes.erase(tapTimes.begin());

        // Calculate BPM from tap intervals
        if (tapTimes.size() >= 2)
        {
            float totalInterval = 0;
            int count = 0;

            for (size_t i = 1; i < tapTimes.size(); ++i)
            {
                int64_t interval = tapTimes[i] - tapTimes[i - 1];
                // Ignore intervals > 2 seconds (reset)
                if (interval < 2000)
                {
                    totalInterval += interval;
                    count++;
                }
                else
                {
                    // Reset on long pause
                    tapTimes.clear();
                    tapTimes.push_back(now);
                    tapBPM = 0;
                    return;
                }
            }

            if (count > 0)
            {
                float avgInterval = totalInterval / count;
                tapBPM = 60000.0f / avgInterval;
            }
        }
    };
    addAndMakeVisible(tapTempoButton);

    startTimerHz(30);
}

BPMDisplay::~BPMDisplay()
{
    stopTimer();
}

//==============================================================================
void BPMDisplay::prepare(double sampleRate, int samplesPerBlock)
{
    detector.prepare(sampleRate, samplesPerBlock);
}

void BPMDisplay::processBlock(const juce::AudioBuffer<float>& buffer)
{
    detector.processBlock(buffer);
}

//==============================================================================
void BPMDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    auto bounds = getLocalBounds().reduced(10);

    // Controls area at top
    auto controlsArea = bounds.removeFromTop(60);
    bounds.removeFromTop(10);

    // Split remaining area
    auto leftArea = bounds.removeFromLeft(bounds.getWidth() / 2 - 5);
    bounds.removeFromLeft(10);
    auto rightArea = bounds;

    // Draw BPM value on left
    drawBPMValue(g, leftArea);

    // Draw onset graph on right
    drawOnsetGraph(g, rightArea);
}

void BPMDisplay::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    auto controlsArea = bounds.removeFromTop(55);

    // First row
    auto row = controlsArea.removeFromTop(25);
    minBPMLabel.setBounds(row.removeFromLeft(60));
    minBPMSlider.setBounds(row.removeFromLeft(150));
    row.removeFromLeft(20);
    maxBPMLabel.setBounds(row.removeFromLeft(60));
    maxBPMSlider.setBounds(row.removeFromLeft(150));

    controlsArea.removeFromTop(5);

    // Second row
    row = controlsArea.removeFromTop(25);
    tapTempoButton.setBounds(row.removeFromLeft(80));
}

//==============================================================================
void BPMDisplay::timerCallback()
{
    displayBPM = detector.getBPM();
    displayConfidence = detector.getConfidence();

    // Beat flash effect
    if (detector.isBeatDetected())
    {
        beatFlash = true;
        beatFlashCounter = 5;  // Flash for 5 frames
    }
    else if (beatFlashCounter > 0)
    {
        beatFlashCounter--;
        if (beatFlashCounter == 0)
            beatFlash = false;
    }

    repaint();
}

void BPMDisplay::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &minBPMSlider)
    {
        float minBPM = (float)minBPMSlider.getValue();
        detector.setMinBPM(minBPM);

        // Ensure max is greater than min
        if (maxBPMSlider.getValue() <= minBPM)
            maxBPMSlider.setValue(minBPM + 10);
    }
    else if (slider == &maxBPMSlider)
    {
        float maxBPM = (float)maxBPMSlider.getValue();
        detector.setMaxBPM(maxBPM);

        // Ensure min is less than max
        if (minBPMSlider.getValue() >= maxBPM)
            minBPMSlider.setValue(maxBPM - 10);
    }
}

//==============================================================================
void BPMDisplay::drawBPMValue(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Beat flash effect
    if (beatFlash)
    {
        float alpha = beatFlashCounter / 5.0f;
        g.setColour(juce::Colour(0xffff6b6b).withAlpha(alpha * 0.3f));
        g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
    }

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(12.0f));
    g.drawText("Detected BPM", bounds.getX() + 10, bounds.getY() + 10, 100, 15, juce::Justification::centredLeft);

    // BPM value - large display
    auto bpmArea = bounds.reduced(20);
    bpmArea.removeFromTop(30);

    g.setFont(juce::Font(72.0f, juce::Font::bold));

    if (displayBPM > 0)
    {
        // Color based on confidence
        float hue = 0.3f + displayConfidence * 0.2f;  // Green to cyan
        g.setColour(juce::Colour::fromHSV(hue, 0.7f, 0.9f, 1.0f));
        g.drawText(juce::String((int)displayBPM), bpmArea.removeFromTop(80), juce::Justification::centred);
    }
    else
    {
        g.setColour(juce::Colours::grey);
        g.drawText("---", bpmArea.removeFromTop(80), juce::Justification::centred);
    }

    // Confidence bar
    auto confBounds = bpmArea.removeFromTop(20).reduced(20, 0);
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));
    g.drawText("Confidence", confBounds.removeFromLeft(70), juce::Justification::centredLeft);

    auto barBounds = confBounds.reduced(5, 5);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRoundedRectangle(barBounds.toFloat(), 3.0f);

    auto filledWidth = barBounds.getWidth() * displayConfidence;
    g.setColour(juce::Colour(0xff4a9eff));
    g.fillRoundedRectangle(barBounds.getX(), barBounds.getY(),
                           filledWidth, barBounds.getHeight(), 3.0f);

    // Tap tempo display
    if (tapBPM > 0)
    {
        g.setColour(juce::Colours::orange);
        g.setFont(juce::Font(14.0f));
        g.drawText("Tap: " + juce::String((int)tapBPM) + " BPM",
                   bounds.getX() + 10, bounds.getBottom() - 30, 150, 20,
                   juce::Justification::centredLeft);
    }
}

void BPMDisplay::drawOnsetGraph(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(12.0f));
    g.drawText("Onset Strength", bounds.getX() + 10, bounds.getY() + 10, 120, 15, juce::Justification::centredLeft);

    auto graphBounds = bounds.reduced(15, 35);
    graphBounds.removeFromTop(5);

    // Grid
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawHorizontalLine(graphBounds.getCentreY(), (float)graphBounds.getX(), (float)graphBounds.getRight());

    // Draw onset strength
    const auto& onset = detector.getOnsetStrength();
    if (onset.empty())
        return;

    // Find max for normalization
    float maxVal = 0.001f;
    for (float v : onset)
        maxVal = std::max(maxVal, v);

    juce::Path onsetPath;
    int numPoints = (int)onset.size();

    for (int i = 0; i < numPoints; ++i)
    {
        float x = graphBounds.getX() + (float)i / numPoints * graphBounds.getWidth();
        float normalized = onset[i] / maxVal;
        float y = graphBounds.getBottom() - normalized * graphBounds.getHeight();

        if (i == 0)
            onsetPath.startNewSubPath(x, y);
        else
            onsetPath.lineTo(x, y);
    }

    g.setColour(juce::Colour(0xff4a9eff));
    g.strokePath(onsetPath, juce::PathStrokeType(1.5f));

    // Draw autocorrelation below (if space)
    if (graphBounds.getHeight() > 100)
    {
        auto autoArea = graphBounds.removeFromBottom(graphBounds.getHeight() / 3);
        autoArea.removeFromTop(10);

        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(10.0f));
        g.drawText("Autocorrelation", autoArea.getX(), autoArea.getY() - 15, 100, 12, juce::Justification::centredLeft);

        const auto& autocorr = detector.getAutocorrelation();
        if (!autocorr.empty())
        {
            juce::Path autoPath;
            int numAutoPts = (int)autocorr.size();

            for (int i = 0; i < numAutoPts; ++i)
            {
                float x = autoArea.getX() + (float)i / numAutoPts * autoArea.getWidth();
                float y = autoArea.getCentreY() - autocorr[i] * autoArea.getHeight() * 0.45f;

                if (i == 0)
                    autoPath.startNewSubPath(x, y);
                else
                    autoPath.lineTo(x, y);
            }

            g.setColour(juce::Colour(0xff00cc66));
            g.strokePath(autoPath, juce::PathStrokeType(1.0f));
        }
    }
}

void BPMDisplay::drawBeatIndicator(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Simple beat indicator circle
    auto indicatorBounds = bounds.reduced(10).withSizeKeepingCentre(40, 40);

    if (beatFlash)
    {
        g.setColour(juce::Colour(0xffff6b6b));
        g.fillEllipse(indicatorBounds.toFloat());
    }
    else
    {
        g.setColour(juce::Colour(0xff4a4a4a));
        g.fillEllipse(indicatorBounds.toFloat());
    }

    g.setColour(juce::Colours::grey);
    g.drawEllipse(indicatorBounds.toFloat(), 1.0f);
}

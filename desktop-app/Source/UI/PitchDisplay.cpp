/*
  ==============================================================================

    PitchDisplay.cpp

    Real-time pitch detection display implementation

  ==============================================================================
*/

#include "PitchDisplay.h"
#include <cmath>

//==============================================================================
PitchDisplay::PitchDisplay()
{
    pitchHistory.resize(maxHistoryLength, 0.0f);
    setupControls();
    startTimerHz(30);
}

void PitchDisplay::setupControls()
{
    // Threshold slider (0.1 - 0.8)
    thresholdSlider.setRange(0.1, 0.8, 0.05);
    thresholdSlider.setValue(pitchDetector.getThreshold());
    thresholdSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    thresholdSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    thresholdSlider.addListener(this);
    addAndMakeVisible(thresholdSlider);

    thresholdLabel.setFont(juce::Font(11.0f));
    thresholdLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(thresholdLabel);

    thresholdValueLabel.setFont(juce::Font(11.0f));
    thresholdValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    thresholdValueLabel.setText(juce::String(pitchDetector.getThreshold(), 2), juce::dontSendNotification);
    addAndMakeVisible(thresholdValueLabel);

    // Min frequency slider (20 - 500 Hz)
    minFreqSlider.setRange(20, 500, 1);
    minFreqSlider.setValue(pitchDetector.getMinFrequency());
    minFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    minFreqSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    minFreqSlider.setSkewFactorFromMidPoint(100);
    minFreqSlider.addListener(this);
    addAndMakeVisible(minFreqSlider);

    minFreqLabel.setFont(juce::Font(11.0f));
    minFreqLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(minFreqLabel);

    minFreqValueLabel.setFont(juce::Font(11.0f));
    minFreqValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    minFreqValueLabel.setText(juce::String((int)pitchDetector.getMinFrequency()) + " Hz", juce::dontSendNotification);
    addAndMakeVisible(minFreqValueLabel);

    // Max frequency slider (500 - 5000 Hz)
    maxFreqSlider.setRange(500, 5000, 10);
    maxFreqSlider.setValue(pitchDetector.getMaxFrequency());
    maxFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    maxFreqSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    maxFreqSlider.setSkewFactorFromMidPoint(1500);
    maxFreqSlider.addListener(this);
    addAndMakeVisible(maxFreqSlider);

    maxFreqLabel.setFont(juce::Font(11.0f));
    maxFreqLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(maxFreqLabel);

    maxFreqValueLabel.setFont(juce::Font(11.0f));
    maxFreqValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    maxFreqValueLabel.setText(juce::String((int)pitchDetector.getMaxFrequency()) + " Hz", juce::dontSendNotification);
    addAndMakeVisible(maxFreqValueLabel);
}

void PitchDisplay::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &thresholdSlider)
    {
        pitchDetector.setThreshold((float)thresholdSlider.getValue());
        thresholdValueLabel.setText(juce::String(thresholdSlider.getValue(), 2), juce::dontSendNotification);
    }
    else if (slider == &minFreqSlider)
    {
        pitchDetector.setMinFrequency((float)minFreqSlider.getValue());
        minFreqValueLabel.setText(juce::String((int)minFreqSlider.getValue()) + " Hz", juce::dontSendNotification);
    }
    else if (slider == &maxFreqSlider)
    {
        pitchDetector.setMaxFrequency((float)maxFreqSlider.getValue());
        maxFreqValueLabel.setText(juce::String((int)maxFreqSlider.getValue()) + " Hz", juce::dontSendNotification);
    }
}

PitchDisplay::~PitchDisplay()
{
    stopTimer();
}

//==============================================================================
void PitchDisplay::setPitchResult(const PitchDetector::PitchResult& result)
{
    currentPitch = result;

    // Update smoothed values
    if (result.isPitched)
    {
        smoothedFrequency = smoothedFrequency + smoothingFactor * (result.frequency - smoothedFrequency);
        smoothedCents = smoothedCents + smoothingFactor * (result.cents - smoothedCents);
        smoothedConfidence = smoothedConfidence + smoothingFactor * (result.confidence - smoothedConfidence);

        // Add to history
        pitchHistory.push_back(result.frequency);
        if ((int)pitchHistory.size() > maxHistoryLength)
            pitchHistory.pop_front();
    }
    else
    {
        smoothedConfidence *= 0.9f;  // Decay confidence when no pitch detected

        // Add zero to history for gaps
        pitchHistory.push_back(0.0f);
        if ((int)pitchHistory.size() > maxHistoryLength)
            pitchHistory.pop_front();
    }
}

void PitchDisplay::pushSample(float sample)
{
    pitchDetector.pushSample(sample);
}

void PitchDisplay::setSampleRate(double rate)
{
    pitchDetector.setSampleRate(rate);
}

void PitchDisplay::setHistoryLength(int length)
{
    maxHistoryLength = juce::jmax(50, length);
    while ((int)pitchHistory.size() > maxHistoryLength)
        pitchHistory.pop_front();
}

//==============================================================================
void PitchDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    drawBackground(g, bounds);

    // Layout - reserve space for controls at bottom
    auto workingBounds = bounds.reduced(10);
    workingBounds.removeFromBottom(95);  // Space for controls

    // Top section: Note display and frequency
    auto topSection = workingBounds.removeFromTop(100);
    auto noteArea = topSection.removeFromLeft(topSection.getWidth() / 2);
    auto freqArea = topSection;

    drawNoteDisplay(g, noteArea);
    drawFrequencyDisplay(g, freqArea);

    workingBounds.removeFromTop(10);

    // Middle section: Pitch meter (cents deviation)
    auto meterArea = workingBounds.removeFromTop(55);
    drawPitchMeter(g, meterArea);

    workingBounds.removeFromTop(5);

    // Confidence meter
    auto confidenceArea = workingBounds.removeFromTop(25);
    drawConfidenceMeter(g, confidenceArea);

    workingBounds.removeFromTop(5);

    // Pitch history graph (above controls)
    if (showHistory && workingBounds.getHeight() > 40)
    {
        drawPitchHistory(g, workingBounds);
    }

    // Draw separator line above controls
    auto controlsTop = getHeight() - 95;
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawHorizontalLine(controlsTop, 10.0f, (float)(getWidth() - 10));
}

void PitchDisplay::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Reserve bottom area for controls
    auto controlsArea = bounds.removeFromBottom(90);
    controlsArea.removeFromTop(5);

    // Layout controls in rows
    int labelWidth = 65;
    int valueWidth = 55;
    int rowHeight = 25;
    int sliderMargin = 5;

    // Threshold row
    auto row = controlsArea.removeFromTop(rowHeight);
    thresholdLabel.setBounds(row.removeFromLeft(labelWidth));
    thresholdValueLabel.setBounds(row.removeFromRight(valueWidth));
    row.removeFromLeft(sliderMargin);
    row.removeFromRight(sliderMargin);
    thresholdSlider.setBounds(row);

    controlsArea.removeFromTop(5);

    // Min frequency row
    row = controlsArea.removeFromTop(rowHeight);
    minFreqLabel.setBounds(row.removeFromLeft(labelWidth));
    minFreqValueLabel.setBounds(row.removeFromRight(valueWidth));
    row.removeFromLeft(sliderMargin);
    row.removeFromRight(sliderMargin);
    minFreqSlider.setBounds(row);

    controlsArea.removeFromTop(5);

    // Max frequency row
    row = controlsArea.removeFromTop(rowHeight);
    maxFreqLabel.setBounds(row.removeFromLeft(labelWidth));
    maxFreqValueLabel.setBounds(row.removeFromRight(valueWidth));
    row.removeFromLeft(sliderMargin);
    row.removeFromRight(sliderMargin);
    maxFreqSlider.setBounds(row);
}

void PitchDisplay::timerCallback()
{
    // Get latest pitch from internal detector
    auto result = pitchDetector.getLatestPitch();
    setPitchResult(result);
    repaint();
}

//==============================================================================
void PitchDisplay::drawBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRect(bounds, 1);
}

void PitchDisplay::drawNoteDisplay(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Note name
    g.setColour(currentPitch.isPitched ? getNoteColor(currentPitch.midiNote) : juce::Colours::grey);
    g.setFont(juce::Font(64.0f, juce::Font::bold));

    juce::String noteText = currentPitch.isPitched ? currentPitch.noteName : "---";
    g.drawText(noteText, bounds, juce::Justification::centred);

    // Label
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(12.0f));
    g.drawText("Note", bounds.getX(), bounds.getBottom() - 20, bounds.getWidth(), 20, juce::Justification::centred);
}

void PitchDisplay::drawFrequencyDisplay(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Frequency value
    g.setColour(currentPitch.isPitched ? juce::Colour(0xff4a9eff) : juce::Colours::grey);
    g.setFont(juce::Font(36.0f, juce::Font::bold));

    juce::String freqText;
    if (currentPitch.isPitched)
    {
        if (smoothedFrequency >= 1000.0f)
            freqText = juce::String(smoothedFrequency / 1000.0f, 2) + " kHz";
        else
            freqText = juce::String(smoothedFrequency, 1) + " Hz";
    }
    else
    {
        freqText = "--- Hz";
    }

    auto textBounds = bounds.reduced(5);
    textBounds.removeFromBottom(20);
    g.drawText(freqText, textBounds, juce::Justification::centred);

    // MIDI note number
    if (currentPitch.isPitched)
    {
        g.setColour(juce::Colours::lightgrey);
        g.setFont(juce::Font(14.0f));
        g.drawText("MIDI: " + juce::String(currentPitch.midiNote),
                  bounds.reduced(5).removeFromBottom(40).removeFromTop(20),
                  juce::Justification::centred);
    }

    // Label
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(12.0f));
    g.drawText("Frequency", bounds.getX(), bounds.getBottom() - 20, bounds.getWidth(), 20, juce::Justification::centred);
}

void PitchDisplay::drawPitchMeter(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    auto meterBounds = bounds.reduced(20, 15);

    // Draw scale markers
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));

    int centerX = meterBounds.getCentreX();
    int meterWidth = meterBounds.getWidth();

    // Draw tick marks
    for (int cents = -50; cents <= 50; cents += 10)
    {
        float normalized = (cents + 50.0f) / 100.0f;
        int x = meterBounds.getX() + (int)(normalized * meterWidth);
        int tickHeight = (cents == 0) ? 15 : ((cents % 25 == 0) ? 10 : 5);

        g.setColour(cents == 0 ? juce::Colours::white : juce::Colours::grey);
        g.drawVerticalLine(x, (float)(meterBounds.getY() + 5), (float)(meterBounds.getY() + 5 + tickHeight));

        if (cents % 25 == 0)
        {
            g.drawText(juce::String(cents), x - 15, meterBounds.getBottom() - 15, 30, 12,
                      juce::Justification::centred);
        }
    }

    // Draw meter bar background
    auto barBounds = meterBounds.reduced(0, 20).withHeight(15);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRoundedRectangle(barBounds.toFloat(), 3.0f);

    // Draw indicator
    if (currentPitch.isPitched)
    {
        float cents = juce::jlimit(-50.0f, 50.0f, smoothedCents);
        float normalized = (cents + 50.0f) / 100.0f;
        int indicatorX = barBounds.getX() + (int)(normalized * barBounds.getWidth());

        // Color based on accuracy (green when close to 0)
        juce::Colour indicatorColor;
        float absCents = std::abs(cents);
        if (absCents < 5.0f)
            indicatorColor = juce::Colour(0xff00ff00);  // Green
        else if (absCents < 15.0f)
            indicatorColor = juce::Colour(0xffffff00);  // Yellow
        else
            indicatorColor = juce::Colour(0xffff6600);  // Orange

        // Draw indicator triangle
        juce::Path indicator;
        indicator.addTriangle((float)indicatorX, (float)barBounds.getY() - 5,
                             (float)indicatorX - 8, (float)barBounds.getY() - 15,
                             (float)indicatorX + 8, (float)barBounds.getY() - 15);
        g.setColour(indicatorColor);
        g.fillPath(indicator);

        // Draw vertical line
        g.drawVerticalLine(indicatorX, (float)barBounds.getY(), (float)barBounds.getBottom());
    }

    // Label
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));
    g.drawText("Cents", bounds.getX() + 5, bounds.getY() + 2, 40, 12, juce::Justification::centredLeft);
}

void PitchDisplay::drawConfidenceMeter(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    auto workingBounds = bounds;

    // Label
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText("Confidence:", workingBounds.removeFromLeft(80), juce::Justification::centredLeft);

    // Bar background
    auto barBounds = workingBounds.reduced(5, 8);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRoundedRectangle(barBounds.toFloat(), 3.0f);

    // Filled portion
    if (smoothedConfidence > 0.0f)
    {
        auto filledBounds = barBounds.withWidth((int)(barBounds.getWidth() * smoothedConfidence));

        // Color gradient based on confidence
        juce::Colour barColor;
        if (smoothedConfidence > 0.7f)
            barColor = juce::Colour(0xff00cc00);  // Green
        else if (smoothedConfidence > 0.4f)
            barColor = juce::Colour(0xffcccc00);  // Yellow
        else
            barColor = juce::Colour(0xffcc6600);  // Orange

        g.setColour(barColor);
        g.fillRoundedRectangle(filledBounds.toFloat(), 3.0f);
    }

    // Percentage text
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(10.0f));
    g.drawText(juce::String((int)(smoothedConfidence * 100)) + "%",
              barBounds, juce::Justification::centred);
}

void PitchDisplay::drawPitchHistory(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    auto graphBounds = bounds.reduced(10, 5);

    // Draw frequency range labels
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(9.0f));

    float minFreq = 50.0f;
    float maxFreq = 2000.0f;

    g.drawText("2kHz", graphBounds.getX(), graphBounds.getY(), 30, 12, juce::Justification::centredLeft);
    g.drawText("50Hz", graphBounds.getX(), graphBounds.getBottom() - 12, 30, 12, juce::Justification::centredLeft);

    graphBounds.removeFromLeft(35);

    // Draw grid lines
    g.setColour(juce::Colour(0xff3a3a3a));
    for (float freq : { 100.0f, 200.0f, 440.0f, 1000.0f })
    {
        float normalized = std::log2(freq / minFreq) / std::log2(maxFreq / minFreq);
        int y = graphBounds.getBottom() - (int)(normalized * graphBounds.getHeight());
        g.drawHorizontalLine(y, (float)graphBounds.getX(), (float)graphBounds.getRight());
    }

    // Draw pitch history
    if (pitchHistory.size() > 1)
    {
        juce::Path historyPath;
        bool pathStarted = false;

        for (size_t i = 0; i < pitchHistory.size(); ++i)
        {
            float freq = pitchHistory[i];
            if (freq > 0.0f)
            {
                float x = graphBounds.getX() + ((float)i / (float)pitchHistory.size()) * graphBounds.getWidth();
                float normalized = std::log2(freq / minFreq) / std::log2(maxFreq / minFreq);
                normalized = juce::jlimit(0.0f, 1.0f, normalized);
                float y = graphBounds.getBottom() - normalized * graphBounds.getHeight();

                if (!pathStarted)
                {
                    historyPath.startNewSubPath(x, y);
                    pathStarted = true;
                }
                else
                {
                    historyPath.lineTo(x, y);
                }
            }
            else if (pathStarted)
            {
                // Gap in pitch detection
                pathStarted = false;
            }
        }

        g.setColour(juce::Colour(0xff4a9eff));
        g.strokePath(historyPath, juce::PathStrokeType(2.0f));
    }

    // Label
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));
    g.drawText("Pitch History", bounds.getX() + 10, bounds.getY() + 2, 80, 12, juce::Justification::centredLeft);
}

//==============================================================================
juce::Colour PitchDisplay::getNoteColor(int midiNote) const
{
    if (midiNote < 0)
        return juce::Colours::grey;

    // Color wheel based on pitch class (C=red, G=green, D=blue, etc.)
    int pitchClass = midiNote % 12;

    static const juce::Colour noteColors[] = {
        juce::Colour(0xffff4444),  // C  - Red
        juce::Colour(0xffff6644),  // C# - Red-Orange
        juce::Colour(0xffff8844),  // D  - Orange
        juce::Colour(0xffffaa44),  // D# - Orange-Yellow
        juce::Colour(0xffffcc44),  // E  - Yellow
        juce::Colour(0xff88ff44),  // F  - Yellow-Green
        juce::Colour(0xff44ff44),  // F# - Green
        juce::Colour(0xff44ffaa),  // G  - Green-Cyan
        juce::Colour(0xff44ffff),  // G# - Cyan
        juce::Colour(0xff44aaff),  // A  - Cyan-Blue
        juce::Colour(0xff4488ff),  // A# - Blue
        juce::Colour(0xff8844ff),  // B  - Blue-Purple
    };

    return noteColors[pitchClass];
}

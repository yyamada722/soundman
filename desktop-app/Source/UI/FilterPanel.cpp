/*
  ==============================================================================

    FilterPanel.cpp

    Filter and EQ control panel implementation

  ==============================================================================
*/

#include "FilterPanel.h"
#include <cmath>

//==============================================================================
FilterPanel::FilterPanel()
{
    setupFilterControls();
    setupEQControls();
    startTimerHz(30);
}

FilterPanel::~FilterPanel()
{
    stopTimer();
}

//==============================================================================
void FilterPanel::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    filter.prepare(sampleRate, samplesPerBlock, 2);
    eq.prepare(sampleRate, samplesPerBlock, 2);
}

//==============================================================================
void FilterPanel::setupFilterControls()
{
    // Filter enable button
    addAndMakeVisible(filterEnableButton);
    filterEnableButton.setToggleState(true, juce::dontSendNotification);
    filterEnableButton.addListener(this);

    // Filter type combo
    addAndMakeVisible(filterTypeCombo);
    filterTypeCombo.addItem("Lowpass", 1);
    filterTypeCombo.addItem("Highpass", 2);
    filterTypeCombo.addItem("Bandpass", 3);
    filterTypeCombo.addItem("Notch", 4);
    filterTypeCombo.setSelectedId(1);
    filterTypeCombo.addListener(this);

    // Frequency slider
    addAndMakeVisible(filterFreqSlider);
    filterFreqSlider.setRange(20.0, 20000.0, 1.0);
    filterFreqSlider.setSkewFactorFromMidPoint(1000.0);
    filterFreqSlider.setValue(1000.0);
    filterFreqSlider.setTextValueSuffix(" Hz");
    filterFreqSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    filterFreqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    filterFreqSlider.addListener(this);

    addAndMakeVisible(filterFreqLabel);
    filterFreqLabel.setJustificationType(juce::Justification::centred);

    // Q slider
    addAndMakeVisible(filterQSlider);
    filterQSlider.setRange(0.1, 10.0, 0.01);
    filterQSlider.setValue(0.707);
    filterQSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    filterQSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    filterQSlider.addListener(this);

    addAndMakeVisible(filterQLabel);
    filterQLabel.setJustificationType(juce::Justification::centred);
}

void FilterPanel::setupEQControls()
{
    // EQ enable button
    addAndMakeVisible(eqEnableButton);
    eqEnableButton.setToggleState(true, juce::dontSendNotification);
    eqEnableButton.addListener(this);

    // Band names
    juce::StringArray bandNames = { "Low", "Mid", "High" };
    float defaultFreqs[] = { 100.0f, 1000.0f, 8000.0f };

    for (int i = 0; i < 3; ++i)
    {
        auto& band = bandControls[i];

        // Enable button
        addAndMakeVisible(band.enableButton);
        band.enableButton.setButtonText(bandNames[i]);
        band.enableButton.setToggleState(true, juce::dontSendNotification);
        band.enableButton.addListener(this);

        // Frequency slider
        addAndMakeVisible(band.freqSlider);
        band.freqSlider.setRange(20.0, 20000.0, 1.0);
        band.freqSlider.setSkewFactorFromMidPoint(1000.0);
        band.freqSlider.setValue(defaultFreqs[i]);
        band.freqSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        band.freqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
        band.freqSlider.setTextValueSuffix(" Hz");
        band.freqSlider.addListener(this);

        addAndMakeVisible(band.freqLabel);
        band.freqLabel.setText("Freq", juce::dontSendNotification);
        band.freqLabel.setJustificationType(juce::Justification::centred);
        band.freqLabel.setFont(juce::Font(10.0f));

        // Gain slider
        addAndMakeVisible(band.gainSlider);
        band.gainSlider.setRange(-12.0, 12.0, 0.1);
        band.gainSlider.setValue(0.0);
        band.gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        band.gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
        band.gainSlider.setTextValueSuffix(" dB");
        band.gainSlider.addListener(this);

        addAndMakeVisible(band.gainLabel);
        band.gainLabel.setText("Gain", juce::dontSendNotification);
        band.gainLabel.setJustificationType(juce::Justification::centred);
        band.gainLabel.setFont(juce::Font(10.0f));

        // Q slider
        addAndMakeVisible(band.qSlider);
        band.qSlider.setRange(0.1, 10.0, 0.01);
        band.qSlider.setValue(1.0);
        band.qSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        band.qSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 16);
        band.qSlider.addListener(this);

        addAndMakeVisible(band.qLabel);
        band.qLabel.setText("Q", juce::dontSendNotification);
        band.qLabel.setJustificationType(juce::Justification::centred);
        band.qLabel.setFont(juce::Font(10.0f));
    }
}

//==============================================================================
void FilterPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRect(getLocalBounds(), 1);

    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("Filter & EQ", 10, 5, 150, 20, juce::Justification::centredLeft);

    // Draw frequency response
    if (!responseArea.isEmpty())
    {
        drawFrequencyResponse(g, responseArea);
    }

    // Section labels
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText("Filter", 10, 195, 60, 18, juce::Justification::centredLeft);
    g.drawText("3-Band EQ", 10, 315, 80, 18, juce::Justification::centredLeft);
}

void FilterPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(25);  // Title

    // Frequency response area
    responseArea = bounds.removeFromTop(160);

    bounds.removeFromTop(10);

    // Filter controls
    auto filterArea = bounds.removeFromTop(100);
    filterArea.removeFromTop(20);  // Label space

    auto filterRow = filterArea.removeFromTop(80);

    filterEnableButton.setBounds(filterRow.removeFromLeft(60));
    filterRow.removeFromLeft(10);

    filterTypeCombo.setBounds(filterRow.removeFromLeft(90).reduced(0, 25));
    filterRow.removeFromLeft(10);

    auto freqArea = filterRow.removeFromLeft(80);
    filterFreqLabel.setBounds(freqArea.removeFromTop(14));
    filterFreqSlider.setBounds(freqArea);

    filterRow.removeFromLeft(5);

    auto qArea = filterRow.removeFromLeft(70);
    filterQLabel.setBounds(qArea.removeFromTop(14));
    filterQSlider.setBounds(qArea);

    bounds.removeFromTop(15);

    // EQ controls
    auto eqArea = bounds;
    eqArea.removeFromTop(20);  // Label space

    auto eqRow = eqArea.removeFromTop(25);
    eqEnableButton.setBounds(eqRow.removeFromLeft(50));

    eqArea.removeFromTop(5);

    // EQ bands
    int bandWidth = (eqArea.getWidth() - 20) / 3;

    for (int i = 0; i < 3; ++i)
    {
        auto& band = bandControls[i];
        auto bandArea = eqArea.removeFromLeft(bandWidth);
        if (i < 2) eqArea.removeFromLeft(10);

        band.enableButton.setBounds(bandArea.removeFromTop(22));

        auto knobRow1 = bandArea.removeFromTop(55);
        auto freqKnob = knobRow1.removeFromLeft(knobRow1.getWidth() / 2);
        band.freqLabel.setBounds(freqKnob.removeFromTop(12));
        band.freqSlider.setBounds(freqKnob);

        band.gainLabel.setBounds(knobRow1.removeFromTop(12));
        band.gainSlider.setBounds(knobRow1);

        bandArea.removeFromTop(5);

        auto qKnobArea = bandArea.removeFromTop(55).withWidth(60);
        band.qLabel.setBounds(qKnobArea.removeFromTop(12));
        band.qSlider.setBounds(qKnobArea);
    }
}

//==============================================================================
void FilterPanel::sliderValueChanged(juce::Slider* slider)
{
    // Filter controls
    if (slider == &filterFreqSlider || slider == &filterQSlider)
    {
        updateFilterFromControls();
    }

    // EQ controls
    for (int i = 0; i < 3; ++i)
    {
        auto& band = bandControls[i];
        if (slider == &band.freqSlider || slider == &band.gainSlider || slider == &band.qSlider)
        {
            updateEQFromControls();
            break;
        }
    }

    if (filterChangedCallback)
        filterChangedCallback();
}

void FilterPanel::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &filterTypeCombo)
    {
        updateFilterFromControls();

        if (filterChangedCallback)
            filterChangedCallback();
    }
}

void FilterPanel::buttonClicked(juce::Button* button)
{
    if (button == &filterEnableButton)
    {
        filter.setEnabled(filterEnableButton.getToggleState());
    }
    else if (button == &eqEnableButton)
    {
        eq.setEnabled(eqEnableButton.getToggleState());
    }
    else
    {
        for (int i = 0; i < 3; ++i)
        {
            if (button == &bandControls[i].enableButton)
            {
                eq.setBandEnabled(i, bandControls[i].enableButton.getToggleState());
                break;
            }
        }
    }

    if (filterChangedCallback)
        filterChangedCallback();

    repaint();
}

void FilterPanel::timerCallback()
{
    repaint();  // Update frequency response display
}

//==============================================================================
void FilterPanel::updateFilterFromControls()
{
    // Set filter type
    int typeId = filterTypeCombo.getSelectedId();
    AudioFilter::FilterType type = AudioFilter::FilterType::Lowpass;

    switch (typeId)
    {
        case 1: type = AudioFilter::FilterType::Lowpass; break;
        case 2: type = AudioFilter::FilterType::Highpass; break;
        case 3: type = AudioFilter::FilterType::Bandpass; break;
        case 4: type = AudioFilter::FilterType::Notch; break;
    }

    filter.setFilterType(type);
    filter.setFrequency((float)filterFreqSlider.getValue());
    filter.setQ((float)filterQSlider.getValue());
}

void FilterPanel::updateEQFromControls()
{
    for (int i = 0; i < 3; ++i)
    {
        auto& band = bandControls[i];
        eq.setBand(i,
                  (float)band.freqSlider.getValue(),
                  (float)band.gainSlider.getValue(),
                  (float)band.qSlider.getValue());
    }
}

//==============================================================================
void FilterPanel::drawFrequencyResponse(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    auto chartBounds = bounds.reduced(30, 20);
    chartBounds.removeFromBottom(20);  // Space for frequency labels

    // Draw grid
    drawGrid(g, chartBounds);

    // Draw combined frequency response
    juce::Path responsePath;
    bool pathStarted = false;

    const int numPoints = chartBounds.getWidth();
    for (int i = 0; i < numPoints; ++i)
    {
        float x = (float)i;
        float freq = 20.0f * std::pow(1000.0f, x / (float)numPoints);

        float magnitude = 1.0f;

        // Apply filter magnitude
        if (filter.isEnabled())
        {
            magnitude *= filter.getMagnitudeForFrequency(freq);
        }

        // Apply EQ magnitude
        if (eq.isEnabled())
        {
            magnitude *= eq.getMagnitudeForFrequency(freq);
        }

        float gainDb = juce::Decibels::gainToDecibels(magnitude);
        gainDb = juce::jlimit(-24.0f, 24.0f, gainDb);

        float y = getYForGain(gainDb, (float)chartBounds.getHeight());

        if (!pathStarted)
        {
            responsePath.startNewSubPath(chartBounds.getX() + x, chartBounds.getY() + y);
            pathStarted = true;
        }
        else
        {
            responsePath.lineTo(chartBounds.getX() + x, chartBounds.getY() + y);
        }
    }

    // Draw response curve
    g.setColour(juce::Colour(0xff4a9eff));
    g.strokePath(responsePath, juce::PathStrokeType(2.0f));

    // Fill under curve
    responsePath.lineTo((float)chartBounds.getRight(), (float)chartBounds.getCentreY());
    responsePath.lineTo((float)chartBounds.getX(), (float)chartBounds.getCentreY());
    responsePath.closeSubPath();

    g.setColour(juce::Colour(0xff4a9eff).withAlpha(0.1f));
    g.fillPath(responsePath);

    // Draw frequency labels
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(9.0f));

    std::vector<std::pair<float, juce::String>> freqLabels = {
        { 50.0f, "50" },
        { 100.0f, "100" },
        { 500.0f, "500" },
        { 1000.0f, "1k" },
        { 5000.0f, "5k" },
        { 10000.0f, "10k" }
    };

    for (const auto& [freq, label] : freqLabels)
    {
        float x = getXForFrequency(freq, (float)chartBounds.getWidth());
        g.drawText(label, chartBounds.getX() + (int)x - 15, chartBounds.getBottom() + 3, 30, 14,
                  juce::Justification::centred);
    }

    // Draw gain labels
    for (float db : { -12.0f, 0.0f, 12.0f })
    {
        float y = getYForGain(db, (float)chartBounds.getHeight());
        g.drawText(juce::String((int)db) + "dB", bounds.getX() + 2, chartBounds.getY() + (int)y - 7, 25, 14,
                  juce::Justification::centredRight);
    }
}

void FilterPanel::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colour(0xff3a3a3a));

    // Frequency grid lines
    std::vector<float> freqs = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f };
    for (float freq : freqs)
    {
        float x = getXForFrequency(freq, (float)bounds.getWidth());
        g.drawVerticalLine(bounds.getX() + (int)x, (float)bounds.getY(), (float)bounds.getBottom());
    }

    // Gain grid lines
    for (float db : { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f })
    {
        float y = getYForGain(db, (float)bounds.getHeight());
        g.setColour(db == 0.0f ? juce::Colour(0xff4a4a4a) : juce::Colour(0xff3a3a3a));
        g.drawHorizontalLine(bounds.getY() + (int)y, (float)bounds.getX(), (float)bounds.getRight());
    }
}

float FilterPanel::getXForFrequency(float freq, float width) const
{
    // Logarithmic scale: 20Hz to 20kHz
    float logMin = std::log10(20.0f);
    float logMax = std::log10(20000.0f);
    float logFreq = std::log10(juce::jlimit(20.0f, 20000.0f, freq));
    return (logFreq - logMin) / (logMax - logMin) * width;
}

float FilterPanel::getYForGain(float gainDb, float height) const
{
    // -24dB to +24dB
    float normalized = (24.0f - gainDb) / 48.0f;
    return juce::jlimit(0.0f, 1.0f, normalized) * height;
}

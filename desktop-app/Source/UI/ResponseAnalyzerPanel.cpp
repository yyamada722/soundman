/*
  ==============================================================================

    ResponseAnalyzerPanel.cpp

    Impulse Response and Frequency Response measurement display implementation

  ==============================================================================
*/

#include "ResponseAnalyzerPanel.h"

//==============================================================================
ResponseAnalyzerPanel::ResponseAnalyzerPanel()
{
    setupControls();

    analyzer.onMeasurementComplete = [this]()
    {
        measureButton.setButtonText("Start Measurement");
        auto result = analyzer.getResult();
        if (result.isValid)
        {
            rt60Label.setText("RT60: " + juce::String(result.rt60, 2) + " s", juce::dontSendNotification);
            peakLabel.setText("Peak: " + juce::String(result.peakLevel, 1) + " dB", juce::dontSendNotification);
        }
        repaint();
    };

    startTimerHz(30);
}

ResponseAnalyzerPanel::~ResponseAnalyzerPanel()
{
    stopTimer();
}

//==============================================================================
void ResponseAnalyzerPanel::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    analyzer.prepare(sampleRate, samplesPerBlock);
}

//==============================================================================
void ResponseAnalyzerPanel::setupControls()
{
    // Measure button
    measureButton.addListener(this);
    addAndMakeVisible(measureButton);

    // Display mode combo
    displayModeCombo.addItem("Both", 1);
    displayModeCombo.addItem("Impulse Response", 2);
    displayModeCombo.addItem("Frequency Response", 3);
    displayModeCombo.setSelectedId(1);
    displayModeCombo.onChange = [this]()
    {
        int id = displayModeCombo.getSelectedId();
        switch (id)
        {
            case 1: displayMode = DisplayMode::Both; break;
            case 2: displayMode = DisplayMode::ImpulseOnly; break;
            case 3: displayMode = DisplayMode::FrequencyOnly; break;
        }
        repaint();
    };
    addAndMakeVisible(displayModeCombo);

    // Duration slider
    durationSlider.setRange(1, 10, 0.5);
    durationSlider.setValue(3);
    durationSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    durationSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    durationSlider.addListener(this);
    addAndMakeVisible(durationSlider);

    durationLabel.setFont(juce::Font(11.0f));
    durationLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(durationLabel);

    durationValueLabel.setFont(juce::Font(11.0f));
    durationValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    durationValueLabel.setText("3.0 s", juce::dontSendNotification);
    addAndMakeVisible(durationValueLabel);

    // Status label
    statusLabel.setFont(juce::Font(12.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    statusLabel.setText("Ready", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    // RT60 label
    rt60Label.setFont(juce::Font(12.0f));
    rt60Label.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    rt60Label.setText("RT60: ---", juce::dontSendNotification);
    addAndMakeVisible(rt60Label);

    // Peak label
    peakLabel.setFont(juce::Font(12.0f));
    peakLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    peakLabel.setText("Peak: ---", juce::dontSendNotification);
    addAndMakeVisible(peakLabel);

    // Progress bar
    progressBar.setColour(juce::ProgressBar::foregroundColourId, juce::Colour(0xff4a9eff));
    addAndMakeVisible(progressBar);
}

//==============================================================================
void ResponseAnalyzerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(75);  // Controls area

    auto result = analyzer.getResult();

    if (displayMode == DisplayMode::Both)
    {
        auto topHalf = bounds.removeFromTop(bounds.getHeight() / 2 - 5);
        bounds.removeFromTop(10);
        auto bottomHalf = bounds;

        drawImpulseResponse(g, topHalf);
        drawFrequencyResponse(g, bottomHalf);
    }
    else if (displayMode == DisplayMode::ImpulseOnly)
    {
        drawImpulseResponse(g, bounds);
    }
    else
    {
        drawFrequencyResponse(g, bounds);
    }
}

void ResponseAnalyzerPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    auto controlsArea = bounds.removeFromTop(70);

    // First row
    auto row = controlsArea.removeFromTop(25);
    measureButton.setBounds(row.removeFromLeft(130));
    row.removeFromLeft(10);
    displayModeCombo.setBounds(row.removeFromLeft(150));
    row.removeFromLeft(20);
    statusLabel.setBounds(row.removeFromLeft(100));
    row.removeFromLeft(10);
    rt60Label.setBounds(row.removeFromLeft(100));
    row.removeFromLeft(10);
    peakLabel.setBounds(row);

    controlsArea.removeFromTop(10);

    // Second row
    row = controlsArea.removeFromTop(25);
    durationLabel.setBounds(row.removeFromLeft(80));
    durationValueLabel.setBounds(row.removeFromRight(50));
    row.removeFromRight(5);
    durationSlider.setBounds(row.removeFromLeft(150));
    row.removeFromLeft(20);
    progressBar.setBounds(row);
}

//==============================================================================
void ResponseAnalyzerPanel::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &durationSlider)
    {
        float duration = (float)durationSlider.getValue();
        analyzer.setSweepDuration(duration);
        durationValueLabel.setText(juce::String(duration, 1) + " s", juce::dontSendNotification);
    }
}

void ResponseAnalyzerPanel::buttonClicked(juce::Button* button)
{
    if (button == &measureButton)
    {
        auto state = analyzer.getState();

        if (state == ImpulseResponseAnalyzer::MeasurementState::Idle ||
            state == ImpulseResponseAnalyzer::MeasurementState::Complete)
        {
            analyzer.startMeasurement();
            measureButton.setButtonText("Stop");
            statusLabel.setText("Measuring...", juce::dontSendNotification);
            rt60Label.setText("RT60: ---", juce::dontSendNotification);
            peakLabel.setText("Peak: ---", juce::dontSendNotification);
        }
        else
        {
            analyzer.stopMeasurement();
            measureButton.setButtonText("Start Measurement");
            statusLabel.setText("Stopped", juce::dontSendNotification);
        }
    }
}

void ResponseAnalyzerPanel::timerCallback()
{
    auto state = analyzer.getState();
    progress = analyzer.getProgress();

    switch (state)
    {
        case ImpulseResponseAnalyzer::MeasurementState::Idle:
            statusLabel.setText("Ready", juce::dontSendNotification);
            break;
        case ImpulseResponseAnalyzer::MeasurementState::GeneratingSweep:
            statusLabel.setText("Measuring...", juce::dontSendNotification);
            break;
        case ImpulseResponseAnalyzer::MeasurementState::Processing:
            statusLabel.setText("Processing...", juce::dontSendNotification);
            break;
        case ImpulseResponseAnalyzer::MeasurementState::Complete:
            statusLabel.setText("Complete", juce::dontSendNotification);
            break;
    }

    if (state == ImpulseResponseAnalyzer::MeasurementState::GeneratingSweep)
        repaint();
}

//==============================================================================
void ResponseAnalyzerPanel::drawImpulseResponse(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText("Impulse Response", bounds.getX() + 5, bounds.getY() + 5, 120, 15, juce::Justification::centredLeft);

    auto graphBounds = bounds.reduced(10, 25);
    graphBounds.removeFromTop(5);

    drawGrid(g, graphBounds, false);

    auto result = analyzer.getResult();
    if (!result.isValid || result.impulseResponse.empty())
        return;

    // Draw impulse response
    juce::Path irPath;
    int numSamples = (int)result.impulseResponse.size();
    int maxSamplesToShow = std::min(numSamples, (int)(currentSampleRate * 0.5));  // Show max 500ms

    for (int i = 0; i < maxSamplesToShow; ++i)
    {
        float x = graphBounds.getX() + ((float)i / maxSamplesToShow) * graphBounds.getWidth();
        float y = graphBounds.getCentreY() - result.impulseResponse[i] * graphBounds.getHeight() * 0.45f;

        if (i == 0)
            irPath.startNewSubPath(x, y);
        else
            irPath.lineTo(x, y);
    }

    g.setColour(juce::Colour(0xff4a9eff));
    g.strokePath(irPath, juce::PathStrokeType(1.5f));

    // Time axis label
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(9.0f));
    g.drawText("0 ms", graphBounds.getX(), graphBounds.getBottom() + 2, 30, 12, juce::Justification::centredLeft);
    g.drawText("500 ms", graphBounds.getRight() - 40, graphBounds.getBottom() + 2, 40, 12, juce::Justification::centredRight);
}

void ResponseAnalyzerPanel::drawFrequencyResponse(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Title
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText("Frequency Response", bounds.getX() + 5, bounds.getY() + 5, 130, 15, juce::Justification::centredLeft);

    auto graphBounds = bounds.reduced(10, 25);
    graphBounds.removeFromTop(5);

    drawGrid(g, graphBounds, true);

    auto result = analyzer.getResult();
    if (!result.isValid || result.frequencyMagnitude.empty())
        return;

    // Draw frequency response
    juce::Path frPath;
    int numBins = (int)result.frequencyMagnitude.size();

    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    float minDb = -60.0f;
    float maxDb = 20.0f;

    bool pathStarted = false;

    for (int i = 1; i < numBins; ++i)
    {
        float freq = result.frequencyAxis[i];
        if (freq < minFreq || freq > maxFreq)
            continue;

        float magnitude = juce::jlimit(minDb, maxDb, result.frequencyMagnitude[i]);

        // Logarithmic frequency scale
        float x = graphBounds.getX() + std::log10(freq / minFreq) / std::log10(maxFreq / minFreq) * graphBounds.getWidth();
        float y = graphBounds.getY() + (1.0f - (magnitude - minDb) / (maxDb - minDb)) * graphBounds.getHeight();

        if (!pathStarted)
        {
            frPath.startNewSubPath(x, y);
            pathStarted = true;
        }
        else
        {
            frPath.lineTo(x, y);
        }
    }

    g.setColour(juce::Colour(0xff00cc66));
    g.strokePath(frPath, juce::PathStrokeType(1.5f));

    // Frequency axis labels
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(9.0f));

    for (float freq : { 100.0f, 1000.0f, 10000.0f })
    {
        float x = graphBounds.getX() + std::log10(freq / minFreq) / std::log10(maxFreq / minFreq) * graphBounds.getWidth();
        juce::String label = (freq >= 1000) ? juce::String((int)(freq / 1000)) + "k" : juce::String((int)freq);
        g.drawText(label, (int)x - 15, graphBounds.getBottom() + 2, 30, 12, juce::Justification::centred);
    }
}

void ResponseAnalyzerPanel::drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds, bool isFrequency)
{
    g.setColour(juce::Colour(0xff3a3a3a));

    if (isFrequency)
    {
        // Vertical grid lines at decade intervals
        float minFreq = 20.0f;
        float maxFreq = 20000.0f;

        for (float freq : { 100.0f, 1000.0f, 10000.0f })
        {
            float x = bounds.getX() + std::log10(freq / minFreq) / std::log10(maxFreq / minFreq) * bounds.getWidth();
            g.drawVerticalLine((int)x, (float)bounds.getY(), (float)bounds.getBottom());
        }

        // Horizontal grid lines
        for (int db = -60; db <= 20; db += 20)
        {
            float y = bounds.getY() + (1.0f - (db + 60.0f) / 80.0f) * bounds.getHeight();
            g.drawHorizontalLine((int)y, (float)bounds.getX(), (float)bounds.getRight());

            g.setColour(juce::Colours::grey);
            g.setFont(juce::Font(8.0f));
            g.drawText(juce::String(db) + "dB", bounds.getX() - 35, (int)y - 6, 30, 12, juce::Justification::centredRight);
            g.setColour(juce::Colour(0xff3a3a3a));
        }
    }
    else
    {
        // Center line for IR
        g.drawHorizontalLine(bounds.getCentreY(), (float)bounds.getX(), (float)bounds.getRight());

        // Vertical time divisions
        for (int i = 1; i < 5; ++i)
        {
            float x = bounds.getX() + (float)i / 5.0f * bounds.getWidth();
            g.drawVerticalLine((int)x, (float)bounds.getY(), (float)bounds.getBottom());
        }
    }
}

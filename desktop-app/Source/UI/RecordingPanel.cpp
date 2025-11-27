/*
  ==============================================================================

    RecordingPanel.cpp

    Real-time audio recording panel implementation

  ==============================================================================
*/

#include "RecordingPanel.h"

//==============================================================================
RecordingPanel::RecordingPanel()
{
    // Title
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Recording", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType(juce::Justification::centred);

    // Device info
    addAndMakeVisible(deviceLabel);
    deviceLabel.setText("Input Device:", juce::dontSendNotification);
    deviceLabel.setFont(juce::Font(12.0f));
    deviceLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible(deviceNameLabel);
    deviceNameLabel.setText("No device", juce::dontSendNotification);
    deviceNameLabel.setFont(juce::Font(12.0f));
    deviceNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    // File name
    addAndMakeVisible(fileNameLabel);
    fileNameLabel.setText("Ready to record", juce::dontSendNotification);
    fileNameLabel.setFont(juce::Font(11.0f));
    fileNameLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    fileNameLabel.setJustificationType(juce::Justification::centred);

    // Duration
    addAndMakeVisible(durationLabel);
    durationLabel.setText("00:00.0", juce::dontSendNotification);
    durationLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    durationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    durationLabel.setJustificationType(juce::Justification::centred);

    // Buttons
    addAndMakeVisible(recordButton);
    recordButton.setButtonText("Record");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffcc3333));
    recordButton.onClick = [this]()
    {
        if (recordCallback)
            recordCallback();
    };

    addAndMakeVisible(stopButton);
    stopButton.setButtonText("Stop");
    stopButton.setEnabled(false);
    stopButton.onClick = [this]()
    {
        if (stopCallback)
            stopCallback();
    };

    addAndMakeVisible(pauseButton);
    pauseButton.setButtonText("Pause");
    pauseButton.setEnabled(false);
    pauseButton.onClick = [this]()
    {
        if (pauseCallback)
            pauseCallback();
    };

    // Start timer for level meter updates
    startTimer(33);  // ~30 fps
}

RecordingPanel::~RecordingPanel()
{
    stopTimer();
}

//==============================================================================
void RecordingPanel::setRecordingState(RecordingState state)
{
    recordingState = state;

    switch (state)
    {
        case RecordingState::Stopped:
            recordButton.setEnabled(true);
            stopButton.setEnabled(false);
            pauseButton.setEnabled(false);
            pauseButton.setButtonText("Pause");
            durationLabel.setText("00:00.0", juce::dontSendNotification);
            break;

        case RecordingState::Recording:
            recordButton.setEnabled(false);
            stopButton.setEnabled(true);
            pauseButton.setEnabled(true);
            pauseButton.setButtonText("Pause");
            break;

        case RecordingState::Paused:
            recordButton.setEnabled(false);
            stopButton.setEnabled(true);
            pauseButton.setEnabled(true);
            pauseButton.setButtonText("Resume");
            break;
    }

    repaint();
}

void RecordingPanel::setInputLevels(float lRMS, float lPeak, float rRMS, float rPeak)
{
    leftRMS = lRMS;
    leftPeak = lPeak;
    rightRMS = rRMS;
    rightPeak = rPeak;

    // Update peak hold
    if (lPeak > leftPeakHold)
    {
        leftPeakHold = lPeak;
        peakHoldCounter = PEAK_HOLD_FRAMES;
    }

    if (rPeak > rightPeakHold)
    {
        rightPeakHold = rPeak;
        peakHoldCounter = PEAK_HOLD_FRAMES;
    }

    repaint();
}

void RecordingPanel::setRecordingDuration(double seconds)
{
    int minutes = (int)(seconds / 60.0);
    double secs = seconds - minutes * 60.0;

    juce::String timeStr = juce::String::formatted("%02d:%04.1f", minutes, secs);
    durationLabel.setText(timeStr, juce::dontSendNotification);
}

void RecordingPanel::setRecordingFileName(const juce::String& fileName)
{
    fileNameLabel.setText(fileName, juce::dontSendNotification);
}

void RecordingPanel::setInputDevice(const juce::String& deviceName)
{
    deviceNameLabel.setText(deviceName, juce::dontSendNotification);
}

//==============================================================================
void RecordingPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);

    // Level meters background
    auto levelBounds = bounds.reduced(10);
    levelBounds.removeFromTop(140);  // Skip top area
    levelBounds.removeFromBottom(110);  // Skip bottom area

    // Draw left channel meter
    auto leftMeterBounds = levelBounds.removeFromLeft((levelBounds.getWidth() - 10) / 2);
    leftMeterBounds.reduce(5, 0);
    drawLevelMeter(g, leftMeterBounds, leftRMS, leftPeak);

    levelBounds.removeFromLeft(10);  // Gap

    // Draw right channel meter
    auto rightMeterBounds = levelBounds;
    rightMeterBounds.reduce(5, 0);
    drawLevelMeter(g, rightMeterBounds, rightRMS, rightPeak);

    // Channel labels
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    auto labelY = leftMeterBounds.getBottom() + 5;
    g.drawText("L", leftMeterBounds.getX(), labelY, leftMeterBounds.getWidth(), 15,
              juce::Justification::centred);
    g.drawText("R", rightMeterBounds.getX(), labelY, rightMeterBounds.getWidth(), 15,
              juce::Justification::centred);
}

void RecordingPanel::drawLevelMeter(juce::Graphics& g, const juce::Rectangle<int>& bounds, float rms, float peak)
{
    // Background
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(bounds);

    // Draw peak hold
    if (peakHoldCounter > 0 && peak > 0.0f)
    {
        float holdHeight = juce::jmap(juce::Decibels::gainToDecibels(peak), -60.0f, 0.0f, 0.0f, (float)bounds.getHeight());
        holdHeight = juce::jlimit(0.0f, (float)bounds.getHeight(), holdHeight);

        int holdY = bounds.getBottom() - (int)holdHeight;

        g.setColour(juce::Colours::yellow);
        g.fillRect(bounds.getX(), holdY - 2, bounds.getWidth(), 3);
    }

    // Draw RMS level
    if (rms > 0.0f)
    {
        float rmsDb = juce::Decibels::gainToDecibels(rms);
        float rmsHeight = juce::jmap(rmsDb, -60.0f, 0.0f, 0.0f, (float)bounds.getHeight());
        rmsHeight = juce::jlimit(0.0f, (float)bounds.getHeight(), rmsHeight);

        auto levelBounds = bounds.withTop(bounds.getBottom() - (int)rmsHeight);

        // Color gradient based on level
        juce::Colour meterColor;
        if (rmsDb > -3.0f)
            meterColor = juce::Colour(0xffcc3333);  // Red
        else if (rmsDb > -10.0f)
            meterColor = juce::Colour(0xffcccc33);  // Yellow
        else
            meterColor = juce::Colour(0xff33cc33);  // Green

        g.setColour(meterColor);
        g.fillRect(levelBounds);
    }

    // Border
    g.setColour(juce::Colour(0xff4a4a4a));
    g.drawRect(bounds, 1);

    // dB scale markers
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.setFont(juce::Font(9.0f));

    for (int db = 0; db >= -60; db -= 10)
    {
        float y = juce::jmap((float)db, -60.0f, 0.0f, (float)bounds.getBottom(), (float)bounds.getY());
        g.drawHorizontalLine((int)y, (float)bounds.getX(), (float)bounds.getRight());

        if (db != 0)
        {
            juce::String label = juce::String(db);
            g.drawText(label, bounds.getRight() + 2, (int)y - 6, 25, 12,
                      juce::Justification::centredLeft);
        }
    }
}

void RecordingPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(5);

    // Device info
    auto deviceRow = bounds.removeFromTop(20);
    deviceLabel.setBounds(deviceRow.removeFromLeft(90));
    deviceNameLabel.setBounds(deviceRow);
    bounds.removeFromTop(10);

    // Duration display
    durationLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(5);

    // File name
    fileNameLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);

    // Level meters (auto-sized by paint)
    bounds.removeFromTop(bounds.getHeight() - 110);  // Leave space for meters
    bounds.removeFromTop(20);  // Channel labels space

    // Buttons at bottom
    auto buttonBounds = bounds.removeFromBottom(30);
    int buttonWidth = (buttonBounds.getWidth() - 20) / 3;

    recordButton.setBounds(buttonBounds.removeFromLeft(buttonWidth));
    buttonBounds.removeFromLeft(10);
    pauseButton.setBounds(buttonBounds.removeFromLeft(buttonWidth));
    buttonBounds.removeFromLeft(10);
    stopButton.setBounds(buttonBounds);
}

void RecordingPanel::timerCallback()
{
    // Decay peak hold
    if (peakHoldCounter > 0)
    {
        peakHoldCounter--;

        if (peakHoldCounter == 0)
        {
            leftPeakHold = 0.0f;
            rightPeakHold = 0.0f;
        }
    }

    repaint();
}

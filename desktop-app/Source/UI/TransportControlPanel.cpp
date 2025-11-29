/*
  ==============================================================================

    TransportControlPanel.cpp

    Transport controls implementation

  ==============================================================================
*/

#include "TransportControlPanel.h"

//==============================================================================
// TimeInputGroup implementation

TransportControlPanel::TimeInputGroup::TimeInputGroup()
{
    setupEditor(hoursInput, 2, "h");
    setupEditor(minutesInput, 2, "m");
    setupEditor(secondsInput, 2, "s");
    setupEditor(msInput, 3, "ms");

    addAndMakeVisible(hoursInput);
    addAndMakeVisible(minutesInput);
    addAndMakeVisible(secondsInput);
    addAndMakeVisible(msInput);

    // Colons and dot
    addAndMakeVisible(colonLabel1);
    addAndMakeVisible(colonLabel2);
    addAndMakeVisible(dotLabel);

    colonLabel1.setFont(juce::Font(14.0f, juce::Font::bold));
    colonLabel2.setFont(juce::Font(14.0f, juce::Font::bold));
    dotLabel.setFont(juce::Font(14.0f, juce::Font::bold));

    colonLabel1.setColour(juce::Label::textColourId, juce::Colours::grey);
    colonLabel2.setColour(juce::Label::textColourId, juce::Colours::grey);
    dotLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    colonLabel1.setJustificationType(juce::Justification::centred);
    colonLabel2.setJustificationType(juce::Justification::centred);
    dotLabel.setJustificationType(juce::Justification::centred);
}

void TransportControlPanel::TimeInputGroup::setupEditor(juce::TextEditor& editor, int maxChars, const juce::String& suffix)
{
    editor.setFont(juce::Font(12.0f));
    editor.setJustification(juce::Justification::centred);
    editor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a2a));
    editor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    editor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff4a4a4a));
    editor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff4a90e2));

    editor.setInputRestrictions(maxChars, "0123456789");
    editor.setText("00");

    editor.onFocusLost = [this]() { notifyChange(); };
    editor.onReturnKey = [this]() { notifyChange(); };

    // Auto-advance to next field when filled
    editor.onTextChange = [this, &editor, maxChars]()
    {
        if (editor.getText().length() >= maxChars)
        {
            // Move focus to next component
            auto* parent = editor.getParentComponent();
            if (parent != nullptr)
            {
                // Simple focus advance - just notify change
                notifyChange();
            }
        }
    };
}

void TransportControlPanel::TimeInputGroup::resized()
{
    auto bounds = getLocalBounds();
    int fieldWidth = 28;
    int msFieldWidth = 35;
    int separatorWidth = 10;

    hoursInput.setBounds(bounds.removeFromLeft(fieldWidth));
    colonLabel1.setBounds(bounds.removeFromLeft(separatorWidth));
    minutesInput.setBounds(bounds.removeFromLeft(fieldWidth));
    colonLabel2.setBounds(bounds.removeFromLeft(separatorWidth));
    secondsInput.setBounds(bounds.removeFromLeft(fieldWidth));
    dotLabel.setBounds(bounds.removeFromLeft(separatorWidth));
    msInput.setBounds(bounds.removeFromLeft(msFieldWidth));
}

void TransportControlPanel::TimeInputGroup::setTime(double seconds)
{
    if (seconds < 0) seconds = 0;

    int totalMs = static_cast<int>(seconds * 1000.0);
    int hours = totalMs / 3600000;
    int minutes = (totalMs % 3600000) / 60000;
    int secs = (totalMs % 60000) / 1000;
    int ms = totalMs % 1000;

    hoursInput.setText(juce::String::formatted("%02d", hours), false);
    minutesInput.setText(juce::String::formatted("%02d", minutes), false);
    secondsInput.setText(juce::String::formatted("%02d", secs), false);
    msInput.setText(juce::String::formatted("%03d", ms), false);
}

double TransportControlPanel::TimeInputGroup::getTime() const
{
    int hours = hoursInput.getText().getIntValue();
    int minutes = minutesInput.getText().getIntValue();
    int seconds = secondsInput.getText().getIntValue();
    int ms = msInput.getText().getIntValue();

    return hours * 3600.0 + minutes * 60.0 + seconds + ms / 1000.0;
}

void TransportControlPanel::TimeInputGroup::notifyChange()
{
    if (onTimeChanged)
        onTimeChanged();
}

//==============================================================================
// TransportControlPanel implementation

TransportControlPanel::TransportControlPanel()
{
    // Current position display
    addAndMakeVisible(positionLabel);
    positionLabel.setText("Position:", juce::dontSendNotification);
    positionLabel.setFont(juce::Font(12.0f));
    positionLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible(positionValueLabel);
    positionValueLabel.setText("00:00:00.000", juce::dontSendNotification);
    positionValueLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    positionValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    positionValueLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(durationLabel);
    durationLabel.setText("/ 00:00:00.000", juce::dontSendNotification);
    durationLabel.setFont(juce::Font(12.0f));
    durationLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    durationLabel.setJustificationType(juce::Justification::centredLeft);

    // Seek to time controls
    addAndMakeVisible(seekLabel);
    seekLabel.setText("Seek:", juce::dontSendNotification);
    seekLabel.setFont(juce::Font(11.0f));
    seekLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible(seekTimeInput);
    seekTimeInput.onTimeChanged = [this]() { /* optional: auto-seek on change */ };

    addAndMakeVisible(seekButton);
    seekButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a90e2));
    seekButton.onClick = [this]() { seekToInputTime(); };

    // Quick seek buttons
    addAndMakeVisible(seekStartButton);
    seekStartButton.setTooltip("Go to start");
    seekStartButton.onClick = [this]()
    {
        if (onSeekToTime)
            onSeekToTime(0.0);
    };

    addAndMakeVisible(seekEndButton);
    seekEndButton.setTooltip("Go to end");
    seekEndButton.onClick = [this]()
    {
        if (onSeekToTime && duration > 0)
            onSeekToTime(duration - 0.01);
    };

    addAndMakeVisible(seekBackButton);
    seekBackButton.setTooltip("Skip backward");
    seekBackButton.onClick = [this]()
    {
        if (onSeekToTime)
        {
            double newPos = juce::jmax(0.0, currentPosition - skipAmountSeconds);
            onSeekToTime(newPos);
        }
    };

    addAndMakeVisible(seekForwardButton);
    seekForwardButton.setTooltip("Skip forward");
    seekForwardButton.onClick = [this]()
    {
        if (onSeekToTime && duration > 0)
        {
            double newPos = juce::jmin(duration, currentPosition + skipAmountSeconds);
            onSeekToTime(newPos);
        }
    };

    addAndMakeVisible(skipAmountLabel);
    skipAmountLabel.setText("Skip:", juce::dontSendNotification);
    skipAmountLabel.setFont(juce::Font(10.0f));
    skipAmountLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    addAndMakeVisible(skipAmountCombo);
    skipAmountCombo.addItem("1s", 1);
    skipAmountCombo.addItem("5s", 2);
    skipAmountCombo.addItem("10s", 3);
    skipAmountCombo.addItem("30s", 4);
    skipAmountCombo.addItem("1m", 5);
    skipAmountCombo.setSelectedId(2);
    skipAmountCombo.onChange = [this]()
    {
        switch (skipAmountCombo.getSelectedId())
        {
            case 1: skipAmountSeconds = 1.0; break;
            case 2: skipAmountSeconds = 5.0; break;
            case 3: skipAmountSeconds = 10.0; break;
            case 4: skipAmountSeconds = 30.0; break;
            case 5: skipAmountSeconds = 60.0; break;
            default: skipAmountSeconds = 5.0; break;
        }
    };

    // Loop controls
    addAndMakeVisible(loopToggle);
    loopToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    loopToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff4a90e2));
    loopToggle.onClick = [this]()
    {
        loopEnabled = loopToggle.getToggleState();
        if (onLoopEnabledChanged)
            onLoopEnabledChanged(loopEnabled);
    };

    addAndMakeVisible(loopStartLabel);
    loopStartLabel.setText("In:", juce::dontSendNotification);
    loopStartLabel.setFont(juce::Font(10.0f));
    loopStartLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible(loopStartInput);
    loopStartInput.onTimeChanged = [this]() { updateLoopRange(); };

    addAndMakeVisible(loopEndLabel);
    loopEndLabel.setText("Out:", juce::dontSendNotification);
    loopEndLabel.setFont(juce::Font(10.0f));
    loopEndLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible(loopEndInput);
    loopEndInput.onTimeChanged = [this]() { updateLoopRange(); };

    addAndMakeVisible(setLoopStartButton);
    setLoopStartButton.setTooltip("Set loop IN point to current position");
    setLoopStartButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a6a2a));
    setLoopStartButton.onClick = [this]()
    {
        loopStartSeconds = currentPosition;
        loopStartInput.setTime(loopStartSeconds);
        updateLoopRange();
    };

    addAndMakeVisible(setLoopEndButton);
    setLoopEndButton.setTooltip("Set loop OUT point to current position");
    setLoopEndButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6a2a2a));
    setLoopEndButton.onClick = [this]()
    {
        loopEndSeconds = currentPosition;
        loopEndInput.setTime(loopEndSeconds);
        updateLoopRange();
    };

    addAndMakeVisible(clearLoopButton);
    clearLoopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a4a));
    clearLoopButton.onClick = [this]()
    {
        loopStartSeconds = 0.0;
        loopEndSeconds = duration;
        loopStartInput.setTime(0.0);
        loopEndInput.setTime(duration);
        updateLoopRange();
    };

    startTimerHz(30);
}

TransportControlPanel::~TransportControlPanel()
{
    stopTimer();
}

//==============================================================================
void TransportControlPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("TRANSPORT", bounds.removeFromTop(22).reduced(8, 0), juce::Justification::centredLeft);

    // Loop indicator bar at bottom when loop is active
    if (loopEnabled && loopEndSeconds > loopStartSeconds && duration > 0)
    {
        auto loopBar = getLocalBounds().removeFromBottom(4);
        float startRatio = static_cast<float>(loopStartSeconds / duration);
        float endRatio = static_cast<float>(loopEndSeconds / duration);

        int startX = loopBar.getX() + static_cast<int>(startRatio * loopBar.getWidth());
        int endX = loopBar.getX() + static_cast<int>(endRatio * loopBar.getWidth());

        g.setColour(juce::Colour(0xff4a90e2));
        g.fillRect(startX, loopBar.getY(), endX - startX, loopBar.getHeight());
    }
}

void TransportControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced(6);
    bounds.removeFromTop(22);  // Title

    // Position display row
    auto posRow = bounds.removeFromTop(28);
    positionLabel.setBounds(posRow.removeFromLeft(50));
    positionValueLabel.setBounds(posRow.removeFromLeft(130));
    durationLabel.setBounds(posRow);

    bounds.removeFromTop(4);

    // Quick seek buttons row
    auto seekButtonsRow = bounds.removeFromTop(24);
    int btnWidth = 28;
    seekStartButton.setBounds(seekButtonsRow.removeFromLeft(btnWidth).reduced(1));
    seekBackButton.setBounds(seekButtonsRow.removeFromLeft(btnWidth).reduced(1));
    seekForwardButton.setBounds(seekButtonsRow.removeFromLeft(btnWidth).reduced(1));
    seekEndButton.setBounds(seekButtonsRow.removeFromLeft(btnWidth).reduced(1));
    seekButtonsRow.removeFromLeft(5);
    skipAmountLabel.setBounds(seekButtonsRow.removeFromLeft(28));
    skipAmountCombo.setBounds(seekButtonsRow.removeFromLeft(50).reduced(1));

    bounds.removeFromTop(4);

    // Seek to time row
    auto seekRow = bounds.removeFromTop(24);
    seekLabel.setBounds(seekRow.removeFromLeft(35));
    seekTimeInput.setBounds(seekRow.removeFromLeft(160).reduced(1));
    seekButton.setBounds(seekRow.removeFromLeft(35).reduced(1));

    bounds.removeFromTop(6);

    // Loop header row
    auto loopHeaderRow = bounds.removeFromTop(22);
    loopToggle.setBounds(loopHeaderRow.removeFromLeft(60));
    setLoopStartButton.setBounds(loopHeaderRow.removeFromLeft(24).reduced(1));
    setLoopEndButton.setBounds(loopHeaderRow.removeFromLeft(24).reduced(1));
    loopHeaderRow.removeFromLeft(5);
    clearLoopButton.setBounds(loopHeaderRow.removeFromLeft(45).reduced(1));

    bounds.removeFromTop(3);

    // Loop In row
    auto loopInRow = bounds.removeFromTop(22);
    loopStartLabel.setBounds(loopInRow.removeFromLeft(25));
    loopStartInput.setBounds(loopInRow.removeFromLeft(160).reduced(1));

    bounds.removeFromTop(2);

    // Loop Out row
    auto loopOutRow = bounds.removeFromTop(22);
    loopEndLabel.setBounds(loopOutRow.removeFromLeft(25));
    loopEndInput.setBounds(loopOutRow.removeFromLeft(160).reduced(1));
}

//==============================================================================
void TransportControlPanel::setPosition(double positionSeconds)
{
    currentPosition = positionSeconds;
    positionValueLabel.setText(formatTime(currentPosition), juce::dontSendNotification);
}

void TransportControlPanel::setDuration(double durationSeconds)
{
    duration = durationSeconds;
    durationLabel.setText("/ " + formatTime(duration), juce::dontSendNotification);

    // Update loop end if it was at the old duration or unset
    if (loopEndSeconds == 0.0 || loopEndSeconds > duration)
    {
        loopEndSeconds = duration;
        loopEndInput.setTime(loopEndSeconds);
    }
}

void TransportControlPanel::setSampleRate(double sampleRate)
{
    currentSampleRate = sampleRate;
}

//==============================================================================
void TransportControlPanel::setLoopEnabled(bool enabled)
{
    loopEnabled = enabled;
    loopToggle.setToggleState(enabled, juce::dontSendNotification);
}

void TransportControlPanel::setLoopRange(double startSeconds, double endSeconds)
{
    loopStartSeconds = startSeconds;
    loopEndSeconds = endSeconds;
    loopStartInput.setTime(loopStartSeconds);
    loopEndInput.setTime(loopEndSeconds);
}

//==============================================================================
void TransportControlPanel::timerCallback()
{
    repaint();
}

juce::String TransportControlPanel::formatTime(double seconds) const
{
    if (seconds < 0) seconds = 0;

    int totalMs = static_cast<int>(seconds * 1000.0);
    int hours = totalMs / 3600000;
    int minutes = (totalMs % 3600000) / 60000;
    int secs = (totalMs % 60000) / 1000;
    int ms = totalMs % 1000;

    return juce::String::formatted("%02d:%02d:%02d.%03d", hours, minutes, secs, ms);
}

void TransportControlPanel::seekToInputTime()
{
    double seekTime = seekTimeInput.getTime();

    // Clamp to valid range
    seekTime = juce::jlimit(0.0, duration, seekTime);

    if (onSeekToTime)
        onSeekToTime(seekTime);
}

void TransportControlPanel::updateLoopRange()
{
    loopStartSeconds = loopStartInput.getTime();
    loopEndSeconds = loopEndInput.getTime();

    // Ensure valid range
    loopStartSeconds = juce::jlimit(0.0, duration, loopStartSeconds);
    loopEndSeconds = juce::jlimit(0.0, duration, loopEndSeconds);

    // Ensure start < end
    if (loopStartSeconds >= loopEndSeconds)
    {
        std::swap(loopStartSeconds, loopEndSeconds);
        loopStartInput.setTime(loopStartSeconds);
        loopEndInput.setTime(loopEndSeconds);
    }

    if (onLoopRangeChanged)
        onLoopRangeChanged(loopStartSeconds, loopEndSeconds);

    repaint();
}

void TransportControlPanel::setLoopFromSelection()
{
    updateLoopRange();
}

// Remove the old parseTimeString since we now use TimeInputGroup
double TransportControlPanel::parseTimeString(const juce::String& text) const
{
    // This is no longer used but kept for compatibility
    return 0.0;
}

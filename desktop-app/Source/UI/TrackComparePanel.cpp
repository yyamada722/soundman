/*
  ==============================================================================

    TrackComparePanel.cpp

    Panel for comparing two audio tracks - implementation

  ==============================================================================
*/

#include "TrackComparePanel.h"

TrackComparePanel::TrackComparePanel()
{
    // Title
    addAndMakeVisible(titleLabel);
    titleLabel.setText("TRACK COMPARE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    // Waveform display
    addAndMakeVisible(waveformDisplay);
    waveformDisplay.onSeek = [this](double pos)
    {
        currentPosition = pos;
        if (onSeek)
            onSeek(pos);
    };

    // Track A controls
    addAndMakeVisible(loadAButton);
    loadAButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a90e2));
    loadAButton.onClick = [this]() { openFileDialogForTrack(ActiveTrack::A); };

    addAndMakeVisible(clearAButton);
    clearAButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff8b0000));
    clearAButton.onClick = [this]()
    {
        waveformDisplay.clearTrackA();
        trackAFile = juce::File();
        trackALabel.setText("No file", juce::dontSendNotification);
    };

    addAndMakeVisible(trackALabel);
    trackALabel.setText("No file", juce::dontSendNotification);
    trackALabel.setFont(juce::Font(11.0f));
    trackALabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Track B controls
    addAndMakeVisible(loadBButton);
    loadBButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe24a4a));
    loadBButton.onClick = [this]() { openFileDialogForTrack(ActiveTrack::B); };

    addAndMakeVisible(clearBButton);
    clearBButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff8b0000));
    clearBButton.onClick = [this]()
    {
        waveformDisplay.clearTrackB();
        trackBFile = juce::File();
        trackBLabel.setText("No file", juce::dontSendNotification);
    };

    addAndMakeVisible(trackBLabel);
    trackBLabel.setText("No file", juce::dontSendNotification);
    trackBLabel.setFont(juce::Font(11.0f));
    trackBLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Playback selection buttons
    addAndMakeVisible(playAButton);
    playAButton.setColour(juce::TextButton::buttonColourId, activeColor);
    playAButton.onClick = [this]() { setActiveTrack(ActiveTrack::A); };

    addAndMakeVisible(playBButton);
    playBButton.setColour(juce::TextButton::buttonColourId, inactiveColor);
    playBButton.onClick = [this]() { setActiveTrack(ActiveTrack::B); };

    addAndMakeVisible(playBothButton);
    playBothButton.setColour(juce::TextButton::buttonColourId, inactiveColor);
    playBothButton.onClick = [this]() { setActiveTrack(ActiveTrack::Both); };

    // Swap button
    addAndMakeVisible(swapButton);
    swapButton.setTooltip("Swap Track A and B");
    swapButton.onClick = [this]() { swapTracks(); };

    // Mix slider
    addAndMakeVisible(mixSlider);
    mixSlider.setRange(0.0, 1.0, 0.01);
    mixSlider.setValue(0.5);
    mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    mixSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff4a90e2));
    mixSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff2a4a6a));
    mixSlider.onValueChange = [this]()
    {
        mixBalance = static_cast<float>(mixSlider.getValue());
        mixLabel.setText("A " + juce::String(int((1.0f - mixBalance) * 100)) + "% / B " +
                        juce::String(int(mixBalance * 100)) + "%", juce::dontSendNotification);
        if (onMixBalanceChanged)
            onMixBalanceChanged(mixBalance);
    };
    mixSlider.setEnabled(false);  // Only enabled in Both mode

    addAndMakeVisible(mixLabel);
    mixLabel.setText("A 50% / B 50%", juce::dontSendNotification);
    mixLabel.setFont(juce::Font(11.0f));
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Display mode combo
    addAndMakeVisible(displayModeCombo);
    displayModeCombo.addItem("Overlay", 1);
    displayModeCombo.addItem("Split", 2);
    displayModeCombo.addItem("Difference", 3);
    displayModeCombo.setSelectedId(1);
    displayModeCombo.onChange = [this]()
    {
        int mode = displayModeCombo.getSelectedId();
        switch (mode)
        {
            case 1: waveformDisplay.setDisplayMode(CompareWaveformDisplay::DisplayMode::Overlay); break;
            case 2: waveformDisplay.setDisplayMode(CompareWaveformDisplay::DisplayMode::Split); break;
            case 3: waveformDisplay.setDisplayMode(CompareWaveformDisplay::DisplayMode::Difference); break;
        }
    };

    updateButtonStates();
}

TrackComparePanel::~TrackComparePanel()
{
}

void TrackComparePanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    auto bounds = getLocalBounds();
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);
}

void TrackComparePanel::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(22));
    bounds.removeFromTop(5);

    // Control area at bottom
    auto controlArea = bounds.removeFromBottom(100);

    // Track load buttons row
    auto loadRow = controlArea.removeFromTop(28);
    auto loadAArea = loadRow.removeFromLeft(loadRow.getWidth() / 2);
    loadAButton.setBounds(loadAArea.removeFromLeft(60).reduced(2));
    clearAButton.setBounds(loadAArea.removeFromLeft(25).reduced(2));
    trackALabel.setBounds(loadAArea.reduced(4, 0));

    loadBButton.setBounds(loadRow.removeFromLeft(60).reduced(2));
    clearBButton.setBounds(loadRow.removeFromLeft(25).reduced(2));
    trackBLabel.setBounds(loadRow.reduced(4, 0));

    controlArea.removeFromTop(5);

    // Playback selection row
    auto playRow = controlArea.removeFromTop(28);
    int buttonWidth = 50;
    playAButton.setBounds(playRow.removeFromLeft(buttonWidth).reduced(2));
    playBButton.setBounds(playRow.removeFromLeft(buttonWidth).reduced(2));
    playBothButton.setBounds(playRow.removeFromLeft(buttonWidth).reduced(2));
    swapButton.setBounds(playRow.removeFromLeft(40).reduced(2));
    displayModeCombo.setBounds(playRow.removeFromRight(100).reduced(2));

    controlArea.removeFromTop(5);

    // Mix slider row
    auto mixRow = controlArea.removeFromTop(20);
    mixLabel.setBounds(mixRow);

    auto sliderRow = controlArea.removeFromTop(20);
    mixSlider.setBounds(sliderRow);

    // Waveform display takes remaining space
    bounds.removeFromBottom(5);
    waveformDisplay.setBounds(bounds);
}

bool TrackComparePanel::loadTrackA(const juce::File& file)
{
    if (formatManager == nullptr)
        return false;

    if (waveformDisplay.loadTrackA(file, *formatManager))
    {
        trackAFile = file;
        trackALabel.setText(file.getFileName(), juce::dontSendNotification);

        if (onTrackLoaded)
            onTrackLoaded(file, ActiveTrack::A);

        return true;
    }
    return false;
}

bool TrackComparePanel::loadTrackB(const juce::File& file)
{
    if (formatManager == nullptr)
        return false;

    if (waveformDisplay.loadTrackB(file, *formatManager))
    {
        trackBFile = file;
        trackBLabel.setText(file.getFileName(), juce::dontSendNotification);

        if (onTrackLoaded)
            onTrackLoaded(file, ActiveTrack::B);

        return true;
    }
    return false;
}

void TrackComparePanel::setActiveTrack(ActiveTrack track)
{
    if (activeTrack != track)
    {
        activeTrack = track;
        updateButtonStates();

        // Enable mix slider only in Both mode
        mixSlider.setEnabled(track == ActiveTrack::Both);
        mixSlider.setAlpha(track == ActiveTrack::Both ? 1.0f : 0.5f);

        if (onActiveTrackChanged)
            onActiveTrackChanged(track);
    }
}

void TrackComparePanel::setMixBalance(float balance)
{
    mixBalance = juce::jlimit(0.0f, 1.0f, balance);
    mixSlider.setValue(mixBalance, juce::dontSendNotification);

    mixLabel.setText("A " + juce::String(int((1.0f - mixBalance) * 100)) + "% / B " +
                    juce::String(int(mixBalance * 100)) + "%", juce::dontSendNotification);
}

void TrackComparePanel::setPosition(double position)
{
    currentPosition = juce::jlimit(0.0, 1.0, position);
    waveformDisplay.setPosition(position);
}

void TrackComparePanel::openFileDialogForTrack(ActiveTrack track)
{
    fileChooser = std::make_unique<juce::FileChooser>(
        track == ActiveTrack::A ? "Select Track A" : "Select Track B",
        juce::File(),
        "*.wav;*.mp3;*.aiff;*.flac"
    );

    auto chooserFlags = juce::FileBrowserComponent::openMode |
                        juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this, track](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File())
            return;

        if (track == ActiveTrack::A)
            loadTrackA(file);
        else
            loadTrackB(file);
    });
}

void TrackComparePanel::updateButtonStates()
{
    playAButton.setColour(juce::TextButton::buttonColourId,
                          activeTrack == ActiveTrack::A ? activeColor : inactiveColor);
    playBButton.setColour(juce::TextButton::buttonColourId,
                          activeTrack == ActiveTrack::B ? activeColor : inactiveColor);
    playBothButton.setColour(juce::TextButton::buttonColourId,
                             activeTrack == ActiveTrack::Both ? activeColor : inactiveColor);
}

void TrackComparePanel::swapTracks()
{
    // Swap files
    std::swap(trackAFile, trackBFile);

    // Reload if we have files
    if (formatManager != nullptr)
    {
        waveformDisplay.clearTrackA();
        waveformDisplay.clearTrackB();

        if (trackAFile.existsAsFile())
        {
            waveformDisplay.loadTrackA(trackAFile, *formatManager);
            trackALabel.setText(trackAFile.getFileName(), juce::dontSendNotification);
        }
        else
        {
            trackALabel.setText("No file", juce::dontSendNotification);
        }

        if (trackBFile.existsAsFile())
        {
            waveformDisplay.loadTrackB(trackBFile, *formatManager);
            trackBLabel.setText(trackBFile.getFileName(), juce::dontSendNotification);
        }
        else
        {
            trackBLabel.setText("No file", juce::dontSendNotification);
        }
    }
}

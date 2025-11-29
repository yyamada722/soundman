/*
  ==============================================================================

    MixerPanel.cpp

    DAW-style mixer panel implementation

  ==============================================================================
*/

#include "MixerPanel.h"

//==============================================================================
// ChannelMeterComponent Implementation
//==============================================================================

ChannelMeterComponent::ChannelMeterComponent()
{
    startTimerHz(30);
}

ChannelMeterComponent::~ChannelMeterComponent()
{
    stopTimer();
}

void ChannelMeterComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    int meterWidth = bounds.getWidth() / 2 - 1;

    // Background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(bounds);

    // Left channel
    auto leftBounds = bounds.removeFromLeft(meterWidth);
    float leftHeight = leftBounds.getHeight() * levelL;

    // Gradient fill for left meter
    juce::ColourGradient leftGradient(
        juce::Colours::green, 0, static_cast<float>(leftBounds.getBottom()),
        juce::Colours::red, 0, static_cast<float>(leftBounds.getY()),
        false);
    leftGradient.addColour(0.6, juce::Colours::yellow);

    g.setGradientFill(leftGradient);
    g.fillRect(leftBounds.getX(), leftBounds.getBottom() - static_cast<int>(leftHeight),
               meterWidth, static_cast<int>(leftHeight));

    // Peak hold left
    if (peakL > 0.0f)
    {
        int peakY = leftBounds.getBottom() - static_cast<int>(leftBounds.getHeight() * peakL);
        g.setColour(peakL > 0.9f ? juce::Colours::red : juce::Colours::white);
        g.fillRect(leftBounds.getX(), peakY, meterWidth, 2);
    }

    bounds.removeFromLeft(2);  // Gap

    // Right channel
    auto rightBounds = bounds;
    float rightHeight = rightBounds.getHeight() * levelR;

    // Gradient fill for right meter
    juce::ColourGradient rightGradient(
        juce::Colours::green, 0, static_cast<float>(rightBounds.getBottom()),
        juce::Colours::red, 0, static_cast<float>(rightBounds.getY()),
        false);
    rightGradient.addColour(0.6, juce::Colours::yellow);

    g.setGradientFill(rightGradient);
    g.fillRect(rightBounds.getX(), rightBounds.getBottom() - static_cast<int>(rightHeight),
               meterWidth, static_cast<int>(rightHeight));

    // Peak hold right
    if (peakR > 0.0f)
    {
        int peakY = rightBounds.getBottom() - static_cast<int>(rightBounds.getHeight() * peakR);
        g.setColour(peakR > 0.9f ? juce::Colours::red : juce::Colours::white);
        g.fillRect(rightBounds.getX(), peakY, meterWidth, 2);
    }

    // Border
    g.setColour(juce::Colours::grey.darker());
    g.drawRect(getLocalBounds());
}

void ChannelMeterComponent::timerCallback()
{
    // Decay peak hold
    peakL *= peakDecayRate;
    peakR *= peakDecayRate;

    if (peakL < 0.001f) peakL = 0.0f;
    if (peakR < 0.001f) peakR = 0.0f;

    // Decay levels
    levelL *= 0.9f;
    levelR *= 0.9f;

    repaint();
}

void ChannelMeterComponent::setLevel(float left, float right)
{
    levelL = juce::jmax(levelL, juce::jlimit(0.0f, 1.0f, left));
    levelR = juce::jmax(levelR, juce::jlimit(0.0f, 1.0f, right));
}

void ChannelMeterComponent::setPeakHold(float left, float right)
{
    peakL = juce::jmax(peakL, juce::jlimit(0.0f, 1.0f, left));
    peakR = juce::jmax(peakR, juce::jlimit(0.0f, 1.0f, right));
}

//==============================================================================
// PanKnobComponent Implementation
//==============================================================================

PanKnobComponent::PanKnobComponent()
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    knob.setRange(-1.0, 1.0, 0.01);
    knob.setValue(0.0);
    knob.setDoubleClickReturnValue(true, 0.0);
    knob.onValueChange = [this]() {
        value = static_cast<float>(knob.getValue());
        if (onValueChange)
            onValueChange(value);
        repaint();
    };
    addAndMakeVisible(knob);
}

void PanKnobComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Draw L and R labels
    g.setColour(juce::Colours::grey);
    g.setFont(10.0f);
    g.drawText("L", bounds.removeFromLeft(12), juce::Justification::centred);
    g.drawText("R", bounds.removeFromRight(12), juce::Justification::centred);

    // Draw center line indicator
    auto knobBounds = bounds.reduced(4);
    float centerX = knobBounds.getCentreX();
    float indicatorX = centerX + (value * (knobBounds.getWidth() / 2 - 4));

    g.setColour(juce::Colours::white);
    g.fillEllipse(indicatorX - 2, bounds.getBottom() - 6, 4, 4);
}

void PanKnobComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.reduce(12, 0);  // Leave space for L/R labels
    knob.setBounds(bounds);
}

void PanKnobComponent::setValue(float panValue)
{
    value = juce::jlimit(-1.0f, 1.0f, panValue);
    knob.setValue(value, juce::dontSendNotification);
    repaint();
}

//==============================================================================
// ChannelStripComponent Implementation
//==============================================================================

ChannelStripComponent::ChannelStripComponent(ProjectManager& pm, const juce::ValueTree& trackState)
    : projectManager(pm)
    , state(trackState)
{
    setupComponents();
    updateFromState();
}

void ChannelStripComponent::setupComponents()
{
    // Name label
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::Font(11.0f));
    nameLabel.setEditable(true);
    nameLabel.onTextChange = [this]() {
        projectManager.setTrackName(state, nameLabel.getText());
    };
    addAndMakeVisible(nameLabel);

    // Fader
    faderSlider.setSliderStyle(juce::Slider::LinearVertical);
    faderSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    faderSlider.setRange(0.0, 2.0, 0.01);
    faderSlider.setValue(1.0);
    faderSlider.setSkewFactorFromMidPoint(1.0);
    faderSlider.onValueChange = [this]() {
        projectManager.setTrackVolume(state, static_cast<float>(faderSlider.getValue()));
        updateLevelLabel();
    };
    addAndMakeVisible(faderSlider);

    // Pan knob
    panKnob.onValueChange = [this](float value) {
        projectManager.setTrackPan(state, value);
    };
    addAndMakeVisible(panKnob);

    // Meter
    addAndMakeVisible(meter);

    // Mute button
    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    muteButton.onClick = [this]() {
        projectManager.setTrackMute(state, muteButton.getToggleState());
    };
    addAndMakeVisible(muteButton);

    // Solo button
    soloButton.setClickingTogglesState(true);
    soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow);
    soloButton.onClick = [this]() {
        projectManager.setTrackSolo(state, soloButton.getToggleState());
    };
    addAndMakeVisible(soloButton);

    // Arm button
    armButton.setClickingTogglesState(true);
    armButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
    armButton.onClick = [this]() {
        state.setProperty(IDs::armed, armButton.getToggleState(), nullptr);
    };
    addAndMakeVisible(armButton);

    // Level label
    levelLabel.setJustificationType(juce::Justification::centred);
    levelLabel.setFont(juce::Font(10.0f));
    levelLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(levelLabel);
}

void ChannelStripComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(bounds);

    // Top color strip
    g.setColour(trackColor);
    g.fillRect(bounds.removeFromTop(4));

    // Border
    g.setColour(juce::Colours::grey.darker());
    g.drawRect(getLocalBounds());
}

void ChannelStripComponent::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    bounds.removeFromTop(4);  // Color strip

    // Name at top
    nameLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(4);

    // Buttons row
    auto buttonRow = bounds.removeFromTop(22);
    int buttonWidth = (buttonRow.getWidth() - 4) / 3;
    muteButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(2);
    soloButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(2);
    armButton.setBounds(buttonRow);

    bounds.removeFromTop(4);

    // Pan knob
    panKnob.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(4);

    // Level label at bottom
    levelLabel.setBounds(bounds.removeFromBottom(16));
    bounds.removeFromBottom(4);

    // Meter on the right
    auto meterBounds = bounds.removeFromRight(24);
    meter.setBounds(meterBounds);

    bounds.removeFromRight(4);

    // Fader takes remaining space
    faderSlider.setBounds(bounds);
}

void ChannelStripComponent::updateFromState()
{
    TrackModel track(state);

    nameLabel.setText(track.getName(), juce::dontSendNotification);
    trackColor = track.getColor();
    faderSlider.setValue(track.getVolume(), juce::dontSendNotification);
    panKnob.setValue(track.getPan());
    muteButton.setToggleState(track.isMuted(), juce::dontSendNotification);
    soloButton.setToggleState(track.isSoloed(), juce::dontSendNotification);
    armButton.setToggleState(track.isArmed(), juce::dontSendNotification);

    updateLevelLabel();
    repaint();
}

juce::String ChannelStripComponent::getTrackId() const
{
    return state[IDs::trackId].toString();
}

void ChannelStripComponent::setMeterLevels(float left, float right)
{
    meter.setLevel(left, right);
    meter.setPeakHold(left, right);
}

void ChannelStripComponent::updateLevelLabel()
{
    float volume = static_cast<float>(faderSlider.getValue());
    float dB = (volume > 0.0001f) ? 20.0f * std::log10(volume) : -60.0f;
    levelLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
}

//==============================================================================
// MasterChannelStripComponent Implementation
//==============================================================================

MasterChannelStripComponent::MasterChannelStripComponent(ProjectManager& pm)
    : projectManager(pm)
{
    setupComponents();
    updateFromProject();
}

void MasterChannelStripComponent::setupComponents()
{
    // Name label
    nameLabel.setText("MASTER", juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    addAndMakeVisible(nameLabel);

    // Fader
    faderSlider.setSliderStyle(juce::Slider::LinearVertical);
    faderSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    faderSlider.setRange(0.0, 2.0, 0.01);
    faderSlider.setValue(1.0);
    faderSlider.setSkewFactorFromMidPoint(1.0);
    faderSlider.onValueChange = [this]() {
        projectManager.setMasterVolume(static_cast<float>(faderSlider.getValue()));
        updateLevelLabel();
    };
    addAndMakeVisible(faderSlider);

    // Pan knob
    panKnob.onValueChange = [this](float value) {
        projectManager.setMasterPan(value);
    };
    addAndMakeVisible(panKnob);

    // Meter
    addAndMakeVisible(meter);

    // Level label
    levelLabel.setJustificationType(juce::Justification::centred);
    levelLabel.setFont(juce::Font(10.0f));
    levelLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(levelLabel);

    updateLevelLabel();
}

void MasterChannelStripComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background (slightly different from channel strips)
    g.setColour(juce::Colour(0xff333333));
    g.fillRect(bounds);

    // Top color strip (master = red)
    g.setColour(juce::Colours::red.darker());
    g.fillRect(bounds.removeFromTop(4));

    // Border
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds());
}

void MasterChannelStripComponent::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    bounds.removeFromTop(4);  // Color strip

    // Name at top
    nameLabel.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(8);

    // Pan knob
    panKnob.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(4);

    // Level label at bottom
    levelLabel.setBounds(bounds.removeFromBottom(16));
    bounds.removeFromBottom(4);

    // Meter on the right
    auto meterBounds = bounds.removeFromRight(30);
    meter.setBounds(meterBounds);

    bounds.removeFromRight(4);

    // Fader takes remaining space
    faderSlider.setBounds(bounds);
}

void MasterChannelStripComponent::updateFromProject()
{
    auto& project = projectManager.getProject();
    faderSlider.setValue(project.getMasterVolume(), juce::dontSendNotification);
    panKnob.setValue(project.getMasterPan());
    updateLevelLabel();
}

void MasterChannelStripComponent::setMeterLevels(float left, float right)
{
    meter.setLevel(left, right);
    meter.setPeakHold(left, right);
}

void MasterChannelStripComponent::updateLevelLabel()
{
    float volume = static_cast<float>(faderSlider.getValue());
    float dB = (volume > 0.0001f) ? 20.0f * std::log10(volume) : -60.0f;
    levelLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
}

//==============================================================================
// MixerPanel Implementation
//==============================================================================

MixerPanel::MixerPanel(ProjectManager& pm)
    : projectManager(pm)
{
    projectManager.addListener(this);

    // Setup viewport
    viewport.setViewedComponent(&stripContainer, false);
    viewport.setScrollBarsShown(false, true);
    addAndMakeVisible(viewport);

    // Create master strip
    masterStrip = std::make_unique<MasterChannelStripComponent>(projectManager);
    addAndMakeVisible(masterStrip.get());

    rebuildStrips();

    startTimerHz(30);
}

MixerPanel::~MixerPanel()
{
    stopTimer();
    projectManager.removeListener(this);
}

void MixerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void MixerPanel::resized()
{
    auto bounds = getLocalBounds();

    // Master strip on the right
    masterStrip->setBounds(bounds.removeFromRight(masterStripWidth));

    // Separator
    bounds.removeFromRight(2);

    // Viewport for channel strips
    viewport.setBounds(bounds);

    layoutStrips();
}

void MixerPanel::timerCallback()
{
    // Update master strip from project (in case external changes)
    masterStrip->updateFromProject();
}

void MixerPanel::projectChanged()
{
    rebuildStrips();
    masterStrip->updateFromProject();
}

void MixerPanel::trackAdded(const juce::ValueTree& track)
{
    juce::ignoreUnused(track);
    rebuildStrips();
}

void MixerPanel::trackRemoved(const juce::ValueTree& track)
{
    juce::ignoreUnused(track);
    rebuildStrips();
}

void MixerPanel::trackPropertyChanged(const juce::ValueTree& track, const juce::Identifier& property)
{
    juce::ignoreUnused(property);

    juce::String trackId = track[IDs::trackId].toString();

    for (auto* strip : channelStrips)
    {
        if (strip->getTrackId() == trackId)
        {
            strip->updateFromState();
            break;
        }
    }
}

void MixerPanel::setTrackLevels(const juce::String& trackId, float left, float right)
{
    for (auto* strip : channelStrips)
    {
        if (strip->getTrackId() == trackId)
        {
            strip->setMeterLevels(left, right);
            break;
        }
    }
}

void MixerPanel::setMasterLevels(float left, float right)
{
    masterStrip->setMeterLevels(left, right);
}

void MixerPanel::rebuildStrips()
{
    channelStrips.clear();

    auto& project = projectManager.getProject();
    auto tracks = project.getTracksSortedByOrder();

    for (auto& trackState : tracks)
    {
        auto* strip = new ChannelStripComponent(projectManager, trackState);
        channelStrips.add(strip);
        stripContainer.addAndMakeVisible(strip);
    }

    layoutStrips();
}

void MixerPanel::layoutStrips()
{
    int numStrips = channelStrips.size();
    int totalWidth = numStrips * stripWidth;

    stripContainer.setSize(juce::jmax(totalWidth, viewport.getWidth()),
                           viewport.getHeight());

    int x = 0;
    for (auto* strip : channelStrips)
    {
        strip->setBounds(x, 0, stripWidth, stripContainer.getHeight());
        x += stripWidth;
    }
}

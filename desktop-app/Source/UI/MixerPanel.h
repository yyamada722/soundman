/*
  ==============================================================================

    MixerPanel.h

    DAW-style mixer panel with channel strips for each track

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Core/ProjectModel.h"
#include "../Core/ProjectManager.h"

//==============================================================================
// Forward declarations
//==============================================================================
class MixerPanel;
class ChannelStripComponent;

//==============================================================================
// Channel Meter Component - Vertical level meter for a channel
//==============================================================================

class ChannelMeterComponent : public juce::Component,
                               public juce::Timer
{
public:
    ChannelMeterComponent();
    ~ChannelMeterComponent() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

    void setLevel(float left, float right);
    void setPeakHold(float left, float right);

private:
    float levelL { 0.0f };
    float levelR { 0.0f };
    float peakL { 0.0f };
    float peakR { 0.0f };

    static constexpr float peakDecayRate = 0.95f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelMeterComponent)
};

//==============================================================================
// Pan Knob Component - Rotary pan control
//==============================================================================

class PanKnobComponent : public juce::Component
{
public:
    PanKnobComponent();
    ~PanKnobComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setValue(float panValue);  // -1.0 to 1.0
    float getValue() const { return value; }

    std::function<void(float)> onValueChange;

private:
    juce::Slider knob;
    float value { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanKnobComponent)
};

//==============================================================================
// Channel Strip Component - Individual mixer channel
//==============================================================================

class ChannelStripComponent : public juce::Component
{
public:
    ChannelStripComponent(ProjectManager& pm, const juce::ValueTree& trackState);
    ~ChannelStripComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateFromState();
    juce::String getTrackId() const;

    // Set levels for metering
    void setMeterLevels(float left, float right);

private:
    ProjectManager& projectManager;
    juce::ValueTree state;

    // UI Components
    juce::Label nameLabel;
    juce::Slider faderSlider;
    PanKnobComponent panKnob;
    ChannelMeterComponent meter;
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };
    juce::TextButton armButton { "R" };
    juce::Label levelLabel;

    juce::Colour trackColor;

    void setupComponents();
    void updateLevelLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStripComponent)
};

//==============================================================================
// Master Channel Strip - Master fader and meters
//==============================================================================

class MasterChannelStripComponent : public juce::Component
{
public:
    MasterChannelStripComponent(ProjectManager& pm);
    ~MasterChannelStripComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateFromProject();
    void setMeterLevels(float left, float right);

private:
    ProjectManager& projectManager;

    // UI Components
    juce::Label nameLabel;
    juce::Slider faderSlider;
    PanKnobComponent panKnob;
    ChannelMeterComponent meter;
    juce::Label levelLabel;

    void setupComponents();
    void updateLevelLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterChannelStripComponent)
};

//==============================================================================
// Mixer Panel - Main container for all channel strips
//==============================================================================

class MixerPanel : public juce::Component,
                   public juce::Timer,
                   public ProjectManager::Listener
{
public:
    MixerPanel(ProjectManager& pm);
    ~MixerPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timer
    //==========================================================================
    void timerCallback() override;

    //==========================================================================
    // ProjectManager::Listener
    //==========================================================================
    void projectChanged() override;
    void trackAdded(const juce::ValueTree& track) override;
    void trackRemoved(const juce::ValueTree& track) override;
    void trackPropertyChanged(const juce::ValueTree& track, const juce::Identifier& property) override;

    //==========================================================================
    // Metering
    //==========================================================================

    // Set levels for a specific track (called from audio thread)
    void setTrackLevels(const juce::String& trackId, float left, float right);

    // Set master levels
    void setMasterLevels(float left, float right);

private:
    ProjectManager& projectManager;

    // UI Components
    juce::Viewport viewport;
    juce::Component stripContainer;
    juce::OwnedArray<ChannelStripComponent> channelStrips;
    std::unique_ptr<MasterChannelStripComponent> masterStrip;

    // Layout
    static constexpr int stripWidth = 80;
    static constexpr int masterStripWidth = 100;

    void rebuildStrips();
    void layoutStrips();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};

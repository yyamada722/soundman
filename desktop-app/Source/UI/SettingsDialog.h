/*
  ==============================================================================

    SettingsDialog.h

    Settings dialog for audio device and application configuration

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <functional>

class SettingsDialog : public juce::Component
{
public:
    //==========================================================================
    SettingsDialog(juce::AudioDeviceManager& deviceManager);
    ~SettingsDialog() override;

    //==========================================================================
    // Settings change callback
    using SettingsChangedCallback = std::function<void()>;
    void setSettingsChangedCallback(SettingsChangedCallback callback) { settingsChangedCallback = callback; }

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    void setupAudioTab();
    void updateDeviceList();
    void updateSampleRateList();
    void updateBufferSizeList();
    void applyAudioSettings();
    void refreshCurrentSettings();

    //==========================================================================
    juce::AudioDeviceManager& audioDeviceManager;

    // Audio settings components
    juce::TabbedComponent tabbedComponent { juce::TabbedButtonBar::TabsAtTop };
    juce::Component audioTab;

    juce::Label outputDeviceLabel;
    juce::ComboBox outputDeviceCombo;

    juce::Label sampleRateLabel;
    juce::ComboBox sampleRateCombo;

    juce::Label bufferSizeLabel;
    juce::ComboBox bufferSizeCombo;

    juce::Label currentSettingsLabel;
    juce::TextEditor currentSettingsDisplay;

    juce::TextButton applyButton;
    juce::TextButton closeButton;

    SettingsChangedCallback settingsChangedCallback;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsDialog)
};

/*
  ==============================================================================

    DeviceControlPanel.h

    Left sidebar panel for device and transport controls

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class DeviceControlPanel : public juce::Component
{
public:
    //==========================================================================
    DeviceControlPanel();
    ~DeviceControlPanel() override;

    //==========================================================================
    // Device info display
    void setDeviceName(const juce::String& name);
    void setSampleRate(double sampleRate);
    void setBufferSize(int bufferSize);
    void setLoadedFileName(const juce::String& name);

    //==========================================================================
    // Callbacks
    using LoadButtonCallback = std::function<void()>;
    using PlayButtonCallback = std::function<void()>;
    using PauseButtonCallback = std::function<void()>;
    using StopButtonCallback = std::function<void()>;

    void setLoadButtonCallback(LoadButtonCallback callback) { loadCallback = callback; }
    void setPlayButtonCallback(PlayButtonCallback callback) { playCallback = callback; }
    void setPauseButtonCallback(PauseButtonCallback callback) { pauseCallback = callback; }
    void setStopButtonCallback(StopButtonCallback callback) { stopCallback = callback; }

    //==========================================================================
    // Button states
    void setPlayButtonEnabled(bool enabled);
    void setPauseButtonEnabled(bool enabled);
    void setStopButtonEnabled(bool enabled);

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    // Transport controls
    juce::TextButton loadButton;
    juce::TextButton playButton;
    juce::TextButton pauseButton;
    juce::TextButton stopButton;

    // Device info labels
    juce::Label deviceLabel;
    juce::Label sampleRateLabel;
    juce::Label bufferSizeLabel;
    juce::Label fileNameLabel;

    juce::String deviceName;
    double currentSampleRate { 0.0 };
    int currentBufferSize { 0 };
    juce::String loadedFileName;

    // Callbacks
    LoadButtonCallback loadCallback;
    PlayButtonCallback playCallback;
    PauseButtonCallback pauseCallback;
    StopButtonCallback stopCallback;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceControlPanel)
};

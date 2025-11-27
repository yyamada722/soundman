/*
  ==============================================================================

    RecordingPanel.h

    Real-time audio recording panel with level monitoring

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <functional>

class RecordingPanel : public juce::Component,
                       public juce::Timer
{
public:
    //==========================================================================
    RecordingPanel();
    ~RecordingPanel() override;

    //==========================================================================
    // Recording state
    enum class RecordingState
    {
        Stopped,
        Recording,
        Paused
    };

    void setRecordingState(RecordingState state);
    RecordingState getRecordingState() const { return recordingState; }

    //==========================================================================
    // Level monitoring
    void setInputLevels(float leftRMS, float leftPeak, float rightRMS, float rightPeak);

    //==========================================================================
    // Recording info
    void setRecordingDuration(double seconds);
    void setRecordingFileName(const juce::String& fileName);
    void setInputDevice(const juce::String& deviceName);

    //==========================================================================
    // Callbacks
    using RecordCallback = std::function<void()>;
    using StopCallback = std::function<void()>;
    using PauseCallback = std::function<void()>;

    void setRecordCallback(RecordCallback callback) { recordCallback = callback; }
    void setStopCallback(StopCallback callback) { stopCallback = callback; }
    void setPauseCallback(PauseCallback callback) { pauseCallback = callback; }

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timer override
    void timerCallback() override;

private:
    //==========================================================================
    void drawLevelMeter(juce::Graphics& g, const juce::Rectangle<int>& bounds, float rms, float peak);

    //==========================================================================
    RecordingState recordingState { RecordingState::Stopped };

    juce::Label titleLabel;
    juce::Label deviceLabel;
    juce::Label deviceNameLabel;
    juce::Label fileNameLabel;
    juce::Label durationLabel;

    juce::TextButton recordButton;
    juce::TextButton stopButton;
    juce::TextButton pauseButton;

    // Level meters
    float leftRMS { 0.0f };
    float leftPeak { 0.0f };
    float rightRMS { 0.0f };
    float rightPeak { 0.0f };

    // Peak hold for visual feedback
    float leftPeakHold { 0.0f };
    float rightPeakHold { 0.0f };
    int peakHoldCounter { 0 };
    static constexpr int PEAK_HOLD_FRAMES = 30;

    RecordCallback recordCallback;
    StopCallback stopCallback;
    PauseCallback pauseCallback;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingPanel)
};

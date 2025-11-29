/*
  ==============================================================================

    TransportControlPanel.h

    Transport controls with timecode seek and range/loop playback

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class TransportControlPanel : public juce::Component,
                               private juce::Timer
{
public:
    //==========================================================================
    TransportControlPanel();
    ~TransportControlPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Position updates
    void setPosition(double positionSeconds);
    void setDuration(double durationSeconds);
    void setSampleRate(double sampleRate);

    double getPosition() const { return currentPosition; }
    double getDuration() const { return duration; }

    //==========================================================================
    // Range/Loop settings
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const { return loopEnabled; }

    void setLoopRange(double startSeconds, double endSeconds);
    double getLoopStart() const { return loopStartSeconds; }
    double getLoopEnd() const { return loopEndSeconds; }

    //==========================================================================
    // Callbacks
    std::function<void(double)> onSeekToTime;           // Seek to time in seconds
    std::function<void(bool)> onLoopEnabledChanged;     // Loop toggle changed
    std::function<void(double, double)> onLoopRangeChanged;  // Loop range changed

private:
    void timerCallback() override;

    // Parse time string to seconds
    double parseTimeString(const juce::String& text) const;

    // Format seconds to time string
    juce::String formatTime(double seconds) const;

    // UI event handlers
    void seekToInputTime();
    void updateLoopRange();
    void setLoopFromSelection();

    //==========================================================================
    // State
    double currentPosition { 0.0 };      // Current position in seconds
    double duration { 0.0 };             // Total duration in seconds
    double currentSampleRate { 44100.0 };

    bool loopEnabled { false };
    double loopStartSeconds { 0.0 };
    double loopEndSeconds { 0.0 };

    //==========================================================================
    // Helper class for time input with separate fields
    class TimeInputGroup : public juce::Component
    {
    public:
        TimeInputGroup();

        void resized() override;

        void setTime(double seconds);
        double getTime() const;

        std::function<void()> onTimeChanged;

    private:
        void setupEditor(juce::TextEditor& editor, int maxChars, const juce::String& suffix);
        void notifyChange();

        juce::TextEditor hoursInput;
        juce::TextEditor minutesInput;
        juce::TextEditor secondsInput;
        juce::TextEditor msInput;

        juce::Label colonLabel1 { {}, ":" };
        juce::Label colonLabel2 { {}, ":" };
        juce::Label dotLabel { {}, "." };
    };

    //==========================================================================
    // UI Components

    // Current position display
    juce::Label positionLabel;
    juce::Label positionValueLabel;
    juce::Label durationLabel;

    // Seek to time
    juce::Label seekLabel;
    TimeInputGroup seekTimeInput;
    juce::TextButton seekButton { "Go" };

    // Quick seek buttons
    juce::TextButton seekStartButton { "|<" };
    juce::TextButton seekEndButton { ">|" };
    juce::TextButton seekBackButton { "<<" };
    juce::TextButton seekForwardButton { ">>" };
    juce::Label skipAmountLabel;
    juce::ComboBox skipAmountCombo;

    // Loop/Range controls
    juce::ToggleButton loopToggle { "Loop" };
    juce::Label loopStartLabel;
    TimeInputGroup loopStartInput;
    juce::Label loopEndLabel;
    TimeInputGroup loopEndInput;
    juce::TextButton setLoopStartButton { "[" };
    juce::TextButton setLoopEndButton { "]" };
    juce::TextButton clearLoopButton { "Clear" };

    // Skip amount in seconds
    double skipAmountSeconds { 5.0 };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportControlPanel)
};

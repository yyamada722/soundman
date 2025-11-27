#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class TimecodeDisplay : public juce::Component,
                        private juce::Timer
{
public:
    enum class TimecodeFormat
    {
        SMPTE,           // HH:MM:SS:FF (frames)
        Samples,         // Sample count
        Milliseconds,    // Time in ms
        Timecode         // HH:MM:SS.mmm
    };

    TimecodeDisplay();
    ~TimecodeDisplay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Update current position
    void setPosition(int64_t samples, double sampleRate);
    void setFormat(TimecodeFormat format);
    void setFrameRate(int fps) { frameRate = fps; }

private:
    void timerCallback() override;
    juce::String formatTimecode(int64_t samples, double sampleRate);
    juce::String formatSMPTE(int64_t samples, double sampleRate);
    juce::String formatSamples(int64_t samples);
    juce::String formatMilliseconds(int64_t samples, double sampleRate);
    juce::String formatTime(int64_t samples, double sampleRate);

    int64_t currentSamples { 0 };
    double currentSampleRate { 44100.0 };
    TimecodeFormat currentFormat { TimecodeFormat::Timecode };
    int frameRate { 30 };

    juce::Label timecodeLabel;
    juce::Label formatLabel;
    juce::ComboBox formatCombo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimecodeDisplay)
};

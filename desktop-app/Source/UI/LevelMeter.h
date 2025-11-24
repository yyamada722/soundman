/*
  ==============================================================================

    LevelMeter.h

    Real-time audio level meter with peak hold and clipping detection

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>

class LevelMeter : public juce::Component,
                   public juce::Timer
{
public:
    //==========================================================================
    LevelMeter();
    ~LevelMeter() override;

    //==========================================================================
    // Set audio levels (thread-safe, called from audio thread)
    void setLevels(float leftRMS, float leftPeak, float rightRMS, float rightPeak);
    void reset();

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timer override
    void timerCallback() override;

private:
    //==========================================================================
    struct ChannelLevel
    {
        std::atomic<float> rms { 0.0f };
        std::atomic<float> peak { 0.0f };
        float peakHold { 0.0f };
        int peakHoldTime { 0 };
        bool clipping { false };
    };

    std::array<ChannelLevel, 2> channels;  // 0=Left, 1=Right

    //==========================================================================
    // Drawing helpers
    void drawChannel(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                    const ChannelLevel& level, const juce::String& label);
    void drawScale(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    float levelToY(float level, int height) const;
    juce::String levelToString(float level) const;

    //==========================================================================
    // Constants
    static constexpr float MIN_DB = -60.0f;
    static constexpr float MAX_DB = 0.0f;
    static constexpr int PEAK_HOLD_FRAMES = 30;  // ~1 second at 30fps

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

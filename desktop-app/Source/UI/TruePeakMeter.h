/*
  ==============================================================================

    TruePeakMeter.h

    True Peak meter with inter-sample peak detection

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

class TruePeakMeter : public juce::Component,
                      public juce::Timer
{
public:
    //==========================================================================
    TruePeakMeter();
    ~TruePeakMeter() override;

    //==========================================================================
    // Set true peak values (in linear 0.0-1.0+, called from audio thread)
    void setTruePeaks(float leftPeak, float rightPeak);

    // Reset peak hold values
    void resetPeakHold();

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    //==========================================================================
    // Timer override (for decay and display updates)
    void timerCallback() override;

private:
    //==========================================================================
    void drawChannel(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                    float peakValue, float holdValue, const juce::String& label);
    juce::Colour getColorForLevel(float dbValue) const;
    float linearToDb(float linear) const;

    //==========================================================================
    // Peak values (atomic for thread safety)
    std::atomic<float> leftTruePeak { 0.0f };
    std::atomic<float> rightTruePeak { 0.0f };

    // Peak hold values
    float leftPeakHold { 0.0f };
    float rightPeakHold { 0.0f };

    // Hold timer
    int leftHoldTimer { 0 };
    int rightHoldTimer { 0 };
    static constexpr int HOLD_TIME_MS = 2000;

    // Display decay
    float leftDisplayPeak { 0.0f };
    float rightDisplayPeak { 0.0f };
    static constexpr float DECAY_RATE = 0.95f;

    // Clipping indicators
    bool leftClipping { false };
    bool rightClipping { false };
    int leftClipTimer { 0 };
    int rightClipTimer { 0 };
    static constexpr int CLIP_HOLD_TIME_MS = 1000;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TruePeakMeter)
};

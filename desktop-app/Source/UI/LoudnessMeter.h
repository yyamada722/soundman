/*
  ==============================================================================

    LoudnessMeter.h

    ITU-R BS.1770-4 compliant loudness meter for broadcast standards

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

class LoudnessMeter : public juce::Component,
                      public juce::Timer
{
public:
    //==========================================================================
    LoudnessMeter();
    ~LoudnessMeter() override;

    //==========================================================================
    // Set loudness values (called from audio thread)
    void setIntegratedLoudness(float lufs);     // Integrated (LUFS I)
    void setShortTermLoudness(float lufs);      // Short-term (LUFS S) - 3s
    void setMomentaryLoudness(float lufs);      // Momentary (LUFS M) - 400ms
    void setLoudnessRange(float lra);           // Loudness Range (LU)

    // Reset measurements
    void reset();

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    //==========================================================================
    // Timer override (for display updates)
    void timerCallback() override;

private:
    //==========================================================================
    void drawMeter(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                   float value, const juce::String& label);
    void drawNumericValues(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawLoudnessBar(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                        float value, const juce::String& label);
    juce::Colour getColorForLoudness(float lufs) const;

    //==========================================================================
    // Loudness values (atomic for thread safety)
    std::atomic<float> integratedLoudness { -70.0f };
    std::atomic<float> shortTermLoudness { -70.0f };
    std::atomic<float> momentaryLoudness { -70.0f };
    std::atomic<float> loudnessRange { 0.0f };

    // Display values with smoothing
    float displayIntegrated { -70.0f };
    float displayShortTerm { -70.0f };
    float displayMomentary { -70.0f };
    float displayLRA { 0.0f };

    static constexpr float SMOOTHING = 0.85f;

    // Broadcast standards reference levels
    static constexpr float TARGET_LEVEL = -23.0f;     // EBU R128 target
    static constexpr float MAX_SHORT_TERM = -18.0f;   // Maximum short-term
    static constexpr float ABSOLUTE_GATE = -70.0f;    // Absolute gate threshold

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoudnessMeter)
};

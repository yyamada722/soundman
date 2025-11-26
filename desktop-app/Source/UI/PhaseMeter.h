/*
  ==============================================================================

    PhaseMeter.h

    Stereo phase correlation meter for monitoring stereo imaging

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

class PhaseMeter : public juce::Component,
                   public juce::Timer
{
public:
    //==========================================================================
    PhaseMeter();
    ~PhaseMeter() override;

    //==========================================================================
    // Set correlation value (called from audio thread)
    // Range: -1.0 (out of phase) to +1.0 (in phase)
    void setCorrelation(float correlation);

    // Reset meter
    void reset();

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timer override (for display updates)
    void timerCallback() override;

private:
    //==========================================================================
    void drawCorrelationMeter(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawNumericValue(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawScale(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    juce::Colour getColorForCorrelation(float corr) const;

    //==========================================================================
    // Correlation value (atomic for thread safety)
    std::atomic<float> currentCorrelation { 0.0f };

    // Display value with smoothing
    float displayCorrelation { 0.0f };
    static constexpr float SMOOTHING = 0.9f;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseMeter)
};

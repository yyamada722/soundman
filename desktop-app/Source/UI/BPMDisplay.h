/*
  ==============================================================================

    BPMDisplay.h

    BPM (Beats Per Minute) detection display component

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/BPMDetector.h"

class BPMDisplay : public juce::Component,
                   public juce::Timer,
                   public juce::Slider::Listener
{
public:
    //==========================================================================
    BPMDisplay();
    ~BPMDisplay() override;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);
    void processBlock(const juce::AudioBuffer<float>& buffer);

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Timer override
    void timerCallback() override;

    // Slider listener
    void sliderValueChanged(juce::Slider* slider) override;

private:
    //==========================================================================
    void drawBPMValue(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawOnsetGraph(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawBeatIndicator(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    //==========================================================================
    BPMDetector detector;

    // Display values
    float displayBPM { 0.0f };
    float displayConfidence { 0.0f };
    bool beatFlash { false };
    int beatFlashCounter { 0 };

    // Controls
    juce::Slider minBPMSlider;
    juce::Slider maxBPMSlider;
    juce::Label minBPMLabel { {}, "Min BPM" };
    juce::Label maxBPMLabel { {}, "Max BPM" };

    // Tap tempo
    juce::TextButton tapTempoButton { "Tap" };
    std::vector<int64_t> tapTimes;
    float tapBPM { 0.0f };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BPMDisplay)
};

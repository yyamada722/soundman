/*
  ==============================================================================

    PitchDisplay.h

    Real-time pitch detection display component

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/PitchDetector.h"
#include <deque>

class PitchDisplay : public juce::Component,
                     public juce::Timer,
                     public juce::Slider::Listener
{
public:
    //==========================================================================
    PitchDisplay();
    ~PitchDisplay() override;

    //==========================================================================
    // Slider::Listener
    void sliderValueChanged(juce::Slider* slider) override;

    //==========================================================================
    // Update pitch data
    void setPitchResult(const PitchDetector::PitchResult& result);

    // Direct sample input (uses internal pitch detector)
    void pushSample(float sample);

    // Set sample rate for internal detector
    void setSampleRate(double rate);

    //==========================================================================
    // Display settings
    void setShowHistory(bool show) { showHistory = show; repaint(); }
    void setHistoryLength(int length);

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timer override
    void timerCallback() override;

private:
    //==========================================================================
    void drawBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawPitchMeter(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawNoteDisplay(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawFrequencyDisplay(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawPitchHistory(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawConfidenceMeter(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    juce::Colour getNoteColor(int midiNote) const;

    //==========================================================================
    PitchDetector pitchDetector;
    PitchDetector::PitchResult currentPitch;

    // Pitch history for visualization
    std::deque<float> pitchHistory;
    int maxHistoryLength { 200 };
    bool showHistory { true };

    // Smoothing
    float smoothedFrequency { 0.0f };
    float smoothedCents { 0.0f };
    float smoothedConfidence { 0.0f };
    static constexpr float smoothingFactor = 0.3f;

    //==========================================================================
    // Settings controls
    void setupControls();

    juce::Slider thresholdSlider;
    juce::Slider minFreqSlider;
    juce::Slider maxFreqSlider;

    juce::Label thresholdLabel { {}, "Threshold" };
    juce::Label minFreqLabel { {}, "Min Freq" };
    juce::Label maxFreqLabel { {}, "Max Freq" };

    juce::Label thresholdValueLabel;
    juce::Label minFreqValueLabel;
    juce::Label maxFreqValueLabel;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchDisplay)
};

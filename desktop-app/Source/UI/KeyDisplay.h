/*
  ==============================================================================

    KeyDisplay.h

    Musical key detection display component

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/KeyDetector.h"

class KeyDisplay : public juce::Component,
                   public juce::Timer
{
public:
    //==========================================================================
    KeyDisplay();
    ~KeyDisplay() override;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);
    void processBlock(const juce::AudioBuffer<float>& buffer);

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Timer override
    void timerCallback() override;

private:
    //==========================================================================
    void drawKeyDisplay(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawChromaDisplay(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawCircleOfFifths(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    //==========================================================================
    KeyDetector detector;

    // Display values
    KeyDetector::Key displayKey { KeyDetector::Key::Unknown };
    float displayConfidence { 0.0f };
    std::array<float, 12> displayChroma {};
    std::array<float, 24> displayCorrelations {};

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyDisplay)
};

/*
  ==============================================================================

    HistogramDisplay.h

    Audio amplitude histogram display - Optimized

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

class HistogramDisplay : public juce::Component,
                         public juce::Timer
{
public:
    //==========================================================================
    HistogramDisplay();
    ~HistogramDisplay() override;

    //==========================================================================
    // Add sample for histogram
    void pushSample(float sample);

    // Clear the histogram
    void clear();

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timer override
    void timerCallback() override;

private:
    //==========================================================================
    void drawHistogram(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    //==========================================================================
    static constexpr int NUM_BINS = 64;  // Reduced from 128
    std::vector<int> histogram;
    int maxBinValue { 1 };

    juce::CriticalSection histogramLock;

    // Decay for smooth histogram
    float decay { 0.95f };

    // Precomputed bar colors
    std::vector<juce::Colour> barColors;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HistogramDisplay)
};

/*
  ==============================================================================

    VectorscopeDisplay.h

    Vectorscope (Lissajous) display for stereo field visualization
    Optimized for performance

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

class VectorscopeDisplay : public juce::Component,
                           public juce::Timer
{
public:
    //==========================================================================
    VectorscopeDisplay();
    ~VectorscopeDisplay() override;

    //==========================================================================
    // Add stereo sample pair for display
    void pushSample(float leftSample, float rightSample);

    // Clear the display
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
    struct SamplePoint
    {
        float left;
        float right;
    };

    //==========================================================================
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawVectorscope(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    //==========================================================================
    static constexpr int MAX_POINTS = 512;  // Reduced from 2048
    static constexpr int DRAW_STEP = 2;     // Draw every Nth point
    std::vector<SamplePoint> sampleBuffer;
    int writeIndex { 0 };
    bool bufferFull { false };

    juce::CriticalSection bufferLock;

    // Cached path for faster drawing
    juce::Path cachedPath;
    bool pathNeedsUpdate { true };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorscopeDisplay)
};

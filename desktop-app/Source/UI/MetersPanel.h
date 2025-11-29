/*
  ==============================================================================

    MetersPanel.h

    Combined panel for visualization meters: Vectorscope, Histogram

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VectorscopeDisplay.h"
#include "HistogramDisplay.h"

class MetersPanel : public juce::Component
{
public:
    //==========================================================================
    MetersPanel();
    ~MetersPanel() override = default;

    //==========================================================================
    // Forward samples
    void pushSample(float sample);
    void pushStereoSample(float left, float right);

    // Access for multi-view
    VectorscopeDisplay* getVectorscopeDisplay() { return &vectorscopeDisplay; }
    HistogramDisplay* getHistogramDisplay() { return &histogramDisplay; }

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    VectorscopeDisplay vectorscopeDisplay;
    HistogramDisplay histogramDisplay;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetersPanel)
};

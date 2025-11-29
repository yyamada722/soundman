/*
  ==============================================================================

    SpectrumPanel.h

    Combined panel for Spectrum and Spectrogram displays

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SpectrumDisplay.h"
#include "SpectrogramDisplay.h"

class SpectrumPanel : public juce::Component
{
public:
    //==========================================================================
    SpectrumPanel();
    ~SpectrumPanel() override = default;

    //==========================================================================
    // Forward samples
    void pushNextSampleIntoFifo(float sample);

    // Access for multi-view
    SpectrumDisplay* getSpectrumDisplay() { return &spectrumDisplay; }

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    SpectrumDisplay spectrumDisplay;
    SpectrogramDisplay spectrogramDisplay;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumPanel)
};

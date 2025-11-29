/*
  ==============================================================================

    AnalysisPanel.h

    Combined panel for music analysis: Pitch, BPM, Key, Harmonics, MFCC

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PitchDisplay.h"
#include "BPMDisplay.h"
#include "KeyDisplay.h"
#include "HarmonicsDisplay.h"
#include "MFCCDisplay.h"

class AnalysisPanel : public juce::Component
{
public:
    //==========================================================================
    AnalysisPanel();
    ~AnalysisPanel() override = default;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);

    // Forward samples to all analyzers
    void pushSample(float sample);

    // Process audio block (for BPM and Key)
    void processBlock(const juce::AudioBuffer<float>& buffer);

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    PitchDisplay pitchDisplay;
    BPMDisplay bpmDisplay;
    KeyDisplay keyDisplay;
    HarmonicsDisplay harmonicsDisplay;
    MFCCDisplay mfccDisplay;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalysisPanel)
};

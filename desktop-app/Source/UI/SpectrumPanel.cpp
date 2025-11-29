/*
  ==============================================================================

    SpectrumPanel.cpp

    Combined panel for Spectrum and Spectrogram displays implementation

  ==============================================================================
*/

#include "SpectrumPanel.h"

//==============================================================================
SpectrumPanel::SpectrumPanel()
{
    // Setup tabbed interface
    tabs.setTabBarDepth(28);
    tabs.setOutline(0);

    // Add spectrum displays as tabs
    tabs.addTab("Spectrum", juce::Colour(0xff2a2a2a), &spectrumDisplay, false);
    tabs.addTab("Spectrogram", juce::Colour(0xff2a2a2a), &spectrogramDisplay, false);

    addAndMakeVisible(tabs);
}

//==============================================================================
void SpectrumPanel::pushNextSampleIntoFifo(float sample)
{
    spectrumDisplay.pushNextSampleIntoFifo(sample);
    spectrogramDisplay.pushNextSampleIntoFifo(sample);
}

//==============================================================================
void SpectrumPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void SpectrumPanel::resized()
{
    tabs.setBounds(getLocalBounds());
}

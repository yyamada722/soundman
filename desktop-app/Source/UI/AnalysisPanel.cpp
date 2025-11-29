/*
  ==============================================================================

    AnalysisPanel.cpp

    Combined panel for music analysis implementation

  ==============================================================================
*/

#include "AnalysisPanel.h"

//==============================================================================
AnalysisPanel::AnalysisPanel()
{
    // Setup tabbed interface
    tabs.setTabBarDepth(28);
    tabs.setOutline(0);

    // Add analysis displays as tabs
    tabs.addTab("Pitch", juce::Colour(0xff2a2a2a), &pitchDisplay, false);
    tabs.addTab("BPM", juce::Colour(0xff2a2a2a), &bpmDisplay, false);
    tabs.addTab("Key", juce::Colour(0xff2a2a2a), &keyDisplay, false);
    tabs.addTab("Harmonics", juce::Colour(0xff2a2a2a), &harmonicsDisplay, false);
    tabs.addTab("MFCC", juce::Colour(0xff2a2a2a), &mfccDisplay, false);

    addAndMakeVisible(tabs);
}

//==============================================================================
void AnalysisPanel::prepare(double sampleRate, int samplesPerBlock)
{
    // Only BPM and Key displays have prepare methods
    bpmDisplay.prepare(sampleRate, samplesPerBlock);
    keyDisplay.prepare(sampleRate, samplesPerBlock);
    // PitchDisplay, HarmonicsDisplay, MFCCDisplay don't need explicit prepare
}

void AnalysisPanel::pushSample(float sample)
{
    pitchDisplay.pushSample(sample);
    harmonicsDisplay.pushSample(sample);
    mfccDisplay.pushSample(sample);
}

void AnalysisPanel::processBlock(const juce::AudioBuffer<float>& buffer)
{
    bpmDisplay.processBlock(buffer);
    keyDisplay.processBlock(buffer);
}

//==============================================================================
void AnalysisPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void AnalysisPanel::resized()
{
    tabs.setBounds(getLocalBounds());
}

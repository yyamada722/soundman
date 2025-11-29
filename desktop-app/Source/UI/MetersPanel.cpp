/*
  ==============================================================================

    MetersPanel.cpp

    Combined panel for visualization meters implementation

  ==============================================================================
*/

#include "MetersPanel.h"

//==============================================================================
MetersPanel::MetersPanel()
{
    // Setup tabbed interface
    tabs.setTabBarDepth(28);
    tabs.setOutline(0);

    // Add meter displays as tabs
    tabs.addTab("Vectorscope", juce::Colour(0xff2a2a2a), &vectorscopeDisplay, false);
    tabs.addTab("Histogram", juce::Colour(0xff2a2a2a), &histogramDisplay, false);

    addAndMakeVisible(tabs);
}

//==============================================================================
void MetersPanel::pushSample(float sample)
{
    histogramDisplay.pushSample(sample);
}

void MetersPanel::pushStereoSample(float left, float right)
{
    vectorscopeDisplay.pushSample(left, right);
}

//==============================================================================
void MetersPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void MetersPanel::resized()
{
    tabs.setBounds(getLocalBounds());
}

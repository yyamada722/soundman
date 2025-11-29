/*
  ==============================================================================

    ToolsPanel.cpp

    Combined panel for audio tools implementation

  ==============================================================================
*/

#include "ToolsPanel.h"

//==============================================================================
ToolsPanel::ToolsPanel()
{
    // Setup tabbed interface
    tabs.setTabBarDepth(28);
    tabs.setOutline(0);

    // Add tool panels as tabs
    tabs.addTab("Filter/EQ", juce::Colour(0xff2a2a2a), &filterPanel, false);
    tabs.addTab("Generator", juce::Colour(0xff2a2a2a), &generatorPanel, false);
    tabs.addTab("IR/FR", juce::Colour(0xff2a2a2a), &responseAnalyzerPanel, false);

    addAndMakeVisible(tabs);
}

//==============================================================================
void ToolsPanel::prepare(double sampleRate, int samplesPerBlock)
{
    filterPanel.prepare(sampleRate, samplesPerBlock);
    generatorPanel.prepare(sampleRate, samplesPerBlock);
    responseAnalyzerPanel.prepare(sampleRate, samplesPerBlock);
}

//==============================================================================
void ToolsPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void ToolsPanel::resized()
{
    tabs.setBounds(getLocalBounds());
}

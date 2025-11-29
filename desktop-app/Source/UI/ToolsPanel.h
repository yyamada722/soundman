/*
  ==============================================================================

    ToolsPanel.h

    Combined panel for audio tools: Filter/EQ, Generator, IR/FR Analyzer

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "FilterPanel.h"
#include "GeneratorPanel.h"
#include "ResponseAnalyzerPanel.h"

class ToolsPanel : public juce::Component
{
public:
    //==========================================================================
    ToolsPanel();
    ~ToolsPanel() override = default;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);

    // Access to filter/EQ for audio processing
    FilterPanel& getFilterPanel() { return filterPanel; }
    GeneratorPanel& getGeneratorPanel() { return generatorPanel; }
    ResponseAnalyzerPanel& getResponseAnalyzerPanel() { return responseAnalyzerPanel; }

    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    FilterPanel filterPanel;
    GeneratorPanel generatorPanel;
    ResponseAnalyzerPanel responseAnalyzerPanel;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolsPanel)
};

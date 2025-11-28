/*
  ==============================================================================

    HarmonicsDisplay.h

    Harmonics visualization component

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/HarmonicsAnalyzer.h"

class HarmonicsDisplay : public juce::Component,
                         public juce::Timer
{
public:
    //==========================================================================
    HarmonicsDisplay();
    ~HarmonicsDisplay() override;

    //==========================================================================
    // Update harmonic data
    void setAnalysisResult(const HarmonicsAnalyzer::AnalysisResult& result);

    // Direct sample input (uses internal analyzer)
    void pushSample(float sample);

    // Set sample rate for internal analyzer
    void setSampleRate(double rate);

    //==========================================================================
    // Display settings
    void setMinDb(float db) { minDb = db; repaint(); }
    void setMaxDb(float db) { maxDb = db; repaint(); }
    void setShowGrid(bool show) { showGrid = show; repaint(); }
    void setShowLabels(bool show) { showLabels = show; repaint(); }

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timer override
    void timerCallback() override;

private:
    //==========================================================================
    void drawBackground(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawHarmonicBars(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawInfoPanel(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawHarmonicSpectrum(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    juce::Colour getHarmonicColor(int harmonicNumber) const;
    float dbToY(float db, float height) const;

    //==========================================================================
    HarmonicsAnalyzer analyzer;
    HarmonicsAnalyzer::AnalysisResult currentResult;

    // Display settings
    float minDb { -60.0f };
    float maxDb { 0.0f };
    bool showGrid { true };
    bool showLabels { true };

    // Smoothed values for display
    std::array<float, HarmonicsAnalyzer::maxHarmonics> smoothedAmplitudes;
    float smoothingFactor { 0.3f };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonicsDisplay)
};

/*
  ==============================================================================

    MFCCDisplay.h

    MFCC visualization component

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/MFCCAnalyzer.h"
#include <deque>

class MFCCDisplay : public juce::Component,
                    public juce::Timer
{
public:
    //==========================================================================
    MFCCDisplay();
    ~MFCCDisplay() override;

    //==========================================================================
    // Update MFCC data
    void setMFCCResult(const MFCCAnalyzer::MFCCResult& result);

    // Direct sample input (uses internal analyzer)
    void pushSample(float sample);

    // Set sample rate for internal analyzer
    void setSampleRate(double rate);

    //==========================================================================
    // Display settings
    void setShowHistory(bool show) { showHistory = show; repaint(); }
    void setHistoryLength(int length);

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
    void drawMFCCBars(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawMelFilterBankDisplay(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawMFCCHistory(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawInfoPanel(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    juce::Colour getMFCCColor(int index, float normalizedValue) const;
    juce::Colour getMelColor(float normalizedValue) const;

    //==========================================================================
    MFCCAnalyzer analyzer;
    MFCCAnalyzer::MFCCResult currentResult;

    // MFCC history for visualization
    std::deque<std::array<float, MFCCAnalyzer::numMFCCs>> mfccHistory;
    int maxHistoryLength { 100 };
    bool showHistory { true };

    // Smoothed values
    std::array<float, MFCCAnalyzer::numMFCCs> smoothedMFCCs;
    std::array<float, MFCCAnalyzer::numMelFilters> smoothedMelEnergies;
    float smoothingFactor { 0.3f };

    // Value ranges for normalization
    float mfccMin { -50.0f };
    float mfccMax { 50.0f };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MFCCDisplay)
};

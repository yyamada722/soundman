/*
  ==============================================================================

    ResponseAnalyzerPanel.h

    Impulse Response and Frequency Response measurement display

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/ImpulseResponseAnalyzer.h"

class ResponseAnalyzerPanel : public juce::Component,
                              public juce::Slider::Listener,
                              public juce::Button::Listener,
                              public juce::Timer
{
public:
    //==========================================================================
    ResponseAnalyzerPanel();
    ~ResponseAnalyzerPanel() override;

    //==========================================================================
    void prepare(double sampleRate, int samplesPerBlock);

    // Get analyzer for audio processing
    ImpulseResponseAnalyzer& getAnalyzer() { return analyzer; }

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Listener overrides
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

    // Timer override
    void timerCallback() override;

private:
    //==========================================================================
    void setupControls();
    void drawImpulseResponse(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawFrequencyResponse(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds, bool isFrequency);

    //==========================================================================
    ImpulseResponseAnalyzer analyzer;

    // Display mode
    enum class DisplayMode { Both, ImpulseOnly, FrequencyOnly };
    DisplayMode displayMode { DisplayMode::Both };

    // Controls
    juce::TextButton measureButton { "Start Measurement" };
    juce::ComboBox displayModeCombo;
    juce::Slider durationSlider;
    juce::Label durationLabel { {}, "Duration (s)" };
    juce::Label durationValueLabel;

    // Info labels
    juce::Label statusLabel;
    juce::Label rt60Label;
    juce::Label peakLabel;

    // Progress
    juce::ProgressBar progressBar { progress };
    double progress { 0.0 };

    double currentSampleRate { 44100.0 };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResponseAnalyzerPanel)
};

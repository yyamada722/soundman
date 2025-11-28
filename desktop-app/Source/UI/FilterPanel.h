/*
  ==============================================================================

    FilterPanel.h

    Filter and EQ control panel with frequency response display

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/AudioFilter.h"
#include <functional>

class FilterPanel : public juce::Component,
                    public juce::Slider::Listener,
                    public juce::ComboBox::Listener,
                    public juce::Button::Listener,
                    public juce::Timer
{
public:
    //==========================================================================
    FilterPanel();
    ~FilterPanel() override;

    //==========================================================================
    // Get filter/EQ references for audio processing
    AudioFilter& getFilter() { return filter; }
    ParametricEQ& getEQ() { return eq; }

    //==========================================================================
    // Prepare for playback
    void prepare(double sampleRate, int samplesPerBlock);

    //==========================================================================
    // Callbacks for parameter changes
    using FilterChangedCallback = std::function<void()>;
    void setFilterChangedCallback(FilterChangedCallback callback) { filterChangedCallback = callback; }

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Listener overrides
    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void buttonClicked(juce::Button* button) override;

    //==========================================================================
    // Timer override
    void timerCallback() override;

private:
    //==========================================================================
    void setupFilterControls();
    void setupEQControls();
    void updateFilterFromControls();
    void updateEQFromControls();

    void drawFrequencyResponse(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    float getXForFrequency(float freq, float width) const;
    float getYForGain(float gainDb, float height) const;

    //==========================================================================
    AudioFilter filter;
    ParametricEQ eq;

    // Filter controls
    juce::ToggleButton filterEnableButton { "Filter" };
    juce::ComboBox filterTypeCombo;
    juce::Slider filterFreqSlider;
    juce::Slider filterQSlider;
    juce::Label filterFreqLabel { {}, "Freq" };
    juce::Label filterQLabel { {}, "Q" };

    // EQ controls
    juce::ToggleButton eqEnableButton { "EQ" };

    // EQ Band controls
    struct BandControls
    {
        juce::ToggleButton enableButton;
        juce::Slider freqSlider;
        juce::Slider gainSlider;
        juce::Slider qSlider;
        juce::Label freqLabel;
        juce::Label gainLabel;
        juce::Label qLabel;
    };
    std::array<BandControls, 3> bandControls;

    // Frequency response display area
    juce::Rectangle<int> responseArea;

    FilterChangedCallback filterChangedCallback;

    double currentSampleRate { 44100.0 };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterPanel)
};

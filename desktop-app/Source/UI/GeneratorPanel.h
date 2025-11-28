/*
  ==============================================================================

    GeneratorPanel.h

    Signal generator and THD measurement panel

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/SignalGenerator.h"
#include "../DSP/THDAnalyzer.h"

class GeneratorPanel : public juce::Component,
                       public juce::Slider::Listener,
                       public juce::ComboBox::Listener,
                       public juce::Button::Listener,
                       public juce::Timer
{
public:
    //==========================================================================
    GeneratorPanel();
    ~GeneratorPanel() override;

    //==========================================================================
    // Prepare generators
    void prepare(double sampleRate, int samplesPerBlock);

    // Process audio (call from audio callback)
    void processAudio(juce::AudioBuffer<float>& buffer);

    // Push sample for THD analysis
    void pushSampleForAnalysis(float sample);

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Listener overrides
    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void buttonClicked(juce::Button* button) override;

    // Timer override
    void timerCallback() override;

private:
    //==========================================================================
    void setupToneControls();
    void setupNoiseControls();
    void setupSweepControls();
    void setupTHDDisplay();

    void updateTHDDisplay();

    //==========================================================================
    ToneGenerator toneGenerator;
    NoiseGenerator noiseGenerator;
    SweepGenerator sweepGenerator;
    THDAnalyzer thdAnalyzer;

    // Tone Generator Controls
    juce::GroupComponent toneGroup { {}, "Tone Generator" };
    juce::ToggleButton toneEnableButton { "Enable" };
    juce::ComboBox toneWaveformCombo;
    juce::Slider toneFreqSlider;
    juce::Slider toneAmpSlider;
    juce::Label toneFreqLabel { {}, "Freq (Hz)" };
    juce::Label toneAmpLabel { {}, "Level (dB)" };
    juce::Label toneFreqValueLabel;
    juce::Label toneAmpValueLabel;

    // Noise Generator Controls
    juce::GroupComponent noiseGroup { {}, "Noise Generator" };
    juce::ToggleButton noiseEnableButton { "Enable" };
    juce::ComboBox noiseTypeCombo;
    juce::Slider noiseAmpSlider;
    juce::Label noiseAmpLabel { {}, "Level (dB)" };
    juce::Label noiseAmpValueLabel;

    // Sweep Generator Controls
    juce::GroupComponent sweepGroup { {}, "Sweep Generator" };
    juce::ToggleButton sweepEnableButton { "Start Sweep" };
    juce::ComboBox sweepTypeCombo;
    juce::Slider sweepStartFreqSlider;
    juce::Slider sweepEndFreqSlider;
    juce::Slider sweepDurationSlider;
    juce::Slider sweepAmpSlider;
    juce::Label sweepStartLabel { {}, "Start (Hz)" };
    juce::Label sweepEndLabel { {}, "End (Hz)" };
    juce::Label sweepDurationLabel { {}, "Duration (s)" };
    juce::Label sweepAmpLabel { {}, "Level (dB)" };
    juce::Label sweepProgressLabel;

    // THD Display
    juce::GroupComponent thdGroup { {}, "THD Measurement" };
    juce::Label thdValueLabel;
    juce::Label thdNValueLabel;
    juce::Label snrValueLabel;
    juce::Label sinadValueLabel;
    juce::Label fundamentalLabel;

    double currentSampleRate { 44100.0 };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneratorPanel)
};

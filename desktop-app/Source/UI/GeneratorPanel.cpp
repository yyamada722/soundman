/*
  ==============================================================================

    GeneratorPanel.cpp

    Signal generator and THD measurement panel implementation

  ==============================================================================
*/

#include "GeneratorPanel.h"

//==============================================================================
GeneratorPanel::GeneratorPanel()
{
    setupToneControls();
    setupNoiseControls();
    setupSweepControls();
    setupTHDDisplay();

    // Setup sweep complete callback
    sweepGenerator.onSweepComplete = [this]()
    {
        sweepEnableButton.setToggleState(false, juce::dontSendNotification);
        sweepEnableButton.setButtonText("Start Sweep");
    };

    startTimerHz(10);
}

GeneratorPanel::~GeneratorPanel()
{
    stopTimer();
}

//==============================================================================
void GeneratorPanel::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    toneGenerator.prepare(sampleRate, samplesPerBlock);
    noiseGenerator.prepare(sampleRate, samplesPerBlock);
    sweepGenerator.prepare(sampleRate, samplesPerBlock);
    thdAnalyzer.prepare(sampleRate, samplesPerBlock);
}

void GeneratorPanel::processAudio(juce::AudioBuffer<float>& buffer)
{
    toneGenerator.process(buffer);
    noiseGenerator.process(buffer);
    sweepGenerator.process(buffer);
}

void GeneratorPanel::pushSampleForAnalysis(float sample)
{
    thdAnalyzer.pushSample(sample);
}

//==============================================================================
void GeneratorPanel::setupToneControls()
{
    addAndMakeVisible(toneGroup);

    // Enable button
    toneEnableButton.addListener(this);
    addAndMakeVisible(toneEnableButton);

    // Waveform combo
    toneWaveformCombo.addItem("Sine", 1);
    toneWaveformCombo.addItem("Square", 2);
    toneWaveformCombo.addItem("Triangle", 3);
    toneWaveformCombo.addItem("Sawtooth", 4);
    toneWaveformCombo.setSelectedId(1);
    toneWaveformCombo.addListener(this);
    addAndMakeVisible(toneWaveformCombo);

    // Frequency slider
    toneFreqSlider.setRange(20, 20000, 1);
    toneFreqSlider.setValue(1000);
    toneFreqSlider.setSkewFactorFromMidPoint(1000);
    toneFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    toneFreqSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    toneFreqSlider.addListener(this);
    addAndMakeVisible(toneFreqSlider);

    toneFreqLabel.setFont(juce::Font(11.0f));
    toneFreqLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(toneFreqLabel);

    toneFreqValueLabel.setFont(juce::Font(11.0f));
    toneFreqValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    toneFreqValueLabel.setText("1000 Hz", juce::dontSendNotification);
    addAndMakeVisible(toneFreqValueLabel);

    // Amplitude slider (in dB)
    toneAmpSlider.setRange(-60, 0, 0.1);
    toneAmpSlider.setValue(-6);
    toneAmpSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    toneAmpSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    toneAmpSlider.addListener(this);
    addAndMakeVisible(toneAmpSlider);

    toneAmpLabel.setFont(juce::Font(11.0f));
    toneAmpLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(toneAmpLabel);

    toneAmpValueLabel.setFont(juce::Font(11.0f));
    toneAmpValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    toneAmpValueLabel.setText("-6 dB", juce::dontSendNotification);
    addAndMakeVisible(toneAmpValueLabel);

    // Initialize generator
    toneGenerator.setFrequency(1000.0f);
    toneGenerator.setAmplitude(0.5f);
    thdAnalyzer.setExpectedFundamental(1000.0f);
}

void GeneratorPanel::setupNoiseControls()
{
    addAndMakeVisible(noiseGroup);

    // Enable button
    noiseEnableButton.addListener(this);
    addAndMakeVisible(noiseEnableButton);

    // Noise type combo
    noiseTypeCombo.addItem("White", 1);
    noiseTypeCombo.addItem("Pink", 2);
    noiseTypeCombo.addItem("Brown", 3);
    noiseTypeCombo.setSelectedId(1);
    noiseTypeCombo.addListener(this);
    addAndMakeVisible(noiseTypeCombo);

    // Amplitude slider
    noiseAmpSlider.setRange(-60, 0, 0.1);
    noiseAmpSlider.setValue(-12);
    noiseAmpSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    noiseAmpSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    noiseAmpSlider.addListener(this);
    addAndMakeVisible(noiseAmpSlider);

    noiseAmpLabel.setFont(juce::Font(11.0f));
    noiseAmpLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(noiseAmpLabel);

    noiseAmpValueLabel.setFont(juce::Font(11.0f));
    noiseAmpValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    noiseAmpValueLabel.setText("-12 dB", juce::dontSendNotification);
    addAndMakeVisible(noiseAmpValueLabel);

    noiseGenerator.setAmplitude(0.25f);
}

void GeneratorPanel::setupSweepControls()
{
    addAndMakeVisible(sweepGroup);

    // Enable/Start button
    sweepEnableButton.addListener(this);
    addAndMakeVisible(sweepEnableButton);

    // Sweep type combo
    sweepTypeCombo.addItem("Logarithmic", 1);
    sweepTypeCombo.addItem("Linear", 2);
    sweepTypeCombo.setSelectedId(1);
    sweepTypeCombo.addListener(this);
    addAndMakeVisible(sweepTypeCombo);

    // Start frequency
    sweepStartFreqSlider.setRange(20, 1000, 1);
    sweepStartFreqSlider.setValue(20);
    sweepStartFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    sweepStartFreqSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sweepStartFreqSlider.addListener(this);
    addAndMakeVisible(sweepStartFreqSlider);

    sweepStartLabel.setFont(juce::Font(11.0f));
    sweepStartLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(sweepStartLabel);

    // End frequency
    sweepEndFreqSlider.setRange(1000, 20000, 10);
    sweepEndFreqSlider.setValue(20000);
    sweepEndFreqSlider.setSkewFactorFromMidPoint(5000);
    sweepEndFreqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    sweepEndFreqSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sweepEndFreqSlider.addListener(this);
    addAndMakeVisible(sweepEndFreqSlider);

    sweepEndLabel.setFont(juce::Font(11.0f));
    sweepEndLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(sweepEndLabel);

    // Duration
    sweepDurationSlider.setRange(1, 60, 1);
    sweepDurationSlider.setValue(10);
    sweepDurationSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    sweepDurationSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sweepDurationSlider.addListener(this);
    addAndMakeVisible(sweepDurationSlider);

    sweepDurationLabel.setFont(juce::Font(11.0f));
    sweepDurationLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(sweepDurationLabel);

    // Amplitude
    sweepAmpSlider.setRange(-60, 0, 0.1);
    sweepAmpSlider.setValue(-6);
    sweepAmpSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    sweepAmpSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sweepAmpSlider.addListener(this);
    addAndMakeVisible(sweepAmpSlider);

    sweepAmpLabel.setFont(juce::Font(11.0f));
    sweepAmpLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(sweepAmpLabel);

    // Progress label
    sweepProgressLabel.setFont(juce::Font(11.0f));
    sweepProgressLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    addAndMakeVisible(sweepProgressLabel);

    sweepGenerator.setStartFrequency(20.0f);
    sweepGenerator.setEndFrequency(20000.0f);
    sweepGenerator.setDuration(10.0f);
    sweepGenerator.setAmplitude(0.5f);
}

void GeneratorPanel::setupTHDDisplay()
{
    addAndMakeVisible(thdGroup);

    auto setupLabel = [this](juce::Label& label, const juce::String& text)
    {
        label.setFont(juce::Font(12.0f));
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setText(text, juce::dontSendNotification);
        addAndMakeVisible(label);
    };

    setupLabel(fundamentalLabel, "Fundamental: --- Hz");
    setupLabel(thdValueLabel, "THD: --- %");
    setupLabel(thdNValueLabel, "THD+N: --- %");
    setupLabel(snrValueLabel, "SNR: --- dB");
    setupLabel(sinadValueLabel, "SINAD: --- dB");
}

//==============================================================================
void GeneratorPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void GeneratorPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    int groupHeight = 130;
    int rowHeight = 22;
    int labelWidth = 70;
    int valueWidth = 60;
    int margin = 5;

    // Tone Generator Group
    auto toneArea = bounds.removeFromTop(groupHeight);
    toneGroup.setBounds(toneArea);
    auto toneContent = toneArea.reduced(10, 20);

    auto row = toneContent.removeFromTop(rowHeight);
    toneEnableButton.setBounds(row.removeFromLeft(70));
    row.removeFromLeft(margin);
    toneWaveformCombo.setBounds(row.removeFromLeft(100));

    toneContent.removeFromTop(margin);
    row = toneContent.removeFromTop(rowHeight);
    toneFreqLabel.setBounds(row.removeFromLeft(labelWidth));
    toneFreqValueLabel.setBounds(row.removeFromRight(valueWidth));
    row.removeFromLeft(margin);
    row.removeFromRight(margin);
    toneFreqSlider.setBounds(row);

    toneContent.removeFromTop(margin);
    row = toneContent.removeFromTop(rowHeight);
    toneAmpLabel.setBounds(row.removeFromLeft(labelWidth));
    toneAmpValueLabel.setBounds(row.removeFromRight(valueWidth));
    row.removeFromLeft(margin);
    row.removeFromRight(margin);
    toneAmpSlider.setBounds(row);

    bounds.removeFromTop(margin);

    // Noise Generator Group
    auto noiseArea = bounds.removeFromTop(100);
    noiseGroup.setBounds(noiseArea);
    auto noiseContent = noiseArea.reduced(10, 20);

    row = noiseContent.removeFromTop(rowHeight);
    noiseEnableButton.setBounds(row.removeFromLeft(70));
    row.removeFromLeft(margin);
    noiseTypeCombo.setBounds(row.removeFromLeft(100));

    noiseContent.removeFromTop(margin);
    row = noiseContent.removeFromTop(rowHeight);
    noiseAmpLabel.setBounds(row.removeFromLeft(labelWidth));
    noiseAmpValueLabel.setBounds(row.removeFromRight(valueWidth));
    row.removeFromLeft(margin);
    row.removeFromRight(margin);
    noiseAmpSlider.setBounds(row);

    bounds.removeFromTop(margin);

    // Sweep Generator Group
    auto sweepArea = bounds.removeFromTop(160);
    sweepGroup.setBounds(sweepArea);
    auto sweepContent = sweepArea.reduced(10, 20);

    row = sweepContent.removeFromTop(rowHeight);
    sweepEnableButton.setBounds(row.removeFromLeft(100));
    row.removeFromLeft(margin);
    sweepTypeCombo.setBounds(row.removeFromLeft(100));
    row.removeFromLeft(margin);
    sweepProgressLabel.setBounds(row);

    sweepContent.removeFromTop(margin);
    row = sweepContent.removeFromTop(rowHeight);
    sweepStartLabel.setBounds(row.removeFromLeft(labelWidth));
    sweepStartFreqSlider.setBounds(row);

    sweepContent.removeFromTop(margin);
    row = sweepContent.removeFromTop(rowHeight);
    sweepEndLabel.setBounds(row.removeFromLeft(labelWidth));
    sweepEndFreqSlider.setBounds(row);

    sweepContent.removeFromTop(margin);
    row = sweepContent.removeFromTop(rowHeight);
    sweepDurationLabel.setBounds(row.removeFromLeft(labelWidth));
    sweepDurationSlider.setBounds(row);

    sweepContent.removeFromTop(margin);
    row = sweepContent.removeFromTop(rowHeight);
    sweepAmpLabel.setBounds(row.removeFromLeft(labelWidth));
    sweepAmpSlider.setBounds(row);

    bounds.removeFromTop(margin);

    // THD Display Group
    auto thdArea = bounds;
    thdGroup.setBounds(thdArea);
    auto thdContent = thdArea.reduced(10, 20);

    fundamentalLabel.setBounds(thdContent.removeFromTop(rowHeight));
    thdContent.removeFromTop(margin);
    thdValueLabel.setBounds(thdContent.removeFromTop(rowHeight));
    thdContent.removeFromTop(margin);
    thdNValueLabel.setBounds(thdContent.removeFromTop(rowHeight));
    thdContent.removeFromTop(margin);
    snrValueLabel.setBounds(thdContent.removeFromTop(rowHeight));
    thdContent.removeFromTop(margin);
    sinadValueLabel.setBounds(thdContent.removeFromTop(rowHeight));
}

//==============================================================================
void GeneratorPanel::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &toneFreqSlider)
    {
        float freq = (float)toneFreqSlider.getValue();
        toneGenerator.setFrequency(freq);
        thdAnalyzer.setExpectedFundamental(freq);
        toneFreqValueLabel.setText(juce::String((int)freq) + " Hz", juce::dontSendNotification);
    }
    else if (slider == &toneAmpSlider)
    {
        float dB = (float)toneAmpSlider.getValue();
        float linear = std::pow(10.0f, dB / 20.0f);
        toneGenerator.setAmplitude(linear);
        toneAmpValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
    }
    else if (slider == &noiseAmpSlider)
    {
        float dB = (float)noiseAmpSlider.getValue();
        float linear = std::pow(10.0f, dB / 20.0f);
        noiseGenerator.setAmplitude(linear);
        noiseAmpValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
    }
    else if (slider == &sweepStartFreqSlider)
    {
        sweepGenerator.setStartFrequency((float)sweepStartFreqSlider.getValue());
    }
    else if (slider == &sweepEndFreqSlider)
    {
        sweepGenerator.setEndFrequency((float)sweepEndFreqSlider.getValue());
    }
    else if (slider == &sweepDurationSlider)
    {
        sweepGenerator.setDuration((float)sweepDurationSlider.getValue());
    }
    else if (slider == &sweepAmpSlider)
    {
        float dB = (float)sweepAmpSlider.getValue();
        float linear = std::pow(10.0f, dB / 20.0f);
        sweepGenerator.setAmplitude(linear);
    }
}

void GeneratorPanel::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &toneWaveformCombo)
    {
        int id = toneWaveformCombo.getSelectedId();
        switch (id)
        {
            case 1: toneGenerator.setWaveform(ToneGenerator::Waveform::Sine); break;
            case 2: toneGenerator.setWaveform(ToneGenerator::Waveform::Square); break;
            case 3: toneGenerator.setWaveform(ToneGenerator::Waveform::Triangle); break;
            case 4: toneGenerator.setWaveform(ToneGenerator::Waveform::Sawtooth); break;
        }
    }
    else if (comboBox == &noiseTypeCombo)
    {
        int id = noiseTypeCombo.getSelectedId();
        switch (id)
        {
            case 1: noiseGenerator.setNoiseType(NoiseGenerator::NoiseType::White); break;
            case 2: noiseGenerator.setNoiseType(NoiseGenerator::NoiseType::Pink); break;
            case 3: noiseGenerator.setNoiseType(NoiseGenerator::NoiseType::Brown); break;
        }
    }
    else if (comboBox == &sweepTypeCombo)
    {
        int id = sweepTypeCombo.getSelectedId();
        sweepGenerator.setSweepType(id == 1 ? SweepGenerator::SweepType::Logarithmic
                                            : SweepGenerator::SweepType::Linear);
    }
}

void GeneratorPanel::buttonClicked(juce::Button* button)
{
    if (button == &toneEnableButton)
    {
        toneGenerator.setEnabled(toneEnableButton.getToggleState());
    }
    else if (button == &noiseEnableButton)
    {
        noiseGenerator.setEnabled(noiseEnableButton.getToggleState());
    }
    else if (button == &sweepEnableButton)
    {
        bool enable = sweepEnableButton.getToggleState();
        sweepGenerator.setEnabled(enable);
        sweepEnableButton.setButtonText(enable ? "Stop Sweep" : "Start Sweep");
    }
}

void GeneratorPanel::timerCallback()
{
    updateTHDDisplay();

    // Update sweep progress
    if (sweepGenerator.isGenerating())
    {
        float progress = sweepGenerator.getProgress() * 100.0f;
        float freq = sweepGenerator.getCurrentFrequency();
        sweepProgressLabel.setText(juce::String((int)progress) + "% - " +
                                   juce::String((int)freq) + " Hz",
                                   juce::dontSendNotification);
    }
    else
    {
        sweepProgressLabel.setText("", juce::dontSendNotification);
    }
}

void GeneratorPanel::updateTHDDisplay()
{
    auto result = thdAnalyzer.getResult();

    if (result.isValid)
    {
        fundamentalLabel.setText("Fundamental: " + juce::String(result.fundamentalFrequency, 1) + " Hz (" +
                                 juce::String(result.fundamentalAmplitude, 1) + " dB)",
                                 juce::dontSendNotification);
        thdValueLabel.setText("THD: " + juce::String(result.thd, 3) + " %",
                              juce::dontSendNotification);
        thdNValueLabel.setText("THD+N: " + juce::String(result.thdPlusNoise, 3) + " %",
                               juce::dontSendNotification);
        snrValueLabel.setText("SNR: " + juce::String(result.snr, 1) + " dB",
                              juce::dontSendNotification);
        sinadValueLabel.setText("SINAD: " + juce::String(result.sinad, 1) + " dB",
                                juce::dontSendNotification);
    }
    else
    {
        fundamentalLabel.setText("Fundamental: --- Hz", juce::dontSendNotification);
        thdValueLabel.setText("THD: --- %", juce::dontSendNotification);
        thdNValueLabel.setText("THD+N: --- %", juce::dontSendNotification);
        snrValueLabel.setText("SNR: --- dB", juce::dontSendNotification);
        sinadValueLabel.setText("SINAD: --- dB", juce::dontSendNotification);
    }
}

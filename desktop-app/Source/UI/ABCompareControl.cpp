/*
  ==============================================================================

    ABCompareControl.cpp

    A/B comparison control implementation

  ==============================================================================
*/

#include "ABCompareControl.h"

ABCompareControl::ABCompareControl()
{
    // Title
    addAndMakeVisible(titleLabel);
    titleLabel.setText("A/B COMPARE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    // A button (Original/Dry)
    addAndMakeVisible(buttonA);
    buttonA.setColour(juce::TextButton::buttonColourId, inactiveColor);
    buttonA.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    buttonA.setTooltip("Original audio (Dry)");
    buttonA.onClick = [this]()
    {
        setMode(CompareMode::A_Original);
    };

    // B button (Processed/Wet)
    addAndMakeVisible(buttonB);
    buttonB.setColour(juce::TextButton::buttonColourId, activeColor);
    buttonB.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    buttonB.setTooltip("Processed audio (Wet)");
    buttonB.onClick = [this]()
    {
        setMode(CompareMode::B_Processed);
    };

    // Mix button
    addAndMakeVisible(buttonMix);
    buttonMix.setColour(juce::TextButton::buttonColourId, inactiveColor);
    buttonMix.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    buttonMix.setTooltip("Blend between A and B");
    buttonMix.onClick = [this]()
    {
        setMode(CompareMode::Mix);
    };

    // Mix slider
    addAndMakeVisible(mixSlider);
    mixSlider.setRange(0.0, 1.0, 0.01);
    mixSlider.setValue(1.0);
    mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    mixSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff4a90e2));
    mixSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff2a4a6a));
    mixSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff1a1a1a));
    mixSlider.onValueChange = [this]()
    {
        mixAmount = static_cast<float>(mixSlider.getValue());
        updateMixLabel();

        if (onMixChanged)
            onMixChanged(mixAmount);
    };

    // Mix percentage label
    addAndMakeVisible(mixLabel);
    mixLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    // Dry/Wet labels
    addAndMakeVisible(dryLabel);
    dryLabel.setFont(juce::Font(10.0f));
    dryLabel.setJustificationType(juce::Justification::centredLeft);
    dryLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    addAndMakeVisible(wetLabel);
    wetLabel.setFont(juce::Font(10.0f));
    wetLabel.setJustificationType(juce::Justification::centredRight);
    wetLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    updateButtonStates();
    updateMixLabel();
}

ABCompareControl::~ABCompareControl()
{
}

void ABCompareControl::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    g.setColour(juce::Colour(0xff404040));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 1.0f);

    // Mode indicator line
    auto indicatorArea = bounds.reduced(10).removeFromBottom(4);
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRoundedRectangle(indicatorArea.toFloat(), 2.0f);

    // Active indicator
    g.setColour(activeColor);
    float indicatorWidth = indicatorArea.getWidth() / 3.0f;
    float indicatorX = indicatorArea.getX();

    switch (currentMode)
    {
        case CompareMode::A_Original:
            indicatorX = indicatorArea.getX();
            break;
        case CompareMode::B_Processed:
            indicatorX = indicatorArea.getX() + indicatorWidth;
            break;
        case CompareMode::Mix:
            indicatorX = indicatorArea.getX() + indicatorWidth * 2;
            break;
    }

    g.fillRoundedRectangle(indicatorX, (float)indicatorArea.getY(),
                           indicatorWidth, (float)indicatorArea.getHeight(), 2.0f);
}

void ABCompareControl::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    titleLabel.setBounds(bounds.removeFromTop(18));
    bounds.removeFromTop(5);

    // Button row
    auto buttonRow = bounds.removeFromTop(28);
    int buttonWidth = buttonRow.getWidth() / 3;
    buttonA.setBounds(buttonRow.removeFromLeft(buttonWidth).reduced(2));
    buttonB.setBounds(buttonRow.removeFromLeft(buttonWidth).reduced(2));
    buttonMix.setBounds(buttonRow.reduced(2));

    bounds.removeFromTop(8);

    // Mix label (percentage)
    mixLabel.setBounds(bounds.removeFromTop(20));

    bounds.removeFromTop(2);

    // Dry/Wet labels row
    auto labelRow = bounds.removeFromTop(14);
    dryLabel.setBounds(labelRow.removeFromLeft(30));
    wetLabel.setBounds(labelRow.removeFromRight(30));

    // Mix slider
    mixSlider.setBounds(bounds.removeFromTop(20));

    // Bottom indicator takes remaining space (drawn in paint)
}

void ABCompareControl::setMode(CompareMode mode)
{
    if (currentMode != mode)
    {
        currentMode = mode;
        updateButtonStates();
        repaint();

        if (onModeChanged)
            onModeChanged(currentMode);

        // Update mix based on mode
        if (mode == CompareMode::A_Original)
        {
            if (onMixChanged)
                onMixChanged(0.0f);
        }
        else if (mode == CompareMode::B_Processed)
        {
            if (onMixChanged)
                onMixChanged(1.0f);
        }
        else // Mix mode
        {
            if (onMixChanged)
                onMixChanged(mixAmount);
        }
    }
}

void ABCompareControl::setMixAmount(float amount)
{
    mixAmount = juce::jlimit(0.0f, 1.0f, amount);
    mixSlider.setValue(mixAmount, juce::dontSendNotification);
    updateMixLabel();
}

void ABCompareControl::toggleAB()
{
    if (currentMode == CompareMode::A_Original)
        setMode(CompareMode::B_Processed);
    else
        setMode(CompareMode::A_Original);
}

void ABCompareControl::updateButtonStates()
{
    buttonA.setColour(juce::TextButton::buttonColourId,
                      currentMode == CompareMode::A_Original ? activeColor : inactiveColor);
    buttonB.setColour(juce::TextButton::buttonColourId,
                      currentMode == CompareMode::B_Processed ? activeColor : inactiveColor);
    buttonMix.setColour(juce::TextButton::buttonColourId,
                        currentMode == CompareMode::Mix ? activeColor : inactiveColor);

    // Enable/disable slider based on mode
    mixSlider.setEnabled(currentMode == CompareMode::Mix);
    mixSlider.setAlpha(currentMode == CompareMode::Mix ? 1.0f : 0.5f);
}

void ABCompareControl::updateMixLabel()
{
    int dryPercent = static_cast<int>((1.0f - mixAmount) * 100);
    int wetPercent = static_cast<int>(mixAmount * 100);

    if (currentMode == CompareMode::A_Original)
        mixLabel.setText("100% Dry", juce::dontSendNotification);
    else if (currentMode == CompareMode::B_Processed)
        mixLabel.setText("100% Wet", juce::dontSendNotification);
    else
        mixLabel.setText(juce::String(dryPercent) + "% / " + juce::String(wetPercent) + "%",
                        juce::dontSendNotification);
}

#include "MasterGainControl.h"

MasterGainControl::MasterGainControl()
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText("MASTER GAIN", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(gainSlider);
    gainSlider.setRange(-60.0, 12.0, 0.5);  // Larger step for smoother control
    gainSlider.setValue(0.0);
    gainSlider.setSliderStyle(juce::Slider::LinearVertical);
    gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gainSlider.setVelocityBasedMode(true);
    gainSlider.setVelocityModeParameters(0.5, 1, 0.1, false);  // Smoother velocity
    gainSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff4a90e2));
    gainSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff2a4a6a));
    gainSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff1a1a1a));
    gainSlider.onValueChange = [this]()
    {
        currentGainDB = static_cast<float>(gainSlider.getValue());
        updateGainLabel();

        if (onGainChanged)
            onGainChanged(getGainLinear());
    };

    addAndMakeVisible(gainLabel);
    gainLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    gainLabel.setJustificationType(juce::Justification::centred);
    gainLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    updateGainLabel();

    addAndMakeVisible(resetButton);
    resetButton.setButtonText("0dB");
    resetButton.onClick = [this]()
    {
        gainSlider.setValue(0.0);
    };
}

MasterGainControl::~MasterGainControl()
{
}

void MasterGainControl::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));

    auto bounds = getLocalBounds().reduced(5);

    // Draw border
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(bounds, 2);

    // Draw scale markings
    auto sliderBounds = bounds.reduced(10);
    sliderBounds.removeFromTop(25); // Title
    sliderBounds.removeFromBottom(60); // Label and button

    g.setColour(juce::Colour(0xff606060));
    g.setFont(10.0f);

    // Draw dB markings
    float dbValues[] = { 12.0f, 6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -48.0f, -60.0f };
    for (float db : dbValues)
    {
        float proportion = (db - (-60.0f)) / (12.0f - (-60.0f));
        int y = sliderBounds.getBottom() - static_cast<int>(proportion * sliderBounds.getHeight());

        g.drawLine(sliderBounds.getX(), static_cast<float>(y),
                   sliderBounds.getX() + 5, static_cast<float>(y));

        juce::String label = (db > 0 ? "+" : "") + juce::String(static_cast<int>(db));
        g.drawText(label, sliderBounds.getX() - 30, y - 6, 25, 12,
                   juce::Justification::centredRight);

        // Highlight 0dB line
        if (db == 0.0f)
        {
            g.setColour(juce::Colour(0xff808080));
            g.drawLine(sliderBounds.getX(), static_cast<float>(y),
                      sliderBounds.getRight(), static_cast<float>(y), 1.5f);
            g.setColour(juce::Colour(0xff606060));
        }
    }
}

void MasterGainControl::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    titleLabel.setBounds(bounds.removeFromTop(25));

    auto bottomArea = bounds.removeFromBottom(60);
    gainLabel.setBounds(bottomArea.removeFromTop(30));
    resetButton.setBounds(bottomArea.reduced(20, 5));

    gainSlider.setBounds(bounds.reduced(20, 10));
}

float MasterGainControl::getGainLinear() const
{
    return juce::Decibels::decibelsToGain(currentGainDB);
}

float MasterGainControl::getGainDecibels() const
{
    return currentGainDB;
}

void MasterGainControl::setGainDecibels(float db)
{
    gainSlider.setValue(db);
}

void MasterGainControl::updateGainLabel()
{
    juce::String text;
    if (currentGainDB > 0)
        text = "+" + juce::String(currentGainDB, 1) + " dB";
    else
        text = juce::String(currentGainDB, 1) + " dB";

    gainLabel.setText(text, juce::dontSendNotification);

    // Color code the label
    if (currentGainDB > 6.0f)
        gainLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    else if (currentGainDB > 0.0f)
        gainLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    else if (currentGainDB > -12.0f)
        gainLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    else
        gainLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
}

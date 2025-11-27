#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>

class MasterGainControl : public juce::Component
{
public:
    MasterGainControl();
    ~MasterGainControl() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Gain control
    float getGainLinear() const;
    float getGainDecibels() const;
    void setGainDecibels(float db);

    // Callback when gain changes
    std::function<void(float)> onGainChanged;

private:
    void updateGainLabel();

    juce::Slider gainSlider;
    juce::Label gainLabel;
    juce::Label titleLabel;
    juce::TextButton resetButton;

    float currentGainDB { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterGainControl)
};

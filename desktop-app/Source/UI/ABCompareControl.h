/*
  ==============================================================================

    ABCompareControl.h

    A/B comparison control for dry/wet audio switching

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class ABCompareControl : public juce::Component
{
public:
    //==========================================================================
    enum class CompareMode
    {
        A_Original,  // Dry - original audio only
        B_Processed, // Wet - processed audio only
        Mix          // Blend between A and B
    };

    //==========================================================================
    ABCompareControl();
    ~ABCompareControl() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // State control
    CompareMode getMode() const { return currentMode; }
    void setMode(CompareMode mode);

    float getMixAmount() const { return mixAmount; }  // 0.0 = A, 1.0 = B
    void setMixAmount(float amount);

    // Quick toggle between A and B
    void toggleAB();

    //==========================================================================
    // Callbacks
    std::function<void(CompareMode)> onModeChanged;
    std::function<void(float)> onMixChanged;  // 0.0 = dry, 1.0 = wet

private:
    void updateButtonStates();
    void updateMixLabel();

    CompareMode currentMode { CompareMode::B_Processed };
    float mixAmount { 1.0f };  // Default to fully wet

    juce::Label titleLabel;
    juce::TextButton buttonA { "A" };
    juce::TextButton buttonB { "B" };
    juce::TextButton buttonMix { "Mix" };

    juce::Slider mixSlider;
    juce::Label mixLabel;
    juce::Label dryLabel { {}, "Dry" };
    juce::Label wetLabel { {}, "Wet" };

    juce::Colour activeColor { 0xff4a90e2 };
    juce::Colour inactiveColor { 0xff3a3a3a };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ABCompareControl)
};

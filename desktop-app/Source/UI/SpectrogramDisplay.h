/*
  ==============================================================================

    SpectrogramDisplay.h

    Real-time spectrogram (waterfall) display component

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

class SpectrogramDisplay : public juce::Component,
                           public juce::Timer
{
public:
    //==========================================================================
    enum class ColorMap
    {
        Viridis,
        Plasma,
        Inferno,
        Magma,
        Grayscale,
        Jet,
        Hot
    };

    //==========================================================================
    SpectrogramDisplay();
    ~SpectrogramDisplay() override;

    //==========================================================================
    // FFT Configuration
    static constexpr int fftOrder = 11;  // 2^11 = 2048 samples (good balance)
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int historySize = 512;  // Number of time slices to display

    //==========================================================================
    // Update spectrum data (call from audio thread)
    void pushNextSampleIntoFifo(float sample);

    //==========================================================================
    // Display settings
    void setMinFrequency(float freq) { minFrequency = freq; repaint(); }
    void setMaxFrequency(float freq) { maxFrequency = freq; repaint(); }
    void setMinDecibels(float db) { minDecibels = db; repaint(); }
    void setMaxDecibels(float db) { maxDecibels = db; repaint(); }
    void setSampleRate(double rate) { sampleRate = rate; }

    float getMinFrequency() const { return minFrequency; }
    float getMaxFrequency() const { return maxFrequency; }
    float getMinDecibels() const { return minDecibels; }
    float getMaxDecibels() const { return maxDecibels; }

    //==========================================================================
    // Colormap settings
    void setColorMap(ColorMap map);
    ColorMap getColorMap() const { return currentColorMap; }
    static juce::StringArray getColorMapNames();

    //==========================================================================
    // Scroll direction
    enum class ScrollDirection { Up, Down, Left, Right };
    void setScrollDirection(ScrollDirection dir) { scrollDirection = dir; }
    ScrollDirection getScrollDirection() const { return scrollDirection; }

    //==========================================================================
    // Clear the spectrogram
    void clear();

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timer override (for display updates)
    void timerCallback() override;

private:
    //==========================================================================
    void drawSpectrogram(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawFrequencyLabels(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawTimeAxis(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawColorBar(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    juce::Colour getColorForValue(float normalizedValue) const;
    float getYForFrequency(float freq, float height) const;
    float getFrequencyForY(float y, float height) const;

    void updateSpectrogramImage();

    //==========================================================================
    // FFT Processing
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    std::vector<float> fifo;
    std::vector<float> fftData;
    int fifoIndex { 0 };
    bool nextFFTBlockReady { false };

    //==========================================================================
    // Spectrogram data (2D array: time x frequency)
    std::vector<std::vector<float>> spectrogramData;
    int currentTimeIndex { 0 };

    // Pre-rendered image for performance
    juce::Image spectrogramImage;
    bool imageNeedsUpdate { true };

    //==========================================================================
    // Display settings
    float minFrequency { 20.0f };
    float maxFrequency { 20000.0f };
    float minDecibels { -90.0f };
    float maxDecibels { 0.0f };
    double sampleRate { 44100.0 };

    ColorMap currentColorMap { ColorMap::Viridis };
    ScrollDirection scrollDirection { ScrollDirection::Up };

    // Colormap lookup table (256 entries)
    std::vector<juce::Colour> colorMapLUT;
    void buildColorMapLUT();

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramDisplay)
};

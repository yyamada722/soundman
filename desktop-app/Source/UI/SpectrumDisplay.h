/*
  ==============================================================================

    SpectrumDisplay.h

    Real-time frequency spectrum analyzer component

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

class SpectrumDisplay : public juce::Component,
                        public juce::Timer
{
public:
    //==========================================================================
    SpectrumDisplay();
    ~SpectrumDisplay() override;

    //==========================================================================
    // FFT Configuration
    static constexpr int fftOrder = 13;  // 2^13 = 8192 samples (higher resolution)
    static constexpr int fftSize = 1 << fftOrder;

    //==========================================================================
    // Update spectrum data (call from audio thread)
    void pushNextSampleIntoFifo(float sample);

    // Get FFT data buffer for external processing
    float* getFFTData() { return fftData.data(); }
    int getFFTSize() const { return fftSize; }

    //==========================================================================
    // Display settings
    void setMinFrequency(float freq) { minFrequency = freq; repaint(); }
    void setMaxFrequency(float freq) { maxFrequency = freq; repaint(); }
    void setMinDecibels(float db) { minDecibels = db; repaint(); }
    void setMaxDecibels(float db) { maxDecibels = db; repaint(); }

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timer override (for display updates)
    void timerCallback() override;

private:
    //==========================================================================
    void drawFrame(juce::Graphics& g);
    void drawSpectrum(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawFrequencyLabels(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawDecibelLabels(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    float getFrequencyForX(float x, float width) const;
    float getXForFrequency(float freq, float width) const;
    float getYForDecibel(float db, float height) const;

    //==========================================================================
    // FFT Processing
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    std::vector<float> fifo;
    std::vector<float> fftData;
    int fifoIndex { 0 };
    bool nextFFTBlockReady { false };

    std::vector<float> scopeData;

    //==========================================================================
    // Display settings
    float minFrequency { 20.0f };
    float maxFrequency { 20000.0f };
    float minDecibels { -60.0f };
    float maxDecibels { 0.0f };

    float sampleRate { 44100.0f };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
};

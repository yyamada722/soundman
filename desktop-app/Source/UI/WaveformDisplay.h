/*
  ==============================================================================

    WaveformDisplay.h

    Audio waveform display component with playback position indicator

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

class WaveformDisplay : public juce::Component,
                        public juce::Timer,
                        public juce::ChangeListener
{
public:
    //==========================================================================
    WaveformDisplay();
    ~WaveformDisplay() override;

    //==========================================================================
    // Load audio file for display
    void loadFile(const juce::File& file, juce::AudioFormatManager& formatManager);
    void clear();

    //==========================================================================
    // Playback position (0.0 to 1.0)
    void setPosition(double position);
    double getPosition() const { return currentPosition; }

    //==========================================================================
    // Callbacks
    using SeekCallback = std::function<void(double)>;
    void setSeekCallback(SeekCallback callback) { seekCallback = callback; }

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    //==========================================================================
    // Timer override (for smooth position updates)
    void timerCallback() override;

    //==========================================================================
    // ChangeListener override (for file changes)
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    //==========================================================================
    void generateThumbnail();
    void drawWaveform(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawPositionMarker(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void handleSeek(int x);

    //==========================================================================
    std::unique_ptr<juce::AudioFormatReader> reader;
    juce::AudioBuffer<float> thumbnailData;

    double currentPosition { 0.0 };
    double duration { 0.0 };

    bool fileLoaded { false };
    int thumbnailSamples { 0 };

    SeekCallback seekCallback;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};

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
    // Zoom and Pan controls
    void setZoom(double newZoom, double centerPosition = -1.0);  // centerPosition: -1 = current center
    double getZoom() const { return zoomLevel; }
    void zoomIn(double centerPosition = -1.0);
    void zoomOut(double centerPosition = -1.0);
    void resetZoom();

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
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

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
    void drawZoomInfo(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void handleSeek(int x);
    void constrainViewRange();
    double screenXToPosition(int x, const juce::Rectangle<int>& bounds) const;

    //==========================================================================
    std::unique_ptr<juce::AudioFormatReader> reader;
    juce::AudioBuffer<float> thumbnailData;

    double currentPosition { 0.0 };
    double duration { 0.0 };

    bool fileLoaded { false };
    int thumbnailSamples { 0 };

    SeekCallback seekCallback;

    //==========================================================================
    // Zoom and Pan state
    double zoomLevel { 1.0 };           // 1.0 = full view, higher = zoomed in
    double viewStart { 0.0 };           // Start of visible range (0.0 to 1.0)
    double viewEnd { 1.0 };             // End of visible range (0.0 to 1.0)

    static constexpr double MIN_ZOOM = 1.0;
    static constexpr double MAX_ZOOM = 100.0;
    static constexpr double ZOOM_STEP = 1.5;

    // Panning state
    bool isPanning { false };
    int lastPanX { 0 };
    double panStartViewStart { 0.0 };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};

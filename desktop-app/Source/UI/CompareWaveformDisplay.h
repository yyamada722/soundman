/*
  ==============================================================================

    CompareWaveformDisplay.h

    Dual waveform overlay display for A/B comparison

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>

class CompareWaveformDisplay : public juce::Component,
                                public juce::Timer
{
public:
    //==========================================================================
    CompareWaveformDisplay();
    ~CompareWaveformDisplay() override;

    //==========================================================================
    // Load audio files for comparison
    bool loadTrackA(const juce::File& file, juce::AudioFormatManager& formatManager);
    bool loadTrackB(const juce::File& file, juce::AudioFormatManager& formatManager);
    void clearTrackA();
    void clearTrackB();

    bool hasTrackA() const { return thumbnailA.getNumChannels() > 0; }
    bool hasTrackB() const { return thumbnailB.getNumChannels() > 0; }

    //==========================================================================
    // Display options
    enum class DisplayMode
    {
        Overlay,      // Both waveforms overlaid
        Split,        // A on top, B on bottom
        Difference    // Show difference between A and B
    };

    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const { return displayMode; }

    void setTrackAColor(juce::Colour color) { trackAColor = color; repaint(); }
    void setTrackBColor(juce::Colour color) { trackBColor = color; repaint(); }

    void setTrackAVisible(bool visible) { showTrackA = visible; repaint(); }
    void setTrackBVisible(bool visible) { showTrackB = visible; repaint(); }

    void setTrackAAlpha(float alpha) { trackAAlpha = alpha; repaint(); }
    void setTrackBAlpha(float alpha) { trackBAlpha = alpha; repaint(); }

    //==========================================================================
    // Playback position
    void setPosition(double position);  // 0.0 to 1.0
    double getPosition() const { return currentPosition; }

    // Zoom and scroll
    void setZoomRange(double start, double end);  // 0.0 to 1.0
    void resetZoom();

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    // Timer for thumbnail loading
    void timerCallback() override;

    //==========================================================================
    // Callbacks
    std::function<void(double)> onSeek;  // Seek callback with position 0.0-1.0

private:
    //==========================================================================
    void drawWaveform(juce::Graphics& g, juce::AudioThumbnail& thumbnail,
                      juce::Rectangle<int> bounds, juce::Colour color, float alpha);
    void drawDifferenceWaveform(juce::Graphics& g, juce::Rectangle<int> bounds);

    double xToPosition(int x) const;
    int positionToX(double position) const;

    //==========================================================================
    // Thumbnails for both tracks
    juce::AudioFormatManager internalFormatManager;
    juce::AudioThumbnailCache thumbnailCache { 2 };
    juce::AudioThumbnail thumbnailA;
    juce::AudioThumbnail thumbnailB;

    // Display settings
    DisplayMode displayMode { DisplayMode::Overlay };
    juce::Colour trackAColor { 0xff4a90e2 };  // Blue
    juce::Colour trackBColor { 0xffe24a4a };  // Red
    float trackAAlpha { 0.8f };
    float trackBAlpha { 0.8f };
    bool showTrackA { true };
    bool showTrackB { true };

    // Position and zoom
    double currentPosition { 0.0 };
    double zoomStart { 0.0 };
    double zoomEnd { 1.0 };

    // Interaction state
    bool isDragging { false };

    // File info
    juce::String trackAName;
    juce::String trackBName;
    double trackADuration { 0.0 };
    double trackBDuration { 0.0 };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompareWaveformDisplay)
};

/*
  ==============================================================================

    AudioTimeline.h

    Professional audio timeline with time ruler, waveform, markers, and editing

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <functional>
#include <vector>

class AudioTimeline : public juce::Component,
                      private juce::Timer
{
public:
    //==========================================================================
    // Marker structure
    struct Marker
    {
        double timeSeconds { 0.0 };
        juce::String name;
        juce::Colour color { juce::Colours::yellow };
        int id { 0 };
    };

    //==========================================================================
    AudioTimeline();
    ~AudioTimeline() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Audio file operations
    bool loadFile(const juce::File& file, juce::AudioFormatManager& formatManager);
    void clearFile();
    bool hasFileLoaded() const { return thumbnail.getNumChannels() > 0; }

    //==========================================================================
    // Playback position (0.0 to 1.0)
    void setPosition(double position);
    double getPosition() const { return currentPosition; }

    // Position in seconds
    void setPositionSeconds(double seconds);
    double getPositionSeconds() const;

    // Duration
    double getDuration() const { return duration; }

    //==========================================================================
    // Selection (in seconds)
    void setSelection(double startSeconds, double endSeconds);
    void clearSelection();
    bool hasSelection() const { return selectionStart < selectionEnd; }
    double getSelectionStart() const { return selectionStart; }
    double getSelectionEnd() const { return selectionEnd; }

    //==========================================================================
    // Loop region (in seconds)
    void setLoopRegion(double startSeconds, double endSeconds);
    void clearLoopRegion();
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const { return loopEnabled; }
    double getLoopStart() const { return loopStart; }
    double getLoopEnd() const { return loopEnd; }

    // Set loop from current selection
    void setLoopFromSelection();

    //==========================================================================
    // Markers
    int addMarker(double timeSeconds, const juce::String& name = "", juce::Colour color = juce::Colours::yellow);
    void addMarkerWithId(int id, double timeSeconds, const juce::String& name, juce::Colour color = juce::Colours::yellow);
    void updateMarker(int markerId, const juce::String& name, double timeSeconds);
    void removeMarker(int markerId);
    void clearAllMarkers();
    const std::vector<Marker>& getMarkers() const { return markers; }
    void jumpToMarker(int markerId);
    void jumpToNextMarker();
    void jumpToPreviousMarker();

    //==========================================================================
    // Zoom and scroll
    void setZoomLevel(double zoom);  // 1.0 = fit to window, >1.0 = zoomed in
    double getZoomLevel() const { return zoomLevel; }
    void setScrollPosition(double position);  // 0.0 to 1.0
    double getScrollPosition() const { return scrollPosition; }
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomToSelection();

    //==========================================================================
    // Display options
    void setShowTimeRuler(bool show) { showTimeRuler = show; repaint(); }
    void setShowWaveform(bool show) { showWaveform = show; repaint(); }
    void setShowMarkers(bool show) { showMarkers = show; repaint(); }
    void setShowLoopRegion(bool show) { showLoopRegion = show; repaint(); }

    void setWaveformColor(juce::Colour color) { waveformColor = color; repaint(); }
    void setSelectionColor(juce::Colour color) { selectionColor = color; repaint(); }
    void setLoopRegionColor(juce::Colour color) { loopRegionColor = color; repaint(); }
    void setPlayheadColor(juce::Colour color) { playheadColor = color; repaint(); }

    //==========================================================================
    // Time format for ruler
    enum class TimeFormat
    {
        Seconds,        // 0.000
        MinSec,         // 0:00.000
        SMPTE,          // 00:00:00:00
        Samples,        // 0
        Bars            // 1.1.1 (requires tempo)
    };
    void setTimeFormat(TimeFormat format) { timeFormat = format; repaint(); }
    void setTempo(double bpm) { tempo = bpm; }

    //==========================================================================
    // Callbacks
    std::function<void(double)> onPositionChanged;      // Position changed (0-1)
    std::function<void(double, double)> onSelectionChanged;  // Selection changed (seconds)
    std::function<void(double, double)> onLoopRegionChanged; // Loop region changed (seconds)
    std::function<void(int)> onMarkerClicked;           // Marker clicked
    std::function<void(int, double, const juce::String&)> onMarkerAdded;  // Marker added (id, time, name)

    //==========================================================================
    // Mouse interaction
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    //==========================================================================
    // Keyboard
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void timerCallback() override;

    //==========================================================================
    // Drawing helpers
    void drawTimeRuler(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawWaveform(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawSelection(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawLoopRegion(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawMarkers(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawPlayhead(juce::Graphics& g, juce::Rectangle<int> bounds);

    //==========================================================================
    // Coordinate conversion
    int timeToX(double seconds) const;
    double xToTime(int x) const;
    double getVisibleStartTime() const;
    double getVisibleEndTime() const;
    double getVisibleDuration() const;

    //==========================================================================
    // Time formatting
    juce::String formatTime(double seconds) const;
    double getRulerTickInterval() const;

    //==========================================================================
    // Thumbnail
    juce::AudioFormatManager internalFormatManager;
    juce::AudioThumbnailCache thumbnailCache { 1 };
    juce::AudioThumbnail thumbnail;

    // State
    double duration { 0.0 };
    double currentPosition { 0.0 };  // 0.0 to 1.0

    // Selection
    double selectionStart { 0.0 };
    double selectionEnd { 0.0 };
    bool isSelecting { false };
    double selectionAnchor { 0.0 };

    // Loop
    double loopStart { 0.0 };
    double loopEnd { 0.0 };
    bool loopEnabled { false };

    // Markers
    std::vector<Marker> markers;
    int nextMarkerId { 1 };

    // Zoom and scroll
    double zoomLevel { 1.0 };      // 1.0 = fit to window
    double scrollPosition { 0.0 }; // 0.0 to 1.0

    // Display options
    bool showTimeRuler { true };
    bool showWaveform { true };
    bool showMarkers { true };
    bool showLoopRegion { true };

    // Colors
    juce::Colour backgroundColor { 0xff1a1a1a };
    juce::Colour rulerColor { 0xff2a2a2a };
    juce::Colour rulerTextColor { 0xffaaaaaa };
    juce::Colour waveformColor { 0xff4a90e2 };
    juce::Colour selectionColor { 0x404a90e2 };
    juce::Colour loopRegionColor { 0x3000ff00 };
    juce::Colour playheadColor { 0xffffffff };
    juce::Colour gridColor { 0xff3a3a3a };

    // Time format
    TimeFormat timeFormat { TimeFormat::MinSec };
    double tempo { 120.0 };

    // Layout
    static constexpr int rulerHeight = 25;
    static constexpr int markerHeight = 20;

    // Interaction state
    enum class DragMode
    {
        None,
        Seeking,
        Selecting,
        MovingLoopStart,
        MovingLoopEnd,
        Scrolling
    };
    DragMode currentDragMode { DragMode::None };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioTimeline)
};

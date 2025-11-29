/*
  ==============================================================================

    TimelinePanel.h

    Multi-track timeline UI with track headers and clip arrangement

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Core/ProjectModel.h"
#include "../Core/ProjectManager.h"

//==============================================================================
// Forward declarations
//==============================================================================
class TimelinePanel;
class TrackHeaderComponent;
class ClipComponent;

//==============================================================================
// Track Header Component - Shows track name, mute/solo/arm buttons
//==============================================================================

class TrackHeaderComponent : public juce::Component
{
public:
    TrackHeaderComponent(ProjectManager& pm, const juce::ValueTree& trackState);
    ~TrackHeaderComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateFromState();
    juce::String getTrackId() const;

private:
    ProjectManager& projectManager;
    juce::ValueTree state;

    juce::Label nameLabel;
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };
    juce::TextButton armButton { "R" };
    juce::Slider volumeSlider;

    juce::Colour trackColor;

    void setupComponents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackHeaderComponent)
};

//==============================================================================
// Clip Component - Visual representation of a clip on the timeline
//==============================================================================

class ClipComponent : public juce::Component
{
public:
    ClipComponent(ProjectManager& pm,
                  const juce::ValueTree& clipState,
                  juce::AudioThumbnailCache& thumbnailCache,
                  juce::AudioFormatManager& formatManager);
    ~ClipComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    void updateFromState();
    juce::String getClipId() const;

    // Get clip timeline bounds in samples
    juce::int64 getTimelineStart() const;
    juce::int64 getTimelineEnd() const;

    // Set selection state
    void setSelected(bool selected);
    bool isSelected() const { return selected; }

    // Callback for when clip is moved
    std::function<void(ClipComponent*, juce::int64 newStart)> onClipMoved;
    std::function<void(ClipComponent*)> onClipSelected;

    // Access to state for parent components
    juce::ValueTree& getState() { return state; }

private:
    ProjectManager& projectManager;
    juce::ValueTree state;

    // Waveform thumbnail
    juce::AudioThumbnail thumbnail;
    bool thumbnailLoaded { false };

    // Clip properties
    juce::String clipName;
    juce::Colour clipColor;
    juce::int64 timelineStart { 0 };
    juce::int64 length { 0 };
    float gain { 1.0f };

    // Interaction state
    bool selected { false };
    bool dragging { false };
    juce::Point<int> dragStartPos;
    juce::int64 dragStartTimelinePos { 0 };

    // Trim handles
    static constexpr int trimHandleWidth = 8;
    enum class TrimMode { None, Start, End };
    TrimMode trimMode { TrimMode::None };

    void loadThumbnail();
    bool isOverTrimHandle(const juce::Point<int>& pos, TrimMode& mode) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};

//==============================================================================
// Track Lane Component - Contains clips for a single track
//==============================================================================

class TrackLaneComponent : public juce::Component
{
public:
    TrackLaneComponent(ProjectManager& pm,
                       const juce::ValueTree& trackState,
                       juce::AudioThumbnailCache& thumbnailCache,
                       juce::AudioFormatManager& formatManager);
    ~TrackLaneComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateFromState();
    void rebuildClips();

    juce::String getTrackId() const;

    // Pixel/sample conversion
    void setPixelsPerSample(double pps) { pixelsPerSample = pps; layoutClips(); }
    double getPixelsPerSample() const { return pixelsPerSample; }

    // Selection
    void clearSelection();
    ClipComponent* getSelectedClip() const { return selectedClip; }

    // Callback for clip selection
    std::function<void(ClipComponent*)> onClipSelected;

private:
    ProjectManager& projectManager;
    juce::ValueTree state;
    juce::AudioThumbnailCache& thumbnailCache;
    juce::AudioFormatManager& formatManager;

    juce::OwnedArray<ClipComponent> clips;
    ClipComponent* selectedClip { nullptr };

    double pixelsPerSample { 0.001 };  // Default: ~22 pixels per 1000 samples

    juce::Colour trackColor;

    void layoutClips();
    void handleClipSelected(ClipComponent* clip);
    void handleClipMoved(ClipComponent* clip, juce::int64 newStart);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackLaneComponent)
};

//==============================================================================
// Timeline Ruler - Shows time/bars at the top
//==============================================================================

class TimelineRuler : public juce::Component
{
public:
    TimelineRuler();
    ~TimelineRuler() override = default;

    void paint(juce::Graphics& g) override;

    void setPixelsPerSample(double pps) { pixelsPerSample = pps; repaint(); }
    void setSampleRate(double rate) { sampleRate = rate; repaint(); }
    void setBpm(double newBpm) { bpm = newBpm; repaint(); }
    void setTimeSignature(int num, int den) { timeSigNum = num; timeSigDen = den; repaint(); }

    void setScrollOffset(int offset) { scrollOffset = offset; repaint(); }

    // Display modes
    enum class DisplayMode { Bars, Time, Samples };
    void setDisplayMode(DisplayMode mode) { displayMode = mode; repaint(); }

private:
    double pixelsPerSample { 0.001 };
    double sampleRate { 44100.0 };
    double bpm { 120.0 };
    int timeSigNum { 4 };
    int timeSigDen { 4 };
    int scrollOffset { 0 };
    DisplayMode displayMode { DisplayMode::Bars };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineRuler)
};

//==============================================================================
// Playhead Component - The moving position indicator
//==============================================================================

class PlayheadComponent : public juce::Component
{
public:
    PlayheadComponent();
    ~PlayheadComponent() override = default;

    void paint(juce::Graphics& g) override;

    void setPosition(juce::int64 samplePos);
    void setPixelsPerSample(double pps) { pixelsPerSample = pps; updatePosition(); }
    void setScrollOffset(int offset) { scrollOffset = offset; updatePosition(); }

private:
    juce::int64 positionSamples { 0 };
    double pixelsPerSample { 0.001 };
    int scrollOffset { 0 };

    void updatePosition();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayheadComponent)
};

//==============================================================================
// Scrollable Viewport - Viewport with scroll callback
//==============================================================================

class ScrollableViewport : public juce::Viewport
{
public:
    std::function<void()> onScroll;

    void visibleAreaChanged(const juce::Rectangle<int>& newVisibleArea) override
    {
        juce::Viewport::visibleAreaChanged(newVisibleArea);
        if (onScroll)
            onScroll();
    }
};

//==============================================================================
// Horizontal Fader Strip - Compact horizontal mixer strip for bottom section
//==============================================================================

class HorizontalFaderStrip : public juce::Component,
                              public juce::Timer
{
public:
    HorizontalFaderStrip(ProjectManager& pm, const juce::ValueTree& trackState);
    ~HorizontalFaderStrip() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void updateFromState();
    juce::String getTrackId() const;

    // Set meter levels
    void setMeterLevels(float left, float right);

private:
    ProjectManager& projectManager;
    juce::ValueTree state;

    // UI Components
    juce::Label nameLabel;
    juce::Slider faderSlider;          // Horizontal fader
    juce::Slider panSlider;            // Small pan knob
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };

    // Meter
    float levelL { 0.0f };
    float levelR { 0.0f };
    float peakL { 0.0f };
    float peakR { 0.0f };

    juce::Colour trackColor;

    void setupComponents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HorizontalFaderStrip)
};

//==============================================================================
// Mixer Section Component - Contains all horizontal fader strips
//==============================================================================

class MixerSectionComponent : public juce::Component
{
public:
    MixerSectionComponent(ProjectManager& pm);
    ~MixerSectionComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void rebuildStrips();
    void updateFromProject();

    // Scroll sync with timeline headers
    void setScrollOffset(int offset);

private:
    ProjectManager& projectManager;

    juce::OwnedArray<HorizontalFaderStrip> strips;
    int scrollOffset { 0 };

    static constexpr int stripHeight = 50;

    void layoutStrips();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerSectionComponent)
};

//==============================================================================
// Timeline Panel - Main container for multi-track timeline
//==============================================================================

class TimelinePanel : public juce::Component,
                      public juce::Timer,
                      public ProjectManager::Listener
{
public:
    TimelinePanel(ProjectManager& pm, juce::AudioFormatManager& formatManager);
    ~TimelinePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDown(const juce::MouseEvent& e) override;

    //==========================================================================
    // Timer
    //==========================================================================
    void timerCallback() override;

    //==========================================================================
    // ProjectManager::Listener
    //==========================================================================
    void projectChanged() override;
    void trackAdded(const juce::ValueTree& track) override;
    void trackRemoved(const juce::ValueTree& track) override;
    void trackPropertyChanged(const juce::ValueTree& track, const juce::Identifier& property) override;
    void clipAdded(const juce::ValueTree& clip) override;
    void clipRemoved(const juce::ValueTree& clip) override;
    void clipPropertyChanged(const juce::ValueTree& clip, const juce::Identifier& property) override;

    //==========================================================================
    // Timeline control
    //==========================================================================

    // Zoom control
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void setZoomLevel(double samplesPerPixel);
    double getZoomLevel() const { return 1.0 / pixelsPerSample; }

    // Scroll control
    void scrollToPosition(juce::int64 samplePos);
    void scrollBy(int pixels);

    // Playhead
    void setPlayheadPosition(juce::int64 samplePos);
    juce::int64 getPlayheadPosition() const { return playheadPosition; }

    // Grid snap
    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    bool isSnapEnabled() const { return snapEnabled; }
    void setSnapResolution(double beatsPerSnap) { snapResolution = beatsPerSnap; }

    // Track height
    void setTrackHeight(int height);
    int getTrackHeight() const { return trackHeight; }

    // Mixer section visibility
    void setMixerVisible(bool visible);
    bool isMixerVisible() const { return mixerVisible; }

    // Callbacks
    std::function<void(juce::int64)> onPlayheadMoved;
    std::function<void(ClipComponent*)> onClipSelected;

private:
    ProjectManager& projectManager;
    juce::AudioFormatManager& formatManager;
    juce::AudioThumbnailCache thumbnailCache { 32 };

    // UI Components
    TimelineRuler ruler;
    PlayheadComponent playhead;
    ScrollableViewport trackViewport;
    juce::Component trackContainer;
    juce::Component headerContainer;
    ScrollableViewport headerViewport;

    juce::OwnedArray<TrackHeaderComponent> trackHeaders;
    juce::OwnedArray<TrackLaneComponent> trackLanes;

    // Mixer section (bottom faders)
    std::unique_ptr<MixerSectionComponent> mixerSection;
    juce::TextButton toggleMixerButton { "Mix" };
    bool mixerVisible { true };

    // Layout
    static constexpr int rulerHeight = 30;
    static constexpr int headerWidth = 150;
    static constexpr int mixerHeight = 120;
    int trackHeight { 80 };

    // Zoom and scroll
    double pixelsPerSample { 0.01 };  // Default zoom level
    int scrollOffsetX { 0 };

    // Playhead
    juce::int64 playheadPosition { 0 };

    // Grid snap
    bool snapEnabled { true };
    double snapResolution { 1.0 };  // 1 beat

    // Build/rebuild UI
    void rebuildTracks();
    void layoutTracks();
    void updateZoom();

    // Snap helpers
    juce::int64 snapToGrid(juce::int64 samplePos) const;

    // Handle clip interactions
    void handleClipSelected(ClipComponent* clip);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelinePanel)
};

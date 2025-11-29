/*
  ==============================================================================

    TopInfoBar.h

    Professional DAW-style top information bar combining transport and file info
    Inspired by Logic Pro and Pro Tools

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>

class TopInfoBar : public juce::Component,
                   public juce::Timer
{
public:
    TopInfoBar();
    ~TopInfoBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==========================================================================
    // File info
    void setFileInfo(const juce::File& file, juce::AudioFormatReader* reader);
    void clearFileInfo();

    //==========================================================================
    // Transport state
    void setPlaying(bool isPlaying);
    void setRecording(bool isRecording);
    void setLoopEnabled(bool enabled);

    //==========================================================================
    // Position and time
    void setPosition(double positionSeconds);
    void setDuration(double durationSeconds);
    void setSampleRate(double rate);
    void setLoopRange(double startSeconds, double endSeconds);

    //==========================================================================
    // Level meters (mini display)
    void setLevels(float leftRMS, float leftPeak, float rightRMS, float rightPeak);

    //==========================================================================
    // BPM/Tempo display
    void setBPM(double bpm);
    void setKey(const juce::String& key);

    //==========================================================================
    // Device info
    void setDeviceName(const juce::String& name);
    void setBufferSize(int size);

    //==========================================================================
    // Callbacks
    std::function<void()> onPlay;
    std::function<void()> onPause;
    std::function<void()> onStop;
    std::function<void()> onRecord;
    std::function<void()> onSkipToStart;
    std::function<void()> onSkipToEnd;
    std::function<void()> onToggleLoop;
    std::function<void(double)> onSeek;  // Seek to position in seconds
    std::function<void()> onOpenFile;
    std::function<void()> onSettings;

private:
    //==========================================================================
    void createTransportButtons();
    void updatePlayButtonIcon();
    juce::String formatTimecode(double seconds) const;
    juce::String formatTimecodeCompact(double seconds) const;
    juce::Font getJapaneseFont(float height, int style = juce::Font::plain) const;
    void drawSection(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title);
    void drawLCDBackground(juce::Graphics& g, juce::Rectangle<int> bounds);

    //==========================================================================
    // File info
    juce::String fileName;
    juce::String filePath;
    juce::String fileFormat;
    double sampleRate { 0.0 };
    int numChannels { 0 };
    int bitsPerSample { 0 };

    // Device info
    juce::String deviceName;
    int bufferSize { 0 };

    // Transport state
    bool playing { false };
    bool recording { false };
    bool loopEnabled { false };

    // Position
    double position { 0.0 };
    double duration { 0.0 };
    double loopStart { 0.0 };
    double loopEnd { 0.0 };

    // Levels (mini meters)
    float leftRMS { 0.0f };
    float leftPeak { 0.0f };
    float rightRMS { 0.0f };
    float rightPeak { 0.0f };

    // BPM/Key
    double bpm { 0.0 };
    juce::String musicalKey;

    // UI Components - Transport buttons
    juce::DrawableButton skipToStartButton { "SkipToStart", juce::DrawableButton::ImageFitted };
    juce::DrawableButton rewindButton { "Rewind", juce::DrawableButton::ImageFitted };
    juce::DrawableButton stopButton { "Stop", juce::DrawableButton::ImageFitted };
    juce::DrawableButton playButton { "Play", juce::DrawableButton::ImageFitted };
    juce::DrawableButton forwardButton { "Forward", juce::DrawableButton::ImageFitted };
    juce::DrawableButton skipToEndButton { "SkipToEnd", juce::DrawableButton::ImageFitted };
    juce::DrawableButton recordButton { "Record", juce::DrawableButton::ImageFitted };
    juce::DrawableButton loopButton { "Loop", juce::DrawableButton::ImageFitted };

    // Utility buttons
    juce::TextButton openFileButton { "Open" };
    juce::TextButton settingsButton { "Settings" };

    // Stored drawable for play/pause toggle
    std::unique_ptr<juce::Drawable> playIcon;
    std::unique_ptr<juce::Drawable> pauseIcon;

    // Cached font name
    mutable juce::String cachedFontName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopInfoBar)
};

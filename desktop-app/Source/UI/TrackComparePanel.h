/*
  ==============================================================================

    TrackComparePanel.h

    Panel for comparing two audio tracks

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "CompareWaveformDisplay.h"

class TrackComparePanel : public juce::Component
{
public:
    //==========================================================================
    TrackComparePanel();
    ~TrackComparePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Track loading
    void setFormatManager(juce::AudioFormatManager* manager) { formatManager = manager; }

    bool loadTrackA(const juce::File& file);
    bool loadTrackB(const juce::File& file);

    bool hasTrackA() const { return waveformDisplay.hasTrackA(); }
    bool hasTrackB() const { return waveformDisplay.hasTrackB(); }

    juce::File getTrackAFile() const { return trackAFile; }
    juce::File getTrackBFile() const { return trackBFile; }

    //==========================================================================
    // Playback control
    enum class ActiveTrack
    {
        A,
        B,
        Both  // Mix both
    };

    void setActiveTrack(ActiveTrack track);
    ActiveTrack getActiveTrack() const { return activeTrack; }

    void setMixBalance(float balance);  // 0.0 = A only, 1.0 = B only, 0.5 = equal mix
    float getMixBalance() const { return mixBalance; }

    void setPosition(double position);
    double getPosition() const { return currentPosition; }

    //==========================================================================
    // Callbacks
    std::function<void(const juce::File&, ActiveTrack)> onTrackLoaded;
    std::function<void(ActiveTrack)> onActiveTrackChanged;
    std::function<void(float)> onMixBalanceChanged;
    std::function<void(double)> onSeek;

private:
    void openFileDialogForTrack(ActiveTrack track);
    void updateButtonStates();
    void swapTracks();

    juce::AudioFormatManager* formatManager { nullptr };

    // Files
    juce::File trackAFile;
    juce::File trackBFile;

    // State
    ActiveTrack activeTrack { ActiveTrack::A };
    float mixBalance { 0.5f };
    double currentPosition { 0.0 };

    // UI Components
    CompareWaveformDisplay waveformDisplay;

    juce::Label titleLabel;

    // Track A controls
    juce::TextButton loadAButton { "Load A" };
    juce::TextButton clearAButton { "X" };
    juce::Label trackALabel;

    // Track B controls
    juce::TextButton loadBButton { "Load B" };
    juce::TextButton clearBButton { "X" };
    juce::Label trackBLabel;

    // Comparison controls
    juce::TextButton playAButton { "A" };
    juce::TextButton playBButton { "B" };
    juce::TextButton playBothButton { "A+B" };
    juce::TextButton swapButton { "<->" };

    juce::Slider mixSlider;
    juce::Label mixLabel;

    // Display mode
    juce::ComboBox displayModeCombo;

    // File chooser
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::Colour activeColor { 0xff4a90e2 };
    juce::Colour inactiveColor { 0xff3a3a3a };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackComparePanel)
};

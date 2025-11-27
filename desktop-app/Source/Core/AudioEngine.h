/*
  ==============================================================================

    AudioEngine.h

    Audio playback engine with device management and file playback support

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <functional>
#include <vector>

class AudioEngine : public juce::AudioIODeviceCallback,
                    public juce::ChangeListener
{
public:
    //==========================================================================
    enum class PlayState
    {
        Stopped,
        Playing,
        Paused
    };

    //==========================================================================
    AudioEngine();
    ~AudioEngine() override;

    //==========================================================================
    // Initialization
    bool initialize();
    void shutdown();

    //==========================================================================
    // File operations
    bool loadFile(const juce::File& file);
    void unloadFile();
    juce::String getCurrentFileName() const;
    bool hasFileLoaded() const;

    //==========================================================================
    // Playback control
    void play();
    void pause();
    void stop();
    void setPosition(double position);  // 0.0 to 1.0

    //==========================================================================
    // State queries
    PlayState getPlayState() const { return playState.load(); }
    bool isPlaying() const { return playState.load() == PlayState::Playing; }
    double getPosition() const;  // 0.0 to 1.0
    double getDuration() const;  // in seconds

    //==========================================================================
    // Level calculation at specific position
    struct AudioLevels
    {
        float leftRMS = 0.0f;
        float leftPeak = 0.0f;
        float rightRMS = 0.0f;
        float rightPeak = 0.0f;
    };
    AudioLevels calculateLevelsAtPosition(double position);  // position 0.0 to 1.0

    //==========================================================================
    // Audio device info
    juce::String getCurrentDeviceName() const;
    double getCurrentSampleRate() const;
    int getCurrentBufferSize() const;

    //==========================================================================
    // Format manager access for waveform display
    juce::AudioFormatManager& getFormatManager() { return formatManager; }
    juce::File getCurrentFile() const { return currentFile; }

    // Device manager access for settings dialog
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    //==========================================================================
    // Callbacks
    using ErrorCallback = std::function<void(const juce::String&)>;
    void setErrorCallback(ErrorCallback callback) { errorCallback = callback; }

    using LevelCallback = std::function<void(float, float, float, float)>;  // leftRMS, leftPeak, rightRMS, rightPeak
    void setLevelCallback(LevelCallback callback) { levelCallback = callback; }

    using SpectrumCallback = std::function<void(float)>;  // sample for FFT analysis
    void setSpectrumCallback(SpectrumCallback callback) { spectrumCallback = callback; }

    using TruePeakCallback = std::function<void(float, float)>;  // leftPeak, rightPeak
    void setTruePeakCallback(TruePeakCallback callback) { truePeakCallback = callback; }

    using PhaseCorrelationCallback = std::function<void(float)>;  // correlation coefficient
    void setPhaseCorrelationCallback(PhaseCorrelationCallback callback) { phaseCorrelationCallback = callback; }

    using LoudnessCallback = std::function<void(float, float, float, float)>;  // integrated, short-term, momentary, LRA
    void setLoudnessCallback(LoudnessCallback callback) { loudnessCallback = callback; }

    //==========================================================================
    // AudioIODeviceCallback implementation
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                         int numInputChannels,
                                         float* const* outputChannelData,
                                         int numOutputChannels,
                                         int numSamples,
                                         const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    //==========================================================================
    // ChangeListener implementation (for transport state changes)
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    //==========================================================================
    void showError(const juce::String& message);
    void prepareToPlay(double sampleRate, int blockSize);
    void releaseResources();

    //==========================================================================
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transportSource;
    juce::MixerAudioSource mixerSource;

    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<juce::ResamplingAudioSource> resamplingSource;

    std::atomic<PlayState> playState { PlayState::Stopped };
    juce::File currentFile;

    ErrorCallback errorCallback;
    LevelCallback levelCallback;
    SpectrumCallback spectrumCallback;
    TruePeakCallback truePeakCallback;
    PhaseCorrelationCallback phaseCorrelationCallback;
    LoudnessCallback loudnessCallback;

    bool initialized { false };
    double preparedSampleRate { 0.0 };
    int preparedBlockSize { 0 };

    // Loudness measurement state
    std::vector<float> loudnessBuffer;           // Circular buffer for loudness blocks
    int loudnessBufferIndex { 0 };
    double sumSquaredSamples { 0.0 };            // For integrated loudness
    int totalSampleCount { 0 };
    static constexpr int MOMENTARY_BLOCKS = 4;   // 400ms / 100ms blocks
    static constexpr int SHORT_TERM_BLOCKS = 30; // 3s / 100ms blocks
    static constexpr int BLOCK_SIZE_MS = 100;    // 100ms blocks

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};

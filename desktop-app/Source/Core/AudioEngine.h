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

    enum class RecordState
    {
        Stopped,
        Recording,
        Paused
    };

    // Track selection for dual-track playback
    enum class ActiveTrack
    {
        A,      // Play only Track A (main)
        B,      // Play only Track B (comparison)
        Both    // Mix both tracks
    };

    //==========================================================================
    AudioEngine();
    ~AudioEngine() override;

    //==========================================================================
    // Initialization
    bool initialize();
    void shutdown();

    //==========================================================================
    // File operations (Track A - main track)
    bool loadFile(const juce::File& file);
    void unloadFile();
    juce::String getCurrentFileName() const;
    bool hasFileLoaded() const;

    // Track B operations (comparison track)
    bool loadTrackB(const juce::File& file);
    void unloadTrackB();
    juce::String getTrackBFileName() const;
    bool hasTrackBLoaded() const;
    juce::File getTrackBFile() const { return trackBFile; }

    // Dual track control
    void setActiveTrack(ActiveTrack track);
    ActiveTrack getActiveTrack() const { return activeTrack.load(); }
    void setTrackMixBalance(float balance);  // 0.0 = A only, 1.0 = B only
    float getTrackMixBalance() const { return trackMixBalance.load(); }

    //==========================================================================
    // Playback control
    void play();
    void pause();
    void stop();
    void setPosition(double position);  // 0.0 to 1.0
    void setPositionSeconds(double seconds);  // Seek to time in seconds

    //==========================================================================
    // Loop/Range playback
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const { return loopEnabled.load(); }

    void setLoopRange(double startSeconds, double endSeconds);
    double getLoopStart() const { return loopStartSeconds.load(); }
    double getLoopEnd() const { return loopEndSeconds.load(); }

    //==========================================================================
    // Recording control
    bool startRecording(const juce::File& outputFile);
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    RecordState getRecordState() const { return recordState.load(); }
    bool isRecording() const { return recordState.load() == RecordState::Recording; }

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
    // Master gain control
    void setMasterGain(float gainLinear) { masterGain.store(gainLinear); }
    float getMasterGain() const { return masterGain.load(); }

    //==========================================================================
    // External source support (for multi-track playback)
    void setMultiTrackSource(juce::PositionableAudioSource* source);
    void clearMultiTrackSource();
    bool isUsingMultiTrackSource() const { return multiTrackSource != nullptr; }

    // Dry/Wet mix control for A/B comparison
    void setDryWetMix(float wetAmount) { dryWetMix.store(juce::jlimit(0.0f, 1.0f, wetAmount)); }
    float getDryWetMix() const { return dryWetMix.load(); }

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

    // Audio processing callback (for filters, EQ, etc.)
    using AudioProcessCallback = std::function<void(juce::AudioBuffer<float>&)>;
    void setAudioProcessCallback(AudioProcessCallback callback) { audioProcessCallback = callback; }

    // Device started callback (for preparing processors with correct sample rate)
    using DeviceStartedCallback = std::function<void(double sampleRate, int blockSize)>;
    void setDeviceStartedCallback(DeviceStartedCallback callback) { deviceStartedCallback = callback; }

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

    // Track A (main track)
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<juce::ResamplingAudioSource> resamplingSource;
    juce::File currentFile;

    // Track B (comparison track)
    juce::AudioTransportSource transportSourceB;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSourceB;
    juce::File trackBFile;

    // Mixer for combining tracks
    juce::MixerAudioSource mixerSource;

    // Track selection and mixing
    std::atomic<ActiveTrack> activeTrack { ActiveTrack::A };
    std::atomic<float> trackMixBalance { 0.5f };  // 0.0 = A only, 1.0 = B only

    // External multi-track source (not owned)
    juce::PositionableAudioSource* multiTrackSource { nullptr };

    std::atomic<PlayState> playState { PlayState::Stopped };

    ErrorCallback errorCallback;
    LevelCallback levelCallback;
    SpectrumCallback spectrumCallback;
    TruePeakCallback truePeakCallback;
    PhaseCorrelationCallback phaseCorrelationCallback;
    LoudnessCallback loudnessCallback;
    AudioProcessCallback audioProcessCallback;
    DeviceStartedCallback deviceStartedCallback;

    bool initialized { false };
    double preparedSampleRate { 0.0 };
    int preparedBlockSize { 0 };

    // Master gain control
    std::atomic<float> masterGain { 1.0f };  // Linear gain (1.0 = 0dB)

    // Dry/Wet mix control (0.0 = dry, 1.0 = wet)
    std::atomic<float> dryWetMix { 1.0f };  // Default to fully wet (processed)

    // Loop/Range playback
    std::atomic<bool> loopEnabled { false };
    std::atomic<double> loopStartSeconds { 0.0 };
    std::atomic<double> loopEndSeconds { 0.0 };

    // Recording state
    std::atomic<RecordState> recordState { RecordState::Stopped };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> recordingWriter;
    juce::File recordingFile;
    juce::CriticalSection writerLock;
    juce::TimeSliceThread recordingThread { "Recording Thread" };

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

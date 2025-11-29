/*
  ==============================================================================

    MultiTrackAudioSource.h

    Multi-track audio playback engine
    Handles mixing multiple tracks with clips, volume, pan, and mute/solo

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "ProjectModel.h"

//==============================================================================
// Forward declarations
//==============================================================================
class TrackAudioSource;
class ClipAudioSource;

//==============================================================================
// Audio File Cache - Shared cache for loaded audio files
//==============================================================================

class AudioFileCache
{
public:
    AudioFileCache(juce::AudioFormatManager& formatManager);
    ~AudioFileCache() = default;

    // Get or load an audio reader source for a file
    // Returns nullptr if file cannot be loaded
    juce::AudioFormatReaderSource* getReaderSource(const juce::String& filePath);

    // Get the sample rate of a cached file
    double getSampleRate(const juce::String& filePath) const;

    // Get the length in samples of a cached file
    juce::int64 getLengthInSamples(const juce::String& filePath) const;

    // Get the number of channels of a cached file
    int getNumChannels(const juce::String& filePath) const;

    // Clear the cache
    void clearCache();

    // Remove a specific file from cache
    void removeFromCache(const juce::String& filePath);

private:
    struct CachedFile
    {
        std::unique_ptr<juce::AudioFormatReader> reader;
        std::unique_ptr<juce::AudioFormatReaderSource> source;
        double sampleRate { 0.0 };
        juce::int64 lengthInSamples { 0 };
        int numChannels { 0 };
    };

    juce::AudioFormatManager& formatManager;
    std::map<juce::String, CachedFile> cache;
    juce::CriticalSection cacheLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFileCache)
};

//==============================================================================
// Clip Audio Source - Handles playback of a single clip
//==============================================================================

class ClipAudioSource : public juce::PositionableAudioSource
{
public:
    ClipAudioSource(AudioFileCache& cache, const juce::ValueTree& clipState);
    ~ClipAudioSource() override = default;

    //==========================================================================
    // PositionableAudioSource interface
    //==========================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void setNextReadPosition(juce::int64 newPosition) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override { return false; }

    //==========================================================================
    // Clip-specific methods
    //==========================================================================

    // Check if this clip plays at the given timeline position
    bool isActiveAt(juce::int64 timelinePosition) const;

    // Get the clip's timeline start position
    juce::int64 getTimelineStart() const;

    // Get the clip's timeline end position
    juce::int64 getTimelineEnd() const;

    // Get the clip's gain
    float getGain() const;

    // Update from ValueTree state
    void updateFromState();

    // Get clip ID
    juce::String getClipId() const;

private:
    AudioFileCache& audioCache;
    juce::ValueTree state;

    // Cached properties from state
    juce::String audioFilePath;
    juce::int64 sourceStart { 0 };
    juce::int64 length { 0 };
    juce::int64 timelineStart { 0 };
    float gain { 1.0f };
    juce::int64 fadeInSamples { 0 };
    juce::int64 fadeOutSamples { 0 };

    // Playback state
    juce::int64 currentPosition { 0 };
    double currentSampleRate { 44100.0 };
    int samplesPerBlock { 512 };

    // Temporary buffer for resampling if needed
    juce::AudioBuffer<float> tempBuffer;

    // Calculate fade gain at a position within the clip
    float calculateFadeGain(juce::int64 positionInClip) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipAudioSource)
};

//==============================================================================
// Track Audio Source - Handles playback of a single track with multiple clips
//==============================================================================

class TrackAudioSource : public juce::PositionableAudioSource
{
public:
    TrackAudioSource(AudioFileCache& cache, const juce::ValueTree& trackState);
    ~TrackAudioSource() override = default;

    //==========================================================================
    // PositionableAudioSource interface
    //==========================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void setNextReadPosition(juce::int64 newPosition) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override { return false; }

    //==========================================================================
    // Track-specific methods
    //==========================================================================

    // Get track properties
    float getVolume() const;
    float getPan() const;
    bool isMuted() const;
    bool isSoloed() const;

    // Set whether solo is active anywhere (affects mute behavior)
    void setSoloActiveInProject(bool soloActive);

    // Update from ValueTree state
    void updateFromState();

    // Get track ID
    juce::String getTrackId() const;

    // Rebuild clip sources from state
    void rebuildClips();

private:
    AudioFileCache& audioCache;
    juce::ValueTree state;

    // Clip sources
    juce::OwnedArray<ClipAudioSource> clips;

    // Track properties
    float volume { 1.0f };
    float pan { 0.0f };
    bool muted { false };
    bool soloed { false };
    bool soloActiveInProject { false };

    // Playback state
    juce::int64 currentPosition { 0 };
    double currentSampleRate { 44100.0 };
    int samplesPerBlock { 512 };

    // Temporary buffer for mixing clips
    juce::AudioBuffer<float> mixBuffer;

    // Apply pan to a stereo buffer
    void applyPan(juce::AudioBuffer<float>& buffer, float panValue);

    // Check if this track should play (considering mute/solo)
    bool shouldPlay() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackAudioSource)
};

//==============================================================================
// Multi-Track Audio Source - Main mixer for all tracks
//==============================================================================

class MultiTrackAudioSource : public juce::PositionableAudioSource,
                               public juce::ValueTree::Listener
{
public:
    MultiTrackAudioSource(juce::AudioFormatManager& formatManager);
    ~MultiTrackAudioSource() override;

    //==========================================================================
    // PositionableAudioSource interface
    //==========================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void setNextReadPosition(juce::int64 newPosition) override;
    juce::int64 getNextReadPosition() const override;
    juce::int64 getTotalLength() const override;
    bool isLooping() const override { return looping; }
    void setLooping(bool shouldLoop) { looping = shouldLoop; }

    //==========================================================================
    // Project management
    //==========================================================================

    // Load a project for playback
    void loadProject(const juce::ValueTree& projectState);

    // Unload current project
    void unloadProject();

    // Rebuild all track sources from current project state
    void rebuildFromProject();

    // Get the project sample rate
    double getProjectSampleRate() const;

    //==========================================================================
    // Transport control
    //==========================================================================

    // Get/set loop range (in samples)
    void setLoopRange(juce::int64 startSample, juce::int64 endSample);
    juce::int64 getLoopStart() const { return loopStart; }
    juce::int64 getLoopEnd() const { return loopEnd; }

    //==========================================================================
    // Master output
    //==========================================================================

    float getMasterVolume() const { return masterVolume; }
    void setMasterVolume(float volume) { masterVolume = juce::jlimit(0.0f, 2.0f, volume); }

    float getMasterPan() const { return masterPan; }
    void setMasterPan(float pan) { masterPan = juce::jlimit(-1.0f, 1.0f, pan); }

    //==========================================================================
    // ValueTree::Listener
    //==========================================================================
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

private:
    juce::AudioFormatManager& formatManager;
    AudioFileCache audioCache;

    // Project state
    juce::ValueTree projectState;

    // Track sources
    juce::OwnedArray<TrackAudioSource> tracks;

    // Playback state
    juce::int64 currentPosition { 0 };
    double currentSampleRate { 44100.0 };
    double projectSampleRate { 44100.0 };
    int samplesPerBlock { 512 };
    bool looping { false };
    juce::int64 loopStart { 0 };
    juce::int64 loopEnd { 0 };

    // Master output
    float masterVolume { 1.0f };
    float masterPan { 0.0f };

    // Temporary buffer for mixing
    juce::AudioBuffer<float> mixBuffer;

    // Thread safety
    juce::CriticalSection lock;

    // Update solo state across all tracks
    void updateSoloState();

    // Apply master pan
    void applyMasterPan(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiTrackAudioSource)
};

/*
  ==============================================================================

    ProjectModel.h

    Multi-track project data model using JUCE ValueTree
    Defines the schema for Project, Track, and Clip structures

  ==============================================================================
*/

#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_graphics/juce_graphics.h>

//==============================================================================
// ValueTree Type Identifiers
//==============================================================================

namespace IDs
{
    // Node types
    const juce::Identifier PROJECT     { "PROJECT" };
    const juce::Identifier TRACK       { "TRACK" };
    const juce::Identifier CLIP        { "CLIP" };
    const juce::Identifier MASTER      { "MASTER" };

    // Project properties
    const juce::Identifier projectName     { "projectName" };
    const juce::Identifier sampleRate      { "sampleRate" };
    const juce::Identifier bpm             { "bpm" };
    const juce::Identifier timeSignatureNum { "timeSignatureNum" };
    const juce::Identifier timeSignatureDen { "timeSignatureDen" };
    const juce::Identifier createdAt       { "createdAt" };
    const juce::Identifier modifiedAt      { "modifiedAt" };

    // Track properties
    const juce::Identifier name            { "name" };
    const juce::Identifier color           { "color" };
    const juce::Identifier volume          { "volume" };
    const juce::Identifier pan             { "pan" };
    const juce::Identifier mute            { "mute" };
    const juce::Identifier solo            { "solo" };
    const juce::Identifier order           { "order" };
    const juce::Identifier trackId         { "trackId" };
    const juce::Identifier armed           { "armed" };
    const juce::Identifier inputChannel    { "inputChannel" };
    const juce::Identifier outputChannel   { "outputChannel" };

    // Clip properties
    const juce::Identifier clipId          { "clipId" };
    const juce::Identifier audioFilePath   { "audioFilePath" };
    const juce::Identifier audioFileId     { "audioFileId" };
    const juce::Identifier sourceStart     { "sourceStart" };      // In samples
    const juce::Identifier length          { "length" };           // In samples
    const juce::Identifier timelineStart   { "timelineStart" };    // In samples
    const juce::Identifier gain            { "gain" };             // Linear 0.0 - 2.0
    const juce::Identifier fadeInSamples   { "fadeInSamples" };
    const juce::Identifier fadeOutSamples  { "fadeOutSamples" };
    const juce::Identifier fadeInCurve     { "fadeInCurve" };      // 0=linear, 1=exp, 2=log
    const juce::Identifier fadeOutCurve    { "fadeOutCurve" };
    const juce::Identifier clipName        { "clipName" };
    const juce::Identifier clipColor       { "clipColor" };
    const juce::Identifier locked          { "locked" };

    // Master properties
    const juce::Identifier masterVolume    { "masterVolume" };
    const juce::Identifier masterPan       { "masterPan" };
}

//==============================================================================
// Clip Model
//==============================================================================

class ClipModel
{
public:
    ClipModel() = default;
    explicit ClipModel(const juce::ValueTree& tree) : state(tree) {}

    // Factory method to create a new clip
    static juce::ValueTree createClip(const juce::String& filePath,
                                       juce::int64 timelineStartSamples,
                                       juce::int64 lengthSamples,
                                       juce::int64 sourceStartSamples = 0);

    // Property accessors
    juce::String getAudioFilePath() const { return state[IDs::audioFilePath].toString(); }
    void setAudioFilePath(const juce::String& path) { state.setProperty(IDs::audioFilePath, path, nullptr); }

    juce::int64 getSourceStart() const { return static_cast<juce::int64>(state[IDs::sourceStart]); }
    void setSourceStart(juce::int64 samples, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::sourceStart, samples, undo); }

    juce::int64 getLength() const { return static_cast<juce::int64>(state[IDs::length]); }
    void setLength(juce::int64 samples, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::length, samples, undo); }

    juce::int64 getTimelineStart() const { return static_cast<juce::int64>(state[IDs::timelineStart]); }
    void setTimelineStart(juce::int64 samples, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::timelineStart, samples, undo); }

    juce::int64 getTimelineEnd() const { return getTimelineStart() + getLength(); }

    float getGain() const { return static_cast<float>(state[IDs::gain]); }
    void setGain(float gain, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::gain, gain, undo); }

    juce::int64 getFadeInSamples() const { return static_cast<juce::int64>(state[IDs::fadeInSamples]); }
    void setFadeInSamples(juce::int64 samples, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::fadeInSamples, samples, undo); }

    juce::int64 getFadeOutSamples() const { return static_cast<juce::int64>(state[IDs::fadeOutSamples]); }
    void setFadeOutSamples(juce::int64 samples, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::fadeOutSamples, samples, undo); }

    juce::String getClipName() const { return state[IDs::clipName].toString(); }
    void setClipName(const juce::String& name, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::clipName, name, undo); }

    juce::Colour getClipColor() const
    {
        if (state.hasProperty(IDs::clipColor))
            return juce::Colour::fromString(state[IDs::clipColor].toString());
        return juce::Colours::lightblue;
    }
    void setClipColor(juce::Colour color, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::clipColor, color.toString(), undo); }

    bool isLocked() const { return static_cast<bool>(state[IDs::locked]); }
    void setLocked(bool locked, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::locked, locked, undo); }

    juce::String getClipId() const { return state[IDs::clipId].toString(); }

    // Utility
    bool isValid() const { return state.isValid() && state.hasType(IDs::CLIP); }
    juce::ValueTree& getState() { return state; }
    const juce::ValueTree& getState() const { return state; }

    // Split clip at given sample position (relative to timeline)
    // Returns the new clip created from the right portion
    static juce::ValueTree splitClip(juce::ValueTree clipTree,
                                      juce::int64 splitPositionSamples,
                                      juce::UndoManager* undo = nullptr);

private:
    juce::ValueTree state;
};

//==============================================================================
// Track Model
//==============================================================================

class TrackModel
{
public:
    TrackModel() = default;
    explicit TrackModel(const juce::ValueTree& tree) : state(tree) {}

    // Factory method to create a new track
    static juce::ValueTree createTrack(const juce::String& name, int order = 0);

    // Property accessors
    juce::String getName() const { return state[IDs::name].toString(); }
    void setName(const juce::String& name, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::name, name, undo); }

    juce::Colour getColor() const
    {
        if (state.hasProperty(IDs::color))
            return juce::Colour::fromString(state[IDs::color].toString());
        return juce::Colours::grey;
    }
    void setColor(juce::Colour color, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::color, color.toString(), undo); }

    float getVolume() const { return static_cast<float>(state[IDs::volume]); }
    void setVolume(float volume, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::volume, juce::jlimit(0.0f, 2.0f, volume), undo); }

    float getPan() const { return static_cast<float>(state[IDs::pan]); }
    void setPan(float pan, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::pan, juce::jlimit(-1.0f, 1.0f, pan), undo); }

    bool isMuted() const { return static_cast<bool>(state[IDs::mute]); }
    void setMuted(bool muted, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::mute, muted, undo); }

    bool isSoloed() const { return static_cast<bool>(state[IDs::solo]); }
    void setSoloed(bool soloed, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::solo, soloed, undo); }

    bool isArmed() const { return static_cast<bool>(state[IDs::armed]); }
    void setArmed(bool armed, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::armed, armed, undo); }

    int getOrder() const { return static_cast<int>(state[IDs::order]); }
    void setOrder(int order, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::order, order, undo); }

    juce::String getTrackId() const { return state[IDs::trackId].toString(); }

    // Clip management
    int getNumClips() const { return state.getNumChildren(); }

    juce::ValueTree getClip(int index) const
    {
        return state.getChild(index);
    }

    ClipModel getClipModel(int index) const
    {
        return ClipModel(state.getChild(index));
    }

    void addClip(const juce::ValueTree& clip, juce::UndoManager* undo = nullptr)
    {
        state.addChild(clip, -1, undo);
    }

    void removeClip(int index, juce::UndoManager* undo = nullptr)
    {
        state.removeChild(index, undo);
    }

    void removeClip(const juce::ValueTree& clip, juce::UndoManager* undo = nullptr)
    {
        state.removeChild(clip, undo);
    }

    // Find clip at timeline position
    juce::ValueTree findClipAt(juce::int64 timelineSamples) const;

    // Get all clips sorted by timeline position
    juce::Array<juce::ValueTree> getClipsSortedByTime() const;

    // Utility
    bool isValid() const { return state.isValid() && state.hasType(IDs::TRACK); }
    juce::ValueTree& getState() { return state; }
    const juce::ValueTree& getState() const { return state; }

private:
    juce::ValueTree state;
};

//==============================================================================
// Project Model
//==============================================================================

class ProjectModel
{
public:
    ProjectModel();
    explicit ProjectModel(const juce::ValueTree& tree);

    // Factory method to create a new project
    static juce::ValueTree createProject(const juce::String& name,
                                          double sampleRate = 44100.0,
                                          double bpm = 120.0);

    // Project properties
    juce::String getProjectName() const { return state[IDs::projectName].toString(); }
    void setProjectName(const juce::String& name, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::projectName, name, undo); }

    double getSampleRate() const { return static_cast<double>(state[IDs::sampleRate]); }
    void setSampleRate(double rate, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::sampleRate, rate, undo); }

    double getBpm() const { return static_cast<double>(state[IDs::bpm]); }
    void setBpm(double bpm, juce::UndoManager* undo = nullptr)
    { state.setProperty(IDs::bpm, bpm, undo); }

    int getTimeSignatureNumerator() const { return static_cast<int>(state[IDs::timeSignatureNum]); }
    int getTimeSignatureDenominator() const { return static_cast<int>(state[IDs::timeSignatureDen]); }
    void setTimeSignature(int numerator, int denominator, juce::UndoManager* undo = nullptr)
    {
        state.setProperty(IDs::timeSignatureNum, numerator, undo);
        state.setProperty(IDs::timeSignatureDen, denominator, undo);
    }

    // Master properties
    float getMasterVolume() const;
    void setMasterVolume(float volume, juce::UndoManager* undo = nullptr);

    float getMasterPan() const;
    void setMasterPan(float pan, juce::UndoManager* undo = nullptr);

    // Track management
    int getNumTracks() const;
    juce::ValueTree getTrack(int index) const;
    TrackModel getTrackModel(int index) const;

    juce::ValueTree addTrack(const juce::String& name, juce::UndoManager* undo = nullptr);
    void removeTrack(int index, juce::UndoManager* undo = nullptr);
    void removeTrack(const juce::ValueTree& track, juce::UndoManager* undo = nullptr);
    void moveTrack(int fromIndex, int toIndex, juce::UndoManager* undo = nullptr);

    // Find track by ID
    juce::ValueTree findTrackById(const juce::String& trackId) const;

    // Get all tracks sorted by order
    juce::Array<juce::ValueTree> getTracksSortedByOrder() const;

    // Check if any track is soloed
    bool hasAnySoloedTrack() const;

    // Timeline utilities
    juce::int64 getProjectLength() const;  // In samples (end of last clip)

    // Time conversion utilities
    double samplesToSeconds(juce::int64 samples) const { return samples / getSampleRate(); }
    juce::int64 secondsToSamples(double seconds) const { return static_cast<juce::int64>(seconds * getSampleRate()); }
    double samplesToBars(juce::int64 samples) const;
    juce::int64 barsToSamples(double bars) const;
    juce::int64 beatsToSamples(double beats) const;

    // Utility
    bool isValid() const { return state.isValid() && state.hasType(IDs::PROJECT); }
    juce::ValueTree& getState() { return state; }
    const juce::ValueTree& getState() const { return state; }

    // Get/Create master node
    juce::ValueTree getMasterNode() const;

private:
    juce::ValueTree state;

    void ensureMasterNode();
};

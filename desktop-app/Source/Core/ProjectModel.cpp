/*
  ==============================================================================

    ProjectModel.cpp

    Multi-track project data model implementation

  ==============================================================================
*/

#include "ProjectModel.h"
#include <algorithm>

//==============================================================================
// Utility: Generate unique ID
//==============================================================================

static juce::String generateUniqueId()
{
    return juce::Uuid().toString();
}

//==============================================================================
// ClipModel Implementation
//==============================================================================

juce::ValueTree ClipModel::createClip(const juce::String& filePath,
                                       juce::int64 timelineStartSamples,
                                       juce::int64 lengthSamples,
                                       juce::int64 sourceStartSamples)
{
    juce::ValueTree clip(IDs::CLIP);

    clip.setProperty(IDs::clipId, generateUniqueId(), nullptr);
    clip.setProperty(IDs::audioFilePath, filePath, nullptr);
    clip.setProperty(IDs::sourceStart, sourceStartSamples, nullptr);
    clip.setProperty(IDs::length, lengthSamples, nullptr);
    clip.setProperty(IDs::timelineStart, timelineStartSamples, nullptr);
    clip.setProperty(IDs::gain, 1.0f, nullptr);
    clip.setProperty(IDs::fadeInSamples, static_cast<juce::int64>(0), nullptr);
    clip.setProperty(IDs::fadeOutSamples, static_cast<juce::int64>(0), nullptr);
    clip.setProperty(IDs::fadeInCurve, 0, nullptr);   // Linear
    clip.setProperty(IDs::fadeOutCurve, 0, nullptr);  // Linear
    clip.setProperty(IDs::locked, false, nullptr);

    // Extract filename for clip name
    juce::File file(filePath);
    clip.setProperty(IDs::clipName, file.getFileNameWithoutExtension(), nullptr);

    return clip;
}

juce::ValueTree ClipModel::splitClip(juce::ValueTree clipTree,
                                      juce::int64 splitPositionSamples,
                                      juce::UndoManager* undo)
{
    ClipModel clip(clipTree);

    juce::int64 clipStart = clip.getTimelineStart();
    juce::int64 clipEnd = clip.getTimelineEnd();

    // Validate split position
    if (splitPositionSamples <= clipStart || splitPositionSamples >= clipEnd)
        return juce::ValueTree();  // Invalid split position

    // Calculate new lengths
    juce::int64 leftLength = splitPositionSamples - clipStart;
    juce::int64 rightLength = clipEnd - splitPositionSamples;

    // Calculate source offset for right clip
    juce::int64 rightSourceStart = clip.getSourceStart() + leftLength;

    // Modify original clip (becomes left clip)
    clip.setLength(leftLength, undo);

    // Adjust fade out - move to right clip if it was at the end
    juce::int64 originalFadeOut = clip.getFadeOutSamples();
    clip.setFadeOutSamples(0, undo);

    // Create right clip
    juce::ValueTree rightClip = createClip(
        clip.getAudioFilePath(),
        splitPositionSamples,
        rightLength,
        rightSourceStart
    );

    // Copy properties to right clip
    ClipModel rightClipModel(rightClip);
    rightClipModel.setGain(clip.getGain());
    rightClipModel.setClipColor(clip.getClipColor());
    rightClipModel.setClipName(clip.getClipName() + " (2)");
    rightClipModel.setFadeOutSamples(originalFadeOut);

    return rightClip;
}

//==============================================================================
// TrackModel Implementation
//==============================================================================

juce::ValueTree TrackModel::createTrack(const juce::String& name, int order)
{
    juce::ValueTree track(IDs::TRACK);

    track.setProperty(IDs::trackId, generateUniqueId(), nullptr);
    track.setProperty(IDs::name, name, nullptr);
    track.setProperty(IDs::volume, 1.0f, nullptr);
    track.setProperty(IDs::pan, 0.0f, nullptr);
    track.setProperty(IDs::mute, false, nullptr);
    track.setProperty(IDs::solo, false, nullptr);
    track.setProperty(IDs::armed, false, nullptr);
    track.setProperty(IDs::order, order, nullptr);
    track.setProperty(IDs::inputChannel, -1, nullptr);   // -1 = none
    track.setProperty(IDs::outputChannel, 0, nullptr);   // 0 = master

    // Generate a random color for the track
    juce::Random random;
    juce::Colour trackColor = juce::Colour::fromHSV(
        random.nextFloat(),
        0.5f + random.nextFloat() * 0.3f,
        0.7f + random.nextFloat() * 0.2f,
        1.0f
    );
    track.setProperty(IDs::color, trackColor.toString(), nullptr);

    return track;
}

juce::ValueTree TrackModel::findClipAt(juce::int64 timelineSamples) const
{
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto clip = state.getChild(i);
        if (clip.hasType(IDs::CLIP))
        {
            ClipModel clipModel(clip);
            if (timelineSamples >= clipModel.getTimelineStart() &&
                timelineSamples < clipModel.getTimelineEnd())
            {
                return clip;
            }
        }
    }
    return juce::ValueTree();
}

juce::Array<juce::ValueTree> TrackModel::getClipsSortedByTime() const
{
    juce::Array<juce::ValueTree> clips;

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild(i);
        if (child.hasType(IDs::CLIP))
            clips.add(child);
    }

    // Sort by timeline start position using std::sort
    std::sort(clips.begin(), clips.end(), [](const juce::ValueTree& a, const juce::ValueTree& b)
    {
        juce::int64 aStart = static_cast<juce::int64>(a[IDs::timelineStart]);
        juce::int64 bStart = static_cast<juce::int64>(b[IDs::timelineStart]);
        return aStart < bStart;
    });

    return clips;
}

//==============================================================================
// ProjectModel Implementation
//==============================================================================

ProjectModel::ProjectModel()
    : state(IDs::PROJECT)
{
    state.setProperty(IDs::projectName, "Untitled Project", nullptr);
    state.setProperty(IDs::sampleRate, 44100.0, nullptr);
    state.setProperty(IDs::bpm, 120.0, nullptr);
    state.setProperty(IDs::timeSignatureNum, 4, nullptr);
    state.setProperty(IDs::timeSignatureDen, 4, nullptr);
    state.setProperty(IDs::createdAt, juce::Time::getCurrentTime().toISO8601(true), nullptr);
    state.setProperty(IDs::modifiedAt, juce::Time::getCurrentTime().toISO8601(true), nullptr);

    ensureMasterNode();
}

ProjectModel::ProjectModel(const juce::ValueTree& tree)
    : state(tree)
{
    jassert(tree.hasType(IDs::PROJECT));
    ensureMasterNode();
}

juce::ValueTree ProjectModel::createProject(const juce::String& name,
                                             double sampleRate,
                                             double bpm)
{
    juce::ValueTree project(IDs::PROJECT);

    project.setProperty(IDs::projectName, name, nullptr);
    project.setProperty(IDs::sampleRate, sampleRate, nullptr);
    project.setProperty(IDs::bpm, bpm, nullptr);
    project.setProperty(IDs::timeSignatureNum, 4, nullptr);
    project.setProperty(IDs::timeSignatureDen, 4, nullptr);
    project.setProperty(IDs::createdAt, juce::Time::getCurrentTime().toISO8601(true), nullptr);
    project.setProperty(IDs::modifiedAt, juce::Time::getCurrentTime().toISO8601(true), nullptr);

    // Create master node
    juce::ValueTree master(IDs::MASTER);
    master.setProperty(IDs::masterVolume, 1.0f, nullptr);
    master.setProperty(IDs::masterPan, 0.0f, nullptr);
    project.addChild(master, 0, nullptr);

    return project;
}

void ProjectModel::ensureMasterNode()
{
    auto master = state.getChildWithName(IDs::MASTER);
    if (!master.isValid())
    {
        juce::ValueTree masterNode(IDs::MASTER);
        masterNode.setProperty(IDs::masterVolume, 1.0f, nullptr);
        masterNode.setProperty(IDs::masterPan, 0.0f, nullptr);
        state.addChild(masterNode, 0, nullptr);
    }
}

juce::ValueTree ProjectModel::getMasterNode() const
{
    return state.getChildWithName(IDs::MASTER);
}

float ProjectModel::getMasterVolume() const
{
    auto master = getMasterNode();
    if (master.isValid())
        return static_cast<float>(master[IDs::masterVolume]);
    return 1.0f;
}

void ProjectModel::setMasterVolume(float volume, juce::UndoManager* undo)
{
    auto master = getMasterNode();
    if (master.isValid())
        master.setProperty(IDs::masterVolume, juce::jlimit(0.0f, 2.0f, volume), undo);
}

float ProjectModel::getMasterPan() const
{
    auto master = getMasterNode();
    if (master.isValid())
        return static_cast<float>(master[IDs::masterPan]);
    return 0.0f;
}

void ProjectModel::setMasterPan(float pan, juce::UndoManager* undo)
{
    auto master = getMasterNode();
    if (master.isValid())
        master.setProperty(IDs::masterPan, juce::jlimit(-1.0f, 1.0f, pan), undo);
}

int ProjectModel::getNumTracks() const
{
    int count = 0;
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        if (state.getChild(i).hasType(IDs::TRACK))
            ++count;
    }
    return count;
}

juce::ValueTree ProjectModel::getTrack(int index) const
{
    int trackIndex = 0;
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild(i);
        if (child.hasType(IDs::TRACK))
        {
            if (trackIndex == index)
                return child;
            ++trackIndex;
        }
    }
    return juce::ValueTree();
}

TrackModel ProjectModel::getTrackModel(int index) const
{
    return TrackModel(getTrack(index));
}

juce::ValueTree ProjectModel::addTrack(const juce::String& name, juce::UndoManager* undo)
{
    int newOrder = getNumTracks();
    auto track = TrackModel::createTrack(name, newOrder);
    state.addChild(track, -1, undo);

    // Update modified time
    state.setProperty(IDs::modifiedAt, juce::Time::getCurrentTime().toISO8601(true), nullptr);

    return track;
}

void ProjectModel::removeTrack(int index, juce::UndoManager* undo)
{
    auto track = getTrack(index);
    if (track.isValid())
    {
        state.removeChild(track, undo);
        state.setProperty(IDs::modifiedAt, juce::Time::getCurrentTime().toISO8601(true), nullptr);
    }
}

void ProjectModel::removeTrack(const juce::ValueTree& track, juce::UndoManager* undo)
{
    state.removeChild(track, undo);
    state.setProperty(IDs::modifiedAt, juce::Time::getCurrentTime().toISO8601(true), nullptr);
}

void ProjectModel::moveTrack(int fromIndex, int toIndex, juce::UndoManager* undo)
{
    if (fromIndex == toIndex)
        return;

    auto track = getTrack(fromIndex);
    if (!track.isValid())
        return;

    // Remove and re-insert at new position
    state.removeChild(track, undo);

    // Calculate actual child index (accounting for MASTER node)
    int insertIndex = 0;
    int trackCount = 0;
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        if (state.getChild(i).hasType(IDs::TRACK))
        {
            if (trackCount == toIndex)
            {
                insertIndex = i;
                break;
            }
            ++trackCount;
        }
        insertIndex = i + 1;
    }

    state.addChild(track, insertIndex, undo);

    // Update order properties for all tracks
    auto tracks = getTracksSortedByOrder();
    for (int i = 0; i < tracks.size(); ++i)
    {
        TrackModel(tracks[i]).setOrder(i, nullptr);
    }

    state.setProperty(IDs::modifiedAt, juce::Time::getCurrentTime().toISO8601(true), nullptr);
}

juce::ValueTree ProjectModel::findTrackById(const juce::String& trackId) const
{
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild(i);
        if (child.hasType(IDs::TRACK) && child[IDs::trackId].toString() == trackId)
            return child;
    }
    return juce::ValueTree();
}

juce::Array<juce::ValueTree> ProjectModel::getTracksSortedByOrder() const
{
    juce::Array<juce::ValueTree> tracks;

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild(i);
        if (child.hasType(IDs::TRACK))
            tracks.add(child);
    }

    // Sort by order property using std::sort
    std::sort(tracks.begin(), tracks.end(), [](const juce::ValueTree& a, const juce::ValueTree& b)
    {
        int aOrder = static_cast<int>(a[IDs::order]);
        int bOrder = static_cast<int>(b[IDs::order]);
        return aOrder < bOrder;
    });

    return tracks;
}

bool ProjectModel::hasAnySoloedTrack() const
{
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild(i);
        if (child.hasType(IDs::TRACK) && static_cast<bool>(child[IDs::solo]))
            return true;
    }
    return false;
}

juce::int64 ProjectModel::getProjectLength() const
{
    juce::int64 maxEnd = 0;

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild(i);
        if (child.hasType(IDs::TRACK))
        {
            TrackModel track(child);
            for (int j = 0; j < track.getNumClips(); ++j)
            {
                ClipModel clip = track.getClipModel(j);
                juce::int64 clipEnd = clip.getTimelineEnd();
                if (clipEnd > maxEnd)
                    maxEnd = clipEnd;
            }
        }
    }

    return maxEnd;
}

double ProjectModel::samplesToBars(juce::int64 samples) const
{
    double sampleRate = getSampleRate();
    double bpm = getBpm();
    int beatsPerBar = getTimeSignatureNumerator();

    double seconds = samples / sampleRate;
    double beats = seconds * (bpm / 60.0);
    return beats / beatsPerBar;
}

juce::int64 ProjectModel::barsToSamples(double bars) const
{
    double sampleRate = getSampleRate();
    double bpm = getBpm();
    int beatsPerBar = getTimeSignatureNumerator();

    double beats = bars * beatsPerBar;
    double seconds = beats * (60.0 / bpm);
    return static_cast<juce::int64>(seconds * sampleRate);
}

juce::int64 ProjectModel::beatsToSamples(double beats) const
{
    double sampleRate = getSampleRate();
    double bpm = getBpm();

    double seconds = beats * (60.0 / bpm);
    return static_cast<juce::int64>(seconds * sampleRate);
}

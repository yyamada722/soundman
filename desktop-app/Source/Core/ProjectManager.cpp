/*
  ==============================================================================

    ProjectManager.cpp

    Project state management implementation

  ==============================================================================
*/

#include "ProjectManager.h"

//==============================================================================
ProjectManager::ProjectManager()
{
    project.getState().addListener(&valueTreeListener);
}

ProjectManager::~ProjectManager()
{
    project.getState().removeListener(&valueTreeListener);
}

//==============================================================================
// Project File Operations
//==============================================================================

void ProjectManager::newProject(const juce::String& name)
{
    // Remove listener from old project
    project.getState().removeListener(&valueTreeListener);

    // Create new project
    project = ProjectModel(ProjectModel::createProject(name));

    // Add listener to new project
    project.getState().addListener(&valueTreeListener);

    // Clear state
    currentProjectFile = juce::File();
    projectModified = false;
    undoManager.clearUndoHistory();

    notifyProjectChanged();
    sendChangeMessage();
}

bool ProjectManager::saveProject(const juce::File& file)
{
    if (file == juce::File())
    {
        if (!currentProjectFile.existsAsFile())
            return false;
        return saveProjectAs(currentProjectFile);
    }
    return saveProjectAs(file);
}

bool ProjectManager::saveProjectAs(const juce::File& file)
{
    juce::File targetFile = file;

    // Ensure correct extension
    if (!targetFile.hasFileExtension(PROJECT_FILE_EXTENSION))
        targetFile = targetFile.withFileExtension(PROJECT_FILE_EXTENSION);

    // Serialize project to XML
    juce::String xmlContent = serializeToXml();

    // Write to file
    if (!targetFile.replaceWithText(xmlContent))
        return false;

    currentProjectFile = targetFile;
    markAsSaved();

    return true;
}

bool ProjectManager::loadProject(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    // Read file content
    juce::String xmlContent = file.loadFileAsString();

    if (xmlContent.isEmpty())
        return false;

    // Deserialize
    if (!deserializeFromXml(xmlContent))
        return false;

    currentProjectFile = file;
    projectModified = false;
    undoManager.clearUndoHistory();

    notifyProjectChanged();
    sendChangeMessage();

    return true;
}

juce::String ProjectManager::serializeToXml() const
{
    // Create XML document
    auto xml = project.getState().createXml();

    if (xml == nullptr)
        return {};

    // Add file version attribute
    xml->setAttribute("fileVersion", PROJECT_FILE_VERSION);

    return xml->toString();
}

bool ProjectManager::deserializeFromXml(const juce::String& xmlString)
{
    auto xml = juce::XmlDocument::parse(xmlString);

    if (xml == nullptr)
        return false;

    // Check file version
    int fileVersion = xml->getIntAttribute("fileVersion", 0);
    if (fileVersion > PROJECT_FILE_VERSION)
    {
        // Future version - might not be compatible
        DBG("Warning: Project file is from a newer version");
    }

    // Convert XML to ValueTree
    juce::ValueTree newState = juce::ValueTree::fromXml(*xml);

    if (!newState.isValid() || !newState.hasType(IDs::PROJECT))
        return false;

    // Remove listener from old project
    project.getState().removeListener(&valueTreeListener);

    // Replace project state
    project = ProjectModel(newState);

    // Add listener to new project
    project.getState().addListener(&valueTreeListener);

    return true;
}

//==============================================================================
// Track Operations
//==============================================================================

juce::ValueTree ProjectManager::addTrack(const juce::String& name)
{
    undoManager.beginNewTransaction("Add Track");
    auto track = project.addTrack(name, &undoManager);
    markAsModified();
    return track;
}

void ProjectManager::removeTrack(int trackIndex)
{
    undoManager.beginNewTransaction("Remove Track");
    project.removeTrack(trackIndex, &undoManager);
    markAsModified();
}

void ProjectManager::removeTrack(const juce::ValueTree& track)
{
    undoManager.beginNewTransaction("Remove Track");
    project.removeTrack(track, &undoManager);
    markAsModified();
}

void ProjectManager::moveTrack(int fromIndex, int toIndex)
{
    undoManager.beginNewTransaction("Move Track");
    project.moveTrack(fromIndex, toIndex, &undoManager);
    markAsModified();
}

//==============================================================================
// Clip Operations
//==============================================================================

juce::ValueTree ProjectManager::addClip(juce::ValueTree& track,
                                         const juce::String& audioFilePath,
                                         juce::int64 timelineStart,
                                         juce::int64 length,
                                         juce::int64 sourceStart)
{
    undoManager.beginNewTransaction("Add Clip");

    auto clip = ClipModel::createClip(audioFilePath, timelineStart, length, sourceStart);
    track.addChild(clip, -1, &undoManager);

    markAsModified();
    return clip;
}

void ProjectManager::removeClip(juce::ValueTree& track, const juce::ValueTree& clip)
{
    undoManager.beginNewTransaction("Remove Clip");
    track.removeChild(clip, &undoManager);
    markAsModified();
}

void ProjectManager::removeClip(juce::ValueTree& track, int clipIndex)
{
    undoManager.beginNewTransaction("Remove Clip");
    track.removeChild(clipIndex, &undoManager);
    markAsModified();
}

void ProjectManager::moveClip(juce::ValueTree& clip, juce::int64 newTimelineStart)
{
    undoManager.beginNewTransaction("Move Clip");
    clip.setProperty(IDs::timelineStart, newTimelineStart, &undoManager);
    markAsModified();
}

void ProjectManager::trimClipStart(juce::ValueTree& clip, juce::int64 newTimelineStart)
{
    undoManager.beginNewTransaction("Trim Clip Start");

    ClipModel clipModel(clip);
    juce::int64 oldStart = clipModel.getTimelineStart();
    juce::int64 delta = newTimelineStart - oldStart;

    // Adjust source start and length
    juce::int64 newSourceStart = clipModel.getSourceStart() + delta;
    juce::int64 newLength = clipModel.getLength() - delta;

    if (newLength > 0 && newSourceStart >= 0)
    {
        clip.setProperty(IDs::timelineStart, newTimelineStart, &undoManager);
        clip.setProperty(IDs::sourceStart, newSourceStart, &undoManager);
        clip.setProperty(IDs::length, newLength, &undoManager);
        markAsModified();
    }
}

void ProjectManager::trimClipEnd(juce::ValueTree& clip, juce::int64 newLength)
{
    undoManager.beginNewTransaction("Trim Clip End");

    if (newLength > 0)
    {
        clip.setProperty(IDs::length, newLength, &undoManager);
        markAsModified();
    }
}

juce::ValueTree ProjectManager::splitClip(juce::ValueTree& track,
                                           juce::ValueTree& clip,
                                           juce::int64 splitPosition)
{
    undoManager.beginNewTransaction("Split Clip");

    auto rightClip = ClipModel::splitClip(clip, splitPosition, &undoManager);

    if (rightClip.isValid())
    {
        track.addChild(rightClip, -1, &undoManager);
        markAsModified();
    }

    return rightClip;
}

//==============================================================================
// Property Change Operations
//==============================================================================

void ProjectManager::setTrackName(juce::ValueTree& track, const juce::String& name)
{
    undoManager.beginNewTransaction("Rename Track");
    track.setProperty(IDs::name, name, &undoManager);
    markAsModified();
}

void ProjectManager::setTrackVolume(juce::ValueTree& track, float volume)
{
    track.setProperty(IDs::volume, juce::jlimit(0.0f, 2.0f, volume), &undoManager);
    markAsModified();
}

void ProjectManager::setTrackPan(juce::ValueTree& track, float pan)
{
    track.setProperty(IDs::pan, juce::jlimit(-1.0f, 1.0f, pan), &undoManager);
    markAsModified();
}

void ProjectManager::setTrackMute(juce::ValueTree& track, bool muted)
{
    undoManager.beginNewTransaction(muted ? "Mute Track" : "Unmute Track");
    track.setProperty(IDs::mute, muted, &undoManager);
    markAsModified();
}

void ProjectManager::setTrackSolo(juce::ValueTree& track, bool soloed)
{
    undoManager.beginNewTransaction(soloed ? "Solo Track" : "Unsolo Track");
    track.setProperty(IDs::solo, soloed, &undoManager);
    markAsModified();
}

void ProjectManager::setTrackColor(juce::ValueTree& track, juce::Colour color)
{
    undoManager.beginNewTransaction("Change Track Color");
    track.setProperty(IDs::color, color.toString(), &undoManager);
    markAsModified();
}

void ProjectManager::setClipGain(juce::ValueTree& clip, float gain)
{
    clip.setProperty(IDs::gain, juce::jlimit(0.0f, 4.0f, gain), &undoManager);
    markAsModified();
}

void ProjectManager::setClipFadeIn(juce::ValueTree& clip, juce::int64 samples)
{
    undoManager.beginNewTransaction("Set Fade In");
    clip.setProperty(IDs::fadeInSamples, samples, &undoManager);
    markAsModified();
}

void ProjectManager::setClipFadeOut(juce::ValueTree& clip, juce::int64 samples)
{
    undoManager.beginNewTransaction("Set Fade Out");
    clip.setProperty(IDs::fadeOutSamples, samples, &undoManager);
    markAsModified();
}

void ProjectManager::setMasterVolume(float volume)
{
    project.setMasterVolume(volume, &undoManager);
    markAsModified();
}

void ProjectManager::setMasterPan(float pan)
{
    project.setMasterPan(pan, &undoManager);
    markAsModified();
}

void ProjectManager::setBpm(double bpm)
{
    undoManager.beginNewTransaction("Change BPM");
    project.setBpm(bpm, &undoManager);
    markAsModified();
}

void ProjectManager::setTimeSignature(int numerator, int denominator)
{
    undoManager.beginNewTransaction("Change Time Signature");
    project.setTimeSignature(numerator, denominator, &undoManager);
    markAsModified();
}

//==============================================================================
// ValueTree Listener Implementation
//==============================================================================

void ProjectManager::ValueTreeListener::valueTreePropertyChanged(juce::ValueTree& tree,
                                                                   const juce::Identifier& property)
{
    if (tree.hasType(IDs::TRACK))
        owner.notifyTrackPropertyChanged(tree, property);
    else if (tree.hasType(IDs::CLIP))
        owner.notifyClipPropertyChanged(tree, property);

    owner.markAsModified();
}

void ProjectManager::ValueTreeListener::valueTreeChildAdded(juce::ValueTree& parent,
                                                             juce::ValueTree& child)
{
    if (child.hasType(IDs::TRACK))
        owner.notifyTrackAdded(child);
    else if (child.hasType(IDs::CLIP))
        owner.notifyClipAdded(child);

    owner.markAsModified();
}

void ProjectManager::ValueTreeListener::valueTreeChildRemoved(juce::ValueTree& parent,
                                                               juce::ValueTree& child,
                                                               int index)
{
    juce::ignoreUnused(parent, index);

    if (child.hasType(IDs::TRACK))
        owner.notifyTrackRemoved(child);
    else if (child.hasType(IDs::CLIP))
        owner.notifyClipRemoved(child);

    owner.markAsModified();
}

void ProjectManager::ValueTreeListener::valueTreeChildOrderChanged(juce::ValueTree& parent,
                                                                     int oldIndex, int newIndex)
{
    juce::ignoreUnused(parent, oldIndex, newIndex);
    owner.markAsModified();
}

void ProjectManager::ValueTreeListener::valueTreeParentChanged(juce::ValueTree& tree)
{
    juce::ignoreUnused(tree);
}

//==============================================================================
// Notification Methods
//==============================================================================

void ProjectManager::notifyProjectChanged()
{
    listeners.call([](Listener& l) { l.projectChanged(); });
}

void ProjectManager::notifyTrackAdded(const juce::ValueTree& track)
{
    listeners.call([&track](Listener& l) { l.trackAdded(track); });
}

void ProjectManager::notifyTrackRemoved(const juce::ValueTree& track)
{
    listeners.call([&track](Listener& l) { l.trackRemoved(track); });
}

void ProjectManager::notifyTrackPropertyChanged(const juce::ValueTree& track,
                                                  const juce::Identifier& property)
{
    listeners.call([&track, &property](Listener& l) { l.trackPropertyChanged(track, property); });
}

void ProjectManager::notifyClipAdded(const juce::ValueTree& clip)
{
    listeners.call([&clip](Listener& l) { l.clipAdded(clip); });
}

void ProjectManager::notifyClipRemoved(const juce::ValueTree& clip)
{
    listeners.call([&clip](Listener& l) { l.clipRemoved(clip); });
}

void ProjectManager::notifyClipPropertyChanged(const juce::ValueTree& clip,
                                                 const juce::Identifier& property)
{
    listeners.call([&clip, &property](Listener& l) { l.clipPropertyChanged(clip, property); });
}

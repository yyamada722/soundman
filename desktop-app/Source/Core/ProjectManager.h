/*
  ==============================================================================

    ProjectManager.h

    Manages the project state, undo/redo, and file I/O
    Implements T1-2 (UndoManager) and T1-3 (Save/Load)

  ==============================================================================
*/

#pragma once

#include "ProjectModel.h"

//==============================================================================
// Project Manager
//==============================================================================

class ProjectManager : public juce::ChangeBroadcaster
{
public:
    //==========================================================================
    ProjectManager();
    ~ProjectManager() override;

    //==========================================================================
    // Project State Access
    //==========================================================================

    ProjectModel& getProject() { return project; }
    const ProjectModel& getProject() const { return project; }

    juce::ValueTree& getProjectState() { return project.getState(); }
    const juce::ValueTree& getProjectState() const { return project.getState(); }

    //==========================================================================
    // Project File Operations (T1-3)
    //==========================================================================

    // Create a new empty project
    void newProject(const juce::String& name = "Untitled Project");

    // Save project to file
    bool saveProject(const juce::File& file);
    bool saveProjectAs(const juce::File& file);

    // Load project from file
    bool loadProject(const juce::File& file);

    // Get current project file
    juce::File getProjectFile() const { return currentProjectFile; }
    bool hasProjectFile() const { return currentProjectFile.existsAsFile(); }

    // Check if project has unsaved changes
    bool hasUnsavedChanges() const { return projectModified; }
    void markAsModified() { projectModified = true; sendChangeMessage(); }
    void markAsSaved() { projectModified = false; sendChangeMessage(); }

    //==========================================================================
    // Undo/Redo (T1-2)
    //==========================================================================

    juce::UndoManager& getUndoManager() { return undoManager; }

    bool canUndo() const { return undoManager.canUndo(); }
    bool canRedo() const { return undoManager.canRedo(); }

    bool undo() { return undoManager.undo(); }
    bool redo() { return undoManager.redo(); }

    juce::String getUndoDescription() const { return undoManager.getUndoDescription(); }
    juce::String getRedoDescription() const { return undoManager.getRedoDescription(); }

    void clearUndoHistory() { undoManager.clearUndoHistory(); }

    // Begin/End transaction for grouping multiple operations
    void beginTransaction(const juce::String& name = juce::String())
    { undoManager.beginNewTransaction(name); }

    //==========================================================================
    // Track Operations (with Undo support)
    //==========================================================================

    juce::ValueTree addTrack(const juce::String& name = "New Track");
    void removeTrack(int trackIndex);
    void removeTrack(const juce::ValueTree& track);
    void moveTrack(int fromIndex, int toIndex);

    //==========================================================================
    // Clip Operations (with Undo support)
    //==========================================================================

    juce::ValueTree addClip(juce::ValueTree& track,
                            const juce::String& audioFilePath,
                            juce::int64 timelineStart,
                            juce::int64 length,
                            juce::int64 sourceStart = 0);

    void removeClip(juce::ValueTree& track, const juce::ValueTree& clip);
    void removeClip(juce::ValueTree& track, int clipIndex);

    void moveClip(juce::ValueTree& clip, juce::int64 newTimelineStart);
    void trimClipStart(juce::ValueTree& clip, juce::int64 newTimelineStart);
    void trimClipEnd(juce::ValueTree& clip, juce::int64 newLength);

    juce::ValueTree splitClip(juce::ValueTree& track,
                              juce::ValueTree& clip,
                              juce::int64 splitPosition);

    //==========================================================================
    // Property Change Operations (with Undo support)
    //==========================================================================

    void setTrackName(juce::ValueTree& track, const juce::String& name);
    void setTrackVolume(juce::ValueTree& track, float volume);
    void setTrackPan(juce::ValueTree& track, float pan);
    void setTrackMute(juce::ValueTree& track, bool muted);
    void setTrackSolo(juce::ValueTree& track, bool soloed);
    void setTrackColor(juce::ValueTree& track, juce::Colour color);

    void setClipGain(juce::ValueTree& clip, float gain);
    void setClipFadeIn(juce::ValueTree& clip, juce::int64 samples);
    void setClipFadeOut(juce::ValueTree& clip, juce::int64 samples);

    void setMasterVolume(float volume);
    void setMasterPan(float pan);
    void setBpm(double bpm);
    void setTimeSignature(int numerator, int denominator);

    //==========================================================================
    // ValueTree Listener for tracking changes
    //==========================================================================

    class Listener
    {
    public:
        virtual ~Listener() = default;

        virtual void projectChanged() {}
        virtual void trackAdded(const juce::ValueTree& track) { juce::ignoreUnused(track); }
        virtual void trackRemoved(const juce::ValueTree& track) { juce::ignoreUnused(track); }
        virtual void trackPropertyChanged(const juce::ValueTree& track, const juce::Identifier& property)
        { juce::ignoreUnused(track, property); }

        virtual void clipAdded(const juce::ValueTree& clip) { juce::ignoreUnused(clip); }
        virtual void clipRemoved(const juce::ValueTree& clip) { juce::ignoreUnused(clip); }
        virtual void clipPropertyChanged(const juce::ValueTree& clip, const juce::Identifier& property)
        { juce::ignoreUnused(clip, property); }
    };

    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }

private:
    //==========================================================================
    ProjectModel project;
    juce::UndoManager undoManager;

    juce::File currentProjectFile;
    bool projectModified { false };

    juce::ListenerList<Listener> listeners;

    // Internal ValueTree listener
    class ValueTreeListener : public juce::ValueTree::Listener
    {
    public:
        ValueTreeListener(ProjectManager& pm) : owner(pm) {}

        void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
        void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
        void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
        void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;
        void valueTreeParentChanged(juce::ValueTree& tree) override;

    private:
        ProjectManager& owner;
    };

    ValueTreeListener valueTreeListener { *this };

    // Serialization helpers
    static constexpr const char* PROJECT_FILE_EXTENSION = ".smproj";
    static constexpr int PROJECT_FILE_VERSION = 1;

    juce::String serializeToXml() const;
    bool deserializeFromXml(const juce::String& xmlString);

    void notifyProjectChanged();
    void notifyTrackAdded(const juce::ValueTree& track);
    void notifyTrackRemoved(const juce::ValueTree& track);
    void notifyTrackPropertyChanged(const juce::ValueTree& track, const juce::Identifier& property);
    void notifyClipAdded(const juce::ValueTree& clip);
    void notifyClipRemoved(const juce::ValueTree& clip);
    void notifyClipPropertyChanged(const juce::ValueTree& clip, const juce::Identifier& property);

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManager)
};

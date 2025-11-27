/*
  ==============================================================================

    PlaylistPanel.h

    Playlist management panel with file queue and auto-advance

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>

class PlaylistPanel : public juce::Component
{
public:
    //==========================================================================
    struct PlaylistItem
    {
        juce::File file;
        juce::String displayName;
        double duration { 0.0 };
        bool played { false };
    };

    //==========================================================================
    PlaylistPanel();
    ~PlaylistPanel() override;

    //==========================================================================
    // Playlist management
    void addFile(const juce::File& file);
    void addFiles(const juce::Array<juce::File>& files);
    void removeSelected();
    void clearPlaylist();
    void moveUp();
    void moveDown();

    //==========================================================================
    // Playback control
    void setCurrentIndex(int index);
    int getCurrentIndex() const { return currentIndex; }
    juce::File getCurrentFile() const;
    juce::File getNextFile() const;
    bool hasNext() const;

    //==========================================================================
    // Auto-advance
    void setAutoAdvance(bool enabled) { autoAdvance = enabled; }
    bool isAutoAdvanceEnabled() const { return autoAdvance; }

    //==========================================================================
    // Callbacks
    using FileSelectedCallback = std::function<void(const juce::File&)>;
    using PlaylistChangedCallback = std::function<void()>;

    void setFileSelectedCallback(FileSelectedCallback callback) { fileSelectedCallback = callback; }
    void setPlaylistChangedCallback(PlaylistChangedCallback callback) { playlistChangedCallback = callback; }

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    class PlaylistListBox : public juce::ListBox,
                           public juce::ListBoxModel
    {
    public:
        PlaylistListBox(PlaylistPanel& owner);

        // ListBoxModel implementation
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

    private:
        PlaylistPanel& owner;
    };

    //==========================================================================
    void updatePlaylist();
    void selectAndPlay(int index);

    //==========================================================================
    juce::Label titleLabel;
    PlaylistListBox playlistListBox;

    juce::TextButton addButton;
    juce::TextButton removeButton;
    juce::TextButton clearButton;
    juce::TextButton moveUpButton;
    juce::TextButton moveDownButton;
    juce::ToggleButton autoAdvanceButton;

    std::vector<PlaylistItem> playlistItems;
    int currentIndex { -1 };
    bool autoAdvance { true };

    FileSelectedCallback fileSelectedCallback;
    PlaylistChangedCallback playlistChangedCallback;

    std::unique_ptr<juce::FileChooser> fileChooser;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistPanel)
};

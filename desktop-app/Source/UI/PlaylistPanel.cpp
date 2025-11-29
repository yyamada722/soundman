/*
  ==============================================================================

    PlaylistPanel.cpp

    Playlist management panel implementation

  ==============================================================================
*/

#include "PlaylistPanel.h"

//==============================================================================
// Japanese font helper
static juce::Font getJapaneseFont(float height, int style = juce::Font::plain)
{
    static juce::String cachedFontName;

    if (cachedFontName.isEmpty())
    {
        juce::StringArray fontNames = juce::Font::findAllTypefaceNames();
        const char* japaneseFonts[] = {
            "Meiryo UI", "Meiryo", "Yu Gothic UI", "Yu Gothic",
            "MS UI Gothic", "MS Gothic", "MS PGothic", nullptr
        };

        for (int i = 0; japaneseFonts[i] != nullptr; ++i)
        {
            if (fontNames.contains(japaneseFonts[i]))
            {
                cachedFontName = japaneseFonts[i];
                break;
            }
        }

        if (cachedFontName.isEmpty())
            cachedFontName = juce::Font::getDefaultSansSerifFontName();
    }

    return juce::Font(cachedFontName, height, style);
}

//==============================================================================
PlaylistPanel::PlaylistListBox::PlaylistListBox(PlaylistPanel& o)
    : owner(o)
{
    setModel(this);
    setRowHeight(24);
    setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::ListBox::outlineColourId, juce::Colour(0xff3a3a3a));
}

int PlaylistPanel::PlaylistListBox::getNumRows()
{
    return (int)owner.playlistItems.size();
}

void PlaylistPanel::PlaylistListBox::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int)owner.playlistItems.size())
        return;

    const auto& item = owner.playlistItems[rowNumber];

    // Background
    if (rowNumber == owner.currentIndex)
    {
        g.setColour(juce::Colour(0xff4a4aff).withAlpha(0.3f));
        g.fillRect(0, 0, width, height);
    }
    else if (rowIsSelected)
    {
        g.setColour(juce::Colour(0xff4a4a4a));
        g.fillRect(0, 0, width, height);
    }

    // Played indicator
    if (item.played)
    {
        g.setColour(juce::Colours::grey);
        g.fillRect(0, 0, 3, height);
    }

    // Text - use Japanese font
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colours::lightgrey);
    g.setFont(getJapaneseFont(13.0f));

    juce::String displayText = item.displayName;
    if (item.duration > 0.0)
    {
        int minutes = (int)(item.duration / 60.0);
        int seconds = (int)item.duration % 60;
        displayText += juce::String::formatted(" [%d:%02d]", minutes, seconds);
    }

    g.drawText(displayText, 10, 0, width - 20, height, juce::Justification::centredLeft, true);

    // Playing indicator
    if (rowNumber == owner.currentIndex)
    {
        g.setColour(juce::Colour(0xff4aff4a));
        g.fillEllipse(width - 25.0f, (height - 10.0f) / 2.0f, 10.0f, 10.0f);
    }
}

void PlaylistPanel::PlaylistListBox::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    owner.selectAndPlay(row);
}

//==============================================================================
PlaylistPanel::PlaylistPanel()
    : playlistListBox(*this)
{
    // Title - use Japanese font
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Playlist", juce::dontSendNotification);
    titleLabel.setFont(getJapaneseFont(16.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType(juce::Justification::centred);

    // List box
    addAndMakeVisible(playlistListBox);

    // Buttons
    addAndMakeVisible(addButton);
    addButton.setButtonText("Add");
    addButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select audio files",
            juce::File(),
            "*.wav;*.mp3;*.aiff;*.flac"
        );

        auto chooserFlags = juce::FileBrowserComponent::openMode |
                           juce::FileBrowserComponent::canSelectFiles |
                           juce::FileBrowserComponent::canSelectMultipleItems;

        fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
        {
            auto files = fc.getResults();
            addFiles(files);
        });
    };

    addAndMakeVisible(removeButton);
    removeButton.setButtonText("Remove");
    removeButton.onClick = [this]() { removeSelected(); };

    addAndMakeVisible(clearButton);
    clearButton.setButtonText("Clear");
    clearButton.onClick = [this]() { clearPlaylist(); };

    addAndMakeVisible(moveUpButton);
    moveUpButton.setButtonText("Up");
    moveUpButton.onClick = [this]() { moveUp(); };

    addAndMakeVisible(moveDownButton);
    moveDownButton.setButtonText("Down");
    moveDownButton.onClick = [this]() { moveDown(); };

    addAndMakeVisible(autoAdvanceButton);
    autoAdvanceButton.setButtonText("Auto-advance");
    autoAdvanceButton.setToggleState(true, juce::dontSendNotification);
    autoAdvanceButton.onClick = [this]()
    {
        autoAdvance = autoAdvanceButton.getToggleState();
    };
}

PlaylistPanel::~PlaylistPanel()
{
}

//==============================================================================
void PlaylistPanel::addFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return;

    PlaylistItem item;
    item.file = file;
    item.displayName = file.getFileNameWithoutExtension();

    playlistItems.push_back(item);
    updatePlaylist();
}

void PlaylistPanel::addFiles(const juce::Array<juce::File>& files)
{
    for (const auto& file : files)
    {
        if (file.existsAsFile())
        {
            PlaylistItem item;
            item.file = file;
            item.displayName = file.getFileNameWithoutExtension();
            playlistItems.push_back(item);
        }
    }

    updatePlaylist();
}

void PlaylistPanel::removeSelected()
{
    int selectedRow = playlistListBox.getSelectedRow();
    if (selectedRow >= 0 && selectedRow < (int)playlistItems.size())
    {
        playlistItems.erase(playlistItems.begin() + selectedRow);

        // Adjust current index
        if (currentIndex == selectedRow)
            currentIndex = -1;
        else if (currentIndex > selectedRow)
            currentIndex--;

        updatePlaylist();
    }
}

void PlaylistPanel::clearPlaylist()
{
    playlistItems.clear();
    currentIndex = -1;
    updatePlaylist();
}

void PlaylistPanel::moveUp()
{
    int selectedRow = playlistListBox.getSelectedRow();
    if (selectedRow > 0 && selectedRow < (int)playlistItems.size())
    {
        std::swap(playlistItems[selectedRow], playlistItems[selectedRow - 1]);

        // Adjust current index
        if (currentIndex == selectedRow)
            currentIndex--;
        else if (currentIndex == selectedRow - 1)
            currentIndex++;

        playlistListBox.selectRow(selectedRow - 1);
        updatePlaylist();
    }
}

void PlaylistPanel::moveDown()
{
    int selectedRow = playlistListBox.getSelectedRow();
    if (selectedRow >= 0 && selectedRow < (int)playlistItems.size() - 1)
    {
        std::swap(playlistItems[selectedRow], playlistItems[selectedRow + 1]);

        // Adjust current index
        if (currentIndex == selectedRow)
            currentIndex++;
        else if (currentIndex == selectedRow + 1)
            currentIndex--;

        playlistListBox.selectRow(selectedRow + 1);
        updatePlaylist();
    }
}

void PlaylistPanel::setCurrentIndex(int index)
{
    if (index >= -1 && index < (int)playlistItems.size())
    {
        // Mark previous item as played
        if (currentIndex >= 0 && currentIndex < (int)playlistItems.size())
        {
            playlistItems[currentIndex].played = true;
        }

        currentIndex = index;
        playlistListBox.updateContent();
        playlistListBox.repaint();
    }
}

juce::File PlaylistPanel::getCurrentFile() const
{
    if (currentIndex >= 0 && currentIndex < (int)playlistItems.size())
        return playlistItems[currentIndex].file;

    return juce::File();
}

juce::File PlaylistPanel::getNextFile() const
{
    if (hasNext())
        return playlistItems[currentIndex + 1].file;

    return juce::File();
}

bool PlaylistPanel::hasNext() const
{
    return currentIndex >= -1 && currentIndex < (int)playlistItems.size() - 1;
}

//==============================================================================
void PlaylistPanel::updatePlaylist()
{
    playlistListBox.updateContent();
    playlistListBox.repaint();

    if (playlistChangedCallback)
        playlistChangedCallback();
}

void PlaylistPanel::selectAndPlay(int index)
{
    if (index >= 0 && index < (int)playlistItems.size())
    {
        setCurrentIndex(index);

        if (fileSelectedCallback)
            fileSelectedCallback(playlistItems[index].file);
    }
}

//==============================================================================
void PlaylistPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(getLocalBounds(), 1);
}

void PlaylistPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(5);

    // Buttons at bottom
    auto buttonRow1 = bounds.removeFromBottom(30);
    bounds.removeFromBottom(5);
    auto buttonRow2 = bounds.removeFromBottom(30);
    bounds.removeFromBottom(10);

    // Row 1: Add, Remove, Clear
    int buttonWidth = (buttonRow1.getWidth() - 20) / 3;
    addButton.setBounds(buttonRow1.removeFromLeft(buttonWidth));
    buttonRow1.removeFromLeft(10);
    removeButton.setBounds(buttonRow1.removeFromLeft(buttonWidth));
    buttonRow1.removeFromLeft(10);
    clearButton.setBounds(buttonRow1);

    // Row 2: Move buttons and auto-advance
    int moveButtonWidth = 40;
    moveUpButton.setBounds(buttonRow2.removeFromLeft(moveButtonWidth));
    buttonRow2.removeFromLeft(5);
    moveDownButton.setBounds(buttonRow2.removeFromLeft(moveButtonWidth));
    buttonRow2.removeFromLeft(10);
    autoAdvanceButton.setBounds(buttonRow2);

    // List box
    playlistListBox.setBounds(bounds);
}

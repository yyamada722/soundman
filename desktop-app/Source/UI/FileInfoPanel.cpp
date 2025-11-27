/*
  ==============================================================================

    FileInfoPanel.cpp

    Panel displaying file metadata and information

  ==============================================================================
*/

#include "FileInfoPanel.h"

//==============================================================================
FileInfoPanel::FileInfoPanel()
{
    // Set up title
    addAndMakeVisible(titleLabel);
    titleLabel.setText("File Information", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType(juce::Justification::centred);

    // Add info rows
    addInfoRow("File Name");
    addInfoRow("Format");
    addInfoRow("File Size");
    addInfoRow("Duration");
    addInfoRow("Sample Rate");
    addInfoRow("Channels");
    addInfoRow("Bit Depth");
    addInfoRow("Samples");
    addInfoRow("Bitrate");

    clearFileInfo();
}

FileInfoPanel::~FileInfoPanel()
{
}

//==============================================================================
void FileInfoPanel::addInfoRow(const juce::String& name)
{
    auto* row = new InfoRow();

    row->labelName.setText(name + ":", juce::dontSendNotification);
    row->labelName.setFont(juce::Font(13.0f, juce::Font::bold));
    row->labelName.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    row->labelName.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(row->labelName);

    row->labelValue.setText("-", juce::dontSendNotification);
    row->labelValue.setFont(juce::Font(13.0f));
    row->labelValue.setColour(juce::Label::textColourId, juce::Colours::white);
    row->labelValue.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(row->labelValue);

    infoRows.add(row);
}

void FileInfoPanel::setInfoValue(int index, const juce::String& value)
{
    if (index >= 0 && index < infoRows.size())
    {
        infoRows[index]->labelValue.setText(value, juce::dontSendNotification);
    }
}

//==============================================================================
void FileInfoPanel::setFileInfo(const juce::File& file, juce::AudioFormatReader* reader)
{
    fileName = file.getFileName();
    fileSize = file.getSize();

    if (reader != nullptr)
    {
        fileFormat = reader->getFormatName();
        sampleRate = reader->sampleRate;
        numChannels = static_cast<int>(reader->numChannels);
        bitsPerSample = static_cast<int>(reader->bitsPerSample);
        numSamples = reader->lengthInSamples;
        duration = numSamples / sampleRate;
    }

    // Update display
    setInfoValue(0, fileName);
    setInfoValue(1, fileFormat);

    // Format file size
    juce::String sizeStr;
    if (fileSize < 1024)
        sizeStr = juce::String(fileSize) + " bytes";
    else if (fileSize < 1024 * 1024)
        sizeStr = juce::String(fileSize / 1024.0, 1) + " KB";
    else
        sizeStr = juce::String(fileSize / (1024.0 * 1024.0), 2) + " MB";
    setInfoValue(2, sizeStr);

    // Format duration
    int totalSeconds = static_cast<int>(duration);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    int milliseconds = static_cast<int>((duration - totalSeconds) * 1000);

    juce::String durationStr;
    if (hours > 0)
        durationStr = juce::String::formatted("%d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
    else
        durationStr = juce::String::formatted("%d:%02d.%03d", minutes, seconds, milliseconds);
    setInfoValue(3, durationStr);

    // Sample rate
    setInfoValue(4, juce::String(sampleRate / 1000.0, 1) + " kHz");

    // Channels
    juce::String channelStr;
    if (numChannels == 1)
        channelStr = "Mono";
    else if (numChannels == 2)
        channelStr = "Stereo";
    else
        channelStr = juce::String(numChannels);
    setInfoValue(5, channelStr);

    // Bit depth
    setInfoValue(6, juce::String(bitsPerSample) + " bit");

    // Samples
    setInfoValue(7, juce::String(numSamples));

    // Bitrate
    if (duration > 0)
    {
        double bitrate = (fileSize * 8.0) / duration;
        juce::String bitrateStr;
        if (bitrate < 1000)
            bitrateStr = juce::String(bitrate, 0) + " bps";
        else
            bitrateStr = juce::String(bitrate / 1000.0, 0) + " kbps";
        setInfoValue(8, bitrateStr);
    }
    else
    {
        setInfoValue(8, "-");
    }

    repaint();
}

void FileInfoPanel::clearFileInfo()
{
    fileName = "";
    fileFormat = "";
    fileSize = 0;
    sampleRate = 0.0;
    numChannels = 0;
    bitsPerSample = 0;
    duration = 0.0;
    numSamples = 0;

    for (size_t i = 0; i < infoRows.size(); ++i)
    {
        setInfoValue(static_cast<int>(i), "-");
    }

    repaint();
}

//==============================================================================
void FileInfoPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Draw border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(getLocalBounds(), 1);
}

void FileInfoPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(10);

    // Info rows
    int rowHeight = 22;
    for (auto* row : infoRows)
    {
        auto rowBounds = bounds.removeFromTop(rowHeight);

        // Split into label and value (40/60 split)
        auto labelBounds = rowBounds.removeFromLeft((int)(getWidth() * 0.4f));
        row->labelName.setBounds(labelBounds);
        row->labelValue.setBounds(rowBounds);

        bounds.removeFromTop(2);
    }
}

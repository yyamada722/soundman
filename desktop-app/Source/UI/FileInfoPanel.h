/*
  ==============================================================================

    FileInfoPanel.h

    Panel displaying file metadata and information

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

class FileInfoPanel : public juce::Component
{
public:
    //==========================================================================
    FileInfoPanel();
    ~FileInfoPanel() override;

    //==========================================================================
    // Update file information
    void setFileInfo(const juce::File& file, juce::AudioFormatReader* reader);
    void clearFileInfo();

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    struct InfoRow
    {
        juce::Label labelName;
        juce::Label labelValue;
    };

    void addInfoRow(const juce::String& name);
    void setInfoValue(int index, const juce::String& value);

    //==========================================================================
    juce::Label titleLabel;
    juce::OwnedArray<InfoRow> infoRows;

    // File information
    juce::String fileName;
    juce::String fileFormat;
    int64_t fileSize { 0 };
    double sampleRate { 0.0 };
    int numChannels { 0 };
    int bitsPerSample { 0 };
    double duration { 0.0 };
    int64_t numSamples { 0 };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileInfoPanel)
};

/*
  ==============================================================================

    DeviceControlPanel.cpp

    Device control panel implementation

  ==============================================================================
*/

#include "DeviceControlPanel.h"

//==============================================================================
DeviceControlPanel::DeviceControlPanel()
{
    // Setup transport buttons
    addAndMakeVisible(loadButton);
    loadButton.setButtonText("Load Audio File");
    loadButton.onClick = [this]() { if (loadCallback) loadCallback(); };

    addAndMakeVisible(playButton);
    playButton.setButtonText("Play");
    playButton.onClick = [this]() { if (playCallback) playCallback(); };
    playButton.setEnabled(false);

    addAndMakeVisible(pauseButton);
    pauseButton.setButtonText("Pause");
    pauseButton.onClick = [this]() { if (pauseCallback) pauseCallback(); };
    pauseButton.setEnabled(false);

    addAndMakeVisible(stopButton);
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this]() { if (stopCallback) stopCallback(); };
    stopButton.setEnabled(false);

    // Setup info labels
    addAndMakeVisible(deviceLabel);
    deviceLabel.setJustificationType(juce::Justification::centredLeft);
    deviceLabel.setFont(juce::Font(12.0f));

    addAndMakeVisible(sampleRateLabel);
    sampleRateLabel.setJustificationType(juce::Justification::centredLeft);
    sampleRateLabel.setFont(juce::Font(12.0f));

    addAndMakeVisible(bufferSizeLabel);
    bufferSizeLabel.setJustificationType(juce::Justification::centredLeft);
    bufferSizeLabel.setFont(juce::Font(12.0f));

    addAndMakeVisible(fileNameLabel);
    fileNameLabel.setJustificationType(juce::Justification::centredLeft);
    fileNameLabel.setFont(juce::Font(12.0f, juce::Font::bold));

    // Initial values
    setDeviceName("No device");
    setSampleRate(0.0);
    setBufferSize(0);
    setLoadedFileName("No file loaded");
}

DeviceControlPanel::~DeviceControlPanel()
{
}

//==============================================================================
void DeviceControlPanel::setDeviceName(const juce::String& name)
{
    deviceName = name;
    deviceLabel.setText("Device: " + name, juce::dontSendNotification);
}

void DeviceControlPanel::setSampleRate(double sampleRate)
{
    currentSampleRate = sampleRate;
    sampleRateLabel.setText(
        juce::String::formatted("Sample Rate: %.1f kHz", sampleRate / 1000.0),
        juce::dontSendNotification
    );
}

void DeviceControlPanel::setBufferSize(int bufferSize)
{
    currentBufferSize = bufferSize;
    bufferSizeLabel.setText(
        "Buffer: " + juce::String(bufferSize) + " samples",
        juce::dontSendNotification
    );
}

void DeviceControlPanel::setLoadedFileName(const juce::String& name)
{
    loadedFileName = name;
    fileNameLabel.setText(name.isEmpty() ? "No file loaded" : name,
                         juce::dontSendNotification);
}

//==============================================================================
void DeviceControlPanel::setPlayButtonEnabled(bool enabled)
{
    playButton.setEnabled(enabled);
}

void DeviceControlPanel::setPauseButtonEnabled(bool enabled)
{
    pauseButton.setEnabled(enabled);
}

void DeviceControlPanel::setStopButtonEnabled(bool enabled)
{
    stopButton.setEnabled(enabled);
}

//==============================================================================
void DeviceControlPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Draw section headers
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));

    g.drawText("Transport", 10, 10, getWidth() - 20, 20,
              juce::Justification::centredLeft);

    g.drawText("File Info", 10, 190, getWidth() - 20, 20,
              juce::Justification::centredLeft);

    g.drawText("Device Info", 10, 260, getWidth() - 20, 20,
              juce::Justification::centredLeft);

    // Draw separator lines
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawHorizontalLine(30, 10.0f, (float)getWidth() - 10);
    g.drawHorizontalLine(210, 10.0f, (float)getWidth() - 10);
    g.drawHorizontalLine(280, 10.0f, (float)getWidth() - 10);
}

void DeviceControlPanel::resized()
{
    auto bounds = getLocalBounds();

    // Transport section
    bounds.removeFromTop(40);  // Skip header
    auto transportBounds = bounds.removeFromTop(140).reduced(10);

    loadButton.setBounds(transportBounds.removeFromTop(30));
    transportBounds.removeFromTop(10);
    playButton.setBounds(transportBounds.removeFromTop(30));
    transportBounds.removeFromTop(10);
    pauseButton.setBounds(transportBounds.removeFromTop(30));
    transportBounds.removeFromTop(10);
    stopButton.setBounds(transportBounds.removeFromTop(30));

    // File info section
    bounds.removeFromTop(40);  // Skip header
    auto fileInfoBounds = bounds.removeFromTop(40).reduced(10);
    fileNameLabel.setBounds(fileInfoBounds);

    // Device info section
    bounds.removeFromTop(40);  // Skip header
    auto deviceInfoBounds = bounds.reduced(10);

    deviceLabel.setBounds(deviceInfoBounds.removeFromTop(25));
    sampleRateLabel.setBounds(deviceInfoBounds.removeFromTop(25));
    bufferSizeLabel.setBounds(deviceInfoBounds.removeFromTop(25));
}

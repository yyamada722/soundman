/*
  ==============================================================================

    SettingsDialog.cpp

    Settings dialog implementation

  ==============================================================================
*/

#include "SettingsDialog.h"

//==============================================================================
SettingsDialog::SettingsDialog(juce::AudioDeviceManager& deviceManager)
    : audioDeviceManager(deviceManager)
{
    // Set up tabbed component
    addAndMakeVisible(tabbedComponent);
    tabbedComponent.setTabBarDepth(30);

    // Set up audio tab
    setupAudioTab();
    tabbedComponent.addTab("Audio", juce::Colour(0xff2a2a2a), &audioTab, false);

    // Set window size
    setSize(600, 500);
}

SettingsDialog::~SettingsDialog()
{
}

//==============================================================================
void SettingsDialog::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void SettingsDialog::resized()
{
    tabbedComponent.setBounds(getLocalBounds());

    // Layout audio tab components
    auto bounds = audioTab.getLocalBounds().reduced(20);

    // Output device
    outputDeviceLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(5);
    outputDeviceCombo.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(15);

    // Sample rate
    sampleRateLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(5);
    sampleRateCombo.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(15);

    // Buffer size
    bufferSizeLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(5);
    bufferSizeCombo.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(20);

    // Current settings display
    currentSettingsLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(5);
    currentSettingsDisplay.setBounds(bounds.removeFromTop(100));
    bounds.removeFromTop(20);

    // Buttons
    auto buttonBounds = bounds.removeFromTop(30);
    int buttonWidth = 100;
    closeButton.setBounds(buttonBounds.removeFromRight(buttonWidth));
    buttonBounds.removeFromRight(10);
    applyButton.setBounds(buttonBounds.removeFromRight(buttonWidth));
}

//==============================================================================
void SettingsDialog::setupAudioTab()
{
    audioTab.addAndMakeVisible(outputDeviceLabel);
    outputDeviceLabel.setText("Output Device:", juce::dontSendNotification);
    outputDeviceLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    audioTab.addAndMakeVisible(outputDeviceCombo);
    outputDeviceCombo.onChange = [this]() { updateSampleRateList(); updateBufferSizeList(); };

    audioTab.addAndMakeVisible(sampleRateLabel);
    sampleRateLabel.setText("Sample Rate:", juce::dontSendNotification);
    sampleRateLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    audioTab.addAndMakeVisible(sampleRateCombo);

    audioTab.addAndMakeVisible(bufferSizeLabel);
    bufferSizeLabel.setText("Buffer Size:", juce::dontSendNotification);
    bufferSizeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    audioTab.addAndMakeVisible(bufferSizeCombo);

    audioTab.addAndMakeVisible(currentSettingsLabel);
    currentSettingsLabel.setText("Current Settings:", juce::dontSendNotification);
    currentSettingsLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    audioTab.addAndMakeVisible(currentSettingsDisplay);
    currentSettingsDisplay.setMultiLine(true);
    currentSettingsDisplay.setReadOnly(true);
    currentSettingsDisplay.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a2a));
    currentSettingsDisplay.setColour(juce::TextEditor::textColourId, juce::Colours::lightgrey);

    audioTab.addAndMakeVisible(applyButton);
    applyButton.setButtonText("Apply");
    applyButton.onClick = [this]() { applyAudioSettings(); };

    audioTab.addAndMakeVisible(closeButton);
    closeButton.setButtonText("Close");
    closeButton.onClick = [this]() {
        if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
            window->exitModalState(0);
    };

    // Initialize lists
    updateDeviceList();
    updateSampleRateList();
    updateBufferSizeList();
    refreshCurrentSettings();
}

void SettingsDialog::updateDeviceList()
{
    outputDeviceCombo.clear();

    auto& deviceTypes = audioDeviceManager.getAvailableDeviceTypes();

    int itemId = 1;
    for (auto* deviceType : deviceTypes)
    {
        auto deviceNames = deviceType->getDeviceNames(false);  // false = output devices

        for (auto& deviceName : deviceNames)
        {
            outputDeviceCombo.addItem(deviceName, itemId++);
        }
    }

    // Select current device
    auto* currentDevice = audioDeviceManager.getCurrentAudioDevice();
    if (currentDevice != nullptr)
    {
        juce::String currentName = currentDevice->getName();
        for (int i = 0; i < outputDeviceCombo.getNumItems(); ++i)
        {
            if (outputDeviceCombo.getItemText(i) == currentName)
            {
                outputDeviceCombo.setSelectedItemIndex(i, juce::dontSendNotification);
                break;
            }
        }
    }
}

void SettingsDialog::updateSampleRateList()
{
    sampleRateCombo.clear();

    auto* currentDevice = audioDeviceManager.getCurrentAudioDevice();
    if (currentDevice != nullptr)
    {
        auto sampleRates = currentDevice->getAvailableSampleRates();

        int itemId = 1;
        for (auto rate : sampleRates)
        {
            juce::String rateText = juce::String(rate / 1000.0, 1) + " kHz";
            sampleRateCombo.addItem(rateText, itemId++);

            // Select current sample rate
            if (std::abs(rate - currentDevice->getCurrentSampleRate()) < 0.1)
            {
                sampleRateCombo.setSelectedItemIndex(itemId - 2, juce::dontSendNotification);
            }
        }
    }
    else
    {
        // Default sample rates if no device
        sampleRateCombo.addItem("44.1 kHz", 1);
        sampleRateCombo.addItem("48.0 kHz", 2);
        sampleRateCombo.addItem("96.0 kHz", 3);
        sampleRateCombo.addItem("192.0 kHz", 4);
    }
}

void SettingsDialog::updateBufferSizeList()
{
    bufferSizeCombo.clear();

    auto* currentDevice = audioDeviceManager.getCurrentAudioDevice();
    if (currentDevice != nullptr)
    {
        auto bufferSizes = currentDevice->getAvailableBufferSizes();

        int itemId = 1;
        for (auto size : bufferSizes)
        {
            juce::String sizeText = juce::String(size) + " samples";
            bufferSizeCombo.addItem(sizeText, itemId++);

            // Select current buffer size
            if (size == currentDevice->getCurrentBufferSizeSamples())
            {
                bufferSizeCombo.setSelectedItemIndex(itemId - 2, juce::dontSendNotification);
            }
        }
    }
    else
    {
        // Default buffer sizes if no device
        bufferSizeCombo.addItem("64 samples", 1);
        bufferSizeCombo.addItem("128 samples", 2);
        bufferSizeCombo.addItem("256 samples", 3);
        bufferSizeCombo.addItem("512 samples", 4);
        bufferSizeCombo.addItem("1024 samples", 5);
        bufferSizeCombo.addItem("2048 samples", 6);
    }
}

void SettingsDialog::applyAudioSettings()
{
    // Get selected device name
    juce::String selectedDevice = outputDeviceCombo.getText();

    if (selectedDevice.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Error",
            "Please select an output device",
            "OK"
        );
        return;
    }

    // Get selected sample rate
    double selectedSampleRate = 44100.0;
    if (sampleRateCombo.getSelectedId() > 0)
    {
        juce::String rateText = sampleRateCombo.getText();
        selectedSampleRate = rateText.upToFirstOccurrenceOf(" ", false, false).getDoubleValue() * 1000.0;
    }

    // Get selected buffer size
    int selectedBufferSize = 512;
    if (bufferSizeCombo.getSelectedId() > 0)
    {
        juce::String sizeText = bufferSizeCombo.getText();
        selectedBufferSize = sizeText.upToFirstOccurrenceOf(" ", false, false).getIntValue();
    }

    // Create audio setup
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    audioDeviceManager.getAudioDeviceSetup(setup);

    setup.outputDeviceName = selectedDevice;
    setup.sampleRate = selectedSampleRate;
    setup.bufferSize = selectedBufferSize;

    // Apply settings
    juce::String error = audioDeviceManager.setAudioDeviceSetup(setup, true);

    if (error.isNotEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Error",
            "Failed to apply audio settings:\n" + error,
            "OK"
        );
    }
    else
    {
        // Refresh current settings display
        refreshCurrentSettings();

        // Notify callback
        if (settingsChangedCallback)
            settingsChangedCallback();

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Success",
            "Audio settings applied successfully",
            "OK"
        );
    }
}

void SettingsDialog::refreshCurrentSettings()
{
    auto* device = audioDeviceManager.getCurrentAudioDevice();

    juce::String settingsText;

    if (device != nullptr)
    {
        settingsText += "Device: " + device->getName() + "\n";
        settingsText += "Sample Rate: " + juce::String(device->getCurrentSampleRate() / 1000.0, 1) + " kHz\n";
        settingsText += "Buffer Size: " + juce::String(device->getCurrentBufferSizeSamples()) + " samples\n";

        double latency = device->getCurrentBufferSizeSamples() / device->getCurrentSampleRate() * 1000.0;
        settingsText += "Latency: " + juce::String(latency, 1) + " ms\n";

        auto inputChannels = device->getActiveInputChannels();
        auto outputChannels = device->getActiveOutputChannels();
        settingsText += "Input Channels: " + juce::String(inputChannels.countNumberOfSetBits()) + "\n";
        settingsText += "Output Channels: " + juce::String(outputChannels.countNumberOfSetBits());
    }
    else
    {
        settingsText = "No audio device active";
    }

    currentSettingsDisplay.setText(settingsText);
}

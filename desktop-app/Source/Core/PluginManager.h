/*
  ==============================================================================

    PluginManager.h

    VST3 Plugin hosting and management

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <vector>
#include <memory>

class PluginManager
{
public:
    //==========================================================================
    PluginManager();
    ~PluginManager();

    //==========================================================================
    // Plugin scanning
    void scanForPlugins();
    void scanDirectory(const juce::File& directory);
    bool isScanning() const { return scanning; }
    float getScanProgress() const { return scanProgress; }

    // Get plugin list
    juce::KnownPluginList& getKnownPluginList() { return knownPluginList; }
    const juce::KnownPluginList& getKnownPluginList() const { return knownPluginList; }

    // Get available plugins
    juce::Array<juce::PluginDescription> getAvailablePlugins() const;
    int getNumPlugins() const { return knownPluginList.getNumTypes(); }

    //==========================================================================
    // Plugin loading
    std::unique_ptr<juce::AudioPluginInstance> loadPlugin(
        const juce::PluginDescription& description,
        double sampleRate,
        int blockSize,
        juce::String& errorMessage);

    //==========================================================================
    // Plugin paths
    void addPluginPath(const juce::File& path);
    void removePluginPath(const juce::File& path);
    juce::StringArray getPluginPaths() const;
    void setDefaultPluginPaths();

    //==========================================================================
    // Save/Load plugin list
    void savePluginList(const juce::File& file);
    void loadPluginList(const juce::File& file);

    //==========================================================================
    // Callbacks
    std::function<void()> onScanStarted;
    std::function<void()> onScanFinished;
    std::function<void(const juce::String&)> onPluginFound;
    std::function<void(float)> onScanProgress;

private:
    //==========================================================================
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;
    juce::FileSearchPath pluginSearchPaths;

    bool scanning { false };
    float scanProgress { 0.0f };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManager)
};

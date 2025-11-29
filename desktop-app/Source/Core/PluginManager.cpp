/*
  ==============================================================================

    PluginManager.cpp

    VST3 Plugin hosting and management implementation

  ==============================================================================
*/

#include "PluginManager.h"

//==============================================================================
PluginManager::PluginManager()
{
    // Register plugin formats
    formatManager.addDefaultFormats();

    // Set default search paths
    setDefaultPluginPaths();
}

PluginManager::~PluginManager()
{
}

//==============================================================================
void PluginManager::setDefaultPluginPaths()
{
    // Reset search paths
    pluginSearchPaths = juce::FileSearchPath();

#if JUCE_WINDOWS
    // Common VST3 paths on Windows
    pluginSearchPaths.add(juce::File("C:\\Program Files\\Common Files\\VST3"));
    pluginSearchPaths.add(juce::File("C:\\Program Files (x86)\\Common Files\\VST3"));

    // User-specific path
    auto userVst3 = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("VST3");
    if (userVst3.exists())
        pluginSearchPaths.add(userVst3);

#elif JUCE_MAC
    // Common VST3 paths on macOS
    pluginSearchPaths.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
    pluginSearchPaths.add(juce::File("~/Library/Audio/Plug-Ins/VST3"));

#elif JUCE_LINUX
    // Common VST3 paths on Linux
    pluginSearchPaths.add(juce::File("/usr/lib/vst3"));
    pluginSearchPaths.add(juce::File("/usr/local/lib/vst3"));
    pluginSearchPaths.add(juce::File("~/.vst3"));
#endif
}

//==============================================================================
void PluginManager::scanForPlugins()
{
    if (scanning)
        return;

    scanning = true;
    scanProgress = 0.0f;

    if (onScanStarted)
        onScanStarted();

    // Clear existing list
    knownPluginList.clear();

    // Get all paths to scan
    juce::StringArray paths;
    for (int i = 0; i < pluginSearchPaths.getNumPaths(); ++i)
    {
        auto path = pluginSearchPaths[i];
        if (path.exists())
            paths.add(path.getFullPathName());
    }

    int totalPaths = paths.size();
    int currentPath = 0;

    for (const auto& pathStr : paths)
    {
        juce::File path(pathStr);
        scanDirectory(path);

        currentPath++;
        scanProgress = (float)currentPath / (float)totalPaths;

        if (onScanProgress)
            onScanProgress(scanProgress);
    }

    scanning = false;
    scanProgress = 1.0f;

    if (onScanFinished)
        onScanFinished();
}

void PluginManager::scanDirectory(const juce::File& directory)
{
    if (!directory.exists() || !directory.isDirectory())
        return;

    // Find VST3 files
    for (const auto& format : formatManager.getFormats())
    {
        juce::PluginDirectoryScanner scanner(
            knownPluginList,
            *format,
            juce::FileSearchPath(directory.getFullPathName()),
            true,  // recursive
            juce::File(),  // dead mans pedal file
            false  // allow plugins requiring async instantiation
        );

        juce::String pluginName;
        while (scanner.scanNextFile(true, pluginName))
        {
            if (pluginName.isNotEmpty() && onPluginFound)
                onPluginFound(pluginName);
        }
    }
}

//==============================================================================
juce::Array<juce::PluginDescription> PluginManager::getAvailablePlugins() const
{
    juce::Array<juce::PluginDescription> plugins;

    const auto& types = knownPluginList.getTypes();
    for (const auto& type : types)
    {
        plugins.add(type);
    }

    return plugins;
}

//==============================================================================
std::unique_ptr<juce::AudioPluginInstance> PluginManager::loadPlugin(
    const juce::PluginDescription& description,
    double sampleRate,
    int blockSize,
    juce::String& errorMessage)
{
    return formatManager.createPluginInstance(
        description,
        sampleRate,
        blockSize,
        errorMessage
    );
}

//==============================================================================
void PluginManager::addPluginPath(const juce::File& path)
{
    if (path.exists() && path.isDirectory())
    {
        pluginSearchPaths.add(path);
    }
}

void PluginManager::removePluginPath(const juce::File& path)
{
    // Find and remove the path by index
    for (int i = 0; i < pluginSearchPaths.getNumPaths(); ++i)
    {
        if (pluginSearchPaths[i] == path)
        {
            pluginSearchPaths.remove(i);
            break;
        }
    }
}

juce::StringArray PluginManager::getPluginPaths() const
{
    juce::StringArray paths;
    for (int i = 0; i < pluginSearchPaths.getNumPaths(); ++i)
    {
        paths.add(pluginSearchPaths[i].getFullPathName());
    }
    return paths;
}

//==============================================================================
void PluginManager::savePluginList(const juce::File& file)
{
    auto xml = knownPluginList.createXml();
    if (xml != nullptr)
    {
        xml->writeTo(file);
    }
}

void PluginManager::loadPluginList(const juce::File& file)
{
    if (file.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(file);
        if (xml != nullptr)
        {
            knownPluginList.recreateFromXml(*xml);
        }
    }
}

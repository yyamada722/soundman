/*
  ==============================================================================

    EffectChain.cpp

    Audio effect chain implementation

  ==============================================================================
*/

#include "EffectChain.h"

//==============================================================================
EffectChain::EffectChain()
{
    processorGraph = std::make_unique<juce::AudioProcessorGraph>();

    // Create input/output nodes
    audioInputNode = processorGraph->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode))->nodeID;

    audioOutputNode = processorGraph->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))->nodeID;

    // Initial connection: input -> output
    connectNodes();
}

EffectChain::~EffectChain()
{
    clearAllPlugins();
}

//==============================================================================
int EffectChain::addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin, const juce::String& name)
{
    if (plugin == nullptr)
        return -1;

    juce::ScopedLock lock(processLock);

    // Prepare the plugin with current settings
    plugin->setPlayConfigDetails(2, 2, currentSampleRate, currentBlockSize);
    plugin->prepareToPlay(currentSampleRate, currentBlockSize);

    // Add to graph
    auto node = processorGraph->addNode(std::move(plugin));
    if (node == nullptr)
        return -1;

    PluginSlot slot;
    slot.plugin = nullptr; // Plugin is now owned by the graph
    slot.nodeId = node->nodeID;
    slot.name = name;
    slot.bypassed = false;

    pluginSlots.push_back(std::move(slot));

    // Rebuild connections
    connectNodes();

    // Re-prepare graph after connection changes
    processorGraph->prepareToPlay(currentSampleRate, currentBlockSize);

    if (onChainChanged)
        onChainChanged();

    return (int)pluginSlots.size() - 1;
}

void EffectChain::removePlugin(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= (int)pluginSlots.size())
        return;

    juce::ScopedLock lock(processLock);

    // Remove from graph
    processorGraph->removeNode(pluginSlots[slotIndex].nodeId);

    // Remove from our list
    pluginSlots.erase(pluginSlots.begin() + slotIndex);

    // Rebuild connections
    connectNodes();

    if (onChainChanged)
        onChainChanged();
}

void EffectChain::movePlugin(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= (int)pluginSlots.size())
        return;
    if (toIndex < 0 || toIndex >= (int)pluginSlots.size())
        return;
    if (fromIndex == toIndex)
        return;

    juce::ScopedLock lock(processLock);

    // Move the slot
    auto slot = std::move(pluginSlots[fromIndex]);
    pluginSlots.erase(pluginSlots.begin() + fromIndex);

    if (toIndex > fromIndex)
        toIndex--;

    pluginSlots.insert(pluginSlots.begin() + toIndex, std::move(slot));

    // Rebuild connections
    connectNodes();

    if (onChainChanged)
        onChainChanged();
}

void EffectChain::clearAllPlugins()
{
    juce::ScopedLock lock(processLock);

    // Remove all plugin nodes
    for (auto& slot : pluginSlots)
    {
        processorGraph->removeNode(slot.nodeId);
    }

    pluginSlots.clear();
    connectNodes();

    if (onChainChanged)
        onChainChanged();
}

EffectChain::PluginSlot* EffectChain::getPluginSlot(int index)
{
    if (index < 0 || index >= (int)pluginSlots.size())
        return nullptr;
    return &pluginSlots[index];
}

const EffectChain::PluginSlot* EffectChain::getPluginSlot(int index) const
{
    if (index < 0 || index >= (int)pluginSlots.size())
        return nullptr;
    return &pluginSlots[index];
}

//==============================================================================
void EffectChain::setPluginBypassed(int slotIndex, bool bypassed)
{
    if (slotIndex < 0 || slotIndex >= (int)pluginSlots.size())
        return;

    pluginSlots[slotIndex].bypassed = bypassed;

    auto node = processorGraph->getNodeForId(pluginSlots[slotIndex].nodeId);
    if (node != nullptr)
    {
        node->setBypassed(bypassed);
    }
}

bool EffectChain::isPluginBypassed(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= (int)pluginSlots.size())
        return false;

    return pluginSlots[slotIndex].bypassed;
}

//==============================================================================
juce::AudioProcessorEditor* EffectChain::createEditorForPlugin(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= (int)pluginSlots.size())
        return nullptr;

    auto node = processorGraph->getNodeForId(pluginSlots[slotIndex].nodeId);
    if (node == nullptr || node->getProcessor() == nullptr)
        return nullptr;

    if (node->getProcessor()->hasEditor())
        return node->getProcessor()->createEditor();

    return nullptr;
}

//==============================================================================
void EffectChain::rebuildGraph()
{
    // Remove all connections
    processorGraph->clear();

    // Re-add input/output nodes
    audioInputNode = processorGraph->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode))->nodeID;

    audioOutputNode = processorGraph->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))->nodeID;

    // Re-add all plugins
    // Note: Plugin instances are owned by the graph, so full rebuild would require save/restore state
    (void)pluginSlots; // Currently not reimplementing plugins after clear

    connectNodes();
}

void EffectChain::connectNodes()
{
    // Remove all existing connections
    for (auto connection : processorGraph->getConnections())
    {
        processorGraph->removeConnection(connection);
    }

    if (pluginSlots.empty())
    {
        // Direct connection: input -> output
        for (int ch = 0; ch < 2; ++ch)
        {
            processorGraph->addConnection({
                { audioInputNode, ch },
                { audioOutputNode, ch }
            });
        }
        return;
    }

    // Build list of effect plugins only (plugins with audio inputs)
    std::vector<size_t> effectIndices;
    for (size_t i = 0; i < pluginSlots.size(); ++i)
    {
        auto node = processorGraph->getNodeForId(pluginSlots[i].nodeId);
        if (node != nullptr && node->getProcessor() != nullptr)
        {
            int numInputs = node->getProcessor()->getTotalNumInputChannels();
            if (numInputs > 0)
            {
                // This is an effect plugin (has audio inputs)
                effectIndices.push_back(i);
            }
            // Instruments (synths) with 0 inputs are skipped from the chain
            // They would need MIDI input to produce sound
        }
    }

    if (effectIndices.empty())
    {
        // No effect plugins, just pass through
        for (int ch = 0; ch < 2; ++ch)
        {
            processorGraph->addConnection({
                { audioInputNode, ch },
                { audioOutputNode, ch }
            });
        }
        return;
    }

    // Connect input to first effect plugin
    auto firstNode = processorGraph->getNodeForId(pluginSlots[effectIndices[0]].nodeId);
    if (firstNode != nullptr && firstNode->getProcessor() != nullptr)
    {
        int numInputs = firstNode->getProcessor()->getTotalNumInputChannels();
        for (int ch = 0; ch < juce::jmin(2, numInputs); ++ch)
        {
            processorGraph->addConnection({
                { audioInputNode, ch },
                { pluginSlots[effectIndices[0]].nodeId, ch }
            });
        }
    }

    // Connect effect plugins in series
    for (size_t i = 0; i < effectIndices.size() - 1; ++i)
    {
        auto currentNode = processorGraph->getNodeForId(pluginSlots[effectIndices[i]].nodeId);
        auto nextNode = processorGraph->getNodeForId(pluginSlots[effectIndices[i + 1]].nodeId);

        if (currentNode != nullptr && nextNode != nullptr &&
            currentNode->getProcessor() != nullptr && nextNode->getProcessor() != nullptr)
        {
            int numOutputs = currentNode->getProcessor()->getTotalNumOutputChannels();
            int numInputs = nextNode->getProcessor()->getTotalNumInputChannels();
            int channels = juce::jmin(numOutputs, numInputs, 2);

            for (int ch = 0; ch < channels; ++ch)
            {
                processorGraph->addConnection({
                    { pluginSlots[effectIndices[i]].nodeId, ch },
                    { pluginSlots[effectIndices[i + 1]].nodeId, ch }
                });
            }
        }
    }

    // Connect last effect plugin to output
    auto lastNode = processorGraph->getNodeForId(pluginSlots[effectIndices.back()].nodeId);
    if (lastNode != nullptr && lastNode->getProcessor() != nullptr)
    {
        int numOutputs = lastNode->getProcessor()->getTotalNumOutputChannels();
        for (int ch = 0; ch < juce::jmin(2, numOutputs); ++ch)
        {
            processorGraph->addConnection({
                { pluginSlots[effectIndices.back()].nodeId, ch },
                { audioOutputNode, ch }
            });
        }
    }
}

//==============================================================================
void EffectChain::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ScopedLock lock(processLock);

    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    processorGraph->setPlayConfigDetails(2, 2, sampleRate, samplesPerBlock);
    processorGraph->prepareToPlay(sampleRate, samplesPerBlock);
}

void EffectChain::releaseResources()
{
    processorGraph->releaseResources();
}

void EffectChain::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedLock lock(processLock);

    if (chainBypassed)
        return;

    processorGraph->processBlock(buffer, midiMessages);
}

double EffectChain::getTailLengthSeconds() const
{
    double maxTail = 0.0;

    for (const auto& slot : pluginSlots)
    {
        auto node = processorGraph->getNodeForId(slot.nodeId);
        if (node != nullptr && node->getProcessor() != nullptr)
        {
            maxTail = std::max(maxTail, node->getProcessor()->getTailLengthSeconds());
        }
    }

    return maxTail;
}

//==============================================================================
void EffectChain::saveChainState(juce::MemoryBlock& destData)
{
    getStateInformation(destData);
}

void EffectChain::loadChainState(const void* data, int sizeInBytes)
{
    setStateInformation(data, sizeInBytes);
}

void EffectChain::getStateInformation(juce::MemoryBlock& destData)
{
    juce::XmlElement xml("EffectChain");

    xml.setAttribute("bypassed", chainBypassed);
    xml.setAttribute("numPlugins", (int)pluginSlots.size());

    for (size_t i = 0; i < pluginSlots.size(); ++i)
    {
        auto pluginXml = xml.createNewChildElement("Plugin");
        pluginXml->setAttribute("index", (int)i);
        pluginXml->setAttribute("name", pluginSlots[i].name);
        pluginXml->setAttribute("bypassed", pluginSlots[i].bypassed);

        // Save plugin state
        auto node = processorGraph->getNodeForId(pluginSlots[i].nodeId);
        if (node != nullptr && node->getProcessor() != nullptr)
        {
            juce::MemoryBlock pluginState;
            node->getProcessor()->getStateInformation(pluginState);
            pluginXml->setAttribute("state", pluginState.toBase64Encoding());
        }
    }

    copyXmlToBinary(xml, destData);
}

void EffectChain::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr)
        return;

    if (xml->getTagName() != "EffectChain")
        return;

    chainBypassed = xml->getBoolAttribute("bypassed", false);

    // Plugin restoration would require the PluginManager to reload plugins
    // This is a simplified version that just restores bypass states
    for (auto* pluginXml : xml->getChildWithTagNameIterator("Plugin"))
    {
        int index = pluginXml->getIntAttribute("index");
        bool bypassed = pluginXml->getBoolAttribute("bypassed", false);

        if (index >= 0 && index < (int)pluginSlots.size())
        {
            setPluginBypassed(index, bypassed);
        }
    }
}

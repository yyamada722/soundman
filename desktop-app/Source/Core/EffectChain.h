/*
  ==============================================================================

    EffectChain.h

    Audio effect chain using AudioProcessorGraph

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>

class EffectChain : public juce::AudioProcessor
{
public:
    //==========================================================================
    EffectChain();
    ~EffectChain() override;

    //==========================================================================
    // Plugin chain management
    struct PluginSlot
    {
        std::unique_ptr<juce::AudioPluginInstance> plugin;
        juce::AudioProcessorGraph::NodeID nodeId;
        bool bypassed { false };
        juce::String name;
    };

    int addPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin, const juce::String& name);
    void removePlugin(int slotIndex);
    void movePlugin(int fromIndex, int toIndex);
    void clearAllPlugins();

    int getNumPlugins() const { return (int)pluginSlots.size(); }
    PluginSlot* getPluginSlot(int index);
    const PluginSlot* getPluginSlot(int index) const;

    //==========================================================================
    // Bypass control
    void setPluginBypassed(int slotIndex, bool bypassed);
    bool isPluginBypassed(int slotIndex) const;
    void setChainBypassed(bool bypassed) { chainBypassed = bypassed; }
    bool isChainBypassed() const { return chainBypassed; }

    //==========================================================================
    // Plugin editor
    juce::AudioProcessorEditor* createEditorForPlugin(int slotIndex);

    //==========================================================================
    // State save/load
    void saveChainState(juce::MemoryBlock& destData);
    void loadChainState(const void* data, int sizeInBytes);

    //==========================================================================
    // AudioProcessor overrides
    const juce::String getName() const override { return "EffectChain"; }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int /*index*/) override {}
    const juce::String getProgramName(int /*index*/) override { return {}; }
    void changeProgramName(int /*index*/, const juce::String& /*newName*/) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    // Callbacks
    std::function<void()> onChainChanged;

private:
    //==========================================================================
    void rebuildGraph();
    void connectNodes();

    //==========================================================================
    std::unique_ptr<juce::AudioProcessorGraph> processorGraph;
    std::vector<PluginSlot> pluginSlots;

    juce::AudioProcessorGraph::NodeID audioInputNode;
    juce::AudioProcessorGraph::NodeID audioOutputNode;

    bool chainBypassed { false };
    double currentSampleRate { 44100.0 };
    int currentBlockSize { 512 };

    juce::CriticalSection processLock;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectChain)
};

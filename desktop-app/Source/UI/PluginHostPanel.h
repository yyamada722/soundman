/*
  ==============================================================================

    PluginHostPanel.h

    VST3 Plugin hosting UI panel

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Core/PluginManager.h"
#include "../Core/EffectChain.h"
#include <memory>

//==============================================================================
class PluginSlotComponent : public juce::Component
{
public:
    PluginSlotComponent(int slotIndex, const juce::String& name);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSelected(bool selected) { isSelected = selected; repaint(); }
    bool getSelected() const { return isSelected; }

    void setBypassed(bool bypassed);
    bool getBypassed() const { return isBypassed; }

    int getSlotIndex() const { return index; }
    juce::String getPluginName() const { return pluginName; }

    std::function<void()> onRemoveClicked;
    std::function<void()> onBypassToggled;
    std::function<void()> onEditClicked;
    std::function<void()> onSelected;

private:
    int index;
    juce::String pluginName;
    bool isSelected { false };
    bool isBypassed { false };

    juce::TextButton editButton { "Edit" };
    juce::TextButton bypassButton { "Bypass" };
    juce::TextButton removeButton { "X" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSlotComponent)
};

//==============================================================================
class PluginListComponent : public juce::Component,
                            public juce::ListBoxModel
{
public:
    PluginListComponent(PluginManager& manager);

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

    void refreshList();

    std::function<void(const juce::PluginDescription&)> onPluginSelected;

private:
    PluginManager& pluginManager;
    juce::ListBox listBox { "PluginList", this };
    juce::Array<juce::PluginDescription> pluginList;

    juce::TextButton scanButton { "Scan Plugins" };
    juce::Label statusLabel;
    juce::ProgressBar* progressBar { nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginListComponent)
};

//==============================================================================
class EffectChainComponent : public juce::Component
{
public:
    EffectChainComponent(EffectChain& chain);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshChain();

    std::function<void(int)> onSlotSelected;
    std::function<void(int)> onSlotRemoved;
    std::function<void(int)> onSlotBypassToggled;
    std::function<void(int)> onSlotEditRequested;

private:
    EffectChain& effectChain;
    juce::OwnedArray<PluginSlotComponent> slotComponents;
    int selectedSlot { -1 };

    juce::Label titleLabel;
    juce::TextButton clearAllButton { "Clear All" };
    juce::ToggleButton bypassChainButton { "Bypass Chain" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectChainComponent)
};

//==============================================================================
class PluginEditorWindow : public juce::DocumentWindow
{
public:
    PluginEditorWindow(juce::AudioProcessorEditor* editor, const juce::String& name);
    ~PluginEditorWindow() override;

    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};

//==============================================================================
class PluginHostPanel : public juce::Component,
                        public juce::Timer
{
public:
    PluginHostPanel();
    ~PluginHostPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Timer for progress updates
    void timerCallback() override;

    // Access to effect chain for audio processing
    EffectChain& getEffectChain() { return effectChain; }

    // Prepare for audio processing
    void prepare(double sampleRate, int samplesPerBlock);

private:
    void addPluginToChain(const juce::PluginDescription& description);
    void showPluginEditor(int slotIndex);
    void closeAllEditorWindows();

    PluginManager pluginManager;
    EffectChain effectChain;

    std::unique_ptr<PluginListComponent> pluginList;
    std::unique_ptr<EffectChainComponent> chainComponent;

    juce::OwnedArray<PluginEditorWindow> editorWindows;

    double currentSampleRate { 44100.0 };
    int currentBlockSize { 512 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHostPanel)
};

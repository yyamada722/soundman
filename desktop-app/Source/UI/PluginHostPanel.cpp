/*
  ==============================================================================

    PluginHostPanel.cpp

    VST3 Plugin hosting UI implementation

  ==============================================================================
*/

#include "PluginHostPanel.h"

//==============================================================================
// PluginSlotComponent
//==============================================================================
PluginSlotComponent::PluginSlotComponent(int slotIndex, const juce::String& name)
    : index(slotIndex), pluginName(name)
{
    addAndMakeVisible(editButton);
    editButton.onClick = [this]()
    {
        if (onEditClicked)
            onEditClicked();
    };

    addAndMakeVisible(bypassButton);
    bypassButton.setClickingTogglesState(true);
    bypassButton.onClick = [this]()
    {
        isBypassed = bypassButton.getToggleState();
        if (onBypassToggled)
            onBypassToggled();
    };

    addAndMakeVisible(removeButton);
    removeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff8b0000));
    removeButton.onClick = [this]()
    {
        if (onRemoveClicked)
            onRemoveClicked();
    };
}

void PluginSlotComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    if (isSelected)
        g.setColour(juce::Colour(0xff4a4a6a));
    else
        g.setColour(juce::Colour(0xff3a3a3a));

    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    if (isBypassed)
        g.setColour(juce::Colour(0xff606060));
    else
        g.setColour(juce::Colour(0xff4a90e2));

    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 2.0f);

    // Slot number
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawText(juce::String(index + 1), bounds.removeFromLeft(25), juce::Justification::centred);

    // Plugin name
    auto textBounds = bounds.reduced(5);
    textBounds.removeFromRight(180); // Space for buttons
    g.setFont(14.0f);
    if (isBypassed)
        g.setColour(juce::Colours::grey);
    g.drawText(pluginName, textBounds, juce::Justification::centredLeft);
}

void PluginSlotComponent::resized()
{
    auto bounds = getLocalBounds().reduced(5);
    bounds.removeFromLeft(25); // Slot number

    auto buttonArea = bounds.removeFromRight(170);
    removeButton.setBounds(buttonArea.removeFromRight(30).reduced(2));
    bypassButton.setBounds(buttonArea.removeFromRight(60).reduced(2));
    editButton.setBounds(buttonArea.removeFromRight(60).reduced(2));
}

void PluginSlotComponent::setBypassed(bool bypassed)
{
    isBypassed = bypassed;
    bypassButton.setToggleState(bypassed, juce::dontSendNotification);
    repaint();
}

//==============================================================================
// PluginListComponent
//==============================================================================
PluginListComponent::PluginListComponent(PluginManager& manager)
    : pluginManager(manager)
{
    addAndMakeVisible(listBox);
    listBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff2a2a2a));
    listBox.setRowHeight(24);

    addAndMakeVisible(scanButton);
    scanButton.onClick = [this]()
    {
        statusLabel.setText("Scanning...", juce::dontSendNotification);

        pluginManager.onScanStarted = [this]()
        {
            juce::MessageManager::callAsync([this]()
            {
                scanButton.setEnabled(false);
            });
        };

        pluginManager.onScanProgress = [this](float progress)
        {
            juce::MessageManager::callAsync([this, progress]()
            {
                statusLabel.setText("Scanning: " + juce::String(int(progress * 100)) + "%",
                                    juce::dontSendNotification);
            });
        };

        pluginManager.onScanFinished = [this]()
        {
            juce::MessageManager::callAsync([this]()
            {
                scanButton.setEnabled(true);
                refreshList();
                statusLabel.setText("Found " + juce::String(pluginManager.getNumPlugins()) + " plugins",
                                    juce::dontSendNotification);
            });
        };

        pluginManager.onPluginFound = [this](const juce::String& name)
        {
            juce::MessageManager::callAsync([this, name]()
            {
                statusLabel.setText("Found: " + name, juce::dontSendNotification);
            });
        };

        // Run scan in background thread
        std::thread([this]()
        {
            pluginManager.scanForPlugins();
        }).detach();
    };

    addAndMakeVisible(statusLabel);
    statusLabel.setFont(juce::Font(12.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    statusLabel.setText("Click 'Scan Plugins' to find VST3 plugins", juce::dontSendNotification);
}

void PluginListComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("Available Plugins", 10, 5, getWidth() - 20, 20, juce::Justification::centredLeft);
}

void PluginListComponent::resized()
{
    auto bounds = getLocalBounds().reduced(5);

    bounds.removeFromTop(25); // Title

    auto bottomArea = bounds.removeFromBottom(60);
    statusLabel.setBounds(bottomArea.removeFromTop(25));
    scanButton.setBounds(bottomArea.reduced(0, 5));

    listBox.setBounds(bounds);
}

int PluginListComponent::getNumRows()
{
    return pluginList.size();
}

void PluginListComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= pluginList.size())
        return;

    if (rowIsSelected)
        g.fillAll(juce::Colour(0xff4a4a6a));
    else if (rowNumber % 2)
        g.fillAll(juce::Colour(0xff323232));

    g.setColour(juce::Colours::white);
    g.setFont(12.0f);

    const auto& desc = pluginList.getReference(rowNumber);
    g.drawText(desc.name, 10, 0, width - 20, height, juce::Justification::centredLeft);
}

void PluginListComponent::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < pluginList.size() && onPluginSelected)
    {
        onPluginSelected(pluginList[row]);
    }
}

void PluginListComponent::refreshList()
{
    pluginList = pluginManager.getAvailablePlugins();
    listBox.updateContent();
    listBox.repaint();
}

//==============================================================================
// EffectChainComponent
//==============================================================================
EffectChainComponent::EffectChainComponent(EffectChain& chain)
    : effectChain(chain)
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Effect Chain", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(clearAllButton);
    clearAllButton.onClick = [this]()
    {
        effectChain.clearAllPlugins();
        refreshChain();
    };

    addAndMakeVisible(bypassChainButton);
    bypassChainButton.onClick = [this]()
    {
        effectChain.setChainBypassed(bypassChainButton.getToggleState());
    };

    effectChain.onChainChanged = [this]()
    {
        juce::MessageManager::callAsync([this]()
        {
            refreshChain();
        });
    };
}

void EffectChainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Draw chain flow arrows between slots
    if (slotComponents.size() > 1)
    {
        g.setColour(juce::Colour(0xff4a90e2));

        for (int i = 0; i < slotComponents.size() - 1; ++i)
        {
            auto slot1 = slotComponents[i]->getBounds();
            auto slot2 = slotComponents[i + 1]->getBounds();

            int arrowX = getWidth() / 2;

            // Draw arrow pointing down
            juce::Path arrow;
            arrow.addArrow({ (float)arrowX, (float)slot1.getBottom() + 2,
                            (float)arrowX, (float)slot2.getY() - 2 },
                          2.0f, 8.0f, 6.0f);
            g.fillPath(arrow);
        }
    }

    // Draw "empty chain" message
    if (slotComponents.isEmpty())
    {
        g.setColour(juce::Colours::grey);
        g.setFont(14.0f);
        auto textArea = getLocalBounds().reduced(20);
        textArea.removeFromTop(60);
        g.drawText("Double-click a plugin to add it to the chain",
                   textArea, juce::Justification::centredTop);
    }
}

void EffectChainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(5);

    auto headerArea = bounds.removeFromTop(30);
    titleLabel.setBounds(headerArea.removeFromLeft(150));
    clearAllButton.setBounds(headerArea.removeFromRight(80).reduced(2));
    bypassChainButton.setBounds(headerArea.removeFromRight(100).reduced(2));

    bounds.removeFromTop(10);

    // Layout slot components vertically with spacing for arrows
    int slotHeight = 40;
    int spacing = 25; // Space for arrows

    for (auto* slot : slotComponents)
    {
        slot->setBounds(bounds.removeFromTop(slotHeight));
        bounds.removeFromTop(spacing);
    }
}

void EffectChainComponent::refreshChain()
{
    slotComponents.clear();

    for (int i = 0; i < effectChain.getNumPlugins(); ++i)
    {
        auto* slot = effectChain.getPluginSlot(i);
        if (slot != nullptr)
        {
            auto* comp = new PluginSlotComponent(i, slot->name);
            comp->setBypassed(slot->bypassed);

            comp->onRemoveClicked = [this, i]()
            {
                if (onSlotRemoved)
                    onSlotRemoved(i);
            };

            comp->onBypassToggled = [this, i]()
            {
                if (onSlotBypassToggled)
                    onSlotBypassToggled(i);
            };

            comp->onEditClicked = [this, i]()
            {
                if (onSlotEditRequested)
                    onSlotEditRequested(i);
            };

            comp->onSelected = [this, i, comp]()
            {
                for (auto* s : slotComponents)
                    s->setSelected(false);
                comp->setSelected(true);
                selectedSlot = i;
                if (onSlotSelected)
                    onSlotSelected(i);
            };

            slotComponents.add(comp);
            addAndMakeVisible(comp);
        }
    }

    resized();
    repaint();
}

//==============================================================================
// PluginEditorWindow
//==============================================================================
PluginEditorWindow::PluginEditorWindow(juce::AudioProcessorEditor* editor, const juce::String& name)
    : DocumentWindow(name, juce::Colour(0xff2a2a2a), DocumentWindow::closeButton)
{
    setContentOwned(editor, true);
    setResizable(true, false);
    centreWithSize(editor->getWidth(), editor->getHeight());
    setVisible(true);
}

PluginEditorWindow::~PluginEditorWindow()
{
}

void PluginEditorWindow::closeButtonPressed()
{
    setVisible(false);
}

//==============================================================================
// PluginHostPanel
//==============================================================================
PluginHostPanel::PluginHostPanel()
{
    pluginList = std::make_unique<PluginListComponent>(pluginManager);
    addAndMakeVisible(pluginList.get());

    pluginList->onPluginSelected = [this](const juce::PluginDescription& desc)
    {
        addPluginToChain(desc);
    };

    chainComponent = std::make_unique<EffectChainComponent>(effectChain);
    addAndMakeVisible(chainComponent.get());

    chainComponent->onSlotRemoved = [this](int index)
    {
        effectChain.removePlugin(index);
    };

    chainComponent->onSlotBypassToggled = [this](int index)
    {
        bool bypassed = effectChain.isPluginBypassed(index);
        effectChain.setPluginBypassed(index, !bypassed);
    };

    chainComponent->onSlotEditRequested = [this](int index)
    {
        showPluginEditor(index);
    };
}

PluginHostPanel::~PluginHostPanel()
{
    closeAllEditorWindows();
}

void PluginHostPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void PluginHostPanel::resized()
{
    auto bounds = getLocalBounds();

    // Left side: plugin list (40%)
    auto listBounds = bounds.removeFromLeft(static_cast<int>(bounds.getWidth() * 0.4f));
    pluginList->setBounds(listBounds);

    // Right side: effect chain (60%)
    chainComponent->setBounds(bounds);
}

void PluginHostPanel::timerCallback()
{
    // Progress updates are handled via callbacks
}

void PluginHostPanel::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    effectChain.prepareToPlay(sampleRate, samplesPerBlock);
}

void PluginHostPanel::addPluginToChain(const juce::PluginDescription& description)
{
    juce::String errorMessage;

    auto plugin = pluginManager.loadPlugin(description, currentSampleRate, currentBlockSize, errorMessage);

    if (plugin != nullptr)
    {
        effectChain.addPlugin(std::move(plugin), description.name);
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Plugin Load Error",
            "Failed to load plugin: " + errorMessage);
    }
}

void PluginHostPanel::showPluginEditor(int slotIndex)
{
    auto* editor = effectChain.createEditorForPlugin(slotIndex);

    if (editor != nullptr)
    {
        auto* slot = effectChain.getPluginSlot(slotIndex);
        juce::String windowName = slot ? slot->name : "Plugin Editor";

        auto* window = new PluginEditorWindow(editor, windowName);
        editorWindows.add(window);
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "No Editor",
            "This plugin does not have a graphical editor.");
    }
}

void PluginHostPanel::closeAllEditorWindows()
{
    editorWindows.clear();
}

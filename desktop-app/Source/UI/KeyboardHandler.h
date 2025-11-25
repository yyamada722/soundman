/*
  ==============================================================================

    KeyboardHandler.h

    Keyboard shortcut handler for the application

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <map>

class KeyboardHandler : public juce::KeyListener
{
public:
    //==========================================================================
    using KeyCallback = std::function<void()>;

    //==========================================================================
    KeyboardHandler();
    ~KeyboardHandler() override;

    //==========================================================================
    // Register keyboard shortcuts
    void registerCommand(const juce::KeyPress& keyPress, KeyCallback callback);
    void clearCommands();

    //==========================================================================
    // KeyListener overrides
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

private:
    //==========================================================================
    // Use string representation of KeyPress as key for better compatibility
    std::map<juce::String, KeyCallback> commands;
    std::map<juce::String, juce::KeyPress> keyPresses;  // Store original KeyPress for matching

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyboardHandler)
};

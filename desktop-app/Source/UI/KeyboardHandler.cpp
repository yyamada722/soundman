/*
  ==============================================================================

    KeyboardHandler.cpp

    Keyboard shortcut handler implementation

  ==============================================================================
*/

#include "KeyboardHandler.h"

//==============================================================================
KeyboardHandler::KeyboardHandler()
{
}

KeyboardHandler::~KeyboardHandler()
{
}

//==============================================================================
void KeyboardHandler::registerCommand(const juce::KeyPress& keyPress, KeyCallback callback)
{
    auto keyString = keyPress.getTextDescription();
    commands[keyString] = callback;
    keyPresses[keyString] = keyPress;
}

void KeyboardHandler::clearCommands()
{
    commands.clear();
    keyPresses.clear();
}

//==============================================================================
bool KeyboardHandler::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    // Check if this key matches any registered command
    for (const auto& pair : keyPresses)
    {
        if (pair.second == key)
        {
            auto cmdIt = commands.find(pair.first);
            if (cmdIt != commands.end() && cmdIt->second)
            {
                cmdIt->second();
                return true;  // Key was handled
            }
        }
    }

    return false;  // Key was not handled
}

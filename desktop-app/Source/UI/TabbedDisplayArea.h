/*
  ==============================================================================

    TabbedDisplayArea.h

    Tabbed display area for switching between different visualization modes

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>

class TabbedDisplayArea : public juce::Component
{
public:
    //==========================================================================
    struct Tab
    {
        juce::String name;
        juce::Component* content;
        bool isVisible;

        Tab(const juce::String& n, juce::Component* c)
            : name(n), content(c), isVisible(true)
        {
        }
    };

    //==========================================================================
    TabbedDisplayArea();
    ~TabbedDisplayArea() override;

    //==========================================================================
    // Tab management
    void addTab(const juce::String& tabName, juce::Component* content);
    void removeTab(int index);
    void removeTab(const juce::String& tabName);
    void clearTabs();

    //==========================================================================
    // Tab selection
    void setCurrentTab(int index);
    void setCurrentTab(const juce::String& tabName);
    int getCurrentTabIndex() const { return currentTabIndex; }
    juce::String getCurrentTabName() const;

    //==========================================================================
    // Tab control
    void setTabVisible(int index, bool visible);
    int getTabCount() const { return (int)tabs.size(); }
    int getNumTabs() const { return (int)tabs.size(); }
    juce::String getTabName(int index) const
    {
        if (index >= 0 && index < (int)tabs.size())
            return tabs[index].name;
        return {};
    }

    //==========================================================================
    // Callbacks
    using TabChangedCallback = std::function<void(int, const juce::String&)>;
    void setTabChangedCallback(TabChangedCallback callback) { tabChangedCallback = callback; }

    //==========================================================================
    // Appearance
    void setTabHeight(int height) { tabHeight = height; resized(); }
    int getTabHeight() const { return tabHeight; }

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

private:
    //==========================================================================
    void updateLayout();
    int getTabIndexAt(int x, int y) const;
    juce::Rectangle<int> getTabBounds(int index) const;
    void notifyTabChanged();

    //==========================================================================
    std::vector<Tab> tabs;
    int currentTabIndex { -1 };
    int hoveredTabIndex { -1 };
    int tabHeight { 40 };

    TabChangedCallback tabChangedCallback;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabbedDisplayArea)
};

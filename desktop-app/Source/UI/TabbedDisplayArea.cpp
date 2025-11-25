/*
  ==============================================================================

    TabbedDisplayArea.cpp

    Tabbed display area implementation

  ==============================================================================
*/

#include "TabbedDisplayArea.h"

//==============================================================================
TabbedDisplayArea::TabbedDisplayArea()
{
}

TabbedDisplayArea::~TabbedDisplayArea()
{
    clearTabs();
}

//==============================================================================
void TabbedDisplayArea::addTab(const juce::String& tabName, juce::Component* content)
{
    if (content == nullptr)
        return;

    tabs.emplace_back(tabName, content);
    addAndMakeVisible(content);

    // If this is the first tab, select it
    if (currentTabIndex < 0)
        setCurrentTab(0);
    else
        updateLayout();
}

void TabbedDisplayArea::removeTab(int index)
{
    if (index < 0 || index >= (int)tabs.size())
        return;

    removeChildComponent(tabs[index].content);
    tabs.erase(tabs.begin() + index);

    // Adjust current tab index
    if (currentTabIndex >= (int)tabs.size())
        currentTabIndex = (int)tabs.size() - 1;

    if (currentTabIndex >= 0)
        setCurrentTab(currentTabIndex);
    else
        repaint();
}

void TabbedDisplayArea::removeTab(const juce::String& tabName)
{
    for (int i = 0; i < (int)tabs.size(); ++i)
    {
        if (tabs[i].name == tabName)
        {
            removeTab(i);
            return;
        }
    }
}

void TabbedDisplayArea::clearTabs()
{
    for (auto& tab : tabs)
        removeChildComponent(tab.content);

    tabs.clear();
    currentTabIndex = -1;
    repaint();
}

//==============================================================================
void TabbedDisplayArea::setCurrentTab(int index)
{
    if (index < 0 || index >= (int)tabs.size())
        return;

    if (currentTabIndex == index)
        return;

    currentTabIndex = index;
    updateLayout();
    notifyTabChanged();
}

void TabbedDisplayArea::setCurrentTab(const juce::String& tabName)
{
    for (int i = 0; i < (int)tabs.size(); ++i)
    {
        if (tabs[i].name == tabName)
        {
            setCurrentTab(i);
            return;
        }
    }
}

juce::String TabbedDisplayArea::getCurrentTabName() const
{
    if (currentTabIndex >= 0 && currentTabIndex < (int)tabs.size())
        return tabs[currentTabIndex].name;

    return {};
}

//==============================================================================
void TabbedDisplayArea::setTabVisible(int index, bool visible)
{
    if (index < 0 || index >= (int)tabs.size())
        return;

    tabs[index].isVisible = visible;
    repaint();
}

//==============================================================================
void TabbedDisplayArea::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Draw tab bar background
    auto tabBarBounds = bounds.removeFromTop(tabHeight);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(tabBarBounds);

    // Draw tabs
    int x = 0;
    for (int i = 0; i < (int)tabs.size(); ++i)
    {
        if (!tabs[i].isVisible)
            continue;

        auto tabBounds = getTabBounds(i);

        // Tab background
        if (i == currentTabIndex)
        {
            // Active tab
            g.setColour(juce::Colour(0xff3a3a3a));
            g.fillRect(tabBounds);
        }
        else if (i == hoveredTabIndex)
        {
            // Hovered tab
            g.setColour(juce::Colour(0xff323232));
            g.fillRect(tabBounds);
        }
        else
        {
            // Inactive tab
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRect(tabBounds);
        }

        // Tab border (bottom accent)
        if (i == currentTabIndex)
        {
            g.setColour(juce::Colour(0xff00aaff));
            g.fillRect(tabBounds.getX(), tabBounds.getBottom() - 3,
                      tabBounds.getWidth(), 3);
        }

        // Tab text
        g.setColour(i == currentTabIndex ? juce::Colours::white : juce::Colours::grey);
        g.setFont(juce::Font(14.0f, i == currentTabIndex ? juce::Font::bold : juce::Font::plain));
        g.drawText(tabs[i].name, tabBounds, juce::Justification::centred);

        // Tab separator
        if (i < (int)tabs.size() - 1)
        {
            g.setColour(juce::Colour(0xff1a1a1a));
            g.drawVerticalLine(tabBounds.getRight(), (float)tabBounds.getY(), (float)tabBounds.getBottom());
        }
    }

    // Draw content area border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);
}

void TabbedDisplayArea::resized()
{
    updateLayout();
}

//==============================================================================
void TabbedDisplayArea::mouseDown(const juce::MouseEvent& event)
{
    int tabIndex = getTabIndexAt(event.x, event.y);

    if (tabIndex >= 0)
        setCurrentTab(tabIndex);
}

void TabbedDisplayArea::mouseMove(const juce::MouseEvent& event)
{
    int newHoveredTab = getTabIndexAt(event.x, event.y);

    if (newHoveredTab != hoveredTabIndex)
    {
        hoveredTabIndex = newHoveredTab;
        repaint();
    }
}

//==============================================================================
void TabbedDisplayArea::updateLayout()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(tabHeight);  // Skip tab bar

    // Hide all content components
    for (auto& tab : tabs)
        tab.content->setVisible(false);

    // Show and position current tab content
    if (currentTabIndex >= 0 && currentTabIndex < (int)tabs.size())
    {
        tabs[currentTabIndex].content->setBounds(bounds);
        tabs[currentTabIndex].content->setVisible(true);
    }

    repaint();
}

int TabbedDisplayArea::getTabIndexAt(int x, int y) const
{
    if (y >= tabHeight)
        return -1;

    for (int i = 0; i < (int)tabs.size(); ++i)
    {
        if (!tabs[i].isVisible)
            continue;

        auto tabBounds = getTabBounds(i);
        if (tabBounds.contains(x, y))
            return i;
    }

    return -1;
}

juce::Rectangle<int> TabbedDisplayArea::getTabBounds(int index) const
{
    if (index < 0 || index >= (int)tabs.size())
        return {};

    // Count visible tabs before this one
    int visibleTabsBefore = 0;
    for (int i = 0; i < index; ++i)
    {
        if (tabs[i].isVisible)
            visibleTabsBefore++;
    }

    // Count total visible tabs
    int totalVisibleTabs = 0;
    for (const auto& tab : tabs)
    {
        if (tab.isVisible)
            totalVisibleTabs++;
    }

    if (totalVisibleTabs == 0)
        return {};

    int tabWidth = getWidth() / totalVisibleTabs;
    int x = visibleTabsBefore * tabWidth;

    return juce::Rectangle<int>(x, 0, tabWidth, tabHeight);
}

void TabbedDisplayArea::notifyTabChanged()
{
    if (tabChangedCallback && currentTabIndex >= 0)
        tabChangedCallback(currentTabIndex, getCurrentTabName());
}

/*
  ==============================================================================

    PanelContainer.cpp

    Resizable panel container implementation

  ==============================================================================
*/

#include "PanelContainer.h"

//==============================================================================
PanelContainer::PanelContainer(Orientation orient)
    : orientation(orient)
{
}

PanelContainer::~PanelContainer()
{
    clearPanels();
}

//==============================================================================
void PanelContainer::addPanel(juce::Component* component, double proportion,
                              int minSize, int maxSize, const juce::String& name)
{
    if (component == nullptr)
        return;

    panels.emplace_back(component, proportion, minSize, maxSize, name);
    addAndMakeVisible(component);

    normalizeProportion();
    resized();
}

void PanelContainer::removePanel(juce::Component* component)
{
    for (size_t i = 0; i < panels.size(); ++i)
    {
        if (panels[i].component == component)
        {
            removePanel((int)i);
            return;
        }
    }
}

void PanelContainer::removePanel(int index)
{
    if (index < 0 || index >= (int)panels.size())
        return;

    removeChildComponent(panels[index].component);
    panels.erase(panels.begin() + index);

    normalizeProportion();
    resized();
}

void PanelContainer::clearPanels()
{
    for (auto& panel : panels)
        removeChildComponent(panel.component);

    panels.clear();
}

//==============================================================================
void PanelContainer::setPanelVisible(int index, bool visible)
{
    if (index < 0 || index >= (int)panels.size())
        return;

    if (panels[index].isVisible != visible)
    {
        panels[index].isVisible = visible;
        panels[index].component->setVisible(visible);
        resized();
    }
}

void PanelContainer::setPanelProportion(int index, double proportion)
{
    if (index < 0 || index >= (int)panels.size())
        return;

    panels[index].proportion = juce::jlimit(0.0, 1.0, proportion);
    normalizeProportion();
    resized();
}

bool PanelContainer::isPanelVisible(int index) const
{
    if (index < 0 || index >= (int)panels.size())
        return false;

    return panels[index].isVisible;
}

//==============================================================================
void PanelContainer::paint(juce::Graphics& g)
{
    // Draw dividers
    g.setColour(juce::Colour(0xff3a3a3a));

    for (int i = 0; i < (int)panels.size() - 1; ++i)
    {
        if (!panels[i].isVisible)
            continue;

        auto dividerBounds = getDividerBounds(i);
        g.fillRect(dividerBounds);

        // Draw highlight on hover
        if (getDividerIndexAt(getMouseXYRelative().x, getMouseXYRelative().y) == i)
        {
            g.setColour(juce::Colour(0xff5a5a5a));
            g.fillRect(dividerBounds);
        }
    }
}

void PanelContainer::resized()
{
    layoutPanels();
}

//==============================================================================
void PanelContainer::layoutPanels()
{
    auto bounds = getLocalBounds();

    if (panels.empty())
        return;

    // Count visible panels
    int visibleCount = 0;
    for (const auto& panel : panels)
    {
        if (panel.isVisible)
            visibleCount++;
    }

    if (visibleCount == 0)
        return;

    // Calculate total available size
    int totalSize = orientation == Orientation::Horizontal
                    ? bounds.getWidth()
                    : bounds.getHeight();

    // Subtract divider widths
    int totalDividers = visibleCount - 1;
    totalSize -= totalDividers * dividerWidth;

    // Calculate sizes for each panel
    std::vector<int> sizes;
    double totalProportion = 0.0;

    // First pass: calculate total proportion
    for (const auto& panel : panels)
    {
        if (panel.isVisible)
            totalProportion += panel.proportion;
    }

    // Second pass: calculate actual sizes
    int allocatedSize = 0;
    for (size_t i = 0; i < panels.size(); ++i)
    {
        if (!panels[i].isVisible)
        {
            sizes.push_back(0);
            continue;
        }

        int size = (int)((panels[i].proportion / totalProportion) * totalSize);

        // Apply min/max constraints
        size = juce::jmax(size, panels[i].minSize);
        if (panels[i].maxSize > 0)
            size = juce::jmin(size, panels[i].maxSize);

        sizes.push_back(size);
        allocatedSize += size;
    }

    // Third pass: distribute remaining space or adjust for overflow
    int remaining = totalSize - allocatedSize;
    if (remaining != 0 && !sizes.empty())
    {
        // Add/subtract from last visible panel
        for (int i = (int)sizes.size() - 1; i >= 0; --i)
        {
            if (sizes[i] > 0)
            {
                sizes[i] += remaining;
                break;
            }
        }
    }

    // Fourth pass: position panels
    int position = 0;
    for (size_t i = 0; i < panels.size(); ++i)
    {
        if (!panels[i].isVisible)
        {
            panels[i].component->setBounds(0, 0, 0, 0);
            continue;
        }

        juce::Rectangle<int> panelBounds;

        if (orientation == Orientation::Horizontal)
        {
            panelBounds = juce::Rectangle<int>(
                bounds.getX() + position,
                bounds.getY(),
                sizes[i],
                bounds.getHeight()
            );
        }
        else  // Vertical
        {
            panelBounds = juce::Rectangle<int>(
                bounds.getX(),
                bounds.getY() + position,
                bounds.getWidth(),
                sizes[i]
            );
        }

        panels[i].component->setBounds(panelBounds);
        position += sizes[i] + dividerWidth;
    }
}

//==============================================================================
void PanelContainer::mouseDown(const juce::MouseEvent& event)
{
    draggingDivider = getDividerIndexAt(event.x, event.y);

    if (draggingDivider >= 0)
    {
        dragStartPosition = orientation == Orientation::Horizontal ? event.x : event.y;

        // Store current proportions
        proportionsBeforeDrag.clear();
        for (const auto& panel : panels)
            proportionsBeforeDrag.push_back(panel.proportion);
    }
}

void PanelContainer::mouseDrag(const juce::MouseEvent& event)
{
    if (draggingDivider < 0)
        return;

    int currentPosition = orientation == Orientation::Horizontal ? event.x : event.y;
    int delta = currentPosition - dragStartPosition;

    updateDividerPosition(draggingDivider, delta);
}

void PanelContainer::mouseUp(const juce::MouseEvent& event)
{
    draggingDivider = -1;
    proportionsBeforeDrag.clear();
}

void PanelContainer::mouseMove(const juce::MouseEvent& event)
{
    int dividerIndex = getDividerIndexAt(event.x, event.y);

    if (dividerIndex >= 0)
    {
        setMouseCursor(orientation == Orientation::Horizontal
                       ? juce::MouseCursor::LeftRightResizeCursor
                       : juce::MouseCursor::UpDownResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    repaint();
}

//==============================================================================
int PanelContainer::getDividerIndexAt(int x, int y) const
{
    for (int i = 0; i < (int)panels.size() - 1; ++i)
    {
        if (!panels[i].isVisible)
            continue;

        auto dividerBounds = getDividerBounds(i);

        if (dividerBounds.contains(x, y))
            return i;
    }

    return -1;
}

juce::Rectangle<int> PanelContainer::getDividerBounds(int dividerIndex) const
{
    if (dividerIndex < 0 || dividerIndex >= (int)panels.size() - 1)
        return {};

    if (!panels[dividerIndex].isVisible)
        return {};

    auto panelBounds = panels[dividerIndex].component->getBounds();

    if (orientation == Orientation::Horizontal)
    {
        return juce::Rectangle<int>(
            panelBounds.getRight(),
            panelBounds.getY(),
            dividerWidth,
            panelBounds.getHeight()
        );
    }
    else  // Vertical
    {
        return juce::Rectangle<int>(
            panelBounds.getX(),
            panelBounds.getBottom(),
            panelBounds.getWidth(),
            dividerWidth
        );
    }
}

void PanelContainer::updateDividerPosition(int dividerIndex, int delta)
{
    if (dividerIndex < 0 || dividerIndex >= (int)panels.size() - 1)
        return;

    // Find next visible panel
    int nextVisibleIndex = dividerIndex + 1;
    while (nextVisibleIndex < (int)panels.size() && !panels[nextVisibleIndex].isVisible)
        nextVisibleIndex++;

    if (nextVisibleIndex >= (int)panels.size())
        return;

    // Get current panel sizes
    auto currentBounds = panels[dividerIndex].component->getBounds();
    auto nextBounds = panels[nextVisibleIndex].component->getBounds();

    int currentSize = orientation == Orientation::Horizontal
                      ? currentBounds.getWidth()
                      : currentBounds.getHeight();
    int nextSize = orientation == Orientation::Horizontal
                   ? nextBounds.getWidth()
                   : nextBounds.getHeight();

    // Calculate new sizes
    int newCurrentSize = currentSize + delta;
    int newNextSize = nextSize - delta;

    // Apply constraints
    newCurrentSize = juce::jmax(newCurrentSize, panels[dividerIndex].minSize);
    newNextSize = juce::jmax(newNextSize, panels[nextVisibleIndex].minSize);

    if (panels[dividerIndex].maxSize > 0)
        newCurrentSize = juce::jmin(newCurrentSize, panels[dividerIndex].maxSize);
    if (panels[nextVisibleIndex].maxSize > 0)
        newNextSize = juce::jmin(newNextSize, panels[nextVisibleIndex].maxSize);

    // Update proportions
    int totalSize = newCurrentSize + newNextSize;
    if (totalSize > 0)
    {
        panels[dividerIndex].proportion = (double)newCurrentSize / totalSize;
        panels[nextVisibleIndex].proportion = (double)newNextSize / totalSize;

        normalizeProportion();
        resized();
    }
}

void PanelContainer::normalizeProportion()
{
    double totalProportion = 0.0;

    for (const auto& panel : panels)
    {
        if (panel.isVisible)
            totalProportion += panel.proportion;
    }

    if (totalProportion > 0.0)
    {
        for (auto& panel : panels)
        {
            if (panel.isVisible)
                panel.proportion /= totalProportion;
        }
    }
}

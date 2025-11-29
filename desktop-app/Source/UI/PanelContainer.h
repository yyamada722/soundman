/*
  ==============================================================================

    PanelContainer.h

    Resizable panel container for modular UI layout

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

class PanelContainer : public juce::Component
{
public:
    //==========================================================================
    enum class Orientation
    {
        Horizontal,  // Panels arranged left-to-right
        Vertical     // Panels arranged top-to-bottom
    };

    struct Panel
    {
        juce::Component* component;
        double proportion;  // 0.0 to 1.0, relative size
        int minSize;        // Minimum size in pixels
        int maxSize;        // Maximum size in pixels (-1 = no limit)
        bool isVisible;     // Panel visibility
        juce::String name;  // Panel identifier

        Panel(juce::Component* comp, double prop = 0.33, int min = 100, int max = -1, const juce::String& n = "")
            : component(comp), proportion(prop), minSize(min), maxSize(max), isVisible(true), name(n)
        {
        }
    };

    //==========================================================================
    explicit PanelContainer(Orientation orient = Orientation::Horizontal);
    ~PanelContainer() override;

    //==========================================================================
    // Panel management
    void addPanel(juce::Component* component, double proportion = 0.33,
                  int minSize = 100, int maxSize = -1, const juce::String& name = "");
    void removePanel(juce::Component* component);
    void removePanel(int index);
    void clearPanels();

    //==========================================================================
    // Panel control
    void setPanelVisible(int index, bool visible);
    void setPanelProportion(int index, double proportion);
    bool isPanelVisible(int index) const;
    int getPanelCount() const { return (int)panels.size(); }

    //==========================================================================
    // Divider control
    void setDividerWidth(int width) { dividerWidth = width; repaint(); }
    int getDividerWidth() const { return dividerWidth; }

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

private:
    //==========================================================================
    void layoutPanels();
    int getDividerIndexAt(int x, int y) const;
    void updateDividerPosition(int dividerIndex, int delta);
    void normalizeProportion();
    juce::Rectangle<int> getDividerBounds(int dividerIndex) const;

    //==========================================================================
    Orientation orientation;
    std::vector<Panel> panels;
    int dividerWidth { 4 };
    int hitAreaWidth { 12 };  // Larger hit area for easier grabbing

    // Dragging state
    int draggingDivider { -1 };
    int dragStartPosition { 0 };
    std::vector<double> proportionsBeforeDrag;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelContainer)
};

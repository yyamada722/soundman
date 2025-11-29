/*
  ==============================================================================

    MarkerPanel.h

    Marker management panel for viewing, adding, and editing markers

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

class MarkerPanel : public juce::Component,
                    public juce::TableListBoxModel
{
public:
    //==========================================================================
    struct Marker
    {
        int id { 0 };
        juce::String name;
        double timeSeconds { 0.0 };
        juce::Colour color { juce::Colours::yellow };
    };

    //==========================================================================
    MarkerPanel();
    ~MarkerPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Marker management
    void addMarker(int id, const juce::String& name, double timeSeconds, juce::Colour color = juce::Colours::yellow);
    void updateMarker(int id, const juce::String& name, double timeSeconds);
    void removeMarker(int id);
    void clearAllMarkers();
    void setMarkers(const std::vector<Marker>& newMarkers);

    // Set current duration for time validation
    void setDuration(double durationSeconds) { duration = durationSeconds; }

    //==========================================================================
    // TableListBoxModel
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent& event) override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent& event) override;

    //==========================================================================
    // Callbacks
    std::function<void(int id, double timeSeconds, const juce::String& name)> onAddMarker;  // Include ID
    std::function<void(int id)> onRemoveMarker;
    std::function<void(int id, const juce::String& name, double timeSeconds)> onMarkerChanged;
    std::function<void(int id)> onJumpToMarker;
    std::function<double()> onGetCurrentPosition;  // Get current playback position

private:
    //==========================================================================
    // Helper class for time input in table
    class TimeInputComponent : public juce::Component
    {
    public:
        TimeInputComponent(MarkerPanel& owner);
        void resized() override;
        void setTime(double seconds);
        double getTime() const;
        void setMarkerId(int id) { markerId = id; }
        int getMarkerId() const { return markerId; }

    private:
        MarkerPanel& owner;
        int markerId { 0 };
        juce::TextEditor hoursInput, minutesInput, secondsInput, msInput;
        juce::Label sep1 { {}, ":" }, sep2 { {}, ":" }, sep3 { {}, "." };
    };

    // Helper class for name editing
    class NameEditComponent : public juce::Component
    {
    public:
        NameEditComponent(MarkerPanel& owner);
        void resized() override;
        void setName(const juce::String& name);
        juce::String getName() const { return nameEditor.getText(); }
        void setMarkerId(int id) { markerId = id; }
        int getMarkerId() const { return markerId; }

    private:
        MarkerPanel& owner;
        int markerId { 0 };
        juce::TextEditor nameEditor;
    };

    // Helper class for color picker button
    class ColorButtonComponent : public juce::Component
    {
    public:
        ColorButtonComponent(MarkerPanel& owner);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void setColor(juce::Colour color) { currentColor = color; repaint(); }
        void setMarkerId(int id) { markerId = id; }
        int getMarkerId() const { return markerId; }

    private:
        MarkerPanel& owner;
        int markerId { 0 };
        juce::Colour currentColor { juce::Colours::yellow };
    };

    // Helper class for action buttons
    class ActionButtonComponent : public juce::Component
    {
    public:
        ActionButtonComponent(MarkerPanel& owner);
        void resized() override;
        void setMarkerId(int id);
        int getMarkerId() const { return markerId; }

    private:
        MarkerPanel& owner;
        int markerId { 0 };
        juce::TextButton jumpButton { ">" };
        juce::TextButton deleteButton { "X" };
    };

    //==========================================================================
    void addMarkerAtCurrentPosition();
    juce::String formatTime(double seconds) const;
    double parseTimeString(const juce::String& text) const;
    void notifyMarkerChanged(int id);
    Marker* findMarker(int id);

    //==========================================================================
    std::vector<Marker> markers;
    int nextMarkerId { 1 };
    double duration { 0.0 };

    // UI
    juce::TableListBox table;
    juce::TextButton addButton { "+ Add Marker" };
    juce::TextButton addAtPositionButton { "+ At Position" };
    juce::TextButton clearAllButton { "Clear All" };

    // Column IDs
    enum ColumnIds
    {
        ColorColumn = 1,
        NameColumn,
        TimeColumn,
        ActionColumn
    };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkerPanel)
};

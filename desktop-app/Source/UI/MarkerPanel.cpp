/*
  ==============================================================================

    MarkerPanel.cpp

    Marker management panel implementation

  ==============================================================================
*/

#include "MarkerPanel.h"

//==============================================================================
// TimeInputComponent

MarkerPanel::TimeInputComponent::TimeInputComponent(MarkerPanel& o)
    : owner(o)
{
    auto setupEditor = [](juce::TextEditor& ed, int maxChars) {
        ed.setFont(juce::Font(11.0f));
        ed.setJustification(juce::Justification::centred);
        ed.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a2a));
        ed.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        ed.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        ed.setInputRestrictions(maxChars, "0123456789");
        ed.setText("00");
    };

    setupEditor(hoursInput, 2);
    setupEditor(minutesInput, 2);
    setupEditor(secondsInput, 2);
    setupEditor(msInput, 3);
    msInput.setText("000");

    addAndMakeVisible(hoursInput);
    addAndMakeVisible(minutesInput);
    addAndMakeVisible(secondsInput);
    addAndMakeVisible(msInput);

    for (auto* sep : { &sep1, &sep2, &sep3 })
    {
        sep->setFont(juce::Font(10.0f));
        sep->setColour(juce::Label::textColourId, juce::Colours::grey);
        sep->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(sep);
    }

    auto notifyChange = [this]()
    {
        if (markerId > 0)
            owner.notifyMarkerChanged(markerId);
    };

    hoursInput.onFocusLost = notifyChange;
    minutesInput.onFocusLost = notifyChange;
    secondsInput.onFocusLost = notifyChange;
    msInput.onFocusLost = notifyChange;
}

void MarkerPanel::TimeInputComponent::resized()
{
    auto bounds = getLocalBounds().reduced(2, 0);
    int fieldW = 20;
    int msFieldW = 26;
    int sepW = 8;

    hoursInput.setBounds(bounds.removeFromLeft(fieldW));
    sep1.setBounds(bounds.removeFromLeft(sepW));
    minutesInput.setBounds(bounds.removeFromLeft(fieldW));
    sep2.setBounds(bounds.removeFromLeft(sepW));
    secondsInput.setBounds(bounds.removeFromLeft(fieldW));
    sep3.setBounds(bounds.removeFromLeft(sepW));
    msInput.setBounds(bounds.removeFromLeft(msFieldW));
}

void MarkerPanel::TimeInputComponent::setTime(double seconds)
{
    int totalMs = static_cast<int>(seconds * 1000.0);
    int h = totalMs / 3600000;
    int m = (totalMs % 3600000) / 60000;
    int s = (totalMs % 60000) / 1000;
    int ms = totalMs % 1000;

    hoursInput.setText(juce::String::formatted("%02d", h), false);
    minutesInput.setText(juce::String::formatted("%02d", m), false);
    secondsInput.setText(juce::String::formatted("%02d", s), false);
    msInput.setText(juce::String::formatted("%03d", ms), false);
}

double MarkerPanel::TimeInputComponent::getTime() const
{
    int h = hoursInput.getText().getIntValue();
    int m = minutesInput.getText().getIntValue();
    int s = secondsInput.getText().getIntValue();
    int ms = msInput.getText().getIntValue();
    return h * 3600.0 + m * 60.0 + s + ms / 1000.0;
}

//==============================================================================
// NameEditComponent

MarkerPanel::NameEditComponent::NameEditComponent(MarkerPanel& o)
    : owner(o)
{
    // Use Meiryo font for Japanese support (available on Windows Vista+)
    nameEditor.setFont(juce::Font("Meiryo", 11.0f, juce::Font::plain));
    nameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    nameEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    nameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    nameEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff4a90e2));

    nameEditor.onFocusLost = [this]() {
        if (markerId > 0)
            owner.notifyMarkerChanged(markerId);
    };
    nameEditor.onReturnKey = [this]() {
        if (markerId > 0)
            owner.notifyMarkerChanged(markerId);
    };

    addAndMakeVisible(nameEditor);
}

void MarkerPanel::NameEditComponent::resized()
{
    nameEditor.setBounds(getLocalBounds().reduced(2, 2));
}

void MarkerPanel::NameEditComponent::setName(const juce::String& name)
{
    nameEditor.setText(name, false);
}

//==============================================================================
// ColorButtonComponent

MarkerPanel::ColorButtonComponent::ColorButtonComponent(MarkerPanel& o)
    : owner(o)
{
}

void MarkerPanel::ColorButtonComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(4);
    g.setColour(currentColor);
    g.fillRoundedRectangle(bounds.toFloat(), 3.0f);
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds.toFloat(), 3.0f, 1.0f);
}

void MarkerPanel::ColorButtonComponent::mouseDown(const juce::MouseEvent&)
{
    if (markerId <= 0)
        return;

    // Cycle through preset colors
    static const juce::Colour colors[] = {
        juce::Colours::yellow,
        juce::Colours::red,
        juce::Colours::green,
        juce::Colours::cyan,
        juce::Colours::orange,
        juce::Colours::magenta,
        juce::Colours::white
    };
    static const int numColors = sizeof(colors) / sizeof(colors[0]);

    int currentIndex = 0;
    for (int i = 0; i < numColors; ++i)
    {
        if (colors[i] == currentColor)
        {
            currentIndex = i;
            break;
        }
    }

    currentColor = colors[(currentIndex + 1) % numColors];

    if (auto* marker = owner.findMarker(markerId))
    {
        marker->color = currentColor;
        owner.notifyMarkerChanged(markerId);
    }

    repaint();
}

//==============================================================================
// ActionButtonComponent

MarkerPanel::ActionButtonComponent::ActionButtonComponent(MarkerPanel& o)
    : owner(o)
{
    jumpButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a90e2));
    jumpButton.setTooltip("Jump to marker");
    jumpButton.onClick = [this]()
    {
        if (markerId > 0 && owner.onJumpToMarker)
            owner.onJumpToMarker(markerId);
    };

    deleteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff8b0000));
    deleteButton.setTooltip("Delete marker");
    deleteButton.onClick = [this]()
    {
        if (markerId > 0)
        {
            if (owner.onRemoveMarker)
                owner.onRemoveMarker(markerId);
            owner.removeMarker(markerId);
        }
    };

    addAndMakeVisible(jumpButton);
    addAndMakeVisible(deleteButton);
}

void MarkerPanel::ActionButtonComponent::setMarkerId(int id)
{
    markerId = id;
}

void MarkerPanel::ActionButtonComponent::resized()
{
    auto bounds = getLocalBounds().reduced(2);
    jumpButton.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2).reduced(1));
    deleteButton.setBounds(bounds.reduced(1));
}

//==============================================================================
// MarkerPanel

// Japanese font name used throughout MarkerPanel
static const char* getJapaneseFontName()
{
    return "Meiryo";
}

MarkerPanel::MarkerPanel()
{
    // Setup table
    table.setModel(this);
    table.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1e1e1e));
    table.setRowHeight(28);
    table.setHeaderHeight(24);

    auto& header = table.getHeader();
    header.addColumn("", ColorColumn, 30, 30, 30, juce::TableHeaderComponent::notResizable);
    header.addColumn("Name", NameColumn, 100, 60, 200);
    header.addColumn("Time", TimeColumn, 120, 100, 150);
    header.addColumn("", ActionColumn, 60, 60, 60, juce::TableHeaderComponent::notResizable);

    header.setColour(juce::TableHeaderComponent::backgroundColourId, juce::Colour(0xff2a2a2a));
    header.setColour(juce::TableHeaderComponent::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible(table);

    // Buttons
    addAndMakeVisible(addButton);
    addButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a6a3a));
    addButton.onClick = [this]()
    {
        double time = 0.0;
        if (onGetCurrentPosition)
            time = onGetCurrentPosition();

        int id = nextMarkerId++;
        juce::String name = "Marker " + juce::String(markers.size() + 1);
        addMarker(id, name, time);

        if (onAddMarker)
            onAddMarker(id, time, name);
    };

    addAndMakeVisible(addAtPositionButton);
    addAtPositionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a90e2));
    addAtPositionButton.setTooltip("Add marker at current playback position");
    addAtPositionButton.onClick = [this]()
    {
        addMarkerAtCurrentPosition();
    };

    addAndMakeVisible(clearAllButton);
    clearAllButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6a3a3a));
    clearAllButton.onClick = [this]()
    {
        clearAllMarkers();
    };
}

MarkerPanel::~MarkerPanel()
{
}

void MarkerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Title - use Japanese font
    auto bounds = getLocalBounds();
    auto titleArea = bounds.removeFromTop(25);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(getJapaneseFontName(), 13.0f, juce::Font::bold));
    g.drawText("MARKERS", titleArea.reduced(8, 0), juce::Justification::centredLeft);

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(getLocalBounds(), 1);
}

void MarkerPanel::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    bounds.removeFromTop(25);  // Title

    // Buttons at bottom
    auto buttonArea = bounds.removeFromBottom(30);
    int buttonWidth = (buttonArea.getWidth() - 8) / 3;
    addButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    addAtPositionButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    clearAllButton.setBounds(buttonArea.reduced(2));

    bounds.removeFromBottom(4);

    // Table
    table.setBounds(bounds);
}

//==============================================================================
void MarkerPanel::addMarker(int id, const juce::String& name, double timeSeconds, juce::Colour color)
{
    // Check for duplicate ID
    for (const auto& m : markers)
    {
        if (m.id == id)
            return;  // Already exists, skip
    }

    Marker marker;
    marker.id = id;
    marker.name = name;
    marker.timeSeconds = juce::jlimit(0.0, duration > 0 ? duration : 999999.0, timeSeconds);
    marker.color = color;

    markers.push_back(marker);

    // Sort by time
    std::sort(markers.begin(), markers.end(),
              [](const Marker& a, const Marker& b) { return a.timeSeconds < b.timeSeconds; });

    table.updateContent();
}

void MarkerPanel::updateMarker(int id, const juce::String& name, double timeSeconds)
{
    if (auto* marker = findMarker(id))
    {
        marker->name = name;
        marker->timeSeconds = juce::jlimit(0.0, duration > 0 ? duration : 999999.0, timeSeconds);

        // Re-sort
        std::sort(markers.begin(), markers.end(),
                  [](const Marker& a, const Marker& b) { return a.timeSeconds < b.timeSeconds; });

        table.updateContent();
    }
}

void MarkerPanel::removeMarker(int id)
{
    markers.erase(std::remove_if(markers.begin(), markers.end(),
                                  [id](const Marker& m) { return m.id == id; }),
                  markers.end());
    table.updateContent();
}

void MarkerPanel::clearAllMarkers()
{
    markers.clear();
    table.updateContent();
}

void MarkerPanel::setMarkers(const std::vector<Marker>& newMarkers)
{
    markers = newMarkers;
    std::sort(markers.begin(), markers.end(),
              [](const Marker& a, const Marker& b) { return a.timeSeconds < b.timeSeconds; });
    table.updateContent();
}

//==============================================================================
int MarkerPanel::getNumRows()
{
    return static_cast<int>(markers.size());
}

void MarkerPanel::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colour(0xff3a5a7a));
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(0xff252525));
    else
        g.fillAll(juce::Colour(0xff1e1e1e));
}

void MarkerPanel::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool)
{
    // Cells are rendered by components, so nothing to paint here
}

juce::Component* MarkerPanel::refreshComponentForCell(int rowNumber, int columnId, bool, juce::Component* existingComponentToUpdate)
{
    if (rowNumber >= static_cast<int>(markers.size()))
    {
        delete existingComponentToUpdate;
        return nullptr;
    }

    auto& marker = markers[static_cast<size_t>(rowNumber)];

    switch (columnId)
    {
        case ColorColumn:
        {
            auto* colorBtn = dynamic_cast<ColorButtonComponent*>(existingComponentToUpdate);
            if (colorBtn == nullptr)
                colorBtn = new ColorButtonComponent(*this);
            colorBtn->setMarkerId(marker.id);  // Always update the marker ID
            colorBtn->setColor(marker.color);
            return colorBtn;
        }

        case NameColumn:
        {
            auto* nameEdit = dynamic_cast<NameEditComponent*>(existingComponentToUpdate);
            if (nameEdit == nullptr)
                nameEdit = new NameEditComponent(*this);
            nameEdit->setMarkerId(marker.id);  // Always update the marker ID
            nameEdit->setName(marker.name);
            return nameEdit;
        }

        case TimeColumn:
        {
            auto* timeInput = dynamic_cast<TimeInputComponent*>(existingComponentToUpdate);
            if (timeInput == nullptr)
                timeInput = new TimeInputComponent(*this);
            timeInput->setMarkerId(marker.id);  // Always update the marker ID
            timeInput->setTime(marker.timeSeconds);
            return timeInput;
        }

        case ActionColumn:
        {
            auto* actionBtns = dynamic_cast<ActionButtonComponent*>(existingComponentToUpdate);
            if (actionBtns == nullptr)
                actionBtns = new ActionButtonComponent(*this);
            actionBtns->setMarkerId(marker.id);  // Always update the marker ID
            return actionBtns;
        }

        default:
            break;
    }

    return nullptr;
}

void MarkerPanel::cellClicked(int rowNumber, int columnId, const juce::MouseEvent&)
{
    table.selectRow(rowNumber);
}

void MarkerPanel::cellDoubleClicked(int rowNumber, int, const juce::MouseEvent&)
{
    if (rowNumber < static_cast<int>(markers.size()))
    {
        if (onJumpToMarker)
            onJumpToMarker(markers[static_cast<size_t>(rowNumber)].id);
    }
}

//==============================================================================
void MarkerPanel::addMarkerAtCurrentPosition()
{
    double time = 0.0;
    if (onGetCurrentPosition)
        time = onGetCurrentPosition();

    int id = nextMarkerId++;
    juce::String name = "M" + juce::String(id);
    addMarker(id, name, time);

    if (onAddMarker)
        onAddMarker(id, time, name);
}

juce::String MarkerPanel::formatTime(double seconds) const
{
    int totalMs = static_cast<int>(seconds * 1000.0);
    int h = totalMs / 3600000;
    int m = (totalMs % 3600000) / 60000;
    int s = (totalMs % 60000) / 1000;
    int ms = totalMs % 1000;
    return juce::String::formatted("%02d:%02d:%02d.%03d", h, m, s, ms);
}

double MarkerPanel::parseTimeString(const juce::String&) const
{
    return 0.0;
}

void MarkerPanel::notifyMarkerChanged(int id)
{
    // Find the marker and get updated values from table components
    for (int row = 0; row < static_cast<int>(markers.size()); ++row)
    {
        if (markers[static_cast<size_t>(row)].id == id)
        {
            // Get time from component
            if (auto* timeComp = dynamic_cast<TimeInputComponent*>(table.getCellComponent(TimeColumn, row)))
            {
                markers[static_cast<size_t>(row)].timeSeconds = timeComp->getTime();
            }

            // Get name from component
            if (auto* nameComp = dynamic_cast<NameEditComponent*>(table.getCellComponent(NameColumn, row)))
            {
                markers[static_cast<size_t>(row)].name = nameComp->getName();
            }

            if (onMarkerChanged)
            {
                auto& m = markers[static_cast<size_t>(row)];
                onMarkerChanged(m.id, m.name, m.timeSeconds);
            }

            // Re-sort after time change
            std::sort(markers.begin(), markers.end(),
                      [](const Marker& a, const Marker& b) { return a.timeSeconds < b.timeSeconds; });

            table.updateContent();
            break;
        }
    }
}

MarkerPanel::Marker* MarkerPanel::findMarker(int id)
{
    for (auto& marker : markers)
    {
        if (marker.id == id)
            return &marker;
    }
    return nullptr;
}

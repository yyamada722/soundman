/*
  ==============================================================================

    AudioTimeline.cpp

    Professional audio timeline implementation

  ==============================================================================
*/

#include "AudioTimeline.h"

//==============================================================================
AudioTimeline::AudioTimeline()
    : thumbnail(512, internalFormatManager, thumbnailCache)
{
    internalFormatManager.registerBasicFormats();
    setWantsKeyboardFocus(true);
    startTimerHz(30);
}

AudioTimeline::~AudioTimeline()
{
    stopTimer();
}

//==============================================================================
bool AudioTimeline::loadFile(const juce::File& file, juce::AudioFormatManager& formatManager)
{
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return false;

    duration = reader->lengthInSamples / reader->sampleRate;
    delete reader;

    thumbnail.setSource(new juce::FileInputSource(file));

    // Reset state
    currentPosition = 0.0;
    selectionStart = 0.0;
    selectionEnd = 0.0;
    loopStart = 0.0;
    loopEnd = duration;
    zoomLevel = 1.0;
    scrollPosition = 0.0;

    repaint();
    return true;
}

void AudioTimeline::clearFile()
{
    thumbnail.clear();
    duration = 0.0;
    currentPosition = 0.0;
    selectionStart = 0.0;
    selectionEnd = 0.0;
    markers.clear();
    repaint();
}

//==============================================================================
void AudioTimeline::setPosition(double position)
{
    currentPosition = juce::jlimit(0.0, 1.0, position);
    repaint();
}

void AudioTimeline::setPositionSeconds(double seconds)
{
    if (duration > 0.0)
        setPosition(seconds / duration);
}

double AudioTimeline::getPositionSeconds() const
{
    return currentPosition * duration;
}

//==============================================================================
void AudioTimeline::setSelection(double startSeconds, double endSeconds)
{
    selectionStart = juce::jlimit(0.0, duration, startSeconds);
    selectionEnd = juce::jlimit(0.0, duration, endSeconds);

    if (selectionStart > selectionEnd)
        std::swap(selectionStart, selectionEnd);

    if (onSelectionChanged)
        onSelectionChanged(selectionStart, selectionEnd);

    repaint();
}

void AudioTimeline::clearSelection()
{
    selectionStart = 0.0;
    selectionEnd = 0.0;
    repaint();
}

//==============================================================================
void AudioTimeline::setLoopRegion(double startSeconds, double endSeconds)
{
    loopStart = juce::jlimit(0.0, duration, startSeconds);
    loopEnd = juce::jlimit(0.0, duration, endSeconds);

    if (loopStart > loopEnd)
        std::swap(loopStart, loopEnd);

    if (onLoopRegionChanged)
        onLoopRegionChanged(loopStart, loopEnd);

    repaint();
}

void AudioTimeline::clearLoopRegion()
{
    loopStart = 0.0;
    loopEnd = duration;
    repaint();
}

void AudioTimeline::setLoopEnabled(bool enabled)
{
    loopEnabled = enabled;
    repaint();
}

void AudioTimeline::setLoopFromSelection()
{
    if (hasSelection())
    {
        setLoopRegion(selectionStart, selectionEnd);
    }
}

//==============================================================================
int AudioTimeline::addMarker(double timeSeconds, const juce::String& name, juce::Colour color)
{
    Marker marker;
    marker.timeSeconds = juce::jlimit(0.0, duration, timeSeconds);
    marker.name = name.isEmpty() ? ("M" + juce::String(nextMarkerId)) : name;
    marker.color = color;
    marker.id = nextMarkerId++;

    markers.push_back(marker);

    // Sort markers by time
    std::sort(markers.begin(), markers.end(),
              [](const Marker& a, const Marker& b) { return a.timeSeconds < b.timeSeconds; });

    if (onMarkerAdded)
        onMarkerAdded(marker.id, marker.timeSeconds, marker.name);

    repaint();
    return marker.id;
}

void AudioTimeline::addMarkerWithId(int id, double timeSeconds, const juce::String& name, juce::Colour color)
{
    // Check for duplicate ID
    for (const auto& m : markers)
    {
        if (m.id == id)
            return;  // Already exists, skip
    }

    Marker marker;
    marker.timeSeconds = juce::jlimit(0.0, duration, timeSeconds);
    marker.name = name.isEmpty() ? ("M" + juce::String(id)) : name;
    marker.color = color;
    marker.id = id;

    // Update nextMarkerId if needed
    if (id >= nextMarkerId)
        nextMarkerId = id + 1;

    markers.push_back(marker);

    // Sort markers by time
    std::sort(markers.begin(), markers.end(),
              [](const Marker& a, const Marker& b) { return a.timeSeconds < b.timeSeconds; });

    // Don't call onMarkerAdded - this is a sync operation
    repaint();
}

void AudioTimeline::updateMarker(int markerId, const juce::String& name, double timeSeconds)
{
    for (auto& marker : markers)
    {
        if (marker.id == markerId)
        {
            marker.name = name;
            marker.timeSeconds = juce::jlimit(0.0, duration, timeSeconds);
            break;
        }
    }

    // Re-sort markers by time
    std::sort(markers.begin(), markers.end(),
              [](const Marker& a, const Marker& b) { return a.timeSeconds < b.timeSeconds; });

    repaint();
}

void AudioTimeline::removeMarker(int markerId)
{
    markers.erase(std::remove_if(markers.begin(), markers.end(),
                                  [markerId](const Marker& m) { return m.id == markerId; }),
                  markers.end());
    repaint();
}

void AudioTimeline::clearAllMarkers()
{
    markers.clear();
    repaint();
}

void AudioTimeline::jumpToMarker(int markerId)
{
    for (const auto& marker : markers)
    {
        if (marker.id == markerId)
        {
            setPositionSeconds(marker.timeSeconds);
            if (onPositionChanged)
                onPositionChanged(currentPosition);
            break;
        }
    }
}

void AudioTimeline::jumpToNextMarker()
{
    double currentTime = getPositionSeconds();
    for (const auto& marker : markers)
    {
        if (marker.timeSeconds > currentTime + 0.01)
        {
            setPositionSeconds(marker.timeSeconds);
            if (onPositionChanged)
                onPositionChanged(currentPosition);
            break;
        }
    }
}

void AudioTimeline::jumpToPreviousMarker()
{
    double currentTime = getPositionSeconds();
    for (auto it = markers.rbegin(); it != markers.rend(); ++it)
    {
        if (it->timeSeconds < currentTime - 0.01)
        {
            setPositionSeconds(it->timeSeconds);
            if (onPositionChanged)
                onPositionChanged(currentPosition);
            break;
        }
    }
}

//==============================================================================
void AudioTimeline::setZoomLevel(double zoom)
{
    zoomLevel = juce::jlimit(1.0, 100.0, zoom);
    repaint();
}

void AudioTimeline::setScrollPosition(double position)
{
    double maxScroll = 1.0 - (1.0 / zoomLevel);
    scrollPosition = juce::jlimit(0.0, maxScroll, position);
    repaint();
}

void AudioTimeline::zoomIn()
{
    // Zoom centered on playhead
    double centerTime = getPositionSeconds();
    setZoomLevel(zoomLevel * 1.5);

    // Adjust scroll to keep playhead centered
    if (duration > 0 && zoomLevel > 1.0)
    {
        double visibleDuration = duration / zoomLevel;
        double newStart = centerTime - visibleDuration / 2.0;
        setScrollPosition(newStart / duration);
    }
}

void AudioTimeline::zoomOut()
{
    double centerTime = getPositionSeconds();
    setZoomLevel(zoomLevel / 1.5);

    if (duration > 0 && zoomLevel > 1.0)
    {
        double visibleDuration = duration / zoomLevel;
        double newStart = centerTime - visibleDuration / 2.0;
        setScrollPosition(newStart / duration);
    }
}

void AudioTimeline::zoomToFit()
{
    zoomLevel = 1.0;
    scrollPosition = 0.0;
    repaint();
}

void AudioTimeline::zoomToSelection()
{
    if (!hasSelection())
        return;

    double selDuration = selectionEnd - selectionStart;
    if (selDuration <= 0)
        return;

    // Calculate zoom to fit selection with some padding
    double padding = selDuration * 0.1;
    double targetDuration = selDuration + padding * 2;
    zoomLevel = duration / targetDuration;
    zoomLevel = juce::jlimit(1.0, 100.0, zoomLevel);

    // Center on selection
    double selCenter = (selectionStart + selectionEnd) / 2.0;
    double visibleDuration = duration / zoomLevel;
    double newStart = selCenter - visibleDuration / 2.0;
    setScrollPosition(newStart / duration);
}

//==============================================================================
void AudioTimeline::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(backgroundColor);

    if (!hasFileLoaded())
    {
        g.setColour(juce::Colours::grey);
        g.setFont(14.0f);
        g.drawText("No audio loaded", bounds, juce::Justification::centred);
        return;
    }

    auto contentBounds = bounds;

    // Draw time ruler
    if (showTimeRuler)
    {
        auto rulerBounds = contentBounds.removeFromTop(rulerHeight);
        drawTimeRuler(g, rulerBounds);
    }

    // Marker track
    if (showMarkers && !markers.empty())
    {
        auto markerBounds = contentBounds.removeFromTop(markerHeight);
        drawMarkers(g, markerBounds);
    }

    // Waveform area
    auto waveformBounds = contentBounds;

    // Draw loop region (behind waveform)
    if (showLoopRegion && loopEnabled)
    {
        drawLoopRegion(g, waveformBounds);
    }

    // Draw selection (behind waveform)
    if (hasSelection())
    {
        drawSelection(g, waveformBounds);
    }

    // Draw waveform
    if (showWaveform)
    {
        drawWaveform(g, waveformBounds);
    }

    // Draw playhead (on top)
    drawPlayhead(g, bounds);

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);
}

void AudioTimeline::drawTimeRuler(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(rulerColor);
    g.fillRect(bounds);

    g.setColour(gridColor);
    g.drawLine((float)bounds.getX(), (float)bounds.getBottom(),
               (float)bounds.getRight(), (float)bounds.getBottom());

    if (duration <= 0)
        return;

    // Calculate tick interval based on zoom
    double tickInterval = getRulerTickInterval();
    double visibleStart = getVisibleStartTime();
    double visibleEnd = getVisibleEndTime();

    // Start from first tick
    double firstTick = std::ceil(visibleStart / tickInterval) * tickInterval;

    g.setFont(10.0f);

    for (double t = firstTick; t <= visibleEnd; t += tickInterval)
    {
        int x = timeToX(t);

        // Major tick
        g.setColour(gridColor);
        g.drawVerticalLine(x, (float)bounds.getY() + bounds.getHeight() * 0.5f, (float)bounds.getBottom());

        // Label
        g.setColour(rulerTextColor);
        juce::String label = formatTime(t);
        g.drawText(label, x - 30, bounds.getY(), 60, bounds.getHeight() - 5,
                   juce::Justification::centred, false);
    }

    // Minor ticks
    double minorInterval = tickInterval / 4.0;
    g.setColour(gridColor.withAlpha(0.5f));

    for (double t = firstTick - tickInterval; t <= visibleEnd; t += minorInterval)
    {
        if (t < visibleStart) continue;
        int x = timeToX(t);
        g.drawVerticalLine(x, (float)bounds.getY() + bounds.getHeight() * 0.75f, (float)bounds.getBottom());
    }
}

void AudioTimeline::drawWaveform(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (thumbnail.getNumChannels() == 0)
        return;

    double visibleStart = getVisibleStartTime();
    double visibleEnd = getVisibleEndTime();

    g.setColour(waveformColor);
    thumbnail.drawChannels(g, bounds, visibleStart, visibleEnd, 1.0f);

    // Center line
    g.setColour(gridColor);
    g.drawHorizontalLine(bounds.getCentreY(), (float)bounds.getX(), (float)bounds.getRight());
}

void AudioTimeline::drawSelection(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    int startX = timeToX(selectionStart);
    int endX = timeToX(selectionEnd);

    g.setColour(selectionColor);
    g.fillRect(startX, bounds.getY(), endX - startX, bounds.getHeight());

    // Selection borders
    g.setColour(waveformColor);
    g.drawVerticalLine(startX, (float)bounds.getY(), (float)bounds.getBottom());
    g.drawVerticalLine(endX, (float)bounds.getY(), (float)bounds.getBottom());
}

void AudioTimeline::drawLoopRegion(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    int startX = timeToX(loopStart);
    int endX = timeToX(loopEnd);

    g.setColour(loopRegionColor);
    g.fillRect(startX, bounds.getY(), endX - startX, bounds.getHeight());

    // Loop region borders with handles
    g.setColour(juce::Colour(0xff00aa00));
    g.fillRect(startX - 3, bounds.getY(), 6, bounds.getHeight());
    g.fillRect(endX - 3, bounds.getY(), 6, bounds.getHeight());
}

void AudioTimeline::drawMarkers(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(juce::Colour(0xff252525));
    g.fillRect(bounds);

    // Use Japanese font for marker names
    g.setFont(juce::Font("Meiryo", 9.0f, juce::Font::plain));

    for (const auto& marker : markers)
    {
        int x = timeToX(marker.timeSeconds);

        // Marker flag
        juce::Path flag;
        flag.addTriangle((float)x, (float)bounds.getY(),
                        (float)x + 10, (float)bounds.getY() + 5,
                        (float)x, (float)bounds.getY() + 10);

        g.setColour(marker.color);
        g.fillPath(flag);

        // Marker line
        g.drawVerticalLine(x, (float)bounds.getY(), (float)bounds.getBottom());

        // Marker name
        g.setColour(juce::Colours::white);
        g.drawText(marker.name, x + 12, bounds.getY(), 80, bounds.getHeight(),
                   juce::Justification::centredLeft, true);
    }
}

void AudioTimeline::drawPlayhead(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    int x = timeToX(getPositionSeconds());

    // Playhead line
    g.setColour(playheadColor);
    g.drawVerticalLine(x, (float)bounds.getY(), (float)bounds.getBottom());

    // Playhead triangle at top
    juce::Path triangle;
    triangle.addTriangle((float)x - 6, (float)bounds.getY(),
                        (float)x + 6, (float)bounds.getY(),
                        (float)x, (float)bounds.getY() + 8);
    g.fillPath(triangle);
}

//==============================================================================
void AudioTimeline::resized()
{
    repaint();
}

void AudioTimeline::timerCallback()
{
    if (thumbnail.isFullyLoaded())
    {
        // Check for thumbnail updates
    }
    repaint();
}

//==============================================================================
int AudioTimeline::timeToX(double seconds) const
{
    auto bounds = getLocalBounds();
    double visibleStart = getVisibleStartTime();
    double visibleDuration = getVisibleDuration();

    if (visibleDuration <= 0)
        return bounds.getX();

    double ratio = (seconds - visibleStart) / visibleDuration;
    return bounds.getX() + static_cast<int>(ratio * bounds.getWidth());
}

double AudioTimeline::xToTime(int x) const
{
    auto bounds = getLocalBounds();
    double visibleStart = getVisibleStartTime();
    double visibleDuration = getVisibleDuration();

    double ratio = static_cast<double>(x - bounds.getX()) / bounds.getWidth();
    return visibleStart + ratio * visibleDuration;
}

double AudioTimeline::getVisibleStartTime() const
{
    return scrollPosition * duration;
}

double AudioTimeline::getVisibleEndTime() const
{
    return getVisibleStartTime() + getVisibleDuration();
}

double AudioTimeline::getVisibleDuration() const
{
    return duration / zoomLevel;
}

juce::String AudioTimeline::formatTime(double seconds) const
{
    switch (timeFormat)
    {
        case TimeFormat::Seconds:
            return juce::String(seconds, 3);

        case TimeFormat::MinSec:
        {
            int mins = static_cast<int>(seconds / 60);
            double secs = seconds - mins * 60;
            return juce::String::formatted("%d:%05.2f", mins, secs);
        }

        case TimeFormat::SMPTE:
        {
            int hours = static_cast<int>(seconds / 3600);
            int mins = static_cast<int>((seconds - hours * 3600) / 60);
            int secs = static_cast<int>(seconds - hours * 3600 - mins * 60);
            int frames = static_cast<int>((seconds - static_cast<int>(seconds)) * 30);
            return juce::String::formatted("%02d:%02d:%02d:%02d", hours, mins, secs, frames);
        }

        case TimeFormat::Samples:
            return juce::String(static_cast<int64_t>(seconds * 44100));

        case TimeFormat::Bars:
        {
            double beatsPerSecond = tempo / 60.0;
            double totalBeats = seconds * beatsPerSecond;
            int bars = static_cast<int>(totalBeats / 4) + 1;
            int beats = static_cast<int>(totalBeats) % 4 + 1;
            return juce::String::formatted("%d.%d", bars, beats);
        }

        default:
            return juce::String(seconds, 2);
    }
}

double AudioTimeline::getRulerTickInterval() const
{
    double visibleDuration = getVisibleDuration();
    auto bounds = getLocalBounds();

    // Aim for roughly 100 pixels between major ticks
    double pixelsPerSecond = bounds.getWidth() / visibleDuration;
    double targetPixels = 100.0;
    double targetInterval = targetPixels / pixelsPerSecond;

    // Round to nice values
    static const double niceIntervals[] = {
        0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5,
        1.0, 2.0, 5.0, 10.0, 15.0, 30.0, 60.0, 120.0, 300.0, 600.0
    };

    for (double interval : niceIntervals)
    {
        if (interval >= targetInterval)
            return interval;
    }

    return 600.0;
}

//==============================================================================
void AudioTimeline::mouseDown(const juce::MouseEvent& event)
{
    if (!hasFileLoaded())
        return;

    double clickTime = xToTime(event.x);

    if (event.mods.isRightButtonDown())
    {
        // Right-click: add marker
        addMarker(clickTime);
        return;
    }

    if (event.mods.isShiftDown())
    {
        // Shift+click: start selection from current position
        selectionAnchor = getPositionSeconds();
        selectionStart = juce::jmin(selectionAnchor, clickTime);
        selectionEnd = juce::jmax(selectionAnchor, clickTime);
        currentDragMode = DragMode::Selecting;
        isSelecting = true;
    }
    else if (event.mods.isAltDown())
    {
        // Alt+click: scroll mode
        currentDragMode = DragMode::Scrolling;
    }
    else
    {
        // Check if clicking on loop handles
        int loopStartX = timeToX(loopStart);
        int loopEndX = timeToX(loopEnd);

        if (loopEnabled && std::abs(event.x - loopStartX) < 8)
        {
            currentDragMode = DragMode::MovingLoopStart;
        }
        else if (loopEnabled && std::abs(event.x - loopEndX) < 8)
        {
            currentDragMode = DragMode::MovingLoopEnd;
        }
        else
        {
            // Normal click: seek
            currentDragMode = DragMode::Seeking;
            setPositionSeconds(clickTime);

            if (onPositionChanged)
                onPositionChanged(currentPosition);
        }
    }

    repaint();
}

void AudioTimeline::mouseDrag(const juce::MouseEvent& event)
{
    if (!hasFileLoaded())
        return;

    double dragTime = xToTime(event.x);
    dragTime = juce::jlimit(0.0, duration, dragTime);

    switch (currentDragMode)
    {
        case DragMode::Seeking:
            setPositionSeconds(dragTime);
            if (onPositionChanged)
                onPositionChanged(currentPosition);
            break;

        case DragMode::Selecting:
            selectionStart = juce::jmin(selectionAnchor, dragTime);
            selectionEnd = juce::jmax(selectionAnchor, dragTime);
            if (onSelectionChanged)
                onSelectionChanged(selectionStart, selectionEnd);
            break;

        case DragMode::MovingLoopStart:
            loopStart = juce::jmin(dragTime, loopEnd - 0.1);
            if (onLoopRegionChanged)
                onLoopRegionChanged(loopStart, loopEnd);
            break;

        case DragMode::MovingLoopEnd:
            loopEnd = juce::jmax(dragTime, loopStart + 0.1);
            if (onLoopRegionChanged)
                onLoopRegionChanged(loopStart, loopEnd);
            break;

        case DragMode::Scrolling:
        {
            double deltaX = event.getDistanceFromDragStartX();
            double deltaTime = -deltaX / (getWidth() / getVisibleDuration());
            // Scroll based on drag distance
            break;
        }

        default:
            break;
    }

    repaint();
}

void AudioTimeline::mouseUp(const juce::MouseEvent& event)
{
    currentDragMode = DragMode::None;
    isSelecting = false;
}

void AudioTimeline::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (!hasFileLoaded())
        return;

    // Double-click: zoom to fit or zoom to selection
    if (hasSelection())
    {
        zoomToSelection();
    }
    else
    {
        zoomToFit();
    }
}

void AudioTimeline::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (!hasFileLoaded())
        return;

    if (event.mods.isCtrlDown() || event.mods.isCommandDown())
    {
        // Ctrl+wheel: zoom
        double mouseTime = xToTime(event.x);

        if (wheel.deltaY > 0)
            setZoomLevel(zoomLevel * 1.2);
        else
            setZoomLevel(zoomLevel / 1.2);

        // Adjust scroll to keep mouse position stable
        if (zoomLevel > 1.0)
        {
            double visibleDuration = duration / zoomLevel;
            double newStart = mouseTime - (event.x - getX()) / (double)getWidth() * visibleDuration;
            setScrollPosition(newStart / duration);
        }
    }
    else
    {
        // Normal wheel: scroll horizontally
        double scrollAmount = wheel.deltaY * 0.1 / zoomLevel;
        setScrollPosition(scrollPosition - scrollAmount);
    }

    repaint();
}

bool AudioTimeline::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress('m') || key == juce::KeyPress('M'))
    {
        // Add marker at current position
        addMarker(getPositionSeconds());
        return true;
    }

    if (key == juce::KeyPress::leftKey)
    {
        // Previous marker
        jumpToPreviousMarker();
        return true;
    }

    if (key == juce::KeyPress::rightKey)
    {
        // Next marker
        jumpToNextMarker();
        return true;
    }

    if (key == juce::KeyPress('l') || key == juce::KeyPress('L'))
    {
        // Toggle loop
        setLoopEnabled(!loopEnabled);
        return true;
    }

    if (key == juce::KeyPress('i') || key == juce::KeyPress('I'))
    {
        // Set loop in point
        loopStart = getPositionSeconds();
        if (onLoopRegionChanged)
            onLoopRegionChanged(loopStart, loopEnd);
        repaint();
        return true;
    }

    if (key == juce::KeyPress('o') || key == juce::KeyPress('O'))
    {
        // Set loop out point
        loopEnd = getPositionSeconds();
        if (onLoopRegionChanged)
            onLoopRegionChanged(loopStart, loopEnd);
        repaint();
        return true;
    }

    if (key == juce::KeyPress::homeKey)
    {
        setPosition(0.0);
        if (onPositionChanged)
            onPositionChanged(currentPosition);
        return true;
    }

    if (key == juce::KeyPress::endKey)
    {
        setPosition(1.0);
        if (onPositionChanged)
            onPositionChanged(currentPosition);
        return true;
    }

    return false;
}

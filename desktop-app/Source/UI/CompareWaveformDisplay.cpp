/*
  ==============================================================================

    CompareWaveformDisplay.cpp

    Dual waveform overlay display implementation

  ==============================================================================
*/

#include "CompareWaveformDisplay.h"

//==============================================================================
CompareWaveformDisplay::CompareWaveformDisplay()
    : thumbnailA(512, internalFormatManager, thumbnailCache),
      thumbnailB(512, internalFormatManager, thumbnailCache)
{
    internalFormatManager.registerBasicFormats();
    startTimer(100);  // Update at 10Hz
}

CompareWaveformDisplay::~CompareWaveformDisplay()
{
    stopTimer();
}

//==============================================================================
bool CompareWaveformDisplay::loadTrackA(const juce::File& file, juce::AudioFormatManager& formatManager)
{
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return false;

    thumbnailA.setSource(new juce::FileInputSource(file));
    trackAName = file.getFileName();
    trackADuration = reader->lengthInSamples / reader->sampleRate;

    delete reader;
    repaint();
    return true;
}

bool CompareWaveformDisplay::loadTrackB(const juce::File& file, juce::AudioFormatManager& formatManager)
{
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
        return false;

    thumbnailB.setSource(new juce::FileInputSource(file));
    trackBName = file.getFileName();
    trackBDuration = reader->lengthInSamples / reader->sampleRate;

    delete reader;
    repaint();
    return true;
}

void CompareWaveformDisplay::clearTrackA()
{
    thumbnailA.clear();
    trackAName.clear();
    trackADuration = 0.0;
    repaint();
}

void CompareWaveformDisplay::clearTrackB()
{
    thumbnailB.clear();
    trackBName.clear();
    trackBDuration = 0.0;
    repaint();
}

//==============================================================================
void CompareWaveformDisplay::setDisplayMode(DisplayMode mode)
{
    if (displayMode != mode)
    {
        displayMode = mode;
        repaint();
    }
}

void CompareWaveformDisplay::setPosition(double position)
{
    currentPosition = juce::jlimit(0.0, 1.0, position);
    repaint();
}

void CompareWaveformDisplay::setZoomRange(double start, double end)
{
    zoomStart = juce::jlimit(0.0, 1.0, start);
    zoomEnd = juce::jlimit(0.0, 1.0, end);
    if (zoomEnd <= zoomStart)
        zoomEnd = zoomStart + 0.01;
    repaint();
}

void CompareWaveformDisplay::resetZoom()
{
    zoomStart = 0.0;
    zoomEnd = 1.0;
    repaint();
}

//==============================================================================
void CompareWaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(bounds);

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);

    auto waveformBounds = bounds.reduced(5);

    // Draw track labels
    auto labelArea = waveformBounds.removeFromTop(20);
    g.setFont(11.0f);

    if (hasTrackA())
    {
        g.setColour(trackAColor);
        g.fillRoundedRectangle(labelArea.getX(), labelArea.getY(), 8, 8, 2);
        g.setColour(juce::Colours::white);
        g.drawText("A: " + trackAName, labelArea.getX() + 12, labelArea.getY(),
                   labelArea.getWidth() / 2 - 15, labelArea.getHeight(),
                   juce::Justification::centredLeft);
    }

    if (hasTrackB())
    {
        g.setColour(trackBColor);
        g.fillRoundedRectangle(labelArea.getCentreX() + 5, labelArea.getY(), 8, 8, 2);
        g.setColour(juce::Colours::white);
        g.drawText("B: " + trackBName, labelArea.getCentreX() + 17, labelArea.getY(),
                   labelArea.getWidth() / 2 - 20, labelArea.getHeight(),
                   juce::Justification::centredLeft);
    }

    // Draw mode indicator
    juce::String modeText;
    switch (displayMode)
    {
        case DisplayMode::Overlay: modeText = "Overlay"; break;
        case DisplayMode::Split: modeText = "Split"; break;
        case DisplayMode::Difference: modeText = "Diff"; break;
    }
    g.setColour(juce::Colours::grey);
    g.drawText(modeText, labelArea, juce::Justification::centredRight);

    waveformBounds.removeFromTop(5);

    // Draw waveforms based on display mode
    if (displayMode == DisplayMode::Split)
    {
        // Split view: A on top, B on bottom
        auto topHalf = waveformBounds.removeFromTop(waveformBounds.getHeight() / 2);
        auto bottomHalf = waveformBounds;

        // Draw divider
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawHorizontalLine(topHalf.getBottom(), topHalf.getX(), topHalf.getRight());

        if (showTrackA && hasTrackA())
            drawWaveform(g, thumbnailA, topHalf, trackAColor, trackAAlpha);

        if (showTrackB && hasTrackB())
            drawWaveform(g, thumbnailB, bottomHalf, trackBColor, trackBAlpha);
    }
    else if (displayMode == DisplayMode::Difference)
    {
        // Difference view
        drawDifferenceWaveform(g, waveformBounds);
    }
    else  // Overlay
    {
        // Draw B first (behind)
        if (showTrackB && hasTrackB())
            drawWaveform(g, thumbnailB, waveformBounds, trackBColor, trackBAlpha * 0.7f);

        // Draw A on top
        if (showTrackA && hasTrackA())
            drawWaveform(g, thumbnailA, waveformBounds, trackAColor, trackAAlpha);
    }

    // Draw playhead
    if (currentPosition > 0.0)
    {
        int playheadX = positionToX(currentPosition);
        g.setColour(juce::Colours::white);
        g.drawVerticalLine(playheadX, waveformBounds.getY(), waveformBounds.getBottom());
    }

    // Draw "no track loaded" message if empty
    if (!hasTrackA() && !hasTrackB())
    {
        g.setColour(juce::Colours::grey);
        g.setFont(14.0f);
        g.drawText("Load tracks for A/B comparison",
                   bounds, juce::Justification::centred);
    }
}

void CompareWaveformDisplay::drawWaveform(juce::Graphics& g, juce::AudioThumbnail& thumbnail,
                                           juce::Rectangle<int> bounds, juce::Colour color, float alpha)
{
    if (thumbnail.getNumChannels() == 0)
        return;

    double duration = thumbnail.getTotalLength();
    if (duration <= 0.0)
        return;

    double startTime = zoomStart * duration;
    double endTime = zoomEnd * duration;

    g.setColour(color.withAlpha(alpha));
    thumbnail.drawChannels(g, bounds, startTime, endTime, 1.0f);
}

void CompareWaveformDisplay::drawDifferenceWaveform(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // For difference mode, we'd need sample-level access which thumbnails don't provide
    // Instead, draw both waveforms with different styling to show comparison

    if (showTrackA && hasTrackA())
    {
        g.setColour(trackAColor.withAlpha(0.5f));
        drawWaveform(g, thumbnailA, bounds, trackAColor, 0.5f);
    }

    if (showTrackB && hasTrackB())
    {
        // Draw B with inverted/different style
        g.setColour(trackBColor.withAlpha(0.5f));
        drawWaveform(g, thumbnailB, bounds, trackBColor, 0.5f);
    }

    // Draw center line to help visualize differences
    g.setColour(juce::Colour(0xff404040));
    g.drawHorizontalLine(bounds.getCentreY(), bounds.getX(), bounds.getRight());
}

void CompareWaveformDisplay::resized()
{
    repaint();
}

void CompareWaveformDisplay::mouseDown(const juce::MouseEvent& event)
{
    if (hasTrackA() || hasTrackB())
    {
        isDragging = true;
        double newPosition = xToPosition(event.x);

        if (onSeek)
            onSeek(newPosition);

        currentPosition = newPosition;
        repaint();
    }
}

void CompareWaveformDisplay::mouseDrag(const juce::MouseEvent& event)
{
    if (isDragging && (hasTrackA() || hasTrackB()))
    {
        double newPosition = xToPosition(event.x);

        if (onSeek)
            onSeek(newPosition);

        currentPosition = newPosition;
        repaint();
    }
}

void CompareWaveformDisplay::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Zoom with mouse wheel
    double zoomCenter = xToPosition(event.x);
    double zoomRange = zoomEnd - zoomStart;
    double zoomFactor = wheel.deltaY > 0 ? 0.8 : 1.25;

    double newRange = zoomRange * zoomFactor;
    newRange = juce::jlimit(0.01, 1.0, newRange);

    double newStart = zoomCenter - newRange * (zoomCenter - zoomStart) / zoomRange;
    double newEnd = newStart + newRange;

    if (newStart < 0.0)
    {
        newStart = 0.0;
        newEnd = newRange;
    }
    if (newEnd > 1.0)
    {
        newEnd = 1.0;
        newStart = 1.0 - newRange;
    }

    setZoomRange(newStart, newEnd);
}

void CompareWaveformDisplay::timerCallback()
{
    // Check if thumbnails are still loading
    if (thumbnailA.isFullyLoaded() && thumbnailB.isFullyLoaded())
    {
        // Both loaded, no need for frequent updates
    }
    repaint();
}

double CompareWaveformDisplay::xToPosition(int x) const
{
    auto bounds = getLocalBounds().reduced(5);
    bounds.removeFromTop(25);  // Account for label area

    double normalizedX = (double)(x - bounds.getX()) / bounds.getWidth();
    double position = zoomStart + normalizedX * (zoomEnd - zoomStart);
    return juce::jlimit(0.0, 1.0, position);
}

int CompareWaveformDisplay::positionToX(double position) const
{
    auto bounds = getLocalBounds().reduced(5);
    bounds.removeFromTop(25);  // Account for label area

    double normalizedPos = (position - zoomStart) / (zoomEnd - zoomStart);
    return bounds.getX() + static_cast<int>(normalizedPos * bounds.getWidth());
}

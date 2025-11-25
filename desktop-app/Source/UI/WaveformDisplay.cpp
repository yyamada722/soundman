/*
  ==============================================================================

    WaveformDisplay.cpp

    Audio waveform display implementation

  ==============================================================================
*/

#include "WaveformDisplay.h"

//==============================================================================
WaveformDisplay::WaveformDisplay()
{
    // Start timer for smooth position updates (30 fps)
    startTimer(33);
}

WaveformDisplay::~WaveformDisplay()
{
    stopTimer();
}

//==============================================================================
void WaveformDisplay::loadFile(const juce::File& file, juce::AudioFormatManager& formatManager)
{
    // Create reader for the file
    reader.reset(formatManager.createReaderFor(file));

    if (reader != nullptr)
    {
        duration = reader->lengthInSamples / reader->sampleRate;
        fileLoaded = true;

        // Generate thumbnail in background
        generateThumbnail();

        repaint();
    }
    else
    {
        clear();
    }
}

void WaveformDisplay::clear()
{
    reader.reset();
    thumbnailData.setSize(0, 0);
    fileLoaded = false;
    currentPosition = 0.0;
    duration = 0.0;
    thumbnailSamples = 0;
    resetZoom();
    repaint();
}

//==============================================================================
void WaveformDisplay::setPosition(double position)
{
    if (currentPosition != position)
    {
        currentPosition = juce::jlimit(0.0, 1.0, position);
        repaint();
    }
}

//==============================================================================
void WaveformDisplay::setZoom(double newZoom, double centerPosition)
{
    newZoom = juce::jlimit(MIN_ZOOM, MAX_ZOOM, newZoom);

    if (newZoom == zoomLevel)
        return;

    // Determine the center point for zooming
    double center = centerPosition;
    if (center < 0.0)
    {
        // Use current view center if not specified
        center = (viewStart + viewEnd) * 0.5;
    }

    // Calculate new view range
    double viewWidth = 1.0 / newZoom;
    viewStart = center - viewWidth * 0.5;
    viewEnd = center + viewWidth * 0.5;

    zoomLevel = newZoom;

    // Constrain to valid range
    constrainViewRange();

    repaint();
}

void WaveformDisplay::zoomIn(double centerPosition)
{
    setZoom(zoomLevel * ZOOM_STEP, centerPosition);
}

void WaveformDisplay::zoomOut(double centerPosition)
{
    setZoom(zoomLevel / ZOOM_STEP, centerPosition);
}

void WaveformDisplay::resetZoom()
{
    zoomLevel = 1.0;
    viewStart = 0.0;
    viewEnd = 1.0;
    repaint();
}

//==============================================================================
void WaveformDisplay::generateThumbnail()
{
    if (reader == nullptr)
        return;

    // Calculate thumbnail resolution (samples per pixel, roughly)
    const int targetSamples = 2048;  // Number of sample points for thumbnail
    const int samplesPerBlock = (int)(reader->lengthInSamples / targetSamples);

    if (samplesPerBlock < 1)
        return;

    thumbnailSamples = (int)(reader->lengthInSamples / samplesPerBlock);

    // Allocate thumbnail buffer (min and max for each channel)
    thumbnailData.setSize(reader->numChannels * 2, thumbnailSamples);
    thumbnailData.clear();

    // Read and downsample audio data
    const int blockSize = 8192;
    juce::AudioBuffer<float> readBuffer(reader->numChannels, blockSize);

    int thumbnailIndex = 0;
    int samplesInBlock = 0;

    std::vector<float> minValues(reader->numChannels, 0.0f);
    std::vector<float> maxValues(reader->numChannels, 0.0f);

    for (juce::int64 pos = 0; pos < reader->lengthInSamples; pos += blockSize)
    {
        const int numToRead = juce::jmin(blockSize, (int)(reader->lengthInSamples - pos));

        if (!reader->read(&readBuffer, 0, numToRead, pos, true, true))
            break;

        // Process each sample in the block
        for (int i = 0; i < numToRead; ++i)
        {
            for (int ch = 0; ch < reader->numChannels; ++ch)
            {
                float sample = readBuffer.getSample(ch, i);
                minValues[ch] = juce::jmin(minValues[ch], sample);
                maxValues[ch] = juce::jmax(maxValues[ch], sample);
            }

            samplesInBlock++;

            // Store min/max when we've accumulated enough samples
            if (samplesInBlock >= samplesPerBlock && thumbnailIndex < thumbnailSamples)
            {
                for (int ch = 0; ch < reader->numChannels; ++ch)
                {
                    thumbnailData.setSample(ch * 2, thumbnailIndex, minValues[ch]);
                    thumbnailData.setSample(ch * 2 + 1, thumbnailIndex, maxValues[ch]);
                    minValues[ch] = 0.0f;
                    maxValues[ch] = 0.0f;
                }

                thumbnailIndex++;
                samplesInBlock = 0;
            }
        }
    }

    // Store final values
    if (samplesInBlock > 0 && thumbnailIndex < thumbnailSamples)
    {
        for (int ch = 0; ch < reader->numChannels; ++ch)
        {
            thumbnailData.setSample(ch * 2, thumbnailIndex, minValues[ch]);
            thumbnailData.setSample(ch * 2 + 1, thumbnailIndex, maxValues[ch]);
        }
    }
}

//==============================================================================
void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRect(bounds, 1);

    if (!fileLoaded)
    {
        // No file loaded message
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(16.0f));
        g.drawText("No audio file loaded", bounds, juce::Justification::centred);
        return;
    }

    // Draw waveform
    drawWaveform(g, bounds.reduced(2));

    // Draw timeline
    drawTimeline(g, bounds.reduced(2));

    // Draw position marker
    drawPositionMarker(g, bounds.reduced(2));

    // Draw zoom info
    drawZoomInfo(g, bounds.reduced(2));
}

void WaveformDisplay::drawWaveform(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    if (thumbnailSamples == 0 || reader == nullptr)
        return;

    const int numChannels = reader->numChannels;
    const int channelHeight = bounds.getHeight() / numChannels;

    // Calculate visible sample range based on zoom/pan
    int startSample = (int)(viewStart * thumbnailSamples);
    int endSample = (int)(viewEnd * thumbnailSamples);
    startSample = juce::jlimit(0, thumbnailSamples - 1, startSample);
    endSample = juce::jlimit(startSample + 1, thumbnailSamples, endSample);

    // Draw each channel
    for (int ch = 0; ch < numChannels; ++ch)
    {
        juce::Rectangle<int> channelBounds = bounds.withY(bounds.getY() + ch * channelHeight)
                                                   .withHeight(channelHeight);

        const float midY = channelBounds.getCentreY();
        const float halfHeight = channelBounds.getHeight() * 0.45f;

        // Draw center line
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawHorizontalLine((int)midY, (float)channelBounds.getX(), (float)channelBounds.getRight());

        // Draw waveform
        g.setColour(juce::Colour(0xff00aaff));

        juce::Path waveformPath;
        bool firstPoint = true;

        // Draw only visible range
        for (int i = startSample; i < endSample; ++i)
        {
            // Map sample index to screen position based on visible range
            const float normalizedPos = (i - startSample) / (float)(endSample - startSample);
            const float x = channelBounds.getX() + normalizedPos * channelBounds.getWidth();

            const float minVal = thumbnailData.getSample(ch * 2, i);
            const float maxVal = thumbnailData.getSample(ch * 2 + 1, i);

            const float y1 = midY - minVal * halfHeight;
            const float y2 = midY - maxVal * halfHeight;

            if (firstPoint)
            {
                waveformPath.startNewSubPath(x, y1);
                firstPoint = false;
            }
            else
            {
                waveformPath.lineTo(x, y2);
            }
        }

        // Draw path back
        for (int i = endSample - 1; i >= startSample; --i)
        {
            const float normalizedPos = (i - startSample) / (float)(endSample - startSample);
            const float x = channelBounds.getX() + normalizedPos * channelBounds.getWidth();

            const float minVal = thumbnailData.getSample(ch * 2, i);
            const float y1 = midY - minVal * halfHeight;

            waveformPath.lineTo(x, y1);
        }

        waveformPath.closeSubPath();
        g.fillPath(waveformPath);

        // Draw channel label
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(12.0f));
        juce::String label = (numChannels == 1) ? "Mono" : (ch == 0 ? "L" : "R");
        g.drawText(label, channelBounds.removeFromLeft(30).reduced(4),
                  juce::Justification::centredLeft);
    }
}

void WaveformDisplay::drawPositionMarker(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    if (!fileLoaded)
        return;

    // Check if position is within visible range
    if (currentPosition < viewStart || currentPosition > viewEnd)
        return;

    // Map position to screen coordinates considering zoom/pan
    const float normalizedPos = (currentPosition - viewStart) / (viewEnd - viewStart);
    const float x = bounds.getX() + normalizedPos * bounds.getWidth();

    // Draw playback position line
    g.setColour(juce::Colour(0xffff4444));
    g.drawVerticalLine((int)x, (float)bounds.getY(), (float)bounds.getBottom());

    // Draw position marker triangle at top
    juce::Path triangle;
    const float triangleSize = 8.0f;
    triangle.addTriangle(x - triangleSize * 0.5f, bounds.getY(),
                        x + triangleSize * 0.5f, bounds.getY(),
                        x, bounds.getY() + triangleSize);
    g.fillPath(triangle);
}

void WaveformDisplay::drawZoomInfo(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    if (zoomLevel <= 1.0)
        return;

    // Draw zoom level indicator in top-right corner
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::Font(12.0f));

    juce::String zoomText = juce::String::formatted("%.1fx", zoomLevel);
    auto textBounds = juce::Rectangle<int>(bounds.getRight() - 60, bounds.getY() + 5, 55, 20);

    // Semi-transparent background
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRect(textBounds);

    g.setColour(juce::Colours::white);
    g.drawText(zoomText, textBounds, juce::Justification::centred);
}

//==============================================================================
void WaveformDisplay::constrainViewRange()
{
    // Ensure view range stays within 0.0 to 1.0
    double viewWidth = viewEnd - viewStart;

    if (viewStart < 0.0)
    {
        viewStart = 0.0;
        viewEnd = viewWidth;
    }

    if (viewEnd > 1.0)
    {
        viewEnd = 1.0;
        viewStart = 1.0 - viewWidth;
    }

    // Final clamp
    viewStart = juce::jlimit(0.0, 1.0, viewStart);
    viewEnd = juce::jlimit(0.0, 1.0, viewEnd);
}

double WaveformDisplay::screenXToPosition(int x, const juce::Rectangle<int>& bounds) const
{
    // Convert screen x coordinate to audio position (0.0 to 1.0) considering zoom/pan
    double normalizedX = (x - bounds.getX()) / (double)bounds.getWidth();
    normalizedX = juce::jlimit(0.0, 1.0, normalizedX);

    // Map to visible range
    return viewStart + normalizedX * (viewEnd - viewStart);
}

//==============================================================================
void WaveformDisplay::resized()
{
    // Regenerate thumbnail if size changes significantly
    // (Optional: could be optimized to only regenerate when needed)
}

void WaveformDisplay::mouseDown(const juce::MouseEvent& event)
{
    if (!fileLoaded)
        return;

    // Middle mouse button or Ctrl+Left drag for panning when zoomed
    if (zoomLevel > 1.0 && (event.mods.isMiddleButtonDown() || event.mods.isCtrlDown()))
    {
        isPanning = true;
        lastPanX = event.x;
        panStartViewStart = viewStart;
    }
    else
    {
        // Regular click for seeking
        handleSeek(event.x);
    }
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& event)
{
    if (!fileLoaded)
        return;

    if (isPanning)
    {
        // Pan the view
        int deltaX = event.x - lastPanX;
        auto bounds = getLocalBounds().reduced(2);
        double viewWidth = viewEnd - viewStart;

        // Convert pixel delta to position delta
        double deltaPos = -(deltaX / (double)bounds.getWidth()) * viewWidth;

        viewStart = panStartViewStart + deltaPos;
        viewEnd = viewStart + viewWidth;

        constrainViewRange();
        repaint();

        // Don't update lastPanX - keep it relative to start position
    }
    else
    {
        // Regular drag for seeking
        handleSeek(event.x);
    }
}

void WaveformDisplay::mouseUp(const juce::MouseEvent& event)
{
    isPanning = false;
}

void WaveformDisplay::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (!fileLoaded)
        return;

    // Get mouse position as a position in the audio file
    auto bounds = getLocalBounds().reduced(2);
    double centerPos = screenXToPosition(event.x, bounds);

    // Zoom in/out based on wheel direction
    if (wheel.deltaY > 0.0f)
    {
        zoomIn(centerPos);
    }
    else if (wheel.deltaY < 0.0f)
    {
        zoomOut(centerPos);
    }
}

void WaveformDisplay::handleSeek(int x)
{
    auto bounds = getLocalBounds().reduced(2);
    double newPosition = screenXToPosition(x, bounds);

    if (seekCallback)
    {
        seekCallback(newPosition);
    }
}

//==============================================================================
void WaveformDisplay::timerCallback()
{
    // Timer is used for any smooth animations or updates if needed
    // Currently position updates are handled externally
}

void WaveformDisplay::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    // Handle change notifications if needed
    repaint();
}

//==============================================================================
void WaveformDisplay::drawTimeline(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    if (!fileLoaded || duration <= 0.0)
        return;

    const int timelineHeight = 25;
    auto workingBounds = bounds;
    auto timelineBounds = workingBounds.removeFromBottom(timelineHeight);

    // Draw timeline background
    g.setColour(juce::Colour(0xff252525));
    g.fillRect(timelineBounds);

    // Draw top border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawHorizontalLine(timelineBounds.getY(), (float)timelineBounds.getX(), (float)timelineBounds.getRight());

    // Calculate time interval based on zoom level
    double interval = calculateTimeInterval();
    double visibleDuration = (viewEnd - viewStart) * duration;

    // Calculate start time for first marker
    double startTime = viewStart * duration;
    double firstMarkerTime = std::ceil(startTime / interval) * interval;

    // Draw time markers
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(10.0f));

    for (double time = firstMarkerTime; time <= viewStart * duration + visibleDuration; time += interval)
    {
        // Calculate position
        double normalizedPos = (time / duration - viewStart) / (viewEnd - viewStart);
        if (normalizedPos < 0.0 || normalizedPos > 1.0)
            continue;

        int x = bounds.getX() + (int)(normalizedPos * bounds.getWidth());

        // Draw tick mark
        g.setColour(juce::Colour(0xff5a5a5a));
        g.drawVerticalLine(x, (float)timelineBounds.getY(), (float)timelineBounds.getBottom());

        // Draw time label
        g.setColour(juce::Colours::lightgrey);
        juce::String timeText = formatTime(time);
        g.drawText(timeText, x - 30, timelineBounds.getY() + 5, 60, 15,
                  juce::Justification::centred);
    }
}

double WaveformDisplay::calculateTimeInterval() const
{
    if (duration <= 0.0)
        return 1.0;

    double visibleDuration = (viewEnd - viewStart) * duration;

    // Choose appropriate interval based on visible duration
    if (visibleDuration < 1.0)
        return 0.1;  // 100ms intervals for very zoomed in
    else if (visibleDuration < 5.0)
        return 0.5;  // 500ms intervals
    else if (visibleDuration < 10.0)
        return 1.0;  // 1 second intervals
    else if (visibleDuration < 30.0)
        return 5.0;  // 5 second intervals
    else if (visibleDuration < 60.0)
        return 10.0;  // 10 second intervals
    else if (visibleDuration < 300.0)
        return 30.0;  // 30 second intervals
    else if (visibleDuration < 600.0)
        return 60.0;  // 1 minute intervals
    else
        return 120.0;  // 2 minute intervals
}

juce::String WaveformDisplay::formatTime(double seconds) const
{
    int minutes = (int)(seconds / 60.0);
    double secs = seconds - minutes * 60.0;

    if (minutes > 0)
        return juce::String::formatted("%d:%05.2f", minutes, secs);
    else
        return juce::String::formatted("%.2f", secs);
}

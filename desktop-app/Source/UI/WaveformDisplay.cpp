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

    // Draw position marker
    drawPositionMarker(g, bounds.reduced(2));
}

void WaveformDisplay::drawWaveform(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    if (thumbnailSamples == 0 || reader == nullptr)
        return;

    const int numChannels = reader->numChannels;
    const int channelHeight = bounds.getHeight() / numChannels;

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

        for (int i = 0; i < thumbnailSamples; ++i)
        {
            const float x = juce::jmap((float)i, 0.0f, (float)thumbnailSamples,
                                      (float)channelBounds.getX(), (float)channelBounds.getRight());

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
        for (int i = thumbnailSamples - 1; i >= 0; --i)
        {
            const float x = juce::jmap((float)i, 0.0f, (float)thumbnailSamples,
                                      (float)channelBounds.getX(), (float)channelBounds.getRight());

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

    const float x = bounds.getX() + currentPosition * bounds.getWidth();

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

//==============================================================================
void WaveformDisplay::resized()
{
    // Regenerate thumbnail if size changes significantly
    // (Optional: could be optimized to only regenerate when needed)
}

void WaveformDisplay::mouseDown(const juce::MouseEvent& event)
{
    if (fileLoaded)
    {
        handleSeek(event.x);
    }
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& event)
{
    if (fileLoaded)
    {
        handleSeek(event.x);
    }
}

void WaveformDisplay::handleSeek(int x)
{
    auto bounds = getLocalBounds().reduced(2);
    double newPosition = juce::jlimit(0.0, 1.0,
                                     (x - bounds.getX()) / (double)bounds.getWidth());

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

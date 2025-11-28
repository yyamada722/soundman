/*
  ==============================================================================

    SpectrogramDisplay.cpp

    Real-time spectrogram (waterfall) display implementation

  ==============================================================================
*/

#include "SpectrogramDisplay.h"
#include <cmath>

//==============================================================================
SpectrogramDisplay::SpectrogramDisplay()
    : forwardFFT(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    // Initialize FFT buffers
    fifo.resize(fftSize, 0.0f);
    fftData.resize(2 * fftSize, 0.0f);

    // Initialize spectrogram data (historySize time slices x fftSize/2 frequency bins)
    spectrogramData.resize(historySize);
    for (auto& slice : spectrogramData)
        slice.resize(fftSize / 2, minDecibels);

    // Build colormap LUT
    buildColorMapLUT();

    // Start timer for display updates (60 FPS for smooth scrolling)
    startTimerHz(60);
}

SpectrogramDisplay::~SpectrogramDisplay()
{
    stopTimer();
}

//==============================================================================
void SpectrogramDisplay::pushNextSampleIntoFifo(float sample)
{
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[fifoIndex++] = sample;
}

//==============================================================================
void SpectrogramDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    auto bounds = getLocalBounds();

    // Draw border
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRect(bounds, 1);

    // Calculate display areas
    auto displayBounds = bounds.reduced(10);
    auto colorBarBounds = displayBounds.removeFromRight(60);
    displayBounds.removeFromRight(10);  // Gap
    auto freqLabelBounds = displayBounds.removeFromLeft(50);
    auto timeLabelBounds = displayBounds.removeFromBottom(25);

    // Draw spectrogram
    drawSpectrogram(g, displayBounds);

    // Draw labels
    drawFrequencyLabels(g, freqLabelBounds.withBottom(displayBounds.getBottom()).withTop(displayBounds.getY()));
    drawTimeAxis(g, timeLabelBounds);
    drawColorBar(g, colorBarBounds);
}

void SpectrogramDisplay::resized()
{
    imageNeedsUpdate = true;
}

//==============================================================================
void SpectrogramDisplay::timerCallback()
{
    if (nextFFTBlockReady)
    {
        // Apply windowing function
        window.multiplyWithWindowingTable(fftData.data(), fftSize);

        // Perform FFT
        forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

        // Convert to dB and store in current time slice
        auto& currentSlice = spectrogramData[currentTimeIndex];
        for (int i = 0; i < fftSize / 2; ++i)
        {
            float magnitude = fftData[i];
            float db = magnitude > 0.0f
                ? juce::Decibels::gainToDecibels(magnitude / (float)(fftSize / 2))
                : minDecibels;
            currentSlice[i] = juce::jlimit(minDecibels, maxDecibels, db);
        }

        // Move to next time index (circular buffer)
        currentTimeIndex = (currentTimeIndex + 1) % historySize;

        nextFFTBlockReady = false;
        imageNeedsUpdate = true;
        repaint();
    }
}

//==============================================================================
void SpectrogramDisplay::drawSpectrogram(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    if (bounds.isEmpty())
        return;

    // Create or update the spectrogram image
    if (spectrogramImage.isNull() ||
        spectrogramImage.getWidth() != bounds.getWidth() ||
        spectrogramImage.getHeight() != bounds.getHeight() ||
        imageNeedsUpdate)
    {
        updateSpectrogramImage();
        spectrogramImage = juce::Image(juce::Image::RGB, bounds.getWidth(), bounds.getHeight(), true);

        juce::Graphics imgG(spectrogramImage);
        imgG.fillAll(juce::Colour(0xff1e1e1e));

        const int width = bounds.getWidth();
        const int height = bounds.getHeight();

        // Draw spectrogram pixel by pixel
        for (int x = 0; x < width; ++x)
        {
            // Map x to time index
            int timeOffset;
            if (scrollDirection == ScrollDirection::Up || scrollDirection == ScrollDirection::Down)
            {
                // For vertical scroll, x represents frequency
                continue;  // Handle in the y loop
            }
            else
            {
                // For horizontal scroll, x represents time
                float normalizedX = (float)x / (float)width;
                timeOffset = (int)(normalizedX * historySize);
            }
        }

        // Vertical scrolling (frequency on Y, time on X) - most common spectrogram view
        if (scrollDirection == ScrollDirection::Left || scrollDirection == ScrollDirection::Right)
        {
            for (int x = 0; x < width; ++x)
            {
                int timeIdx;
                if (scrollDirection == ScrollDirection::Left)
                {
                    // Newest on right, oldest on left
                    int offset = (int)((float)x / width * historySize);
                    timeIdx = (currentTimeIndex - historySize + offset + historySize) % historySize;
                }
                else
                {
                    // Newest on left, oldest on right
                    int offset = (int)((float)(width - 1 - x) / width * historySize);
                    timeIdx = (currentTimeIndex - historySize + offset + historySize) % historySize;
                }

                const auto& slice = spectrogramData[timeIdx];

                for (int y = 0; y < height; ++y)
                {
                    // Map y to frequency (logarithmic scale)
                    float freq = getFrequencyForY((float)y, (float)height);
                    int binIndex = (int)(freq * fftSize / sampleRate);
                    binIndex = juce::jlimit(0, fftSize / 2 - 1, binIndex);

                    float db = slice[binIndex];
                    float normalized = (db - minDecibels) / (maxDecibels - minDecibels);
                    normalized = juce::jlimit(0.0f, 1.0f, normalized);

                    imgG.setColour(getColorForValue(normalized));
                    imgG.fillRect(x, y, 1, 1);
                }
            }
        }
        else
        {
            // Horizontal scrolling (frequency on X, time on Y)
            for (int y = 0; y < height; ++y)
            {
                int timeIdx;
                if (scrollDirection == ScrollDirection::Up)
                {
                    // Newest at bottom, oldest at top
                    int offset = (int)((float)(height - 1 - y) / height * historySize);
                    timeIdx = (currentTimeIndex - historySize + offset + historySize) % historySize;
                }
                else
                {
                    // Newest at top, oldest at bottom
                    int offset = (int)((float)y / height * historySize);
                    timeIdx = (currentTimeIndex - historySize + offset + historySize) % historySize;
                }

                const auto& slice = spectrogramData[timeIdx];

                for (int x = 0; x < width; ++x)
                {
                    // Map x to frequency (logarithmic scale)
                    float normalizedX = (float)x / (float)width;
                    float logMin = std::log10(minFrequency);
                    float logMax = std::log10(maxFrequency);
                    float freq = std::pow(10.0f, logMin + normalizedX * (logMax - logMin));

                    int binIndex = (int)(freq * fftSize / sampleRate);
                    binIndex = juce::jlimit(0, fftSize / 2 - 1, binIndex);

                    float db = slice[binIndex];
                    float normalized = (db - minDecibels) / (maxDecibels - minDecibels);
                    normalized = juce::jlimit(0.0f, 1.0f, normalized);

                    imgG.setColour(getColorForValue(normalized));
                    imgG.fillRect(x, y, 1, 1);
                }
            }
        }

        imageNeedsUpdate = false;
    }

    g.drawImageAt(spectrogramImage, bounds.getX(), bounds.getY());
}

void SpectrogramDisplay::drawFrequencyLabels(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(10.0f));

    std::vector<std::pair<float, juce::String>> labels = {
        { 50.0f, "50" },
        { 100.0f, "100" },
        { 200.0f, "200" },
        { 500.0f, "500" },
        { 1000.0f, "1k" },
        { 2000.0f, "2k" },
        { 5000.0f, "5k" },
        { 10000.0f, "10k" },
        { 20000.0f, "20k" }
    };

    if (scrollDirection == ScrollDirection::Left || scrollDirection == ScrollDirection::Right)
    {
        // Frequency on Y axis
        for (const auto& [freq, label] : labels)
        {
            if (freq >= minFrequency && freq <= maxFrequency)
            {
                float y = getYForFrequency(freq, (float)bounds.getHeight());
                g.drawText(label,
                          bounds.getX(),
                          bounds.getY() + (int)y - 8,
                          bounds.getWidth() - 5,
                          16,
                          juce::Justification::centredRight);
            }
        }
    }
    else
    {
        // Frequency on X axis (handled in drawTimeAxis)
    }
}

void SpectrogramDisplay::drawTimeAxis(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(10.0f));

    if (scrollDirection == ScrollDirection::Left || scrollDirection == ScrollDirection::Right)
    {
        // Time on X axis
        g.drawText("Time", bounds, juce::Justification::centred);
    }
    else
    {
        // Frequency on X axis
        std::vector<std::pair<float, juce::String>> labels = {
            { 100.0f, "100" },
            { 500.0f, "500" },
            { 1000.0f, "1k" },
            { 5000.0f, "5k" },
            { 10000.0f, "10k" }
        };

        for (const auto& [freq, label] : labels)
        {
            if (freq >= minFrequency && freq <= maxFrequency)
            {
                float logMin = std::log10(minFrequency);
                float logMax = std::log10(maxFrequency);
                float logFreq = std::log10(freq);
                float x = (logFreq - logMin) / (logMax - logMin) * bounds.getWidth();

                g.drawText(label,
                          bounds.getX() + (int)x - 20,
                          bounds.getY(),
                          40, 20,
                          juce::Justification::centred);
            }
        }

        g.drawText("Hz", bounds.getRight() - 30, bounds.getY(), 25, 20,
                  juce::Justification::centredRight);
    }
}

void SpectrogramDisplay::drawColorBar(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Draw color scale bar
    auto barBounds = bounds.reduced(5).withWidth(20);

    for (int y = 0; y < barBounds.getHeight(); ++y)
    {
        float normalized = 1.0f - (float)y / (float)barBounds.getHeight();
        g.setColour(getColorForValue(normalized));
        g.fillRect(barBounds.getX(), barBounds.getY() + y, barBounds.getWidth(), 1);
    }

    // Draw border
    g.setColour(juce::Colours::grey);
    g.drawRect(barBounds, 1);

    // Draw labels
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(10.0f));

    auto labelBounds = bounds.withLeft(barBounds.getRight() + 5);

    g.drawText(juce::String((int)maxDecibels) + " dB",
              labelBounds.getX(), barBounds.getY() - 5,
              labelBounds.getWidth(), 14,
              juce::Justification::centredLeft);

    g.drawText(juce::String((int)((maxDecibels + minDecibels) / 2)) + " dB",
              labelBounds.getX(), barBounds.getCentreY() - 7,
              labelBounds.getWidth(), 14,
              juce::Justification::centredLeft);

    g.drawText(juce::String((int)minDecibels) + " dB",
              labelBounds.getX(), barBounds.getBottom() - 9,
              labelBounds.getWidth(), 14,
              juce::Justification::centredLeft);
}

//==============================================================================
juce::Colour SpectrogramDisplay::getColorForValue(float normalizedValue) const
{
    int index = juce::jlimit(0, 255, (int)(normalizedValue * 255.0f));
    return colorMapLUT[index];
}

float SpectrogramDisplay::getYForFrequency(float freq, float height) const
{
    // Logarithmic frequency scale (inverted - low freq at bottom)
    float logMin = std::log10(minFrequency);
    float logMax = std::log10(maxFrequency);
    float logFreq = std::log10(juce::jlimit(minFrequency, maxFrequency, freq));
    float normalized = (logFreq - logMin) / (logMax - logMin);
    return (1.0f - normalized) * height;  // Inverted for display
}

float SpectrogramDisplay::getFrequencyForY(float y, float height) const
{
    // Logarithmic frequency scale (inverted)
    float normalized = 1.0f - y / height;
    float logMin = std::log10(minFrequency);
    float logMax = std::log10(maxFrequency);
    return std::pow(10.0f, logMin + normalized * (logMax - logMin));
}

void SpectrogramDisplay::updateSpectrogramImage()
{
    // Mark that image needs to be redrawn
    imageNeedsUpdate = true;
}

//==============================================================================
void SpectrogramDisplay::setColorMap(ColorMap map)
{
    currentColorMap = map;
    buildColorMapLUT();
    imageNeedsUpdate = true;
    repaint();
}

juce::StringArray SpectrogramDisplay::getColorMapNames()
{
    return { "Viridis", "Plasma", "Inferno", "Magma", "Grayscale", "Jet", "Hot" };
}

void SpectrogramDisplay::buildColorMapLUT()
{
    colorMapLUT.resize(256);

    for (int i = 0; i < 256; ++i)
    {
        float t = (float)i / 255.0f;

        switch (currentColorMap)
        {
            case ColorMap::Viridis:
            {
                // Viridis colormap (perceptually uniform)
                float r = 0.267004f + t * (0.282327f + t * (-0.827603f + t * (2.453482f + t * (-2.139177f + t * 0.670997f))));
                float g = 0.004874f + t * (1.015861f + t * (0.284654f + t * (-1.764195f + t * (1.628440f - t * 0.453376f))));
                float b = 0.329415f + t * (1.242892f + t * (-2.699941f + t * (4.048237f + t * (-2.738317f + t * 0.741292f))));
                colorMapLUT[i] = juce::Colour::fromFloatRGBA(
                    juce::jlimit(0.0f, 1.0f, r),
                    juce::jlimit(0.0f, 1.0f, g),
                    juce::jlimit(0.0f, 1.0f, b),
                    1.0f);
                break;
            }

            case ColorMap::Plasma:
            {
                // Plasma colormap
                float r = 0.050383f + t * (2.021066f + t * (-1.313261f + t * (-0.797801f + t * 1.039830f)));
                float g = 0.029803f + t * (-0.563236f + t * (2.912177f + t * (-3.221587f + t * 1.561088f)));
                float b = 0.527975f + t * (1.622430f + t * (-4.864137f + t * (5.488868f + t * (-2.523960f))));
                colorMapLUT[i] = juce::Colour::fromFloatRGBA(
                    juce::jlimit(0.0f, 1.0f, r),
                    juce::jlimit(0.0f, 1.0f, g),
                    juce::jlimit(0.0f, 1.0f, b),
                    1.0f);
                break;
            }

            case ColorMap::Inferno:
            {
                // Inferno colormap
                float r = 0.001462f + t * (0.634065f + t * (2.438963f + t * (-4.812899f + t * (2.931619f))));
                float g = 0.000466f + t * (-0.227256f + t * (1.813934f + t * (-1.653152f + t * 0.703621f)));
                float b = 0.013866f + t * (1.932624f + t * (-4.649717f + t * (4.773017f + t * (-1.873465f))));
                colorMapLUT[i] = juce::Colour::fromFloatRGBA(
                    juce::jlimit(0.0f, 1.0f, r),
                    juce::jlimit(0.0f, 1.0f, g),
                    juce::jlimit(0.0f, 1.0f, b),
                    1.0f);
                break;
            }

            case ColorMap::Magma:
            {
                // Magma colormap
                float r = 0.001462f + t * (0.506116f + t * (2.625049f + t * (-3.938616f + t * 1.954629f)));
                float g = 0.000466f + t * (-0.171817f + t * (0.866683f + t * (0.400410f + t * -0.261296f)));
                float b = 0.013866f + t * (1.981117f + t * (-4.024510f + t * (3.292621f + t * -0.970954f)));
                colorMapLUT[i] = juce::Colour::fromFloatRGBA(
                    juce::jlimit(0.0f, 1.0f, r),
                    juce::jlimit(0.0f, 1.0f, g),
                    juce::jlimit(0.0f, 1.0f, b),
                    1.0f);
                break;
            }

            case ColorMap::Grayscale:
            {
                colorMapLUT[i] = juce::Colour::fromFloatRGBA(t, t, t, 1.0f);
                break;
            }

            case ColorMap::Jet:
            {
                // Jet colormap (classic but not perceptually uniform)
                float r, g, b;
                if (t < 0.125f) {
                    r = 0.0f; g = 0.0f; b = 0.5f + t * 4.0f;
                } else if (t < 0.375f) {
                    r = 0.0f; g = (t - 0.125f) * 4.0f; b = 1.0f;
                } else if (t < 0.625f) {
                    r = (t - 0.375f) * 4.0f; g = 1.0f; b = 1.0f - (t - 0.375f) * 4.0f;
                } else if (t < 0.875f) {
                    r = 1.0f; g = 1.0f - (t - 0.625f) * 4.0f; b = 0.0f;
                } else {
                    r = 1.0f - (t - 0.875f) * 4.0f; g = 0.0f; b = 0.0f;
                }
                colorMapLUT[i] = juce::Colour::fromFloatRGBA(
                    juce::jlimit(0.0f, 1.0f, r),
                    juce::jlimit(0.0f, 1.0f, g),
                    juce::jlimit(0.0f, 1.0f, b),
                    1.0f);
                break;
            }

            case ColorMap::Hot:
            {
                // Hot colormap (black -> red -> yellow -> white)
                float r = juce::jlimit(0.0f, 1.0f, t * 3.0f);
                float g = juce::jlimit(0.0f, 1.0f, (t - 0.333f) * 3.0f);
                float b = juce::jlimit(0.0f, 1.0f, (t - 0.666f) * 3.0f);
                colorMapLUT[i] = juce::Colour::fromFloatRGBA(r, g, b, 1.0f);
                break;
            }
        }
    }
}

//==============================================================================
void SpectrogramDisplay::clear()
{
    for (auto& slice : spectrogramData)
        std::fill(slice.begin(), slice.end(), minDecibels);

    currentTimeIndex = 0;
    imageNeedsUpdate = true;
    repaint();
}

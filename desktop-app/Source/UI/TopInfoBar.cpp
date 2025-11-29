/*
  ==============================================================================

    TopInfoBar.cpp

    Professional DAW-style top information bar implementation
    Layout: [File Info] [Transport] [LCD Timecode] [Duration/BPM] [Meters] [Device]

  ==============================================================================
*/

#include "TopInfoBar.h"

//==============================================================================
TopInfoBar::TopInfoBar()
{
    createTransportButtons();
    createModeButtons();

    // Open file button
    openFileButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    openFileButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    openFileButton.onClick = [this]() { if (onOpenFile) onOpenFile(); };
    addAndMakeVisible(openFileButton);

    // Settings button
    settingsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    settingsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    settingsButton.onClick = [this]() { if (onSettings) onSettings(); };
    addAndMakeVisible(settingsButton);

    startTimerHz(30);
}

TopInfoBar::~TopInfoBar()
{
    stopTimer();
}

//==============================================================================
void TopInfoBar::createTransportButtons()
{
    // Skip to start button (|<)
    {
        auto icon = std::make_unique<juce::DrawablePath>();
        juce::Path p;
        p.addRectangle(0.0f, 2.0f, 2.0f, 12.0f);
        p.addTriangle(4.0f, 8.0f, 14.0f, 2.0f, 14.0f, 14.0f);
        icon->setPath(p);
        icon->setFill(juce::Colours::white);
        skipToStartButton.setImages(icon.get());
        skipToStartButton.onClick = [this]() { if (onSkipToStart) onSkipToStart(); };
        addAndMakeVisible(skipToStartButton);
    }

    // Rewind button (<<)
    {
        auto icon = std::make_unique<juce::DrawablePath>();
        juce::Path p;
        p.addTriangle(0.0f, 8.0f, 8.0f, 2.0f, 8.0f, 14.0f);
        p.addTriangle(6.0f, 8.0f, 14.0f, 2.0f, 14.0f, 14.0f);
        icon->setPath(p);
        icon->setFill(juce::Colours::white);
        rewindButton.setImages(icon.get());
        rewindButton.onClick = [this]() {
            if (onSeek) onSeek(juce::jmax(0.0, position - 5.0));
        };
        addAndMakeVisible(rewindButton);
    }

    // Stop button (square)
    {
        auto icon = std::make_unique<juce::DrawablePath>();
        juce::Path p;
        p.addRectangle(2.0f, 2.0f, 12.0f, 12.0f);
        icon->setPath(p);
        icon->setFill(juce::Colours::white);
        stopButton.setImages(icon.get());
        stopButton.onClick = [this]() { if (onStop) onStop(); };
        addAndMakeVisible(stopButton);
    }

    // Play button (triangle) - will toggle to pause
    {
        // Play icon
        playIcon = std::make_unique<juce::DrawablePath>();
        juce::Path playPath;
        playPath.addTriangle(2.0f, 0.0f, 14.0f, 8.0f, 2.0f, 16.0f);
        static_cast<juce::DrawablePath*>(playIcon.get())->setPath(playPath);
        static_cast<juce::DrawablePath*>(playIcon.get())->setFill(juce::Colours::white);

        // Pause icon
        pauseIcon = std::make_unique<juce::DrawablePath>();
        juce::Path pausePath;
        pausePath.addRectangle(2.0f, 0.0f, 4.0f, 16.0f);
        pausePath.addRectangle(10.0f, 0.0f, 4.0f, 16.0f);
        static_cast<juce::DrawablePath*>(pauseIcon.get())->setPath(pausePath);
        static_cast<juce::DrawablePath*>(pauseIcon.get())->setFill(juce::Colours::white);

        playButton.setImages(playIcon.get());
        playButton.onClick = [this]() {
            if (playing) {
                if (onPause) onPause();
            } else {
                if (onPlay) onPlay();
            }
        };
        addAndMakeVisible(playButton);
    }

    // Forward button (>>)
    {
        auto icon = std::make_unique<juce::DrawablePath>();
        juce::Path p;
        p.addTriangle(0.0f, 2.0f, 8.0f, 8.0f, 0.0f, 14.0f);
        p.addTriangle(6.0f, 2.0f, 14.0f, 8.0f, 6.0f, 14.0f);
        icon->setPath(p);
        icon->setFill(juce::Colours::white);
        forwardButton.setImages(icon.get());
        forwardButton.onClick = [this]() {
            if (onSeek) onSeek(juce::jmin(duration, position + 5.0));
        };
        addAndMakeVisible(forwardButton);
    }

    // Skip to end button (>|)
    {
        auto icon = std::make_unique<juce::DrawablePath>();
        juce::Path p;
        p.addTriangle(0.0f, 2.0f, 10.0f, 8.0f, 0.0f, 14.0f);
        p.addRectangle(12.0f, 2.0f, 2.0f, 12.0f);
        icon->setPath(p);
        icon->setFill(juce::Colours::white);
        skipToEndButton.setImages(icon.get());
        skipToEndButton.onClick = [this]() { if (onSkipToEnd) onSkipToEnd(); };
        addAndMakeVisible(skipToEndButton);
    }

    // Record button (circle)
    {
        auto icon = std::make_unique<juce::DrawablePath>();
        juce::Path p;
        p.addEllipse(2.0f, 2.0f, 12.0f, 12.0f);
        icon->setPath(p);
        icon->setFill(juce::Colours::red.withAlpha(0.8f));
        recordButton.setImages(icon.get());
        recordButton.onClick = [this]() { if (onRecord) onRecord(); };
        addAndMakeVisible(recordButton);
    }

    // Loop button
    {
        auto icon = std::make_unique<juce::DrawablePath>();
        juce::Path p;
        // Loop arrows
        p.addArrow(juce::Line<float>(12.0f, 4.0f, 4.0f, 4.0f), 2.0f, 6.0f, 4.0f);
        p.addArrow(juce::Line<float>(4.0f, 12.0f, 12.0f, 12.0f), 2.0f, 6.0f, 4.0f);
        icon->setPath(p);
        icon->setFill(juce::Colours::white);
        loopButton.setImages(icon.get());
        loopButton.onClick = [this]() { if (onToggleLoop) onToggleLoop(); };
        addAndMakeVisible(loopButton);
    }
}

void TopInfoBar::updatePlayButtonIcon()
{
    if (playing)
        playButton.setImages(pauseIcon.get());
    else
        playButton.setImages(playIcon.get());
}

//==============================================================================
void TopInfoBar::createModeButtons()
{
    auto setupModeButton = [this](juce::TextButton& button, PlaybackMode mode, const juce::String& tooltip)
    {
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff353535));
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff0077dd));
        button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffaaaaaa));
        button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        button.setClickingTogglesState(false);  // We'll manage state manually
        button.setTooltip(tooltip);
        button.onClick = [this, mode]()
        {
            setPlaybackMode(mode);
            if (onPlaybackModeChanged)
                onPlaybackModeChanged(mode);
        };
        addAndMakeVisible(button);
    };

    setupModeButton(singleFileModeButton, PlaybackMode::SingleFile, "Single File Playback - Play loaded audio file");
    setupModeButton(multiTrackModeButton, PlaybackMode::MultiTrack, "Multi-Track Project - DAW-style multi-track playback");
    setupModeButton(abCompareModeButton, PlaybackMode::ABCompare, "A/B Comparison - Compare two audio tracks");

    updateModeButtonStates();
}

void TopInfoBar::updateModeButtonStates()
{
    // Update button appearances based on current mode
    auto setButtonActive = [](juce::TextButton& button, bool active)
    {
        if (active)
        {
            button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0077dd));
            button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        }
        else
        {
            button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff353535));
            button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffaaaaaa));
        }
        button.repaint();
    };

    setButtonActive(singleFileModeButton, playbackMode == PlaybackMode::SingleFile);
    setButtonActive(multiTrackModeButton, playbackMode == PlaybackMode::MultiTrack);
    setButtonActive(abCompareModeButton, playbackMode == PlaybackMode::ABCompare);

    repaint();
}

void TopInfoBar::setPlaybackMode(PlaybackMode mode)
{
    if (playbackMode != mode)
    {
        playbackMode = mode;
        updateModeButtonStates();
    }
}

//==============================================================================
juce::Font TopInfoBar::getJapaneseFont(float height, int style) const
{
    if (cachedFontName.isEmpty())
    {
        juce::StringArray fontNames = juce::Font::findAllTypefaceNames();
        const char* japaneseFonts[] = {
            "Meiryo UI", "Meiryo", "Yu Gothic UI", "Yu Gothic",
            "MS UI Gothic", "MS Gothic", "MS PGothic", nullptr
        };

        for (int i = 0; japaneseFonts[i] != nullptr; ++i)
        {
            if (fontNames.contains(japaneseFonts[i]))
            {
                cachedFontName = japaneseFonts[i];
                break;
            }
        }

        if (cachedFontName.isEmpty())
            cachedFontName = juce::Font::getDefaultSansSerifFontName();
    }

    return juce::Font(cachedFontName, height, style);
}

juce::String TopInfoBar::formatTimecode(double seconds) const
{
    if (seconds < 0) seconds = 0;

    int hours = static_cast<int>(seconds / 3600);
    int mins = static_cast<int>((seconds - hours * 3600) / 60);
    int secs = static_cast<int>(seconds) % 60;
    int ms = static_cast<int>((seconds - static_cast<int>(seconds)) * 1000);

    if (hours > 0)
        return juce::String::formatted("%d:%02d:%02d.%03d", hours, mins, secs, ms);
    else
        return juce::String::formatted("%02d:%02d.%03d", mins, secs, ms);
}

juce::String TopInfoBar::formatTimecodeCompact(double seconds) const
{
    if (seconds < 0) seconds = 0;

    int mins = static_cast<int>(seconds / 60);
    int secs = static_cast<int>(seconds) % 60;

    return juce::String::formatted("%d:%02d", mins, secs);
}

//==============================================================================
void TopInfoBar::setFileInfo(const juce::File& file, juce::AudioFormatReader* reader)
{
    fileName = file.getFileName();
    filePath = file.getFullPathName();

    if (reader != nullptr)
    {
        fileFormat = reader->getFormatName();
        sampleRate = reader->sampleRate;
        numChannels = static_cast<int>(reader->numChannels);
        bitsPerSample = static_cast<int>(reader->bitsPerSample);
        duration = reader->lengthInSamples / reader->sampleRate;
    }

    repaint();
}

void TopInfoBar::clearFileInfo()
{
    fileName = "";
    filePath = "";
    fileFormat = "";
    sampleRate = 0.0;
    numChannels = 0;
    bitsPerSample = 0;
    duration = 0.0;
    position = 0.0;
    bpm = 0.0;
    musicalKey = "";
    repaint();
}

//==============================================================================
void TopInfoBar::setPlaying(bool isPlaying)
{
    if (playing != isPlaying)
    {
        playing = isPlaying;
        updatePlayButtonIcon();
        repaint();
    }
}

void TopInfoBar::setRecording(bool isRecording)
{
    recording = isRecording;
    repaint();
}

void TopInfoBar::setLoopEnabled(bool enabled)
{
    loopEnabled = enabled;
    repaint();
}

void TopInfoBar::setPosition(double positionSeconds)
{
    position = positionSeconds;
}

void TopInfoBar::setDuration(double durationSeconds)
{
    duration = durationSeconds;
}

void TopInfoBar::setSampleRate(double rate)
{
    sampleRate = rate;
}

void TopInfoBar::setLoopRange(double startSeconds, double endSeconds)
{
    loopStart = startSeconds;
    loopEnd = endSeconds;
    repaint();
}

void TopInfoBar::setLevels(float lRMS, float lPeak, float rRMS, float rPeak)
{
    leftRMS = lRMS;
    leftPeak = lPeak;
    rightRMS = rRMS;
    rightPeak = rPeak;
}

void TopInfoBar::setBPM(double newBpm)
{
    bpm = newBpm;
}

void TopInfoBar::setKey(const juce::String& key)
{
    musicalKey = key;
}

void TopInfoBar::setDeviceName(const juce::String& name)
{
    deviceName = name;
}

void TopInfoBar::setBufferSize(int size)
{
    bufferSize = size;
}

//==============================================================================
void TopInfoBar::drawSection(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& /*title*/)
{
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);
}

void TopInfoBar::drawLCDBackground(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Dark LCD background
    g.setColour(juce::Colour(0xff0a0f0a));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Subtle inner glow
    g.setColour(juce::Colour(0xff1a1f1a));
    g.drawRoundedRectangle(bounds.reduced(1).toFloat(), 3.0f, 1.0f);

    // Border
    g.setColour(juce::Colour(0xff2a2f2a));
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);
}

//==============================================================================
void TopInfoBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    int height = bounds.getHeight();

    // Main background gradient
    juce::ColourGradient gradient(
        juce::Colour(0xff2d2d2d), 0.0f, 0.0f,
        juce::Colour(0xff1a1a1a), 0.0f, static_cast<float>(height),
        false
    );
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    // Top highlight line
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(bounds.getWidth()));

    // Bottom shadow line
    g.setColour(juce::Colour(0xff0a0a0a));
    g.drawHorizontalLine(height - 1, 0.0f, static_cast<float>(bounds.getWidth()));

    int margin = 8;
    int sectionY = margin;
    int sectionHeight = height - margin * 2;

    // ========================================
    // LEFT SECTION: Logo + File Info
    // ========================================
    int fileInfoX = 80;  // After Open/Settings buttons
    int fileInfoWidth = 220;
    auto fileInfoBounds = juce::Rectangle<int>(fileInfoX, sectionY, fileInfoWidth, sectionHeight);
    drawSection(g, fileInfoBounds, "FILE");

    // File name (truncated if too long)
    g.setFont(getJapaneseFont(12.0f, juce::Font::bold));
    g.setColour(juce::Colours::white);
    juce::String displayName = fileName.isEmpty() ? "No File" : fileName;
    if (displayName.length() > 28)
        displayName = displayName.substring(0, 25) + "...";
    g.drawText(displayName, fileInfoBounds.reduced(8, 4).removeFromTop(16),
               juce::Justification::centredLeft, true);

    // Format info
    if (!fileName.isEmpty())
    {
        g.setFont(getJapaneseFont(10.0f));
        g.setColour(juce::Colour(0xff888888));

        juce::String infoLine;
        if (sampleRate > 0)
            infoLine = juce::String(sampleRate / 1000.0, 1) + "kHz";
        if (bitsPerSample > 0)
            infoLine += " " + juce::String(bitsPerSample) + "bit";
        if (numChannels > 0)
            infoLine += " " + (numChannels == 1 ? "Mono" : (numChannels == 2 ? "Stereo" : juce::String(numChannels) + "ch"));

        g.drawText(infoLine, fileInfoBounds.reduced(8, 4).removeFromBottom(14),
                   juce::Justification::centredLeft, true);
    }

    // ========================================
    // CENTER: Transport buttons + Mode buttons are placed in resized()
    // ========================================

    // ========================================
    // CENTER-RIGHT: LCD Time Display (after mode buttons)
    // ========================================
    // Transport: 8 buttons at (28+4) = 256, Mode: 3 buttons at (52+2) = 162
    int transportEndX = fileInfoX + fileInfoWidth + 10 + (28 + 4) * 8 + 16 + (52 + 2) * 3 + 20;
    int lcdWidth = 160;
    auto lcdBounds = juce::Rectangle<int>(transportEndX, sectionY, lcdWidth, sectionHeight);
    drawLCDBackground(g, lcdBounds);

    // Current position (large green digits)
    g.setFont(getJapaneseFont(28.0f, juce::Font::bold));
    juce::Colour timeColor = playing ? juce::Colour(0xff00ff00) : juce::Colour(0xff00cc00);
    if (recording)
        timeColor = juce::Colours::red;
    g.setColour(timeColor);
    g.drawText(formatTimecode(position), lcdBounds.reduced(8, 2),
               juce::Justification::centred, false);

    // ========================================
    // Duration / Remaining
    // ========================================
    int durationX = transportEndX + lcdWidth + 10;
    int durationWidth = 100;
    auto durationBounds = juce::Rectangle<int>(durationX, sectionY, durationWidth, sectionHeight);
    drawSection(g, durationBounds, "");

    g.setFont(getJapaneseFont(9.0f));
    g.setColour(juce::Colour(0xff666666));
    g.drawText("DURATION", durationBounds.reduced(6, 2).removeFromTop(12),
               juce::Justification::centredLeft, false);

    g.setFont(getJapaneseFont(13.0f));
    g.setColour(juce::Colour(0xffaaaaaa));
    g.drawText(formatTimecode(duration), durationBounds.reduced(6, 2).removeFromBottom(18),
               juce::Justification::centredLeft, false);

    // ========================================
    // BPM / Key (if detected)
    // ========================================
    int bpmX = durationX + durationWidth + 10;
    int bpmWidth = 90;
    auto bpmBounds = juce::Rectangle<int>(bpmX, sectionY, bpmWidth, sectionHeight);
    drawSection(g, bpmBounds, "");

    g.setFont(getJapaneseFont(9.0f));
    g.setColour(juce::Colour(0xff666666));
    g.drawText("BPM", bpmBounds.reduced(6, 2).removeFromTop(12),
               juce::Justification::centredLeft, false);

    g.setFont(getJapaneseFont(14.0f, juce::Font::bold));
    if (bpm > 0)
    {
        g.setColour(juce::Colour(0xffffaa00));
        g.drawText(juce::String(bpm, 1), bpmBounds.reduced(6, 0),
                   juce::Justification::centred, false);
    }
    else
    {
        g.setColour(juce::Colour(0xff555555));
        g.drawText("---", bpmBounds.reduced(6, 0),
                   juce::Justification::centred, false);
    }

    // Key display
    if (!musicalKey.isEmpty())
    {
        g.setFont(getJapaneseFont(10.0f));
        g.setColour(juce::Colour(0xff00aaff));
        g.drawText(musicalKey, bpmBounds.reduced(6, 2).removeFromBottom(14),
                   juce::Justification::centredLeft, false);
    }

    // ========================================
    // Mini Level Meters
    // ========================================
    int metersX = bpmX + bpmWidth + 10;
    int metersWidth = 60;
    auto metersBounds = juce::Rectangle<int>(metersX, sectionY, metersWidth, sectionHeight);
    drawSection(g, metersBounds, "");

    // Draw mini meters
    int meterHeight = sectionHeight - 8;
    int meterWidth = 8;
    int meterY = sectionY + 4;

    // Left channel
    int leftMeterX = metersX + 12;
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(leftMeterX, meterY, meterWidth, meterHeight);

    float leftLevel = juce::jlimit(0.0f, 1.0f, leftPeak);
    int leftFillHeight = static_cast<int>(leftLevel * meterHeight);
    if (leftFillHeight > 0)
    {
        juce::Colour meterColor = leftLevel > 0.9f ? juce::Colours::red :
                                  leftLevel > 0.7f ? juce::Colours::yellow :
                                  juce::Colours::green;
        g.setColour(meterColor);
        g.fillRect(leftMeterX, meterY + meterHeight - leftFillHeight, meterWidth, leftFillHeight);
    }

    // Right channel
    int rightMeterX = leftMeterX + meterWidth + 4;
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(rightMeterX, meterY, meterWidth, meterHeight);

    float rightLevel = juce::jlimit(0.0f, 1.0f, rightPeak);
    int rightFillHeight = static_cast<int>(rightLevel * meterHeight);
    if (rightFillHeight > 0)
    {
        juce::Colour meterColor = rightLevel > 0.9f ? juce::Colours::red :
                                  rightLevel > 0.7f ? juce::Colours::yellow :
                                  juce::Colours::green;
        g.setColour(meterColor);
        g.fillRect(rightMeterX, meterY + meterHeight - rightFillHeight, meterWidth, rightFillHeight);
    }

    // L/R labels
    g.setFont(getJapaneseFont(8.0f));
    g.setColour(juce::Colour(0xff666666));
    g.drawText("L", leftMeterX, meterY + meterHeight + 1, meterWidth, 10, juce::Justification::centred, false);
    g.drawText("R", rightMeterX, meterY + meterHeight + 1, meterWidth, 10, juce::Justification::centred, false);

    // ========================================
    // RIGHT SECTION: Device Info
    // ========================================
    int deviceX = bounds.getWidth() - 160;
    int deviceWidth = 150;
    auto deviceBounds = juce::Rectangle<int>(deviceX, sectionY, deviceWidth, sectionHeight);
    drawSection(g, deviceBounds, "");

    g.setFont(getJapaneseFont(9.0f));
    g.setColour(juce::Colour(0xff666666));
    g.drawText("DEVICE", deviceBounds.reduced(6, 2).removeFromTop(12),
               juce::Justification::centredLeft, false);

    g.setFont(getJapaneseFont(10.0f));
    g.setColour(juce::Colour(0xff888888));
    juce::String deviceDisplay = deviceName.isEmpty() ? "No Device" : deviceName;
    if (deviceDisplay.length() > 18)
        deviceDisplay = deviceDisplay.substring(0, 15) + "...";
    g.drawText(deviceDisplay, deviceBounds.reduced(6, 2).translated(0, 10).removeFromTop(14),
               juce::Justification::centredLeft, true);

    // Buffer size
    if (bufferSize > 0)
    {
        g.setColour(juce::Colour(0xff666666));
        g.drawText(juce::String(bufferSize) + " samples", deviceBounds.reduced(6, 2).removeFromBottom(12),
                   juce::Justification::centredLeft, false);
    }

    // ========================================
    // MODE SELECTOR Section (after transport buttons)
    // ========================================
    // Calculate position after transport controls: after loop button + spacing
    int modeSectionX = fileInfoX + fileInfoWidth + 10 + (28 + 4) * 8 + 16;

    // Vertical separator line before mode buttons
    g.setColour(juce::Colour(0xff444444));
    g.drawLine(static_cast<float>(modeSectionX - 8), static_cast<float>(sectionY + 4),
               static_cast<float>(modeSectionX - 8), static_cast<float>(sectionY + sectionHeight - 4), 1.0f);

    // "SOURCE" label above buttons
    g.setFont(getJapaneseFont(8.0f));
    g.setColour(juce::Colour(0xff666666));
    g.drawText("SOURCE", modeSectionX, sectionY, 160, 10, juce::Justification::centredLeft, false);

    // ========================================
    // Loop indicator
    // ========================================
    if (loopEnabled)
    {
        int loopIndicatorX = deviceX - 60;
        g.setColour(juce::Colour(0xff00aa00));
        g.setFont(getJapaneseFont(10.0f, juce::Font::bold));
        g.drawText("LOOP", loopIndicatorX, sectionY + 4, 40, 14, juce::Justification::centred, false);

        g.setFont(getJapaneseFont(8.0f));
        g.setColour(juce::Colour(0xff00cc00));
        juce::String loopRange = formatTimecodeCompact(loopStart) + "-" + formatTimecodeCompact(loopEnd);
        g.drawText(loopRange, loopIndicatorX - 10, sectionY + 18, 60, 12, juce::Justification::centred, false);
    }

    // ========================================
    // Status indicators (playing/recording)
    // ========================================
    if (playing)
    {
        int statusX = bounds.getWidth() - 30;
        g.setColour(juce::Colour(0xff00ff00));
        g.fillEllipse(static_cast<float>(statusX), static_cast<float>(sectionY + 8), 8.0f, 8.0f);
    }

    if (recording)
    {
        int statusX = bounds.getWidth() - 30;
        g.setColour(juce::Colours::red);
        g.fillEllipse(static_cast<float>(statusX), static_cast<float>(sectionY + 22), 8.0f, 8.0f);
    }
}

void TopInfoBar::resized()
{
    auto bounds = getLocalBounds();
    int margin = 8;
    int sectionY = margin;
    int sectionHeight = bounds.getHeight() - margin * 2;

    // Utility buttons on the left
    int buttonHeight = 24;
    int buttonY = (bounds.getHeight() - buttonHeight) / 2;

    openFileButton.setBounds(8, buttonY, 32, buttonHeight);
    settingsButton.setBounds(44, buttonY, 32, buttonHeight);

    // Transport buttons
    int buttonSize = 28;
    int transportX = 80 + 220 + 10;  // After file info section
    int transportY = (bounds.getHeight() - buttonSize) / 2;

    skipToStartButton.setBounds(transportX, transportY, buttonSize, buttonSize);
    transportX += buttonSize + 4;

    rewindButton.setBounds(transportX, transportY, buttonSize, buttonSize);
    transportX += buttonSize + 4;

    stopButton.setBounds(transportX, transportY, buttonSize, buttonSize);
    transportX += buttonSize + 4;

    playButton.setBounds(transportX, transportY, buttonSize + 4, buttonSize);  // Slightly larger
    transportX += buttonSize + 8;

    forwardButton.setBounds(transportX, transportY, buttonSize, buttonSize);
    transportX += buttonSize + 4;

    skipToEndButton.setBounds(transportX, transportY, buttonSize, buttonSize);
    transportX += buttonSize + 8;

    recordButton.setBounds(transportX, transportY, buttonSize, buttonSize);
    transportX += buttonSize + 8;

    loopButton.setBounds(transportX, transportY, buttonSize, buttonSize);
    transportX += buttonSize + 16;  // Extra space before mode selector

    // Mode selector buttons (right after transport, prominent position)
    int modeButtonWidth = 52;
    int modeButtonHeight = 28;
    int modeButtonY = (bounds.getHeight() - modeButtonHeight) / 2;

    singleFileModeButton.setBounds(transportX, modeButtonY, modeButtonWidth, modeButtonHeight);
    transportX += modeButtonWidth + 2;

    multiTrackModeButton.setBounds(transportX, modeButtonY, modeButtonWidth, modeButtonHeight);
    transportX += modeButtonWidth + 2;

    abCompareModeButton.setBounds(transportX, modeButtonY, modeButtonWidth, modeButtonHeight);
}

void TopInfoBar::timerCallback()
{
    repaint();
}

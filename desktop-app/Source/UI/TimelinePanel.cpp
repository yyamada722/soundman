/*
  ==============================================================================

    TimelinePanel.cpp

    Multi-track timeline UI implementation

  ==============================================================================
*/

#include "TimelinePanel.h"

//==============================================================================
// TrackHeaderComponent Implementation
//==============================================================================

TrackHeaderComponent::TrackHeaderComponent(ProjectManager& pm, const juce::ValueTree& trackState)
    : projectManager(pm)
    , state(trackState)
{
    setupComponents();
    updateFromState();
}

void TrackHeaderComponent::setupComponents()
{
    // Name label
    nameLabel.setEditable(true);
    nameLabel.setJustificationType(juce::Justification::centredLeft);
    nameLabel.onTextChange = [this]() {
        projectManager.setTrackName(state, nameLabel.getText());
    };
    addAndMakeVisible(nameLabel);

    // Mute button
    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    muteButton.onClick = [this]() {
        projectManager.setTrackMute(state, muteButton.getToggleState());
    };
    addAndMakeVisible(muteButton);

    // Solo button
    soloButton.setClickingTogglesState(true);
    soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow);
    soloButton.onClick = [this]() {
        projectManager.setTrackSolo(state, soloButton.getToggleState());
    };
    addAndMakeVisible(soloButton);

    // Arm button
    armButton.setClickingTogglesState(true);
    armButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
    armButton.onClick = [this]() {
        state.setProperty(IDs::armed, armButton.getToggleState(), nullptr);
    };
    addAndMakeVisible(armButton);

    // Volume slider
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    volumeSlider.setRange(0.0, 2.0, 0.01);
    volumeSlider.setValue(1.0);
    volumeSlider.onValueChange = [this]() {
        projectManager.setTrackVolume(state, static_cast<float>(volumeSlider.getValue()));
    };
    addAndMakeVisible(volumeSlider);
}

void TrackHeaderComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background with track color
    g.setColour(trackColor.darker(0.3f));
    g.fillRect(bounds);

    // Left color strip
    g.setColour(trackColor);
    g.fillRect(bounds.removeFromLeft(4));

    // Border
    g.setColour(juce::Colours::grey.darker());
    g.drawRect(getLocalBounds());
}

void TrackHeaderComponent::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    bounds.removeFromLeft(4);  // Space for color strip

    // Top row: name
    auto topRow = bounds.removeFromTop(20);
    nameLabel.setBounds(topRow);

    bounds.removeFromTop(4);

    // Middle row: M S R buttons
    auto buttonRow = bounds.removeFromTop(22);
    int buttonWidth = 24;
    muteButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(2);
    soloButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(2);
    armButton.setBounds(buttonRow.removeFromLeft(buttonWidth));

    bounds.removeFromTop(4);

    // Bottom row: volume slider
    volumeSlider.setBounds(bounds.removeFromTop(20));
}

void TrackHeaderComponent::updateFromState()
{
    TrackModel track(state);

    nameLabel.setText(track.getName(), juce::dontSendNotification);
    trackColor = track.getColor();
    muteButton.setToggleState(track.isMuted(), juce::dontSendNotification);
    soloButton.setToggleState(track.isSoloed(), juce::dontSendNotification);
    armButton.setToggleState(track.isArmed(), juce::dontSendNotification);
    volumeSlider.setValue(track.getVolume(), juce::dontSendNotification);

    repaint();
}

juce::String TrackHeaderComponent::getTrackId() const
{
    return state[IDs::trackId].toString();
}

//==============================================================================
// ClipComponent Implementation
//==============================================================================

ClipComponent::ClipComponent(ProjectManager& pm,
                             const juce::ValueTree& clipState,
                             juce::AudioThumbnailCache& thumbnailCache,
                             juce::AudioFormatManager& formatManager)
    : projectManager(pm)
    , state(clipState)
    , thumbnail(512, formatManager, thumbnailCache)
{
    updateFromState();
    loadThumbnail();
}

void ClipComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(clipColor.darker(0.2f));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Waveform
    if (thumbnail.getTotalLength() > 0)
    {
        auto waveformBounds = bounds.reduced(2, 16);
        g.setColour(clipColor.brighter(0.3f));

        double startTime = static_cast<double>(state[IDs::sourceStart]) / thumbnail.getNumSamplesFinished() * thumbnail.getTotalLength();
        double endTime = startTime + (static_cast<double>(length) / thumbnail.getNumSamplesFinished() * thumbnail.getTotalLength());

        thumbnail.drawChannels(g, waveformBounds.toNearestInt(),
            startTime, endTime, 1.0f);
    }

    // Clip name
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawText(clipName, bounds.reduced(4, 2).removeFromTop(14),
        juce::Justification::centredLeft, true);

    // Selection highlight
    if (selected)
    {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.reduced(1), 4.0f, 2.0f);
    }

    // Trim handles (visible when selected)
    if (selected)
    {
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.fillRect(0, 0, trimHandleWidth, getHeight());
        g.fillRect(getWidth() - trimHandleWidth, 0, trimHandleWidth, getHeight());
    }

    // Border
    g.setColour(clipColor.brighter(0.1f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}

void ClipComponent::resized()
{
    // Nothing specific needed
}

void ClipComponent::mouseDown(const juce::MouseEvent& e)
{
    // Check for trim handles first
    TrimMode mode;
    if (selected && isOverTrimHandle(e.getPosition(), mode))
    {
        trimMode = mode;
        dragStartPos = e.getPosition();
        dragStartTimelinePos = timelineStart;
        return;
    }

    // Regular selection/drag
    if (onClipSelected)
        onClipSelected(this);

    dragging = true;
    dragStartPos = e.getPosition();
    dragStartTimelinePos = timelineStart;
}

void ClipComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (trimMode != TrimMode::None)
    {
        // Handle trimming (to be implemented in T3-4)
        return;
    }

    if (dragging)
    {
        // Calculate new position
        auto parentBounds = getParentComponent()->getLocalBounds();
        int deltaX = e.getPosition().x - dragStartPos.x;

        // Get the timeline panel to access pixelsPerSample
        if (auto* lane = dynamic_cast<TrackLaneComponent*>(getParentComponent()))
        {
            double pps = lane->getPixelsPerSample();
            juce::int64 deltaSamples = static_cast<juce::int64>(deltaX / pps);
            juce::int64 newStart = juce::jmax(juce::int64(0), dragStartTimelinePos + deltaSamples);

            if (onClipMoved)
                onClipMoved(this, newStart);
        }
    }
}

void ClipComponent::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    dragging = false;
    trimMode = TrimMode::None;
}

void ClipComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    // Could open clip editor
}

void ClipComponent::updateFromState()
{
    ClipModel clip(state);

    clipName = clip.getClipName();
    clipColor = clip.getClipColor();
    timelineStart = clip.getTimelineStart();
    length = clip.getLength();
    gain = clip.getGain();

    repaint();
}

juce::String ClipComponent::getClipId() const
{
    return state[IDs::clipId].toString();
}

juce::int64 ClipComponent::getTimelineStart() const
{
    return timelineStart;
}

juce::int64 ClipComponent::getTimelineEnd() const
{
    return timelineStart + length;
}

void ClipComponent::setSelected(bool sel)
{
    selected = sel;
    repaint();
}

void ClipComponent::loadThumbnail()
{
    juce::String filePath = state[IDs::audioFilePath].toString();
    juce::File file(filePath);

    if (file.existsAsFile())
    {
        thumbnail.setSource(new juce::FileInputSource(file));
        thumbnailLoaded = true;
    }
}

bool ClipComponent::isOverTrimHandle(const juce::Point<int>& pos, TrimMode& mode) const
{
    if (pos.x < trimHandleWidth)
    {
        mode = TrimMode::Start;
        return true;
    }
    if (pos.x > getWidth() - trimHandleWidth)
    {
        mode = TrimMode::End;
        return true;
    }
    mode = TrimMode::None;
    return false;
}

//==============================================================================
// TrackLaneComponent Implementation
//==============================================================================

TrackLaneComponent::TrackLaneComponent(ProjectManager& pm,
                                       const juce::ValueTree& trackState,
                                       juce::AudioThumbnailCache& cache,
                                       juce::AudioFormatManager& fm)
    : projectManager(pm)
    , state(trackState)
    , thumbnailCache(cache)
    , formatManager(fm)
{
    TrackModel track(trackState);
    trackColor = track.getColor();
    rebuildClips();
}

void TrackLaneComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(trackColor.withAlpha(0.1f));
    g.fillRect(bounds);

    // Grid lines (every beat)
    // TODO: Draw grid based on BPM and time signature

    // Border
    g.setColour(juce::Colours::grey.darker());
    g.drawLine(0, static_cast<float>(bounds.getBottom() - 1),
               static_cast<float>(bounds.getWidth()),
               static_cast<float>(bounds.getBottom() - 1));
}

void TrackLaneComponent::resized()
{
    layoutClips();
}

void TrackLaneComponent::updateFromState()
{
    TrackModel track(state);
    trackColor = track.getColor();
    repaint();
}

void TrackLaneComponent::rebuildClips()
{
    clips.clear();
    selectedClip = nullptr;

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild(i);
        if (child.hasType(IDs::CLIP))
        {
            auto* clip = new ClipComponent(projectManager, child, thumbnailCache, formatManager);
            clip->onClipSelected = [this](ClipComponent* c) { handleClipSelected(c); };
            clip->onClipMoved = [this](ClipComponent* c, juce::int64 pos) { handleClipMoved(c, pos); };
            clips.add(clip);
            addAndMakeVisible(clip);
        }
    }

    layoutClips();
}

juce::String TrackLaneComponent::getTrackId() const
{
    return state[IDs::trackId].toString();
}

void TrackLaneComponent::clearSelection()
{
    if (selectedClip != nullptr)
    {
        selectedClip->setSelected(false);
        selectedClip = nullptr;
    }
}

void TrackLaneComponent::layoutClips()
{
    int height = getHeight() - 4;

    for (auto* clip : clips)
    {
        juce::int64 startSample = clip->getTimelineStart();
        juce::int64 endSample = clip->getTimelineEnd();

        int x = static_cast<int>(startSample * pixelsPerSample);
        int width = static_cast<int>((endSample - startSample) * pixelsPerSample);

        clip->setBounds(x, 2, juce::jmax(10, width), height);
    }
}

void TrackLaneComponent::handleClipSelected(ClipComponent* clip)
{
    // Deselect previous
    if (selectedClip != nullptr && selectedClip != clip)
        selectedClip->setSelected(false);

    selectedClip = clip;
    selectedClip->setSelected(true);

    if (onClipSelected)
        onClipSelected(clip);
}

void TrackLaneComponent::handleClipMoved(ClipComponent* clip, juce::int64 newStart)
{
    // Update the clip's state through the project manager
    auto clipState = clip->getState();
    projectManager.moveClip(clipState, newStart);

    // The clip will update its own position from the state change
    clip->updateFromState();
    layoutClips();
}

//==============================================================================
// HorizontalFaderStrip Implementation
//==============================================================================

HorizontalFaderStrip::HorizontalFaderStrip(ProjectManager& pm, const juce::ValueTree& trackState)
    : projectManager(pm)
    , state(trackState)
{
    setupComponents();
    updateFromState();
    startTimerHz(30);
}

HorizontalFaderStrip::~HorizontalFaderStrip()
{
    stopTimer();
}

void HorizontalFaderStrip::setupComponents()
{
    // Track name label
    nameLabel.setJustificationType(juce::Justification::centredLeft);
    nameLabel.setFont(juce::Font(11.0f));
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(nameLabel);

    // Mute button
    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    muteButton.onClick = [this]() {
        projectManager.setTrackMute(state, muteButton.getToggleState());
    };
    addAndMakeVisible(muteButton);

    // Solo button
    soloButton.setClickingTogglesState(true);
    soloButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow);
    soloButton.onClick = [this]() {
        projectManager.setTrackSolo(state, soloButton.getToggleState());
    };
    addAndMakeVisible(soloButton);

    // Pan slider (small rotary)
    panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.setValue(0.0);
    panSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00aaff));
    panSlider.onValueChange = [this]() {
        projectManager.setTrackPan(state, static_cast<float>(panSlider.getValue()));
    };
    addAndMakeVisible(panSlider);

    // Volume fader (horizontal)
    faderSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    faderSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    faderSlider.setRange(-60.0, 6.0, 0.1);
    faderSlider.setValue(0.0);
    faderSlider.setSkewFactorFromMidPoint(-6.0);
    faderSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff3a3a3a));
    faderSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    faderSlider.onValueChange = [this]() {
        float gainDb = static_cast<float>(faderSlider.getValue());
        float gainLinear = juce::Decibels::decibelsToGain(gainDb);
        projectManager.setTrackVolume(state, gainLinear);
    };
    addAndMakeVisible(faderSlider);
}

void HorizontalFaderStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(juce::Colour(0xff252525));
    g.fillRect(bounds);

    // Left color strip
    g.setColour(trackColor);
    g.fillRect(0, 0, 4, bounds.getHeight());

    // Mini horizontal meter (below fader)
    auto meterBounds = bounds.removeFromBottom(8).reduced(80, 1);
    meterBounds.removeFromLeft(4);

    // Meter background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(meterBounds);

    // Left channel meter
    float leftLevel = juce::jlimit(0.0f, 1.0f, levelL);
    int leftWidth = static_cast<int>(leftLevel * meterBounds.getWidth() / 2);
    auto leftMeterBounds = meterBounds.removeFromLeft(meterBounds.getWidth() / 2);
    if (leftWidth > 0)
    {
        juce::Colour meterColor = leftLevel > 0.9f ? juce::Colours::red :
                                  leftLevel > 0.7f ? juce::Colours::yellow :
                                  juce::Colours::green;
        g.setColour(meterColor);
        g.fillRect(leftMeterBounds.getX(), leftMeterBounds.getY(), leftWidth, leftMeterBounds.getHeight());
    }

    // Right channel meter
    float rightLevel = juce::jlimit(0.0f, 1.0f, levelR);
    int rightWidth = static_cast<int>(rightLevel * meterBounds.getWidth());
    if (rightWidth > 0)
    {
        juce::Colour meterColor = rightLevel > 0.9f ? juce::Colours::red :
                                  rightLevel > 0.7f ? juce::Colours::yellow :
                                  juce::Colours::green;
        g.setColour(meterColor);
        g.fillRect(meterBounds.getX(), meterBounds.getY(), rightWidth, meterBounds.getHeight());
    }

    // Bottom border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawLine(0, static_cast<float>(getHeight() - 1),
               static_cast<float>(getWidth()), static_cast<float>(getHeight() - 1));
}

void HorizontalFaderStrip::resized()
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromLeft(6);  // Space for color strip

    // Top row: name, M, S, Pan
    auto topRow = bounds.removeFromTop(22);

    nameLabel.setBounds(topRow.removeFromLeft(70));
    topRow.removeFromLeft(4);

    muteButton.setBounds(topRow.removeFromLeft(20));
    topRow.removeFromLeft(2);
    soloButton.setBounds(topRow.removeFromLeft(20));
    topRow.removeFromLeft(4);

    panSlider.setBounds(topRow.removeFromLeft(24).reduced(0, 2));

    // Bottom row: fader (leaving space for meter)
    bounds.removeFromBottom(10);  // Space for meter
    faderSlider.setBounds(bounds.reduced(0, 2));
}

void HorizontalFaderStrip::timerCallback()
{
    // Decay peaks
    peakL *= 0.95f;
    peakR *= 0.95f;

    if (levelL < peakL) levelL = peakL;
    if (levelR < peakR) levelR = peakR;

    repaint();
}

void HorizontalFaderStrip::updateFromState()
{
    TrackModel track(state);

    nameLabel.setText(track.getName(), juce::dontSendNotification);
    trackColor = track.getColor();
    muteButton.setToggleState(track.isMuted(), juce::dontSendNotification);
    soloButton.setToggleState(track.isSoloed(), juce::dontSendNotification);

    float volume = track.getVolume();
    float volumeDb = juce::Decibels::gainToDecibels(volume, -60.0f);
    faderSlider.setValue(volumeDb, juce::dontSendNotification);

    panSlider.setValue(track.getPan(), juce::dontSendNotification);

    repaint();
}

juce::String HorizontalFaderStrip::getTrackId() const
{
    return state[IDs::trackId].toString();
}

void HorizontalFaderStrip::setMeterLevels(float left, float right)
{
    levelL = left;
    levelR = right;
    peakL = juce::jmax(peakL, left);
    peakR = juce::jmax(peakR, right);
}

//==============================================================================
// MixerSectionComponent Implementation
//==============================================================================

MixerSectionComponent::MixerSectionComponent(ProjectManager& pm)
    : projectManager(pm)
{
    rebuildStrips();
}

void MixerSectionComponent::paint(juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Top border/separator
    g.setColour(juce::Colour(0xff444444));
    g.drawLine(0, 0, static_cast<float>(getWidth()), 0, 2.0f);

    // "MIXER" label on left
    g.setColour(juce::Colour(0xff666666));
    g.setFont(10.0f);
    g.drawText("MIX", 4, 4, 40, 14, juce::Justification::centredLeft, false);
}

void MixerSectionComponent::resized()
{
    layoutStrips();
}

void MixerSectionComponent::rebuildStrips()
{
    strips.clear();

    auto& project = projectManager.getProject();
    auto tracks = project.getTracksSortedByOrder();

    for (auto& trackState : tracks)
    {
        auto* strip = new HorizontalFaderStrip(projectManager, trackState);
        strips.add(strip);
        addAndMakeVisible(strip);
    }

    layoutStrips();
}

void MixerSectionComponent::updateFromProject()
{
    for (auto* strip : strips)
    {
        strip->updateFromState();
    }
}

void MixerSectionComponent::setScrollOffset(int offset)
{
    if (scrollOffset != offset)
    {
        scrollOffset = offset;
        layoutStrips();
    }
}

void MixerSectionComponent::layoutStrips()
{
    int y = 20;  // Leave space for header
    int currentStripHeight = (getHeight() - 20) / juce::jmax(1, strips.size());
    currentStripHeight = juce::jlimit(40, 60, currentStripHeight);

    for (auto* strip : strips)
    {
        strip->setBounds(0, y, getWidth(), currentStripHeight);
        y += currentStripHeight;
    }
}

//==============================================================================
// TimelineRuler Implementation
//==============================================================================

TimelineRuler::TimelineRuler()
{
}

void TimelineRuler::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(bounds);

    g.setColour(juce::Colours::grey);
    g.setFont(10.0f);

    // Calculate visible range
    double samplesPerBeat = sampleRate * 60.0 / bpm;
    double samplesPerBar = samplesPerBeat * timeSigNum;
    double pixelsPerBar = samplesPerBar * pixelsPerSample;
    double pixelsPerBeat = samplesPerBeat * pixelsPerSample;

    // Determine tick spacing based on zoom level
    double majorTickSpacing = pixelsPerBar;
    double minorTickSpacing = pixelsPerBeat;

    // Adjust for very zoomed out views
    while (majorTickSpacing < 50)
    {
        majorTickSpacing *= 2;
        minorTickSpacing *= 2;
    }

    // Draw ticks
    double startX = -scrollOffset;
    int bar = 1;

    for (double x = startX; x < bounds.getWidth(); x += majorTickSpacing)
    {
        if (x >= 0)
        {
            // Major tick (bar)
            g.setColour(juce::Colours::grey);
            g.drawLine(static_cast<float>(x), static_cast<float>(bounds.getHeight() - 15),
                       static_cast<float>(x), static_cast<float>(bounds.getHeight()), 1.0f);

            // Label
            juce::String label;
            if (displayMode == DisplayMode::Bars)
                label = juce::String(bar);
            else if (displayMode == DisplayMode::Time)
            {
                double seconds = (x + scrollOffset) / pixelsPerSample / sampleRate;
                int mins = static_cast<int>(seconds) / 60;
                double secs = std::fmod(seconds, 60.0);
                label = juce::String::formatted("%d:%05.2f", mins, secs);
            }
            else
            {
                juce::int64 samples = static_cast<juce::int64>((x + scrollOffset) / pixelsPerSample);
                label = juce::String(samples);
            }

            g.drawText(label, static_cast<int>(x) + 2, 2, 60, 12,
                juce::Justification::centredLeft, false);
        }

        // Minor ticks (beats within bar)
        if (minorTickSpacing > 10)
        {
            for (int beat = 1; beat < timeSigNum; ++beat)
            {
                double beatX = x + beat * minorTickSpacing;
                if (beatX >= 0 && beatX < bounds.getWidth())
                {
                    g.setColour(juce::Colours::grey.darker());
                    g.drawLine(static_cast<float>(beatX),
                               static_cast<float>(bounds.getHeight() - 8),
                               static_cast<float>(beatX),
                               static_cast<float>(bounds.getHeight()), 0.5f);
                }
            }
        }

        bar++;
    }

    // Bottom border
    g.setColour(juce::Colours::grey.darker());
    g.drawLine(0, static_cast<float>(bounds.getHeight() - 1),
               static_cast<float>(bounds.getWidth()),
               static_cast<float>(bounds.getHeight() - 1));
}

//==============================================================================
// PlayheadComponent Implementation
//==============================================================================

PlayheadComponent::PlayheadComponent()
{
    setInterceptsMouseClicks(false, false);
}

void PlayheadComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Playhead line
    g.setColour(juce::Colours::red);
    g.fillRect(0, 0, 2, bounds.getHeight());

    // Top triangle
    juce::Path triangle;
    triangle.addTriangle(0, 0, 8, 0, 4, 8);
    g.fillPath(triangle);
}

void PlayheadComponent::setPosition(juce::int64 samplePos)
{
    positionSamples = samplePos;
    updatePosition();
}

void PlayheadComponent::updatePosition()
{
    int x = static_cast<int>(positionSamples * pixelsPerSample) - scrollOffset;
    setBounds(x - 4, 0, 10, getParentHeight());
}

//==============================================================================
// TimelinePanel Implementation
//==============================================================================

TimelinePanel::TimelinePanel(ProjectManager& pm, juce::AudioFormatManager& fm)
    : projectManager(pm)
    , formatManager(fm)
{
    projectManager.addListener(this);

    // Setup ruler
    addAndMakeVisible(ruler);

    // Setup playhead
    addAndMakeVisible(playhead);

    // Setup header viewport
    headerViewport.setViewedComponent(&headerContainer, false);
    headerViewport.setScrollBarsShown(false, false);
    addAndMakeVisible(headerViewport);

    // Setup track viewport
    trackViewport.setViewedComponent(&trackContainer, false);
    trackViewport.setScrollBarsShown(true, true);
    trackViewport.onScroll = [this]() {
        // Sync header scroll with track scroll
        headerViewport.setViewPosition(0, trackViewport.getViewPositionY());
        scrollOffsetX = trackViewport.getViewPositionX();
        ruler.setScrollOffset(scrollOffsetX);
        playhead.setScrollOffset(scrollOffsetX);
    };
    addAndMakeVisible(trackViewport);

    // Setup mixer section (bottom faders)
    mixerSection = std::make_unique<MixerSectionComponent>(projectManager);
    addAndMakeVisible(*mixerSection);

    // Toggle mixer button
    toggleMixerButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    toggleMixerButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff0066cc));
    toggleMixerButton.setClickingTogglesState(true);
    toggleMixerButton.setToggleState(true, juce::dontSendNotification);
    toggleMixerButton.onClick = [this]() {
        setMixerVisible(toggleMixerButton.getToggleState());
    };
    addAndMakeVisible(toggleMixerButton);

    // Initial setup from project
    rebuildTracks();

    startTimerHz(30);
}

TimelinePanel::~TimelinePanel()
{
    stopTimer();
    projectManager.removeListener(this);
}

void TimelinePanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
}

void TimelinePanel::resized()
{
    auto bounds = getLocalBounds();

    // Toggle mixer button in top-left corner
    toggleMixerButton.setBounds(2, 2, 40, 26);

    // Mixer section at bottom (if visible)
    int currentMixerHeight = mixerVisible ? mixerHeight : 0;
    if (mixerSection != nullptr)
    {
        if (mixerVisible)
        {
            mixerSection->setBounds(0, bounds.getHeight() - mixerHeight, bounds.getWidth(), mixerHeight);
            mixerSection->setVisible(true);
        }
        else
        {
            mixerSection->setVisible(false);
        }
    }

    // Remaining area for timeline
    auto timelineArea = bounds.withTrimmedBottom(currentMixerHeight);

    // Ruler at top
    ruler.setBounds(headerWidth, 0, timelineArea.getWidth() - headerWidth, rulerHeight);

    // Header area on left
    auto headerArea = timelineArea.removeFromLeft(headerWidth);
    headerArea.removeFromTop(rulerHeight);
    headerViewport.setBounds(headerArea);

    // Track area fills the rest
    timelineArea.removeFromTop(rulerHeight);
    trackViewport.setBounds(timelineArea);

    // Playhead spans timeline height (not mixer)
    playhead.setBounds(headerWidth, 0, 10, bounds.getHeight() - currentMixerHeight);

    layoutTracks();
}

void TimelinePanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCtrlDown())
    {
        // Zoom
        if (wheel.deltaY > 0)
            zoomIn();
        else if (wheel.deltaY < 0)
            zoomOut();
    }
    else
    {
        // Scroll (let viewport handle it)
        trackViewport.mouseWheelMove(e, wheel);
    }
}

void TimelinePanel::mouseDown(const juce::MouseEvent& e)
{
    // Click on ruler or timeline to set playhead position
    if (e.y < rulerHeight || e.x > headerWidth)
    {
        int clickX = e.x - headerWidth + scrollOffsetX;
        juce::int64 newPos = static_cast<juce::int64>(clickX / pixelsPerSample);
        newPos = juce::jmax(juce::int64(0), newPos);

        if (snapEnabled)
            newPos = snapToGrid(newPos);

        setPlayheadPosition(newPos);

        if (onPlayheadMoved)
            onPlayheadMoved(newPos);
    }
}

void TimelinePanel::timerCallback()
{
    // Update playhead position from external source if needed
    playhead.repaint();
}

//==============================================================================
// ProjectManager::Listener
//==============================================================================

void TimelinePanel::projectChanged()
{
    rebuildTracks();
}

void TimelinePanel::trackAdded(const juce::ValueTree& track)
{
    juce::ignoreUnused(track);
    rebuildTracks();
}

void TimelinePanel::trackRemoved(const juce::ValueTree& track)
{
    juce::ignoreUnused(track);
    rebuildTracks();
}

void TimelinePanel::trackPropertyChanged(const juce::ValueTree& track, const juce::Identifier& property)
{
    juce::ignoreUnused(property);

    juce::String trackId = track[IDs::trackId].toString();

    // Update header
    for (auto* header : trackHeaders)
    {
        if (header->getTrackId() == trackId)
        {
            header->updateFromState();
            break;
        }
    }

    // Update lane
    for (auto* lane : trackLanes)
    {
        if (lane->getTrackId() == trackId)
        {
            lane->updateFromState();
            break;
        }
    }
}

void TimelinePanel::clipAdded(const juce::ValueTree& clip)
{
    auto parent = clip.getParent();
    if (parent.isValid() && parent.hasType(IDs::TRACK))
    {
        juce::String trackId = parent[IDs::trackId].toString();
        for (auto* lane : trackLanes)
        {
            if (lane->getTrackId() == trackId)
            {
                lane->rebuildClips();
                break;
            }
        }
    }
}

void TimelinePanel::clipRemoved(const juce::ValueTree& clip)
{
    juce::ignoreUnused(clip);
    // The clip's parent is no longer valid at this point,
    // so we rebuild all lanes (could be optimized)
    for (auto* lane : trackLanes)
        lane->rebuildClips();
}

void TimelinePanel::clipPropertyChanged(const juce::ValueTree& clip, const juce::Identifier& property)
{
    juce::ignoreUnused(property);

    auto parent = clip.getParent();
    if (parent.isValid() && parent.hasType(IDs::TRACK))
    {
        juce::String trackId = parent[IDs::trackId].toString();
        for (auto* lane : trackLanes)
        {
            if (lane->getTrackId() == trackId)
            {
                lane->rebuildClips();  // Could be optimized to just update the specific clip
                break;
            }
        }
    }
}

//==============================================================================
// Timeline Control
//==============================================================================

void TimelinePanel::zoomIn()
{
    setZoomLevel(getZoomLevel() / 1.5);
}

void TimelinePanel::zoomOut()
{
    setZoomLevel(getZoomLevel() * 1.5);
}

void TimelinePanel::zoomToFit()
{
    juce::int64 projectLength = projectManager.getProject().getProjectLength();
    if (projectLength > 0)
    {
        double width = trackViewport.getWidth();
        setZoomLevel(projectLength / width);
    }
}

void TimelinePanel::setZoomLevel(double samplesPerPixel)
{
    // Clamp zoom level
    samplesPerPixel = juce::jlimit(1.0, 10000.0, samplesPerPixel);
    pixelsPerSample = 1.0 / samplesPerPixel;

    updateZoom();
}

void TimelinePanel::scrollToPosition(juce::int64 samplePos)
{
    int x = static_cast<int>(samplePos * pixelsPerSample);
    trackViewport.setViewPosition(x, trackViewport.getViewPositionY());
}

void TimelinePanel::scrollBy(int pixels)
{
    auto pos = trackViewport.getViewPosition();
    trackViewport.setViewPosition(pos.x + pixels, pos.y);
}

void TimelinePanel::setPlayheadPosition(juce::int64 samplePos)
{
    playheadPosition = samplePos;
    playhead.setPosition(samplePos);
}

void TimelinePanel::setTrackHeight(int height)
{
    trackHeight = juce::jlimit(40, 200, height);
    layoutTracks();
}

void TimelinePanel::setMixerVisible(bool visible)
{
    if (mixerVisible != visible)
    {
        mixerVisible = visible;
        toggleMixerButton.setToggleState(visible, juce::dontSendNotification);
        resized();
    }
}

//==============================================================================
// Private Methods
//==============================================================================

void TimelinePanel::rebuildTracks()
{
    trackHeaders.clear();
    trackLanes.clear();

    auto& project = projectManager.getProject();

    // Update ruler with project settings
    ruler.setSampleRate(project.getSampleRate());
    ruler.setBpm(project.getBpm());
    ruler.setTimeSignature(project.getTimeSignatureNumerator(),
                           project.getTimeSignatureDenominator());

    // Create track headers and lanes
    auto tracks = project.getTracksSortedByOrder();

    for (auto& trackState : tracks)
    {
        // Header
        auto* header = new TrackHeaderComponent(projectManager, trackState);
        trackHeaders.add(header);
        headerContainer.addAndMakeVisible(header);

        // Lane
        auto* lane = new TrackLaneComponent(projectManager, trackState, thumbnailCache, formatManager);
        lane->setPixelsPerSample(pixelsPerSample);
        lane->onClipSelected = [this](ClipComponent* c) { handleClipSelected(c); };
        trackLanes.add(lane);
        trackContainer.addAndMakeVisible(lane);
    }

    // Rebuild mixer section
    if (mixerSection != nullptr)
        mixerSection->rebuildStrips();

    layoutTracks();
}

void TimelinePanel::layoutTracks()
{
    int numTracks = trackHeaders.size();
    int totalHeight = numTracks * trackHeight;

    // Calculate timeline width based on project length
    juce::int64 projectLength = projectManager.getProject().getProjectLength();
    int timelineWidth = static_cast<int>(projectLength * pixelsPerSample) + 1000;  // Extra space
    timelineWidth = juce::jmax(timelineWidth, trackViewport.getWidth());

    // Size containers
    headerContainer.setSize(headerWidth, totalHeight);
    trackContainer.setSize(timelineWidth, totalHeight);

    // Layout headers
    int y = 0;
    for (auto* header : trackHeaders)
    {
        header->setBounds(0, y, headerWidth, trackHeight);
        y += trackHeight;
    }

    // Layout lanes
    y = 0;
    for (auto* lane : trackLanes)
    {
        lane->setBounds(0, y, timelineWidth, trackHeight);
        y += trackHeight;
    }
}

void TimelinePanel::updateZoom()
{
    ruler.setPixelsPerSample(pixelsPerSample);
    playhead.setPixelsPerSample(pixelsPerSample);

    for (auto* lane : trackLanes)
        lane->setPixelsPerSample(pixelsPerSample);

    layoutTracks();
}

juce::int64 TimelinePanel::snapToGrid(juce::int64 samplePos) const
{
    auto& project = projectManager.getProject();
    double sampleRate = project.getSampleRate();
    double bpm = project.getBpm();

    double samplesPerBeat = sampleRate * 60.0 / bpm;
    double samplesPerSnap = samplesPerBeat * snapResolution;

    return static_cast<juce::int64>(std::round(samplePos / samplesPerSnap) * samplesPerSnap);
}

void TimelinePanel::handleClipSelected(ClipComponent* clip)
{
    // Clear selection on all lanes
    for (auto* lane : trackLanes)
    {
        if (lane->getSelectedClip() != clip)
            lane->clearSelection();
    }

    if (onClipSelected)
        onClipSelected(clip);
}

/*
  ==============================================================================

    MultiTrackAudioSource.cpp

    Multi-track audio playback engine implementation

  ==============================================================================
*/

#include "MultiTrackAudioSource.h"

//==============================================================================
// AudioFileCache Implementation
//==============================================================================

AudioFileCache::AudioFileCache(juce::AudioFormatManager& fm)
    : formatManager(fm)
{
}

juce::AudioFormatReaderSource* AudioFileCache::getReaderSource(const juce::String& filePath)
{
    juce::ScopedLock sl(cacheLock);

    // Check if already cached
    auto it = cache.find(filePath);
    if (it != cache.end() && it->second.source != nullptr)
    {
        return it->second.source.get();
    }

    // Try to load the file
    juce::File file(filePath);
    if (!file.existsAsFile())
        return nullptr;

    auto reader = std::unique_ptr<juce::AudioFormatReader>(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
        return nullptr;

    // Create cached entry
    CachedFile entry;
    entry.sampleRate = reader->sampleRate;
    entry.lengthInSamples = reader->lengthInSamples;
    entry.numChannels = static_cast<int>(reader->numChannels);

    auto* rawReader = reader.get();
    entry.reader = std::move(reader);
    entry.source = std::make_unique<juce::AudioFormatReaderSource>(rawReader, false);

    auto* sourcePtr = entry.source.get();
    cache[filePath] = std::move(entry);

    return sourcePtr;
}

double AudioFileCache::getSampleRate(const juce::String& filePath) const
{
    juce::ScopedLock sl(cacheLock);
    auto it = cache.find(filePath);
    if (it != cache.end())
        return it->second.sampleRate;
    return 0.0;
}

juce::int64 AudioFileCache::getLengthInSamples(const juce::String& filePath) const
{
    juce::ScopedLock sl(cacheLock);
    auto it = cache.find(filePath);
    if (it != cache.end())
        return it->second.lengthInSamples;
    return 0;
}

int AudioFileCache::getNumChannels(const juce::String& filePath) const
{
    juce::ScopedLock sl(cacheLock);
    auto it = cache.find(filePath);
    if (it != cache.end())
        return it->second.numChannels;
    return 0;
}

void AudioFileCache::clearCache()
{
    juce::ScopedLock sl(cacheLock);
    cache.clear();
}

void AudioFileCache::removeFromCache(const juce::String& filePath)
{
    juce::ScopedLock sl(cacheLock);
    cache.erase(filePath);
}

//==============================================================================
// ClipAudioSource Implementation
//==============================================================================

ClipAudioSource::ClipAudioSource(AudioFileCache& cache, const juce::ValueTree& clipState)
    : audioCache(cache)
    , state(clipState)
{
    updateFromState();
}

void ClipAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    samplesPerBlock = samplesPerBlockExpected;
    currentSampleRate = sampleRate;
    tempBuffer.setSize(2, samplesPerBlockExpected);
}

void ClipAudioSource::releaseResources()
{
    tempBuffer.setSize(0, 0);
}

void ClipAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Check if clip is active at current position
    if (!isActiveAt(currentPosition))
    {
        bufferToFill.clearActiveBufferRegion();
        currentPosition += bufferToFill.numSamples;
        return;
    }

    auto* readerSource = audioCache.getReaderSource(audioFilePath);
    if (readerSource == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        currentPosition += bufferToFill.numSamples;
        return;
    }

    // Calculate position within clip
    juce::int64 clipStart = timelineStart;
    juce::int64 positionInClip = currentPosition - clipStart;

    // Set reader position (sourceStart + position within clip)
    readerSource->setNextReadPosition(sourceStart + positionInClip);

    // Read audio
    readerSource->getNextAudioBlock(bufferToFill);

    // Apply gain and fades
    auto* buffer = bufferToFill.buffer;
    int startSample = bufferToFill.startSample;
    int numSamples = bufferToFill.numSamples;

    for (int i = 0; i < numSamples; ++i)
    {
        juce::int64 samplePos = positionInClip + i;
        float fadeGain = calculateFadeGain(samplePos);
        float totalGain = gain * fadeGain;

        for (int ch = 0; ch < buffer->getNumChannels(); ++ch)
        {
            buffer->setSample(ch, startSample + i,
                buffer->getSample(ch, startSample + i) * totalGain);
        }
    }

    currentPosition += numSamples;
}

void ClipAudioSource::setNextReadPosition(juce::int64 newPosition)
{
    currentPosition = newPosition;
}

juce::int64 ClipAudioSource::getNextReadPosition() const
{
    return currentPosition;
}

juce::int64 ClipAudioSource::getTotalLength() const
{
    return timelineStart + length;
}

bool ClipAudioSource::isActiveAt(juce::int64 timelinePosition) const
{
    return timelinePosition >= timelineStart &&
           timelinePosition < (timelineStart + length);
}

juce::int64 ClipAudioSource::getTimelineStart() const
{
    return timelineStart;
}

juce::int64 ClipAudioSource::getTimelineEnd() const
{
    return timelineStart + length;
}

float ClipAudioSource::getGain() const
{
    return gain;
}

void ClipAudioSource::updateFromState()
{
    audioFilePath = state[IDs::audioFilePath].toString();
    sourceStart = static_cast<juce::int64>(state[IDs::sourceStart]);
    length = static_cast<juce::int64>(state[IDs::length]);
    timelineStart = static_cast<juce::int64>(state[IDs::timelineStart]);
    gain = static_cast<float>(state[IDs::gain]);
    fadeInSamples = static_cast<juce::int64>(state[IDs::fadeInSamples]);
    fadeOutSamples = static_cast<juce::int64>(state[IDs::fadeOutSamples]);
}

juce::String ClipAudioSource::getClipId() const
{
    return state[IDs::clipId].toString();
}

float ClipAudioSource::calculateFadeGain(juce::int64 positionInClip) const
{
    float fadeGain = 1.0f;

    // Fade in
    if (fadeInSamples > 0 && positionInClip < fadeInSamples)
    {
        fadeGain *= static_cast<float>(positionInClip) / static_cast<float>(fadeInSamples);
    }

    // Fade out
    if (fadeOutSamples > 0)
    {
        juce::int64 fadeOutStart = length - fadeOutSamples;
        if (positionInClip >= fadeOutStart)
        {
            juce::int64 posInFadeOut = positionInClip - fadeOutStart;
            fadeGain *= 1.0f - (static_cast<float>(posInFadeOut) / static_cast<float>(fadeOutSamples));
        }
    }

    return juce::jlimit(0.0f, 1.0f, fadeGain);
}

//==============================================================================
// TrackAudioSource Implementation
//==============================================================================

TrackAudioSource::TrackAudioSource(AudioFileCache& cache, const juce::ValueTree& trackState)
    : audioCache(cache)
    , state(trackState)
{
    updateFromState();
    rebuildClips();
}

void TrackAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    samplesPerBlock = samplesPerBlockExpected;
    currentSampleRate = sampleRate;
    mixBuffer.setSize(2, samplesPerBlockExpected);

    for (auto* clip : clips)
        clip->prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void TrackAudioSource::releaseResources()
{
    mixBuffer.setSize(0, 0);

    for (auto* clip : clips)
        clip->releaseResources();
}

void TrackAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Check if track should play
    if (!shouldPlay())
    {
        bufferToFill.clearActiveBufferRegion();
        currentPosition += bufferToFill.numSamples;
        return;
    }

    // Clear output buffer
    bufferToFill.clearActiveBufferRegion();

    // Mix all clips
    for (auto* clip : clips)
    {
        if (clip->isActiveAt(currentPosition))
        {
            // Prepare temp buffer for this clip
            mixBuffer.clear();
            juce::AudioSourceChannelInfo clipInfo(&mixBuffer, 0, bufferToFill.numSamples);

            clip->getNextAudioBlock(clipInfo);

            // Add to output buffer
            for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
            {
                int srcCh = ch < mixBuffer.getNumChannels() ? ch : 0;
                bufferToFill.buffer->addFrom(ch, bufferToFill.startSample,
                    mixBuffer, srcCh, 0, bufferToFill.numSamples);
            }
        }
        else
        {
            // Still advance clip position
            clip->setNextReadPosition(currentPosition + bufferToFill.numSamples);
        }
    }

    // Apply volume
    if (volume != 1.0f)
    {
        bufferToFill.buffer->applyGain(bufferToFill.startSample,
            bufferToFill.numSamples, volume);
    }

    // Apply pan
    if (pan != 0.0f && bufferToFill.buffer->getNumChannels() >= 2)
    {
        applyPan(*bufferToFill.buffer, pan);
    }

    currentPosition += bufferToFill.numSamples;
}

void TrackAudioSource::setNextReadPosition(juce::int64 newPosition)
{
    currentPosition = newPosition;

    for (auto* clip : clips)
        clip->setNextReadPosition(newPosition);
}

juce::int64 TrackAudioSource::getNextReadPosition() const
{
    return currentPosition;
}

juce::int64 TrackAudioSource::getTotalLength() const
{
    juce::int64 maxLength = 0;

    for (auto* clip : clips)
    {
        juce::int64 clipEnd = clip->getTimelineEnd();
        if (clipEnd > maxLength)
            maxLength = clipEnd;
    }

    return maxLength;
}

float TrackAudioSource::getVolume() const
{
    return volume;
}

float TrackAudioSource::getPan() const
{
    return pan;
}

bool TrackAudioSource::isMuted() const
{
    return muted;
}

bool TrackAudioSource::isSoloed() const
{
    return soloed;
}

void TrackAudioSource::setSoloActiveInProject(bool soloActive)
{
    soloActiveInProject = soloActive;
}

void TrackAudioSource::updateFromState()
{
    volume = static_cast<float>(state[IDs::volume]);
    pan = static_cast<float>(state[IDs::pan]);
    muted = static_cast<bool>(state[IDs::mute]);
    soloed = static_cast<bool>(state[IDs::solo]);
}

juce::String TrackAudioSource::getTrackId() const
{
    return state[IDs::trackId].toString();
}

void TrackAudioSource::rebuildClips()
{
    clips.clear();

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild(i);
        if (child.hasType(IDs::CLIP))
        {
            auto* clip = new ClipAudioSource(audioCache, child);
            clip->prepareToPlay(samplesPerBlock, currentSampleRate);
            clips.add(clip);
        }
    }
}

void TrackAudioSource::applyPan(juce::AudioBuffer<float>& buffer, float panValue)
{
    // Simple equal-power pan law
    float leftGain = std::cos((panValue + 1.0f) * juce::MathConstants<float>::halfPi / 2.0f);
    float rightGain = std::sin((panValue + 1.0f) * juce::MathConstants<float>::halfPi / 2.0f);

    if (buffer.getNumChannels() >= 2)
    {
        buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
        buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
    }
}

bool TrackAudioSource::shouldPlay() const
{
    if (muted)
        return false;

    if (soloActiveInProject && !soloed)
        return false;

    return true;
}

//==============================================================================
// MultiTrackAudioSource Implementation
//==============================================================================

MultiTrackAudioSource::MultiTrackAudioSource(juce::AudioFormatManager& fm)
    : formatManager(fm)
    , audioCache(fm)
{
}

MultiTrackAudioSource::~MultiTrackAudioSource()
{
    if (projectState.isValid())
        projectState.removeListener(this);
}

void MultiTrackAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    juce::ScopedLock sl(lock);

    samplesPerBlock = samplesPerBlockExpected;
    currentSampleRate = sampleRate;
    mixBuffer.setSize(2, samplesPerBlockExpected);

    for (auto* track : tracks)
        track->prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MultiTrackAudioSource::releaseResources()
{
    juce::ScopedLock sl(lock);

    mixBuffer.setSize(0, 0);

    for (auto* track : tracks)
        track->releaseResources();
}

void MultiTrackAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    juce::ScopedLock sl(lock);

    // Clear output buffer
    bufferToFill.clearActiveBufferRegion();

    if (tracks.isEmpty())
        return;

    // Handle looping
    if (looping && loopEnd > loopStart)
    {
        if (currentPosition >= loopEnd)
        {
            currentPosition = loopStart;
            for (auto* track : tracks)
                track->setNextReadPosition(loopStart);
        }
    }

    // Mix all tracks
    for (auto* track : tracks)
    {
        mixBuffer.clear();
        juce::AudioSourceChannelInfo trackInfo(&mixBuffer, 0, bufferToFill.numSamples);

        track->getNextAudioBlock(trackInfo);

        // Add to output buffer
        for (int ch = 0; ch < bufferToFill.buffer->getNumChannels(); ++ch)
        {
            int srcCh = ch < mixBuffer.getNumChannels() ? ch : 0;
            bufferToFill.buffer->addFrom(ch, bufferToFill.startSample,
                mixBuffer, srcCh, 0, bufferToFill.numSamples);
        }
    }

    // Apply master volume
    if (masterVolume != 1.0f)
    {
        bufferToFill.buffer->applyGain(bufferToFill.startSample,
            bufferToFill.numSamples, masterVolume);
    }

    // Apply master pan
    if (masterPan != 0.0f)
    {
        applyMasterPan(*bufferToFill.buffer);
    }

    currentPosition += bufferToFill.numSamples;
}

void MultiTrackAudioSource::setNextReadPosition(juce::int64 newPosition)
{
    juce::ScopedLock sl(lock);

    currentPosition = newPosition;

    for (auto* track : tracks)
        track->setNextReadPosition(newPosition);
}

juce::int64 MultiTrackAudioSource::getNextReadPosition() const
{
    return currentPosition;
}

juce::int64 MultiTrackAudioSource::getTotalLength() const
{
    juce::int64 maxLength = 0;

    for (auto* track : tracks)
    {
        juce::int64 trackLength = track->getTotalLength();
        if (trackLength > maxLength)
            maxLength = trackLength;
    }

    return maxLength;
}

void MultiTrackAudioSource::loadProject(const juce::ValueTree& newProjectState)
{
    juce::ScopedLock sl(lock);

    // Remove listener from old state
    if (projectState.isValid())
        projectState.removeListener(this);

    projectState = newProjectState;

    if (projectState.isValid())
    {
        projectState.addListener(this);

        // Get project sample rate
        projectSampleRate = static_cast<double>(projectState[IDs::sampleRate]);

        // Get master properties
        auto masterNode = projectState.getChildWithName(IDs::MASTER);
        if (masterNode.isValid())
        {
            masterVolume = static_cast<float>(masterNode[IDs::masterVolume]);
            masterPan = static_cast<float>(masterNode[IDs::masterPan]);
        }
    }

    rebuildFromProject();
}

void MultiTrackAudioSource::unloadProject()
{
    juce::ScopedLock sl(lock);

    if (projectState.isValid())
        projectState.removeListener(this);

    projectState = juce::ValueTree();
    tracks.clear();
    audioCache.clearCache();
    currentPosition = 0;
}

void MultiTrackAudioSource::rebuildFromProject()
{
    juce::ScopedLock sl(lock);

    tracks.clear();

    if (!projectState.isValid())
        return;

    for (int i = 0; i < projectState.getNumChildren(); ++i)
    {
        auto child = projectState.getChild(i);
        if (child.hasType(IDs::TRACK))
        {
            auto* track = new TrackAudioSource(audioCache, child);
            track->prepareToPlay(samplesPerBlock, currentSampleRate);
            tracks.add(track);
        }
    }

    updateSoloState();
}

double MultiTrackAudioSource::getProjectSampleRate() const
{
    return projectSampleRate;
}

void MultiTrackAudioSource::setLoopRange(juce::int64 startSample, juce::int64 endSample)
{
    loopStart = startSample;
    loopEnd = endSample;
}

void MultiTrackAudioSource::updateSoloState()
{
    bool anySoloed = false;

    for (auto* track : tracks)
    {
        if (track->isSoloed())
        {
            anySoloed = true;
            break;
        }
    }

    for (auto* track : tracks)
    {
        track->setSoloActiveInProject(anySoloed);
    }
}

void MultiTrackAudioSource::applyMasterPan(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() < 2)
        return;

    // Simple equal-power pan law
    float leftGain = std::cos((masterPan + 1.0f) * juce::MathConstants<float>::halfPi / 2.0f);
    float rightGain = std::sin((masterPan + 1.0f) * juce::MathConstants<float>::halfPi / 2.0f);

    buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
    buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
}

//==============================================================================
// ValueTree::Listener Implementation
//==============================================================================

void MultiTrackAudioSource::valueTreePropertyChanged(juce::ValueTree& tree,
                                                      const juce::Identifier& property)
{
    juce::ScopedLock sl(lock);

    if (tree.hasType(IDs::TRACK))
    {
        // Find and update the track
        juce::String trackId = tree[IDs::trackId].toString();
        for (auto* track : tracks)
        {
            if (track->getTrackId() == trackId)
            {
                track->updateFromState();

                if (property == IDs::solo || property == IDs::mute)
                    updateSoloState();

                break;
            }
        }
    }
    else if (tree.hasType(IDs::CLIP))
    {
        // Find the track containing this clip and rebuild its clips
        auto parentTrack = tree.getParent();
        if (parentTrack.isValid() && parentTrack.hasType(IDs::TRACK))
        {
            juce::String trackId = parentTrack[IDs::trackId].toString();
            for (auto* track : tracks)
            {
                if (track->getTrackId() == trackId)
                {
                    track->rebuildClips();
                    break;
                }
            }
        }
    }
    else if (tree.hasType(IDs::MASTER))
    {
        if (property == IDs::masterVolume)
            masterVolume = static_cast<float>(tree[IDs::masterVolume]);
        else if (property == IDs::masterPan)
            masterPan = static_cast<float>(tree[IDs::masterPan]);
    }
}

void MultiTrackAudioSource::valueTreeChildAdded(juce::ValueTree& parent,
                                                 juce::ValueTree& child)
{
    juce::ScopedLock sl(lock);

    if (child.hasType(IDs::TRACK))
    {
        auto* track = new TrackAudioSource(audioCache, child);
        track->prepareToPlay(samplesPerBlock, currentSampleRate);
        tracks.add(track);
        updateSoloState();
    }
    else if (child.hasType(IDs::CLIP))
    {
        // Find the track and rebuild its clips
        juce::String trackId = parent[IDs::trackId].toString();
        for (auto* track : tracks)
        {
            if (track->getTrackId() == trackId)
            {
                track->rebuildClips();
                break;
            }
        }
    }
}

void MultiTrackAudioSource::valueTreeChildRemoved(juce::ValueTree& parent,
                                                   juce::ValueTree& child,
                                                   int index)
{
    juce::ignoreUnused(index);
    juce::ScopedLock sl(lock);

    if (child.hasType(IDs::TRACK))
    {
        // Find and remove the track
        juce::String trackId = child[IDs::trackId].toString();
        for (int i = 0; i < tracks.size(); ++i)
        {
            if (tracks[i]->getTrackId() == trackId)
            {
                tracks.remove(i);
                break;
            }
        }
        updateSoloState();
    }
    else if (child.hasType(IDs::CLIP))
    {
        // Find the track and rebuild its clips
        juce::String trackId = parent[IDs::trackId].toString();
        for (auto* track : tracks)
        {
            if (track->getTrackId() == trackId)
            {
                track->rebuildClips();
                break;
            }
        }
    }
}

void MultiTrackAudioSource::valueTreeChildOrderChanged(juce::ValueTree& parent,
                                                        int oldIndex, int newIndex)
{
    juce::ignoreUnused(parent, oldIndex, newIndex);
    // Track order doesn't affect audio mixing
}

void MultiTrackAudioSource::valueTreeParentChanged(juce::ValueTree& tree)
{
    juce::ignoreUnused(tree);
}

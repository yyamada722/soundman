/*
  ==============================================================================

    AudioEngine.cpp

    Audio playback engine implementation

  ==============================================================================
*/

#include "AudioEngine.h"
#include <algorithm>

//==============================================================================
AudioEngine::AudioEngine()
{
    // Register audio formats
    formatManager.registerBasicFormats();  // WAV, AIFF

    // Listen for transport state changes (both tracks)
    transportSource.addChangeListener(this);
    transportSourceB.addChangeListener(this);
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

//==============================================================================
bool AudioEngine::initialize()
{
    if (initialized)
        return true;

    // Initialize audio device with default settings
    juce::String error = deviceManager.initialiseWithDefaultDevices(0, 2);

    if (error.isNotEmpty())
    {
        showError("Failed to initialize audio device: " + error);
        return false;
    }

    // Check if device was actually opened
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        showError("No audio output device available");
        return false;
    }

    // Add this as audio callback
    deviceManager.addAudioCallback(this);

    initialized = true;
    return true;
}

void AudioEngine::shutdown()
{
    if (!initialized)
        return;

    // Stop playback
    stop();
    unloadFile();

    // Remove audio callback
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();

    initialized = false;
}

//==============================================================================
bool AudioEngine::loadFile(const juce::File& file)
{
    // Check if file exists
    if (!file.existsAsFile())
    {
        showError("File not found: " + file.getFullPathName());
        return false;
    }

    // Create reader for the file
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
    {
        showError("Unsupported audio format: " + file.getFileExtension());
        return false;
    }

    // Stop current playback
    stop();

    // Create new reader source
    auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);

    // Set source to transport (Phase 1: simplified without resampling)
    transportSource.setSource(newSource.get());

    readerSource = std::move(newSource);
    resamplingSource.reset();  // Clear any previous resampling source
    currentFile = file;
    playState = PlayState::Stopped;

    return true;
}

void AudioEngine::unloadFile()
{
    stop();
    transportSource.setSource(nullptr);
    resamplingSource.reset();
    readerSource.reset();
    currentFile = juce::File();
    playState = PlayState::Stopped;
}

juce::String AudioEngine::getCurrentFileName() const
{
    return currentFile.getFileName();
}

bool AudioEngine::hasFileLoaded() const
{
    return readerSource != nullptr;
}

//==============================================================================
// Track B operations

bool AudioEngine::loadTrackB(const juce::File& file)
{
    // Check if file exists
    if (!file.existsAsFile())
    {
        showError("File not found: " + file.getFullPathName());
        return false;
    }

    // Create reader for the file
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr)
    {
        showError("Unsupported audio format: " + file.getFileExtension());
        return false;
    }

    // Stop Track B if playing
    transportSourceB.stop();
    transportSourceB.setSource(nullptr);

    // Create new reader source
    auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);

    // Set source to transport B
    transportSourceB.setSource(newSource.get());

    // Prepare if audio device is already running
    if (preparedSampleRate > 0)
    {
        transportSourceB.prepareToPlay(preparedBlockSize, preparedSampleRate);
    }

    readerSourceB = std::move(newSource);
    trackBFile = file;

    return true;
}

void AudioEngine::unloadTrackB()
{
    transportSourceB.stop();
    transportSourceB.setSource(nullptr);
    readerSourceB.reset();
    trackBFile = juce::File();
}

juce::String AudioEngine::getTrackBFileName() const
{
    return trackBFile.getFileName();
}

bool AudioEngine::hasTrackBLoaded() const
{
    return readerSourceB != nullptr;
}

void AudioEngine::setActiveTrack(ActiveTrack track)
{
    activeTrack.store(track);
}

void AudioEngine::setTrackMixBalance(float balance)
{
    trackMixBalance.store(juce::jlimit(0.0f, 1.0f, balance));
}

//==============================================================================
void AudioEngine::setMultiTrackSource(juce::PositionableAudioSource* source)
{
    // Prepare the source if we have a valid sample rate
    if (source != nullptr && preparedSampleRate > 0)
    {
        source->prepareToPlay(preparedBlockSize, preparedSampleRate);
    }
    multiTrackSource = source;
}

void AudioEngine::clearMultiTrackSource()
{
    if (multiTrackSource != nullptr)
    {
        multiTrackSource->releaseResources();
        multiTrackSource = nullptr;
    }
}

//==============================================================================
void AudioEngine::play()
{
    // Check if we have any source to play
    bool hasMultiTrack = (multiTrackSource != nullptr);
    bool hasAnySingleFile = hasFileLoaded() || hasTrackBLoaded();

    if (!hasMultiTrack && !hasAnySingleFile)
        return;

    auto currentState = playState.load();
    auto track = activeTrack.load();

    if (currentState == PlayState::Stopped)
    {
        // Multi-track source playback
        if (hasMultiTrack)
        {
            multiTrackSource->setNextReadPosition(0);
        }

        // Single file playback
        if (!hasMultiTrack && hasAnySingleFile)
        {
            if (hasFileLoaded() && (track == ActiveTrack::A || track == ActiveTrack::Both))
            {
                transportSource.setPosition(0.0);
                transportSource.start();
            }

            if (hasTrackBLoaded() && (track == ActiveTrack::B || track == ActiveTrack::Both))
            {
                transportSourceB.setPosition(0.0);
                transportSourceB.start();
            }
        }

        playState = PlayState::Playing;
    }
    else if (currentState == PlayState::Paused)
    {
        // Resume playback
        if (!hasMultiTrack && hasAnySingleFile)
        {
            if (hasFileLoaded() && (track == ActiveTrack::A || track == ActiveTrack::Both))
                transportSource.start();

            if (hasTrackBLoaded() && (track == ActiveTrack::B || track == ActiveTrack::Both))
                transportSourceB.start();
        }

        playState = PlayState::Playing;
    }
}

void AudioEngine::pause()
{
    if (playState.load() == PlayState::Playing)
    {
        transportSource.stop();
        transportSourceB.stop();
        playState = PlayState::Paused;
    }
}

void AudioEngine::stop()
{
    if (playState.load() != PlayState::Stopped)
    {
        transportSource.stop();
        transportSource.setPosition(0.0);
        transportSourceB.stop();
        transportSourceB.setPosition(0.0);
        playState = PlayState::Stopped;
    }
}

void AudioEngine::setPosition(double position)
{
    // Set position for Track A
    if (hasFileLoaded())
    {
        double durationA = transportSource.getLengthInSeconds();
        if (durationA > 0.0)
        {
            transportSource.setPosition(position * durationA);
        }
    }

    // Set position for Track B
    if (hasTrackBLoaded())
    {
        double durationB = transportSourceB.getLengthInSeconds();
        if (durationB > 0.0)
        {
            transportSourceB.setPosition(position * durationB);
        }
    }
}

void AudioEngine::setPositionSeconds(double seconds)
{
    // Set position in seconds for Track A
    if (hasFileLoaded())
    {
        double durationA = transportSource.getLengthInSeconds();
        seconds = juce::jlimit(0.0, durationA, seconds);
        transportSource.setPosition(seconds);
    }

    // Set position for Track B (proportionally)
    if (hasTrackBLoaded())
    {
        double durationA = hasFileLoaded() ? transportSource.getLengthInSeconds() : 1.0;
        double durationB = transportSourceB.getLengthInSeconds();
        if (durationA > 0.0 && durationB > 0.0)
        {
            double ratio = seconds / durationA;
            transportSourceB.setPosition(ratio * durationB);
        }
    }
}

//==============================================================================
// Loop/Range playback

void AudioEngine::setLoopEnabled(bool enabled)
{
    loopEnabled.store(enabled);
}

void AudioEngine::setLoopRange(double startSeconds, double endSeconds)
{
    // Ensure start < end
    if (startSeconds > endSeconds)
        std::swap(startSeconds, endSeconds);

    loopStartSeconds.store(startSeconds);
    loopEndSeconds.store(endSeconds);
}

//==============================================================================
double AudioEngine::getPosition() const
{
    if (!hasFileLoaded())
        return 0.0;

    double duration = getDuration();
    if (duration <= 0.0)
        return 0.0;

    return transportSource.getCurrentPosition() / duration;
}

double AudioEngine::getDuration() const
{
    if (!hasFileLoaded() || readerSource == nullptr)
        return 0.0;

    return transportSource.getLengthInSeconds();
}

//==============================================================================
AudioEngine::AudioLevels AudioEngine::calculateLevelsAtPosition(double position)
{
    AudioLevels levels;

    if (!hasFileLoaded() || readerSource == nullptr)
        return levels;

    // Get the audio reader
    auto* reader = readerSource->getAudioFormatReader();
    if (reader == nullptr)
        return levels;

    // Calculate sample position
    double duration = getDuration();
    if (duration <= 0.0)
        return levels;

    juce::int64 samplePos = (juce::int64)(position * duration * reader->sampleRate);
    samplePos = juce::jlimit((juce::int64)0, reader->lengthInSamples - 1, samplePos);

    // Read a small buffer around this position (2048 samples = ~46ms at 44.1kHz)
    const int bufferSize = 2048;
    juce::AudioBuffer<float> buffer(reader->numChannels, bufferSize);

    // Read audio data
    int numSamples = juce::jmin(bufferSize, (int)(reader->lengthInSamples - samplePos));
    if (!reader->read(&buffer, 0, numSamples, samplePos, true, true))
        return levels;

    // Calculate levels for left channel
    if (reader->numChannels >= 1)
    {
        const float* leftData = buffer.getReadPointer(0);
        float sumSquares = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float sample = std::abs(leftData[i]);
            levels.leftPeak = juce::jmax(levels.leftPeak, sample);
            sumSquares += leftData[i] * leftData[i];
        }

        levels.leftRMS = std::sqrt(sumSquares / numSamples);
    }

    // Calculate levels for right channel
    if (reader->numChannels >= 2)
    {
        const float* rightData = buffer.getReadPointer(1);
        float sumSquares = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float sample = std::abs(rightData[i]);
            levels.rightPeak = juce::jmax(levels.rightPeak, sample);
            sumSquares += rightData[i] * rightData[i];
        }

        levels.rightRMS = std::sqrt(sumSquares / numSamples);
    }
    else
    {
        // Mono - copy left to right
        levels.rightRMS = levels.leftRMS;
        levels.rightPeak = levels.leftPeak;
    }

    return levels;
}

//==============================================================================
juce::String AudioEngine::getCurrentDeviceName() const
{
    auto* device = deviceManager.getCurrentAudioDevice();
    return device != nullptr ? device->getName() : "No device";
}

double AudioEngine::getCurrentSampleRate() const
{
    auto* device = deviceManager.getCurrentAudioDevice();
    return device != nullptr ? device->getCurrentSampleRate() : 0.0;
}

int AudioEngine::getCurrentBufferSize() const
{
    auto* device = deviceManager.getCurrentAudioDevice();
    return device != nullptr ? device->getCurrentBufferSizeSamples() : 0;
}

//==============================================================================
void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                   int numInputChannels,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext& context)
{
    // Create buffer wrapper (no allocation, just wraps the raw pointers)
    juce::AudioBuffer<float> buffer(const_cast<float**>(outputChannelData), numOutputChannels, numSamples);

    // Clear output buffers first
    buffer.clear();

    // Create channel info for getting audio
    juce::AudioSourceChannelInfo channelInfo;
    channelInfo.buffer = &buffer;
    channelInfo.startSample = 0;
    channelInfo.numSamples = numSamples;

    // Check if we're using multi-track source
    if (multiTrackSource != nullptr && playState.load() == PlayState::Playing)
    {
        // Get audio from multi-track source
        multiTrackSource->getNextAudioBlock(channelInfo);
    }
    else
    {
        // Single file playback - handle dual track mode
        auto track = activeTrack.load();

        // Handle dual track playback based on active track setting
        if (track == ActiveTrack::A && hasFileLoaded())
        {
            // Track A only
            transportSource.getNextAudioBlock(channelInfo);
        }
        else if (track == ActiveTrack::B && hasTrackBLoaded())
        {
            // Track B only
            transportSourceB.getNextAudioBlock(channelInfo);
        }
        else if (track == ActiveTrack::Both)
        {
            // Mix both tracks
            float balance = trackMixBalance.load();
            float gainA = 1.0f - balance;  // 0.0 -> 1.0, 1.0 -> 0.0
            float gainB = balance;          // 0.0 -> 0.0, 1.0 -> 1.0

            // Get Track A audio
            if (hasFileLoaded())
            {
                transportSource.getNextAudioBlock(channelInfo);
                buffer.applyGain(gainA);
            }

            // Get Track B audio and mix
            if (hasTrackBLoaded())
            {
                juce::AudioBuffer<float> bufferB(numOutputChannels, numSamples);
                bufferB.clear();

                juce::AudioSourceChannelInfo channelInfoB;
                channelInfoB.buffer = &bufferB;
                channelInfoB.startSample = 0;
                channelInfoB.numSamples = numSamples;

                transportSourceB.getNextAudioBlock(channelInfoB);

                // Add Track B to output with gain
                for (int ch = 0; ch < numOutputChannels; ++ch)
                {
                    if (ch < bufferB.getNumChannels())
                    {
                        buffer.addFrom(ch, 0, bufferB, ch, 0, numSamples, gainB);
                    }
                }
            }
        }
        else if (hasFileLoaded())
        {
            // Default: play Track A if available
            transportSource.getNextAudioBlock(channelInfo);
        }
    }  // End of else block for single file playback

    // Check for loop point
    if (loopEnabled.load() && playState.load() == PlayState::Playing)
    {
        double loopEnd = loopEndSeconds.load();
        double loopStart = loopStartSeconds.load();

        if (loopEnd > loopStart)
        {
            double currentPos = transportSource.getCurrentPosition();

            if (currentPos >= loopEnd)
            {
                // Loop back to start
                juce::MessageManager::callAsync([this, loopStart]()
                {
                    if (hasFileLoaded())
                        transportSource.setPosition(loopStart);
                    if (hasTrackBLoaded())
                    {
                        double durationA = hasFileLoaded() ? transportSource.getLengthInSeconds() : 1.0;
                        double durationB = transportSourceB.getLengthInSeconds();
                        if (durationA > 0.0 && durationB > 0.0)
                        {
                            double ratio = loopStart / durationA;
                            transportSourceB.setPosition(ratio * durationB);
                        }
                    }
                });
            }
        }
    }

    // Get dry/wet mix amount
    float wetMix = dryWetMix.load();

    // Save dry (original) buffer for A/B comparison
    juce::AudioBuffer<float> dryBuffer;
    if (wetMix < 1.0f && audioProcessCallback)
    {
        dryBuffer.makeCopyOf(buffer);
    }

    // Apply audio processing (filters, EQ, VST plugins, etc.)
    if (audioProcessCallback)
    {
        audioProcessCallback(buffer);
    }

    // Mix dry and wet signals based on dryWetMix
    if (wetMix < 1.0f && dryBuffer.getNumChannels() > 0)
    {
        float dryMix = 1.0f - wetMix;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            if (ch < dryBuffer.getNumChannels())
            {
                // buffer = wet * wetMix + dry * dryMix
                buffer.applyGain(ch, 0, numSamples, wetMix);
                buffer.addFrom(ch, 0, dryBuffer, ch, 0, numSamples, dryMix);
            }
        }
    }

    // Apply master gain
    float gain = masterGain.load();
    if (gain != 1.0f)
    {
        buffer.applyGain(gain);
    }

    // Write to recording file if recording
    if (recordState.load() == RecordState::Recording)
    {
        const juce::ScopedLock sl(writerLock);
        if (recordingWriter != nullptr)
        {
            // If we have input channels, record from input; otherwise record output
            if (numInputChannels > 0 && inputChannelData != nullptr)
            {
                juce::AudioBuffer<float> inputBuffer(const_cast<float**>(inputChannelData), numInputChannels, numSamples);
                recordingWriter->write(inputChannelData, numSamples);
            }
            else
            {
                recordingWriter->write(outputChannelData, numSamples);
            }
        }
    }

    // Calculate levels for level meter and true peak meter
    float leftRMS = 0.0f;
    float leftPeak = 0.0f;
    float rightRMS = 0.0f;
    float rightPeak = 0.0f;

    // Calculate levels for level meter (if callback is set and playing)
    // Only calculate during playback to avoid overriding manual position-based calculations
    if (levelCallback && numOutputChannels > 0 && playState.load() == PlayState::Playing)
    {

        // Calculate left channel
        if (numOutputChannels >= 1)
        {
            const float* leftData = buffer.getReadPointer(0);
            float sumSquares = 0.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                float sample = std::abs(leftData[i]);
                leftPeak = juce::jmax(leftPeak, sample);
                sumSquares += leftData[i] * leftData[i];
            }

            leftRMS = std::sqrt(sumSquares / numSamples);
        }

        // Calculate right channel (or copy left if mono)
        if (numOutputChannels >= 2)
        {
            const float* rightData = buffer.getReadPointer(1);
            float sumSquares = 0.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                float sample = std::abs(rightData[i]);
                rightPeak = juce::jmax(rightPeak, sample);
                sumSquares += rightData[i] * rightData[i];
            }

            rightRMS = std::sqrt(sumSquares / numSamples);
        }
        else
        {
            // Mono - copy left to right
            rightRMS = leftRMS;
            rightPeak = leftPeak;
        }

        // Call level callback (thread-safe)
        levelCallback(leftRMS, leftPeak, rightRMS, rightPeak);
    }

    // Push samples to spectrum analyzer (if callback is set)
    if (spectrumCallback && numOutputChannels > 0 && playState.load() == PlayState::Playing)
    {
        // Push mono mix of left and right channels for spectrum analysis
        const float* leftData = buffer.getReadPointer(0);
        const float* rightData = numOutputChannels >= 2 ? buffer.getReadPointer(1) : leftData;

        for (int i = 0; i < numSamples; ++i)
        {
            float monoSample = (leftData[i] + rightData[i]) * 0.5f;
            spectrumCallback(monoSample);
        }
    }

    // Send true peak values (if callback is set)
    // Note: This is a simplified implementation. For true inter-sample peak detection,
    // oversampling should be used, but this provides a good approximation.
    if (truePeakCallback && numOutputChannels > 0 && playState.load() == PlayState::Playing)
    {
        truePeakCallback(leftPeak, rightPeak);
    }

    // Calculate and send phase correlation (if callback is set)
    if (phaseCorrelationCallback && numOutputChannels >= 2 && playState.load() == PlayState::Playing)
    {
        const float* leftData = buffer.getReadPointer(0);
        const float* rightData = buffer.getReadPointer(1);

        // Calculate correlation coefficient
        // correlation = sum(L*R) / sqrt(sum(L^2) * sum(R^2))
        double sumLR = 0.0;
        double sumLL = 0.0;
        double sumRR = 0.0;

        for (int i = 0; i < numSamples; ++i)
        {
            double L = leftData[i];
            double R = rightData[i];
            sumLR += L * R;
            sumLL += L * L;
            sumRR += R * R;
        }

        float correlation = 0.0f;
        double denominator = std::sqrt(sumLL * sumRR);
        if (denominator > 0.0)
        {
            correlation = (float)(sumLR / denominator);
        }

        phaseCorrelationCallback(correlation);
    }

    // Calculate and send loudness (if callback is set)
    if (loudnessCallback && numOutputChannels > 0 && playState.load() == PlayState::Playing)
    {
        // Initialize loudness buffer if needed
        if (loudnessBuffer.empty() && preparedSampleRate > 0)
        {
            int blockSamples = (int)(preparedSampleRate * BLOCK_SIZE_MS / 1000.0);
            int numBlocks = SHORT_TERM_BLOCKS + 10;  // Extra space for circular buffer
            loudnessBuffer.resize(numBlocks, -70.0f);
        }

        // Calculate mean square for this buffer (simplified without K-weighting)
        double meanSquare = 0.0;
        int sampleCount = 0;

        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            const float* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                double sample = channelData[i];
                meanSquare += sample * sample;
                sampleCount++;
            }
        }

        if (sampleCount > 0)
        {
            meanSquare /= sampleCount;

            // Convert to LUFS (simplified formula: LUFS = -0.691 + 10 * log10(MS))
            float blockLoudness = -70.0f;
            if (meanSquare > 0.0)
            {
                blockLoudness = -0.691f + 10.0f * std::log10((float)meanSquare);
            }

            // Store in circular buffer
            loudnessBuffer[loudnessBufferIndex] = blockLoudness;
            loudnessBufferIndex = (loudnessBufferIndex + 1) % loudnessBuffer.size();

            // Calculate momentary loudness (400ms = 4 blocks)
            float momentary = -70.0f;
            double momentarySum = 0.0;
            int momentaryCount = 0;
            for (int i = 0; i < MOMENTARY_BLOCKS && i < loudnessBuffer.size(); ++i)
            {
                int idx = (loudnessBufferIndex - 1 - i + loudnessBuffer.size()) % loudnessBuffer.size();
                if (loudnessBuffer[idx] > -69.0f)
                {
                    // Convert back to linear, sum, then convert to LUFS
                    double linear = std::pow(10.0, (loudnessBuffer[idx] + 0.691) / 10.0);
                    momentarySum += linear;
                    momentaryCount++;
                }
            }
            if (momentaryCount > 0)
            {
                momentary = -0.691f + 10.0f * std::log10((float)(momentarySum / momentaryCount));
            }

            // Calculate short-term loudness (3s = 30 blocks)
            float shortTerm = -70.0f;
            double shortTermSum = 0.0;
            int shortTermCount = 0;
            for (int i = 0; i < SHORT_TERM_BLOCKS && i < loudnessBuffer.size(); ++i)
            {
                int idx = (loudnessBufferIndex - 1 - i + loudnessBuffer.size()) % loudnessBuffer.size();
                if (loudnessBuffer[idx] > -69.0f)
                {
                    double linear = std::pow(10.0, (loudnessBuffer[idx] + 0.691) / 10.0);
                    shortTermSum += linear;
                    shortTermCount++;
                }
            }
            if (shortTermCount > 0)
            {
                shortTerm = -0.691f + 10.0f * std::log10((float)(shortTermSum / shortTermCount));
            }

            // Calculate integrated loudness (running average)
            sumSquaredSamples += meanSquare;
            totalSampleCount++;
            float integrated = -70.0f;
            if (totalSampleCount > 0)
            {
                double avgMeanSquare = sumSquaredSamples / totalSampleCount;
                if (avgMeanSquare > 0.0)
                {
                    integrated = -0.691f + 10.0f * std::log10((float)avgMeanSquare);
                }
            }

            // Calculate loudness range (simplified: difference between 95th and 10th percentile)
            float lra = 0.0f;
            if (loudnessBuffer.size() > 10)
            {
                std::vector<float> sortedBuffer = loudnessBuffer;
                std::sort(sortedBuffer.begin(), sortedBuffer.end());
                int idx10 = (int)(sortedBuffer.size() * 0.10f);
                int idx95 = (int)(sortedBuffer.size() * 0.95f);
                if (sortedBuffer[idx10] > -69.0f && sortedBuffer[idx95] > -69.0f)
                {
                    lra = sortedBuffer[idx95] - sortedBuffer[idx10];
                }
            }

            // Send loudness values
            loudnessCallback(integrated, shortTerm, momentary, lra);
        }
    }
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device == nullptr)
        return;

    preparedSampleRate = device->getCurrentSampleRate();
    preparedBlockSize = device->getCurrentBufferSizeSamples();

    prepareToPlay(preparedSampleRate, preparedBlockSize);

    // Notify listeners about device start (for preparing external processors)
    if (deviceStartedCallback)
        deviceStartedCallback(preparedSampleRate, preparedBlockSize);
}

void AudioEngine::audioDeviceStopped()
{
    releaseResources();
}

//==============================================================================
void AudioEngine::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    // Check if playback has finished for Track A
    if (source == &transportSource && playState.load() == PlayState::Playing)
    {
        auto track = activeTrack.load();
        bool trackAFinished = transportSource.hasStreamFinished();
        bool trackBFinished = !hasTrackBLoaded() || transportSourceB.hasStreamFinished();

        // Stop only if both relevant tracks have finished
        bool shouldStop = false;
        if (track == ActiveTrack::A && trackAFinished)
            shouldStop = true;
        else if (track == ActiveTrack::B && trackBFinished)
            shouldStop = true;
        else if (track == ActiveTrack::Both && trackAFinished && trackBFinished)
            shouldStop = true;

        if (shouldStop)
        {
            juce::MessageManager::callAsync([this]()
            {
                stop();
            });
        }
    }

    // Check if playback has finished for Track B
    if (source == &transportSourceB && playState.load() == PlayState::Playing)
    {
        auto track = activeTrack.load();
        bool trackAFinished = !hasFileLoaded() || transportSource.hasStreamFinished();
        bool trackBFinished = transportSourceB.hasStreamFinished();

        bool shouldStop = false;
        if (track == ActiveTrack::B && trackBFinished)
            shouldStop = true;
        else if (track == ActiveTrack::Both && trackAFinished && trackBFinished)
            shouldStop = true;

        if (shouldStop)
        {
            juce::MessageManager::callAsync([this]()
            {
                stop();
            });
        }
    }
}

//==============================================================================
void AudioEngine::showError(const juce::String& message)
{
    if (errorCallback)
    {
        juce::MessageManager::callAsync([this, message]()
        {
            errorCallback(message);
        });
    }
}

void AudioEngine::prepareToPlay(double sampleRate, int blockSize)
{
    // Prepare Track A
    transportSource.prepareToPlay(blockSize, sampleRate);
    if (resamplingSource != nullptr)
        resamplingSource->prepareToPlay(blockSize, sampleRate);

    // Prepare Track B
    transportSourceB.prepareToPlay(blockSize, sampleRate);
}

//==============================================================================
// Recording methods

bool AudioEngine::startRecording(const juce::File& outputFile)
{
    stopRecording();  // Stop any existing recording

    recordingFile = outputFile;

    // Ensure parent directory exists
    recordingFile.getParentDirectory().createDirectory();

    // Create WAV format writer
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> fileStream(new juce::FileOutputStream(recordingFile));

    if (!fileStream->openedOk())
    {
        showError("Failed to create recording file: " + recordingFile.getFullPathName());
        return false;
    }

    // Get current audio settings
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        showError("No audio device available for recording");
        return false;
    }

    double sampleRate = device->getCurrentSampleRate();
    int numChannels = device->getActiveInputChannels().countNumberOfSetBits();

    if (numChannels == 0)
    {
        numChannels = 2;  // Default to stereo if no input channels
    }

    // Create audio format writer
    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
        fileStream.release(),
        sampleRate,
        (juce::uint32)numChannels,
        16,  // 16-bit
        {},
        0
    ));

    if (writer == nullptr)
    {
        showError("Failed to create audio writer");
        return false;
    }

    // Start recording thread if not already running
    if (!recordingThread.isThreadRunning())
        recordingThread.startThread();

    // Create threaded writer for better performance
    {
        const juce::ScopedLock sl(writerLock);
        recordingWriter.reset(new juce::AudioFormatWriter::ThreadedWriter(writer.release(), recordingThread, 32768));
    }

    recordState.store(RecordState::Recording);
    return true;
}

void AudioEngine::stopRecording()
{
    if (recordState.load() == RecordState::Stopped)
        return;

    recordState.store(RecordState::Stopped);

    // Flush and close the writer
    {
        const juce::ScopedLock sl(writerLock);
        recordingWriter.reset();
    }
}

void AudioEngine::pauseRecording()
{
    if (recordState.load() == RecordState::Recording)
        recordState.store(RecordState::Paused);
}

void AudioEngine::resumeRecording()
{
    if (recordState.load() == RecordState::Paused)
        recordState.store(RecordState::Recording);
}

//==============================================================================

void AudioEngine::releaseResources()
{
    // Release Track A
    transportSource.releaseResources();
    if (resamplingSource != nullptr)
        resamplingSource->releaseResources();

    // Release Track B
    transportSourceB.releaseResources();
}

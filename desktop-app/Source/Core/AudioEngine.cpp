/*
  ==============================================================================

    AudioEngine.cpp

    Audio playback engine implementation

  ==============================================================================
*/

#include "AudioEngine.h"

//==============================================================================
AudioEngine::AudioEngine()
{
    // Register audio formats
    formatManager.registerBasicFormats();  // WAV, AIFF

    // Add transport source to mixer
    mixerSource.addInputSource(&transportSource, false);

    // Listen for transport state changes
    transportSource.addChangeListener(this);
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
void AudioEngine::play()
{
    if (!hasFileLoaded())
        return;

    auto currentState = playState.load();

    if (currentState == PlayState::Stopped)
    {
        transportSource.setPosition(0.0);
        transportSource.start();
        playState = PlayState::Playing;
    }
    else if (currentState == PlayState::Paused)
    {
        transportSource.start();
        playState = PlayState::Playing;
    }
}

void AudioEngine::pause()
{
    if (playState.load() == PlayState::Playing)
    {
        transportSource.stop();
        playState = PlayState::Paused;
    }
}

void AudioEngine::stop()
{
    if (playState.load() != PlayState::Stopped)
    {
        transportSource.stop();
        transportSource.setPosition(0.0);
        playState = PlayState::Stopped;
    }
}

void AudioEngine::setPosition(double position)
{
    if (!hasFileLoaded())
        return;

    double duration = getDuration();
    if (duration > 0.0)
    {
        transportSource.setPosition(position * duration);
    }
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

    // Get audio from mixer (which includes transport source)
    juce::AudioSourceChannelInfo channelInfo;
    channelInfo.buffer = &buffer;
    channelInfo.startSample = 0;
    channelInfo.numSamples = numSamples;

    mixerSource.getNextAudioBlock(channelInfo);

    // Calculate levels for level meter (if callback is set and playing)
    // Only calculate during playback to avoid overriding manual position-based calculations
    if (levelCallback && numOutputChannels > 0 && playState.load() == PlayState::Playing)
    {
        float leftRMS = 0.0f;
        float leftPeak = 0.0f;
        float rightRMS = 0.0f;
        float rightPeak = 0.0f;

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
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device == nullptr)
        return;

    preparedSampleRate = device->getCurrentSampleRate();
    preparedBlockSize = device->getCurrentBufferSizeSamples();

    prepareToPlay(preparedSampleRate, preparedBlockSize);
}

void AudioEngine::audioDeviceStopped()
{
    releaseResources();
}

//==============================================================================
void AudioEngine::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        // Check if playback has finished
        if (transportSource.hasStreamFinished() && playState.load() == PlayState::Playing)
        {
            // Automatically stop when finished
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
    mixerSource.prepareToPlay(blockSize, sampleRate);
    transportSource.prepareToPlay(blockSize, sampleRate);

    if (resamplingSource != nullptr)
        resamplingSource->prepareToPlay(blockSize, sampleRate);
}

void AudioEngine::releaseResources()
{
    transportSource.releaseResources();
    mixerSource.releaseResources();

    if (resamplingSource != nullptr)
        resamplingSource->releaseResources();
}

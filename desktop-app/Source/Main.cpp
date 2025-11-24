/*
  ==============================================================================

    Soundman Desktop - Main Entry Point

    Real-time Audio Analysis Tool with Audio Playback

  ==============================================================================
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Core/AudioEngine.h"
#include "UI/WaveformDisplay.h"
#include "UI/LevelMeter.h"

//==============================================================================
/**
    Main Application Window Component with Audio Playback
*/
class MainComponent : public juce::Component,
                      public juce::Timer
{
public:
    //==========================================================================
    MainComponent()
    {
        // Initialize Audio Engine
        if (!audioEngine.initialize())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Audio Error",
                "Failed to initialize audio system",
                "OK"
            );
        }

        // Set error callback
        audioEngine.setErrorCallback([this](const juce::String& error)
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Error",
                error,
                "OK"
            );
        });

        // Setup UI components
        addAndMakeVisible(loadButton);
        loadButton.setButtonText("Load Audio File");
        loadButton.onClick = [this]() { loadAudioFile(); };

        addAndMakeVisible(playButton);
        playButton.setButtonText("Play");
        playButton.onClick = [this]() { audioEngine.play(); };
        playButton.setEnabled(false);

        addAndMakeVisible(pauseButton);
        pauseButton.setButtonText("Pause");
        pauseButton.onClick = [this]() { audioEngine.pause(); };
        pauseButton.setEnabled(false);

        addAndMakeVisible(stopButton);
        stopButton.setButtonText("Stop");
        stopButton.onClick = [this]() { audioEngine.stop(); };
        stopButton.setEnabled(false);

        addAndMakeVisible(fileLabel);
        fileLabel.setText("No file loaded", juce::dontSendNotification);
        fileLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(statusLabel);
        statusLabel.setText("Status: Stopped", juce::dontSendNotification);
        statusLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(deviceInfoLabel);
        updateDeviceInfo();
        deviceInfoLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(waveformDisplay);
        waveformDisplay.setSeekCallback([this](double position)
        {
            audioEngine.setPosition(position);

            // Update level meter when seeking (always update, ignore throttle)
            updateLevelMeterAtPosition(position);
            lastLevelUpdatePosition = position;
        });

        addAndMakeVisible(levelMeter);

        // Set level callback from audio engine to level meter
        audioEngine.setLevelCallback([this](float leftRMS, float leftPeak, float rightRMS, float rightPeak)
        {
            levelMeter.setLevels(leftRMS, leftPeak, rightRMS, rightPeak);
        });

        setSize(800, 600);

        // Start timer for UI updates
        startTimer(100);  // 10 Hz
    }

    ~MainComponent() override
    {
        stopTimer();
        audioEngine.shutdown();
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2a2a2a));

        // Title
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(32.0f, juce::Font::bold));
        g.drawText("Soundman Desktop", getLocalBounds().removeFromTop(80),
                   juce::Justification::centred, true);

        // Version
        g.setFont(juce::Font(16.0f));
        g.drawText("Version 0.1.0 (Phase 1)", getLocalBounds().removeFromTop(120),
                   juce::Justification::centred, true);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(140);  // Skip title area

        auto buttonArea = bounds.removeFromTop(60).reduced(20);
        auto buttonWidth = buttonArea.getWidth() / 4 - 10;

        loadButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(10);
        playButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(10);
        pauseButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(10);
        stopButton.setBounds(buttonArea.removeFromLeft(buttonWidth));

        auto labelArea = bounds.reduced(20);
        fileLabel.setBounds(labelArea.removeFromTop(30));
        labelArea.removeFromTop(10);
        statusLabel.setBounds(labelArea.removeFromTop(30));
        labelArea.removeFromTop(10);
        deviceInfoLabel.setBounds(labelArea.removeFromTop(30));
        labelArea.removeFromTop(10);

        // Level meter on the right side
        auto meterBounds = labelArea.removeFromRight(120);
        levelMeter.setBounds(meterBounds);

        // Add spacing
        labelArea.removeFromRight(10);

        // Waveform display takes remaining space
        waveformDisplay.setBounds(labelArea);
    }

    void timerCallback() override
    {
        // Update UI based on audio engine state
        updateUI();
    }

private:
    //==========================================================================
    void loadAudioFile()
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select an audio file",
            juce::File(),
            "*.wav;*.mp3;*.aiff;*.flac"
        );

        auto chooserFlags = juce::FileBrowserComponent::openMode |
                            juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file == juce::File())
                return;

            if (audioEngine.loadFile(file))
            {
                fileLabel.setText("File: " + audioEngine.getCurrentFileName(),
                                juce::dontSendNotification);
                enablePlaybackButtons(true);

                // Load waveform display
                waveformDisplay.loadFile(file, audioEngine.getFormatManager());

                // Reset level update tracking
                lastLevelUpdatePosition = -1.0;

                // Update level meter at start position
                updateLevelMeterAtPosition(0.0);
                lastLevelUpdatePosition = 0.0;
            }
        });
    }

    void updateUI()
    {
        // Update status label
        auto state = audioEngine.getPlayState();
        juce::String statusText = "Status: ";

        switch (state)
        {
            case AudioEngine::PlayState::Stopped:
                statusText += "Stopped";
                break;
            case AudioEngine::PlayState::Playing:
                statusText += "Playing";
                break;
            case AudioEngine::PlayState::Paused:
                statusText += "Paused";
                break;
        }

        if (audioEngine.hasFileLoaded())
        {
            double position = audioEngine.getPosition();
            double duration = audioEngine.getDuration();
            statusText += juce::String::formatted(" - %.1f / %.1f seconds (%.0f%%)",
                                                  position * duration,
                                                  duration,
                                                  position * 100.0);
        }

        statusLabel.setText(statusText, juce::dontSendNotification);

        // Update button states
        bool hasFile = audioEngine.hasFileLoaded();
        playButton.setEnabled(hasFile && state != AudioEngine::PlayState::Playing);
        pauseButton.setEnabled(state == AudioEngine::PlayState::Playing);
        stopButton.setEnabled(state != AudioEngine::PlayState::Stopped);

        // Update waveform position
        if (hasFile)
        {
            double currentPos = audioEngine.getPosition();
            waveformDisplay.setPosition(currentPos);

            // Update level meter when not playing (stopped or paused)
            // Only update if position has changed significantly (avoid redundant disk reads)
            if (state != AudioEngine::PlayState::Playing)
            {
                if (std::abs(currentPos - lastLevelUpdatePosition) > 0.001)  // ~0.1% change
                {
                    updateLevelMeterAtPosition(currentPos);
                    lastLevelUpdatePosition = currentPos;
                }
            }
        }
    }

    void updateLevelMeterAtPosition(double position)
    {
        if (!audioEngine.hasFileLoaded())
            return;

        // Calculate levels at this position
        auto levels = audioEngine.calculateLevelsAtPosition(position);

        // Update level meter
        levelMeter.setLevels(levels.leftRMS, levels.leftPeak,
                            levels.rightRMS, levels.rightPeak);
    }

    void enablePlaybackButtons(bool enable)
    {
        playButton.setEnabled(enable);
        pauseButton.setEnabled(false);
        stopButton.setEnabled(false);
    }

    void updateDeviceInfo()
    {
        juce::String info = "Audio Device: " + audioEngine.getCurrentDeviceName();
        info += juce::String::formatted(" | %.1f kHz | Buffer: %d samples",
                                        audioEngine.getCurrentSampleRate() / 1000.0,
                                        audioEngine.getCurrentBufferSize());
        deviceInfoLabel.setText(info, juce::dontSendNotification);
    }

    //==========================================================================
    AudioEngine audioEngine;

    juce::TextButton loadButton;
    juce::TextButton playButton;
    juce::TextButton pauseButton;
    juce::TextButton stopButton;

    juce::Label fileLabel;
    juce::Label statusLabel;
    juce::Label deviceInfoLabel;

    WaveformDisplay waveformDisplay;
    LevelMeter levelMeter;

    std::unique_ptr<juce::FileChooser> fileChooser;

    double lastLevelUpdatePosition { -1.0 };  // Track last position for level updates

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

//==============================================================================
/**
    Main Application Class
*/
class SoundmanApplication : public juce::JUCEApplication
{
public:
    //==========================================================================
    SoundmanApplication() {}

    const juce::String getApplicationName() override       { return "Soundman"; }
    const juce::String getApplicationVersion() override    { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    //==========================================================================
    void initialise(const juce::String& commandLine) override
    {
        // Create and show the main window
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override
    {
        // Cleanup
        mainWindow = nullptr;
    }

    //==========================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        // Handle multiple instances if needed
    }

    //==========================================================================
    /**
        Main Window Class
    */
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name,
                           juce::Colour(0xff2a2a2a),
                           DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
           #else
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
           #endif

            setVisible(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(SoundmanApplication)

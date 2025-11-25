/*
  ==============================================================================

    Soundman Desktop - Main Entry Point

    Real-time Audio Analysis Tool with Modular UI Architecture

  ==============================================================================
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Core/AudioEngine.h"
#include "UI/WaveformDisplay.h"
#include "UI/LevelMeter.h"
#include "UI/PanelContainer.h"
#include "UI/TabbedDisplayArea.h"
#include "UI/DeviceControlPanel.h"
#include "UI/KeyboardHandler.h"
#include "UI/SpectrumDisplay.h"

//==============================================================================
/**
    Main Application Window Component with Modular UI
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
        setupPanels();
        setupDevicePanel();
        setupCenterDisplay();
        setupRightPanel();
        setupKeyboardShortcuts();

        // Set window size (professional size)
        setSize(1920, 1080);

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

        // Title bar
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(24.0f, juce::Font::bold));
        g.drawText("Soundman Desktop", getLocalBounds().removeFromTop(60),
                   juce::Justification::centred, true);

        // Version
        g.setFont(juce::Font(12.0f));
        g.setColour(juce::Colours::grey);
        g.drawText("Version 0.2.0 (Phase 2 - Modular UI)", getLocalBounds().removeFromTop(85),
                   juce::Justification::centred, true);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(100);  // Skip title area

        // Main panel container takes the rest
        mainPanelContainer.setBounds(bounds.reduced(0, 0).withTrimmedBottom(30));

        // Status bar at bottom
        statusBar.setBounds(bounds.removeFromBottom(30).reduced(10, 0));
    }

    void timerCallback() override
    {
        // Update UI based on audio engine state
        updateUI();
    }

private:
    //==========================================================================
    void setupPanels()
    {
        // Create main 3-column layout
        addAndMakeVisible(mainPanelContainer);

        // Left panel: Device controls
        mainPanelContainer.addPanel(&deviceControlPanel, 0.15, 220, 400, "Device");

        // Center panel: Tabbed display area
        mainPanelContainer.addPanel(&tabbedDisplay, 0.60, 600, -1, "Display");

        // Right panel: Level meters and analysis controls
        mainPanelContainer.addPanel(&rightPanelContainer, 0.25, 300, 500, "Analysis");

        // Status bar
        addAndMakeVisible(statusBar);
        statusBar.setText("Ready", juce::dontSendNotification);
        statusBar.setJustificationType(juce::Justification::centredLeft);
        statusBar.setFont(juce::Font(12.0f));
    }

    void setupDevicePanel()
    {
        // Setup device control panel callbacks
        deviceControlPanel.setLoadButtonCallback([this]() { loadAudioFile(); });
        deviceControlPanel.setPlayButtonCallback([this]() { audioEngine.play(); });
        deviceControlPanel.setPauseButtonCallback([this]() { audioEngine.pause(); });
        deviceControlPanel.setStopButtonCallback([this]() { audioEngine.stop(); });

        // Update device info
        deviceControlPanel.setDeviceName(audioEngine.getCurrentDeviceName());
        deviceControlPanel.setSampleRate(audioEngine.getCurrentSampleRate());
        deviceControlPanel.setBufferSize(audioEngine.getCurrentBufferSize());
    }

    void setupCenterDisplay()
    {
        // Add waveform display as the first tab
        tabbedDisplay.addTab("Waveform", &waveformDisplay);

        // Add spectrum display as the second tab
        tabbedDisplay.addTab("Spectrum", &spectrumDisplay);

        // Setup waveform display
        waveformDisplay.setSeekCallback([this](double position)
        {
            audioEngine.setPosition(position);
            updateLevelMeterAtPosition(position);
            lastLevelUpdatePosition = position;
        });

        // Setup spectrum analyzer callback from audio engine
        audioEngine.setSpectrumCallback([this](float sample)
        {
            spectrumDisplay.pushNextSampleIntoFifo(sample);
        });

        // Tab changed callback
        tabbedDisplay.setTabChangedCallback([this](int index, const juce::String& name)
        {
            DBG("Tab changed to: " + name);
        });
    }

    void setupRightPanel()
    {
        // Right panel is divided into level meter and analysis controls
        addAndMakeVisible(rightPanelContainer);

        // Add level meter at top
        rightPanelContainer.addPanel(&levelMeter, 0.70, 200, -1, "Levels");

        // Add placeholder for future analysis controls
        addAndMakeVisible(analysisControlsPlaceholder);
        analysisControlsPlaceholder.setColour(juce::Label::backgroundColourId,
                                             juce::Colour(0xff1a1a1a));
        analysisControlsPlaceholder.setText("Analysis Controls\n(Coming in Phase 3)",
                                           juce::dontSendNotification);
        analysisControlsPlaceholder.setJustificationType(juce::Justification::centred);
        rightPanelContainer.addPanel(&analysisControlsPlaceholder, 0.30, 100, -1, "Controls");

        // Set level callback from audio engine
        audioEngine.setLevelCallback([this](float leftRMS, float leftPeak, float rightRMS, float rightPeak)
        {
            levelMeter.setLevels(leftRMS, leftPeak, rightRMS, rightPeak);
        });
    }

    void setupKeyboardShortcuts()
    {
        // Register keyboard shortcuts
        keyboardHandler.registerCommand(
            juce::KeyPress(juce::KeyPress::spaceKey),
            [this]() { togglePlayPause(); }
        );

        keyboardHandler.registerCommand(
            juce::KeyPress('s', juce::ModifierKeys::noModifiers, 0),
            [this]() { audioEngine.stop(); }
        );

        keyboardHandler.registerCommand(
            juce::KeyPress('o', juce::ModifierKeys::commandModifier, 0),
            [this]() { loadAudioFile(); }
        );

        keyboardHandler.registerCommand(
            juce::KeyPress('1', juce::ModifierKeys::commandModifier, 0),
            [this]() { tabbedDisplay.setCurrentTab(0); }
        );

        keyboardHandler.registerCommand(
            juce::KeyPress('2', juce::ModifierKeys::commandModifier, 0),
            [this]() { tabbedDisplay.setCurrentTab(1); }
        );

        // Add keyboard listener to the main component
        addKeyListener(&keyboardHandler);
        setWantsKeyboardFocus(true);
    }

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
                deviceControlPanel.setLoadedFileName(audioEngine.getCurrentFileName());
                deviceControlPanel.setPlayButtonEnabled(true);

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

    void togglePlayPause()
    {
        auto state = audioEngine.getPlayState();

        if (state == AudioEngine::PlayState::Playing)
            audioEngine.pause();
        else
            audioEngine.play();
    }

    void updateUI()
    {
        // Update button states
        auto state = audioEngine.getPlayState();
        bool hasFile = audioEngine.hasFileLoaded();

        deviceControlPanel.setPlayButtonEnabled(hasFile && state != AudioEngine::PlayState::Playing);
        deviceControlPanel.setPauseButtonEnabled(state == AudioEngine::PlayState::Playing);
        deviceControlPanel.setStopButtonEnabled(state != AudioEngine::PlayState::Stopped);

        // Update status bar
        juce::String statusText;

        switch (state)
        {
            case AudioEngine::PlayState::Stopped:
                statusText = "Stopped";
                break;
            case AudioEngine::PlayState::Playing:
                statusText = "Playing";
                break;
            case AudioEngine::PlayState::Paused:
                statusText = "Paused";
                break;
        }

        if (hasFile)
        {
            double position = audioEngine.getPosition();
            double duration = audioEngine.getDuration();
            statusText += juce::String::formatted(" | %.1f / %.1f s (%.0f%%) | %s | %.1f kHz | %d samples",
                                                  position * duration,
                                                  duration,
                                                  position * 100.0,
                                                  audioEngine.getCurrentDeviceName().toRawUTF8(),
                                                  audioEngine.getCurrentSampleRate() / 1000.0,
                                                  audioEngine.getCurrentBufferSize());
        }

        statusBar.setText(statusText, juce::dontSendNotification);

        // Update waveform position
        if (hasFile)
        {
            double currentPos = audioEngine.getPosition();
            waveformDisplay.setPosition(currentPos);

            // Update level meter when not playing
            if (state != AudioEngine::PlayState::Playing)
            {
                if (std::abs(currentPos - lastLevelUpdatePosition) > 0.001)
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

        auto levels = audioEngine.calculateLevelsAtPosition(position);
        levelMeter.setLevels(levels.leftRMS, levels.leftPeak,
                            levels.rightRMS, levels.rightPeak);
    }

    //==========================================================================
    AudioEngine audioEngine;

    // UI Components
    PanelContainer mainPanelContainer { PanelContainer::Orientation::Horizontal };
    PanelContainer rightPanelContainer { PanelContainer::Orientation::Vertical };

    DeviceControlPanel deviceControlPanel;
    TabbedDisplayArea tabbedDisplay;
    WaveformDisplay waveformDisplay;
    SpectrumDisplay spectrumDisplay;
    LevelMeter levelMeter;
    juce::Label analysisControlsPlaceholder;
    juce::Label statusBar;

    KeyboardHandler keyboardHandler;

    std::unique_ptr<juce::FileChooser> fileChooser;
    double lastLevelUpdatePosition { -1.0 };

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
    const juce::String getApplicationVersion() override    { return "0.2.0"; }
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

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
#include "UI/TruePeakMeter.h"
#include "UI/PhaseMeter.h"
#include "UI/LoudnessMeter.h"
#include "UI/SettingsDialog.h"
#include "UI/FileInfoPanel.h"
#include "UI/RecordingPanel.h"
#include "UI/PlaylistPanel.h"
#include "UI/VectorscopeDisplay.h"
#include "UI/HistogramDisplay.h"
#include "UI/MultiViewContainer.h"
#include "UI/TimecodeDisplay.h"
#include "UI/MasterGainControl.h"

//==============================================================================
/**
    Main Application Window Component with Modular UI
*/
class MainComponent : public juce::Component,
                      public juce::Timer,
                      public juce::MenuBarModel
{
public:
    //==========================================================================
    // Menu IDs
    enum MenuItemIDs
    {
        // File menu
        fileOpen = 1,
        fileSettings,
        fileExit,

        // View menu
        viewWaveform = 100,
        viewSpectrum,
        viewResetZoom,
        viewFullScreen,

        // Playback menu
        playbackPlayPause = 200,
        playbackStop,
        playbackSkipToStart,
        playbackSkipToEnd,

        // Help menu
        helpAbout = 300,
        helpKeyboardShortcuts,
        helpGitHub
    };
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
        setupRecordingPanel();
        setupPlaylistPanel();
        setupKeyboardShortcuts();
        setupMenuBar();

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

        // Left panel: Device controls, file info, recording, and playlist
        addAndMakeVisible(leftPanelContainer);
        leftPanelContainer.addPanel(&deviceControlPanel, 0.25, 200, -1, "Device");
        leftPanelContainer.addPanel(&fileInfoPanel, 0.25, 250, -1, "File Info");
        leftPanelContainer.addPanel(&recordingPanel, 0.25, 250, -1, "Recording");
        leftPanelContainer.addPanel(&playlistPanel, 0.25, 250, -1, "Playlist");
        mainPanelContainer.addPanel(&leftPanelContainer, 0.15, 220, 400, "Control");

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

        // Add vectorscope display
        tabbedDisplay.addTab("Vectorscope", &vectorscopeDisplay);

        // Add histogram display
        tabbedDisplay.addTab("Histogram", &histogramDisplay);

        // Add multi-view container
        tabbedDisplay.addTab("Multi-View", &multiViewContainer);

        // Add timecode display
        tabbedDisplay.addTab("Timecode", &timecodeDisplay);

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
            histogramDisplay.pushSample(sample);

            // Feed to multi-view container components
            if (auto* spectrum = multiViewContainer.getSpectrumDisplay())
                spectrum->pushNextSampleIntoFifo(sample);
            if (auto* histogram = multiViewContainer.getHistogramDisplay())
                histogram->pushSample(sample);
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
        rightPanelContainer.addPanel(&levelMeter, 0.25, 200, -1, "Levels");

        // Add true peak meter
        rightPanelContainer.addPanel(&truePeakMeter, 0.25, 150, -1, "True Peak");

        // Add phase meter
        rightPanelContainer.addPanel(&phaseMeter, 0.25, 150, -1, "Phase");

        // Add loudness meter
        rightPanelContainer.addPanel(&loudnessMeter, 0.25, 200, -1, "Loudness");

        // Add master gain control
        rightPanelContainer.addPanel(&masterGainControl, 0.0, 180, 180, "Master Gain");

        // Set master gain callback
        masterGainControl.onGainChanged = [this](float gainLinear)
        {
            audioEngine.setMasterGain(gainLinear);
        };

        // Set level callback from audio engine
        audioEngine.setLevelCallback([this](float leftRMS, float leftPeak, float rightRMS, float rightPeak)
        {
            levelMeter.setLevels(leftRMS, leftPeak, rightRMS, rightPeak);

            // Feed to vectorscope (using peak values as samples)
            vectorscopeDisplay.pushSample(leftPeak, rightPeak);

            // Feed to multi-view container vectorscope
            if (auto* vectorscope = multiViewContainer.getVectorscopeDisplay())
                vectorscope->pushSample(leftPeak, rightPeak);
        });

        // Set true peak callback from audio engine
        audioEngine.setTruePeakCallback([this](float leftPeak, float rightPeak)
        {
            truePeakMeter.setTruePeaks(leftPeak, rightPeak);
        });

        // Set phase correlation callback from audio engine
        audioEngine.setPhaseCorrelationCallback([this](float correlation)
        {
            phaseMeter.setCorrelation(correlation);
        });

        // Set loudness callback from audio engine
        audioEngine.setLoudnessCallback([this](float integrated, float shortTerm, float momentary, float lra)
        {
            loudnessMeter.setIntegratedLoudness(integrated);
            loudnessMeter.setShortTermLoudness(shortTerm);
            loudnessMeter.setMomentaryLoudness(momentary);
            loudnessMeter.setLoudnessRange(lra);
        });
    }

    void setupRecordingPanel()
    {
        // Set input device info
        recordingPanel.setInputDevice(audioEngine.getCurrentDeviceName());

        // Recording button callbacks (placeholders for now)
        recordingPanel.setRecordCallback([this]()
        {
            // TODO: Implement recording functionality
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Recording",
                "Recording functionality will be implemented in a future update",
                "OK"
            );
        });

        recordingPanel.setStopCallback([this]()
        {
            recordingPanel.setRecordingState(RecordingPanel::RecordingState::Stopped);
        });

        recordingPanel.setPauseCallback([this]()
        {
            auto state = recordingPanel.getRecordingState();
            if (state == RecordingPanel::RecordingState::Recording)
                recordingPanel.setRecordingState(RecordingPanel::RecordingState::Paused);
            else if (state == RecordingPanel::RecordingState::Paused)
                recordingPanel.setRecordingState(RecordingPanel::RecordingState::Recording);
        });
    }

    void setupPlaylistPanel()
    {
        // File selected callback
        playlistPanel.setFileSelectedCallback([this](const juce::File& file)
        {
            if (audioEngine.loadFile(file))
            {
                deviceControlPanel.setLoadedFileName(audioEngine.getCurrentFileName());
                deviceControlPanel.setPlayButtonEnabled(true);

                // Load waveform display
                waveformDisplay.loadFile(file, audioEngine.getFormatManager());

                // Update file info panel
                auto* reader = audioEngine.getFormatManager().createReaderFor(file);
                if (reader != nullptr)
                {
                    fileInfoPanel.setFileInfo(file, reader);
                    delete reader;
                }

                // Reset level update tracking
                lastLevelUpdatePosition = -1.0;

                // Update level meter at start position
                updateLevelMeterAtPosition(0.0);
                lastLevelUpdatePosition = 0.0;

                // Auto-play if enabled
                audioEngine.play();
            }
        });

        playlistPanel.setPlaylistChangedCallback([this]()
        {
            // Update UI when playlist changes
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
            juce::KeyPress(',', juce::ModifierKeys::commandModifier, 0),
            [this]() { showSettings(); }
        );

        keyboardHandler.registerCommand(
            juce::KeyPress('1', juce::ModifierKeys::commandModifier, 0),
            [this]() { tabbedDisplay.setCurrentTab(0); }
        );

        keyboardHandler.registerCommand(
            juce::KeyPress('2', juce::ModifierKeys::commandModifier, 0),
            [this]() { tabbedDisplay.setCurrentTab(1); }
        );

        keyboardHandler.registerCommand(
            juce::KeyPress(juce::KeyPress::homeKey),
            [this]() { audioEngine.setPosition(0.0); }
        );

        keyboardHandler.registerCommand(
            juce::KeyPress(juce::KeyPress::endKey),
            [this]() { audioEngine.setPosition(1.0); }
        );

        keyboardHandler.registerCommand(
            juce::KeyPress(juce::KeyPress::F11Key),
            [this]() { toggleFullScreen(); }
        );

        // Add keyboard listener to the main component
        addKeyListener(&keyboardHandler);
        setWantsKeyboardFocus(true);
    }

    void setupMenuBar()
    {
        // Menu bar is set up in the MainWindow class
        // This component provides the MenuBarModel implementation
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

                // Update file info panel
                auto* reader = audioEngine.getFormatManager().createReaderFor(file);
                if (reader != nullptr)
                {
                    fileInfoPanel.setFileInfo(file, reader);
                    delete reader;
                }

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

    void showSettings()
    {
        auto* settingsDialog = new SettingsDialog(audioEngine.getDeviceManager());

        settingsDialog->setSettingsChangedCallback([this]()
        {
            // Update device info display
            deviceControlPanel.setDeviceName(audioEngine.getCurrentDeviceName());
            deviceControlPanel.setSampleRate(audioEngine.getCurrentSampleRate());
            deviceControlPanel.setBufferSize(audioEngine.getCurrentBufferSize());
        });

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(settingsDialog);
        options.dialogTitle = "Settings";
        options.dialogBackgroundColour = juce::Colour(0xff1e1e1e);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;

        options.launchAsync();
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
    // MenuBarModel implementation
    juce::StringArray getMenuBarNames() override
    {
        return { "File", "View", "Playback", "Help" };
    }

    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& /*menuName*/) override
    {
        juce::PopupMenu menu;

        if (menuIndex == 0)  // File menu
        {
            menu.addItem(fileOpen, "Open Audio File...     Cmd+O");
            menu.addSeparator();
            menu.addItem(fileSettings, "Settings...     Cmd+,");
            menu.addSeparator();
            menu.addItem(fileExit, "Exit     Cmd+Q");
        }
        else if (menuIndex == 1)  // View menu
        {
            menu.addItem(viewWaveform, "Waveform     Cmd+1", true, tabbedDisplay.getCurrentTabIndex() == 0);
            menu.addItem(viewSpectrum, "Spectrum     Cmd+2", true, tabbedDisplay.getCurrentTabIndex() == 1);
            menu.addSeparator();
            menu.addItem(viewResetZoom, "Reset Zoom", audioEngine.hasFileLoaded());
            menu.addSeparator();
            menu.addItem(viewFullScreen, "Toggle Full Screen     F11");
        }
        else if (menuIndex == 2)  // Playback menu
        {
            bool hasFile = audioEngine.hasFileLoaded();
            bool isPlaying = audioEngine.isPlaying();

            menu.addItem(playbackPlayPause, (isPlaying ? "Pause" : "Play") + juce::String("     Space"), hasFile);
            menu.addItem(playbackStop, "Stop     S", hasFile);
            menu.addSeparator();
            menu.addItem(playbackSkipToStart, "Skip to Start     Home", hasFile);
            menu.addItem(playbackSkipToEnd, "Skip to End     End", hasFile);
        }
        else if (menuIndex == 3)  // Help menu
        {
            menu.addItem(helpAbout, "About Soundman...");
            menu.addItem(helpKeyboardShortcuts, "Keyboard Shortcuts...");
            menu.addSeparator();
            menu.addItem(helpGitHub, "Visit GitHub Repository...");
        }

        return menu;
    }

    void menuItemSelected(int menuItemID, int /*menuIndex*/) override
    {
        switch (menuItemID)
        {
            case fileOpen:
                loadAudioFile();
                break;

            case fileSettings:
                showSettings();
                break;

            case fileExit:
                juce::JUCEApplication::getInstance()->systemRequestedQuit();
                break;

            case viewWaveform:
                tabbedDisplay.setCurrentTab(0);
                break;

            case viewSpectrum:
                tabbedDisplay.setCurrentTab(1);
                break;

            case viewResetZoom:
                waveformDisplay.resetZoom();
                break;

            case viewFullScreen:
                toggleFullScreen();
                break;

            case playbackPlayPause:
                togglePlayPause();
                break;

            case playbackStop:
                audioEngine.stop();
                break;

            case playbackSkipToStart:
                audioEngine.setPosition(0.0);
                break;

            case playbackSkipToEnd:
                audioEngine.setPosition(1.0);
                break;

            case helpAbout:
                showAboutDialog();
                break;

            case helpKeyboardShortcuts:
                showKeyboardShortcuts();
                break;

            case helpGitHub:
                juce::URL("https://github.com/yyamada722/soundman").launchInDefaultBrowser();
                break;

            default:
                break;
        }
    }

    void toggleFullScreen()
    {
        auto* window = getTopLevelComponent();
        if (window != nullptr)
        {
            if (auto* docWindow = dynamic_cast<juce::DocumentWindow*>(window))
            {
                docWindow->setFullScreen(!docWindow->isFullScreen());
            }
        }
    }

    void showAboutDialog()
    {
        juce::String aboutText =
            "Soundman Desktop v0.2.0\n\n"
            "Real-time Audio Analysis Tool\n\n"
            "Features:\n"
            "• Waveform display with zoom/pan\n"
            "• Real-time spectrum analyzer\n"
            "• Level meters\n"
            "• Professional modular UI\n\n"
            "Built with JUCE Framework\n"
            "© 2024 Soundman Project";

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "About Soundman",
            aboutText,
            "OK"
        );
    }

    void showKeyboardShortcuts()
    {
        juce::String shortcutsText =
            "File:\n"
            "  Cmd+O - Open Audio File\n"
            "  Cmd+Q - Exit\n\n"
            "View:\n"
            "  Cmd+1 - Waveform Display\n"
            "  Cmd+2 - Spectrum Analyzer\n"
            "  F11 - Toggle Full Screen\n\n"
            "Playback:\n"
            "  Space - Play/Pause\n"
            "  S - Stop\n"
            "  Home - Skip to Start\n"
            "  End - Skip to End\n\n"
            "Waveform:\n"
            "  Mouse Wheel - Zoom In/Out\n"
            "  Ctrl+Drag - Pan\n"
            "  Click - Seek";

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Keyboard Shortcuts",
            shortcutsText,
            "OK"
        );
    }

    //==========================================================================
    AudioEngine audioEngine;

    // UI Components
    PanelContainer mainPanelContainer { PanelContainer::Orientation::Horizontal };
    PanelContainer leftPanelContainer { PanelContainer::Orientation::Vertical };
    PanelContainer rightPanelContainer { PanelContainer::Orientation::Vertical };

    DeviceControlPanel deviceControlPanel;
    FileInfoPanel fileInfoPanel;
    RecordingPanel recordingPanel;
    PlaylistPanel playlistPanel;
    TabbedDisplayArea tabbedDisplay;
    WaveformDisplay waveformDisplay;
    SpectrumDisplay spectrumDisplay;
    VectorscopeDisplay vectorscopeDisplay;
    HistogramDisplay histogramDisplay;
    MultiViewContainer multiViewContainer;
    TimecodeDisplay timecodeDisplay;
    MasterGainControl masterGainControl;
    LevelMeter levelMeter;
    TruePeakMeter truePeakMeter;
    PhaseMeter phaseMeter;
    LoudnessMeter loudnessMeter;
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

            auto* mainComp = new MainComponent();
            setContentOwned(mainComp, true);

            // Set up menu bar
           #if JUCE_MAC
            setMenuBar(mainComp);
           #else
            setMenuBar(mainComp, 24);  // 24 pixel height menu bar
           #endif

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
           #else
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
           #endif

            setVisible(true);
        }

        ~MainWindow()
        {
            // Clear menu bar before destroying content
            setMenuBar(nullptr);
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

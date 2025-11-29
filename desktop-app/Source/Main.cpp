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
#include "UI/SpectrumPanel.h"
#include "UI/AnalysisPanel.h"
#include "UI/ToolsPanel.h"
#include "UI/MetersPanel.h"
#include "UI/TruePeakMeter.h"
#include "UI/PhaseMeter.h"
#include "UI/LoudnessMeter.h"
#include "UI/SettingsDialog.h"
#include "UI/FileInfoPanel.h"
#include "UI/RecordingPanel.h"
#include "UI/PlaylistPanel.h"
#include "UI/MultiViewContainer.h"
#include "UI/TimecodeDisplay.h"
#include "UI/MasterGainControl.h"
#include "UI/PluginHostPanel.h"
#include "UI/ABCompareControl.h"
#include "UI/TrackComparePanel.h"
#include "UI/TransportControlPanel.h"
#include "UI/AudioTimeline.h"
#include "UI/MarkerPanel.h"
#include "UI/TopInfoBar.h"
#include "UI/TimelinePanel.h"
#include "UI/MixerPanel.h"
#include "Core/ProjectManager.h"
#include "Core/MultiTrackAudioSource.h"

//==============================================================================
/**
    Custom LookAndFeel with Japanese font support
*/
class JapaneseLookAndFeel : public juce::LookAndFeel_V4
{
public:
    JapaneseLookAndFeel()
    {
        // Find a Japanese font
        juce::StringArray fontNames = juce::Font::findAllTypefaceNames();

        const char* japaneseFonts[] = {
            "Meiryo UI", "Meiryo", "Yu Gothic UI", "Yu Gothic",
            "MS UI Gothic", "MS Gothic", "MS PGothic", nullptr
        };

        for (int i = 0; japaneseFonts[i] != nullptr; ++i)
        {
            if (fontNames.contains(japaneseFonts[i]))
            {
                japaneseFontName = japaneseFonts[i];
                break;
            }
        }

        if (japaneseFontName.isEmpty())
            japaneseFontName = juce::Font::getDefaultSansSerifFontName();

        // Set default typeface
        setDefaultSansSerifTypeface(juce::Typeface::createSystemTypefaceFor(
            juce::Font(japaneseFontName, 12.0f, juce::Font::plain)));
    }

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& font) override
    {
        // Always use Japanese font for any font request
        juce::Font japaneseFont(japaneseFontName, font.getHeight(), font.getStyleFlags());
        return juce::Typeface::createSystemTypefaceFor(japaneseFont);
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        return juce::Font(japaneseFontName, label.getFont().getHeight(), label.getFont().getStyleFlags());
    }

private:
    juce::String japaneseFontName;
};

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
        fileAddToTrack,
        fileNewProject,
        fileAddTrack,
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
        // Apply Japanese LookAndFeel for proper Japanese text rendering
        setLookAndFeel(&japaneseLookAndFeel);
        juce::LookAndFeel::setDefaultLookAndFeel(&japaneseLookAndFeel);

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
        setupTransportPanel();
        setupAudioTimeline();
        setupMarkerPanel();
        setupCenterDisplay();
        setupRightPanel();
        setupRecordingPanel();
        setupPlaylistPanel();
        setupKeyboardShortcuts();
        setupMenuBar();
        setupMultiTrackComponents();

        // Set window size (professional size)
        setSize(1920, 1080);

        // Start timer for UI updates
        startTimer(100);  // 10 Hz
    }

    ~MainComponent() override
    {
        stopTimer();
        audioEngine.shutdown();
        setLookAndFeel(nullptr);
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2a2a2a));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // TopInfoBar at the very top (50 pixels high)
        topInfoBar.setBounds(bounds.removeFromTop(50));

        // Status bar at bottom
        auto statusBounds = bounds.removeFromBottom(24);
        statusBar.setBounds(statusBounds.reduced(10, 0));

        // Main panel container takes the rest
        mainPanelContainer.setBounds(bounds);
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
        // Top info bar (Logic/Pro Tools style)
        addAndMakeVisible(topInfoBar);
        setupTopInfoBar();

        // Create main 3-column layout
        addAndMakeVisible(mainPanelContainer);

        // Left panel: Device controls, markers, recording, and playlist
        // (Transport is now in TopInfoBar)
        addAndMakeVisible(leftPanelContainer);
        leftPanelContainer.addPanel(&deviceControlPanel, 0.15, 100, -1, "Device");
        leftPanelContainer.addPanel(&markerPanel, 0.30, 180, -1, "Markers");
        leftPanelContainer.addPanel(&recordingPanel, 0.15, 100, -1, "Recording");
        leftPanelContainer.addPanel(&playlistPanel, 0.40, 200, -1, "Playlist");
        mainPanelContainer.addPanel(&leftPanelContainer, 0.18, 280, 450, "Control");

        // Center panel: Tabbed display area
        mainPanelContainer.addPanel(&tabbedDisplay, 0.60, 600, -1, "Display");

        // Right panel: Level meters and analysis controls
        mainPanelContainer.addPanel(&rightPanelContainer, 0.25, 300, 500, "Analysis");

        // Status bar - explicitly set Japanese font
        addAndMakeVisible(statusBar);
        statusBar.setText("Ready", juce::dontSendNotification);
        statusBar.setJustificationType(juce::Justification::centredLeft);
        statusBar.setFont(japaneseLookAndFeel.getLabelFont(statusBar));
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

    void setupTransportPanel()
    {
        // Set initial sample rate
        transportControlPanel.setSampleRate(audioEngine.getCurrentSampleRate());

        // Seek to time callback
        transportControlPanel.onSeekToTime = [this](double seconds)
        {
            audioEngine.setPositionSeconds(seconds);
            updateLevelMeterAtPosition(seconds / audioEngine.getDuration());
        };

        // Loop enabled callback
        transportControlPanel.onLoopEnabledChanged = [this](bool enabled)
        {
            audioEngine.setLoopEnabled(enabled);
            audioTimeline.setLoopEnabled(enabled);
        };

        // Loop range callback
        transportControlPanel.onLoopRangeChanged = [this](double startSeconds, double endSeconds)
        {
            audioEngine.setLoopRange(startSeconds, endSeconds);
            audioTimeline.setLoopRegion(startSeconds, endSeconds);
        };
    }

    void setupAudioTimeline()
    {
        // Position changed callback
        audioTimeline.onPositionChanged = [this](double position)
        {
            audioEngine.setPosition(position);
            updateLevelMeterAtPosition(position);
        };

        // Selection changed callback
        audioTimeline.onSelectionChanged = [this](double startSeconds, double endSeconds)
        {
            // Update transport panel with selection
            DBG("Selection: " + juce::String(startSeconds, 2) + " - " + juce::String(endSeconds, 2));
        };

        // Loop region changed callback
        audioTimeline.onLoopRegionChanged = [this](double startSeconds, double endSeconds)
        {
            audioEngine.setLoopRange(startSeconds, endSeconds);
            transportControlPanel.setLoopRange(startSeconds, endSeconds);
        };

        // Marker clicked callback
        audioTimeline.onMarkerClicked = [this](int markerId)
        {
            audioTimeline.jumpToMarker(markerId);
        };

        // Marker added from timeline callback
        audioTimeline.onMarkerAdded = [this](int id, double timeSeconds, const juce::String& name)
        {
            markerPanel.addMarker(id, name, timeSeconds);
        };
    }

    void setupMarkerPanel()
    {
        // Set duration when file is loaded (handled in loadFile callbacks)

        // Get current position callback
        markerPanel.onGetCurrentPosition = [this]() -> double
        {
            return audioEngine.getPosition() * audioEngine.getDuration();
        };

        // Add marker callback - sync to timeline with same ID
        markerPanel.onAddMarker = [this](int id, double timeSeconds, const juce::String& name)
        {
            audioTimeline.addMarkerWithId(id, timeSeconds, name, juce::Colours::yellow);
        };

        // Remove marker callback - sync to timeline
        markerPanel.onRemoveMarker = [this](int id)
        {
            audioTimeline.removeMarker(id);
        };

        // Marker changed callback - sync to timeline
        markerPanel.onMarkerChanged = [this](int id, const juce::String& name, double timeSeconds)
        {
            audioTimeline.updateMarker(id, name, timeSeconds);
        };

        // Jump to marker callback
        markerPanel.onJumpToMarker = [this](int id)
        {
            audioTimeline.jumpToMarker(id);
        };
    }

    void setupCenterDisplay()
    {
        // Organized tabs: 15 tabs -> 5 main categories
        // 1. Waveform - main view
        tabbedDisplay.addTab("Waveform", &waveformDisplay);

        // 2. Spectrum - contains Spectrum + Spectrogram
        tabbedDisplay.addTab("Spectrum", &spectrumPanel);

        // 3. Analysis - contains Pitch, BPM, Key, Harmonics, MFCC
        tabbedDisplay.addTab("Analysis", &analysisPanel);

        // 4. Meters - contains Vectorscope, Histogram
        tabbedDisplay.addTab("Meters", &metersPanel);

        // 5. Tools - contains Filter/EQ, Generator, IR/FR
        tabbedDisplay.addTab("Tools", &toolsPanel);

        // 6. Plugins - VST3 host panel
        tabbedDisplay.addTab("Plugins", &pluginHostPanel);

        // 7. Compare - Dual track comparison
        tabbedDisplay.addTab("Compare", &trackComparePanel);

        // 8. Timeline - Professional audio timeline
        tabbedDisplay.addTab("Timeline", &audioTimeline);

        // Setup track compare panel
        trackComparePanel.setFormatManager(&audioEngine.getFormatManager());

        // Connect TrackComparePanel to AudioEngine for dual-track playback
        trackComparePanel.onTrackLoaded = [this](const juce::File& file, TrackComparePanel::ActiveTrack track)
        {
            if (track == TrackComparePanel::ActiveTrack::A)
            {
                // Track A loaded via compare panel - also load to main audio engine Track A
                audioEngine.loadFile(file);
                deviceControlPanel.setLoadedFileName(audioEngine.getCurrentFileName());
                deviceControlPanel.setPlayButtonEnabled(true);
                waveformDisplay.loadFile(file, audioEngine.getFormatManager());

                // Update file info panel
                auto* reader = audioEngine.getFormatManager().createReaderFor(file);
                if (reader != nullptr)
                {
                    fileInfoPanel.setFileInfo(file, reader);
                    delete reader;
                }
            }
            else if (track == TrackComparePanel::ActiveTrack::B)
            {
                // Track B loaded - load to audio engine Track B
                audioEngine.loadTrackB(file);
            }
        };

        trackComparePanel.onActiveTrackChanged = [this](TrackComparePanel::ActiveTrack track)
        {
            // Convert TrackComparePanel::ActiveTrack to AudioEngine::ActiveTrack
            AudioEngine::ActiveTrack engineTrack = AudioEngine::ActiveTrack::A;
            switch (track)
            {
                case TrackComparePanel::ActiveTrack::A:
                    engineTrack = AudioEngine::ActiveTrack::A;
                    break;
                case TrackComparePanel::ActiveTrack::B:
                    engineTrack = AudioEngine::ActiveTrack::B;
                    break;
                case TrackComparePanel::ActiveTrack::Both:
                    engineTrack = AudioEngine::ActiveTrack::Both;
                    break;
            }
            audioEngine.setActiveTrack(engineTrack);
        };

        trackComparePanel.onMixBalanceChanged = [this](float balance)
        {
            audioEngine.setTrackMixBalance(balance);
        };

        trackComparePanel.onSeek = [this](double position)
        {
            audioEngine.setPosition(position);
        };

        // Prepare all panels
        double sampleRate = audioEngine.getCurrentSampleRate();
        int bufferSize = audioEngine.getCurrentBufferSize();

        analysisPanel.prepare(sampleRate, bufferSize);
        toolsPanel.prepare(sampleRate, bufferSize);
        pluginHostPanel.prepare(sampleRate, bufferSize);

        // Set audio processing callback
        audioEngine.setAudioProcessCallback([this](juce::AudioBuffer<float>& buffer)
        {
            auto& filterPanel = toolsPanel.getFilterPanel();
            auto& generatorPanel = toolsPanel.getGeneratorPanel();
            auto& responsePanel = toolsPanel.getResponseAnalyzerPanel();

            // Apply filters and EQ
            filterPanel.getFilter().process(buffer);
            filterPanel.getEQ().process(buffer);

            // Process through VST3 effect chain
            juce::MidiBuffer midiBuffer;
            pluginHostPanel.getEffectChain().processBlock(buffer, midiBuffer);

            // Generate test signals
            generatorPanel.processAudio(buffer);

            // Push samples to THD analyzer
            if (buffer.getNumChannels() > 0)
            {
                const float* data = buffer.getReadPointer(0);
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    generatorPanel.pushSampleForAnalysis(data[i]);
                }
            }

            // IR measurement
            auto& irAnalyzer = responsePanel.getAnalyzer();
            if (irAnalyzer.getState() == ImpulseResponseAnalyzer::MeasurementState::GeneratingSweep)
            {
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    float inputSample = buffer.getNumChannels() > 0 ? buffer.getSample(0, i) : 0.0f;
                    float sweepOutput = irAnalyzer.processSample(inputSample);
                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    {
                        buffer.addSample(ch, i, sweepOutput);
                    }
                }
            }

            // BPM and Key detection
            analysisPanel.processBlock(buffer);
        });

        // Set device started callback to prepare effect chain with correct sample rate
        audioEngine.setDeviceStartedCallback([this](double sampleRate, int blockSize)
        {
            pluginHostPanel.prepare(sampleRate, blockSize);
            analysisPanel.prepare(sampleRate, blockSize);
            toolsPanel.prepare(sampleRate, blockSize);
        });

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
            // Spectrum panel
            spectrumPanel.pushNextSampleIntoFifo(sample);

            // Analysis panel
            analysisPanel.pushSample(sample);

            // Meters panel
            metersPanel.pushSample(sample);

            // Multi-view container
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

        // Create horizontal container for Master Gain + Level Meter
        metersRowContainer.addPanel(&masterGainControl, 0.35, 80, 120, "Gain");
        metersRowContainer.addPanel(&levelMeter, 0.65, 120, -1, "Levels");
        rightPanelContainer.addPanel(&metersRowContainer, 0.22, 150, -1, "");

        // Add true peak meter
        rightPanelContainer.addPanel(&truePeakMeter, 0.15, 100, -1, "True Peak");

        // Add phase meter
        rightPanelContainer.addPanel(&phaseMeter, 0.15, 100, -1, "Phase");

        // Add loudness meter
        rightPanelContainer.addPanel(&loudnessMeter, 0.20, 150, -1, "Loudness");

        // Add A/B comparison control
        rightPanelContainer.addPanel(&abCompareControl, 0.15, 100, 150, "A/B Compare");

        // Set A/B compare callbacks
        abCompareControl.onMixChanged = [this](float wetAmount)
        {
            audioEngine.setDryWetMix(wetAmount);
        };

        // Set master gain callback
        masterGainControl.onGainChanged = [this](float gainLinear)
        {
            audioEngine.setMasterGain(gainLinear);
        };

        // Set level callback from audio engine
        audioEngine.setLevelCallback([this](float leftRMS, float leftPeak, float rightRMS, float rightPeak)
        {
            levelMeter.setLevels(leftRMS, leftPeak, rightRMS, rightPeak);

            // Update TopInfoBar mini meters
            topInfoBar.setLevels(leftRMS, leftPeak, rightRMS, rightPeak);

            // Feed to meters panel vectorscope
            metersPanel.pushStereoSample(leftPeak, rightPeak);

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

                // Also load to Track Compare Panel as Track A
                trackComparePanel.loadTrackA(file);

                // Load to AudioTimeline
                audioTimeline.loadFile(file, audioEngine.getFormatManager());

                // Update marker panel duration and clear markers for new file
                markerPanel.setDuration(audioEngine.getDuration());
                markerPanel.clearAllMarkers();
                audioTimeline.clearAllMarkers();

                // Update file info panel and TopInfoBar
                auto* reader = audioEngine.getFormatManager().createReaderFor(file);
                if (reader != nullptr)
                {
                    fileInfoPanel.setFileInfo(file, reader);
                    topInfoBar.setFileInfo(file, reader);
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
            juce::KeyPress('a', juce::ModifierKeys::noModifiers, 0),
            [this]() { abCompareControl.toggleAB(); }
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

    void setupMultiTrackComponents()
    {
        // Create multi-track audio source
        multiTrackSource = std::make_unique<MultiTrackAudioSource>(audioEngine.getFormatManager());

        // Create timeline panel
        multiTrackTimeline = std::make_unique<TimelinePanel>(projectManager, audioEngine.getFormatManager());

        // Create mixer panel
        mixerPanel = std::make_unique<MixerPanel>(projectManager);

        // Add to tabbed display
        tabbedDisplay.addTab("Multi-Track", multiTrackTimeline.get());
        tabbedDisplay.addTab("Mixer", mixerPanel.get());

        // Setup callbacks
        multiTrackTimeline->onPlayheadMoved = [this](juce::int64 samplePos)
        {
            multiTrackSource->setNextReadPosition(samplePos);
        };

        // Create a demo project with tracks
        createDemoProject();
    }

    void createDemoProject()
    {
        // Create new project
        projectManager.newProject("Demo Project");

        // Add some demo tracks
        projectManager.addTrack("Track 1");
        projectManager.addTrack("Track 2");
        projectManager.addTrack("Track 3");

        // Load project into multi-track source
        multiTrackSource->loadProject(projectManager.getProjectState());
    }

    void addFileToTrack(const juce::File& file, int trackIndex)
    {
        auto& project = projectManager.getProject();

        if (trackIndex < 0 || trackIndex >= project.getNumTracks())
            return;

        auto track = project.getTrack(trackIndex);
        if (!track.isValid())
            return;

        // Get file info to determine clip length
        auto* reader = audioEngine.getFormatManager().createReaderFor(file);
        if (reader == nullptr)
            return;

        juce::int64 lengthInSamples = reader->lengthInSamples;
        delete reader;

        // Find the end of the last clip on this track
        TrackModel trackModel(track);
        auto clips = trackModel.getClipsSortedByTime();
        juce::int64 timelineStart = 0;

        if (!clips.isEmpty())
        {
            auto lastClip = clips.getLast();
            ClipModel lastClipModel(lastClip);
            timelineStart = lastClipModel.getTimelineEnd();
        }

        // Add clip to track
        projectManager.addClip(track, file.getFullPathName(), timelineStart, lengthInSamples);

        // Refresh timeline display
        if (multiTrackTimeline != nullptr)
            multiTrackTimeline->projectChanged();
    }

    void showAddToTrackDialog(const juce::File& file)
    {
        auto& project = projectManager.getProject();
        int numTracks = project.getNumTracks();

        if (numTracks == 0)
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "No Tracks",
                "Please create a track first in the Multi-Track tab.",
                "OK"
            );
            return;
        }

        juce::StringArray trackNames;
        for (int i = 0; i < numTracks; ++i)
        {
            TrackModel track(project.getTrack(i));
            trackNames.add(track.getName());
        }

        auto* alertWindow = new juce::AlertWindow(
            "Add to Track",
            "Select a track to add \"" + file.getFileName() + "\" to:",
            juce::AlertWindow::QuestionIcon
        );

        alertWindow->addComboBox("track", trackNames, "Track");
        alertWindow->addButton("Add", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alertWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        alertWindow->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, file, alertWindow](int result)
            {
                if (result == 1)
                {
                    int trackIndex = alertWindow->getComboBoxComponent("track")->getSelectedItemIndex();
                    addFileToTrack(file, trackIndex);
                }
                delete alertWindow;
            }
        ), true);
    }

    void setupTopInfoBar()
    {
        // Initialize with device info
        topInfoBar.setDeviceName(audioEngine.getCurrentDeviceName());
        topInfoBar.setSampleRate(audioEngine.getCurrentSampleRate());
        topInfoBar.setBufferSize(audioEngine.getCurrentBufferSize());

        // Transport callbacks
        topInfoBar.onPlay = [this]() { audioEngine.play(); };
        topInfoBar.onPause = [this]() { audioEngine.pause(); };
        topInfoBar.onStop = [this]() { audioEngine.stop(); };
        topInfoBar.onSkipToStart = [this]() { audioEngine.setPosition(0.0); };
        topInfoBar.onSkipToEnd = [this]() { audioEngine.setPosition(1.0); };

        topInfoBar.onSeek = [this](double seconds)
        {
            double position = seconds / audioEngine.getDuration();
            audioEngine.setPosition(juce::jlimit(0.0, 1.0, position));
        };

        topInfoBar.onToggleLoop = [this]()
        {
            bool newState = !audioEngine.isLoopEnabled();
            audioEngine.setLoopEnabled(newState);
            topInfoBar.setLoopEnabled(newState);
            audioTimeline.setLoopEnabled(newState);
        };

        topInfoBar.onRecord = [this]()
        {
            // TODO: Implement recording
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Recording",
                "Recording functionality will be implemented in a future update",
                "OK"
            );
        };

        // Utility button callbacks
        topInfoBar.onOpenFile = [this]() { loadAudioFile(); };
        topInfoBar.onSettings = [this]() { showSettings(); };
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

                // Also load to Track Compare Panel as Track A
                trackComparePanel.loadTrackA(file);

                // Load to AudioTimeline
                audioTimeline.loadFile(file, audioEngine.getFormatManager());

                // Update marker panel duration and clear markers for new file
                markerPanel.setDuration(audioEngine.getDuration());
                markerPanel.clearAllMarkers();
                audioTimeline.clearAllMarkers();

                // Update file info panel and TopInfoBar
                auto* reader = audioEngine.getFormatManager().createReaderFor(file);
                if (reader != nullptr)
                {
                    fileInfoPanel.setFileInfo(file, reader);
                    topInfoBar.setFileInfo(file, reader);
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

            // Update TopInfoBar device info
            topInfoBar.setDeviceName(audioEngine.getCurrentDeviceName());
            topInfoBar.setSampleRate(audioEngine.getCurrentSampleRate());
            topInfoBar.setBufferSize(audioEngine.getCurrentBufferSize());
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
        bool isPlaying = state == AudioEngine::PlayState::Playing;

        deviceControlPanel.setPlayButtonEnabled(hasFile && !isPlaying);
        deviceControlPanel.setPauseButtonEnabled(isPlaying);
        deviceControlPanel.setStopButtonEnabled(state != AudioEngine::PlayState::Stopped);

        // Update TopInfoBar state
        topInfoBar.setPlaying(isPlaying);

        // Update status bar (compact version - main info is now in TopInfoBar)
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
            statusText += " | " + juce::String(position * 100.0, 1) + "% | " + audioEngine.getCurrentFileName();
        }

        statusBar.setText(statusText, juce::dontSendNotification);

        // Update positions in all components
        if (hasFile)
        {
            double currentPos = audioEngine.getPosition();
            double currentSeconds = currentPos * audioEngine.getDuration();
            double durationSecs = audioEngine.getDuration();

            // Update TopInfoBar position and duration
            topInfoBar.setPosition(currentSeconds);
            topInfoBar.setDuration(durationSecs);

            // Update waveform display
            waveformDisplay.setPosition(currentPos);

            // Update Track Compare Panel position
            trackComparePanel.setPosition(currentPos);

            // Update Transport Control Panel
            transportControlPanel.setPosition(currentSeconds);
            transportControlPanel.setDuration(durationSecs);

            // Update Audio Timeline position
            audioTimeline.setPosition(currentPos);

            // Update level meter when not playing
            if (!isPlaying)
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
            menu.addItem(fileAddToTrack, "Add File to Track...", audioEngine.hasFileLoaded());
            menu.addSeparator();
            menu.addItem(fileNewProject, "New Multi-Track Project");
            menu.addItem(fileAddTrack, "Add Track");
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

            case fileAddToTrack:
                if (audioEngine.hasFileLoaded())
                {
                    juce::File currentFile = audioEngine.getCurrentFile();
                    if (currentFile.existsAsFile())
                        showAddToTrackDialog(currentFile);
                }
                break;

            case fileNewProject:
                projectManager.newProject("New Project");
                projectManager.addTrack("Track 1");
                multiTrackSource->loadProject(projectManager.getProjectState());
                break;

            case fileAddTrack:
                {
                    int trackNum = projectManager.getProject().getNumTracks() + 1;
                    projectManager.addTrack("Track " + juce::String(trackNum));
                }
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
            "Processing:\n"
            "  A - Toggle A/B Compare (Dry/Wet)\n\n"
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
    JapaneseLookAndFeel japaneseLookAndFeel;
    AudioEngine audioEngine;

    // UI Components
    PanelContainer mainPanelContainer { PanelContainer::Orientation::Horizontal };
    PanelContainer leftPanelContainer { PanelContainer::Orientation::Vertical };
    PanelContainer rightPanelContainer { PanelContainer::Orientation::Vertical };
    PanelContainer metersRowContainer { PanelContainer::Orientation::Horizontal };

    DeviceControlPanel deviceControlPanel;
    TransportControlPanel transportControlPanel;
    TopInfoBar topInfoBar;
    FileInfoPanel fileInfoPanel;
    RecordingPanel recordingPanel;
    PlaylistPanel playlistPanel;
    MarkerPanel markerPanel;
    TabbedDisplayArea tabbedDisplay;
    WaveformDisplay waveformDisplay;

    // Combined panels (organized UI)
    SpectrumPanel spectrumPanel;       // Spectrum + Spectrogram
    AnalysisPanel analysisPanel;       // Pitch, BPM, Key, Harmonics, MFCC
    MetersPanel metersPanel;           // Vectorscope, Histogram
    ToolsPanel toolsPanel;             // Filter/EQ, Generator, IR/FR
    PluginHostPanel pluginHostPanel;   // VST3 Plugin Host
    TrackComparePanel trackComparePanel; // Dual track comparison
    AudioTimeline audioTimeline;          // Professional audio timeline

    // Multi-track DAW components
    ProjectManager projectManager;
    std::unique_ptr<TimelinePanel> multiTrackTimeline;
    std::unique_ptr<MixerPanel> mixerPanel;
    std::unique_ptr<MultiTrackAudioSource> multiTrackSource;

    MultiViewContainer multiViewContainer;
    ABCompareControl abCompareControl;
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

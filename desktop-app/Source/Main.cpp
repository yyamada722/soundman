/*
  ==============================================================================

    Soundman Desktop - Main Entry Point

    Real-time Audio Analysis Tool

  ==============================================================================
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

//==============================================================================
/**
    Main Application Window Component
*/
class MainComponent : public juce::Component
{
public:
    //==========================================================================
    MainComponent()
    {
        setSize(800, 600);
    }

    ~MainComponent() override
    {
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        // Background
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

        // Title
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(32.0f, juce::Font::bold));
        g.drawText("Soundman Desktop", getLocalBounds().removeFromTop(80),
                   juce::Justification::centred, true);

        // Version info
        g.setFont(juce::Font(16.0f));
        g.drawText("Version 0.1.0 (Alpha)", getLocalBounds().removeFromTop(120),
                   juce::Justification::centred, true);

        // Status text
        g.setFont(juce::Font(14.0f));
        g.setColour(juce::Colours::lightgrey);
        g.drawText("Real-time Audio Analysis Tool",
                   getLocalBounds().withTrimmedTop(140),
                   juce::Justification::centredTop, true);

        // Instructions
        auto instructionBounds = getLocalBounds().reduced(40);
        instructionBounds.removeFromTop(200);

        g.setFont(juce::Font(12.0f));
        g.drawMultiLineText(
            "Project successfully initialized!\n\n"
            "Next steps:\n"
            "1. Implement AudioEngine in Source/Core/\n"
            "2. Create UI components in Source/UI/\n"
            "3. Add DSP processors in Source/DSP/\n\n"
            "See docs/ROADMAP.md for the full development plan.",
            instructionBounds.getX(),
            instructionBounds.getY(),
            instructionBounds.getWidth()
        );
    }

    void resized() override
    {
        // Layout components here when they are added
    }

private:
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
                           juce::Desktop::getInstance().getDefaultLookAndFeel()
                                                       .findColour(juce::ResizableWindow::backgroundColourId),
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

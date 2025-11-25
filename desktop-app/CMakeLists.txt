cmake_minimum_required(VERSION 3.18)

project(SoundmanDesktop VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ======================================
# JUCE Framework
# ======================================
# Uncomment and modify the path to JUCE on your system
# add_subdirectory(path/to/JUCE)

# Or use FetchContent to download JUCE automatically
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.9  # Use the latest stable version
)
FetchContent_MakeAvailable(JUCE)

# ======================================
# Application Target
# ======================================
juce_add_gui_app(SoundmanDesktop
    PRODUCT_NAME "Soundman"
    COMPANY_NAME "Soundman Project"
    BUNDLE_ID "com.soundman.desktop"
)

# ======================================
# Source Files
# ======================================
target_sources(SoundmanDesktop PRIVATE
    Source/Main.cpp
    # Core
    Source/Core/AudioEngine.cpp
    # Source/Core/AudioDeviceManager.cpp
    # Source/Core/FileManager.cpp

    # DSP
    # Source/DSP/FFTAnalyzer.cpp
    # Source/DSP/LevelMeter.cpp
    # Source/DSP/Filters.cpp

    # UI
    # Source/UI/MainComponent.cpp
    Source/UI/WaveformDisplay.cpp
    Source/UI/LevelMeter.cpp
    Source/UI/PanelContainer.cpp
    Source/UI/TabbedDisplayArea.cpp
    Source/UI/DeviceControlPanel.cpp
    Source/UI/KeyboardHandler.cpp
    # Source/UI/SpectrumDisplay.cpp
    # Source/UI/SettingsPanel.cpp

    # Data
    # Source/Data/DataManager.cpp
    # Source/Data/MetadataWriter.cpp

    # Utils
    # Source/Utils/Logger.cpp
    # Source/Utils/Config.cpp
)

# ======================================
# JUCE Modules
# ======================================
target_link_libraries(SoundmanDesktop
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ======================================
# Compile Definitions
# ======================================
target_compile_definitions(SoundmanDesktop
    PRIVATE
        # JUCE Config
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:SoundmanDesktop,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:SoundmanDesktop,JUCE_VERSION>"

        # Audio Config
        JUCE_ASIO=0
        JUCE_WASAPI=1
        JUCE_DIRECTSOUND=1
        JUCE_ALSA=1
        JUCE_JACK=1
        JUCE_COREAUDIO=1
)

# ======================================
# Include Directories
# ======================================
target_include_directories(SoundmanDesktop
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# ======================================
# Platform-specific Settings
# ======================================
if(WIN32)
    # Windows-specific settings
    target_compile_definitions(SoundmanDesktop PRIVATE JUCE_WINDOWS=1)
elseif(APPLE)
    # macOS-specific settings
    target_compile_definitions(SoundmanDesktop PRIVATE JUCE_MAC=1)
elseif(UNIX)
    # Linux-specific settings
    target_compile_definitions(SoundmanDesktop PRIVATE JUCE_LINUX=1)
endif()

# ======================================
# Testing (Optional)
# ======================================
# enable_testing()
# add_subdirectory(Tests)

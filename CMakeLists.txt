cmake_minimum_required(VERSION 3.22)
project(TutorKeyBox01 VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE-master/CMakeLists.txt")
    add_subdirectory(JUCE-master JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_gui_app(TutorKeybox
    PRODUCT_NAME "TutorKeybox"
    ICON_BIG "Resources/AppIcon.icns"
)

target_sources(TutorKeybox
    PRIVATE
        Source/Main.cpp
        Source/Core/MidiState.h
        Source/Audio/AudioEngine.h
        Source/Audio/AudioEngine.cpp
        Source/GUI/TelemetryHeaderView.h
        Source/GUI/TelemetryHeaderView.cpp
        Source/GUI/HardwareSelectorBar.h
        Source/GUI/HardwareSelectorBar.cpp
        Source/GUI/LayerCardComponent.h
        Source/GUI/LayerCardComponent.cpp
        Source/GUI/MainComponent.h
        Source/GUI/MainComponent.cpp
        Source/Synth/SampleHeader.h
        Source/Synth/CustomSamplerSound.h
        Source/Synth/CustomSamplerVoice.h
        Source/Synth/CustomSamplerVoice.cpp
        Source/Synth/LayeredSynth.h
        Source/Synth/LayeredSynth.cpp
        Source/Synth/SampleContainerReader.h
        Source/Synth/SampleContainerReader.cpp
        Source/Synth/SamplePackager.h
        Source/Synth/SamplePackager.cpp
)

target_link_libraries(TutorKeybox
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
)

target_compile_definitions(TutorKeybox
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

juce_generate_juce_header(TutorKeybox)

# ==============================================================================
# Standalone Command-Line Sample Packager Utility
# ==============================================================================
juce_add_console_app(TKBPackager
    PRODUCT_NAME "TKBPackager"
)

target_sources(TKBPackager
    PRIVATE
        Tools/Packager.cpp
        Source/Synth/SampleHeader.h
        Source/Synth/SamplePackager.h
        Source/Synth/SamplePackager.cpp
)

target_link_libraries(TKBPackager
    PRIVATE
        juce::juce_audio_formats
        juce::juce_audio_basics
        juce::juce_core
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
)

target_compile_definitions(TKBPackager
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

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
        Source/GUI/ChannelStripComponent.h
        Source/GUI/ChannelStripComponent.cpp
        Source/GUI/SamplerWindow.h
        Source/GUI/SamplerWindow.cpp
        Source/GUI/ReverbWindow.h
        Source/GUI/ReverbWindow.cpp
        Source/GUI/FilterWindow.h
        Source/GUI/FilterWindow.cpp
        Source/GUI/MidiMappingDrawer.h
        Source/GUI/MidiMappingDrawer.cpp
        Source/GUI/VelocityCurveEditor.h
        Source/GUI/VelocityCurveEditor.cpp
        Source/GUI/MainComponent.h
        Source/GUI/MainComponent.cpp
        Source/Audio/ConvolutionReverbEngine.h
        Source/Audio/ConvolutionReverbEngine.cpp
        Source/Audio/PerLayerFilter.h
        Source/Audio/PerLayerFilter.cpp
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
        Source/Core/PresetManager.h
        Source/Core/PresetManager.cpp
        Source/Core/LibraryManager.h
        Source/Core/LibraryManager.cpp
        Source/Core/MidiRouter.h
        Source/Core/MidiRouter.cpp
        Source/Network/NetworkProtocol.h
        Source/Network/NetworkMidiReceiver.h
        Source/Network/NetworkMidiReceiver.cpp
        Source/Network/DeployClient.h
        Source/Network/DeployClient.cpp
        Source/GUI/DeployControlBar.h
        Source/GUI/DeployControlBar.cpp
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
        juce::juce_dsp
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
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
)

target_compile_definitions(TKBPackager
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

# ==============================================================================
# Headless Audio Engine Core (Raspberry Pi 5 Daemon - 0% GUI Overhead)
# ==============================================================================
juce_add_console_app(TutorKeyboxCore
    PRODUCT_NAME "TutorKeyboxCore"
)

target_sources(TutorKeyboxCore
    PRIVATE
        Source/Core/HeadlessMain.cpp
        Source/Core/MidiState.h
        Source/Audio/AudioEngine.h
        Source/Audio/AudioEngine.cpp
        Source/Audio/ConvolutionReverbEngine.h
        Source/Audio/ConvolutionReverbEngine.cpp
        Source/Audio/PerLayerFilter.h
        Source/Audio/PerLayerFilter.cpp
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
        Source/Core/PresetManager.h
        Source/Core/PresetManager.cpp
        Source/Core/LibraryManager.h
        Source/Core/LibraryManager.cpp
        Source/Core/MidiRouter.h
        Source/Core/MidiRouter.cpp
        Source/Network/NetworkProtocol.h
        Source/Network/NetworkServer.h
        Source/Network/NetworkServer.cpp
)

target_link_libraries(TutorKeyboxCore
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_recommended_config_flags
)

target_compile_definitions(TutorKeyboxCore
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

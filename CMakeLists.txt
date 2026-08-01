cmake_minimum_required(VERSION 3.22)
project(TutorKeyBox01 VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

add_subdirectory(JUCE-master JUCE)

juce_add_gui_app(TutorKeybox
    PRODUCT_NAME "TutorKeybox"
    
)

target_sources(TutorKeybox
    PRIVATE
        Source/Main.cpp
        Source/Core/MidiState.h
        Source/Audio/AudioEngine.h
        Source/Audio/AudioEngine.cpp
        Source/GUI/MidiMonitorView.h
        Source/GUI/MidiMonitorView.cpp
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
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
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

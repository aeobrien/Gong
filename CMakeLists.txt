cmake_minimum_required(VERSION 3.22)

project(GongEnergySynthesizer VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE
add_subdirectory(JUCE)

# Define the application
juce_add_gui_app(GongSynth
    PRODUCT_NAME "Gong Energy Synthesizer"
    COMPANY_NAME "Gong Audio"
    BUNDLE_ID "com.gongaudio.gongsynth"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Gong Synthesizer needs microphone access for audio input excitation and strike detection."
)

# Source files
target_sources(GongSynth PRIVATE
    # Core
    src/Main.cpp
    src/MainComponent.cpp

    # Audio Engine
    src/EnergyAccumulator.cpp
    src/SpreadVoiceResonator.cpp
    src/ResonatorBank.cpp
    src/GongSynthesizer.cpp
    src/ImpulseGenerator.cpp
    src/SyntheticImpulseGenerator.cpp
    src/MidiControllerMock.cpp

    # Effects
    src/ConvolutionEngine.cpp
    src/ExciterProcessor.cpp
    src/MultibandCompressor.cpp

    # Presets
    src/PresetManager.cpp

    # Server
    src/PerformanceServer.cpp

    # UI
    src/IRWaveformComponent.cpp
)

# Include paths for third-party libs
target_include_directories(GongSynth PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/httplib
)

# JUCE modules
target_compile_definitions(GongSynth PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(GongSynth PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Generate JuceHeader.h
juce_generate_juce_header(GongSynth)

# Resources directory for IR files
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# Presets directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/presets)

# Test harness
add_subdirectory(tests/ConvolutionTest)

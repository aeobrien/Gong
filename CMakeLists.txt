cmake_minimum_required(VERSION 3.22)

project(GongConvolutionReverb VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE
add_subdirectory(JUCE)

# Define the application
juce_add_gui_app(GongReverb
    PRODUCT_NAME "Gong Reverb"
    COMPANY_NAME "Gong Audio"
    BUNDLE_ID "com.gongaudio.gongreverb"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Gong Reverb needs microphone access for audio input excitation and envelope following."
)

# Source files
target_sources(GongReverb PRIVATE
    src/Main.cpp
    src/MainComponent.cpp
    src/ConvolutionEngine.cpp
    src/ImpulseGenerator.cpp
    src/Resonator.cpp
    src/ResonatorVoice.cpp
    src/ResonatorSynth.cpp
    src/EnvelopeFollower.cpp
    src/IRWaveformComponent.cpp
)

# JUCE modules
target_compile_definitions(GongReverb PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(GongReverb PRIVATE
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
juce_generate_juce_header(GongReverb)

# Resources directory for IR files
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources)

cmake_minimum_required(VERSION 3.22)

juce_add_console_app(ConvolutionTest
    PRODUCT_NAME "Convolution Test"
)

target_sources(ConvolutionTest PRIVATE
    Main.cpp
    ${CMAKE_SOURCE_DIR}/src/ConvolutionEngine.cpp
)

target_include_directories(ConvolutionTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(ConvolutionTest PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ConvolutionTest PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_core
    juce::juce_dsp
    juce::juce_events
    juce::juce_recommended_config_flags
)

juce_generate_juce_header(ConvolutionTest)

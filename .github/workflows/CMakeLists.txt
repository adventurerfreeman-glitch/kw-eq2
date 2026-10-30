cmake_minimum_required(VERSION 3.22)
project(KWTiltLeanEQ VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(KWTiltLeanEQ
    COMPANY_NAME "KWBoost"
    PLUGIN_MANUFACTURER_CODE KWbs
    PLUGIN_CODE KWeq
    FORMATS VST3
    PRODUCT_NAME "KW Tilt Lean EQ"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(KWTiltLeanEQ PRIVATE Source/PluginProcessor.cpp Source/PluginProcessor.h Source/KwEq.h)
target_compile_features(KWTiltLeanEQ PRIVATE cxx_std_17)
target_compile_definitions(KWTiltLeanEQ PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(KWTiltLeanEQ PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

option(KW_BUILD_TESTS "Build the response test tool" OFF)
if(KW_BUILD_TESTS)
    juce_add_console_app(KWResponseTest PRODUCT_NAME "KWResponseTest")
    target_sources(KWResponseTest PRIVATE Tests/ResponseTest.cpp Source/KwEq.h)
    target_include_directories(KWResponseTest PRIVATE Source)
    target_compile_features(KWResponseTest PRIVATE cxx_std_17)
    target_compile_definitions(KWResponseTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(KWResponseTest PRIVATE juce::juce_dsp juce::juce_core)
endif()

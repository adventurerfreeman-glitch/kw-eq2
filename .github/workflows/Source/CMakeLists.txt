cmake_minimum_required(VERSION 3.22)
project(KWTiltLeanEQ VERSION 0.2.0)

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

target_sources(KWTiltLeanEQ PRIVATE
    Source/PluginProcessor.cpp Source/PluginProcessor.h
    Source/PluginEditor.cpp Source/PluginEditor.h
    Source/KwEq.h)
target_compile_features(KWTiltLeanEQ PRIVATE cxx_std_17)
target_compile_definitions(KWTiltLeanEQ PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(KWTiltLeanEQ PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# Optional developer tools (not needed to use the plugin)
option(KW_BUILD_TESTS "Build the editor snapshot tool" OFF)
if(KW_BUILD_TESTS)
    juce_add_console_app(KWEditorSnapshot PRODUCT_NAME "KWEditorSnapshot")
    target_sources(KWEditorSnapshot PRIVATE Tests/Snapshot.cpp
        Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(KWEditorSnapshot PRIVATE Source)
    target_compile_features(KWEditorSnapshot PRIVATE cxx_std_17)
    target_compile_definitions(KWEditorSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_PLUGINHOST_VST3=0 JUCE_MODAL_LOOPS_PERMITTED=0)
    target_link_libraries(KWEditorSnapshot PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()

cmake_minimum_required(VERSION 3.22)

project(KWTiltLeanEQ VERSION 0.1.0)

# Include JUCE (adjust path if your JUCE folder is named differently)
add_subdirectory(JUCE)

juce_add_plugin(KWTiltLeanEQ
    COMPANY_NAME "KW"
    PLUGIN_NAME "KW Dual Matrix EQ"
    FORMATS VST3 Standalone
    PRODUCT_NAME "KW Dual Matrix EQ")

target_sources(KWTiltLeanEQ
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/KwEq.h)

target_compile_definitions(KWTiltLeanEQ
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(KWTiltLeanEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_data_structures
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

cmake_minimum_required(VERSION 3.22)
project(GrowlForge VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use a local JUCE folder if present, otherwise download it (needed for CI builds).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(GF_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

juce_add_plugin(GrowlForge
    COMPANY_NAME "GrowlForge Audio"
    PLUGIN_MANUFACTURER_CODE Grfa
    PLUGIN_CODE Grlf
    FORMATS VST3
    PRODUCT_NAME "GrowlForge"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(GrowlForge PRIVATE ${GF_SOURCES})

target_compile_definitions(GrowlForge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(GrowlForge
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# Headless offline test (renders audio through the processor and checks it).
juce_add_console_app(OfflineTest PRODUCT_NAME "OfflineTest")
target_sources(OfflineTest PRIVATE ${GF_SOURCES} Tests/OfflineTest.cpp)
target_compile_definitions(OfflineTest PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(OfflineTest PRIVATE
    juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)

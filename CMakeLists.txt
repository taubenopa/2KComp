cmake_minimum_required(VERSION 3.22)
project(TwoKComp VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(TwoKComp
    COMPANY_NAME            "Jxhn2k"
    PLUGIN_MANUFACTURER_CODE Jxhn
    PLUGIN_CODE             TwoK
    FORMATS                 VST3
    PRODUCT_NAME            "2KComp"
    VST3_CATEGORIES         Fx Dynamics
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(TwoKAssets SOURCES jxhn2k.png)

target_sources(TwoKComp PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp)

target_compile_definitions(TwoKComp PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(TwoKComp
    PRIVATE
        TwoKAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

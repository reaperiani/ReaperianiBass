cmake_minimum_required(VERSION 3.22)

project(ReaperianiBass VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Build the standalone DSP smoke tests" ON)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

set(REAPERIANI_FORMATS VST3)
if(APPLE)
    list(APPEND REAPERIANI_FORMATS AU)
endif()

juce_add_plugin(ReaperianiBass
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "Reaperiani"
    BUNDLE_ID "com.reaperiani.reaperianibass"
    PLUGIN_MANUFACTURER_CODE Rprn
    PLUGIN_CODE Rbs1
    PRODUCT_NAME "ReaperianiBass"
    DESCRIPTION "Psychoacoustic harmonic bass enhancer"
    FORMATS ${REAPERIANI_FORMATS}
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
)

juce_generate_juce_header(ReaperianiBass)

target_sources(ReaperianiBass PRIVATE
    Source/BassEnhancerDSP.cpp
    Source/PluginEditor.cpp
    Source/PluginProcessor.cpp
)

target_include_directories(ReaperianiBass PRIVATE Source)

target_compile_definitions(ReaperianiBass
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ReaperianiBass
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(BUILD_TESTING)
    include(CTest)
    add_executable(ReaperianiBassDSPTests
        Source/BassEnhancerDSP.cpp
        tests/dsp_smoke_test.cpp
    )
    target_include_directories(ReaperianiBassDSPTests PRIVATE Source)
    if(MSVC)
        target_compile_options(ReaperianiBassDSPTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(ReaperianiBassDSPTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME ReaperianiBassDSPTests COMMAND ReaperianiBassDSPTests)
endif()

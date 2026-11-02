cmake_minimum_required(VERSION 3.22)

project(SilverChannel01 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows: runtime estatico (o .vst3 nao depende de DLLs do Visual C++)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(SC_BUILD_TESTS "Compila o app de teste offline (Linux/CI)" OFF)

# JUCE: usa uma copia local (-DJUCE_DIR=...) ou baixa do GitHub
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(SC_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Panels.cpp
    Source/InsertHost.cpp
    Source/Presets.cpp)

set(SC_DEFINES
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

juce_add_plugin(SilverChannel01
    COMPANY_NAME                "SILVER"
    COMPANY_WEBSITE             "https://github.com/SILVERVST3"
    PLUGIN_MANUFACTURER_CODE    Slvr
    PLUGIN_CODE                 SCh1
    FORMATS                     VST3
    PRODUCT_NAME                "SilverChannel 01"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx Dynamics EQ)

target_sources(SilverChannel01 PRIVATE ${SC_SOURCES})
target_compile_definitions(SilverChannel01 PUBLIC ${SC_DEFINES})
target_link_libraries(SilverChannel01
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(SC_BUILD_TESTS)
    juce_add_console_app(SCTest PRODUCT_NAME "SCTest")
    target_sources(SCTest PRIVATE ${SC_SOURCES} tests/TestMain.cpp)
    target_compile_definitions(SCTest PRIVATE ${SC_DEFINES} SC_TEST_BUILD=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(SCTest PRIVATE
        juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()

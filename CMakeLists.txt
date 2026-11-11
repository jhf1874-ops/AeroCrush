cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel). Must be set before project().
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(AeroCrush VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AeroCrush
    COMPANY_NAME "AeroCrush Audio"
    PRODUCT_NAME "AeroCrush"
    BUNDLE_ID "com.aerocrush.aerocrush"
    PLUGIN_MANUFACTURER_CODE Aero
    PLUGIN_CODE AcR1
    FORMATS VST3 AU Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Distortion Delay
    COPY_PLUGIN_AFTER_BUILD TRUE)   # installs into ~/Library/Audio/Plug-Ins/{VST3,Components}

juce_generate_juce_header(AeroCrush)

target_sources(AeroCrush PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(AeroCrush PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(AeroCrush
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# DSP self-test (no JUCE needed):  ctest --test-dir build -C Release
enable_testing()
add_executable(dsp_selftest tests/dsp_selftest.cpp)
target_include_directories(dsp_selftest PRIVATE Source/DSP)
add_test(NAME dsp_selftest COMMAND dsp_selftest)

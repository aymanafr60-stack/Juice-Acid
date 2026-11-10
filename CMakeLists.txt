cmake_minimum_required(VERSION 3.22)
project(Squelch VERSION 0.8.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Squelch
    COMPANY_NAME                "Aymane"
    PLUGIN_MANUFACTURER_CODE    Aymn
    PLUGIN_CODE                 Sqlc
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "Squelch"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Synth)

juce_add_binary_data(SquelchData SOURCES
    Resources/PlexMono-500.ttf
    Resources/PlexMono-700.ttf
    Resources/DotGothic16.ttf
    Resources/Fredoka-600.ttf
    Resources/Fredoka-700.ttf
    Resources/Manrope-800.ttf
    Resources/bg_phosphor.jpg
    Resources/bg_matrix.jpg
    Resources/bg_kawaii.jpg
    Resources/knob_phosphor_46.png
    Resources/knob_phosphor_56.png
    Resources/knob_phosphor_76.png
    Resources/knob_matrix_46.png
    Resources/knob_matrix_56.png
    Resources/knob_matrix_76.png
    Resources/knob_kawaii_46.png
    Resources/knob_kawaii_56.png
    Resources/knob_kawaii_76.png)

target_sources(Squelch PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Squelch PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Squelch
    PRIVATE
        SquelchData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

cmake_minimum_required(VERSION 3.24)
project(SoLyP VERSION 1.0.0 LANGUAGES CXX)

cmake_policy(SET CMP0169 OLD)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(JUCE)

set(SoLyP_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Data/Song.cpp
    Source/Midi/MidiManager.cpp
    Source/UI/ControlsPanel.cpp
    Source/UI/LeftPanel.cpp
)

juce_add_plugin(SoLyP
    PRODUCT_NAME                     "SoLyP"
    FORMATS                          VST3 Standalone
    PLUGIN_MANUFACTURER_CODE         Soly
    PLUGIN_CODE                      LyPr
    DESCRIPTION                      "Song Lyrics Prompter"
    VERSION                          ${PROJECT_VERSION}

    NEEDS_MIDI_INPUT                 TRUE
    IS_MIDI_EFFECT                   TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS      FALSE
    COPY_PLUGIN_AFTER_BUILD          FALSE
    ICON_PATH                        "${CMAKE_SOURCE_DIR}/Source/UI/icons/app_icon.ico"
)

target_sources(SoLyP PRIVATE ${SoLyP_SOURCES})
target_include_directories(SoLyP PRIVATE Source)
target_link_libraries(SoLyP PRIVATE juce::juce_audio_utils juce::juce_audio_devices)

juce_generate_juce_header(SoLyP)

target_compile_definitions(SoLyP PRIVATE
    SOLYP_VERSION="${PROJECT_VERSION}"
)

# embed icon in standalone executable (Windows)
if(WIN32)
    set(ICON_RC_PATH "${CMAKE_BINARY_DIR}/generated/app_icon.rc")
    file(WRITE "${ICON_RC_PATH}" "1 ICON \"${CMAKE_SOURCE_DIR}/Source/UI/icons/app_icon.ico\"\n")
    target_sources(SoLyP_Standalone PRIVATE "${ICON_RC_PATH}")
endif()

if(MSVC)
    target_compile_options(SoLyP PRIVATE /W4)
else()
    target_compile_options(SoLyP PRIVATE -Wall -Wextra)
endif()

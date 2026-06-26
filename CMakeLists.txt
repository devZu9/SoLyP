cmake_minimum_required(VERSION 3.24)
project(SoLyP VERSION 1.1.12 LANGUAGES CXX)

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
    Source/Logic.cpp
    Source/Display.cpp
    Source/UI/Display/DisplayCallbacks.cpp
    Source/UI/Display/DisplayActions.cpp
    Source/UI/Display/DisplayHelpers.cpp
    Source/Data/Song.cpp
    Source/Data/SettingsManager.cpp
    Source/Data/I18n.cpp
    Source/Midi/MidiManager.cpp
    Source/UI/ControlsPanel.cpp
    Source/UI/LeftPanel.cpp
    Source/UI/SaveDialogComponent.cpp
    Source/UI/SettingsCard.cpp
    Source/UI/SettingsComponent.cpp
    Source/UI/Theme.cpp
    Source/Standalone/StandaloneMain.cpp
)

juce_add_plugin(SoLyP
    PRODUCT_NAME                     "SoLyP"
    COMPANY_NAME                     "Miska Moloka"
    FORMATS                          VST3 Standalone
    PLUGIN_MANUFACTURER_CODE         MskM
    PLUGIN_CODE                      spMM
    BUNDLE_ID                        "com.miskamoloka.solyp"
    DESCRIPTION                      "Song Lyrics Prompter"
    VERSION                          ${PROJECT_VERSION}

    NEEDS_MIDI_INPUT                 TRUE
    NEEDS_MIDI_OUTPUT                TRUE
    IS_MIDI_EFFECT                   FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS      FALSE
    COPY_PLUGIN_AFTER_BUILD          FALSE
    ICON_PATH                        "${CMAKE_SOURCE_DIR}/Source/UI/icons/app_icon.ico"
)

target_sources(SoLyP PRIVATE ${SoLyP_SOURCES})
target_compile_definitions(SoLyP PRIVATE JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)
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

# copy lang and theme files to build output
file(COPY ${CMAKE_SOURCE_DIR}/langs/ DESTINATION ${CMAKE_BINARY_DIR}/langs)
file(COPY ${CMAKE_SOURCE_DIR}/langs/ DESTINATION ${CMAKE_BINARY_DIR}/SoLyP_artefacts/Debug/Standalone/langs)
file(COPY ${CMAKE_SOURCE_DIR}/Documents/SoLyP/themes/ DESTINATION ${CMAKE_BINARY_DIR}/themes)
file(COPY ${CMAKE_SOURCE_DIR}/Documents/SoLyP/themes/ DESTINATION ${CMAKE_BINARY_DIR}/SoLyP_artefacts/Debug/Standalone/themes)

if(MSVC)
    target_compile_options(SoLyP PRIVATE /W4 /utf-8)
else()
    target_compile_options(SoLyP PRIVATE -Wall -Wextra)
endif()

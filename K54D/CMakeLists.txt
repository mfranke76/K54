# K-54 D · Drumcomputer · Keller54 · by Frank E.
cmake_minimum_required(VERSION 3.22)

# Universal Binary fuer Apple Silicon und Intel-Macs
option(K54D_NATIVE_ONLY "Nur fuer diesen Mac bauen (schneller, fuer lokale Tests)" OFF)
if(APPLE AND NOT K54D_NATIVE_ONLY)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)

# Windows: Laufzeitbibliothek statisch einbinden (kein extra "Visual C++ Redistributable" noetig)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(K54D VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(K54D_BUILD_TESTS "Offline-Tests bauen (Linux/CI)" OFF)
option(K54D_COPY_AFTER_BUILD "Plugins nach dem Bauen in den Benutzer-Plug-In-Ordner kopieren" OFF)

# JUCE: -DJUCE_DIR=… oder Ordner "JUCE" neben dieser Datei, sonst automatisch von GitHub
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.16-security
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(K54D_FORMATS AU VST3 Standalone)
else()
    set(K54D_FORMATS VST3 Standalone)
endif()

juce_add_plugin(K54D
    VERSION                     ${PROJECT_VERSION}
    COMPANY_NAME                "Keller54"
    COMPANY_WEBSITE             "https://keller54.de"
    COMPANY_EMAIL               "info@keller54.de"
    COMPANY_COPYRIGHT           "Keller54 / Frank E."
    BUNDLE_ID                   "de.keller54.k54d"
    PRODUCT_NAME                "K-54 D"
    PLUGIN_NAME                 "K-54 D"
    DESCRIPTION                 "Drumcomputer von Keller54 - 808 bis heute"
    PLUGIN_MANUFACTURER_CODE    Kl54
    PLUGIN_CODE                 K5D1
    FORMATS                     ${K54D_FORMATS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    VST3_CATEGORIES             Instrument Drum
    COPY_PLUGIN_AFTER_BUILD     ${K54D_COPY_AFTER_BUILD})

juce_add_binary_data(K54DAssets SOURCES
    Resources/fonts/Syncopate-Bold.ttf
    Resources/fonts/Syncopate-Regular.ttf
    Resources/fonts/Monoton-Regular.ttf
    Resources/frank_portrait.png)

set(K54D_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp)

target_sources(K54D PRIVATE ${K54D_SOURCES})
target_include_directories(K54D PRIVATE Source)

set(K54D_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_FLAC=1
    JUCE_USE_OGGVORBIS=1
    JUCE_USE_MP3AUDIOFORMAT=1)

target_compile_definitions(K54D PUBLIC ${K54D_DEFS})

if(MSVC)
    target_compile_options(K54D PRIVATE /utf-8)
endif()

target_link_libraries(K54D
    PRIVATE
        K54DAssets
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_cryptography
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Offline-Tests: Presets rendern, DAW-Sync pruefen, Screenshot
if(K54D_BUILD_TESTS)
    juce_add_console_app(K54DTest PRODUCT_NAME "K54DTest")
    target_sources(K54DTest PRIVATE Tests/DrumTest.cpp ${K54D_SOURCES})
    target_include_directories(K54DTest PRIVATE Source)
    target_compile_definitions(K54DTest PRIVATE ${K54D_DEFS} JucePlugin_VersionString="${PROJECT_VERSION}"
        JucePlugin_Name="K-54 D" JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(K54DTest PRIVATE K54DAssets
        juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp juce::juce_cryptography
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()

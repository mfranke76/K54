# K-54 Analyzer · Keller54 · by Frank E.
cmake_minimum_required(VERSION 3.22)

# Universal Binary fuer Apple Silicon und Intel-Macs
option(K54A_NATIVE_ONLY "Nur fuer diesen Mac bauen (schneller, fuer lokale Tests)" OFF)
if(APPLE AND NOT K54A_NATIVE_ONLY)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)

# Windows: Laufzeitbibliothek statisch einbinden (kein extra "Visual C++ Redistributable" noetig)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(K54A VERSION 1.5.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(K54A_BUILD_TESTS "Offline-Tests bauen (Linux/CI)" OFF)

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

set(K54A_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND K54A_FORMATS AU)
endif()

juce_add_plugin(K54Analyzer
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "Keller54"
    COMPANY_WEBSITE "https://keller54.de"
    COMPANY_EMAIL "info@keller54.de"
    BUNDLE_ID de.keller54.k54analyzer
    PLUGIN_MANUFACTURER_CODE Kl54
    PLUGIN_CODE K5A1
    FORMATS ${K54A_FORMATS}
    PRODUCT_NAME "K-54 Analyzer"
    DESCRIPTION "Analyzer mit Spektrum, LUFS und Referenz-A/B"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Analyzer
    AU_MAIN_TYPE kAudioUnitType_Effect
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Der K-54 Analyzer misst das Eingangssignal.")

set(K54A_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(K54Analyzer PRIVATE ${K54A_SOURCES})

set(K54A_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_FLAC=1
    JUCE_USE_OGGVORBIS=1
    JUCE_USE_WINDOWS_MEDIA_FORMAT=1)

target_compile_definitions(K54Analyzer PUBLIC ${K54A_DEFS})

if(MSVC)
    target_compile_options(K54Analyzer PRIVATE /utf-8)
endif()

target_link_libraries(K54Analyzer
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(K54A_BUILD_TESTS)
    juce_add_console_app(K54ATest PRODUCT_NAME "K54ATest")
    target_sources(K54ATest PRIVATE Tests/AnalyzerTest.cpp ${K54A_SOURCES})
    target_include_directories(K54ATest PRIVATE Source)
    target_compile_definitions(K54ATest PRIVATE ${K54A_DEFS})
    target_link_libraries(K54ATest PRIVATE
        juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()

cmake_minimum_required(VERSION 3.22)

# Universal Binary fuer Apple Silicon und Intel-Macs
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

# Windows: Laufzeitbibliothek statisch einbinden (kein extra "Visual C++ Redistributable" noetig)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(K54 VERSION 1.4.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: liegt ein Ordner "JUCE" neben dieser Datei, wird er genommen,
# sonst wird JUCE automatisch von GitHub geladen.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.16-security
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

option(K54_COPY_AFTER_BUILD "Plugins nach dem Bauen in den Benutzer-Plug-In-Ordner kopieren" ON)

if(APPLE)
    set(K54_FORMATS AU VST3 Standalone)
else()
    set(K54_FORMATS VST3 Standalone)
endif()

juce_add_plugin(K54
    COMPANY_NAME                "Keller54"
    COMPANY_WEBSITE             ""
    COMPANY_COPYRIGHT           "Keller54 / Frank E."
    BUNDLE_ID                   "de.keller54.k54"
    PRODUCT_NAME                "K-54"
    PLUGIN_NAME                 "K-54"
    DESCRIPTION                 "Analog-inspirierter Ladder-Synthesizer von Keller54"
    PLUGIN_MANUFACTURER_CODE    Kl54
    PLUGIN_CODE                 K54s
    FORMATS                     ${K54_FORMATS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    VST3_CATEGORIES             Instrument Synth
    COPY_PLUGIN_AFTER_BUILD     ${K54_COPY_AFTER_BUILD})

juce_add_binary_data(K54Assets SOURCES
    Resources/fonts/Syncopate-Bold.ttf
    Resources/fonts/Syncopate-Regular.ttf
    Resources/fonts/Monoton-Regular.ttf)

target_sources(K54 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp)

target_include_directories(K54 PRIVATE Source)

target_compile_definitions(K54 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(K54
    PRIVATE
        K54Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_cryptography
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Optionaler Offline-Test (nicht fuer den normalen Build noetig)
option(K54_BUILD_TESTS "Offline-Render-Test bauen" OFF)
if(K54_BUILD_TESTS)
    add_executable(K54Test Tests/RenderTest.cpp)
    target_include_directories(K54Test PRIVATE Source)
    target_compile_definitions(K54Test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1 JucePlugin_Name="K-54")
    target_link_libraries(K54Test PRIVATE K54 K54Assets juce::juce_audio_utils juce::juce_dsp juce::juce_cryptography)
endif()

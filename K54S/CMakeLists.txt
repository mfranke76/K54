cmake_minimum_required(VERSION 3.22)

# Universal Binary fuer Apple Silicon und Intel-Macs
option(K55_NATIVE_ONLY "Nur fuer diesen Mac bauen (schneller, fuer lokale Tests)" OFF)
if(APPLE AND K55_NATIVE_ONLY)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
elseif(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

# Windows: Laufzeitbibliothek statisch einbinden (kein extra "Visual C++ Redistributable" noetig)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(K55 VERSION 1.5.0 LANGUAGES C CXX)

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

option(K55_COPY_AFTER_BUILD "Plugins nach dem Bauen in den Benutzer-Plug-In-Ordner kopieren" ON)

if(APPLE)
    set(K55_FORMATS AU VST3 Standalone)
else()
    set(K55_FORMATS VST3 Standalone)
endif()

juce_add_plugin(K55
    COMPANY_NAME                "Keller54"
    COMPANY_WEBSITE             ""
    COMPANY_COPYRIGHT           "Keller54 / Frank E."
    BUNDLE_ID                   "de.keller54.k54s"
    PRODUCT_NAME                "K-54 S"
    PLUGIN_NAME                 "K-54 S"
    DESCRIPTION                 "Analog-, Wavetable- und Sample-Synthesizer von Keller54"
    PLUGIN_MANUFACTURER_CODE    Kl54
    PLUGIN_CODE                 K5S1
    FORMATS                     ${K55_FORMATS}
    IS_SYNTH                    TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT  "Der K-54 S nimmt Samples vom Audio-Eingang auf."
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    VST3_CATEGORIES             Instrument Synth
    COPY_PLUGIN_AFTER_BUILD     ${K55_COPY_AFTER_BUILD})

juce_add_binary_data(K55Assets SOURCES
    Resources/fonts/Syncopate-Bold.ttf
    Resources/fonts/Syncopate-Regular.ttf
    Resources/fonts/Monoton-Regular.ttf
    Resources/frank_portrait.png
    Resources/piano/piano_A0.ogg
    Resources/piano/piano_A1.ogg
    Resources/piano/piano_A2.ogg
    Resources/piano/piano_A3.ogg
    Resources/piano/piano_A4.ogg
    Resources/piano/piano_A5.ogg
    Resources/piano/piano_A6.ogg
    Resources/piano/piano_A7.ogg
    Resources/piano/piano_C1.ogg
    Resources/piano/piano_C2.ogg
    Resources/piano/piano_C3.ogg
    Resources/piano/piano_C4.ogg
    Resources/piano/piano_C5.ogg
    Resources/piano/piano_C6.ogg
    Resources/piano/piano_C7.ogg
    Resources/piano/piano_C8.ogg
    Resources/piano/piano_Ds1.ogg
    Resources/piano/piano_Ds2.ogg
    Resources/piano/piano_Ds3.ogg
    Resources/piano/piano_Ds4.ogg
    Resources/piano/piano_Ds5.ogg
    Resources/piano/piano_Ds6.ogg
    Resources/piano/piano_Ds7.ogg
    Resources/piano/piano_Fs1.ogg
    Resources/piano/piano_Fs2.ogg
    Resources/piano/piano_Fs3.ogg
    Resources/piano/piano_Fs4.ogg
    Resources/piano/piano_Fs5.ogg
    Resources/piano/piano_Fs6.ogg
    Resources/piano/piano_Fs7.ogg)

target_sources(K55 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp)

target_include_directories(K55 PRIVATE Source)

target_compile_definitions(K55 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(K55
    PRIVATE
        K55Assets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_formats
        juce::juce_cryptography
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Optionaler Offline-Test (nicht fuer den normalen Build noetig)
option(K55_BUILD_TESTS "Offline-Render-Test bauen" OFF)
if(K55_BUILD_TESTS)
    add_executable(K55Test Tests/RenderTest.cpp)
    target_include_directories(K55Test PRIVATE Source)
    target_compile_definitions(K55Test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1 JucePlugin_Name="K-54 S")
    target_link_libraries(K55Test PRIVATE K55 K55Assets juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography)
endif()

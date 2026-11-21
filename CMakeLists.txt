cmake_minimum_required(VERSION 3.22)
project(MIME VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(MIME_BUILD_CLI "Build the offline test tool" OFF)
set(MIME_JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout (optional)")

if(MIME_JUCE_PATH)
    add_subdirectory(${MIME_JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(MSVC)
    # static runtime so the .vst3 runs without installing the VC++ redistributable
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(MIME_CORE_SOURCES
    Source/dsp/MimeEngine.cpp
    Source/analysis/BeatAnalyzer.cpp
    Source/presets/PresetGenerator.cpp)

juce_add_plugin(MIME
    COMPANY_NAME "qverbo"
    COMPANY_WEBSITE "https://github.com/qverbo/mime"
    BUNDLE_ID com.qverbo.mime
    PLUGIN_MANUFACTURER_CODE Qvrb
    PLUGIN_CODE Mime
    PRODUCT_NAME "MIME"
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Distortion)

juce_add_binary_data(MimeAssets SOURCES Resources/mime_guy.png)

target_sources(MIME PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    ${MIME_CORE_SOURCES})

target_compile_definitions(MIME PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(MIME
    PRIVATE
        MimeAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------- FILIPINA RAGE
juce_add_binary_data(RageAssets SOURCES Resources/filipina_rage.jpg)

juce_add_plugin(FilipinaRage
    COMPANY_NAME "qverbo"
    COMPANY_WEBSITE "https://github.com/qverbo/mime"
    BUNDLE_ID com.qverbo.filipinarage
    PLUGIN_MANUFACTURER_CODE Qvrb
    PLUGIN_CODE Fprg
    PRODUCT_NAME "FILIPINA RAGE"
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Distortion)

target_sources(FilipinaRage PRIVATE
    Source/rage/RageProcessor.cpp
    Source/rage/RageEditor.cpp
    Source/rage/RageEngine.cpp)

target_compile_definitions(FilipinaRage PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(FilipinaRage
    PRIVATE
        RageAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(MIME_BUILD_CLI)
    juce_add_console_app(rage_cli PRODUCT_NAME "rage_cli")
    target_sources(rage_cli PRIVATE tools/rage_cli.cpp Source/rage/RageEngine.cpp)
    target_compile_definitions(rage_cli PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(rage_cli PRIVATE juce::juce_audio_formats juce::juce_dsp juce::juce_data_structures juce::juce_recommended_config_flags)
endif()

if(MIME_BUILD_CLI)
    juce_add_console_app(mime_cli PRODUCT_NAME "mime_cli")
    target_sources(mime_cli PRIVATE tools/mime_cli.cpp ${MIME_CORE_SOURCES})
    target_compile_definitions(mime_cli PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_MP3AUDIOFORMAT=1)
    target_link_libraries(mime_cli PRIVATE juce::juce_audio_formats juce::juce_dsp juce::juce_data_structures juce::juce_recommended_config_flags)
endif()

cmake_minimum_required(VERSION 3.22)
project(ORYMYBass VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Runtime estatico en Windows: el usuario no necesita instalar Visual C++ Redistributable
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.6
  GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ORYMYBass
  COMPANY_NAME "YMY Sounds"
  PRODUCT_NAME "ORYMY Bass"
  BUNDLE_ID com.ymysounds.orymybass
  PLUGIN_MANUFACTURER_CODE Ymys
  PLUGIN_CODE Obas
  FORMATS VST3 Standalone
  VST3_CATEGORIES Fx Distortion
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon.png")

juce_add_binary_data(OrymyAssets SOURCES
  Assets/art.png
  Assets/logo_eye.png
  Assets/logo_ymy.png
  Assets/clef.png
  Assets/headstock.png)

target_sources(ORYMYBass PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_compile_definitions(ORYMYBass PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

if(MSVC)
  target_compile_options(ORYMYBass PRIVATE /utf-8)
endif()

target_link_libraries(ORYMYBass
  PRIVATE
    OrymyAssets
    juce::juce_audio_utils
    juce::juce_dsp
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

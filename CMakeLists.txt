cmake_minimum_required(VERSION 3.15)
project(CutOffPlugin VERSION 1.0.0)

# Fetch JUCE framework automatically from GitHub
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.5
)
FetchContent_MakeAvailable(juce)

# Define the Plugin
juce_add_plugin(CutOff
    COMPANY_NAME "Lineage Audio Labs"
    PLUGIN_MANUFACTURER_CODE "LinA"
    PLUGIN_CODE "CutO"
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Cut-Off"
    NEEDS_JUCE_HEADER TRUE  # <--- THIS IS THE FIX!
)

# Embed BOTH Logos into the BinaryData C++ class
juce_add_binary_data(CutOffBinaryData
    HEADER_NAME "BinaryData.h"
    NAMESPACE "BinaryData"
    SOURCES
        Assets/CutOffLogo.svg
        Assets/LineageLogo.svg
)

# Add Source Files
target_sources(CutOff PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

# Link Libraries
target_link_libraries(CutOff PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    CutOffBinaryData
)

target_compile_features(CutOff PRIVATE cxx_std_17)

# --- ADD THIS LINE AT THE VERY BOTTOM ---
target_compile_definitions(CutOff PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)
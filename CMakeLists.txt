cmake_minimum_required(VERSION 3.22)

project(EXS2DS VERSION 1.1.0)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_console_app(EXS2DS
    PRODUCT_NAME "EXS2DS"
    COMPANY_NAME "Decidedly"
    COMPANY_COPYRIGHT "Copyright 2021-2024 Decidedly"
    COMPANY_WEBSITE "https://decentsamples.com"
    COMPANY_EMAIL "dhilowitz@gmail.com"
)

juce_generate_juce_header(EXS2DS)

target_sources(EXS2DS PRIVATE
    Source/Main.cpp
    Source/DSPresetConverter/Source/DSPresetConverter.cpp
    Source/DSPresetConverter/Source/DSEXS24.cpp
)

target_include_directories(EXS2DS PRIVATE
    include
)

target_compile_definitions(EXS2DS PRIVATE
    JUCE_DISABLE_JUCE_VERSION_PRINTING=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(EXS2DS PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_core
    juce::juce_data_structures
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

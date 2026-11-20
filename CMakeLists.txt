cmake_minimum_required(VERSION 3.22)
project(LibertyGuitarBiosphere VERSION 0.5.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CTest)

# Unit tests for the JUCE-independent DSP core.
if(BUILD_TESTING)
    add_executable(BiosphereDSPTests Tests/DspSmoke.cpp)
    target_include_directories(BiosphereDSPTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(BiosphereDSPTests PRIVATE cxx_std_20)
    add_test(NAME BiosphereDSPTests COMMAND BiosphereDSPTests)

    add_executable(BiosphereDspRoutingTests Tests/DspRouting.cpp)
    target_include_directories(BiosphereDspRoutingTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(BiosphereDspRoutingTests PRIVATE cxx_std_20)
    add_test(NAME BiosphereDspRoutingTests COMMAND BiosphereDspRoutingTests)

    add_executable(BiosphereSoundEnginesProTests Tests/SoundEnginesPro.cpp)
    target_include_directories(BiosphereSoundEnginesProTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(BiosphereSoundEnginesProTests PRIVATE cxx_std_20)
    add_test(NAME BiosphereSoundEnginesProTests COMMAND BiosphereSoundEnginesProTests)

    add_executable(BiosphereGuitarIntelligenceTests Tests/GuitarIntelligence.cpp)
    target_include_directories(BiosphereGuitarIntelligenceTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(BiosphereGuitarIntelligenceTests PRIVATE cxx_std_20)
    add_test(NAME BiosphereGuitarIntelligenceTests COMMAND BiosphereGuitarIntelligenceTests)

    add_executable(BiosphereEvolutionTests Tests/EvolutionDynamics.cpp)
    target_include_directories(BiosphereEvolutionTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(BiosphereEvolutionTests PRIVATE cxx_std_20)
    add_test(NAME BiosphereEvolutionTests COMMAND BiosphereEvolutionTests)

    add_executable(BiosphereReactionTests Tests/ReactionRouting.cpp)
    target_include_directories(BiosphereReactionTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(BiosphereReactionTests PRIVATE cxx_std_20)
    add_test(NAME BiosphereReactionTests COMMAND BiosphereReactionTests)

    add_executable(BiosphereAnalysisTests Tests/AnalysisSmoke.cpp)
    target_include_directories(BiosphereAnalysisTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(BiosphereAnalysisTests PRIVATE cxx_std_20)
    add_test(NAME BiosphereAnalysisTests COMMAND BiosphereAnalysisTests)

    add_executable(BiosphereEcosystemLayoutTests Tests/EcosystemLayoutSmoke.cpp)
    target_include_directories(BiosphereEcosystemLayoutTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(BiosphereEcosystemLayoutTests PRIVATE cxx_std_20)
    add_test(NAME BiosphereEcosystemLayoutTests COMMAND BiosphereEcosystemLayoutTests)

    add_executable(BiosphereNavigationTests Tests/NavigationSmoke.cpp)
    target_include_directories(BiosphereNavigationTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(BiosphereNavigationTests PRIVATE cxx_std_20)
    add_test(NAME BiosphereNavigationTests COMMAND BiosphereNavigationTests)
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(BIOSPHERE_FORMATS AU VST3)
else()
    set(BIOSPHERE_FORMATS VST3)
endif()

juce_add_plugin(GuitarBiosphere
    VERSION 0.5.0
    COMPANY_NAME "Liberty Music Software"
    BUNDLE_ID "com.libertymusicsoftware.guitarbiosphere"
    PLUGIN_MANUFACTURER_CODE Lbms
    PLUGIN_CODE GBio
    PRODUCT_NAME "Guitar Biosphere"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS ${BIOSPHERE_FORMATS}
)

juce_generate_juce_header(GuitarBiosphere)

target_sources(GuitarBiosphere PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/Parameters/BiosphereParameters.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Ui/BiosphereTheme.h
    Source/Ui/Localization.h
    Source/Ui/PresetLibrary.h
    Source/Ui/PresetLibrary.cpp
    Source/Ui/PresetBrowser.h
    Source/Ui/PresetBrowser.cpp
    Source/Ui/EnginePanels.h
    Source/Ui/EnginePanels.cpp
    Source/Ui/EngineNavigation.h
    Source/Ui/EcosystemLayout.h
    Source/Ui/EcosystemView.h
    Source/Ui/EcosystemView.cpp
    Source/Dsp/ReactionMatrix.h
    Source/Dsp/EvolutionEngine.h
    Source/Dsp/GuitarPitchTracker.h
    Source/Dsp/BiosphereDSP.h
)

if(MSVC)
    target_compile_options(GuitarBiosphere PRIVATE /utf-8)
endif()

target_compile_definitions(GuitarBiosphere PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(GuitarBiosphere PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

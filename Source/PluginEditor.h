#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

class GuitarBiosphereAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit GuitarBiosphereAudioProcessorEditor(GuitarBiosphereAudioProcessor&);
    ~GuitarBiosphereAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Control
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    GuitarBiosphereAudioProcessor& processor_;
    std::array<Control, 11> controls_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarBiosphereAudioProcessorEditor)
};

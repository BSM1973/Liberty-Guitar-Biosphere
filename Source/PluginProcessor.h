#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "Ui/EngineNavigation.h"
#include "Dsp/BiosphereDSP.h"

class GuitarBiosphereAudioProcessor final : public juce::AudioProcessor
{
public:
    GuitarBiosphereAudioProcessor();
    ~GuitarBiosphereAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Initial"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState parameters;

    int getSelectedEditorPage() const noexcept
    {
        return selectedEditorPage_.load(std::memory_order_relaxed);
    }

    void setSelectedEditorPage(int index) noexcept
    {
        if (index >= 0 && index < biosphere::ui::EngineNavigationState::pageCount)
            selectedEditorPage_.store(index, std::memory_order_relaxed);
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameterLayout();
    biosphere::BiosphereDSP dsp_;
    std::atomic<int> selectedEditorPage_ { biosphere::ui::EngineNavigationState::defaultPage };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarBiosphereAudioProcessor)
};

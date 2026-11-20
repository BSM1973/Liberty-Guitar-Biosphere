#include "Ui/PresetLabState.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

class FakeProcessor final : public juce::AudioProcessor
{
public:
    FakeProcessor()
        : juce::AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          parameters(*this, nullptr, "TEST", makeLayout()) {}

    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout result;
        result.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID("gain", 1), "Gain",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        result.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID("mix", 1), "Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        return result;
    }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    const juce::String getName() const override { return "Lab Test"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Test"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    juce::AudioProcessorValueTreeState parameters;
};

void check(bool condition, const char* reason)
{
    if (!condition) throw std::runtime_error(reason);
}
float gain(FakeProcessor& p)
{
    return p.parameters.getRawParameterValue("gain")->load(
        std::memory_order_relaxed);
}
void write(FakeProcessor& p, float amount)
{
    p.parameters.getParameter("gain")->setValueNotifyingHost(amount);
}

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI init;
        FakeProcessor processor;
        biosphere::ui::PresetLabState lab(processor.parameters);
        check(lab.activeSlot() == 0, "A must be active by default");
        check(std::abs(gain(processor) - 0.5f) < 0.0001f,
              "Original factory value not loaded");
        write(processor, 0.72f);
        lab.commitNow();
        check(lab.undo(), "Couldn't Undo parameter edit");
        check(std::abs(gain(processor) - 0.5f) < 0.0001f,
              "Undo did not restore AudioProcessorValueTreeState");
        check(lab.redo(), "Couldn't Redo parameter edit");
        check(std::abs(gain(processor) - 0.72f) < 0.0001f,
              "Redo did not restore the edited value");

        lab.choose(1);
        check(lab.activeSlot() == 1 && std::abs(gain(processor) - 0.5f) < 0.0001f,
              "Independent B wasn't initialized from original state");
        write(processor, 0.83f);
        lab.commitNow();
        lab.choose(0);
        check(std::abs(gain(processor) - 0.72f) < 0.0001f,
              "Returning to A lost its edit");
        lab.choose(1);
        check(std::abs(gain(processor) - 0.83f) < 0.0001f,
              "Returning to B lost its edit");

        const auto project = processor.parameters.copyState();
        auto metadata = project.getChildWithName("PRESET_LAB");
        check(metadata.isValid() && metadata.getNumChildren() == 2,
              "The DAW project did not preserve both A/B states");
        FakeProcessor reopened;
        reopened.parameters.replaceState(project);
        biosphere::ui::PresetLabState recalled(reopened.parameters);
        check(recalled.activeSlot() == 1 && std::abs(gain(reopened) - 0.83f) < 0.0001f,
              "DAW recall failed to restore B");
        recalled.choose(0);
        check(std::abs(gain(reopened) - 0.72f) < 0.0001f,
              "DAW recall failed to retain inactive A snapshot");
        check(recalled.undo() && std::abs(gain(reopened) - 0.83f) < 0.0001f,
              "Undoing A/B switch did not restore previous active slot");
        check(recalled.redo() && std::abs(gain(reopened) - 0.72f) < 0.0001f,
              "Redoing A/B switch failed");

        std::cout << "PRESET LAB NATIVE PASS: real APVTS A/B, Undo/Redo and host state persistence\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "PRESET LAB NATIVE FAIL: " << e.what() << '\n';
        return 1;
    }
}

#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <array>
#include "Ui/EngineNavigation.h"
#include "Dsp/BiosphereDSP.h"
#include "Midi/MidiLearnRouter.h"

// DSP writes audio measurements, UI reads them at a fixed timer rate.
// All fields are atomic to avoid a JUCE audio/message-thread data race.
struct SharedAnalysisState
{
    std::atomic<float> envelope {0.0f}, attacks {0.0f};
    std::atomic<float> rhythmicActivity {0.0f}, memoryActivity {0.0f};
    std::atomic<float> tonalEnergy {0.0f}, rootHz {0.0f};
    std::atomic<float> pitchConfidence {0.0f};
    std::atomic<int> midiNote {-1};
    std::atomic<float> inputPeak {0.0f}, outputPeak {0.0f};
    std::atomic<float> hostBpm {0.0f};
    std::atomic<float> evolutionEnergy {0.0f};
    std::atomic<int> evolutionState {0};
};

class GuitarBiosphereAudioProcessor final : public juce::AudioProcessor, private juce::Timer
{
public:
    GuitarBiosphereAudioProcessor();
    ~GuitarBiosphereAudioProcessor() override { stopTimer(); }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
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
    SharedAnalysisState analysis;

    int getSelectedEditorPage() const noexcept
    {
        return selectedEditorPage_.load(std::memory_order_relaxed);
    }

    void setSelectedEditorPage(int index) noexcept
    {
        if (index >= 0 && index < biosphere::ui::EngineNavigationState::pageCount)
            selectedEditorPage_.store(index, std::memory_order_relaxed);
    }

    // MIDI CC learning is UI-only metadata, not an additional APVTS ID.
    // The message-thread timer commits host-visible parameter changes.
    void armMidiLearn(const juce::String& parameterId);
    void cancelMidiLearn() noexcept { midiRouter_.cancel(); }
    void clearMidiLearn(const juce::String& parameterId);
    void clearAllMidiLearn() noexcept { midiRouter_.clearAll(); }
    int midiLearnTarget() const noexcept { return midiRouter_.learnTarget(); }
    int midiAssignment(const juce::String& parameterId) const;
    juce::String midiParameterName(int index) const;
    int consumeMidiLearnNotification() noexcept
    { return midiRouter_.consumeLearnNotification(); }

    // Presentation state only — FR is the default; language is never an audio parameter.
    bool isEnglish() const noexcept { return english_.load(std::memory_order_relaxed); }
    void setEnglish(bool english) noexcept { english_.store(english, std::memory_order_relaxed); }

private:
    void timerCallback() override;
    int midiParameterIndex(const juce::String& id) const;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameterLayout();

    struct ReactionParameterHandles
    {
        std::atomic<float>* active = nullptr;
        std::atomic<float>* source = nullptr;
        std::atomic<float>* destination = nullptr;
        std::atomic<float>* amount = nullptr;
    };

    // Resolve the 32 route ID lookups once in the constructor, never while
    // processing an audio block (no dynamic JUCE String creation in callback).
    std::array<ReactionParameterHandles, 8> reactionHandles_ {};
    biosphere::BiosphereDSP dsp_;
    biosphere::midi::LearnRouter midiRouter_;
    std::array<juce::RangedAudioParameter*, biosphere::midi::LearnRouter::maximumParameters> midiParameters_ {};
    std::array<juce::String, biosphere::midi::LearnRouter::maximumParameters> midiParameterIds_ {};
    int midiParameterCount_ = 0;
    std::atomic<int> selectedEditorPage_ { biosphere::ui::EngineNavigationState::defaultPage };
    std::atomic<bool> english_ { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarBiosphereAudioProcessor)
};

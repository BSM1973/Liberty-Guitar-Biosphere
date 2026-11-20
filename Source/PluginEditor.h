#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>
#include "PluginProcessor.h"
#include "Ui/BiosphereTheme.h"
#include "Ui/EcosystemView.h"
#include "Ui/EnginePanels.h"
#include "Ui/EngineNavigation.h"
#include "Ui/PresetLibrary.h"
#include "Ui/PresetBrowser.h"
#include "Ui/PresetLabState.h"
#include "Ui/Localization.h"

class GuitarBiosphereAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit GuitarBiosphereAudioProcessorEditor(GuitarBiosphereAudioProcessor&);
    ~GuitarBiosphereAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent&) override;
    void showMidiMenu(juce::Component&, const juce::String& parameterId);
    void updateMidiStatus();
    void setLanguage(bool english);
    void refreshPresetSelector();
    void openBrowser();
    void chooseRelativePreset(int increment);
    void loadCurrentPreset();
    void refreshPresetLabButtons();
    struct GlobalControl
    {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void selectEngine(int index);
    void initialiseGlobalControl(int index, const juce::String& label,
                                 const juce::String& parameterId,
                                 float minimum, float maximum, float initial,
                                 const juce::String& suffix);

    biosphere::ui::LookAndFeel theme_;
    GuitarBiosphereAudioProcessor& processor_;
    biosphere::ui::PresetLibrary presetLibrary_;
    biosphere::ui::PresetBrowser browser_;
    biosphere::ui::PresetLabState presetLab_;

    biosphere::ui::EcosystemView ecosystem_;
    biosphere::ui::AnalysisPanel analysis_;

    biosphere::ui::PulsePanel pulse_;
    biosphere::ui::HarmonicPanel harmonic_;
    biosphere::ui::MutationPanel mutation_;
    biosphere::ui::ReactionPanel reaction_;
    biosphere::ui::MemoryPanel memory_;
    biosphere::ui::EvolutionPanel evolution_;
    std::array<biosphere::ui::EnginePanel*, 6> pages_ {};

    std::array<GlobalControl, 10> globals_ {};
    juce::ComboBox preset_;
    juce::TextButton previous_ { "<" }, next_ { ">" }, library_ { "..." };
    juce::TextButton compareA_ { "A" }, compareB_ { "B" };
    juce::TextButton undo_ { "ANNULER" }, redo_ { "REFAIRE" };
    juce::TextButton french_ { "FR" }, english_ { "EN" };
    juce::Label midiLearnStatus_;
    int lastMidiLearned_ = -1;
    juce::TextButton sync_ { "Synchro hote" }, freeze_ { "Gel global" },
                     bypass_ { "Bypass global" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment_;

    biosphere::ui::EngineNavigationState navigation_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarBiosphereAudioProcessorEditor)
};

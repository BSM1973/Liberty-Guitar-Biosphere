#pragma once

#include <JuceHeader.h>
#include <memory>
#include <array>
#include <vector>
#include "BiosphereTheme.h"
#include "Localization.h"

namespace biosphere::ui
{
enum class ControlKind { knob, choice, switchButton };

struct ControlSpec
{
    juce::String name;
    juce::String parameterId; // Stable host-automatable APVTS ID.
    ControlKind kind = ControlKind::knob;
    float min = 0.0f, max = 1.0f, initial = 0.5f;
    juce::String suffix = " %";
    juce::StringArray choices;

    static ControlSpec dial(const char* label, const char* parameter, float minimum,
                            float maximum, float initialValue, const char* unit)
    {
        return { juce::String::fromUTF8(label), parameter, ControlKind::knob,
                 minimum, maximum, initialValue, juce::String::fromUTF8(unit), {} };
    }
    static ControlSpec menu(const char* label, const char* parameter, std::initializer_list<const char*> options)
    {
        ControlSpec result;
        result.name = juce::String::fromUTF8(label);
        result.parameterId = parameter;
        result.kind = ControlKind::choice;
        for (const auto* option : options)
            result.choices.add(juce::String::fromUTF8(option));
        return result;
    }
    static ControlSpec toggle(const char* label, const char* parameter, bool initiallyOn = false)
    {
        ControlSpec result;
        result.name = juce::String::fromUTF8(label);
        result.parameterId = parameter;
        result.kind = ControlKind::switchButton;
        result.initial = initiallyOn ? 1.0f : 0.0f;
        return result;
    }
};

class EnginePanel : public juce::Component
{
public:
    EnginePanel(juce::AudioProcessorValueTreeState& state, juce::String number,
                juce::String title, juce::String summary, juce::Colour accent,
                std::vector<ControlSpec> specification, bool dspAvailable);
    ~EnginePanel() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    void setLanguage(bool english);
    const juce::String& getTitle() const noexcept { return title_; }

private:
    struct Slot
    {
        ControlSpec spec;
        juce::Label caption;
        juce::Slider dial;
        juce::ComboBox choice;
        juce::TextButton toggle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> choiceAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachment;
    };

    struct RouteControls
    {
        juce::TextButton enabled { "●" };
        juce::ComboBox source, destination;
        juce::Slider amount;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttachment, destinationAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    };

    juce::String number_, title_, summary_;
    juce::Colour accent_;
    bool dspAvailable_ = false;
    bool english_ = false;
    std::vector<std::unique_ptr<Slot>> slots_;
    juce::TextButton active_ { "ACTIF" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> activeAttachment_;
    std::array<std::unique_ptr<RouteControls>, 8> routes_ {};
};

class PulsePanel final : public EnginePanel
{
public:
    explicit PulsePanel(juce::AudioProcessorValueTreeState&);
};
class HarmonicPanel final : public EnginePanel
{
public:
    explicit HarmonicPanel(juce::AudioProcessorValueTreeState&);
};
class MutationPanel final : public EnginePanel
{
public:
    explicit MutationPanel(juce::AudioProcessorValueTreeState&);
};
class ReactionPanel final : public EnginePanel
{
public:
    explicit ReactionPanel(juce::AudioProcessorValueTreeState&);
};
class MemoryPanel final : public EnginePanel
{
public:
    explicit MemoryPanel(juce::AudioProcessorValueTreeState&);
};
class EvolutionPanel final : public EnginePanel
{
public:
    explicit EvolutionPanel(juce::AudioProcessorValueTreeState&);
};
} // namespace biosphere::ui

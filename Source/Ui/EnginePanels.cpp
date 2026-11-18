#include "EnginePanels.h"
#include <algorithm>
#include <cmath>

namespace biosphere::ui
{
EnginePanel::EnginePanel(juce::AudioProcessorValueTreeState& state, juce::String number,
                         juce::String title, juce::String summary, juce::Colour accent,
                         std::vector<ControlSpec> specification, bool dspAvailable)
    : number_(std::move(number)), title_(std::move(title)), summary_(std::move(summary)),
      accent_(accent), dspAvailable_(dspAvailable)
{
    for (const auto& spec : specification)
    {
        auto slot = std::make_unique<Slot>();
        slot->spec = spec;
        slot->caption.setText(spec.name, juce::dontSendNotification);
        slot->caption.setJustificationType(juce::Justification::centred);
        slot->caption.setFont(juce::Font(juce::FontOptions(11.5f)));
        slot->caption.setColour(juce::Label::textColourId, white);
        addAndMakeVisible(slot->caption);

        if (spec.kind == ControlKind::knob)
        {
            slot->dial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slot->dial.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                           juce::MathConstants<float>::pi * 2.75f, true);
            slot->dial.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 85, 20);
            slot->dial.setColour(juce::Slider::textBoxTextColourId, gold);
            slot->dial.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            slot->dial.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            slot->dial.setRange(spec.min, spec.max, 0.001);
            slot->dial.setValue(spec.initial, juce::dontSendNotification);
            const auto suffix = spec.suffix;
            slot->dial.textFromValueFunction = [suffix, max = spec.max](double value)
            {
                const bool percent = suffix == " %";
                const double display = percent && max <= 1.0f ? value * 100.0 : value;
                return juce::String(display, (percent || suffix == " Hz") ? 0 : 1) + suffix;
            };
            addAndMakeVisible(slot->dial);

            if (spec.parameterId.isNotEmpty() && state.getParameter(spec.parameterId) != nullptr)
            {
                slot->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    state, spec.parameterId, slot->dial);
            }
            else
            {
                // Phase 1: visible, but intentionally not manipulable until its DSP parameter exists.
                slot->dial.setEnabled(false);
                slot->dial.setTooltip("Paramètre prévu pour la phase 2 : connexion au moteur DSP.");
            }
        }
        else if (spec.kind == ControlKind::choice)
        {
            for (int i = 0; i < spec.choices.size(); ++i)
                slot->choice.addItem(spec.choices[i], i + 1);
            slot->choice.setSelectedId(1, juce::dontSendNotification);
            slot->choice.setJustificationType(juce::Justification::centred);
            slot->choice.setEnabled(false);
            slot->choice.setTooltip("Sélecteur à relier au DSP en phase 2.");
            addAndMakeVisible(slot->choice);
        }
        else
        {
            slot->toggle.setButtonText(spec.initial > 0.5f ? "Oui" : "Non");
            slot->toggle.setEnabled(false);
            slot->toggle.setTooltip("Interrupteur à relier au DSP en phase 2.");
            addAndMakeVisible(slot->toggle);
        }
        slots_.push_back(std::move(slot));
    }
}

void EnginePanel::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced(1.0f);
    roundedPanel(g, area, 10.0f, accent_.withAlpha(0.80f));

    g.setColour(accent_);
    g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    g.drawText(number_ + "   " + title_, 19, 10, 235, 31, juce::Justification::centredLeft);

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(11.5f)));
    g.drawFittedText(summary_, 260, 12, getWidth() - 417, 28,
                     juce::Justification::centredLeft, 1);

    auto status = juce::Rectangle<float>(static_cast<float>(getWidth() - 144), 11.0f, 124.0f, 29.0f);
    g.setColour(dspAvailable_ ? raised.brighter(0.28f) : raised);
    g.fillRoundedRectangle(status, 5.0f);
    g.setColour(dspAvailable_ ? gold : muted);
    g.drawRoundedRectangle(status, 5.0f, 1.0f);
    g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
    g.drawText(dspAvailable_ ? "DSP PARTIEL" : "À RELIER", status.toNearestInt(),
               juce::Justification::centred);

    g.setColour(border.withAlpha(0.60f));
    g.drawLine(20.0f, 47.0f, static_cast<float>(getWidth() - 20), 47.0f, 0.9f);

    // The eight REACTION routes are faithfully laid out but inactive until Phase 2/3.
    if (title_ == "REACTION")
    {
        const int left = 382;
        const int usableWidth = getWidth() - left - 25;
        const int columnWidth = usableWidth / 2 - 7;
        g.setColour(muted);
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText("ROUTES D'INTERACTION", left, 54, 240, 16, juce::Justification::left);
        const juce::StringArray sources { "Attaque", "Enveloppe", "Pulse", "Harmonique",
                                         "Mutation", "Mémoire", "Évolution", "Silence" };
        const juce::StringArray destinations { "Grain", "Pulse", "Harmonique", "Mémoire",
                                              "Résonance", "Mouvement", "Granulaire", "Pulse" };
        for (int index = 0; index < 8; ++index)
        {
            const int column = index / 4;
            const int row = index % 4;
            const int xx = left + column * (columnWidth + 14);
            const int yy = 73 + row * 31;
            const auto frame = juce::Rectangle<float>(static_cast<float>(xx), static_cast<float>(yy),
                                                      static_cast<float>(columnWidth), 27.0f);
            g.setColour(raised.withAlpha(0.78f));
            g.fillRoundedRectangle(frame, 4.0f);
            g.setColour(border.withAlpha(0.85f));
            g.drawRoundedRectangle(frame, 4.0f, 0.75f);
            g.setColour(gold.withAlpha(0.70f));
            g.setFont(juce::Font(juce::FontOptions(10.5f)));
            g.drawText(juce::String(index + 1).paddedLeft('0', 2), xx + 7, yy + 3, 24, 21,
                       juce::Justification::left);
            g.setColour(white.withAlpha(0.72f));
            g.drawFittedText(sources[index] + "    →    " + destinations[index],
                             xx + 37, yy + 3, columnWidth - 44, 21,
                             juce::Justification::centredLeft, 1);
        }
    }
}

void EnginePanel::resized()
{
    const bool reaction = title_ == "REACTION";
    const int left = 19;
    const int usable = reaction ? 350 : getWidth() - left * 2;
    const int count = static_cast<int>(slots_.size());
    if (count == 0) return;

    const float cellWidth = static_cast<float>(usable) / static_cast<float>(count);
    for (int i = 0; i < count; ++i)
    {
        auto& slot = *slots_[static_cast<std::size_t>(i)];
        const int x = left + static_cast<int>(cellWidth * static_cast<float>(i));
        const int cell = static_cast<int>(cellWidth);
        slot.caption.setBounds(x, 60, cell, 22);
        if (slot.spec.kind == ControlKind::knob)
        {
            const int knobWidth = std::min(94, cell - 5);
            slot.dial.setBounds(x + (cell - knobWidth) / 2, 85, knobWidth, 118);
        }
        else if (slot.spec.kind == ControlKind::choice)
            slot.choice.setBounds(x + 5, 119, cell - 10, 32);
        else
            slot.toggle.setBounds(x + 7, 119, cell - 14, 32);
    }
}

PulsePanel::PulsePanel(juce::AudioProcessorValueTreeState& state)
    : EnginePanel(state, "01", "PULSE", "Le geste devient rythme : attaques, pulsations et échos vivants.",
                  blue, {
        ControlSpec::dial("Sensibilité", "", 0, 1, 0.55f, " %"),
        ControlSpec::dial("Cadence libre", "rate", 0.1f, 12, 2, " Hz"),
        ControlSpec::menu("Division", {"1/4", "1/8", "1/8T", "1/16", "1/16T"}),
        ControlSpec::dial("Profondeur", "pulse", 0, 1, 0.35f, " %"),
        ControlSpec::toggle("Synchro", true),
        ControlSpec::dial("Swing", "", 0, 1, 0.08f, " %"),
        ControlSpec::dial("Réinjection", "", 0, 1, 0.24f, " %"),
        ControlSpec::dial("Largeur stéréo", "", 0, 1, 0.60f, " %"),
        ControlSpec::dial("Dosage", "", 0, 1, 0.52f, " %")
    }, true) {}

HarmonicPanel::HarmonicPanel(juce::AudioProcessorValueTreeState& state)
    : EnginePanel(state, "02", "HARMONIC LIFE", "Résonances harmoniques excitées par votre guitare.",
                  gold, {
        ControlSpec::dial("Fondamentale", "root", 40, 500, 165, " Hz"),
        ControlSpec::menu("Harmoniques", {"Quintes", "Octaves", "Tierces", "Suspendues"}),
        ControlSpec::dial("Résonance", "", 0, 1, 0.55f, " %"),
        ControlSpec::dial("Déclin", "", 0.1f, 6, 2.4f, " s"),
        ControlSpec::dial("Amortissement", "", 0, 1, 0.48f, " %"),
        ControlSpec::dial("Mouvement", "", 0, 1, 0.18f, " %"),
        ControlSpec::dial("Dispersion", "", 0, 1, 0.65f, " %"),
        ControlSpec::dial("Dosage", "harmonic", 0, 1, 0.38f, " %"),
        ControlSpec::toggle("Suivi mono")
    }, true) {}

MutationPanel::MutationPanel(juce::AudioProcessorValueTreeState& state)
    : EnginePanel(state, "03", "MUTATION", "Des fragments de votre jeu se transforment en matière sonore.",
                  blue, {
        ControlSpec::dial("Taille du grain", "grain", 35, 220, 110, " ms"),
        ControlSpec::dial("Densité", "", 1, 40, 12, " gr/s"),
        ControlSpec::menu("Caractère", {"Organique", "Cristal", "Métallique", "Nuage"}),
        ControlSpec::dial("Mouvement", "", 0, 1, 0.25f, " %"),
        ControlSpec::dial("Hauteur", "", -12, 12, 0, " st"),
        ControlSpec::dial("Aléatoire", "", 0, 1, 0.20f, " %"),
        ControlSpec::dial("Stéréo", "", 0, 1, 0.65f, " %"),
        ControlSpec::dial("Dosage", "mutation", 0, 1, 0.28f, " %")
    }, true) {}

ReactionPanel::ReactionPanel(juce::AudioProcessorValueTreeState& state)
    : EnginePanel(state, "04", "REACTION", "Huit interactions entre les moteurs.",
                  gold, {
        ControlSpec::dial("Réactivité", "", 0, 1, 0.55f, " %"),
        ControlSpec::dial("Interaction", "", 0, 1, 0.50f, " %"),
        ControlSpec::dial("Sensibilité", "", 0, 1, 0.55f, " %"),
        ControlSpec::dial("Réponse", "", 30, 500, 120, " ms"),
        ControlSpec::dial("Complexité", "", 0, 1, 0.65f, " %")
    }, false) {}

MemoryPanel::MemoryPanel(juce::AudioProcessorValueTreeState& state)
    : EnginePanel(state, "05", "MEMORY", "Votre jeu récent revient en fragments, souvenirs et notes fantômes.",
                  gold, {
        ControlSpec::dial("Durée mémoire", "", 0.5f, 3.0f, 3.0f, " s"),
        ControlSpec::dial("Capture", "", 0, 1, 0.80f, " %"),
        ControlSpec::dial("Rappel", "", 0, 1, 0.48f, " %"),
        ControlSpec::menu("Répétition", {"1/4", "1/8", "1/16", "Libre"}),
        ControlSpec::dial("Inversion", "", 0, 1, 0.25f, " %"),
        ControlSpec::dial("Réinjection", "feedback", 0, 0.95f, 0.40f, " %"),
        ControlSpec::dial("Fondu", "", 5, 300, 45, " ms"),
        ControlSpec::dial("Dosage", "memory", 0, 1, 0.30f, " %"),
        ControlSpec::toggle("Gel mémoire")
    }, true) {}

EvolutionPanel::EvolutionPanel(juce::AudioProcessorValueTreeState& state)
    : EnginePanel(state, "06", "EVOLUTION", "Un écosystème déterministe qui se développe avec votre jeu.",
                  blue, {
        ControlSpec::dial("Cadence", "", 0.25f, 4, 1, " x"),
        ControlSpec::dial("Intensité", "", 0, 1, 0.60f, " %"),
        ControlSpec::dial("Sensibilité", "", 0, 1, 0.55f, " %"),
        ControlSpec::dial("Stabilité", "", 0, 1, 0.65f, " %"),
        ControlSpec::dial("Complexité", "", 0, 1, 0.65f, " %"),
        ControlSpec::dial("Récupération", "", 0.25f, 10, 4, " s")
    }, false) {}
} // namespace biosphere::ui

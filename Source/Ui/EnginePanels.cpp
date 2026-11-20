#include "EnginePanels.h"
#include <algorithm>
#include <cmath>

namespace biosphere::ui
{
EnginePanel::EnginePanel(juce::AudioProcessorValueTreeState& state, juce::String number,
                         juce::String title, const char* summaryUtf8, juce::Colour accent,
                         std::vector<ControlSpec> specification, bool dspAvailable)
    : number_(std::move(number)), title_(std::move(title)), summary_(juce::String::fromUTF8(summaryUtf8)),
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

            jassert(state.getParameter(spec.parameterId) != nullptr);
            slot->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                state, spec.parameterId, slot->dial);
            slot->dial.setTooltip(juce::String::fromUTF8("Paramètre : ") + spec.parameterId
                + (dspAvailable_ ? juce::String::fromUTF8(". Contrôle le moteur DSP natif.")
                                 : juce::String::fromUTF8(". Moteur de modulation à développer.")));
        }
        else if (spec.kind == ControlKind::choice)
        {
            for (int i = 0; i < spec.choices.size(); ++i)
                slot->choice.addItem(spec.choices[i], i + 1);
            slot->choice.setSelectedId(1, juce::dontSendNotification);
            slot->choice.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(slot->choice);
            jassert(state.getParameter(spec.parameterId) != nullptr);
            slot->choiceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                state, spec.parameterId, slot->choice);
        }
        else
        {
            slot->toggle.setButtonText("Oui / Non");
            slot->toggle.setClickingTogglesState(true);
            slot->toggle.setToggleState(spec.initial > 0.5f, juce::dontSendNotification);
            slot->toggle.setColour(juce::TextButton::buttonOnColourId, gold.darker(0.65f));
            addAndMakeVisible(slot->toggle);
            jassert(state.getParameter(spec.parameterId) != nullptr);
            slot->buttonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                state, spec.parameterId, slot->toggle);
        }

        slots_.push_back(std::move(slot));
    }

    const juce::String activeId = (title_ == "PULSE") ? "pulse.active"
        : (title_ == "HARMONIC LIFE") ? "harm.active"
        : (title_ == "MUTATION") ? "mut.active"
        : (title_ == "REACTION") ? "react.active"
        : (title_ == "MEMORY") ? "mem.active" : "evo.active";
    active_.setClickingTogglesState(true);
    active_.setToggleState(true, juce::dontSendNotification);
    active_.setColour(juce::TextButton::buttonOnColourId, gold.darker(0.68f));
    active_.setColour(juce::TextButton::textColourOnId, white);
    active_.setColour(juce::TextButton::buttonColourId, raised);
    active_.setTooltip(dspAvailable_
        ? juce::String::fromUTF8("Active ou coupe ce moteur DSP (paramètre mémorisé et automatisable).")
        : juce::String::fromUTF8("Activé dans les réglages, moteur audio à développer."));
    addAndMakeVisible(active_);
    jassert(state.getParameter(activeId) != nullptr);
    activeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        state, activeId, active_);

    if (title_ == "REACTION")
    {
        const juce::StringArray sourceNames {
            "Attaque", "Enveloppe", "Pulse", "Harmonique",
            "Mutation", juce::String::fromUTF8("Mémoire"), juce::String::fromUTF8("Évolution"), "Silence"
        };
        const juce::StringArray destinationNames {
            juce::String::fromUTF8("G: Densité"), juce::String::fromUTF8("P: Profondeur"), juce::String::fromUTF8("H: Mouvement"), juce::String::fromUTF8("M: Rappel"),
            juce::String::fromUTF8("H: Résonance"), juce::String::fromUTF8("G: Mouvement"), juce::String::fromUTF8("M: Réinjection"), juce::String::fromUTF8("P: Cadence")
        };
        constexpr float amounts[] { .45f, .30f, .32f, .28f, .18f, .22f, .24f, -.25f };
        for (int i = 0; i < 8; ++i)
        {
            auto route = std::make_unique<RouteControls>();
            const auto prefix = "react.route" + juce::String(i + 1);
            route->enabled.setButtonText("R");
            route->enabled.setClickingTogglesState(true);
            route->enabled.setToggleState(true, juce::dontSendNotification);
            route->enabled.setColour(juce::TextButton::buttonColourId, raised);
            route->enabled.setColour(juce::TextButton::buttonOnColourId, gold.darker(0.60f));
            addAndMakeVisible(route->enabled);
            for (int choice = 0; choice < sourceNames.size(); ++choice)
                route->source.addItem(sourceNames[choice], choice + 1);
            for (int choice = 0; choice < destinationNames.size(); ++choice)
                route->destination.addItem(destinationNames[choice], choice + 1);
            route->source.setSelectedId(i + 1, juce::dontSendNotification);
            route->source.setTooltip(juce::String::fromUTF8(
                "Source réelle issue du signal DSP. ÉVOLUTION restera silencieuse tant que son moteur n'existe pas."));
            route->destination.setSelectedId(i + 1, juce::dontSendNotification);
            addAndMakeVisible(route->source);
            addAndMakeVisible(route->destination);
            route->amount.setSliderStyle(juce::Slider::LinearHorizontal);
            route->amount.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            route->amount.setRange(-1, 1, 0.01);
            route->amount.setValue(amounts[i], juce::dontSendNotification);
            route->amount.setColour(juce::Slider::trackColourId, gold);
            route->amount.setColour(juce::Slider::thumbColourId, white);
            route->amount.setTooltip("Montant bipolaire -100 % à +100 % (automatisable).");
            addAndMakeVisible(route->amount);

            route->enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                state, prefix + ".active", route->enabled);
            route->sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                state, prefix + ".source", route->source);
            route->destinationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                state, prefix + ".destination", route->destination);
            route->amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                state, prefix + ".amount", route->amount);
            routes_[static_cast<std::size_t>(i)] = std::move(route);
        }
    }
}

void EnginePanel::setLanguage(bool english)
{
    english_ = english;
    active_.setButtonText(tr("ACTIF", english_));
    for (auto& slot : slots_)
    {
        slot->caption.setText(tr(slot->spec.name, english_), juce::dontSendNotification);
        if (slot->spec.kind == ControlKind::switchButton)
            slot->toggle.setButtonText(tr("Oui / Non", english_));
        else if (slot->spec.kind == ControlKind::choice)
            for (int index = 0; index < slot->spec.choices.size(); ++index)
                slot->choice.changeItemText(index + 1, tr(slot->spec.choices[index], english_));
    }
    if (title_ == "REACTION")
    {
        const juce::StringArray sources {
            "Attaque", "Enveloppe", "Pulse", "Harmonique",
            "Mutation", juce::String::fromUTF8("Mémoire"), juce::String::fromUTF8("Évolution"), "Silence"
        };
        const juce::StringArray destinations {
            juce::String::fromUTF8("G: Densité"), juce::String::fromUTF8("P: Profondeur"), juce::String::fromUTF8("H: Mouvement"), juce::String::fromUTF8("M: Rappel"),
            juce::String::fromUTF8("H: Résonance"), juce::String::fromUTF8("G: Mouvement"), juce::String::fromUTF8("M: Réinjection"), juce::String::fromUTF8("P: Cadence")
        };
        for (auto& route : routes_)
        {
            for (int i = 0; i < sources.size(); ++i)
                route->source.changeItemText(i + 1, tr(sources[i], english_));
            for (int i = 0; i < destinations.size(); ++i)
                route->destination.changeItemText(i + 1, tr(destinations[i], english_));
        }
    }
    repaint();
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
    g.drawFittedText(tr(summary_, english_), 260, 12, getWidth() - 417, 28,
                     juce::Justification::centredLeft, 1);

    g.setColour(border.withAlpha(0.60f));
    g.drawLine(20.0f, 47.0f, static_cast<float>(getWidth() - 20), 47.0f, 0.9f);

    if (title_ == "REACTION")
    {
        g.setColour(muted);
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText("ROUTES D'INTERACTION", 448, 54, 260, 16, juce::Justification::left);
        const int columnWidth = (getWidth() - 448 - 25) / 2 - 7;
        for (int i = 0; i < 8; ++i)
        {
            const int x = 448 + (i / 4) * (columnWidth + 14);
            const int y = 73 + (i % 4) * 31;
            g.setColour(border.withAlpha(0.60f));
            g.drawRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                                   static_cast<float>(columnWidth), 27.0f, 4.0f, 0.75f);
        }
    }
}

void EnginePanel::resized()
{
    active_.setBounds(getWidth() - 144, 11, 124, 29);
    if (title_ == "REACTION")
    {
        const int left = 448;
        const int columnWidth = (getWidth() - left - 25) / 2 - 7;
        for (int i = 0; i < 8; ++i)
        {
            auto& route = *routes_[static_cast<std::size_t>(i)];
            const int x = left + (i / 4) * (columnWidth + 14);
            const int y = 73 + (i % 4) * 31;
            route.enabled.setBounds(x + 4, y + 3, 22, 21);
            route.source.setBounds(x + 29, y + 3, 96, 21);
            route.destination.setBounds(x + 138, y + 3, 98, 21);
            route.amount.setBounds(x + 244, y + 3, columnWidth - 249, 21);
        }
    }
    const bool reaction = title_ == "REACTION";
    const int left = 19;
    const int usable = reaction ? 415 : getWidth() - left * 2;
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
        ControlSpec::dial("Sensibilité", "pulse.sensitivity", 0, 1, 0.55f, " %"),
        ControlSpec::dial("Cadence libre", "rate", 0.1f, 12, 2, " Hz"),
        ControlSpec::menu("Division", "pulse.division", {"1/4", "1/8", "1/8T", "1/16", "1/16T"}),
        ControlSpec::dial("Profondeur", "pulse", 0, 1, 0.35f, " %"),
        ControlSpec::toggle("Synchro", "pulse.sync", true),
        ControlSpec::dial("Swing", "pulse.swing", 0, 1, 0.08f, " %"),
        ControlSpec::dial("Réinjection", "pulse.feedback", 0, 1, 0.24f, " %"),
        ControlSpec::dial("Largeur stéréo", "pulse.width", 0, 1, 0.60f, " %"),
        ControlSpec::dial("Dosage", "pulse.mix", 0, 1, 0.52f, " %")
    }, true) {}

HarmonicPanel::HarmonicPanel(juce::AudioProcessorValueTreeState& state)
    : EnginePanel(state, "02", "HARMONIC LIFE", "Résonances harmoniques excitées par votre guitare.",
                  gold, {
        ControlSpec::dial("Fondamentale", "root", 40, 500, 165, " Hz"),
        ControlSpec::menu("Harmoniques", "harm.mode", {"Quintes", "Octaves", "Tierces", "Suspendues"}),
        ControlSpec::dial("Résonance", "harm.resonance", 0, 1, 0.55f, " %"),
        ControlSpec::dial("Déclin", "harm.decay", 0.1f, 6, 2.4f, " s"),
        ControlSpec::dial("Amortissement", "harm.damping", 0, 1, 0.48f, " %"),
        ControlSpec::dial("Mouvement", "harm.motion", 0, 1, 0.18f, " %"),
        ControlSpec::dial("Dispersion", "harm.spread", 0, 1, 0.65f, " %"),
        ControlSpec::dial("Dosage", "harmonic", 0, 1, 0.38f, " %"),
        ControlSpec::toggle("Suivi mono", "harm.mono")
    }, true) {}

MutationPanel::MutationPanel(juce::AudioProcessorValueTreeState& state)
    : EnginePanel(state, "03", "MUTATION", "Des fragments de votre jeu se transforment en matière sonore.",
                  blue, {
        ControlSpec::dial("Taille du grain", "grain", 35, 220, 110, " ms"),
        ControlSpec::dial("Densité", "mut.density", 1, 40, 12, " gr/s"),
        ControlSpec::menu("Caractère", "mut.character", {"Organique", "Cristal", "Métallique", "Nuage"}),
        ControlSpec::dial("Mouvement", "mut.motion", 0, 1, 0.25f, " %"),
        ControlSpec::dial("Hauteur", "mut.pitch", -12, 12, 0, " st"),
        ControlSpec::dial("Aléatoire", "mut.random", 0, 1, 0.20f, " %"),
        ControlSpec::dial("Stéréo", "mut.stereo", 0, 1, 0.65f, " %"),
        ControlSpec::dial("Dosage", "mutation", 0, 1, 0.28f, " %")
    }, true) {}

ReactionPanel::ReactionPanel(juce::AudioProcessorValueTreeState& state)
    : EnginePanel(state, "04", "REACTION", "Huit interactions entre les moteurs.",
                  gold, {
        ControlSpec::dial("Réactivité", "react.reactivity", 0, 1, 0.55f, " %"),
        ControlSpec::dial("Interaction", "react.interaction", 0, 1, 0.50f, " %"),
        ControlSpec::dial("Sensibilité", "react.sensitivity", 0, 1, 0.55f, " %"),
        ControlSpec::dial("Réponse", "react.response", 30, 500, 120, " ms"),
        ControlSpec::dial("Complexité", "react.complexity", 0, 1, 0.65f, " %")
    }, true) {}

MemoryPanel::MemoryPanel(juce::AudioProcessorValueTreeState& state)
    : EnginePanel(state, "05", "MEMORY", "Votre jeu récent revient en fragments, souvenirs et notes fantômes.",
                  gold, {
        ControlSpec::dial("Durée mémoire", "mem.length", 0.5f, 3.0f, 3.0f, " s"),
        ControlSpec::dial("Capture", "mem.capture", 0, 1, 0.80f, " %"),
        ControlSpec::dial("Rappel", "mem.recall", 0, 1, 0.48f, " %"),
        ControlSpec::menu("Répétition", "mem.repeat", {"1/4", "1/8", "1/16", "Libre"}),
        ControlSpec::dial("Inversion", "mem.reverse", 0, 1, 0.25f, " %"),
        ControlSpec::dial("Réinjection", "feedback", 0, 0.95f, 0.40f, " %"),
        ControlSpec::dial("Fondu", "mem.fade", 5, 300, 45, " ms"),
        ControlSpec::dial("Dosage", "memory", 0, 1, 0.30f, " %"),
        ControlSpec::toggle("Gel mémoire", "mem.freeze")
    }, true) {}

EvolutionPanel::EvolutionPanel(juce::AudioProcessorValueTreeState& state)
    : EnginePanel(state, "06", "EVOLUTION", "Un écosystème déterministe qui se développe avec votre jeu.",
                  blue, {
        ControlSpec::dial("Cadence", "evo.rate", 0.25f, 4, 1, " x"),
        ControlSpec::dial("Intensité", "evo.intensity", 0, 1, 0.60f, " %"),
        ControlSpec::dial("Sensibilité", "evo.sensitivity", 0, 1, 0.55f, " %"),
        ControlSpec::dial("Stabilité", "evo.stability", 0, 1, 0.65f, " %"),
        ControlSpec::dial("Complexité", "evo.complexity", 0, 1, 0.65f, " %"),
        ControlSpec::dial("Récupération", "evo.recovery", 0.25f, 10, 4, " s")
    }, true) {}
} // namespace biosphere::ui

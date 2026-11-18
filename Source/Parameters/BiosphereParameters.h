#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

// Stable public parameter IDs. Never rename the first 11 legacy IDs:
// existing AU/VST3 sessions already save them.
namespace biosphere::params
{
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using Float = juce::AudioParameterFloat;
    using Bool = juce::AudioParameterBool;
    using Choice = juce::AudioParameterChoice;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> items;
    items.reserve(130);

    auto number = [&](const char* id, const char* title, float min, float max,
                      float initial, float step = 0.01f)
    {
        items.push_back(std::make_unique<Float>(
            juce::ParameterID(id, 1), juce::String::fromUTF8(title),
            juce::NormalisableRange<float>(min, max, step), initial));
    };
    auto toggle = [&](const char* id, const char* title, bool initial)
    {
        items.push_back(std::make_unique<Bool>(
            juce::ParameterID(id, 1), juce::String::fromUTF8(title), initial));
    };
    auto menu = [&](const char* id, const char* title, const juce::StringArray& choices,
                    int initial)
    {
        items.push_back(std::make_unique<Choice>(
            juce::ParameterID(id, 1), juce::String::fromUTF8(title), choices, initial));
    };

    // Preserve ALL legacy ranges, defaults and host parameter IDs for saved projects.
    number("input",       "Input (dB)",          -24,    18, 0,    0.1f);
    number("output",      "Output (dB)",         -24,    12, 0,    0.1f);
    number("mix",         "Master Mix",            0,     1, 0.65f);
    number("pulse",       "Pulse",                 0,     1, 0.35f);
    number("harmonic",    "Harmonic Life",         0,     1, 0.30f);
    number("mutation",    "Mutation",              0,     1, 0.20f);
    number("memory",      "Memory",                0,     1, 0.20f);
    number("rate",        "Pulse Rate (Hz)",       0.1f, 12, 2, 0.1f);
    number("root",        "Resonance Root (Hz)",  40,   500, 110,  1);
    number("grain",       "Grain Length (ms)",    35,   220, 95,   1);
    number("feedback",    "Memory Feedback",      0,  0.95f, 0.40f);

    // Master bus, four performance macros and global switches.
    number("global.intensity",  "Intensité globale", 0, 1, 0.5f);
    number("global.speed",      "Vitesse globale", 0.25f, 4, 1, 0.01f);
    number("global.width",      "Largeur stéréo globale", 0, 1, 0.8f);
    toggle("global.sync",       "Synchro hôte", true);
    toggle("global.freeze",     "Gel global", false);
    toggle("global.bypass",     "Bypass global", false);
    number("macro.growth",      "Croissance", 0, 1, 0.35f);
    number("macro.motion",      "Mouvement", 0, 1, 0.35f);
    number("macro.space",       "Espace", 0, 1, 0.50f);
    number("macro.energy",      "Énergie", 0, 1, 0.35f);

    // PULSE — existing depth and oscillator speed keep their legacy IDs.
    toggle("pulse.active",      "Pulse actif", true);
    number("pulse.sensitivity", "Pulse sensibilité", 0, 1, 0.55f);
    menu("pulse.division",      "Division Pulse",
         { "1/4", "1/8", "1/8T", "1/16", "1/16T" }, 1);
    toggle("pulse.sync",        "Synchro Pulse", true);
    number("pulse.swing",       "Swing Pulse", 0, 1, 0.08f);
    number("pulse.feedback",    "Réinjection Pulse", 0, 1, 0.24f);
    number("pulse.width",       "Largeur Pulse", 0, 1, 0.60f);
    number("pulse.mix",         "Dosage Pulse", 0, 1, 0.52f);

    // HARMONIC LIFE — root and current wet gain keep legacy IDs.
    toggle("harm.active",       "Harmonic Life actif", true);
    menu("harm.mode",           "Harmoniques",
         { "Quintes", "Octaves", "Tierces", "Suspendues" }, 0);
    number("harm.resonance",    "Résonance", 0, 1, 0.55f);
    number("harm.decay",        "Déclin", 0.1f, 6, 2.4f);
    number("harm.damping",      "Amortissement", 0, 1, 0.48f);
    number("harm.motion",       "Mouvement harmonique", 0, 1, 0.18f);
    number("harm.spread",       "Dispersion", 0, 1, 0.65f);
    toggle("harm.mono",         "Suivi mono", false);

    // MUTATION — grain and current wet gain keep legacy IDs.
    toggle("mut.active",        "Mutation actif", true);
    number("mut.density",       "Densité", 1, 40, 12, 0.1f);
    menu("mut.character",       "Caractère",
         { "Organique", "Cristal", "Métallique", "Nuage" }, 0);
    number("mut.motion",        "Mouvement granulaire", 0, 1, 0.25f);
    number("mut.pitch",         "Hauteur", -12, 12, 0, 0.1f);
    number("mut.random",        "Aléatoire", 0, 1, 0.20f);
    number("mut.stereo",        "Stéréo granulaire", 0, 1, 0.65f);

    // REACTION — eight independent, host-automatable routes.
    toggle("react.active",      "Reaction actif", true);
    number("react.reactivity",  "Réactivité", 0, 1, 0.55f);
    number("react.interaction", "Interaction", 0, 1, 0.50f);
    number("react.sensitivity", "Sensibilité Reaction", 0, 1, 0.55f);
    number("react.response",    "Réponse (ms)", 30, 500, 120, 1);
    number("react.complexity",  "Complexité Reaction", 0, 1, 0.65f);

    const juce::StringArray sources {
        "Attaque", "Enveloppe", "Pulse", "Harmonique",
        "Mutation", "Mémoire", "Évolution", "Silence"
    };
    const juce::StringArray destinations {
        "G·Densité", "P·Profondeur", "H·Mouvement", "M·Rappel",
        "H·Résonance", "G·Mouvement", "M·Réinjection", "P·Cadence"
    };
    constexpr float amounts[] { .45f, .30f, .32f, .28f, .18f, .22f, .24f, -.25f };
    for (int i = 0; i < 8; ++i)
    {
        const auto prefix = "react.route" + juce::String(i + 1);
        const auto name = "Route " + juce::String(i + 1);
        toggle((prefix + ".active").toRawUTF8(), (name + " active").toRawUTF8(), true);
        menu((prefix + ".source").toRawUTF8(), (name + " source").toRawUTF8(), sources, i);
        menu((prefix + ".destination").toRawUTF8(), (name + " destination").toRawUTF8(), destinations, i);
        number((prefix + ".amount").toRawUTF8(), (name + " montant").toRawUTF8(), -1, 1, amounts[i]);
    }

    // MEMORY — feedback and current wet gain keep legacy IDs.
    toggle("mem.active",        "Memory actif", true);
    number("mem.length",        "Durée mémoire", 0.5f, 3, 3, 0.01f);
    number("mem.capture",       "Capture", 0, 1, 0.80f);
    number("mem.recall",        "Rappel", 0, 1, 0.48f);
    menu("mem.repeat",          "Répétition",
         { "1/4", "1/8", "1/16", "Libre" }, 0);
    number("mem.reverse",       "Inversion", 0, 1, 0.25f);
    number("mem.fade",          "Fondu (ms)", 5, 300, 45, 1);
    toggle("mem.freeze",        "Gel mémoire", false);

    // EVOLUTION — state transitions and DSP routing are implemented later.
    toggle("evo.active",        "Evolution actif", true);
    number("evo.rate",          "Cadence évolution", 0.25f, 4, 1);
    number("evo.intensity",     "Intensité évolution", 0, 1, 0.60f);
    number("evo.sensitivity",   "Sensibilité évolution", 0, 1, 0.55f);
    number("evo.stability",     "Stabilité évolution", 0, 1, 0.65f);
    number("evo.complexity",    "Complexité évolution", 0, 1, 0.65f);
    number("evo.recovery",      "Récupération (s)", 0.25f, 10, 4);

    // The navigational preset browser and FR/EN translator are not audio
    // parameters; they will receive application state handling in later phases.
    return { items.begin(), items.end() };
}
} // namespace biosphere::params

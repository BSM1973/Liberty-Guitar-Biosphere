#include "Ui/FactoryPresetCatalog.h"
#include "Dsp/BiosphereDSP.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using Patch = biosphere::factory::Patch;
constexpr int sampleRate = 48000;

void need(bool truth, const std::string& message)
{
    if (!truth) throw std::runtime_error(message);
}

float value(const Patch& patch, const char* key, float fallback)
{
    for (const auto& [id, amount] : patch.values)
        if (id == key) return amount;
    return fallback;
}

biosphere::Parameters settings(const Patch& patch)
{
    biosphere::Parameters p;
    const auto read = [&patch](const char* id, float otherwise)
    { return value(patch, id, otherwise); };
    p.mix = read("mix", p.mix);
    p.pulse = read("pulse", p.pulse);
    p.harmonic = read("harmonic", p.harmonic);
    p.mutation = read("mutation", p.mutation);
    p.memory = read("memory", p.memory);
    p.rateHz = read("rate", p.rateHz);
    p.rootHz = read("root", p.rootHz);
    p.grainMs = read("grain", p.grainMs);
    p.memoryFeedback = read("feedback", p.memoryFeedback);
    p.globalIntensity = read("global.intensity", p.globalIntensity);
    p.globalSpeed = read("global.speed", p.globalSpeed);
    p.globalWidth = read("global.width", p.globalWidth);
    p.macroGrowth = read("macro.growth", p.macroGrowth);
    p.macroMotion = read("macro.motion", p.macroMotion);
    p.macroSpace = read("macro.space", p.macroSpace);
    p.macroEnergy = read("macro.energy", p.macroEnergy);
    p.pulseActive = read("pulse.active", 1.0f) > 0.5f;
    p.pulseSensitivity = read("pulse.sensitivity", p.pulseSensitivity);
    p.pulseDivision = static_cast<int>(read("pulse.division", p.pulseDivision));
    p.pulseSync = read("pulse.sync", 1.0f) > 0.5f;
    p.pulseSwing = read("pulse.swing", p.pulseSwing);
    p.pulseFeedback = read("pulse.feedback", p.pulseFeedback);
    p.pulseWidth = read("pulse.width", p.pulseWidth);
    p.pulseMix = read("pulse.mix", p.pulseMix);
    p.harmonicActive = read("harm.active", 1.0f) > 0.5f;
    p.harmonicMono = read("harm.mono", 0.0f) > 0.5f;
    p.harmonicMode = static_cast<int>(read("harm.mode", p.harmonicMode));
    p.harmonicResonance = read("harm.resonance", p.harmonicResonance);
    p.harmonicDecay = read("harm.decay", p.harmonicDecay);
    p.harmonicDamping = read("harm.damping", p.harmonicDamping);
    p.harmonicMotion = read("harm.motion", p.harmonicMotion);
    p.harmonicSpread = read("harm.spread", p.harmonicSpread);
    p.mutationActive = read("mut.active", 1.0f) > 0.5f;
    p.mutationDensity = read("mut.density", p.mutationDensity);
    p.mutationCharacter = static_cast<int>(read("mut.character", p.mutationCharacter));
    p.mutationMotion = read("mut.motion", p.mutationMotion);
    p.mutationPitch = read("mut.pitch", p.mutationPitch);
    p.mutationRandom = read("mut.random", p.mutationRandom);
    p.mutationStereo = read("mut.stereo", p.mutationStereo);
    p.memoryActive = read("mem.active", 1.0f) > 0.5f;
    p.memoryLength = read("mem.length", p.memoryLength);
    p.memoryCapture = read("mem.capture", p.memoryCapture);
    p.memoryRecall = read("mem.recall", p.memoryRecall);
    p.memoryRepeat = static_cast<int>(read("mem.repeat", p.memoryRepeat));
    p.memoryReverse = read("mem.reverse", p.memoryReverse);
    p.memoryFade = read("mem.fade", p.memoryFade);
    p.memoryFreeze = read("mem.freeze", 0.0f) > 0.5f;
    p.reaction.active = read("react.active", 0.0f) > 0.5f;
    p.reaction.reactivity = read("react.reactivity", p.reaction.reactivity);
    p.reaction.interaction = read("react.interaction", p.reaction.interaction);
    p.reaction.sensitivity = read("react.sensitivity", p.reaction.sensitivity);
    p.reaction.responseMs = read("react.response", p.reaction.responseMs);
    p.reaction.complexity = read("react.complexity", p.reaction.complexity);
    for (int route = 1; route <= 8; ++route)
    {
        const auto prefix = "react.route" + std::to_string(route);
        auto& slot = p.reaction.routes[static_cast<std::size_t>(route - 1)];
        slot.active = read((prefix + ".active").c_str(), 0.0f) > 0.5f;
        slot.source = static_cast<int>(read((prefix + ".source").c_str(), 0.0f));
        slot.destination = static_cast<int>(read((prefix + ".destination").c_str(), 0.0f));
        slot.amount = read((prefix + ".amount").c_str(), 0.0f);
    }
    p.evolution.active = read("evo.active", 0.0f) > 0.5f;
    p.evolution.rate = read("evo.rate", p.evolution.rate);
    p.evolution.intensity = read("evo.intensity", p.evolution.intensity);
    p.evolution.sensitivity = read("evo.sensitivity", p.evolution.sensitivity);
    p.evolution.stability = read("evo.stability", p.evolution.stability);
    p.evolution.complexity = read("evo.complexity", p.evolution.complexity);
    p.evolution.recovery = read("evo.recovery", p.evolution.recovery);
    return p;
}

struct Metrics
{
    float rms = 0.0f, peak = 0.0f;
    double accumulatedDifference = 0.0;
};

Metrics render(const Patch& patch)
{
    biosphere::BiosphereDSP dsp;
    dsp.prepare(sampleRate, 256, 1);
    auto p = settings(patch);
    std::vector<float> samples(static_cast<std::size_t>(sampleRate * 1.3), 0.0f);
    std::vector<float> original(samples.size(), 0.0f);
    constexpr double tau = 6.28318530717958647692;
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        const float time = static_cast<float>(i) / sampleRate;
        const int note = static_cast<int>(i) / 10240 % 3;
        const float frequency = note == 0 ? 164.81f : note == 1 ? 196.0f : 246.94f;
        const int within = static_cast<int>(i) % 10240;
        const float attack = within < 100
            ? 0.65f * std::exp(-static_cast<float>(within) / 29.0f) : 0.0f;
        original[i] = samples[i] = static_cast<float>(0.20 * std::sin(
            tau * frequency * time) + 0.055 * std::sin(tau * frequency * 2.0 * time))
            + attack;
    }
    for (std::size_t start = 0; start < samples.size(); start += 256)
    {
        float* data[] {samples.data() + start};
        dsp.process(data, 1, static_cast<int>(std::min<std::size_t>(
            256, samples.size() - start)), p);
    }

    Metrics m;
    double energy = 0.0;
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        const float sample = samples[i];
        need(std::isfinite(sample), patch.name + ": NaN/Inf in rendered audio");
        need(std::abs(sample) <= 1.001f, patch.name + ": peak outside audio limiter");
        energy += sample * sample;
        m.peak = std::max(m.peak, std::abs(sample));
        m.accumulatedDifference += std::abs(sample - original[i]);
    }
    m.rms = static_cast<float>(std::sqrt(energy / samples.size()));
    need(m.rms > 0.02f && m.rms < 0.75f,
         patch.name + ": implausible output loudness/unstable state");
    need(m.accumulatedDifference > 0.02,
         patch.name + ": no measurable audio processing");
    return m;
}
} // namespace

int main()
{
    try
    {
        const auto patches = biosphere::factory::build();
        need(patches.size() == 100, "Factory catalog does not contain exactly 100 patches");
        std::set<std::string> names, fingerprints;
        std::map<std::string, int> categories;
        float minRms = 1.0f, maxRms = 0.0f, highestPeak = 0.0f;
        for (std::size_t i = 0; i < patches.size(); ++i)
        {
            const auto& patch = patches[i];
            const std::string prefix =
                (i < 9 ? "0" : "") + std::to_string(i + 1);
            need(patch.name.rfind(prefix, 0) == 0, "Factory numbering/order is not stable");
            need(names.insert(patch.name).second, patch.name + ": duplicate preset name");
            ++categories[patch.category];
            need(patch.values.size() >= 5, patch.name + ": incomplete parameter recipe");
            std::set<std::string> ids;
            std::string fingerprint;
            for (const auto& [key, amount] : patch.values)
            {
                need(ids.insert(key).second, patch.name + ": duplicate parameter " + key);
                need(std::isfinite(amount), patch.name + ": invalid parameter value");
                // Signature records the unique complete settings across all engines,
                // not just a different name or decorative index.
                fingerprint += key + ":" + std::to_string(amount) + ";";
            }
            need(fingerprints.insert(fingerprint).second,
                 patch.name + ": duplicate parameter set");
            const auto m = render(patch);
            minRms = std::min(minRms, m.rms);
            maxRms = std::max(maxRms, m.rms);
            highestPeak = std::max(highestPeak, m.peak);
        }
        need(categories.size() == 10, "Must have ten actual musical families");
        for (const auto& [category, count] : categories)
            need(count == 10, category + ": expected exactly ten factory sounds");
        std::cout << "FACTORY PRESET QA PASS: 100 distinct parameter sets, "
                     "10 families of 10, all 100 rendered with real DSP; "
                     "RMS range=" << minRms << ".." << maxRms
                  << ", highest peak=" << highestPeak << '\n';
        // We do not claim manual guitar audition or LUFS matching from these tests.
    }
    catch (const std::exception& e)
    {
        std::cerr << "FACTORY PRESET QA FAIL: " << e.what() << '\n';
        return 1;
    }
}

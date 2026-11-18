#include "Dsp/BiosphereDSP.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr int sampleRate = 48000;
constexpr int duration = 5;
using biosphere::BiosphereDSP;
using biosphere::Parameters;
using Buffer = std::array<std::vector<float>, 2>;

Buffer source()
{
    Buffer data;
    for (auto& channel : data)
        channel.resize(sampleRate * duration);
    for (std::size_t i = 0; i < data[0].size(); ++i)
    {
        const float time = static_cast<float>(i) / sampleRate;
        const float pick = static_cast<float>(i % 12500);
        const float attack = pick < 170 ? std::exp(-pick / 28.0f) * 0.43f : 0.0f;
        data[0][i] = 0.20f * std::sin(2.0f * 3.14159265f * 192.0f * time) + attack;
        data[1][i] = 0.13f * std::sin(2.0f * 3.14159265f * 282.0f * time + 0.35f)
                    + attack * 0.70f;
    }
    return data;
}

Buffer render(const Buffer& original, Parameters p, int channels = 2)
{
    auto result = original;
    BiosphereDSP dsp;
    dsp.prepare(sampleRate, 256, channels);
    for (std::size_t offset = 0; offset < result[0].size(); offset += 256)
    {
        float* data[2] { result[0].data() + offset, result[1].data() + offset };
        dsp.process(data, channels, static_cast<int>(
            std::min<std::size_t>(256, result[0].size() - offset)), p);
    }
    return result;
}

double difference(const Buffer& a, const Buffer& b, int channels = 2)
{
    double total = 0.0;
    for (int ch = 0; ch < channels; ++ch)
        for (std::size_t i = 0; i < a[ch].size(); ++i)
            total += std::abs(a[ch][i] - b[ch][i]);
    return total;
}

void checkFinite(const Buffer& data)
{
    for (const auto& channel : data)
        for (const auto sample : channel)
            if (!std::isfinite(sample))
                throw std::runtime_error("DSP generated a non-finite audio sample");
}

void changed(const char* name, const Buffer& a, const Buffer& b, double minimum = 0.05)
{
    checkFinite(a);
    checkFinite(b);
    const auto amount = difference(a, b);
    if (amount <= minimum)
        throw std::runtime_error(std::string(name) + " has no measurable influence");
    std::cout << name << ": PASS (difference=" << amount << ")\n";
}

Parameters onlyPulse()
{
    Parameters p;
    p.mix = 1;
    p.harmonic = p.mutation = p.memory = 0;
    p.pulse = 0.8f;
    p.globalSync = false;
    return p;
}
Parameters onlyHarmonic()
{
    Parameters p;
    p.mix = 1;
    p.pulse = p.mutation = p.memory = 0;
    p.harmonic = 0.9f;
    return p;
}
Parameters onlyMutation()
{
    Parameters p;
    p.mix = 1;
    p.pulse = p.harmonic = p.memory = 0;
    p.mutation = 0.8f;
    return p;
}
Parameters onlyMemory()
{
    Parameters p;
    p.mix = 1;
    p.pulse = p.harmonic = p.mutation = 0;
    p.memory = 0.95f;
    p.memoryRecall = 1.0f;
    p.memoryLength = 0.5f;
    return p;
}
} // namespace

int main()
{
    try
    {
        const auto audio = source();
        Parameters dry;
        dry.mix = 0;
        changed("Input gain", render(audio, dry),
                render(audio, [&] { auto p = dry; p.inputGain = 0.35f; return p; }()));
        changed("Output gain", render(audio, dry),
                render(audio, [&] { auto p = dry; p.outputGain = 0.48f; return p; }()));

        // Dry path and bypass are bit transparent below limiter threshold.
        const auto dryOutput = render(audio, dry);
        if (difference(audio, dryOutput) > 0.001)
            throw std::runtime_error("Master mix zero must pass dry unchanged");
        auto bypass = onlyPulse();
        bypass.inputGain = 1.8f;
        bypass.outputGain = 0.4f;
        bypass.bypass = true;
        if (difference(audio, render(audio, bypass)) > 0.001)
            throw std::runtime_error("Global bypass must pass raw audio unchanged");

        const auto pPulse = onlyPulse();
        changed("Pulse active", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.pulseActive = false; return p; }()));
        changed("Pulse sensitivity", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.pulseSensitivity = 0; return p; }()));
        changed("Pulse free rate", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.rateHz = 8; return p; }()));
        changed("Pulse swing", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.pulseSwing = 0.9f; return p; }()));
        changed("Pulse feedback", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.pulseFeedback = 0; return p; }()));
        changed("Pulse mix", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.pulseMix = 0; return p; }()));
        changed("Pulse stereo width", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.pulseWidth = 0; return p; }()));
        Parameters pulseSynced = pPulse;
        pulseSynced.globalSync = pulseSynced.pulseSync = pulseSynced.hasHostBpm = true;
        pulseSynced.hostBpm = 123;
        changed("Pulse division/host BPM", render(audio, pulseSynced),
                render(audio, [&] { auto p = pulseSynced; p.pulseDivision = 4; return p; }()));

        const auto pHarm = onlyHarmonic();
        changed("Harmonic active", render(audio, pHarm),
                render(audio, [&] { auto p = pHarm; p.harmonicActive = false; return p; }()));
        changed("Harmonic root", render(audio, pHarm),
                render(audio, [&] { auto p = pHarm; p.rootHz = 275; return p; }()));
        changed("Harmonic mode", render(audio, pHarm),
                render(audio, [&] { auto p = pHarm; p.harmonicMode = 2; return p; }()));
        changed("Harmonic resonance", render(audio, pHarm),
                render(audio, [&] { auto p = pHarm; p.harmonicResonance = 0; return p; }()));
        changed("Harmonic decay", render(audio, pHarm),
                render(audio, [&] { auto p = pHarm; p.harmonicDecay = 0.25f; return p; }()));
        changed("Harmonic damping", render(audio, pHarm),
                render(audio, [&] { auto p = pHarm; p.harmonicDamping = 0; return p; }()));
        changed("Harmonic motion", render(audio, pHarm),
                render(audio, [&] { auto p = pHarm; p.harmonicMotion = 1; return p; }()));
        changed("Harmonic spread", render(audio, pHarm),
                render(audio, [&] { auto p = pHarm; p.harmonicSpread = 0; return p; }()));
        changed("Harmonic monophonic follow", render(audio, pHarm),
                render(audio, [&] { auto p = pHarm; p.harmonicMono = true; return p; }()));

        const auto pMut = onlyMutation();
        changed("Mutation active", render(audio, pMut),
                render(audio, [&] { auto p = pMut; p.mutationActive = false; return p; }()));
        changed("Grain size", render(audio, pMut),
                render(audio, [&] { auto p = pMut; p.grainMs = 200; return p; }()));
        changed("Grain density", render(audio, pMut),
                render(audio, [&] { auto p = pMut; p.mutationDensity = 38; return p; }()));
        changed("Grain character", render(audio, pMut),
                render(audio, [&] { auto p = pMut; p.mutationCharacter = 2; return p; }()));
        changed("Grain motion", render(audio, pMut),
                render(audio, [&] { auto p = pMut; p.mutationMotion = 1; return p; }()));
        changed("Grain pitch", render(audio, pMut),
                render(audio, [&] { auto p = pMut; p.mutationPitch = 9; return p; }()));
        changed("Grain randomness", render(audio, pMut),
                render(audio, [&] { auto p = pMut; p.mutationRandom = 1; return p; }()));
        changed("Grain stereo", render(audio, pMut),
                render(audio, [&] { auto p = pMut; p.mutationStereo = 0; return p; }()));

        const auto pMem = onlyMemory();
        changed("Memory active", render(audio, pMem),
                render(audio, [&] { auto p = pMem; p.memoryActive = false; return p; }()));
        changed("Memory length", render(audio, pMem),
                render(audio, [&] { auto p = pMem; p.memoryLength = 2.8f; return p; }()));
        changed("Memory capture", render(audio, pMem),
                render(audio, [&] { auto p = pMem; p.memoryCapture = 0; return p; }()));
        changed("Memory recall", render(audio, pMem),
                render(audio, [&] { auto p = pMem; p.memoryRecall = 0; return p; }()));
        changed("Memory repeat", render(audio, pMem),
                render(audio, [&] { auto p = pMem; p.memoryRepeat = 2; return p; }()));
        changed("Memory reverse", render(audio, pMem),
                render(audio, [&] { auto p = pMem; p.memoryReverse = 1; return p; }()));
        changed("Memory feedback", render(audio, pMem),
                render(audio, [&] { auto p = pMem; p.memoryFeedback = 0; return p; }()));
        changed("Memory fade", render(audio, pMem),
                render(audio, [&] { auto p = pMem; p.memoryFade = 250; return p; }()));
        changed("Global freeze", render(audio, pMem),
                render(audio, [&] { auto p = pMem; p.globalFreeze = true; return p; }()));
        changed("Memory freeze", render(audio, pMem),
                render(audio, [&] { auto p = pMem; p.memoryFreeze = true; return p; }()));

        // Performance macros must have independently audible, bounded effects.
        changed("Macro Growth", render(audio, pHarm),
                render(audio, [&] { auto p = pHarm; p.macroGrowth = 1; return p; }()));
        changed("Macro Motion", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.macroMotion = 1; return p; }()));
        changed("Macro Space", render(audio, pHarm),
                render(audio, [&] { auto p = pHarm; p.macroSpace = 0; return p; }()));
        changed("Macro Energy", render(audio, pMut),
                render(audio, [&] { auto p = pMut; p.macroEnergy = 1; return p; }()));

        // Master bus must act on actual samples and be safe with stereo input.
        changed("Master dry/wet", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.mix = 0; return p; }()));
        changed("Master intensity", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.globalIntensity = 0; return p; }()));
        changed("Master speed", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.globalSpeed = 4; return p; }()));
        changed("Master width", render(audio, pPulse),
                render(audio, [&] { auto p = pPulse; p.globalWidth = 0; return p; }()));

        auto extreme = pMut;
        extreme.outputGain = 8;
        extreme.inputGain = 8;
        extreme.mix = 1;
        auto output = render(audio, extreme);
        checkFinite(output);
        for (const auto& channel : output)
            for (const float sample : channel)
                if (std::abs(sample) > 1.001f)
                    throw std::runtime_error("Output peak protection failed");

        // Mono output path must not dereference the absent second channel.
        auto mono = render(audio, pPulse, 1);
        checkFinite(mono);
        std::cout << "Phase 4 DSP routing: PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Phase 4 DSP routing: FAIL: " << error.what() << '\n';
        return 1;
    }
}

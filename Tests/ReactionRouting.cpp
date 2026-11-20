#include "Dsp/BiosphereDSP.h"
#include "Dsp/ReactionMatrix.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
using namespace biosphere;
constexpr int sampleRate = 48000;
constexpr int sampleCount = 96000;

void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

ReactionSettings settings()
{
    ReactionSettings s;
    s.active = true;
    s.reactivity = 1.0f;
    s.interaction = 1.0f;
    s.sensitivity = 1.0f;
    s.complexity = 0.0f;
    s.responseMs = 30.0f;
    return s;
}

float converge(ReactionMatrix& matrix, const ReactionSettings& s,
               const ReactionSources& sources, ReactionDestination destination)
{
    float result = 0.0f;
    for (int sample = 0; sample < sampleRate; ++sample)
        result = matrix.tick(s, sources).at(destination);
    return result;
}

std::vector<float> guitar()
{
    std::vector<float> signal(sampleCount);
    constexpr float twoPi = 6.28318530718f;
    for (int i = 0; i < sampleCount; ++i)
    {
        const float time = static_cast<float>(i) / sampleRate;
        const int phase = i % 11800;
        const float attack = phase < 110
            ? 0.65f * std::exp(-static_cast<float>(phase) / 24.0f) : 0.0f;
        signal[static_cast<std::size_t>(i)] =
            0.22f * std::sin(twoPi * 196.0f * time) + attack;
    }
    return signal;
}

std::vector<float> render(const std::vector<float>& input, Parameters p)
{
    auto result = input;
    BiosphereDSP dsp;
    dsp.prepare(sampleRate, 256, 1);
    for (std::size_t offset = 0; offset < result.size(); offset += 256)
    {
        float* pointers[2] {result.data() + offset, nullptr};
        const int count = static_cast<int>(std::min<std::size_t>(256, result.size() - offset));
        dsp.process(pointers, 1, count, p);
    }
    for (const float value : result)
        check(std::isfinite(value) && std::abs(value) <= 1.001f,
              "REACTION output nonfinite or above output peak limiter");
    return result;
}

double deviation(const std::vector<float>& lhs, const std::vector<float>& rhs)
{
    double sum = 0.0;
    for (std::size_t i = 0; i < lhs.size(); ++i)
        sum += std::abs(lhs[i] - rhs[i]);
    return sum;
}
} // namespace

int main()
{
    try
    {
        ReactionMatrix matrix;
        matrix.prepare(sampleRate);
        auto s = settings();
        matrix.configure(s.responseMs, s.reactivity);
        ReactionSources sources;
        sources.values[static_cast<int>(ReactionSource::attack)] = 1.0f;
        s.routes[0] = {true, 0, 1, 0.9f};
        const float positive = converge(matrix, s, sources, ReactionDestination::pulseDepth);
        check(positive > 0.30f, "Positive route doesn't modulate PULSE");

        matrix.reset();
        matrix.configure(30.0f, 1.0f);
        s.routes[0].amount = -0.9f;
        const float negative = converge(matrix, s, sources, ReactionDestination::pulseDepth);
        check(negative < -0.30f, "Negative polarity wasn't preserved");

        s.active = false;
        const float disabled = converge(matrix, s, sources, ReactionDestination::pulseDepth);
        check(std::abs(disabled) < 0.015f, "Disabling REACTION didn't release its modulation");

        s = settings();
        s.routes[0] = {true, 0, 1, 0.9f};
        matrix.reset();
        matrix.configure(30.0f, 1.0f);
        const float ready = converge(matrix, s, sources, ReactionDestination::pulseDepth);
        check(ready > 0.30f, "Route setup failed");
        s.routes[0].destination = 2;
        const auto transition = matrix.tick(s, sources);
        check(std::abs(transition.at(ReactionDestination::harmonicMotion)) < 0.15f,
              "Changing destination caused an abrupt modulation jump");
        const float newValue = converge(matrix, s, sources, ReactionDestination::harmonicMotion);
        check(newValue > 0.30f, "Rerouted destination didn't receive modulation");
        check(std::abs(matrix.tick(s, sources).at(ReactionDestination::pulseDepth)) < 0.015f,
              "Old destination failed to fade out");

        // A reserved EVOLUTION source must remain silent until its DSP exists.
        s = settings();
        s.routes[0] = {true, 6, 1, 1.0f};
        sources.values.fill(0.0f);
        matrix.reset();
        matrix.configure(30.0f, 1.0f);
        check(std::abs(converge(matrix, s, sources, ReactionDestination::pulseDepth)) < 0.0001f,
              "EVOLUTION produces fictional modulation");

        // Eight routes summing at the same target must remain bounded.
        s = settings();
        sources.values.fill(1.0f);
        for (auto& route : s.routes)
            route = {true, 0, 1, 1.0f};
        matrix.reset();
        matrix.configure(30.0f, 1.0f);
        check(converge(matrix, s, sources, ReactionDestination::pulseDepth) <= 1.0001f,
              "Eight concurrent routes exceeded modulation headroom");

        // End-to-end audio path: REACTION ON must audibly differ from OFF.
        const auto input = guitar();
        Parameters pulse;
        pulse.mix = 0.85f;
        pulse.pulse = 0.72f;
        pulse.harmonic = pulse.memory = pulse.mutation = 0.0f;
        pulse.globalSync = false;
        const auto pulseDry = render(input, pulse);
        pulse.reaction = settings();
        pulse.reaction.routes[0] = {true, 7, 1, 0.95f}; // Silence -> PULSE depth
        const auto pulseWet = render(input, pulse);
        check(deviation(pulseDry, pulseWet) > 0.5,
              "REACTION source did not change the rendered PULSE audio");

        Parameters harmonic;
        harmonic.pulse = harmonic.mutation = harmonic.memory = 0.0f;
        harmonic.harmonic = 0.9f;
        harmonic.mix = 0.90f;
        const auto harmonicDry = render(input, harmonic);
        harmonic.reaction = settings();
        harmonic.reaction.routes[0] = {true, 7, 4, 1.0f}; // Silence -> harmonic resonance
        check(deviation(harmonicDry, render(input, harmonic)) > 0.1,
              "REACTION failed to change real harmonic output");

        Parameters memory;
        memory.pulse = memory.harmonic = memory.mutation = 0.0f;
        memory.memory = 0.90f;
        memory.memoryRecall = 0.50f;
        memory.memoryLength = 0.5f;
        memory.mix = 0.9f;
        const auto memoryDry = render(input, memory);
        memory.reaction = settings();
        memory.reaction.routes[0] = {true, 1, 3, 1.0f}; // Guitar envelope -> MEMORY recall
        check(deviation(memoryDry, render(input, memory)) > 0.1,
              "REACTION failed to modulate a real MEMORY fragment");

        // Attack -> PULSE -> harmonic -> MEMORY cascade using existing IDs.
        Parameters cascade;
        cascade.mix = 0.85f;
        cascade.pulse = 0.8f;
        cascade.harmonic = 0.9f;
        cascade.memory = 0.8f;
        cascade.mutation = 0.2f;
        cascade.memoryRecall = 0.45f;
        cascade.memoryLength = 0.5f;
        cascade.reaction = settings();
        cascade.reaction.routes[0] = {true, 0, 1, 1.0f}; // Attack -> PULSE
        cascade.reaction.routes[1] = {true, 2, 2, 1.0f}; // PULSE -> harmonic motion
        cascade.reaction.routes[2] = {true, 3, 3, 1.0f}; // Harmonic -> MEMORY
        const auto active = render(input, cascade);
        cascade.reaction.active = false;
        const auto passive = render(input, cascade);
        check(deviation(active, passive) > 0.12,
              "Real attack/PULSE/harmonic/MEMORY cascade has no audio effect");

        std::cout << "REACTION DSP PASS: 8 routes, +/- polarity, fades, bounds, "
                     "four-engine control and cascade\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "REACTION DSP FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

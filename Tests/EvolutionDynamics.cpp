#include "Dsp/EvolutionEngine.h"
#include "Dsp/BiosphereDSP.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
using biosphere::EvolutionEngine;
using biosphere::EvolutionFrame;
using biosphere::EvolutionSettings;
using biosphere::EvolutionState;
constexpr int sampleRate = 48000;

void demand(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

EvolutionFrame feed(EvolutionEngine& engine, const EvolutionSettings& settings,
                    float level, float attack, int count,
                    float* maxChange = nullptr)
{
    EvolutionFrame frame {};
    float last = -1.0f;
    for (int i = 0; i < count; ++i)
    {
        frame = engine.tick(settings, level, i == 0 ? attack : 0.0f);
        demand(std::isfinite(frame.drive) && std::isfinite(frame.energy)
                && std::isfinite(frame.activity), "Non-finite EVOLUTION telemetry");
        demand(frame.drive >= 0.0f && frame.drive <= 1.0f
                && frame.energy >= 0.0f && frame.energy <= 1.0f,
                "EVOLUTION activity left normalized range");
        if (maxChange && last >= 0)
            *maxChange = std::max(*maxChange, std::abs(frame.drive - last));
        last = frame.drive;
    }
    return frame;
}

std::vector<float> inputAudio()
{
    std::vector<float> result(sampleRate * 3);
    constexpr float pi2 = 6.28318530718f;
    for (std::size_t i = 0; i < result.size(); ++i)
    {
        const auto time = static_cast<float>(i) / sampleRate;
        const auto attack = i % 12000 < 90
            ? 0.57f * std::exp(-static_cast<float>(i % 12000) / 24.0f) : 0.0f;
        result[i] = 0.27f * std::sin(pi2 * 196.0f * time) + attack;
    }
    return result;
}

std::vector<float> render(const std::vector<float>& source, biosphere::Parameters settings)
{
    biosphere::BiosphereDSP engine;
    engine.prepare(sampleRate, 256, 1);
    auto out = source;
    for (std::size_t offset = 0; offset < out.size(); offset += 256)
    {
        float* data[2] {out.data() + offset, nullptr};
        engine.process(data, 1,
            static_cast<int>(std::min<std::size_t>(256, out.size() - offset)),
            settings);
    }
    for (const auto sample : out)
        demand(std::isfinite(sample) && std::abs(sample) <= 1.001f,
               "EVOLUTION broke sample safety/peak limiting");
    return out;
}

double difference(const std::vector<float>& a, const std::vector<float>& b)
{
    double total = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i)
        total += std::abs(a[i] - b[i]);
    return total;
}
}

int main()
{
    try
    {
        EvolutionEngine engine;
        EvolutionSettings settings;
        engine.prepare(sampleRate);
        demand(engine.state() == EvolutionState::calm, "Engine not CALM on prepare");

        const auto soft = feed(engine, settings, 0.10f, 0.90f, sampleRate);
        demand(soft.state == EvolutionState::growth,
               "Soft guitar playing must enter GROWTH");
        demand(soft.drive > 0.01f, "GROWTH did not change effective DSP controls");

        float maxDelta = 0.0f;
        const auto strong = feed(engine, settings, 0.35f, 1.0f, sampleRate,
                                 &maxDelta);
        demand(strong.state == EvolutionState::storm,
               "Strong picking must enter STORM");
        demand(strong.drive > soft.drive, "STORM not stronger than GROWTH");

        const auto silent = feed(engine, settings, 0.0f, 0.0f,
                                  sampleRate / 2, &maxDelta);
        demand(silent.state == EvolutionState::decline,
               "Silence after energetic playing must enter DECLINE");
        const auto rested = feed(engine, settings, 0.0f, 0.0f,
                                  sampleRate * 8, &maxDelta);
        demand(rested.state == EvolutionState::calm,
               "DECLINE must recover to CALM after release");
        demand(rested.drive < 0.015f,
               "Long silence did not gently release EVOLUTION modulation");
        demand(maxDelta < 0.004f,
               "Abrupt effective modulation step could cause an audio click");

        engine.reset();
        demand(engine.state() == EvolutionState::calm, "reset must clear state");
        auto noIntensity = settings;
        noIntensity.intensity = 0.0f;
        const auto mutedModulation = feed(engine, noIntensity, 0.38f, 1.0f,
                                           sampleRate);
        demand(mutedModulation.state == EvolutionState::storm,
               "Intensity should not disable musical-state detection");
        demand(mutedModulation.drive == 0.0f,
               "Zero EVOLUTION intensity must not change DSP output");

        auto fast = settings;
        fast.recovery = 0.25f;
        auto slow = settings;
        slow.recovery = 10.0f;
        EvolutionEngine fastEngine, slowEngine;
        fastEngine.prepare(sampleRate);
        slowEngine.prepare(sampleRate);
        feed(fastEngine, fast, 0.36f, 1.0f, sampleRate);
        feed(slowEngine, slow, 0.36f, 1.0f, sampleRate);
        const auto quicklyRested = feed(fastEngine, fast, 0.0f, 0.0f,
                                         sampleRate * 3);
        const auto slowlyRested = feed(slowEngine, slow, 0.0f, 0.0f,
                                        sampleRate * 3);
        demand(quicklyRested.state == EvolutionState::calm,
               "Fast recovery must return to CALM");
        demand(slowlyRested.energy > quicklyRested.energy,
               "Recovery control does not change decay dynamics");
        demand(slowlyRested.drive > quicklyRested.drive,
               "Slow recovery should preserve modulation tails");

        // Rapidly changing notes/level around the GROWTH/STORM boundary must
        // not make states flicker every few milliseconds.
        engine.reset();
        int transitions = 0;
        auto lastState = engine.state();
        for (int i = 0; i < sampleRate * 2; ++i)
        {
            const float level = (i / 1600) % 2 == 0 ? 0.17f : 0.23f;
            const auto frame = engine.tick(settings, level, i % 1600 == 0 ? 1.0f : 0.0f);
            if (frame.state != lastState)
            {
                ++transitions;
                lastState = frame.state;
            }
        }
        demand(transitions < 15, "EVOLUTION transitions flicker around hysteresis");

        engine.reset();
        feed(engine, settings, 0.36f, 1.0f, sampleRate);
        settings.active = false;
        const auto disabledStart = engine.tick(settings, 0.36f, 0.0f);
        demand(disabledStart.state == EvolutionState::calm,
               "EVOLUTION bypass not reflected in state");
        demand(disabledStart.drive > 0.0f,
               "EVOLUTION bypass should smoothly fade, not hard-mute modulation");
        const auto disabledEnd = feed(engine, settings, 0.36f, 0.0f,
                                      sampleRate * 6);
        demand(disabledEnd.drive < 0.005f,
               "Disabling EVOLUTION should eventually remove its modulation");

        // Real audio must change when EVOLUTION is active, without clipping.
        const auto guitar = inputAudio();
        biosphere::Parameters audio;
        audio.mix = 0.82f;
        audio.pulse = 0.64f;
        audio.harmonic = 0.72f;
        audio.mutation = 0.62f;
        audio.memory = 0.53f;
        audio.evolution.intensity = 1.0f;
        const auto live = render(guitar, audio);
        audio.evolution.active = false;
        const auto staticAudio = render(guitar, audio);
        demand(difference(live, staticAudio) > 0.2,
               "EVOLUTION fails to change actual sound on a played guitar");

        // REACTION's source 6 now comes from real EVOLUTION activity.
        audio.evolution.active = true;
        audio.reaction.active = true;
        audio.reaction.interaction = 1.0f;
        audio.reaction.reactivity = 1.0f;
        audio.reaction.routes[0] = {true, 6, 1, 1.0f};
        const auto livingRoute = render(guitar, audio);
        audio.evolution.active = false;
        const auto bypassedRoute = render(guitar, audio);
        demand(difference(livingRoute, bypassedRoute) > 0.2,
               "REACTION source EVOLUTION is not connected to audio");

        // Neither the controller nor REACTION should touch fully dry audio.
        audio.mix = 0.0f;
        audio.evolution.active = true;
        demand(difference(guitar, render(guitar, audio)) < 0.001,
               "EVOLUTION altered fully dry input");

        std::cout << "EVOLUTION PASS: calm / growth / storm / decline, "
                     "hysteresis, tails, no abrupt modulation, real audio + REACTION\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "EVOLUTION FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

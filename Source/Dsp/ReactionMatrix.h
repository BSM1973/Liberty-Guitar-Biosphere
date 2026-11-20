#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace biosphere
{
// Public mapping of the 8 already-existing APVTS source/destination choices.
// The integer ordering MUST stay unchanged for host preset compatibility.
enum class ReactionSource : int
{
    attack = 0, envelope, pulse, harmonic, mutation, memory, evolution, silence
};
enum class ReactionDestination : int
{
    grainDensity = 0, pulseDepth, harmonicMotion, memoryRecall,
    harmonicResonance, grainMotion, memoryFeedback, pulseRate
};

struct ReactionRoute
{
    bool active = false;
    int source = 0;
    int destination = 0;
    float amount = 0.0f; // Bipolar, -1..+1
};

struct ReactionSettings
{
    bool active = false;
    float reactivity = 0.55f;
    float interaction = 0.50f;
    float sensitivity = 0.55f;
    float responseMs = 120.0f;
    float complexity = 0.65f;
    std::array<ReactionRoute, 8> routes {};
};

struct ReactionSources
{
    // Values derived from DSP audio, normalized to [0, 1].
    // 'evolution' is deliberately silent until the EVOLUTION DSP exists.
    std::array<float, 8> values {};
};

struct ReactionFrame
{
    std::array<float, 8> destinations {};
    float at(ReactionDestination destination) const noexcept
    {
        return destinations[static_cast<std::size_t>(destination)];
    }
};

// Real-time safe, allocation-free 8-route cross-engine modulation engine.
// One-sample-delayed engine sources prevent algebraic feedback loops. Each
// route's signal is smoothed independently so changing source/destination,
// bypassing REACTION or automating a bipolar amount cannot abruptly jump.
class ReactionMatrix final
{
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = std::clamp(sampleRate, 8000.0, 384000.0);
        reset();
    }

    void reset() noexcept
    {
        smoothed_.fill(0.0f);
        rise_ = release_ = 0.001f;
    }

    void configure(float responseMs, float reactivity) noexcept
    {
        const double seconds = static_cast<double>(std::clamp(responseMs, 30.0f, 500.0f))
                               * 0.001;
        // Quick but finite attack preserves picking transients; slower release
        // keeps transitions smooth and helps prevent clicks.
        const float response = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate_ * seconds)));
        rise_ = std::clamp(response * (3.0f + 9.0f * std::clamp(reactivity, 0.0f, 1.0f)),
                           0.000001f, 1.0f);
        release_ = std::clamp(response, 0.000001f, 1.0f);
    }

    ReactionFrame tick(const ReactionSettings& settings,
                       const ReactionSources& sources) noexcept
    {
        ReactionFrame frame;
        const float sensitivity = std::clamp(settings.sensitivity, 0.0f, 1.0f);
        const float reactivity = std::clamp(settings.reactivity, 0.0f, 1.0f);
        const float interaction = std::clamp(settings.interaction, 0.0f, 1.0f);
        const float complexity = std::clamp(settings.complexity, 0.0f, 1.0f);
        const float gate = 0.14f * (1.0f - sensitivity);

        for (std::size_t i = 0; i < settings.routes.size(); ++i)
        {
            const auto& route = settings.routes[i];
            float target = 0.0f;
            if (settings.active && route.active
                && route.source >= 0 && route.source < 8
                && route.destination >= 0 && route.destination < 8)
            {
                const float rawSource = std::clamp(
                    sources.values[static_cast<std::size_t>(route.source)], 0.0f, 1.0f);
                const float activated = std::clamp((rawSource - gate) / (1.0f - gate),
                                                    0.0f, 1.0f);
                // 0 = linear; 1 = sharper, energy-sensitive cross-modulation.
                const float shaped = activated * (1.0f - complexity)
                                     + activated * activated * complexity;
                target = std::clamp(route.amount, -1.0f, 1.0f)
                         * shaped * interaction * (0.4f + 1.2f * reactivity);
                target = std::clamp(target, -1.0f, 1.0f);
            }
            const float coefficient = std::abs(target) > std::abs(smoothed_[i])
                                         ? rise_ : release_;
            smoothed_[i] += coefficient * (target - smoothed_[i]);
            if (std::abs(smoothed_[i]) < 1.0e-9f)
                smoothed_[i] = 0.0f;

            if (route.destination >= 0 && route.destination < 8)
            {
                auto& value = frame.destinations[static_cast<std::size_t>(route.destination)];
                value = std::clamp(value + smoothed_[i], -1.0f, 1.0f);
            }
        }
        return frame;
    }

private:
    double sampleRate_ = 48000.0;
    float rise_ = 0.001f, release_ = 0.001f;
    std::array<float, 8> smoothed_ {};
};
} // namespace biosphere

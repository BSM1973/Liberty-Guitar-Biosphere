#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace biosphere
{
// Existing host APVTS IDs are retained; this struct is purely DSP-side.
struct EvolutionSettings
{
    bool active = true;
    float rate = 1.0f;         // 0.25..4: adaptation speed
    float intensity = 0.60f;   // 0..1: influence on existing audio engines
    float sensitivity = 0.55f; // 0..1: picking/level threshold
    float stability = 0.65f;   // 0..1: minimum dwell/hysteresis
    float complexity = 0.65f;  // 0..1: granular vs harmonic modulation
    float recovery = 4.0f;    // 0.25..10 seconds: how long to relax
};

enum class EvolutionState : int
{
    calm = 0,
    growth = 1,
    storm = 2,
    decline = 3
};

struct EvolutionFrame
{
    EvolutionState state = EvolutionState::calm;
    float drive = 0.0f;        // smooth 0..1 modifier (NOT an audio gain)
    float energy = 0.0f;       // smoothed picking-energy observation
    float activity = 0.0f;     // safe source for REACTION's EVOLUTION slot
};

// No allocations, locks, timers, or unpredictable random oscillators.
// Runs once per sample. The musical state is categorical, but its DSP influence
// morphs separately with a time-constant so state changes don't click.
// The frame carries NO host parameter updates and never mutes an existing tail.
class EvolutionEngine final
{
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = std::clamp(sampleRate, 8000.0, 384000.0);
        reset();
    }

    void reset() noexcept
    {
        state_ = EvolutionState::calm;
        energy_ = drive_ = 0.0f;
        holdSamples_ = quietSamples_ = aboveStormSamples_ = 0;
    }

    EvolutionFrame tick(const EvolutionSettings& requested,
                        float level, float attackActivity) noexcept
    {
        if (!requested.active)
        {
            // Keep the envelope fading after a switch rather than stepping DSP
            // gain. The state label returns to CALM immediately on deactivation.
            state_ = EvolutionState::calm;
            quietSamples_ = aboveStormSamples_ = holdSamples_ = 0;
        }

        const float rate = std::clamp(requested.rate, 0.25f, 4.0f);
        const float sensitivity = std::clamp(requested.sensitivity, 0.0f, 1.0f);
        const float stability = std::clamp(requested.stability, 0.0f, 1.0f);
        const float intensity = std::clamp(requested.intensity, 0.0f, 1.0f);
        const float complexity = std::clamp(requested.complexity, 0.0f, 1.0f);
        const float recovery = std::clamp(requested.recovery, 0.25f, 10.0f);
        const float safeLevel = std::clamp(std::isfinite(level) ? level : 0.0f, 0.0f, 1.0f);
        const float attack = std::clamp(std::isfinite(attackActivity) ? attackActivity : 0.0f,
                                        0.0f, 1.0f);
        const float stimulation = std::clamp(safeLevel * (2.1f + 2.5f * sensitivity)
                                             + attack * 0.15f, 0.0f, 1.0f);
        const auto coefficient = [this](double seconds) noexcept
        {
            return static_cast<float>(1.0 - std::exp(-1.0 /
                   (sampleRate_ * std::max(0.002, seconds))));
        };
        const float rise = coefficient(0.055 / rate);
        const float fall = coefficient(recovery * 0.26);
        energy_ += (stimulation > energy_ ? rise : fall) * (stimulation - energy_);
        energy_ = std::clamp(energy_, 0.0f, 1.0f);

        if (requested.active)
        {
            const int quietDelay = static_cast<int>(sampleRate_ * (0.12f + 0.15f * stability));
            const float growThreshold = 0.22f + (1.0f - sensitivity) * 0.09f;
            const float stormThreshold = 0.61f + stability * 0.09f;
            const bool isQuiet = safeLevel < 0.013f && attack < 0.03f;
            quietSamples_ = isQuiet ? std::min(quietSamples_ + 1,
                                               static_cast<int>(sampleRate_ * 2.0))
                                    : 0;
            aboveStormSamples_ = energy_ > stormThreshold
                ? std::min(aboveStormSamples_ + 1, static_cast<int>(sampleRate_ * 2.0))
                : 0;
            if (holdSamples_ > 0) --holdSamples_;
            const bool quietEnough = quietSamples_ > quietDelay;
            if (holdSamples_ == 0)
            {
                auto next = state_;
                switch (state_)
                {
                    case EvolutionState::calm:
                        if (!quietEnough && energy_ > growThreshold)
                            next = EvolutionState::growth;
                        break;
                    case EvolutionState::growth:
                        if (quietEnough)
                            next = EvolutionState::decline;
                        else if (aboveStormSamples_ > static_cast<int>(
                                     sampleRate_ * (0.06f + 0.12f * stability)))
                            next = EvolutionState::storm;
                        break;
                    case EvolutionState::storm:
                        if (quietEnough)
                            next = EvolutionState::decline;
                        else if (energy_ < 0.47f)
                            next = EvolutionState::growth;
                        break;
                    case EvolutionState::decline:
                        if (!quietEnough && energy_ > growThreshold)
                            next = EvolutionState::growth;
                        else if (energy_ < 0.075f)
                            next = EvolutionState::calm;
                        break;
                }
                if (next != state_)
                {
                    state_ = next;
                    // Hysteresis/dwell prevents state flicker at thresholds.
                    holdSamples_ = static_cast<int>(sampleRate_
                        * (0.045f + stability * 0.27f) / rate);
                }
            }
        }

        const float target = !requested.active ? 0.0f
            : state_ == EvolutionState::calm ? 0.0f
            : state_ == EvolutionState::growth ? 0.42f
            : state_ == EvolutionState::storm ? 1.0f
            : 0.10f; // DECLINE fades out harmonically, without chopping tails.
        const float morph = coefficient(
            target > drive_ ? (0.14 + 0.20 * stability) / rate
                            : std::max(0.16, static_cast<double>(recovery) * 0.28));
        drive_ += morph * (target - drive_);
        drive_ = std::clamp(drive_, 0.0f, 1.0f);

        EvolutionFrame frame;
        frame.state = state_;
        frame.drive = drive_ * intensity;
        frame.energy = energy_;
        // Evolution source is actual performance-driven motion, not a
        // synthetic oscillator or a made-up meter.
        frame.activity = std::clamp(frame.drive * (0.7f + 0.3f * complexity),
                                    0.0f, 1.0f);
        return frame;
    }

    EvolutionState state() const noexcept { return state_; }

private:
    double sampleRate_ = 48000.0;
    EvolutionState state_ = EvolutionState::calm;
    float energy_ = 0.0f, drive_ = 0.0f;
    int holdSamples_ = 0, quietSamples_ = 0, aboveStormSamples_ = 0;
};
} // namespace biosphere

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "ReactionMatrix.h"
#include "EvolutionEngine.h"
#include "GuitarPitchTracker.h"

namespace biosphere
{
// All controls are copied from APVTS at block boundaries by the processor.
// REACTION uses eight cross-engine APVTS routes; EVOLUTION remains separate.
struct Parameters
{
    // Legacy public host IDs — keep names/defaults for existing tests/sessions.
    float inputGain = 1.0f, outputGain = 1.0f, mix = 0.65f;
    float pulse = 0.35f, harmonic = 0.30f, mutation = 0.20f, memory = 0.20f;
    float rateHz = 2.0f, rootHz = 110.0f, grainMs = 95.0f, memoryFeedback = 0.4f;

    // Master and performance macros.
    float globalIntensity = 0.5f, globalSpeed = 1.0f, globalWidth = 0.8f;
    bool globalSync = true, globalFreeze = false, bypass = false;
    float macroGrowth = 0.35f, macroMotion = 0.35f;
    float macroSpace = 0.5f, macroEnergy = 0.35f;
    float hostBpm = 120.0f;
    bool hasHostBpm = false;

    // Four existing audio engines.
    bool pulseActive = true, pulseSync = true;
    float pulseSensitivity = 0.55f, pulseSwing = 0.08f, pulseFeedback = 0.24f;
    float pulseWidth = 0.6f, pulseMix = 0.52f;
    int pulseDivision = 1;

    bool harmonicActive = true, harmonicMono = false;
    int harmonicMode = 0;
    float harmonicResonance = 0.55f, harmonicDecay = 2.4f, harmonicDamping = 0.48f;
    float harmonicMotion = 0.18f, harmonicSpread = 0.65f;

    bool mutationActive = true;
    float mutationDensity = 12.0f, mutationMotion = 0.25f, mutationPitch = 0.0f;
    float mutationRandom = 0.2f, mutationStereo = 0.65f;
    int mutationCharacter = 0;

    bool memoryActive = true, memoryFreeze = false;
    float memoryLength = 3.0f, memoryCapture = 0.8f, memoryRecall = 0.48f;
    float memoryReverse = 0.25f, memoryFade = 45.0f;
    int memoryRepeat = 0;

    // Eight configurable links; IDs and choice indices match Phase-2 APVTS.
    ReactionSettings reaction;
    EvolutionSettings evolution;
};

// Published only AFTER processing a block. The processor transfers these
// values into atomics; the editor must never read the DSP state directly.
struct AnalysisValues
{
    float envelope = 0.0f, attacks = 0.0f, rhythmicActivity = 0.0f;
    float memoryActivity = 0.0f, tonalEnergy = 0.0f, rootHz = 0.0f;
    float evolutionEnergy = 0.0f;
    int evolutionState = 0;
    float pitchConfidence = 0.0f;
    int midiNote = -1;
};

// The four macros are non-destructive multipliers: raw APVTS values never change.
// All calculations are bounded and deterministic, with no dynamic allocation.
inline Parameters mapPerformanceControls(Parameters p) noexcept
{
    const auto unit = [](float value) { return std::clamp(value, 0.0f, 1.0f); };
    const float growth = unit(p.macroGrowth);
    const float motion = unit(p.macroMotion);
    const float space = unit(p.macroSpace);
    const float energy = unit(p.macroEnergy);

    // GROWTH: harmonic sustain + captured memory recall.
    p.harmonicDecay *= 0.65f + 1.15f * growth;
    p.memoryRecall *= 0.60f + 0.80f * growth;

    // MOTION: rhythmic speed and granular density/movement.
    p.rateHz *= 0.65f + 1.10f * motion;
    p.mutationDensity *= 0.65f + 1.05f * motion;
    p.mutationMotion = unit(p.mutationMotion * (0.65f + 1.05f * motion));

    // SPACE: only wet spatial movement is affected; dry L/R remains intact.
    p.harmonicSpread = unit(p.harmonicSpread * (0.50f + 0.95f * space));
    p.mutationStereo = unit(p.mutationStereo * (0.50f + 0.95f * space));
    p.globalWidth = unit(p.globalWidth * (0.55f + 0.90f * space));

    // ENERGY: engine response and the four effect contributions.
    const float energyFactor = 0.60f + 1.15f * energy;
    p.pulse *= energyFactor;
    p.harmonic *= energyFactor;
    p.mutation *= energyFactor;
    p.memory *= energyFactor;
    p.pulseSensitivity = unit(p.pulseSensitivity * (0.60f + 0.8f * energy));
    return p;
}

class BiosphereDSP
{
public:
    AnalysisValues getAnalysisValues() const noexcept
    {
        return { envelope_, attackActivity_, rhythmicActivity_,
                 memoryActivity_, tonalEnergy_, trackedRoot_,
                 evolutionEnergy_, evolutionState_, pitchConfidence_, trackedMidiNote_ };
    }

    void prepare(double sampleRate, int, int channels)
    {
        sampleRate_ = std::max(8000.0, sampleRate);
        channels_ = std::clamp(channels, 1, 2);
        grainCapacity_ = static_cast<std::size_t>(std::ceil(sampleRate_ * 3.0));
        // 12 seconds: at 3 s capture + 3 repeats the underlying source is
        // not overwritten before the last playback completes.
        memoryCapacity_ = static_cast<std::size_t>(std::ceil(sampleRate_ * 12.0));
        pulseCapacity_ = static_cast<std::size_t>(std::ceil(sampleRate_ * 2.0));
        for (int channel = 0; channel < 2; ++channel)
        {
            grainsBuffer_[channel].assign(grainCapacity_, 0.0f);
            memoryBuffer_[channel].assign(memoryCapacity_, 0.0f);
            pulseBuffer_[channel].assign(pulseCapacity_, 0.0f);
        }
        reactionMatrix_.prepare(sampleRate_);
        evolutionEngine_.prepare(sampleRate_);
        guitarPitch_.prepare(sampleRate_);
        reset();
    }

    void reset() noexcept
    {
        phase_ = envelope_ = 0.0f;
        attackActivity_ = rhythmicActivity_ = memoryActivity_ = tonalEnergy_ = 0.0f;
        grainWrite_ = memoryWrite_ = pulseWrite_ = 0;
        processedSamples_ = 0;
        attackCooldown_ = memoryCooldown_ = grainCountdown_ = 0;
        grainVoice_ = 0;
        memoryAge_ = -1;
        memoryStart_ = 0;
        memoryLength_ = 1;
        memoryHasCapture_ = false;
        smoothed_ = false;
        wetBypass_ = 1.0f;
        pulseTapFade_ = 1.0f;
        pulseDelayCurrent_ = pulseDelayOld_ = 0;
        pulseFeedbackTone_.fill(0.0f);
        guitarPitch_.reset();
        trackedRoot_ = pitchConfidence_ = smoothedHarmonicRoot_ = 0.0f;
        trackedMidiNote_ = -1;
        noiseSeed_ = 0x8fe34ae1u;
        reactionMatrix_.reset();
        evolutionEngine_.reset();
        evolutionEnergy_ = 0.0f;
        evolutionState_ = 0;
        pulseSource_ = harmonicSource_ = grainSource_ = memorySource_ = 0.0f;
        previousMemoryExcitation_ = 0.0f;
        for (auto& channel : resonators_)
            for (auto& state : channel) state = {};
        for (auto& channel : grains_)
            for (auto& voice : channel) voice = {};
        for (auto& channel : grainsBuffer_)
            std::fill(channel.begin(), channel.end(), 0.0f);
        for (auto& channel : memoryBuffer_)
            std::fill(channel.begin(), channel.end(), 0.0f);
        for (auto& channel : pulseBuffer_)
            std::fill(channel.begin(), channel.end(), 0.0f);
    }

    void process(float* const* data, int channels, int samples, const Parameters& requested) noexcept
    {
        if (grainCapacity_ == 0 || data == nullptr || samples <= 0)
            return;
        channels = std::clamp(channels, 0, channels_);
        if (channels < 1 || data[0] == nullptr || (channels == 2 && data[1] == nullptr))
            return;

        const Parameters p = sanitise(mapPerformanceControls(requested));
        reactionMatrix_.configure(p.reaction.responseMs, p.reaction.reactivity);
        const float tempo = p.hasHostBpm ? p.hostBpm : 120.0f;
        const float beatHz = tempo / 60.0f;
        constexpr std::array<float, 5> divisions { 1, 2, 3, 4, 6 };
        const int division = std::clamp(p.pulseDivision, 0, 4);
        const float rate = (p.globalSync && p.pulseSync && p.hasHostBpm)
            ? beatHz * divisions[static_cast<std::size_t>(division)] * p.globalSpeed
            : p.rateHz * p.globalSpeed;
        const float basePhaseIncrement = static_cast<float>(twoPi / sampleRate_)
            * std::clamp(rate, 0.08f, 24.0f);
        const float grainLengthSeconds = p.grainMs * 0.001f;
        const int grainLength = std::clamp(static_cast<int>(sampleRate_ * grainLengthSeconds),
                                           64, static_cast<int>(grainCapacity_ / 3));
        // Grain spacing will be computed from the current reaction-modulated
        // density on each grain trigger; there are no control-rate steps.
        const double grainPitch = std::exp2(static_cast<double>(p.mutationPitch) / 12.0);
        const int captureLength = std::clamp(static_cast<int>(sampleRate_ * p.memoryLength),
                                             64, static_cast<int>(sampleRate_ * 3.0));
        constexpr std::array<float, 4> repeatDivisions { 1.0f, 2.0f, 4.0f, 1.0f };
        const float repeatRate = p.memoryRepeat == 3
            ? std::clamp(p.rateHz, 0.2f, 12.0f) : tempo / 60.0f
                * repeatDivisions[static_cast<std::size_t>(p.memoryRepeat)];
        const int memoryWait = std::clamp(static_cast<int>(sampleRate_ / repeatRate),
                                        64, static_cast<int>(sampleRate_ * 3.0));
        const int fadeSamples = std::max(5, static_cast<int>(sampleRate_ * p.memoryFade * 0.001f));
        const int delaySamples = std::clamp(static_cast<int>(sampleRate_ / std::max(0.1f, rate)),
                                            64, static_cast<int>(pulseCapacity_ - 2));
        const float pulseTapStep = static_cast<float>(
            1.0 / std::max(64.0, sampleRate_ * 0.045)); // 45 ms crossfade
        const bool freeze = p.globalFreeze || p.memoryFreeze;

        if (!smoothed_)
        {
            smoothedInput_ = p.inputGain;
            smoothedOutput_ = p.outputGain;
            smoothedMix_ = p.mix;
            smoothedWidth_ = p.globalWidth;
            smoothedIntensity_ = p.globalIntensity;
            smoothedHarmonicRoot_ = p.rootHz;
            pulseDelayCurrent_ = pulseDelayOld_ = delaySamples;
            pulseTapFade_ = 1.0f;
            wetBypass_ = p.bypass ? 0.0f : 1.0f;
            smoothed_ = true;
        }
        const float smooth = std::clamp(static_cast<float>(1.0 - std::exp(-1.0
                                 / (sampleRate_ * 0.007))), 0.000001f, 1.0f);
        const float harmonicTuneSmoothing = static_cast<float>(
            1.0 - std::exp(-1.0 / (sampleRate_ * 0.022)));

        for (int sample = 0; sample < samples; ++sample)
        {
            smoothedInput_ += smooth * (p.inputGain - smoothedInput_);
            smoothedOutput_ += smooth * (p.outputGain - smoothedOutput_);
            smoothedMix_ += smooth * (p.mix - smoothedMix_);
            smoothedWidth_ += smooth * (p.globalWidth - smoothedWidth_);
            smoothedIntensity_ += smooth * (p.globalIntensity - smoothedIntensity_);
            wetBypass_ += smooth * ((p.bypass ? 0.0f : 1.0f) - wetBypass_);

            // PULSE PRO: independently crossfade old/new delay read heads
            // after tempo changes, rather than jumping the circular-buffer
            // index (which makes a broadband click).
            if (pulseTapFade_ >= 1.0f && pulseDelayCurrent_ != delaySamples)
            {
                pulseDelayOld_ = pulseDelayCurrent_;
                pulseDelayCurrent_ = delaySamples;
                pulseTapFade_ = 0.0f;
            }
            if (pulseTapFade_ < 1.0f)
                pulseTapFade_ = std::min(1.0f, pulseTapFade_ + pulseTapStep);

            std::array<float, 2> raw {}, dry {}, delta {};
            float level = 0.0f;
            for (int channel = 0; channel < channels; ++channel)
            {
                raw[static_cast<std::size_t>(channel)] = data[channel][sample];
                dry[static_cast<std::size_t>(channel)] = raw[static_cast<std::size_t>(channel)]
                                                      * smoothedInput_;
                level += std::abs(dry[static_cast<std::size_t>(channel)]);
            }
            level /= static_cast<float>(channels);

            const float prior = envelope_;
            envelope_ += (level > envelope_ ? 0.065f : 0.002f) * (level - envelope_);
            if (attackCooldown_ > 0) --attackCooldown_;
            if (memoryCooldown_ > 0) --memoryCooldown_;
            const float threshold = 0.045f + 0.20f * (1.0f - p.pulseSensitivity);
            const bool onset = attackCooldown_ == 0 && level > threshold && level > prior * 1.65f;
            if (onset) attackCooldown_ = static_cast<int>(sampleRate_ * 0.095);
            attackActivity_ = onset ? 1.0f : attackActivity_ * 0.998f;

            // EVOLUTION: real playing-energy tracker + four-state controller.
            // State is discrete; engine influence is independently smoothed.
            const auto evolving = evolutionEngine_.tick(p.evolution, level, attackActivity_);
            evolutionEnergy_ = evolving.energy;
            evolutionState_ = static_cast<int>(evolving.state);

            // Cross-engine inputs use the last finished audio sample.
            // Attack/envelope are live; PULSE, HARMONIC, GRAIN and MEMORY
            // come from measured engine outputs, preventing algebraic loops.
            ReactionSources reactionSources;
            reactionSources.values = {
                attackActivity_,
                std::clamp(envelope_ * 5.0f, 0.0f, 1.0f),
                pulseSource_, harmonicSource_, grainSource_, memorySource_,
                evolving.activity, // actual EVOLUTION playing-driven DSP activity
                1.0f - std::clamp(envelope_ * 6.0f, 0.0f, 1.0f)
            };
            const auto cross = reactionMatrix_.tick(p.reaction, reactionSources);
            const float pulseDepth = std::clamp(p.pulse * (1.0f + 1.5f
                * cross.at(ReactionDestination::pulseDepth))
                * (1.0f + 0.30f * evolving.drive), 0.0f, 1.0f);
            const float texture = std::clamp(p.evolution.complexity, 0.0f, 1.0f);
            const float harmonicMovement = std::clamp(p.harmonicMotion + 0.70f
                * cross.at(ReactionDestination::harmonicMotion)
                + 0.25f * evolving.drive * (1.0f - 0.45f * texture), 0.0f, 1.0f);
            const float harmonicStrength = std::clamp(p.harmonicResonance + 0.75f
                * cross.at(ReactionDestination::harmonicResonance)
                + 0.18f * evolving.drive * (1.0f - 0.35f * texture), 0.0f, 1.0f);
            const float grainDensity = std::clamp(p.mutationDensity * (1.0f + 1.8f
                * cross.at(ReactionDestination::grainDensity))
                * (1.0f + 0.75f * evolving.drive * (0.25f + 0.75f * texture)),
                1.0f, 80.0f);
            const float grainMotion = std::clamp(p.mutationMotion + 0.80f
                * cross.at(ReactionDestination::grainMotion)
                + 0.30f * evolving.drive * texture, 0.0f, 1.0f);
            const float memoryRecall = std::clamp(p.memoryRecall + 0.90f
                * cross.at(ReactionDestination::memoryRecall)
                + 0.18f * evolving.drive, 0.0f, 1.0f);
            const float memoryFeedback = std::clamp(p.memoryFeedback + 0.65f
                * cross.at(ReactionDestination::memoryFeedback), 0.0f, 0.95f);
            const float rateMultiplier = std::clamp(1.0f + 1.50f
                * cross.at(ReactionDestination::pulseRate), 0.2f, 3.0f);

            // HARMONIC LIFE can re-trigger a stored MEMORY fragment.
            // Threshold + cooldown keep it musical (not one trigger/sample).
            const float memoryExcitation = std::max(0.0f,
                cross.at(ReactionDestination::memoryRecall));
            const bool harmonicMemoryTrigger = p.reaction.active && p.memoryActive
                && memoryCooldown_ == 0 && !freeze && p.memory > 0.0001f
                && memoryExcitation > 0.004f && previousMemoryExcitation_ <= 0.004f;
            previousMemoryExcitation_ = memoryExcitation;

            // GUITAR INTELLIGENCE: YIN-style periodicity tracking with
            // attack guard, note hysteresis and confidence; analysis is done
            // on pre-effect guitar, never on resonator/feedback return.
            // A mixed stereo input is deliberately NOT averaged here: phase-
            // cancelled guitars would destroy the note. The first channel is
            // the monophonic tracking reference for both channels' harmonies.
            guitarPitch_.processSample(dry[0], onset);
            const auto pitch = guitarPitch_.frame();
            trackedRoot_ = pitch.voiced ? pitch.frequencyHz : 0.0f;
            pitchConfidence_ = pitch.voiced ? pitch.confidence : 0.0f;
            trackedMidiNote_ = pitch.voiced ? pitch.midiNote : -1;
            // Pitch-follow can be toggled on an existing preset without a
            // discontinuity; 55ms glide also softens semitone transitions.
            const float targetRoot = (p.harmonicMono && pitch.voiced
                                      && pitch.confidence >= 0.81f)
                ? pitch.frequencyHz : p.rootHz;
            const float harmonicGlide = static_cast<float>(
                1.0 - std::exp(-1.0 / (sampleRate_ * 0.055)));
            smoothedHarmonicRoot_ += harmonicGlide
                                    * (targetRoot - smoothedHarmonicRoot_);

            if ((onset || harmonicMemoryTrigger) && memoryCooldown_ == 0
                && p.memoryActive && !freeze && p.memory > 0.0001f
                && p.memoryCapture > 0.0001f
                && (harmonicMemoryTrigger || level > threshold * (1.20f - 0.80f * p.memoryCapture))
                && processedSamples_ > static_cast<std::size_t>(sampleRate_ * 0.35))
            {
                // Early attacks can recall a shorter available history. No need
                // to wait 3 seconds before the first audible memory fragment.
                memoryLength_ = std::min(captureLength,
                    static_cast<int>(processedSamples_ - 16));
                memoryStart_ = (memoryWrite_ + memoryCapacity_
                                - static_cast<std::size_t>(memoryLength_ + 1)) % memoryCapacity_;
                memoryAge_ = 0;
                memoryHasCapture_ = true;
                memoryCooldown_ = harmonicMemoryTrigger
                    ? std::max(static_cast<int>(sampleRate_ * 0.20), memoryWait / 2)
                    : memoryWait;
            }

            // A captured buffer may loop indefinitely while global/memory freeze is active.
            // No copies/allocations occur in the audio callback.
            const bool shouldRecall = memoryHasCapture_ && p.memoryActive
                && (memoryAge_ >= 0 || freeze);
            if (shouldRecall && memoryAge_ < 0) memoryAge_ = 0;

            if (--grainCountdown_ <= 0 && p.mutationActive && p.mutation > 0.0001f)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    auto& voice = grains_[static_cast<std::size_t>(channel)]
                                         [static_cast<std::size_t>(grainVoice_)];
                    voice.active = true;
                    voice.age = 0;
                    voice.length = grainLength;
                    // Stay behind the write head even when pitch shifts upward.
                    const auto offset = static_cast<std::size_t>(grainLength * 3);
                    voice.readPosition = static_cast<double>(
                        (grainWrite_ + grainCapacity_ - offset - 1) % grainCapacity_);
                    const float noise = nextNoise();
                    const float stereoBias = channel == 0 ? -1.0f : 1.0f;
                    const double variation = 1.0 + noise * p.mutationRandom * 0.3;
                    const double movement = 1.0 + stereoBias * p.mutationStereo
                                            * grainMotion * 0.18;
                    voice.speed = std::clamp(grainPitch * variation * movement, 0.25, 4.0);
                }
                grainVoice_ = 1 - grainVoice_;
                grainCountdown_ = std::max(48,
                    static_cast<int>(sampleRate_ / grainDensity));
            }

            phase_ += basePhaseIncrement * rateMultiplier;
            if (phase_ >= static_cast<float>(twoPi)) phase_ -= static_cast<float>(twoPi);
            float pulsePosition = phase_ / static_cast<float>(twoPi);
            const float swingDivider = 0.5f + 0.23f * p.pulseSwing;
            pulsePosition = pulsePosition < swingDivider
                ? (pulsePosition / swingDivider) * 0.5f
                : 0.5f + (pulsePosition - swingDivider) / (1.0f - swingDivider) * 0.5f;

            const float root = p.harmonicMono ? smoothedHarmonicRoot_ : p.rootHz;
            const float rhythmTarget = p.pulseActive
                ? envelope_ * (0.5f + 0.5f * std::sin(phase_)) * pulseDepth : 0.0f;
            rhythmicActivity_ += 0.007f * (std::clamp(rhythmTarget * 4.0f, 0.0f, 1.0f)
                                            - rhythmicActivity_);
            constexpr std::array<std::array<float, 3>, 4> fixedIntervals {{
                {{ 1.0f, 1.5f, 2.0f }}, // historical fifths
                {{ 1.0f, 2.0f, 4.0f }}, // historical octaves
                {{ 1.0f, 1.25f, 1.5f }}, // historical thirds
                {{ 1.0f, 1.3333333f, 1.5f }} // historical suspended
            }};
            // Tracked notes use exact equal-tempered semitone relationships,
            // not fixed detuned ratios. Existing presets with Suivi mono OFF
            // retain their original harmonic interval spectrum.
            constexpr std::array<std::array<float, 3>, 4> musicalIntervals {{
                {{ 1.0f, 1.49830708f, 2.0f }}, // root + fifth (7st) + octave
                {{ 1.0f, 2.0f, 4.0f }}, // root + octaves
                {{ 1.0f, 1.25992105f, 1.49830708f }}, // 4st + 7st
                {{ 1.0f, 1.33483985f, 1.49830708f }} // 5st + 7st
            }};
            const auto& intervals = p.harmonicMono
                ? musicalIntervals[static_cast<std::size_t>(p.harmonicMode)]
                : fixedIntervals[static_cast<std::size_t>(p.harmonicMode)];
            float tonalInstant = 0.0f;
            float pulseInstant = 0.0f, grainInstant = 0.0f, memoryInstant = 0.0f;
            for (int channel = 0; channel < channels; ++channel)
            {
                const auto ch = static_cast<std::size_t>(channel);
                if (p.mutationActive)
                    grainsBuffer_[ch][grainWrite_] = dry[ch];
                if (!freeze)
                    memoryBuffer_[ch][memoryWrite_] = dry[ch];

                // PULSE: rhythmic modulation plus a bounded synchronised feedback echo.
                float pulseDelta = 0.0f;
                const auto readDelayTap = [&](int delay) noexcept
                {
                    return pulseBuffer_[ch][(pulseWrite_ + pulseCapacity_
                        - static_cast<std::size_t>(delay)) % pulseCapacity_];
                };
                const float oldTap = readDelayTap(pulseDelayOld_);
                const float newTap = readDelayTap(pulseDelayCurrent_);
                const float delayed = oldTap + (newTap - oldTap) * pulseTapFade_;
                // Band-limit the feedback loop and apply soft saturation.
                // The wet delay tap stays full-band; only recirculated energy
                // is progressively damped so multiple echoes do not scream.
                pulseFeedbackTone_[ch] += 0.38f * (delayed - pulseFeedbackTone_[ch]);
                pulseBuffer_[ch][pulseWrite_] = std::tanh(dry[ch]
                        + pulseFeedbackTone_[ch] * p.pulseFeedback * 0.72f);
                if (p.pulseActive && pulseDepth > 0.0001f)
                {
                    const float stereoPhase = channel == 1
                        ? 0.20f * p.pulseWidth : 0.0f;
                    const float position = std::fmod(pulsePosition + stereoPhase, 1.0f);
                    const float wave = 0.5f - 0.5f * std::cos(static_cast<float>(twoPi) * position);
                    const float dynamic = 0.22f + 0.78f * std::clamp(envelope_ * 4.0f * (0.4f + p.pulseSensitivity * 1.3f), 0.0f, 1.0f);
                    pulseDelta = pulseDepth * p.pulseMix *
                        (-dry[ch] * dynamic * (1.0f - wave) + delayed * p.pulseFeedback * 0.25f);
                }

                pulseInstant += std::abs(pulseDelta);

                // HARMONIC LIFE: three independently tuned, bounded resonators.
                float harmonicDelta = 0.0f;
                for (int voice = 0; voice < 3; ++voice)
                {
                    const float movement = harmonicMovement
                        * std::sin(phase_ * 0.43f + voice * 1.7f);
                    const float targetFrequency = std::clamp(
                        root * intervals[static_cast<std::size_t>(voice)]
                            * (1.0f + 0.035f * movement),
                        45.0f, static_cast<float>(sampleRate_ * 0.42));
                    auto& state = resonators_[ch][static_cast<std::size_t>(voice)];
                    // Independent per-voice tuning glide prevents abrupt
                    // coefficient jumps when a new guitar note is detected.
                    if (state.frequency <= 0.0f)
                        state.frequency = targetFrequency;
                    state.frequency += harmonicTuneSmoothing
                                       * (targetFrequency - state.frequency);
                    const double decayScale = std::clamp(
                        static_cast<double>(p.harmonicDecay), 0.1, 9.0);
                    const double damp = 0.65 + (1.0 - p.harmonicDamping) * 0.85;
                    const double radius = std::exp(-state.frequency * damp
                                                       / (12.0 * sampleRate_ * decayScale));
                    const double angle = twoPi * state.frequency / sampleRate_;
                    const double excitation = std::tanh(dry[ch] * 1.50f) * 0.78;
                    const double next = (1.0 - radius) * excitation
                        + 2.0 * radius * std::cos(angle) * state.last
                        - radius * radius * state.previous;
                    state.previous = state.last;
                    state.last = std::clamp(next, -5.0, 5.0);
                    // Damp upper harmonics of the resonator output without
                    // suppressing note tracking or smoothing the master dry.
                    const float dampingMix = 0.14f + 0.42f * (1.0f - p.harmonicDamping);
                    state.tone += dampingMix * (static_cast<float>(
                        std::tanh(state.last * 0.85)) - state.tone);
                    harmonicDelta += state.tone;
                }
                const float stereoSpread = channels == 2
                    ? (channel == 0 ? 1.0f - 0.12f * p.harmonicSpread
                                     : 1.0f + 0.12f * p.harmonicSpread) : 1.0f;
                if (p.harmonicActive)
                    harmonicDelta *= p.harmonic * harmonicStrength * 0.35f * stereoSpread;
                else
                    harmonicDelta = 0.0f;
                tonalInstant += std::abs(harmonicDelta);

                // MUTATION: overlapping tapered grains from live circular audio.
                float grainWet = 0.0f;
                for (auto& voice : grains_[ch])
                {
                    if (!voice.active) continue;
                    const float progress = static_cast<float>(voice.age)
                                            / static_cast<float>(std::max(1, voice.length));
                    const float window = std::sin(static_cast<float>(pi) * progress);
                    float sampleGrain = readInterpolated(grainsBuffer_[ch], voice.readPosition);
                    if (p.mutationCharacter == 1) sampleGrain = std::tanh(sampleGrain * 2.0f);
                    if (p.mutationCharacter == 2) sampleGrain = std::sin(sampleGrain * 12.0f) * 0.48f;
                    if (p.mutationCharacter == 3) sampleGrain = std::tanh(sampleGrain * 0.75f);
                    grainWet += sampleGrain * window * window * 0.65f;
                    voice.readPosition += voice.speed;
                    if (voice.readPosition >= static_cast<double>(grainCapacity_))
                        voice.readPosition -= static_cast<double>(grainCapacity_);
                    if (++voice.age >= voice.length) voice.active = false;
                }
                const float mutationDelta = p.mutationActive
                    ? grainWet * p.mutation : 0.0f;
                grainInstant += std::abs(mutationDelta);

                // MEMORY: windowed fragment with reverse blend and bounded repeats.
                float memoryDelta = 0.0f;
                if (shouldRecall && memoryLength_ > 1)
                {
                    const int position = memoryAge_ % memoryLength_;
                    const int repetition = memoryAge_ / memoryLength_;
                    const float up = std::min(1.0f, static_cast<float>(position)
                                                 / static_cast<float>(fadeSamples));
                    const float down = std::min(1.0f,
                        static_cast<float>(memoryLength_ - position - 1)
                         / static_cast<float>(fadeSamples));
                    const float window = std::max(0.0f, std::min(up, down));
                    const auto forward = (memoryStart_ + static_cast<std::size_t>(position))
                                         % memoryCapacity_;
                    const auto backward = (memoryStart_ + static_cast<std::size_t>(
                                              memoryLength_ - 1 - position)) % memoryCapacity_;
                    const float echo = memoryBuffer_[ch][forward] * (1.0f - p.memoryReverse)
                                     + memoryBuffer_[ch][backward] * p.memoryReverse;
                    const float gain = std::pow(memoryFeedback,
                        static_cast<float>(repetition % 3));
                    memoryDelta = echo * window * gain * p.memory * memoryRecall;
                }

                memoryInstant += std::abs(memoryDelta);
                delta[ch] = pulseDelta + harmonicDelta + mutationDelta + memoryDelta;
            }

            // Real engine-to-engine meters, normalized before the next
            // sample is evaluated by the REACTION matrix.
            const float invChannels = 1.0f / static_cast<float>(channels);
            pulseSource_ += 0.015f * (std::clamp(pulseInstant * invChannels * 12.0f,
                                                0.0f, 1.0f) - pulseSource_);
            harmonicSource_ += 0.015f * (std::clamp(tonalInstant * invChannels * 18.0f,
                                                   0.0f, 1.0f) - harmonicSource_);
            grainSource_ += 0.015f * (std::clamp(grainInstant * invChannels * 9.0f,
                                                0.0f, 1.0f) - grainSource_);
            memorySource_ += 0.015f * (std::clamp(memoryInstant * invChannels * 8.0f,
                                                 0.0f, 1.0f) - memorySource_);

            if (shouldRecall)
            {
                ++memoryAge_;
                if (memoryAge_ >= memoryLength_ * 3)
                    memoryAge_ = freeze ? 0 : -1;
            }

            // Master stereo width applies to wet DIFFERENCE only:
            // the input's original mid/side balance is never altered.
            if (channels == 2)
            {
                const float mid = 0.5f * (delta[0] + delta[1]);
                const float side = 0.5f * (delta[0] - delta[1]) * smoothedWidth_;
                delta[0] = mid + side;
                delta[1] = mid - side;
            }
            // Audio-derived activity indicators: no invented/synthetic meter levels.
            const float memoryTarget = shouldRecall ? 1.0f : 0.0f;
            memoryActivity_ += 0.004f * (memoryTarget - memoryActivity_);
            const float tonalTarget = std::clamp(
                (tonalInstant / static_cast<float>(channels)) * 3.0f, 0.0f, 1.0f);
            tonalEnergy_ += 0.004f * (tonalTarget - tonalEnergy_);
            for (int channel = 0; channel < channels; ++channel)
            {
                const auto ch = static_cast<std::size_t>(channel);
                const float intensity = 0.15f + smoothedIntensity_ * 1.70f;
                const float processed = (dry[ch] + delta[ch] * smoothedMix_ * intensity)
                                          * smoothedOutput_;
                // Crossfade to untreated input when bypassed, avoiding hard clicks.
                const float magnitude = std::abs(processed);
                const float limited = magnitude <= 0.95f ? processed :
                    std::copysign(0.95f + 0.05f
                          * std::tanh((magnitude - 0.95f) * 12.0f), processed);
                // Bypassed audio is truly raw/unaltered after the short crossfade.
                const float output = raw[ch] + (limited - raw[ch]) * wetBypass_;
                data[channel][sample] = std::isfinite(output) ? output : 0.0f;
            }

            grainWrite_ = (grainWrite_ + 1) % grainCapacity_;
            if (!freeze) memoryWrite_ = (memoryWrite_ + 1) % memoryCapacity_;
            pulseWrite_ = (pulseWrite_ + 1) % pulseCapacity_;
            ++processedSamples_;
        }
    }

private:
    static constexpr double pi = 3.14159265358979323846;
    static constexpr double twoPi = 2.0 * pi;

    struct Resonator
    {
        double last = 0.0, previous = 0.0;
        float frequency = 0.0f, tone = 0.0f;
    };
    struct Grain
    {
        double readPosition = 0.0, speed = 1.0;
        int age = 0, length = 1;
        bool active = false;
    };

    // Defensive bounds on every numeric APVTS field before use.
    static Parameters sanitise(Parameters p) noexcept
    {
        const auto unit = [](float value) { return std::clamp(value, 0.0f, 1.0f); };
        p.inputGain = std::clamp(p.inputGain, 0.0f, 8.0f);
        p.outputGain = std::clamp(p.outputGain, 0.0f, 8.0f);
        p.mix = unit(p.mix);
        p.pulse = unit(p.pulse);
        p.harmonic = unit(p.harmonic);
        p.mutation = unit(p.mutation);
        p.memory = unit(p.memory);
        p.rateHz = std::clamp(p.rateHz, 0.05f, 24.0f);
        p.rootHz = std::clamp(p.rootHz, 40.0f, 500.0f);
        p.grainMs = std::clamp(p.grainMs, 35.0f, 220.0f);
        p.memoryFeedback = std::clamp(p.memoryFeedback, 0.0f, 0.95f);
        p.globalIntensity = unit(p.globalIntensity);
        p.globalSpeed = std::clamp(p.globalSpeed, 0.25f, 4.0f);
        p.globalWidth = unit(p.globalWidth);
        p.hostBpm = std::clamp(p.hostBpm, 20.0f, 300.0f);
        p.pulseSensitivity = unit(p.pulseSensitivity);
        p.pulseSwing = unit(p.pulseSwing);
        p.pulseFeedback = unit(p.pulseFeedback);
        p.pulseWidth = unit(p.pulseWidth);
        p.pulseMix = unit(p.pulseMix);
        p.pulseDivision = std::clamp(p.pulseDivision, 0, 4);
        p.harmonicMode = std::clamp(p.harmonicMode, 0, 3);
        p.harmonicResonance = unit(p.harmonicResonance);
        p.harmonicDecay = std::clamp(p.harmonicDecay, 0.1f, 9.0f);
        p.harmonicDamping = unit(p.harmonicDamping);
        p.harmonicMotion = unit(p.harmonicMotion);
        p.harmonicSpread = unit(p.harmonicSpread);
        p.mutationDensity = std::clamp(p.mutationDensity, 1.0f, 80.0f);
        p.mutationMotion = unit(p.mutationMotion);
        p.mutationPitch = std::clamp(p.mutationPitch, -12.0f, 12.0f);
        p.mutationRandom = unit(p.mutationRandom);
        p.mutationStereo = unit(p.mutationStereo);
        p.mutationCharacter = std::clamp(p.mutationCharacter, 0, 3);
        p.memoryLength = std::clamp(p.memoryLength, 0.5f, 3.0f);
        p.memoryCapture = unit(p.memoryCapture);
        p.memoryRecall = unit(p.memoryRecall);
        p.memoryReverse = unit(p.memoryReverse);
        p.memoryFade = std::clamp(p.memoryFade, 5.0f, 300.0f);
        p.memoryRepeat = std::clamp(p.memoryRepeat, 0, 3);
        return p;
    }

    float nextNoise() noexcept
    {
        noiseSeed_ ^= noiseSeed_ << 13;
        noiseSeed_ ^= noiseSeed_ >> 17;
        noiseSeed_ ^= noiseSeed_ << 5;
        return static_cast<float>(noiseSeed_ & 0xffffu) / 32767.5f - 1.0f;
    }

    float readInterpolated(const std::vector<float>& buffer, double position) const noexcept
    {
        const auto base = static_cast<std::size_t>(position) % grainCapacity_;
        const auto next = (base + 1) % grainCapacity_;
        const auto fraction = static_cast<float>(position - std::floor(position));
        return buffer[base] + (buffer[next] - buffer[base]) * fraction;
    }

    double sampleRate_ = 48000.0;
    int channels_ = 2;
    std::size_t grainCapacity_ = 0, memoryCapacity_ = 0, pulseCapacity_ = 0;
    std::size_t grainWrite_ = 0, memoryWrite_ = 0, pulseWrite_ = 0;
    std::size_t processedSamples_ = 0, memoryStart_ = 0;
    int attackCooldown_ = 0, memoryCooldown_ = 0, grainCountdown_ = 0, grainVoice_ = 0;
    int memoryAge_ = -1, memoryLength_ = 1;
    bool memoryHasCapture_ = false, smoothed_ = false;
    float phase_ = 0.0f, envelope_ = 0.0f;
    float attackActivity_ = 0.0f, rhythmicActivity_ = 0.0f;
    float pulseSource_ = 0.0f, harmonicSource_ = 0.0f;
    float grainSource_ = 0.0f, memorySource_ = 0.0f;
    float previousMemoryExcitation_ = 0.0f;
    float evolutionEnergy_ = 0.0f;
    int evolutionState_ = 0;
    ReactionMatrix reactionMatrix_;
    EvolutionEngine evolutionEngine_;
    float memoryActivity_ = 0.0f, tonalEnergy_ = 0.0f;
    float smoothedInput_ = 1.0f, smoothedOutput_ = 1.0f, smoothedMix_ = 0.65f;
    float smoothedWidth_ = 0.8f, smoothedIntensity_ = 0.5f, wetBypass_ = 1.0f;
    int pulseDelayCurrent_ = 0, pulseDelayOld_ = 0;
    float pulseTapFade_ = 1.0f;
    std::array<float, 2> pulseFeedbackTone_ {};
    GuitarPitchTracker guitarPitch_;
    float trackedRoot_ = 0.0f, pitchConfidence_ = 0.0f;
    float smoothedHarmonicRoot_ = 0.0f;
    int trackedMidiNote_ = -1;
    uint32_t noiseSeed_ = 0x8fe34ae1u;
    std::array<std::vector<float>, 2> grainsBuffer_, memoryBuffer_, pulseBuffer_;
    std::array<std::array<Resonator, 3>, 2> resonators_;
    std::array<std::array<Grain, 2>, 2> grains_;
};
} // namespace biosphere

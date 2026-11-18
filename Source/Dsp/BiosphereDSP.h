#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace biosphere
{
struct Parameters
{
    float inputGain = 1.0f;
    float outputGain = 1.0f;
    float mix = 0.65f;
    float pulse = 0.35f;
    float harmonic = 0.30f;
    float mutation = 0.20f;
    float memory = 0.20f;
    float rateHz = 2.0f;
    float rootHz = 110.0f;
    float grainMs = 95.0f;
    float memoryFeedback = 0.4f;
};

// JUCE-independent DSP core. All buffers are allocated in prepare(), never in process().
class BiosphereDSP
{
public:
    void prepare(double sampleRate, int, int channels)
    {
        sampleRate_ = std::max(8000.0, sampleRate);
        channels_ = std::clamp(channels, 1, 2);
        capacity_ = static_cast<std::size_t>(std::ceil(sampleRate_ * 3.0));
        for (int channel = 0; channel < 2; ++channel)
        {
            grainBuffer_[channel].assign(capacity_, 0.0f);
            memoryBuffer_[channel].assign(capacity_, 0.0f);
        }
        reset();
    }

    void reset() noexcept
    {
        phase_ = envelope_ = 0.0f;
        writePosition_ = 0;
        processedSamples_ = 0;
        attackCooldown_ = 0;
        memoryCooldown_ = 0;
        grainCountdown_ = 0;
        grainVoice_ = 0;
        memoryAge_ = -1;
        memoryStart_ = 0;
        memoryLength_ = 1;
        for (auto& channel : resonators_)
            for (auto& state : channel)
                state = {};
        for (auto& channel : grains_)
            for (auto& voice : channel)
                voice = {};
        for (auto& channel : grainBuffer_)
            std::fill(channel.begin(), channel.end(), 0.0f);
        for (auto& channel : memoryBuffer_)
            std::fill(channel.begin(), channel.end(), 0.0f);
    }

    void process(float* const* data, int channels, int samples, const Parameters& requested) noexcept
    {
        if (capacity_ == 0 || data == nullptr || samples <= 0)
            return;

        channels = std::clamp(channels, 0, channels_);
        if (channels == 0)
            return;

        const auto p = sanitise(requested);
        const int grainLength = std::clamp(static_cast<int>(sampleRate_ * p.grainMs * 0.001f),
                                           64, static_cast<int>(capacity_ / 4));
        const int grainSpacing = std::max(32, grainLength / 2);
        const float angleIncrement = static_cast<float>(twoPi * p.rateHz / sampleRate_);
        const int captureLength = std::max(64, static_cast<int>(sampleRate_ * 0.27));
        const auto captureOffset = static_cast<std::size_t>(sampleRate_ * 0.42);
        const int memoryWait = static_cast<int>(sampleRate_ * 0.46);

        for (int sample = 0; sample < samples; ++sample)
        {
            std::array<float, 2> input {};
            float level = 0.0f;
            for (int channel = 0; channel < channels; ++channel)
            {
                input[channel] = data[channel][sample] * p.inputGain;
                level += std::abs(input[channel]);
            }
            level /= static_cast<float>(channels);

            // Shared performance analysis: envelope and refractory-limited onset detector.
            const float previousEnvelope = envelope_;
            const float smoothing = level > envelope_ ? 0.06f : 0.002f;
            envelope_ += smoothing * (level - envelope_);
            if (attackCooldown_ > 0) --attackCooldown_;
            if (memoryCooldown_ > 0) --memoryCooldown_;
            const bool onset = attackCooldown_ == 0 && level > 0.09f
                               && level > previousEnvelope * 2.25f;
            if (onset) attackCooldown_ = static_cast<int>(sampleRate_ * 0.12);

            // Memory recalls an earlier, windowed slice; never captures into heap memory.
            if (onset && memoryCooldown_ == 0 && p.memory > 0.001f
                && processedSamples_ > static_cast<std::size_t>(sampleRate_ * 0.55))
            {
                memoryStart_ = (writePosition_ + capacity_ - captureOffset) % capacity_;
                memoryLength_ = captureLength;
                memoryAge_ = 0;
                memoryCooldown_ = memoryWait;
            }

            // Two overlapping grains, shared timing between stereo channels.
            if (--grainCountdown_ <= 0)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    auto& voice = grains_[channel][grainVoice_];
                    voice.active = true;
                    voice.age = 0;
                    voice.length = grainLength;
                    voice.readPosition = static_cast<double>(
                        (writePosition_ + capacity_ - static_cast<std::size_t>(grainLength + 2)) % capacity_);
                    voice.speed = (grainVoice_ == 0 ? 0.86 : 1.14);
                }
                grainVoice_ = 1 - grainVoice_;
                grainCountdown_ = grainSpacing;
            }

            phase_ += angleIncrement;
            if (phase_ >= static_cast<float>(twoPi))
                phase_ -= static_cast<float>(twoPi);

            const float pulseShape = 0.5f + 0.5f * std::sin(phase_);
            const float dynamicDepth = 0.3f + 0.7f * std::clamp(envelope_ * 3.0f, 0.0f, 1.0f);

            for (int channel = 0; channel < channels; ++channel)
            {
                const float dry = input[channel];
                grainBuffer_[channel][writePosition_] = dry;
                memoryBuffer_[channel][writePosition_] = dry;

                // PULSE: an amplitude gate driven by a continuous rhythmic oscillator.
                const float pulseWet = dry * (1.0f - p.pulse * dynamicDepth * (1.0f - pulseShape));

                // HARMONIC LIFE: three independently tuned, damped resonators.
                const std::array<float, 3> ratios { 1.0f, 1.5f, 2.0f };
                float harmonicWet = 0.0f;
                for (int voice = 0; voice < 3; ++voice)
                {
                    const auto frequency = std::clamp(p.rootHz * ratios[voice], 40.0f,
                                                       static_cast<float>(sampleRate_ * 0.42));
                    const double radius = std::exp(-static_cast<double>(twoPi) * frequency
                                                    / (15.0 * sampleRate_));
                    const double angle = twoPi * frequency / sampleRate_;
                    auto& state = resonators_[channel][voice];
                    const double next = (1.0 - radius) * dry
                                        + 2.0 * radius * std::cos(angle) * state.last
                                        - radius * radius * state.previous;
                    state.previous = state.last;
                    state.last = std::clamp(next, -5.0, 5.0);
                    harmonicWet += static_cast<float>(std::tanh(state.last * 0.65));
                }
                harmonicWet *= p.harmonic * 0.24f;

                // MUTATION: two windowed, interpolated grains reading the circular buffer.
                float grainWet = 0.0f;
                for (auto& grain : grains_[channel])
                {
                    if (!grain.active) continue;
                    const float progress = static_cast<float>(grain.age) / static_cast<float>(grain.length);
                    const float window = std::sin(static_cast<float>(pi) * progress);
                    grainWet += readInterpolated(grainBuffer_[channel], grain.readPosition)
                                * window * window * 0.65f;
                    grain.readPosition += grain.speed;
                    if (grain.readPosition >= static_cast<double>(capacity_))
                        grain.readPosition -= static_cast<double>(capacity_);
                    if (++grain.age >= grain.length) grain.active = false;
                }

                // MEMORY: three fading replays of a captured 270 ms fragment.
                float memoryWet = 0.0f;
                if (memoryAge_ >= 0 && memoryAge_ < memoryLength_ * 3)
                {
                    const int position = memoryAge_ % memoryLength_;
                    const int repetition = memoryAge_ / memoryLength_;
                    const float progress = static_cast<float>(position) / static_cast<float>(memoryLength_);
                    const float window = std::sin(static_cast<float>(pi) * progress);
                    const float attenuation = std::pow(p.memoryFeedback, static_cast<float>(repetition));
                    const auto index = (memoryStart_ + static_cast<std::size_t>(position)) % capacity_;
                    memoryWet = memoryBuffer_[channel][index] * window * window * attenuation * p.memory;
                }

                float wet = pulseWet + harmonicWet + grainWet * p.mutation + memoryWet;
                float output = (dry + (wet - dry) * p.mix) * p.outputGain;

                // Soft overload protection; unity dry signal passes unchanged below 0.95.
                const float magnitude = std::abs(output);
                if (magnitude > 0.95f)
                    output = std::copysign(0.95f + 0.05f * std::tanh((magnitude - 0.95f) * 12.0f), output);

                data[channel][sample] = std::isfinite(output) ? output : 0.0f;
            }
            if (memoryAge_ >= 0 && ++memoryAge_ >= memoryLength_ * 3)
                memoryAge_ = -1;
            writePosition_ = (writePosition_ + 1) % capacity_;
            ++processedSamples_;
        }
    }

private:
    static constexpr double pi = 3.14159265358979323846;
    static constexpr double twoPi = 2.0 * pi;

    struct Resonator { double last = 0.0, previous = 0.0; };
    struct Grain
    {
        double readPosition = 0.0;
        double speed = 1.0;
        int age = 0, length = 1;
        bool active = false;
    };

    static Parameters sanitise(Parameters p) noexcept
    {
        p.inputGain = std::clamp(p.inputGain, 0.0f, 4.0f);
        p.outputGain = std::clamp(p.outputGain, 0.0f, 4.0f);
        p.mix = std::clamp(p.mix, 0.0f, 1.0f);
        p.pulse = std::clamp(p.pulse, 0.0f, 1.0f);
        p.harmonic = std::clamp(p.harmonic, 0.0f, 1.0f);
        p.mutation = std::clamp(p.mutation, 0.0f, 1.0f);
        p.memory = std::clamp(p.memory, 0.0f, 1.0f);
        p.rateHz = std::clamp(p.rateHz, 0.05f, 12.0f);
        p.rootHz = std::clamp(p.rootHz, 40.0f, 500.0f);
        p.grainMs = std::clamp(p.grainMs, 35.0f, 220.0f);
        p.memoryFeedback = std::clamp(p.memoryFeedback, 0.0f, 0.95f);
        return p;
    }

    float readInterpolated(const std::vector<float>& buffer, double position) const noexcept
    {
        const auto first = static_cast<std::size_t>(position);
        const auto second = (first + 1) % capacity_;
        const auto fraction = static_cast<float>(position - static_cast<double>(first));
        return buffer[first] + (buffer[second] - buffer[first]) * fraction;
    }

    double sampleRate_ = 48000.0;
    int channels_ = 2;
    std::size_t capacity_ = 0, writePosition_ = 0, processedSamples_ = 0, memoryStart_ = 0;
    int attackCooldown_ = 0, memoryCooldown_ = 0, grainCountdown_ = 0;
    int grainVoice_ = 0, memoryAge_ = -1, memoryLength_ = 1;
    float phase_ = 0.0f, envelope_ = 0.0f;
    std::array<std::vector<float>, 2> grainBuffer_, memoryBuffer_;
    std::array<std::array<Resonator, 3>, 2> resonators_;
    std::array<std::array<Grain, 2>, 2> grains_;
};
} // namespace biosphere

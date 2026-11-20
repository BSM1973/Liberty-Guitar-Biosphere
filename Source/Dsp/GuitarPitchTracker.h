#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace biosphere
{
// JUCE-free monophonic guitar tracker: downsampled YIN-style normalized
// difference with parabolic lag refinement and note-change hysteresis.
// Designed for E2-E5 and normal bends/vibrato; not a polyphonic chord tracker.
// All memory is fixed-size and allocated with the DSP object, never in process.
struct GuitarPitchFrame
{
    float frequencyHz = 0.0f;
    float confidence = 0.0f;
    float rms = 0.0f;
    int midiNote = -1;
    bool voiced = false;
};

class GuitarPitchTracker final
{
public:
    void prepare(double sampleRate) noexcept
    {
        sampleRate_ = std::clamp(sampleRate, 8000.0, 384000.0);
        decimation_ = std::max(1, static_cast<int>(std::lround(sampleRate_ / 11025.0)));
        analysisRate_ = sampleRate_ / static_cast<double>(decimation_);
        minLag_ = std::max(8, static_cast<int>(analysisRate_ / 920.0));
        maxLag_ = std::clamp(static_cast<int>(std::ceil(analysisRate_ / 74.0)),
                             minLag_ + 3, 200);
        filterAlpha_ = static_cast<float>(1.0 - std::exp(-2.0 * 3.141592653589793
                                                        * 1700.0 / sampleRate_));
        reset();
    }

    void reset() noexcept
    {
        buffer_.fill(0.0f);
        differences_.fill(0.0f);
        normalized_.fill(1.0f);
        write_ = decimatedCount_ = decimationCount_ = hop_ = 0;
        attackHold_ = 0;
        silenceHops_ = 0;
        candidateCount_ = 0;
        pendingHz_ = filtered_ = trackedHz_ = 0.0f;
        frame_ = {};
    }

    void processSample(float sample, bool onset) noexcept
    {
        if (!std::isfinite(sample)) sample = 0.0f;
        sample = std::clamp(sample, -2.0f, 2.0f);
        if (onset)
            attackHold_ = static_cast<int>(sampleRate_ * 0.024);
        if (attackHold_ > 0) --attackHold_;

        filtered_ += filterAlpha_ * (sample - filtered_);
        if (++decimationCount_ < decimation_) return;
        decimationCount_ = 0;
        buffer_[static_cast<std::size_t>(write_)] = filtered_;
        write_ = (write_ + 1) & (bufferSize - 1);
        decimatedCount_ = std::min(decimatedCount_ + 1, bufferSize);
        if (++hop_ < analysisHop || decimatedCount_ < window + maxLag_ + 2)
            return;
        hop_ = 0;
        analyse();
    }

    GuitarPitchFrame frame() const noexcept { return frame_; }
    double analysisRate() const noexcept { return analysisRate_; }

private:
    static constexpr int bufferSize = 1024; // power-of-two circular addressing
    static constexpr int window = 512;
    static constexpr int analysisHop = 192; // ~17ms at 11kHz
    std::array<float, bufferSize> buffer_ {};
    std::array<float, 202> differences_ {};
    std::array<float, 202> normalized_ {};

    float sampleDelayed(int delay) const noexcept
    {
        return buffer_[static_cast<std::size_t>(
            (write_ - 1 - delay + 2 * bufferSize) & (bufferSize - 1))];
    }

    void analyse() noexcept
    {
        double power = 0.0;
        for (int i = 0; i < window; ++i)
        {
            const float sample = sampleDelayed(i);
            power += static_cast<double>(sample) * sample;
        }
        frame_.rms = static_cast<float>(std::sqrt(power / window));
        // Silence or released notes: retain the last pitch only briefly,
        // avoiding random frequencies from hiss or the tail of an effect.
        if (frame_.rms < 0.007f)
        {
            ++silenceHops_;
            if (silenceHops_ >= 8)
            {
                trackedHz_ = 0.0f;
                frame_.voiced = false;
                frame_.frequencyHz = frame_.confidence = 0.0f;
                frame_.midiNote = -1;
                candidateCount_ = 0;
            }
            return;
        }
        silenceHops_ = 0;
        // Ignore the immediate scratch/broadband transient after an onset;
        // a new stable note can be identified within the next analysis hops.
        if (attackHold_ > 0) return;

        for (int lag = 1; lag <= maxLag_ + 1; ++lag)
        {
            double difference = 0.0;
            for (int i = 0; i < window; ++i)
            {
                const double delta = sampleDelayed(i) - sampleDelayed(i + lag);
                difference += delta * delta;
            }
            differences_[static_cast<std::size_t>(lag)]
                = static_cast<float>(difference);
        }
        double running = 0.0;
        for (int lag = 1; lag <= maxLag_ + 1; ++lag)
        {
            running += differences_[static_cast<std::size_t>(lag)];
            normalized_[static_cast<std::size_t>(lag)] =
                static_cast<float>(differences_[static_cast<std::size_t>(lag)]
                                   * lag / (running + 1.0e-12));
        }

        int best = -1;
        float bestValue = 1.0f;
        // First significant local minimum is preferred to higher harmonics
        // or doubled periods; fallback only for high-confidence minima.
        for (int lag = minLag_; lag <= maxLag_; ++lag)
        {
            const float value = normalized_[static_cast<std::size_t>(lag)];
            if (value < 0.16f
                && value <= normalized_[static_cast<std::size_t>(lag - 1)]
                && value <= normalized_[static_cast<std::size_t>(lag + 1)])
            {
                best = lag;
                bestValue = value;
                break;
            }
        }
        if (best < 0)
        {
            for (int lag = minLag_; lag <= maxLag_; ++lag)
            {
                const float value = normalized_[static_cast<std::size_t>(lag)];
                if (value < bestValue)
                {
                    best = lag;
                    bestValue = value;
                }
            }
            if (bestValue > 0.19f) best = -1;
        }
        if (best < 0)
        {
            frame_.confidence *= 0.78f;
            return;
        }

        const float confidence = std::clamp(1.0f - bestValue, 0.0f, 1.0f);
        if (confidence < 0.81f) return;
        const float left = normalized_[static_cast<std::size_t>(best - 1)];
        const float middle = normalized_[static_cast<std::size_t>(best)];
        const float right = normalized_[static_cast<std::size_t>(best + 1)];
        const float denominator = left - 2.0f * middle + right;
        const float offset = std::abs(denominator) > 1.0e-8f
            ? std::clamp(0.5f * (left - right) / denominator, -0.5f, 0.5f)
            : 0.0f;
        const float hz = static_cast<float>(analysisRate_ / (best + offset));
        if (!std::isfinite(hz) || hz < 75.0f || hz > 920.0f) return;

        if (trackedHz_ > 0.0f)
        {
            const float semitones = std::abs(12.0f * std::log2(hz / trackedHz_));
            // Require two consecutive estimates for large interval jumps,
            // preventing a pick transient or an octave subharmonic from
            // retuning HARMONIC LIFE instantly.
            if (semitones >= 1.0f)
            {
                if (pendingHz_ <= 0.0f
                    || std::abs(12.0f * std::log2(hz / pendingHz_)) > 0.65f)
                {
                    pendingHz_ = hz;
                    candidateCount_ = 1;
                    return;
                }
                if (++candidateCount_ < 2) return;
                trackedHz_ = hz;
                pendingHz_ = 0.0f;
                candidateCount_ = 0;
            }
            else
            {
                trackedHz_ *= std::exp2(0.32f * std::log2(hz / trackedHz_));
                candidateCount_ = 0;
                pendingHz_ = 0.0f;
            }
        }
        else
        {
            trackedHz_ = hz;
            pendingHz_ = 0.0f;
            candidateCount_ = 0;
        }

        frame_.frequencyHz = trackedHz_;
        frame_.confidence = confidence;
        frame_.midiNote = std::clamp(
            static_cast<int>(std::lround(69.0 + 12.0 * std::log2(trackedHz_ / 440.0f))),
            0, 127);
        frame_.voiced = true;
    }

    double sampleRate_ = 48000.0, analysisRate_ = 12000.0;
    int decimation_ = 4, minLag_ = 12, maxLag_ = 164;
    int write_ = 0, decimatedCount_ = 0, decimationCount_ = 0, hop_ = 0;
    int attackHold_ = 0, silenceHops_ = 0, candidateCount_ = 0;
    float filterAlpha_ = 0.2f, filtered_ = 0.0f;
    float trackedHz_ = 0.0f, pendingHz_ = 0.0f;
    GuitarPitchFrame frame_ {};
};
} // namespace biosphere

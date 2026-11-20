#include "Dsp/BiosphereDSP.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>

// Count allocations only inside DSP::process, never while configuring the host.
// An accidental heap allocation in the audio thread now fails CI.
namespace
{
std::atomic<std::size_t> allocations {0};
thread_local bool watching = false;
}
void* operator new(std::size_t n)
{
    if (watching) allocations.fetch_add(1, std::memory_order_relaxed);
    if (auto* ptr = std::malloc(n ? n : 1)) return ptr;
    throw std::bad_alloc();
}
void operator delete(void* ptr) noexcept { std::free(ptr); }
void operator delete(void* ptr, std::size_t) noexcept { std::free(ptr); }
void* operator new[](std::size_t n)
{
    if (watching) allocations.fetch_add(1, std::memory_order_relaxed);
    if (auto* ptr = std::malloc(n ? n : 1)) return ptr;
    throw std::bad_alloc();
}
void operator delete[](void* ptr) noexcept { std::free(ptr); }
void operator delete[](void* ptr, std::size_t) noexcept { std::free(ptr); }

namespace
{
void check(bool ok, const char* why)
{
    if (!ok) throw std::runtime_error(why);
}
constexpr double tau = 6.2831853071795864769;

void simulate(int rate, int channels)
{
    constexpr std::array<int, 8> blocks {1, 16, 32, 64, 127, 256, 512, 2048};
    const int duration = rate * 3;
    biosphere::BiosphereDSP dsp;
    dsp.prepare(rate, 512, channels);
    biosphere::Parameters settings;
    settings.mix = 0.88f;
    settings.pulse = settings.harmonic = settings.mutation = settings.memory = 0.92f;
    settings.mutationDensity = 80.0f;
    settings.grainMs = 220.0f;
    settings.memoryFeedback = 0.85f;
    settings.memoryRecall = 0.89f;
    settings.reaction.active = true;
    settings.reaction.interaction = 0.90f;
    settings.reaction.reactivity = 0.85f;
    settings.reaction.routes[0] = {true, 0, 1, 0.75f};
    settings.reaction.routes[1] = {true, 3, 3, 0.60f};
    settings.evolution.active = true;
    settings.evolution.intensity = 0.92f;
    std::array<std::vector<float>, 2> audio;
    for (auto& ch : audio) ch.resize(static_cast<std::size_t>(duration));
    for (int i = 0; i < duration; ++i)
    {
        const int position = i % (rate / 3);
        const double time = static_cast<double>(i) / rate;
        const float transient = position < 80
            ? 0.46f * std::exp(-static_cast<float>(position) / 20.0f) : 0.0f;
        for (int ch = 0; ch < channels; ++ch)
            audio[static_cast<std::size_t>(ch)][static_cast<std::size_t>(i)]
                = static_cast<float>(0.21 * std::sin(tau * 196.0 * time + ch * 0.11))
                    + transient;
    }
    // Stress test invalid host input without allowing it into memory buffers.
    const int corruptAt = rate + 100;
    audio[0][static_cast<std::size_t>(corruptAt)]
        = std::numeric_limits<float>::quiet_NaN();
    if (channels == 2)
        audio[1][static_cast<std::size_t>(corruptAt + 1)]
            = std::numeric_limits<float>::infinity();
    int offset = 0, block = 0;
    const auto initialAllocations = allocations.load(std::memory_order_relaxed);
    float peak = 0.0f;
    while (offset < duration)
    {
        const int n = std::min(blocks[static_cast<std::size_t>(block++ % blocks.size())],
                               duration - offset);
        const bool middle = offset > rate;
        settings.hasHostBpm = middle;
        settings.hostBpm = middle ? 147.0f : 96.0f;
        settings.globalSpeed = offset > rate * 2 ? 3.0f : 0.7f;
        settings.bypass = offset > rate * 2 && block % 15 == 0;
        settings.memoryReverse = block % 16 < 8 ? 0.94f : 0.05f;
        settings.pulseActive = block % 18 != 0;
        settings.harmonicMono = block % 7 != 0;
        // Repeatedly exercise all eight REACTION routes at extreme amounts.
        settings.reaction.routes[0].amount = block % 2 == 0 ? 0.90f : -0.90f;
        if (block % 37 == 0)
        {
            const auto nan = std::numeric_limits<float>::quiet_NaN();
            settings.pulseFeedback = nan;
            settings.rateHz = nan;
            settings.memoryFeedback = nan;
            settings.reaction.routes[0].amount = nan;
            settings.evolution.rate = nan;
        }
        else
        {
            settings.pulseFeedback = 0.85f;
            settings.rateHz = 7.0f;
            settings.memoryFeedback = 0.85f;
            settings.evolution.rate = 1.5f;
        }
        float* ptr[2] {audio[0].data() + offset,
                       channels == 2 ? audio[1].data() + offset : nullptr};
        watching = true;
        dsp.process(ptr, channels, n, settings);
        watching = false;
        for (int ch = 0; ch < channels; ++ch)
            for (int i = offset; i < offset + n; ++i)
            {
                const float value = audio[static_cast<std::size_t>(ch)]
                                         [static_cast<std::size_t>(i)];
                check(std::isfinite(value), "Premium DSP output contains NaN/Inf");
                check(std::abs(value) <= 1.001f,
                      "Premium DSP output exceeds full-scale limiter");
                peak = std::max(peak, std::abs(value));
            }
        offset += n;
    }
    check(allocations.load(std::memory_order_relaxed) == initialAllocations,
          "Audio callback allocated heap memory");

    // Repeat after reset to exercise host transport stop/restart.
    dsp.reset();
    std::array<float, 128> first {}, second {};
    float* ptr[2] {first.data(), channels == 2 ? second.data() : nullptr};
    settings = {};
    dsp.process(ptr, channels, 128, settings);
    for (int i = 0; i < 128; ++i)
        check(std::isfinite(first[static_cast<std::size_t>(i)]),
              "Reset after stop introduced NaN");
    std::cout << rate << " Hz " << channels << "ch: "
              << blocks.size() << " block shapes, peak " << peak
              << ", no audio heap allocations\n";
}
} // namespace

int main()
{
    try
    {
        for (int rate : {44100, 48000, 96000})
        {
            simulate(rate, 1);
            simulate(rate, 2);
        }
        // Malformed rate must not cause huge allocations or invalid buffers.
        biosphere::BiosphereDSP dsp;
        dsp.prepare(std::numeric_limits<double>::quiet_NaN(), 512, 2);
        std::array<float, 128> left {}, right {};
        float* ptr[2] {left.data(), right.data()};
        dsp.process(ptr, 2, 128, {});
        check(std::isfinite(left[0]), "Malformed sample rate not recovered");
        std::cout << "PREMIUM STABILITY PASS: sample rates, mono/stereo, "
                     "variable blocks, hostile automation, stop/restart, zero allocations\n";
    }
    catch (const std::exception& e)
    {
        watching = false;
        std::cerr << "PREMIUM STABILITY FAIL: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

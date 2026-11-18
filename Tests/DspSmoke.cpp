#include "Dsp/BiosphereDSP.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr int sampleRate = 48000;

std::vector<float> makeGuitarLikeSignal()
{
    std::vector<float> signal(sampleRate * 3);
    for (std::size_t n = 0; n < signal.size(); ++n)
    {
        const float t = static_cast<float>(n) / static_cast<float>(sampleRate);
        const float harmonic = 0.18f * std::sin(2.0f * 3.14159265f * 220.0f * t);
        const float attack = n % 19000 < 180
            ? 0.45f * std::exp(-static_cast<float>(n % 19000) / 45.0f) : 0.0f;
        signal[n] = harmonic + attack;
    }
    return signal;
}

std::vector<float> render(const std::vector<float>& source, biosphere::Parameters parameters)
{
    auto output = source;
    biosphere::BiosphereDSP engine;
    engine.prepare(sampleRate, 256, 1);
    for (std::size_t offset = 0; offset < output.size(); offset += 256)
    {
        float* channels[] = { output.data() + offset };
        engine.process(channels, 1,
            static_cast<int>(std::min<std::size_t>(256, output.size() - offset)), parameters);
    }
    return output;
}

void verifyDifference(const char* name, const std::vector<float>& source,
                      const std::vector<float>& processed)
{
    double difference = 0.0;
    for (std::size_t i = 0; i < source.size(); ++i)
    {
        if (!std::isfinite(processed[i]) || std::abs(processed[i]) > 1.001f)
            throw std::runtime_error(std::string(name) + ": non-finite or unsafe output");
        difference += std::abs(source[i] - processed[i]);
    }
    if (difference < 0.1)
        throw std::runtime_error(std::string(name) + ": no measurable signal difference");
    std::cout << name << ": PASS (difference=" << difference << ")\n";
}

void verifyDryTransparency(const std::vector<float>& source)
{
    biosphere::Parameters p;
    p.mix = 1.0f;
    p.pulse = p.harmonic = p.mutation = p.memory = 0.0f;
    const auto dry = render(source, p);
    for (std::size_t i = 0; i < dry.size(); ++i)
        if (std::abs(dry[i] - source[i]) > 0.00001f)
            throw std::runtime_error("Dry signal was modified");
    std::cout << "Dry transparency: PASS\n";
}
} // namespace

int main()
{
    try
    {
        const auto source = makeGuitarLikeSignal();
        verifyDryTransparency(source);
        biosphere::Parameters p;
        p.mix = 1.0f;
        p.pulse = p.harmonic = p.mutation = p.memory = 0.0f;
        p.pulse = 0.8f;
        verifyDifference("PULSE", source, render(source, p));
        p.pulse = 0.0f; p.harmonic = 0.7f;
        verifyDifference("HARMONIC LIFE", source, render(source, p));
        p.harmonic = 0.0f; p.mutation = 0.7f;
        verifyDifference("MUTATION", source, render(source, p));
        p.mutation = 0.0f; p.memory = 0.9f;
        verifyDifference("MEMORY", source, render(source, p));
        std::cout << "DSP smoke test: PASS\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "DSP smoke test: FAIL - " << e.what() << "\n";
        return 1;
    }
}

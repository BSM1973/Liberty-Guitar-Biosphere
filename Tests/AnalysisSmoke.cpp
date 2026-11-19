#include "Dsp/BiosphereDSP.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

int main()
{
    constexpr int rate = 48000;
    biosphere::BiosphereDSP engine;
    engine.prepare(rate, 256, 1);
    biosphere::Parameters p;
    p.mix = 0.8f;
    p.memory = 0.90f;
    p.memoryRecall = 1.0f;
    p.memoryLength = 0.6f;
    p.memoryCapture = 1.0f;
    p.pulse = 0.9f;
    p.harmonic = 0.9f;

    std::vector<float> block(256, 0.0f);
    float maxEnvelope = 0, maxOnsets = 0, maxRhythm = 0;
    float maxMemory = 0, maxTonal = 0, maxRoot = 0;
    for (int offset = 0; offset < rate * 3; offset += 256)
    {
        const int count = std::min<int>(256, rate * 3 - offset);
        for (int i = 0; i < count; ++i)
        {
            const int frame = offset + i;
            const float t = static_cast<float>(frame) / rate;
            const int inNote = frame % 12000;
            const float attack = inNote < 120
                ? 0.85f * std::exp(-static_cast<float>(inNote) / 28.0f) : 0.0f;
            block[static_cast<std::size_t>(i)] =
                0.19f * std::sin(2.0f * 3.14159265f * 196.0f * t) + attack;
        }
        float* output[] {block.data()};
        engine.process(output, 1, count, p);
        const auto values = engine.getAnalysisValues();
        for (auto number : { values.envelope, values.attacks, values.rhythmicActivity,
                             values.memoryActivity, values.tonalEnergy, values.rootHz })
            if (!std::isfinite(number) || number < 0.0f)
                throw std::runtime_error("Non-finite or negative DSP analysis data");
        maxEnvelope = std::max(maxEnvelope, values.envelope);
        maxOnsets = std::max(maxOnsets, values.attacks);
        maxRhythm = std::max(maxRhythm, values.rhythmicActivity);
        maxMemory = std::max(maxMemory, values.memoryActivity);
        maxTonal = std::max(maxTonal, values.tonalEnergy);
        maxRoot = std::max(maxRoot, values.rootHz);
    }
    if (maxEnvelope < 0.02f || maxOnsets < 0.05f || maxRhythm < 0.01f
        || maxMemory < 0.02f || maxTonal < 0.01f)
        throw std::runtime_error("One or more shared analysis meters are disconnected");
    // Root is an opportunistic monophonic zero-crossing estimate, not
    // guaranteed on noisy or polyphonic inputs, but our sine must yield it.
    if (maxRoot < 65.0f || maxRoot > 520.0f)
        throw std::runtime_error("Root frequency analyzer did not track monophonic test tone");

    engine.reset();
    const auto reset = engine.getAnalysisValues();
    if (reset.envelope != 0.0f || reset.memoryActivity != 0.0f || reset.rootHz != 0.0f)
        throw std::runtime_error("Analysis reset did not clear data");

    std::cout << "Shared DSP analysis: PASS (envelope=" << maxEnvelope
              << ", attacks=" << maxOnsets << ", memory=" << maxMemory
              << ", tonal=" << maxTonal << ", root=" << maxRoot << " Hz)\n";
    return 0;
}

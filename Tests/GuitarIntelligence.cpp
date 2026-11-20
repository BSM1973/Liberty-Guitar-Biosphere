#include "Dsp/GuitarPitchTracker.h"
#include "Dsp/BiosphereDSP.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
constexpr double pi = 3.14159265358979323846;
void need(bool test, const char* reason)
{
    if (!test) throw std::runtime_error(reason);
}
float cents(float measured, float reference)
{
    return 1200.0f * std::log2(measured / reference);
}
biosphere::GuitarPitchFrame play(biosphere::GuitarPitchTracker& tracker,
                                 int sr, float hz, float seconds,
                                 float amplitude = 0.21f, bool harmonics = true)
{
    const int samples = static_cast<int>(seconds * sr);
    biosphere::GuitarPitchFrame frame;
    for (int i = 0; i < samples; ++i)
    {
        const double angle = 2.0 * pi * hz * i / sr;
        const float sample = amplitude * (
            std::sin(angle) + (harmonics ? 0.30f * std::sin(2.0 * angle)
                                        + 0.12f * std::sin(3.0 * angle) : 0.0f));
        tracker.processSample(sample, i == 0);
        frame = tracker.frame();
        need(std::isfinite(frame.confidence) && std::isfinite(frame.frequencyHz),
             "Pitch tracker created invalid values");
    }
    return frame;
}

void testRate(int sampleRate)
{
    biosphere::GuitarPitchTracker tracker;
    tracker.prepare(sampleRate);

    const auto e2 = play(tracker, sampleRate, 82.4069f, 0.30f);
    need(e2.voiced && std::abs(cents(e2.frequencyHz, 82.4069f)) < 45.0f,
         "Low E2 guitar string was not tracked");

    const auto g3 = play(tracker, sampleRate, 196.0f, 0.38f);
    need(g3.voiced && std::abs(cents(g3.frequencyHz, 196.0f)) < 30.0f,
         "Monophonic G3 note change was not tracked");
    need(g3.midiNote == 55, "Detected MIDI note for G3 is incorrect");
    need(g3.confidence > 0.82f, "Stable note confidence is too low");

    const auto e4 = play(tracker, sampleRate, 329.6276f, 0.38f);
    need(e4.voiced && std::abs(cents(e4.frequencyHz, 329.6276f)) < 35.0f,
         "Monophonic E4 note change was not tracked");

    const auto a4 = play(tracker, sampleRate, 440.0f, 0.37f);
    need(a4.voiced && std::abs(cents(a4.frequencyHz, 440.0f)) < 35.0f,
         "Monophonic A4 note was not tracked");

    for (int sample = 0; sample < sampleRate; ++sample)
        tracker.processSample(0.0f, false);
    const auto quiet = tracker.frame();
    need(!quiet.voiced && quiet.frequencyHz == 0.0f
         && quiet.midiNote == -1, "Pitch must clear on sustained silence");

    const auto recovered = play(tracker, sampleRate, 146.8324f, 0.35f);
    need(recovered.voiced && std::abs(cents(recovered.frequencyHz, 146.8324f)) < 35.0f,
         "Tracking did not recover after silence");
}

std::vector<float> guitar()
{
    constexpr int sr = 48000;
    constexpr int count = sr * 2;
    std::vector<float> source(static_cast<std::size_t>(count), 0.0f);
    for (int i = 0; i < count; ++i)
    {
        const auto angle = 2.0 * pi * 196.0 * i / sr;
        const float transient = i % 12000 < 90
            ? 0.55f * std::exp(-static_cast<float>(i % 12000) / 26.0f) : 0.0f;
        source[static_cast<std::size_t>(i)] =
            0.24f * static_cast<float>(std::sin(angle)
                       + 0.22 * std::sin(2.0 * angle)) + transient;
    }
    return source;
}

struct Render
{
    std::vector<float> samples;
    biosphere::AnalysisValues analysis;
};

Render process(const std::vector<float>& source, bool follow)
{
    biosphere::BiosphereDSP engine;
    engine.prepare(48000, 256, 1);
    biosphere::Parameters settings;
    settings.pulse = settings.mutation = settings.memory = 0.0f;
    settings.harmonic = 1.0f;
    settings.mix = 0.93f;
    settings.rootHz = 110.0f;
    settings.harmonicMono = follow;
    settings.harmonicResonance = 0.9f;
    settings.harmonicMode = 2; // tempered thirds & fifths
    auto out = source;
    for (std::size_t pos = 0; pos < out.size(); pos += 256)
    {
        float* data[2] {out.data() + pos, nullptr};
        engine.process(data, 1, static_cast<int>(
            std::min<std::size_t>(256, out.size() - pos)), settings);
    }
    return {out, engine.getAnalysisValues()};
}
}

int main()
{
    try
    {
        for (const int rate : {44100, 48000, 96000})
            testRate(rate);

        const auto input = guitar();
        const auto staticHarmony = process(input, false);
        const auto trackedHarmony = process(input, true);
        need(trackedHarmony.analysis.rootHz > 185.0f
             && trackedHarmony.analysis.rootHz < 207.0f,
             "DSP did not publish stable note tracking");
        need(trackedHarmony.analysis.midiNote == 55,
             "DSP musical note is disconnected");
        need(trackedHarmony.analysis.pitchConfidence > 0.80f,
             "DSP confidence was not forwarded");

        double difference = 0.0;
        for (std::size_t i = 0; i < input.size(); ++i)
        {
            difference += std::abs(staticHarmony.samples[i] - trackedHarmony.samples[i]);
            need(std::isfinite(trackedHarmony.samples[i])
                 && std::abs(trackedHarmony.samples[i]) <= 1.001f,
                 "Harmonic note tracking caused clip or NaN");
        }
        need(difference > 0.35,
             "HARMONIC LIFE's note-following mode has no measurable audio effect");

        // A fully dry setting must be transparent even with pitch tracking on.
        biosphere::BiosphereDSP engine;
        engine.prepare(48000, 256, 1);
        auto transparent = input;
        biosphere::Parameters dry;
        dry.mix = 0.0f;
        dry.harmonicMono = true;
        dry.inputGain = dry.outputGain = 1.0f;
        for (std::size_t pos = 0; pos < transparent.size(); pos += 256)
        {
            float* ptr[2] {transparent.data() + pos, nullptr};
            engine.process(ptr, 1, static_cast<int>(
                std::min<std::size_t>(256, transparent.size() - pos)), dry);
        }
        double deviation = 0.0;
        for (std::size_t i = 0; i < input.size(); ++i)
            deviation += std::abs(input[i] - transparent[i]);
        need(deviation < 0.001, "Pitch tracking changed the dry signal");

        std::cout << "GUITAR INTELLIGENCE PASS: E2/G3/E4/A4 across "
                     "44.1/48/96k, note changes, silence, tempered harmonies\n";
    }
    catch (const std::exception& err)
    {
        std::cerr << "GUITAR INTELLIGENCE FAIL: " << err.what() << '\n';
        return 1;
    }
    return 0;
}

#pragma once
// Factory catalog is JUCE-free so all 100 patches can undergo real DSP tests.
// All 20 original preset names/values remain in their original order.
#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace biosphere::factory
{
struct Patch
{
    std::string name, category;
    std::vector<std::pair<std::string, float>> values;
};
inline std::vector<Patch> build()
{
    std::vector<Patch> patches;
    patches.reserve(100);
    auto append = [&patches](const char* name, const char* category,
                             std::initializer_list<std::pair<const char*, float>> entries)
    {
        Patch patch;
        patch.name = name;
        patch.category = category;
        for (const auto& [key, value] : entries)
            patch.values.emplace_back(key, value);
        patches.push_back(std::move(patch));
    };
    append("01 - Pure Guitar", "ORGANIC CLEAN",
               {{"mix", 0.08f}, {"pulse", 0.0f}, {"harmonic", 0.08f},
                {"mutation", 0.0f}, {"memory", 0.0f}});
    append("02 - Living Fingers", "ORGANIC CLEAN",
               {{"mix", 0.30f}, {"harmonic", 0.20f}, {"harm.decay", 1.0f},
                {"pulse", 0.12f}, {"macro.energy", 0.18f}});
    append("03 - Quiet Halo", "LIVING AMBIENCE",
               {{"mix", 0.62f}, {"harmonic", 0.44f}, {"harm.decay", 4.7f},
                {"mutation", 0.18f}, {"macro.space", 0.85f}});
    append("04 - Midnight Air", "LIVING AMBIENCE",
               {{"mix", 0.69f}, {"harmonic", 0.35f}, {"mutation", 0.28f},
                {"mut.character", 3.0f}, {"mut.density", 18.0f}});
    append("05 - Soft Tremor", "RHYTHMIC PULSE",
               {{"mix", 0.65f}, {"pulse", 0.70f}, {"rate", 3.0f},
                {"pulse.swing", 0.10f}, {"pulse.mix", 0.85f}});
    append("06 - Swung Sparks", "RHYTHMIC PULSE",
               {{"mix", 0.73f}, {"pulse", 0.88f}, {"rate", 4.2f},
                {"pulse.swing", 0.72f}, {"pulse.feedback", 0.45f}});
    append("07 - Fifth Bloom", "HARMONIC BLOOM",
               {{"mix", 0.75f}, {"pulse", 0.0f}, {"harmonic", 0.82f},
                {"harm.mode", 0.0f}, {"harm.decay", 3.2f}});
    append("08 - Golden Octaves", "HARMONIC BLOOM",
               {{"mix", 0.66f}, {"harmonic", 0.85f}, {"harm.mode", 1.0f},
                {"root", 147.0f}, {"macro.growth", 0.83f}});
    append("09 - Crystal Seeds", "GRANULAR MUTATIONS",
               {{"mix", 0.82f}, {"mutation", 0.86f}, {"grain", 65.0f},
                {"mut.character", 1.0f}, {"mut.density", 25.0f}});
    append("10 - Moving Metal", "GRANULAR MUTATIONS",
               {{"mix", 0.86f}, {"mutation", 0.82f}, {"mut.character", 2.0f},
                {"mut.pitch", -5.0f}, {"mut.motion", 0.75f}});
    append("11 - Afterimage", "MEMORY GHOSTS",
               {{"mix", 0.82f}, {"memory", 0.82f}, {"mem.length", 0.6f},
                {"mem.recall", 0.91f}, {"feedback", 0.58f}});
    append("12 - Reversed Shadows", "MEMORY GHOSTS",
               {{"mix", 0.88f}, {"memory", 0.85f}, {"mem.reverse", 0.95f},
                {"mem.fade", 120.0f}, {"mem.length", 1.2f}});
    append("13 - Rising Lead", "REACTIVE LEADS",
               {{"mix", 0.56f}, {"harmonic", 0.64f}, {"harm.mode", 2.0f},
                {"macro.energy", 0.80f}, {"pulse.sensitivity", 0.82f}});
    append("14 - Touch & Motion", "REACTIVE LEADS",
               {{"mix", 0.60f}, {"pulse", 0.47f}, {"mutation", 0.31f},
                {"macro.motion", 0.82f}, {"pulse.mix", 0.60f}});
    append("15 - Wide Orbit", "CINEMATIC WORLDS",
               {{"mix", 0.87f}, {"harmonic", 0.60f}, {"mutation", 0.45f},
                {"global.width", 1.0f}, {"macro.space", 1.0f}});
    append("16 - Distant Lights", "CINEMATIC WORLDS",
               {{"mix", 0.90f}, {"harmonic", 0.52f}, {"memory", 0.39f},
                {"harm.decay", 5.2f}, {"mem.fade", 160.0f}});
    append("17 - Fractured Strings", "EXPERIMENTAL LIFE",
               {{"mix", 0.94f}, {"mutation", 0.94f}, {"mut.random", 0.88f},
                {"mut.pitch", 7.0f}, {"mut.density", 33.0f}});
    append("18 - Broken Time", "EXPERIMENTAL LIFE",
               {{"mix", 0.93f}, {"pulse", 0.76f}, {"memory", 0.72f},
                {"rate", 7.0f}, {"mem.reverse", 0.69f}});
    append("19 - Growing Ecosystem", "PERFORMANCE ECOSYSTEMS",
               {{"mix", 0.82f}, {"harmonic", 0.60f}, {"mutation", 0.48f},
                {"macro.growth", 0.94f}, {"macro.motion", 0.74f}});
    append("20 - The Living Guitar", "PERFORMANCE ECOSYSTEMS",
               {{"mix", 0.84f}, {"pulse", 0.72f}, {"harmonic", 0.78f},
                {"mutation", 0.39f}, {"memory", 0.74f},
                {"mem.length", 0.58f}, {"mem.recall", 0.53f},
                {"macro.energy", 0.70f}, {"macro.space", 0.78f},
                {"react.active", 1.0f}, {"react.reactivity", 0.88f},
                {"react.interaction", 0.90f}, {"react.sensitivity", 0.90f},
                {"react.response", 45.0f}, {"react.complexity", 0.32f},
                {"react.route1.active", 1.0f}, {"react.route1.source", 0.0f},
                {"react.route1.destination", 1.0f}, {"react.route1.amount", 0.98f},
                {"react.route2.active", 1.0f}, {"react.route2.source", 2.0f},
                {"react.route2.destination", 2.0f}, {"react.route2.amount", 0.88f},
                {"react.route3.active", 1.0f}, {"react.route3.source", 3.0f},
                {"react.route3.destination", 3.0f}, {"react.route3.amount", 0.88f},
                {"react.route4.active", 0.0f}, {"react.route5.active", 0.0f},
                {"react.route6.active", 0.0f}, {"react.route7.active", 0.0f},
                {"react.route8.active", 0.0f}});


    // 80 additional purpose-designed recipes: different balances, pitch
    // characters, macro dynamics, rate patterns and evolving interactions.
    // These are algorithmically verified starter designs, NOT an assertion of
    // manual listening or final loudness matching.
    struct Design
    {
        const char* name;
        int family;
        float mix, pulse, harmonic, mutation, memory, space, motion, energy;
        int variation;
    };
    static constexpr std::array<Design, 80> designs {{
        { "21 - Glass Fingerstyle", 0, 0.325f, 0.090f, 0.130f, 0.021f, 0.016f, 0.210f, 0.110f, 0.200f, 0 },
        { "22 - Warm Mahogany", 0, 0.297f, 0.105f, 0.158f, 0.023f, 0.017f, 0.450f, 0.270f, 0.360f, 1 },
        { "23 - First Light", 0, 0.276f, 0.086f, 0.184f, 0.026f, 0.019f, 0.740f, 0.440f, 0.510f, 2 },
        { "24 - Pick Whisper", 0, 0.309f, 0.076f, 0.211f, 0.030f, 0.021f, 0.900f, 0.690f, 0.680f, 3 },
        { "25 - Velvet Notes", 0, 0.322f, 0.098f, 0.144f, 0.032f, 0.016f, 0.330f, 0.880f, 0.300f, 4 },
        { "26 - Clean Arpeggios", 0, 0.285f, 0.102f, 0.202f, 0.027f, 0.020f, 0.830f, 0.540f, 0.840f, 5 },
        { "27 - Night Strings", 0, 0.282f, 0.079f, 0.169f, 0.031f, 0.019f, 0.640f, 0.750f, 0.610f, 6 },
        { "28 - Amber Chords", 0, 0.320f, 0.081f, 0.221f, 0.034f, 0.021f, 1.000f, 0.970f, 0.910f, 7 },
        { "29 - Mist Horizon", 1, 0.685f, 0.114f, 0.432f, 0.155f, 0.171f, 0.210f, 0.110f, 0.200f, 0 },
        { "30 - Lunar Drift", 1, 0.657f, 0.107f, 0.528f, 0.172f, 0.191f, 0.450f, 0.270f, 0.360f, 1 },
        { "31 - Blue Aurora", 1, 0.636f, 0.084f, 0.612f, 0.190f, 0.214f, 0.740f, 0.440f, 0.510f, 2 },
        { "32 - Floating Tides", 1, 0.669f, 0.097f, 0.702f, 0.217f, 0.227f, 0.900f, 0.690f, 0.680f, 3 },
        { "33 - Distant Halo", 1, 0.682f, 0.117f, 0.480f, 0.238f, 0.181f, 0.330f, 0.880f, 0.300f, 4 },
        { "34 - Velvet Nebula", 1, 0.645f, 0.099f, 0.672f, 0.201f, 0.222f, 0.830f, 0.540f, 0.840f, 5 },
        { "35 - Silent Constellation", 1, 0.642f, 0.083f, 0.564f, 0.224f, 0.206f, 0.640f, 0.750f, 0.610f, 6 },
        { "36 - Reverie Field", 1, 0.680f, 0.106f, 0.738f, 0.248f, 0.235f, 1.000f, 0.970f, 0.910f, 7 },
        { "37 - Tremolo Heart", 2, 0.705f, 0.901f, 0.158f, 0.084f, 0.093f, 0.210f, 0.110f, 0.200f, 0 },
        { "38 - Syncopated Air", 2, 0.677f, 0.710f, 0.194f, 0.094f, 0.104f, 0.450f, 0.270f, 0.360f, 1 },
        { "39 - Shifting Eighths", 2, 0.656f, 0.678f, 0.224f, 0.104f, 0.117f, 0.740f, 0.440f, 0.510f, 2 },
        { "40 - Pulse Caravan", 2, 0.689f, 0.877f, 0.257f, 0.119f, 0.124f, 0.900f, 0.690f, 0.680f, 3 },
        { "41 - Elastic Triplets", 2, 0.702f, 0.858f, 0.176f, 0.130f, 0.099f, 0.330f, 0.880f, 0.300f, 4 },
        { "42 - Midnight Motor", 2, 0.665f, 0.663f, 0.246f, 0.110f, 0.121f, 0.830f, 0.540f, 0.840f, 5 },
        { "43 - Broken Clocks", 2, 0.662f, 0.732f, 0.207f, 0.122f, 0.112f, 0.640f, 0.750f, 0.610f, 6 },
        { "44 - Kinetic Picking", 2, 0.700f, 0.909f, 0.271f, 0.135f, 0.128f, 1.000f, 0.970f, 0.910f, 7 },
        { "45 - Perfect Fifth Garden", 3, 0.745f, 0.092f, 0.605f, 0.084f, 0.093f, 0.210f, 0.110f, 0.200f, 0 },
        { "46 - Harmonic Lanterns", 3, 0.717f, 0.075f, 0.739f, 0.094f, 0.104f, 0.450f, 0.270f, 0.360f, 1 },
        { "47 - Soft Thirds", 3, 0.696f, 0.092f, 0.857f, 0.104f, 0.117f, 0.740f, 0.440f, 0.510f, 2 },
        { "48 - Orbiting Octaves", 3, 0.729f, 0.105f, 0.983f, 0.119f, 0.124f, 0.900f, 0.690f, 0.680f, 3 },
        { "49 - Suspended Dawn", 3, 0.742f, 0.084f, 0.672f, 0.130f, 0.099f, 0.330f, 0.880f, 0.300f, 4 },
        { "50 - Crystal Resonance", 3, 0.705f, 0.077f, 0.941f, 0.110f, 0.121f, 0.830f, 0.540f, 0.840f, 5 },
        { "51 - Singing Overtones", 3, 0.702f, 0.099f, 0.790f, 0.122f, 0.112f, 0.640f, 0.750f, 0.610f, 6 },
        { "52 - Choir of Strings", 3, 0.740f, 0.101f, 0.990f, 0.135f, 0.128f, 1.000f, 0.970f, 0.910f, 7 },
        { "53 - Glass Fragments", 4, 0.805f, 0.078f, 0.202f, 0.584f, 0.078f, 0.210f, 0.110f, 0.200f, 0 },
        { "54 - Grain River", 4, 0.777f, 0.082f, 0.246f, 0.649f, 0.087f, 0.450f, 0.270f, 0.360f, 1 },
        { "55 - Silver Dust", 4, 0.756f, 0.104f, 0.286f, 0.718f, 0.097f, 0.740f, 0.440f, 0.510f, 2 },
        { "56 - Granular Breeze", 4, 0.789f, 0.095f, 0.328f, 0.820f, 0.103f, 0.900f, 0.690f, 0.680f, 3 },
        { "57 - Falling Particles", 4, 0.802f, 0.075f, 0.224f, 0.897f, 0.082f, 0.330f, 0.880f, 0.300f, 4 },
        { "58 - Pitch Lantern", 4, 0.765f, 0.089f, 0.314f, 0.759f, 0.101f, 0.830f, 0.540f, 0.840f, 5 },
        { "59 - Metal Mirage", 4, 0.762f, 0.105f, 0.263f, 0.845f, 0.094f, 0.640f, 0.750f, 0.610f, 6 },
        { "60 - Shimmering Shards", 4, 0.800f, 0.087f, 0.344f, 0.934f, 0.107f, 1.000f, 0.970f, 0.910f, 7 },
        { "61 - Ghost of Yesterday", 5, 0.815f, 0.067f, 0.173f, 0.127f, 0.661f, 0.210f, 0.110f, 0.200f, 0 },
        { "62 - Reverse Footsteps", 5, 0.787f, 0.086f, 0.211f, 0.141f, 0.737f, 0.450f, 0.270f, 0.360f, 1 },
        { "63 - Captured Rain", 5, 0.766f, 0.092f, 0.245f, 0.156f, 0.828f, 0.740f, 0.440f, 0.510f, 2 },
        { "64 - Last Chord", 5, 0.799f, 0.071f, 0.281f, 0.178f, 0.878f, 0.900f, 0.690f, 0.680f, 3 },
        { "65 - Echoes Within", 5, 0.812f, 0.071f, 0.192f, 0.195f, 0.699f, 0.330f, 0.880f, 0.300f, 4 },
        { "66 - Memory Lantern", 5, 0.775f, 0.091f, 0.269f, 0.165f, 0.856f, 0.830f, 0.540f, 0.840f, 5 },
        { "67 - Forgotten Arpeggio", 5, 0.772f, 0.087f, 0.226f, 0.183f, 0.796f, 0.640f, 0.750f, 0.610f, 6 },
        { "68 - Fading Cassette", 5, 0.810f, 0.067f, 0.295f, 0.203f, 0.909f, 1.000f, 0.970f, 0.910f, 7 },
        { "69 - Edge of Light", 6, 0.675f, 0.371f, 0.482f, 0.063f, 0.132f, 0.210f, 0.110f, 0.200f, 0 },
        { "70 - Dynamic Silk", 6, 0.647f, 0.456f, 0.590f, 0.070f, 0.147f, 0.450f, 0.270f, 0.360f, 1 },
        { "71 - Touch Response", 6, 0.626f, 0.392f, 0.683f, 0.078f, 0.166f, 0.740f, 0.440f, 0.510f, 2 },
        { "72 - Lead Guardian", 6, 0.659f, 0.324f, 0.784f, 0.089f, 0.176f, 0.900f, 0.690f, 0.680f, 3 },
        { "73 - Elastic Melody", 6, 0.672f, 0.405f, 0.536f, 0.097f, 0.140f, 0.330f, 0.880f, 0.300f, 4 },
        { "74 - Living Vibrato", 6, 0.635f, 0.452f, 0.750f, 0.082f, 0.171f, 0.830f, 0.540f, 0.840f, 5 },
        { "75 - Singing Wire", 6, 0.632f, 0.359f, 0.630f, 0.092f, 0.159f, 0.640f, 0.750f, 0.610f, 6 },
        { "76 - Neon Solo", 6, 0.670f, 0.336f, 0.824f, 0.101f, 0.182f, 1.000f, 0.970f, 0.910f, 7 },
        { "77 - Deep Blue Canvas", 7, 0.815f, 0.245f, 0.482f, 0.331f, 0.381f, 0.210f, 0.110f, 0.200f, 0 },
        { "78 - Wide Horizon", 7, 0.787f, 0.245f, 0.590f, 0.368f, 0.425f, 0.450f, 0.270f, 0.360f, 1 },
        { "79 - Distant City", 7, 0.766f, 0.189f, 0.683f, 0.407f, 0.477f, 0.740f, 0.440f, 0.510f, 2 },
        { "80 - Slow Orbit", 7, 0.799f, 0.203f, 0.784f, 0.464f, 0.506f, 0.900f, 0.690f, 0.680f, 3 },
        { "81 - Nocturnal Shore", 7, 0.812f, 0.255f, 0.536f, 0.508f, 0.403f, 0.330f, 0.880f, 0.300f, 4 },
        { "82 - Lost Satellites", 7, 0.775f, 0.228f, 0.750f, 0.430f, 0.493f, 0.830f, 0.540f, 0.840f, 5 },
        { "83 - Cinematic Bloom", 7, 0.772f, 0.183f, 0.630f, 0.478f, 0.459f, 0.640f, 0.750f, 0.610f, 6 },
        { "84 - Endless Valley", 7, 0.810f, 0.222f, 0.824f, 0.529f, 0.524f, 1.000f, 0.970f, 0.910f, 7 },
        { "85 - Fractal Strings", 8, 0.855f, 0.409f, 0.353f, 0.563f, 0.443f, 0.210f, 0.110f, 0.200f, 0 },
        { "86 - Alien Resonance", 8, 0.827f, 0.334f, 0.431f, 0.626f, 0.494f, 0.450f, 0.270f, 0.360f, 1 },
        { "87 - Time Fracture", 8, 0.806f, 0.295f, 0.500f, 0.692f, 0.555f, 0.740f, 0.440f, 0.510f, 2 },
        { "88 - Spectral Drift", 8, 0.839f, 0.380f, 0.573f, 0.790f, 0.589f, 0.900f, 0.690f, 0.680f, 3 },
        { "89 - Bending Reality", 8, 0.852f, 0.397f, 0.392f, 0.865f, 0.469f, 0.330f, 0.880f, 0.300f, 4 },
        { "90 - Broken Signals", 8, 0.815f, 0.308f, 0.549f, 0.732f, 0.574f, 0.830f, 0.540f, 0.840f, 5 },
        { "91 - Hidden Organism", 8, 0.812f, 0.314f, 0.461f, 0.814f, 0.534f, 0.640f, 0.750f, 0.610f, 6 },
        { "92 - Metal Bloom", 8, 0.850f, 0.402f, 0.603f, 0.900f, 0.610f, 1.000f, 0.970f, 0.910f, 7 },
        { "93 - Living Network", 9, 0.825f, 0.663f, 0.482f, 0.359f, 0.490f, 0.210f, 0.110f, 0.200f, 0 },
        { "94 - Evolving Sanctuary", 9, 0.797f, 0.519f, 0.590f, 0.399f, 0.546f, 0.450f, 0.270f, 0.360f, 1 },
        { "95 - Reactive Forest", 9, 0.776f, 0.603f, 0.683f, 0.441f, 0.613f, 0.740f, 0.440f, 0.510f, 2 },
        { "96 - Wild Biosphere", 9, 0.809f, 0.725f, 0.784f, 0.504f, 0.651f, 0.900f, 0.690f, 0.680f, 3 },
        { "97 - Growth and Memory", 9, 0.822f, 0.610f, 0.536f, 0.551f, 0.518f, 0.330f, 0.880f, 0.300f, 4 },
        { "98 - Storm and Calm", 9, 0.785f, 0.517f, 0.750f, 0.466f, 0.634f, 0.830f, 0.540f, 0.840f, 5 },
        { "99 - Intelligent Guitar", 9, 0.782f, 0.656f, 0.630f, 0.519f, 0.590f, 0.640f, 0.750f, 0.610f, 6 },
        { "100 - Biosphere Finale", 9, 0.820f, 0.714f, 0.824f, 0.574f, 0.674f, 1.000f, 0.970f, 0.910f, 7 },
    }};
    static constexpr std::array<const char*, 10> categories {{
        "ORGANIC CLEAN", "LIVING AMBIENCE", "RHYTHMIC PULSE",
        "HARMONIC BLOOM", "GRANULAR MUTATIONS", "MEMORY GHOSTS",
        "REACTIVE LEADS", "CINEMATIC WORLDS", "EXPERIMENTAL LIFE",
        "PERFORMANCE ECOSYSTEMS"
    }};
    const auto limit = [](float value, float lo, float hi)
    {
        return std::clamp(value, lo, hi);
    };

    for (const auto& d : designs)
    {
        const float v = static_cast<float>(d.variation);
        Patch patch;
        patch.name = d.name;
        patch.category = categories[static_cast<std::size_t>(d.family)];
        auto put = [&patch](const char* id, float value)
        {
            patch.values.emplace_back(id, value);
        };
        put("mix", d.mix);
        put("pulse", d.pulse);
        put("harmonic", d.harmonic);
        put("mutation", d.mutation);
        put("memory", d.memory);
        put("macro.growth", limit(0.21f + 0.09f * v, 0.0f, 1.0f));
        put("macro.motion", d.motion);
        put("macro.space", d.space);
        put("macro.energy", d.energy);
        put("global.intensity", limit(0.31f + d.energy * 0.38f, 0.0f, 1.0f));
        put("global.width", d.space);
        put("rate", limit(0.75f + v * 0.93f + d.family * 0.15f, 0.1f, 12.0f));
        put("root", 82.0f + 16.0f * static_cast<float>((d.variation + d.family) % 10));
        put("harm.mode", static_cast<float>((d.variation + d.family) % 4));
        put("harm.mono", (d.family == 3 || d.family == 6 || d.family == 9) ? 1.0f : 0.0f);
        put("harm.resonance", limit(0.22f + d.harmonic * 0.59f, 0.0f, 1.0f));
        put("harm.decay", limit(0.7f + d.harmonic * 4.8f + 0.13f * v, 0.1f, 6.0f));
        put("harm.motion", limit(0.08f + d.motion * 0.68f, 0.0f, 1.0f));
        put("harm.spread", d.space);
        put("harm.damping", limit(0.80f - d.energy * 0.45f, 0.0f, 1.0f));
        put("pulse.sensitivity", limit(0.24f + d.energy * 0.65f, 0.0f, 1.0f));
        put("pulse.division", static_cast<float>(d.variation % 5));
        put("pulse.swing", limit(d.motion * 0.53f, 0.0f, 1.0f));
        put("pulse.feedback", limit(0.06f + d.pulse * 0.55f, 0.0f, 1.0f));
        put("pulse.width", d.space);
        put("pulse.mix", limit(0.38f + d.energy * 0.50f, 0.0f, 1.0f));
        put("grain", limit(42.0f + 18.0f * v + d.family * 2.4f, 35.0f, 220.0f));
        put("mut.density", limit(6.0f + 3.7f * v + d.mutation * 11.0f, 1.0f, 40.0f));
        put("mut.character", static_cast<float>((d.family + d.variation) % 4));
        put("mut.motion", d.motion);
        put("mut.pitch", static_cast<float>( ((d.variation + 2 * d.family) % 7) * 3 - 9));
        put("mut.random", limit(0.12f + 0.70f * d.motion, 0.0f, 1.0f));
        put("mut.stereo", d.space);
        put("mem.length", limit(0.55f + 0.27f * v + 0.2f * d.memory, 0.5f, 3.0f));
        put("mem.capture", limit(0.46f + d.energy * 0.50f, 0.0f, 1.0f));
        put("mem.recall", limit(0.24f + d.memory * 0.69f, 0.0f, 1.0f));
        put("mem.reverse", limit((d.family == 5 || d.family == 8 ? 0.12f : 0.0f)
                                    + (d.variation % 4) * 0.21f, 0.0f, 1.0f));
        put("mem.fade", limit(24.0f + 19.0f * v, 5.0f, 300.0f));
        put("mem.repeat", static_cast<float>((d.variation + d.family) % 4));
        put("feedback", limit(0.13f + d.memory * 0.65f, 0.0f, 0.95f));
        // Unlike early factory defaults, do not accidentally turn on every
        // route: a recipe enables only the interactions intentionally chosen.
        for (int route = 1; route <= 8; ++route)
            patch.values.emplace_back("react.route" + std::to_string(route) + ".active", 0.0f);
        const bool interactive = d.family == 6 || d.family == 8 || d.family == 9;
        put("react.active", interactive ? 1.0f : 0.0f);
        put("react.reactivity", d.energy);
        put("react.interaction", limit(0.30f + d.motion * 0.58f, 0.0f, 1.0f));
        put("react.sensitivity", limit(0.44f + d.energy * 0.42f, 0.0f, 1.0f));
        put("react.response", 50.0f + 30.0f * v);
        put("react.complexity", d.motion);
        if (interactive)
        {
            // Explicit musical cascade: transient -> PULSE, HARMONIC ->
            // MEMORY, with controlled bipolar contrast in experimental sound.
            put("react.route1.active", 1.0f);
            put("react.route1.source", 0.0f);
            put("react.route1.destination", 1.0f);
            put("react.route1.amount", limit(0.40f + d.energy * 0.50f, 0.0f, 1.0f));
            put("react.route2.active", 1.0f);
            put("react.route2.source", 3.0f);
            put("react.route2.destination", 3.0f);
            put("react.route2.amount", d.family == 8 ? -0.34f : 0.48f);
            if (d.family == 9)
            {
                put("react.route3.active", 1.0f);
                put("react.route3.source", 6.0f);
                put("react.route3.destination", 0.0f);
                put("react.route3.amount", 0.35f + 0.06f * v);
            }
        }
        const bool evolving = d.family == 1 || d.family == 7 || d.family == 9;
        put("evo.active", evolving ? 1.0f : 0.0f);
        put("evo.rate", 0.45f + 0.35f * v);
        put("evo.intensity", limit(0.32f + d.energy * 0.58f, 0.0f, 1.0f));
        put("evo.sensitivity", limit(0.36f + d.energy * 0.48f, 0.0f, 1.0f));
        put("evo.stability", limit(0.42f + 0.06f * v, 0.0f, 1.0f));
        put("evo.complexity", d.motion);
        put("evo.recovery", 1.1f + 0.85f * v);
        patches.push_back(std::move(patch));
    }
    return patches;
}
} // namespace biosphere::factory

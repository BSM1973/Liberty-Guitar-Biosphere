# GUITAR BIOSPHERE
### The Living Guitar Effects Engine — Liberty Music Software

Native C++20 / JUCE 8 guitar audio-effect plugin for macOS (AU / VST3) and Windows (VST3).

## Development status — Phase 1

This is an **early functional DSP foundation**, not a finished commercial product.

Implemented:
- **PULSE**: picking-responsive rhythmic amplitude modulation.
- **HARMONIC LIFE**: three tuned, damped harmonic resonators.
- **MUTATION**: live dual-grain overlap processing with a circular audio buffer.
- **MEMORY**: transient-triggered ghost fragments, repeated with fading feedback.
- Mono and stereo processing, bounded wet mix and output protection.
- 11 automatable parameters with persistent host session state.
- New fixed-size midnight-blue/gold JUCE interface based on the approved screenshots.
- Six independent engine panel classes (PulsePanel, HarmonicPanel, MutationPanel, ReactionPanel, MemoryPanel, EvolutionPanel).
- Clicking the six ecosystem nodes switches the central engine panel.
- Existing eleven automatable DSP parameters remain reachable from appropriate knobs; controls without a DSP parameter are intentionally disabled.
- REACTION routes and shared analysis are currently **UI placeholders**, marked as such.
- Standalone, JUCE-free DSP smoke tests, and cross-platform GitHub Actions builds.

Not yet implemented:
- REACTION matrix and EVOLUTION ecosystem controller.
- 100 factory presets, preset browser and user preset storage.
- Active French/English localization switch (FR displayed; EN currently disabled).
- Production-level audio validation and signed/notarized installers.

No feature listed above as pending is represented as complete in the UI.

## Interface — phase 1

The **old editor has been fully replaced** (not layered under a static screenshot).
The approved midnight/gold visual language is rendered by JUCE components and vector graphics.

Layout: top header, ecosystem node navigation, analysis area, one of six parameter panels,
four macros and master bus. No scrollbars and no editor resizing (1200 × 748).

This is an **interface-only milestone**. The full APVTS parameter mapping, real-time analysis,
browser and 100 factory presets are separate tasks. Disabled controls are deliberate:
they must never pretend to affect audio.

## Building

The project retrieves **JUCE 8.0.10** using CMake FetchContent during configuration. Requires CMake >= 3.22, a C++20 compiler and internet access for the initial JUCE fetch.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

On macOS, configure with `-DCMAKE_OSX_ARCHITECTURES=arm64` for Apple Silicon binaries.

On Windows with Visual Studio 2022, use `cmake -S . -B build -A x64`.

### Fast DSP smoke test without JUCE

```bash
c++ -std=c++20 -O2 -ISource Tests/DspSmoke.cpp -o biosphere-test
./biosphere-test
```

Build results are published under **Actions**. Successful workflows upload **unsigned** AU/VST3 bundles to their run artifacts. These are development binaries; commercial distribution will require validation, signing and notarization where applicable.

## Audio architecture

```text
Guitar In -> Shared envelope/onset analysis
          +-> PULSE (rhythmic tremolo)
          +-> HARMONIC LIFE (tuned resonators)
          +-> MUTATION (live granular delay)
          +-> MEMORY (transient-triggered ghost slices)
          -> Wet mixer + Dry / Wet -> Output protection -> Guitar Out

REACTION and EVOLUTION: planned modulation/control layers, not yet connected.
```

Source: `Source/Dsp/BiosphereDSP.h` is independent of JUCE, enabling deterministic tests of the actual sample processing algorithm.

## Next milestones

1. Stabilize macOS ARM64 AU/VST3 and Windows VST3 builds.
2. Add configurable inter-engine REACTION modulation routing.
3. Add deterministic four-state EVOLUTION controller.
4. Implement factory/user preset browser and 100 individually designed presets.
5. Extend FR/EN localization, QA, performance optimization and deployment.

© Liberty Music Software. All rights reserved. No redistribution license is granted by this repository.

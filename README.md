# GUITAR BIOSPHERE
### The Living Guitar Effects Engine — Liberty Music Software

Native C++20 / JUCE 8 guitar audio-effect plugin for macOS (AU / VST3) and Windows (VST3).

## Development status — Phase 2

This is an **early functional DSP foundation**, not a finished commercial product.

Implemented:
- **PULSE**: picking-responsive rhythmic amplitude modulation.
- **HARMONIC LIFE**: three tuned, damped harmonic resonators.
- **MUTATION**: live dual-grain overlap processing with a circular audio buffer.
- **MEMORY**: transient-triggered ghost fragments, repeated with fading feedback.
- Mono and stereo processing, bounded wet mix and output protection.
- **97 stable APVTS parameters**, including the 11 original host IDs unchanged for existing sessions.
- Six panels have working host-automatable Slider / ComboBox / Button attachments.
- **8 REACTION routes** expose on/off, source, destination and bipolar amount (32 APVTS parameters).
- All 4 macros, 6 master knobs, engine enable switches and 3 global switches are attached.
- Global bypass is wired to a real dry-through path; setting/restore via APVTS is implemented.
- GitHub CI audits parameter IDs and UI attachment coverage.
- New fixed-size midnight-blue/gold JUCE interface based on the approved screenshots.
- Six independent engine panel classes (PulsePanel, HarmonicPanel, MutationPanel, ReactionPanel, MemoryPanel, EvolutionPanel).
- Clicking the six ecosystem nodes switches the central engine panel.
- Existing DSP controls stay connected using their original parameter IDs. New control values are host-automatable and saved, but most do not alter audio until the subsequent DSP integration.
- REACTION route controls are now fully editable and saved as parameters, although the modulation engine is pending. Shared analysis is still a UI placeholder.
- Standalone, JUCE-free DSP smoke tests, and cross-platform GitHub Actions builds.

Not yet implemented:
- REACTION matrix and EVOLUTION ecosystem controller.
- 100 factory presets, preset browser and user preset storage.
- Active French/English localization switch (FR displayed; EN currently disabled).
- Production-level audio validation and signed/notarized installers.

No feature listed above as pending is represented as complete in the UI.

## UI/APVTS — phase 2

The **old editor has been fully replaced** (not layered under a static screenshot).
The approved midnight/gold visual language is rendered by JUCE components and vector graphics.

Layout: top header, ecosystem node navigation, analysis area, one of six parameter panels,
four macros and master bus. No scrollbars and no editor resizing (1200 × 748).

This is an **APVTS/automation milestone**. All audio-control knobs, combo menus,
engine enable buttons, macros, master switches and eight route editors have stable parameter IDs
and DAW state recall. The actual modulation behaviors of REACTION, EVOLUTION and new advanced
settings still require DSP development. The preset browser navigation and FR/EN localization
remain disabled: those are application workflows, not host audio parameters.

The 11 legacy IDs are retained unchanged (input, output, mix, pulse, harmonic, mutation,
memory, rate, root, grain, feedback) so old DAW project state can still recall them.

**Phase 2 verification:** the Linux CI step runs Tests/verify_parameter_bindings.py,
checking 97 unique IDs, 46 panel controls, 10 master knobs and all eight four-parameter routes.
Cross-platform build jobs verify the JUCE attachment code compiles.

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

# GUITAR BIOSPHERE
### The Living Guitar Effects Engine — Liberty Music Software

Native C++20 / JUCE 8 guitar audio-effect plugin for macOS (AU / VST3) and Windows (VST3).

## Development status — Phase 10

This is an **early functional DSP foundation**, not a finished commercial product.

Implemented:
- **PULSE PRO**: attack-aware rhythmic amplitude and delay, 45 ms tempo-retiming crossfade, damped soft-saturated feedback.
- **HARMONIC LIFE PRO**: three individually glide-tuned resonators, controlled excitation and additional high-frequency damping.
- **MUTATION PRO**: up to 24 overlapping grains per channel, fixed precomputed Hann windows, bounded cubic playback interpolation and smooth overlap normalization.
- **MEMORY PRO**: repeating fragments with 22 ms old/new capture crossfade, smoothstep loop edges and smoothed reverse/feedback automation.
- Mono and stereo processing, bounded wet mix and output protection; four engine on/off controls have 23 ms non-clicking gain ramps.
- **97 stable APVTS parameters**, including the 11 original host IDs unchanged for existing sessions.
- Six panels have working host-automatable Slider / ComboBox / Button attachments.
- **8 REACTION routes** expose on/off, source, destination and bipolar amount (32 APVTS parameters).
- All 4 macros, 6 master knobs, engine enable switches and 3 global switches are attached.
- Global bypass is wired to a real dry-through path; setting/restore via APVTS is implemented.
- GitHub CI audits parameter IDs and UI attachment coverage.
- New fixed-size midnight-blue/gold JUCE interface based on the approved screenshots.
- Six independent engine panel classes (PulsePanel, HarmonicPanel, MutationPanel, ReactionPanel, MemoryPanel, EvolutionPanel).
- Six full-width **native JUCE button hotspots** (icon and label), mouse/keyboard accessible, switch exclusively to the corresponding parameter panel.
- Last selected panel is serialized as UI-only session state and restored when reopening the plugin or DAW session.
- Selection changes do not reconstruct or reset APVTS controls; per-engine parameter values are retained.
- Independent navigation regression tests check all six node/panel mappings and invalid indices.
- The existing four native audio engines now read their APVTS controls in the processor and produce measurable parameter-dependent audio.
- PULSE: intensity, sensitivity, synced/free tempo, swing, feedback delay, stereo modulation, independent mix and enable.
- HARMONIC LIFE: mode, root, resonance, decay, damping, motion, spread and YIN-style monophonic musical note-following.
- MUTATION: grain size, density, character, pitch, motion, randomness and stereo variation.
- MEMORY: recall duration, capture sensitivity, repeat rate, reverse blend, feedback, fade, freeze, independent mix and enable.
- Four live performance macros (GROWTH, MOTION, SPACE, ENERGY) change multiple engine parameters without overwriting stored APVTS values.
- Master bus: smoothed input/output gain, dry/wet, intensity, speed, processed stereo width, host BPM sync, global memory freeze and smooth raw bypass.
- **EVOLUTION DSP is now connected**: deterministic CALME, CROISSANCE, TEMPÊTE and DÉCLIN states follow playing dynamics, with hysteresis and continuous modulation smoothing.
- **REACTION DSP is now connected**: 8 independently smoothed, host-automatable bipolar routes link real PULSE, HARMONIC LIFE, MUTATION and MEMORY audio activity to 8 modulation destinations.
- All 6 REACTION global controls (including on/off) and all 32 route controls are read from APVTS; no route-ID strings are allocated on the audio thread.
- Routes map to live grain density, PULSE depth, harmonic movement/resonance, MEMORY recall/feedback, grain movement and PULSE rate.
- A rising harmonic-to-MEMORY recall route can retrigger a recorded fragment, respecting the memory cooldown and freeze switches.
- Factory patch 20 retains its attack → PULSE → harmonic → MEMORY cascade; all **100** factory sounds are available in ten 10-preset families.
- **Shared analysis is live:** input/output peaks, envelope, attack activity, pulse/rhythm activity, MEMORY playback, measured harmonic output activity and a YIN-style monophonic note and pitch-confidence estimate.
- Audio publishes analysis into atomic values, and the JUCE UI polls at 30 Hz (no UI work in the audio callback).
- **Functional preset browser:** category/search/favorites, 100 factory patches, previous/next, user XML save/load, two-step delete confirmation, and user-specific favorite persistence.
- **Working FR/EN choice**, default FR, applied to all engine panels, shared analysis, macros, master bus, menus, routes and browser; language and last page are stored with the DAW session.
- Standalone, JUCE-free DSP smoke tests, and cross-platform GitHub Actions builds.

Not yet implemented:
- Dedicated DAW listening/visual QA of EVOLUTION's state thresholds and release tails.
- Independent listening and loudness matching of all 100 presets on real guitars: the catalog and automated DSP rendering exist, but musical mastering is not yet signed off.
- Final listening QA, nuanced translation QA and any remaining design refinements after in-DAW testing.
- Production-level audio validation and signed/notarized installers.

No feature listed above as pending is represented as complete in the UI.

## PRESET LAB — phase 10

**100 functional factory patches across 10 musical categories.**
The original presets 01–20 retain their names, order and parameter recipes.
The 80 additional sounds deliberately distribute PULSE, HARMONIC LIFE,
MUTATION, MEMORY, macros and selected REACTION/EVOLUTION interactions across
these ten families:

1. ORGANIC CLEAN — clean articulation, fingerstyle and subtle harmonics
2. LIVING AMBIENCE — evolving sustained textures
3. RHYTHMIC PULSE — tempos, swing, tremolo and feedback gestures
4. HARMONIC BLOOM — tracked musical intervals and resonators
5. GRANULAR MUTATIONS — granular texture, pitch and density
6. MEMORY GHOSTS — captures, reverses and fading fragments
7. REACTIVE LEADS — attack-responsive solo sounds
8. CINEMATIC WORLDS — wide harmonics and slow evolving atmospheres
9. EXPERIMENTAL LIFE — unpredictable but bounded routing and textures
10. PERFORMANCE ECOSYSTEMS — combined sound ecosystems and evolving chains

**A/B comparison** uses two independently editable, full APVTS parameter
snapshots. Clicking A or B recalls that snapshot. A and B persist together
as UI-only PRESET_LAB data within the host project state; this is not an
automatable parameter and does not alter the existing 97 host IDs.

**Undo/Redo** capture all parameter changes and preset loads as bounded
full-state history. Knob gestures are grouped into approximately 200 ms
transactions; loading a factory or user preset records a separate step.
Undo/Redo also restores the active A/B slot and name. Undo history is in
the open editor; A/B comparison slots survive DAW project reloads. User
preset XML contains only the selected sound, not unrelated A/B snapshots.

The controls sit in the free header space above the preset combo box:
A, B, ANNULER/UNDO and REFAIRE/REDO, checked with a geometry audit.
No new scrollbars, missing glyphs, layout changes to the six engines or
new audio parameters.

**Automated coverage** renders all 100 presets with the existing DSP,
checks each family's count, unique settings, output finiteness, peak bounds
and RMS sanity, and tests Undo/Redo history and native APVTS A/B persistence.
These checks are engineering QA, not claims of hearing or manually LUFS
matching 100 guitar sounds. A listening and level-adjustment pass in a
DAW is still required before commercial release.

## SOUND ENGINES PRO — phase 9

These improvements are connected to the **existing** four engines. No new
APVTS IDs, preset serialization changes, panel dimensions or UI labels.
The original 97-ID preset/automation contract is preserved.

**PULSE PRO**
- During tempo changes, the original delay position crossfades with the new
  read head over **45 ms**, avoiding an immediate arbitrary sample jump.
- Feedback recirculation is gently low-pass filtered and soft-limited;
  the direct delay tap remains full-range for attack intelligibility.
- The existing swing, attack sensitivity, wet control and REACTION routing
  still affect the actual sound.

**HARMONIC LIFE PRO**
- Each of the three resonators now follows its target frequency with an
  individual **22 ms pitch glide**. GUITAR INTELLIGENCE keeps its 55 ms
  root glide, and both can operate together without discontinuous
  resonator coefficient jumps.
- Saturated input excitation and a controlled damping filter reduce sharp
  edges and excessive high-frequency buildup. Existing four musical modes
  and fixed-root compatibility remain available.

**MUTATION PRO**
- Voice pool increased from **2 to 24 grains per channel**. Free voices
  are selected first instead of repeatedly replacing one of only two
  sustaining grains; the worst-case 80 grains/s × 220 ms occupies
  approximately 18 simultaneous grains per channel.
- A **1025-sample raised-cosine (Hann) table** is prepared outside the
  audio callback, then interpolated during overlapping grain playback.
- **Bounded cubic Hermite interpolation** replaces simple linear playback
  interpolation; long/re-pitched grains sound less stepped. This is not a
  claim of fully alias-free pitch shifting.
- Smooth density-dependent normalization and mild saturation prevent
  high-overlap accumulation from becoming disproportionately loud.

**MEMORY PRO**
- A new captured ghost can overlap the previous ghost for **22 ms**:
  existing tails are not immediately cut at the re-trigger sample.
- Each fragment uses smoothstep in/out edges, minimizing slope changes at
  fragment boundaries. Reverse and feedback automation is smoothed over
  **14 ms**. Repeat and Freeze continue to work with the original controls.

**Click-resistant active switches**: all four engines now use **23 ms**
gain ramps on activation/deactivation without forcing the master dry signal
to stop. This complements the already smoothed bypass and master controls.

**Validation:** the dedicated C++ stereo stress test renders all four
engines at challenging modulation settings and checks finite output,
bounded peaks, click-safe transitions, recall activity and unchanged dry
audio; CI retains previous DSP, REACTION, EVOLUTION, YIN pitch, UTF-8 and
interface collision checks. CPU duration is reported as a measurement,
not a guarantee of DAW real-time performance. Actual guitar audition and
host/UI inspection remain required before a commercial release.

## GUITAR INTELLIGENCE — phase 8

The former simple zero-crossing guitar fundamental estimate has been replaced
with a **downsampled YIN-style cumulative mean normalized difference**
monophonic tracker. The tracker supports approximately E2-E5, estimates
confidence, rejects low-confidence/noisy segments, protects the immediate
attack transient, requires repeated evidence before accepting a large
pitch jump, smooths sustain/bends, and releases notes in silence. It uses
fixed-size preallocated arrays in the audio callback, with approximately
17 ms analysis hops at common sample rates and a longer observation window.

- The existing **Suivi mono / Mono tracking** APVTS switch controls musical
  HARMONIC LIFE tracking. No new APVTS IDs and no changed factory preset IDs.
- When **ON** and confidence is strong, the guitar's detected note controls
  the root of the existing three harmonic resonators. The existing four
  harmony menus use **12-tone equal-tempered** note relationships:
  fifths (0,+7,+12), octaves (0,+12,+24), thirds (0,+4,+7), suspended
  (0,+5,+7 semitones).
- When **OFF**, original fixed-root behavior and existing harmonic
  ratios remain unchanged. Loss of a reliable monophonic pitch causes a
  55 ms glide back to the original root parameter, not a sudden retune.
- The existing shared-analysis panel shows a detected note (for example
  G3), its measured Hz and a confidence percentage. An unstable or
  absent pitch shows a dash and a neutral status, not a fabricated note.
- The source is the pre-effect first guitar audio channel, not the wet
  output: reverb/delay/resonator feedback cannot recursively fool pitch
  tracking. Input level, high noise, chords or strong distortion can still
  reduce tracking accuracy; **this is not polyphonic transcription**.
- Existing REACTION and EVOLUTION modulation, master processing and presets
  stay intact. The window's geometry and colors remain unchanged.

Dedicated C++ regression checks pitch accuracy across 44.1, 48 and 96kHz,
note transitions, sustained notes, silence/recovery, harmonic audio effect,
and dry-input transparency. Visual inspection and audio comparison against
real guitars in multiple DAWs are still needed.

## EVOLUTION — phase 7

The existing six EVOLUTION controls already stored in APVTS are now read
by the processor (no host parameter renaming):
- **Cadence / Rate** (0.25–4x): how quickly the playing-state controller adapts
- **Intensité / Intensity**: how much EVOLUTION changes the sound
- **Sensibilité / Sensitivity**: when the guitar's energy becomes significant
- **Stabilité / Stability**: hysteresis and minimum dwell to prevent flashing states
- **Complexité / Complexity**: balance between granular and harmonic change
- **Récupération / Recovery** (0.25–10s): how slowly intensity fades after playing

Four actual states, measured from guitar input level and attack information:
1. **CALME / CALM:** quiet, existing effect settings unmodified
2. **CROISSANCE / GROWTH:** gentle increase of harmonic movement, pulse and texture
3. **TEMPÊTE / STORM:** more active granular movement/density and richer harmonics
4. **DÉCLIN / DECAY:** natural fade of additional modulation while existing
   MEMORY fragments and harmonic resonances continue without sudden cuts

Transitions are guarded by hysteresis and continuous low-pass smoothing.
Changing state does NOT switch entire effects on/off, override user APVTS
controls, reset buffers or apply abrupt gains to the master output.
REACTION source index 6 now uses EVOLUTION's real evolving activity.

The existing central ecosystem hub shows the actual state, with a small
energy bar. FR/EN localization uses explicit UTF-8. The visual layout,
fixed-size navy/gold design, and the six engine hotspots are unchanged.

Automated tests synthesize gentle picking, intense playing, silence,
recoveries, on/off transitions and live REACTION modulation. They do
NOT replace real guitar monitoring in a host DAW.

## REACTION — phase 6

The 8 existing routes are now connected to actual DSP signal sources. Source
index values (existing preset format) are kept stable:
0 attack; 1 envelope; 2 PULSE; 3 HARMONIC LIFE; 4 MUTATION; 5 MEMORY;
6 EVOLUTION (live playing-driven state activity); 7 silence.

The 8 destinations retain the same stable index order:
0 grain density; 1 PULSE depth; 2 harmonic movement; 3 MEMORY recall;
4 harmonic resonance; 5 grain movement; 6 MEMORY feedback; 7 PULSE cadence.

Each route has on/off, source, destination and amount from -100% to +100%.
The six global controls affect enable, reactivity, interaction depth,
sensitivity, response time and complexity. Modulation is bounded; per-route
and per-destination smoothing avoid discontinuities during live automation
and reassignment. Sources from engines use the previous completed sample,
preventing algebraic feedback cycles. All buffers and routing state are
preallocated; no UI interaction, mutex or new objects in the sample callback.

A harmonic-to-MEMORY recall route can start a stored fragment from a
musical excitation, with cooldown protection. Memory's original
attack-triggered recording remains functional.

To audition a complete chain, select preset **20 - The Living Guitar**:
ATTACK → PULSE depth → HARMONIC movement → MEMORY recall. The other 5
routes are disabled in that example, and can be edited in REACTION.
This adds **no new APVTS IDs** and does not overwrite legacy presets.

## Analysis, preset browser and languages — phase 5

The shared analysis panel now reads real audio-engine measurements:
- RMS-like smoothed input envelope and onset activity from the DSP
- pulse-related rhythmic activity and MEMORY playback activity
- absolute measured harmonic contribution (not a decorative meter)
- confidence-gated YIN-style guitar fundamental detection; displays musical note, Hz and confidence when voiced
- actual per-block input and output peaks in the upper bar
- 30 Hz atomic transfer from the audio thread to the UI; no repaint on the audio callback

The preset browser uses native JUCE controls, with the original midnight-blue and gold palette.
Factory presets **always reset all 97 APVTS parameters to declared defaults before applying
their named changes**. User presets save and restore complete APVTS XML to the per-user
application-data directory, never to the plugin bundle; favorites persist independently.
The browser supports category filtering, text search, keyboard/double-click loading, favorites,
previous/next navigation, saving user presets and confirmed deletion. The current preset name
is preserved in the host state. The 100 factory entries are now organized into ten categories of ten. Their
parameter structures and audio renders are tested automatically. Independent
in-DAW guitar audition and final loudness balancing remain pending.

FR is the default, and FR/EN are changeable live. The language choice is a UI-only
DAW-session property, not a host-automatable DSP parameter. APVTS IDs never change.
The native window remains 1200 × 748, fixed-size, without scrolling.
Future releases still need an in-host visual review and listening QA.

## DSP, macros and master bus — phase 4

The four sound-producing DSP engines have **real mappings** from APVTS parameters.
REACTION interconnects the engines through a real-time matrix, while EVOLUTION now
modulates them based on guitar playing strength, attacks and natural decay.

DSP mappings:

- **GROWTH:** harmonic decay and MEMORY recall
- **MOTION:** PULSE speed and MUTATION density/motion
- **SPACE:** processed stereo width, harmonic spread and grain stereo movement
- **ENERGY:** PULSE/harmonic/MUTATION/MEMORY intensity and PULSE responsiveness
- **GLOBAL INTENSITY:** scales total wet engine contribution, preserving dry input
- **GLOBAL SPEED:** multiplies free-running or DAW-synced PULSE frequency
- **GLOBAL WIDTH:** applies mid/side width only to the wet difference, preserving dry stereo
- **GLOBAL FREEZE:** holds MEMORY's capture buffer and continuously replays captured material
- **GLOBAL BYPASS:** short smoothed crossfade to unprocessed raw input, avoiding a hard toggle
- **HOST SYNC:** reads BPM from JUCE host position only if available; uses free rate otherwise

A JUCE-independent DSP test and a processor mapping audit now run under GitHub Actions.
All engine audio storage is allocated in `prepare()`, not `process()`. GUITAR INTELLIGENCE uses fixed-size arrays and an analysis hop. It does not claim perfect polyphonic tracking or note recognition from arbitrary chords.
Memory uses bounded 12-second preallocated circular buffers to avoid overwriting
3-second fragments before their third replay.

## UI/navigation — phase 3

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

**Phase 3 verification:** the Linux CI runs a JUCE-independent navigation C++ test and a
source-contract check covering the six native click targets, exclusive panel visibility
and persistence of the last selected engine. DAW visual validation remains necessary.

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

REACTION: working 8-route bipolar inter-engine modulation; EVOLUTION: reserved and silent.
```

Source: `Source/Dsp/BiosphereDSP.h` is independent of JUCE, enabling deterministic tests of the actual sample processing algorithm.

## Next milestones

1. Check Phase-5 AU/VST3 artifacts and visually validate the exact 13-inch editor in DAWs.
2. Audition EVOLUTION states and balance their intensity using guitars and DAW recordings.
3. Design, audition and balance all 100 final factory presets.
4. Verify bilingual presentation and UI spacing in real hosts; finish stability/CPU tests.
5. Apple Developer ID signing, notarization and Windows release preparation.

© Liberty Music Software. All rights reserved. No redistribution license is granted by this repository.


## Texte français/anglais — correction UTF-8

Les textes accentués sont décodés explicitement avec
`juce::String::fromUTF8` aux frontières entre littéraux C++ et JUCE :
traductions, descriptions des moteurs, menus, messages d'aide
et noms de presets. Les accents sont conservés. Les symboles
décoratifs superflus sont remplacés par des libellés simples et lisibles.
Le test `Tests/verify_utf8.py` bloque les régressions d'encodage
les plus courantes. Aucun identifiant APVTS ni paramètre DSP n'a changé.

#!/usr/bin/env python3
"""Audit the APVTS -> processor -> native audio core bridge for Phase 4."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
processor = (root / "Source/PluginProcessor.cpp").read_text(encoding="utf-8")
dsp = (root / "Source/Dsp/BiosphereDSP.h").read_text(encoding="utf-8")
registry = (root / "Source/Parameters/BiosphereParameters.h").read_text(encoding="utf-8")
expected = {
    "input", "output", "mix", "pulse", "harmonic", "mutation",
    "memory", "rate", "root", "grain", "feedback",
    "global.intensity", "global.speed", "global.width", "global.sync",
    "global.freeze", "global.bypass",
    "macro.growth", "macro.motion", "macro.space", "macro.energy",
    "pulse.active", "pulse.sensitivity", "pulse.division", "pulse.sync",
    "pulse.swing", "pulse.feedback", "pulse.width", "pulse.mix",
    "harm.active", "harm.mode", "harm.resonance", "harm.decay",
    "harm.damping", "harm.motion", "harm.spread", "harm.mono",
    "mut.active", "mut.density", "mut.character", "mut.motion",
    "mut.pitch", "mut.random", "mut.stereo",
    "mem.active", "mem.length", "mem.capture", "mem.recall",
    "mem.repeat", "mem.reverse", "mem.fade", "mem.freeze",
}
for param in expected:
    assert f'"{param}"' in processor, f"APVTS parameter {param} is not read by processor"
    assert f'"{param}"' in registry, f"APVTS parameter {param} was removed"
for name in (
    "mapPerformanceControls", "pulseMix", "pulseFeedback", "pulseSwing",
    "harmonicDecay", "harmonicDamping", "mutationDensity", "mutationPitch",
    "memoryLength", "memoryRecall", "memoryReverse", "memoryFreeze",
    "globalIntensity", "globalWidth", "globalSync", "wetBypass_",
):
    assert name in dsp, f"DSP core missing mapped parameter: {name}"
assert "p.hasHostBpm = true" in processor, "Host tempo was not forwarded"
assert "dsp_.process(audio, channels, samples, p)" in processor, "DSP call missing"
assert "p.bypass = enabled" in processor, "Smoothed bypass not wired"
# REACTION (eight routes) and EVOLUTION (four states) are both active DSP
# controllers. All host IDs are unchanged.
for name in ("react.active", "react.reactivity", "react.interaction",
             "react.sensitivity", "react.response", "react.complexity"):
    assert f'"{name}"' in processor, f"REACTION control missing: {name}"
for route in range(1, 9):
    for suffix in (".active", ".source", ".destination", ".amount"):
        assert f'"react.route{route}{suffix}"' in processor, (
            f"REACTION route {route} missing {suffix}")
for field in ("active", "source", "destination", "amount"):
    assert f"handles.{field}->load(std::memory_order_relaxed)" in processor
assert 'juce::String prefix = "react.route"' not in processor, (
    "Dynamic route-ID creation is forbidden in the real-time audio callback")
assert "p.reaction.active = enabled" in processor
assert "ReactionMatrix reactionMatrix_" in dsp
assert 'reactionMatrix_.tick(p.reaction, reactionSources)' in dsp
for name in ("evo.active", "evo.rate", "evo.intensity", "evo.sensitivity",
             "evo.stability", "evo.complexity", "evo.recovery"):
    assert f'"{name}"' in processor, f"EVOLUTION APVTS control missing: {name}"
assert "EvolutionEngine evolutionEngine_" in dsp
assert "evolutionEngine_.tick(p.evolution, level, attackActivity_)" in dsp
assert "evolving.activity, // actual EVOLUTION" in dsp
assert "analysis.evolutionState.store(" in processor
assert "analysis.evolutionEnergy.store(" in processor
assert "ecosystem_.setLivingState(" in (root / "Source/PluginEditor.cpp").read_text(encoding="utf-8")
# Guitar Intelligence: the YIN tracker must replace zero crossings, publish
# confidence/MIDI and retune the existing HARMONIC LIFE follow-mono switch.
pitch = (root / "Source/Dsp/GuitarPitchTracker.h").read_text(encoding="utf-8")
editor = (root / "Source/PluginEditor.cpp").read_text(encoding="utf-8")
panel = (root / "Source/Ui/EcosystemView.cpp").read_text(encoding="utf-8")
assert "class GuitarPitchTracker" in pitch
for token in ("normalized_", "candidateCount_", "attackHold_", "silenceHops_"):
    assert token in pitch
assert "guitarPitch_.processSample(dry[0], onset)" in dsp
assert "guitarPitch_.frame()" in dsp
assert "musicalIntervals" in dsp and "fixedIntervals" in dsp
assert "p.harmonicMono ? smoothedHarmonicRoot_ : p.rootHz" in dsp
assert "zeroCrossCounter_" not in dsp
for key in ("pitchConfidence", "midiNote"):
    assert f"analysis.{key}.store(" in processor
    assert f"m.{key}.load(" in editor
    assert f"metrics_.{key}" in panel
print(f"DSP wiring audit PASS: {len(expected)} base APVTS controls + GUITAR INTELLIGENCE")

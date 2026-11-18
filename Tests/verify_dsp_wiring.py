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
# REACTION and EVOLUTION are still separate, pending DSP engines.
assert "REACTION/EVO parameters" in processor
print(f"DSP wiring audit PASS: {len(expected)} Phase-4 APVTS parameters forwarded")

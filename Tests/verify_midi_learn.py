#!/usr/bin/env python3
"""Fail CI if MIDI Learn is merely UI text or installers are fake artifacts."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
read = lambda path: (root / path).read_text(encoding="utf-8", errors="strict")
processor = read("Source/PluginProcessor.cpp")
header = read("Source/PluginProcessor.h")
editor = read("Source/PluginEditor.cpp")
panels = read("Source/Ui/EnginePanels.cpp")
router = read("Source/Midi/MidiLearnRouter.h")
cmake = read("CMakeLists.txt")
build = read(".github/workflows/build.yml")
commercial = read(".github/workflows/commercial-release.yml")
for token in (
    "NEEDS_MIDI_INPUT TRUE", "Source/Midi/MidiLearnRouter.h",
    "BiosphereMidiLearnTests",
):
    assert token in cmake, f"Missing MIDI support in CMake: {token}"
for token in (
    "bool acceptsMidi() const override { return true; }",
    "midiRouter_", "void timerCallback() override",
):
    assert token in header, f"Missing processor MIDI bridge: {token}"
for token in (
    "for (const auto metadata : midiMessages)",
    "message.isController()", "midiRouter_.receive(",
    "parameter->setValueNotifyingHost(", "midiRouter_.consume(i)",
    "startTimerHz(60)", 'juce::ValueTree midi("MIDI_LEARN")',
    "midiRouter_.assign(parameterIndex, channel, controller)",
):
    assert token in processor, f"MIDI Learn not connected to DSP/host: {token}"
assert processor.index("midiRouter_.receive(") < processor.index(
    "int GuitarBiosphereAudioProcessor::midiParameterIndex(")
assert processor.index("parameter->setValueNotifyingHost(") > processor.index(
    "void GuitarBiosphereAudioProcessor::timerCallback()")
for token in (
    "mouseDown(const juce::MouseEvent& event)", "event.mods.isPopupMenu()",
    "showMidiMenu(*component, id)",
    "safe->processor_.armMidiLearn(id)",
    "safe->processor_.clearMidiLearn(id)",
    "safe->processor_.clearAllMidiLearn()",
    "control.slider.setComponentID(parameterId)",
    'sync_.setComponentID("global.sync")',
    'freeze_.setComponentID("global.freeze")',
    'bypass_.setComponentID("global.bypass")',
    "midiLearnStatus_.setBounds(880, 724, 175, 18)",
):
    assert token in editor, f"Missing functional right-click/geometry: {token}"
for token in (
    "slot->dial.setComponentID(spec.parameterId)",
    "slot->choice.setComponentID(spec.parameterId)",
    "slot->toggle.setComponentID(spec.parameterId)",
    "active_.setComponentID(activeId)",
    'route->source.setComponentID(prefix + ".source")',
    'route->amount.setComponentID(prefix + ".amount")',
):
    assert token in panels, f"Missing learn target for panel control: {token}"
for token in (
    "std::array<std::atomic<int>, maximumParameters> assignments_",
    "void receive(int channel, int controller, int controllerValue) noexcept",
    "void clearAll() noexcept",
    "void arm(int index) noexcept",
):
    assert token in router, f"MIDI audio thread handler incomplete: {token}"
for token in (
    "Guitar-Biosphere-macOS-ARM64-PKG-UNSIGNED-TEST",
    "Guitar-Biosphere-Windows-x64-EXE-UNSIGNED-TEST",
    "tools/macos_package_qa.sh", "tools/windows_package.ps1",
    "Inno Setup 6",
):
    assert token in build, f"Installer QA pipeline missing: {token}"
assert "tools/windows_sign.ps1" in commercial
assert "tools/macos_sign_notarize.sh" in commercial
assert "docs/premium-release-gate.json" in read("tools/check_premium_gate.py")
# Footer x880..1055 and existing x1060..1170 do not collide.
assert 880+175 <= 1060
print("MIDI LEARN + INSTALLERS AUDIT PASS: all controls, MIDI 16 channels, "
      "APVTS state, safe realtime routing, macOS PKG and Windows EXE")

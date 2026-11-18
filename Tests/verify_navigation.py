#!/usr/bin/env python3
"""Check that the six JUCE nodes and six panels share a consistent order."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
editor = (root / "Source/PluginEditor.cpp").read_text(encoding="utf-8")
ecosystem = (root / "Source/Ui/EcosystemView.cpp").read_text(encoding="utf-8")
processor = (root / "Source/PluginProcessor.cpp").read_text(encoding="utf-8")
header = (root / "Source/Ui/EcosystemView.h").read_text(encoding="utf-8")
model = (root / "Source/Ui/EngineNavigation.h").read_text(encoding="utf-8")

# These arrays must not drift apart: the order follows top and bottom rows.
assert '"PULSE", "HARMONIC LIFE", "MUTATION", "EVOLUTION", "MEMORY", "REACTION"' in model
assert 'pages_ = { &pulse_, &harmonic_, &mutation_, &evolution_, &memory_, &reaction_ };' in editor

# Actual JUCE buttons must dispatch the selected node (not just paint a hover state).
for expected in [
    "juce::TextButton", "std::array<std::unique_ptr<Hotspot>",
    "button->onClick = [this, index]", "onEngineSelected(index)",
    "setBounds(getNodeHitBox(i).toNearestInt())",
    "setClickingTogglesState(true)", "setWantsKeyboardFocus(true)",
]:
    assert expected in header + ecosystem, f"Missing native button contract: {expected}"

# One page at a time; no accidental reconstruction of panels and attachments.
for expected in [
    "navigation_.select(index)",
    "pages_[static_cast<std::size_t>(i)]->setVisible(navigation_.visible(i))",
    "ecosystem_.setSelectedEngine(navigation_.selected())",
    "processor_.setSelectedEditorPage(navigation_.selected())",
    "selectEngine(processor_.getSelectedEditorPage())",
]:
    assert expected in editor, f"Missing exclusive page switching: {expected}"

# Page preference stays UI-only and is recalled with existing DAW session state.
assert 'state.setProperty("editorPage", getSelectedEditorPage(), nullptr)' in processor
assert 'restored.getProperty(' in processor and '"editorPage"' in processor
print("JUCE node-to-panel contract: PASS (6 accessible native nodes, persistent exclusive navigation)")

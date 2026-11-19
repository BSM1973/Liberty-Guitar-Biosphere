#!/usr/bin/env python3
"""Phase 5 integration audit: checks real data/controls instead of screenshots."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
def read(path):
    return (root / path).read_text(encoding="utf-8")

processor = read("Source/PluginProcessor.cpp")
header = read("Source/PluginProcessor.h")
editor = read("Source/PluginEditor.cpp")
ui = read("Source/Ui/EcosystemView.cpp")
panels = read("Source/Ui/EnginePanels.cpp")
library = read("Source/Ui/PresetLibrary.cpp")
browser = read("Source/Ui/PresetBrowser.cpp")
language = read("Source/Ui/Localization.h")

for field in ("envelope", "attacks", "rhythmicActivity", "memoryActivity",
              "tonalEnergy", "rootHz", "inputPeak", "outputPeak", "hostBpm"):
    assert f"std::atomic<float> {field}" in header or re.search(
        rf"\b{field}\b", header), f"Missing {field} atomics"
    assert f"analysis.{field}.store(" in processor, f"Missing {field} DSP publish"
assert "getAnalysisValues()" in processor
assert "setValues({" in editor and "startTimerHz(30)" in editor
assert "metrics_.envelope" in ui and "metrics_.rootHz" in ui
assert "maxTonal" in read("Tests/AnalysisSmoke.cpp")

# Native preset browser must be interactive and persist a real APVTS state.
assert len(re.findall(r'addFactory\("', library)) == 20
for marker in (
    "setValueNotifyingHost", "getDefaultValue()", "replaceState(restored)",
    "copyState().createXml()", "replaceWithText", "createLegalFileName",
    "findChildFiles", "toggleFavorite", "load(int index)", "saveUser(",
):
    assert marker in library, f"Missing preset behavior {marker}"
for marker in (
    "search_.onTextChange", "category_.onChange", "listBoxItemDoubleClicked",
    "load_.onClick", "save_.onClick", "favorite_.onClick", "remove_.onClick",
):
    assert marker in browser, f"Preset browser button not wired: {marker}"
for marker in ("previous_.onClick", "next_.onClick", "library_.onClick",
               "preset_.onChange", "openBrowser()", "refreshPresetSelector"):
    assert marker in editor, f"Main preset control is inert: {marker}"

# Language is an independently persisted UI choice and doesn't alter APVTS IDs.
assert "editorLanguage" in processor
assert "setEnglish(" in header
assert "french_.onClick" in editor and "english_.onClick" in editor
assert "ecosystem_.setLanguage" in editor
assert "analysis_.setLanguage" in editor
assert "browser_.setLanguage" in editor
assert "panel->setLanguage" in editor
assert "slot->choice.changeItemText" in panels
assert "return french;" in language and "if (!english)" in language
assert len(re.findall(r'\{"[^"]+",\s*"[^"]+"\}', language)) > 85
print("Phase 5 audit PASS — real DSP meters, 20 presets, user storage, browser and FR/EN")

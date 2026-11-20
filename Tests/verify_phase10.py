#!/usr/bin/env python3
"""Audit 100 unique factory recipes, APVTS contract, header geometry and A/B wiring."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda file: (root / file).read_text(encoding="utf-8", errors="strict")
catalog = read("Source/Ui/FactoryPresetCatalog.h")
parameters = read("Source/Parameters/BiosphereParameters.h")
editor = read("Source/PluginEditor.cpp")
header = read("Source/PluginEditor.h")
lab = read("Source/Ui/PresetLabState.h")
browser = read("Source/Ui/PresetBrowser.cpp")
library = read("Source/Ui/PresetLibrary.cpp")
test = read("Tests/FactoryPresetQuality.cpp")

original = re.findall(r'^\s*append\("(\d\d - [^"]+)"', catalog, re.M)
expanded = re.findall(r'^\s*\{\s*"(\d{2,3} - [^"]+)",\s*\d+,\s*', catalog, re.M)
assert len(original) == 20, f"Original 20 not preserved: {len(original)}"
assert len(expanded) == 80, f"Missing expanded factory recipes: {len(expanded)}"
assert len(set(original + expanded)) == 100
for n, name in enumerate(original + expanded, 1):
    assert name.startswith(f"{n:02d} - "), f"Wrong factory order: {name}"
assert 'factory::build()' in library and 'catalog.size() == 100' in library
assert "juce::String::fromUTF8(patch.name.c_str())" in library
assert '"PRESET_LAB"' in library and 'presetState.removeChild(lab, nullptr)' in library

# Every explicit recipe ID must exist in unchanged APVTS registry; generated
# route IDs are checked through all 8 declarations.
api_ids = set(re.findall(r'(?:number|toggle|menu)\("([^"]+)"', parameters))
for route in range(1, 9):
    for field in ("active", "source", "destination", "amount"):
        api_ids.add(f"react.route{route}.{field}")
recipe_ids = set(re.findall(r'\{"([a-z][a-z0-9.]+)",\s*-?[0-9.]+f', catalog))
recipe_ids |= set(re.findall(r'\bput\("([a-z][a-z0-9.]+)"', catalog))
assert recipe_ids <= api_ids, f"Catalog references nonexistent APVTS keys: {recipe_ids - api_ids}"
assert "factory::build()" in library and "parameter->setValueNotifyingHost" in library

for marker in (
    "std::array<juce::ValueTree, 2>", "PresetHistory<juce::ValueTree>",
    'juce::ValueTree("PRESET_LAB")', "state_.replaceState(recalled)",
    "state_.replaceState(entry.snapshot.createCopy())",
    "history_.undo()", "history_.redo()", "history_.push(",
    "publish();", "pendingFrames_ >= 6", "snapshot() const",
):
    assert marker in lab, f"Missing state/history behavior {marker}"
for marker in (
    "compareA_.onClick", "compareB_.onClick", "undo_.onClick",
    "redo_.onClick", "presetLab_.choose(0)", "presetLab_.choose(1)",
    "presetLab_.undo()", "presetLab_.redo()", "presetLab_.tick()",
    "presetLab_.commitNow()", "browser_.onBeforePresetLoad",
):
    assert marker in editor, f"Inert PRESET LAB button: {marker}"
assert "onBeforePresetLoad) onBeforePresetLoad()" in browser
assert "100 factory sounds" in browser

# Actual header collision areas: PRESET text, A/B, Undo, Redo, preset combo.
regions = {
    "label": (567, 19, 185, 17),
    "A": (756, 17, 26, 20),
    "B": (788, 17, 26, 20),
    "Undo": (821, 17, 55, 20),
    "Redo": (883, 17, 57, 20),
    "selector": (567, 42, 244, 33),
    "previous": (821, 42, 35, 33),
    "next": (861, 42, 35, 33),
    "library": (901, 42, 40, 33),
    "FR": (1094, 17, 37, 29),
    "EN": (1138, 17, 37, 29),
}
def intersect(a, b):
    ax, ay, aw, ah = a
    bx, by, bw, bh = b
    return ax < bx + bw and ax + aw > bx and ay < by + bh and ay + ah > by
for first, box in regions.items():
    for second, other in regions.items():
        if first < second:
            assert not intersect(box, other), f"UI collision: {first}/{second}"
for name, box in regions.items():
    assert all(i >= 0 for i in box[:2]) and box[0] + box[2] <= 1200, name
    assert f"{box[0]}, {box[1]}, {box[2]}, {box[3]}" in editor, (
        f"{name} does not use the audited on-screen geometry")
assert "for (std::size_t i = 0; i < patches.size(); ++i)" in test
assert "render(patch)" in test
assert "m.rms" in test and "m.peak" in test

print("PRESET LAB audit PASS: 100 catalog recipes, valid APVTS IDs, 4 live UI buttons and collision-free header")

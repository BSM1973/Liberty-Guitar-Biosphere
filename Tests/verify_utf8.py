#!/usr/bin/env python3
"""Prevent unreadable UTF-8 labels from reappearing in the JUCE UI."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
paths = [
    "Source/Ui/Localization.h",
    "Source/Ui/EnginePanels.h", "Source/Ui/EnginePanels.cpp",
    "Source/Ui/EcosystemView.cpp", "Source/Ui/PresetBrowser.cpp",
    "Source/Ui/PresetLibrary.cpp", "Source/PluginEditor.h",
    "Source/PluginEditor.cpp", "Source/Parameters/BiosphereParameters.h",
]
sources = {path: (root / path).read_text(encoding="utf-8", errors="strict") for path in paths}
for path, code in sources.items():
    for broken in ("\ufffd", "Ã©", "Ã¨", "Â·", "â€™", "â€œ", "â€", "â€¢"):
        assert broken not in code, f"Corrupted UTF-8 in {path}: {broken}"
    for ornament in ("★", "☆", "●", "·"):
        assert ornament not in code, f"Unsupported symbol in {path}: {ornament}"

loc = sources["Source/Ui/Localization.h"]
assert "tr(const char* frenchUtf8, bool english)" in loc
assert "return tr(juce::String::fromUTF8(frenchUtf8), english);" in loc
assert "juce::String::fromUTF8(entry.fr)" in loc
assert "juce::String::fromUTF8(entry.en)" in loc

panels = sources["Source/Ui/EnginePanels.cpp"]
assert "const char* summaryUtf8" in panels
assert "summary_(juce::String::fromUTF8(summaryUtf8))" in panels
assert 'juce::String::fromUTF8("Mémoire")' in panels
assert 'juce::String::fromUTF8("Évolution")' in panels
assert 'juce::String::fromUTF8("Paramètre : ")' in panels

library = sources["Source/Ui/PresetLibrary.cpp"]
assert "item.name = juce::String::fromUTF8(patch.name.c_str());" in library
catalog = (root / "Source/Ui/FactoryPresetCatalog.h").read_text(encoding="utf-8")
assert catalog.count('append("') == 20
assert "std::array<Design, 80>" in catalog
browser = sources["Source/Ui/PresetBrowser.cpp"]
assert 'juce::String::fromUTF8(" résultats")' in browser
assert "juce::String(\"F\")" in browser
editor = sources["Source/PluginEditor.cpp"]
for label in ("Mix maître", "Largeur stéréo", "Préréglage précédent"):
    assert f'juce::String::fromUTF8("{label}")' in editor
parameters = sources["Source/Parameters/BiosphereParameters.h"]
assert 'juce::String::fromUTF8("Métallique")' in parameters
assert 'juce::String::fromUTF8("Mémoire")' in parameters
assert "juce::String::fromUTF8(title)" in parameters

print("UTF-8 UI audit PASS — accented text and bilingual labels decoded explicitly")

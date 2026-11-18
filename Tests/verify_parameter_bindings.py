#!/usr/bin/env python3
"""Static APVTS contract test: no visible audio control may point to a missing ID.

This intentionally does not replace JUCE host or DAW integration tests.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
registry = (ROOT / "Source/Parameters/BiosphereParameters.h").read_text(encoding="utf-8")
panels = (ROOT / "Source/Ui/EnginePanels.cpp").read_text(encoding="utf-8")
editor = (ROOT / "Source/PluginEditor.cpp").read_text(encoding="utf-8")
processor = (ROOT / "Source/PluginProcessor.cpp").read_text(encoding="utf-8")

registered = re.findall(r'\b(?:number|toggle|menu)\("([^"]+)"', registry)
assert len(registered) == len(set(registered)), "Duplicate APVTS IDs in the registry"
assert len(registered) == 65, f"Expected 65 static IDs, found {len(registered)}"
all_ids = set(registered)

for number in range(1, 9):
    for part in ("active", "source", "destination", "amount"):
        route_id = f"react.route{number}.{part}"
        assert route_id not in all_ids
        all_ids.add(route_id)
assert len(all_ids) == 97, f"Expected 97 stable parameters, got {len(all_ids)}"

panel_ids = re.findall(
    r'ControlSpec::(?:dial|menu|toggle)\("[^"]+",\s*"([^"]+)"', panels
)
assert len(panel_ids) == 46, f"Expected 46 engine controls, got {len(panel_ids)}"
assert all(panel_ids), "A visible engine control lacks a parameter ID"

global_ids = re.findall(r'initialiseGlobalControl\(\d+,\s*(?:juce::String::fromUTF8\()?"[^"]+"\)?,\s*"([^"]+)"', editor)
assert len(global_ids) == 10, f"Expected 10 macro/master knobs, got {len(global_ids)}"
assert all(global_ids), "A macro/master knob lacks an APVTS ID"

for parameter in panel_ids + global_ids:
    assert parameter in all_ids, f"UI requests an unknown parameter: {parameter}"

for parameter in (
    "pulse.active", "harm.active", "mut.active", "react.active",
    "mem.active", "evo.active", "global.sync", "global.freeze", "global.bypass"
):
    assert parameter in all_ids, f"Missing switch: {parameter}"

for token in ("prefix + \".active\"", "prefix + \".source\"",
              "prefix + \".destination\"", "prefix + \".amount\""):
    assert token in panels, f"Route attachment missing: {token}"

for param in ("global.sync", "global.freeze", "global.bypass"):
    assert f'"{param}"' in editor, f"Missing global switch attachment: {param}"

for legacy in ("input", "output", "mix", "pulse", "harmonic", "mutation",
               "memory", "rate", "root", "grain", "feedback"):
    assert legacy in registered, f"Legacy parameter ID was changed: {legacy}"
    assert f'value("{legacy}")' in processor, f"Legacy DSP parameter disconnected: {legacy}"

assert (
    'value("global.bypass")' in processor
    or 'getRawParameterValue("global.bypass")' in processor
    or 'enabled("global.bypass")' in processor
), "Global bypass is no longer read from APVTS"
print(f"APVTS mapping OK: {len(all_ids)} IDs, {len(panel_ids)} panel controls, "
      f"{len(global_ids)} master controls, 8 four-parameter routes.")

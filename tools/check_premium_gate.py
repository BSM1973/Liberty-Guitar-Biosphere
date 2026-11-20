#!/usr/bin/env python3
"""Conservative release gate: schema is CI-tested, approval is manual evidence."""
import json
import re
import sys
from pathlib import Path

gate = json.loads(Path("docs/premium-release-gate.json").read_text(encoding="utf-8"))
keys = (
    "logic_pro_mac_au",
    "ableton_live_mac_vst3",
    "reaper_windows_vst3",
    "preset_100_listening_and_balance",
    "ab_undo_redo_project_reopen",
    "screen_13inch_fr_en_no_overlap",
    "midi_learn_real_controller_and_project_reload",
    "macos_pkg_install_reopen",
    "windows_exe_install_reopen",
)
assert re.fullmatch(r"\d+\.\d+\.\d+", gate["version"])
assert isinstance(gate["approved"], bool)
assert set(gate["manual_qa"]) == set(keys)
assert all(type(gate["manual_qa"][key]) is bool for key in keys)
assert "evidence" in gate and all(
    key in gate["evidence"] for key in ("notes", "approved_by", "approved_at")
)
if "--schema-only" in sys.argv:
    print("PREMIUM RELEASE GATE schema PASS (approval status is independent)")
    sys.exit(0)
expected = sys.argv[1] if len(sys.argv) > 1 else ""
assert gate["version"] == expected, "Version mismatch; release refused"
assert gate["approved"] is True, "Commercial release not yet approved"
assert all(gate["manual_qa"].values()), "Real DAW/listening/UI acceptance incomplete"
assert gate["evidence"]["notes"].strip(), "No manual evidence documented"
assert gate["evidence"]["approved_by"].strip(), "No reviewer approved the release"
assert gate["evidence"]["approved_at"].strip(), "Missing approval date"
print("PREMIUM RELEASE GATE PASS: documented manual evidence and approval present")

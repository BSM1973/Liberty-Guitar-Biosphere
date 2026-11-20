# GUITAR BIOSPHERE — PHASE 11 PREMIUM QA

This is a **release candidate test plan**, not a certificate that these
tests were performed. The signed commercial release remains **blocked**
until the actual DAW checks have been performed and documented.

## Automated evidence — GitHub Actions

- All native CTest suites on macOS ARM64 and Windows x64 pass
- Linux C++ regression suite, 100 factory render tests and UTF-8/layout audits pass
- PremiumStability: 44.1/48/96 kHz; mono/stereo; irregular blocks from 1 to 2048 samples;
  changes in host tempo, automated routing, NaN/Inf host input, restart/reset;
  zero allocations inside process() (instrumented global operator new)
- Tracktion pluginval (pinned revision, strictness 5, headless host API checks)
  must return success for AU and VST3 macOS and VST3 Windows
- Store pluginval JUnit reports and record SHA256 hashes of delivered artifacts
- Never equate these automated API checks with a real Logic/Ableton/Reaper GUI session

## Manual host acceptance — ALL must be checked

| Host | Format | Steps | Status |
| --- | --- | --- | --- |
| Logic Pro, macOS Apple Silicon | AU | Rescan, load/save/reopen project, change tempo, automate 8 routes, MIDI Learn CC with mapped hardware, test 100 presets, A/B and Undo/Redo; listen for clicks at 64/128/512 buffer | NOT RUN |
| Ableton Live, macOS Apple Silicon | VST3 | Rescan, playback/record, MIDI routing and Learn recall, automation, bypass/freeze, pitch tracking, repeated preset changes, offline bounce and reopen | NOT RUN |
| REAPER, Windows x64 | VST3 | Host rescan, mono/stereo, sample-rate change, MIDI CC Learn/remap/clear, parameter automation, state restore and 100-preset navigation | NOT RUN |
| macOS 13-inch display | FR / EN | Open every engine panel, browser and all A/B/Undo controls; inspect every string, accent and border at 100% scaling | NOT RUN |

The plugin must remain stable after stop/play and when opening or closing the
editor repeatedly. No symbol may replace any accented French letter. No
overlapping labels, tooltips or controls anywhere in the six panels.

## 100-preset quality acceptance — NOT RUN

Using **at least three** recorded guitar DI types (single coil, humbucker,
sustained/noisy high-gain), audition each of the 100 presets and record:

- Note tracking accuracy and response to gentle/strong attacks
- Sound character and musical distinction relative to adjacent presets
- Perceived loudness vs dry input at the same gain; prevent unexpected jumps
- Peak, clipping, long tails, silence, feedback, clicks at preset changes
- REACTION and EVOLUTION responsiveness; MEMORY availability after onset
- 32/64/128/512 sample block CPU and dropouts in host under real playback
- A/B comparison and Undo/Redo with presets, followed by project save/reopen

Technical RMS tests currently show sane output levels but **do not establish**
artist-approved perceived loudness or full LUFS matching.

## Mac signed release

Six GitHub Actions repository secrets (matching the Liberty Organix convention):

- APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64
- APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64
- APPLE_P12_PASSWORD
- APPLE_NOTARY_APPLE_ID
- APPLE_NOTARY_APP_PASSWORD
- APPLE_NOTARY_TEAM_ID

A trusted manual workflow from **main** builds/tests plugins again, runs
pluginval, signs AU/VST3 with Developer ID, signs the installer, submits to
Apple notarytool, requires **Accepted**, staples the ticket and verifies
Gatekeeper. It then downloads and re-verifies the exact public artifact.

## Windows signed release

GitHub Actions secrets required:

- WINDOWS_SIGNING_PFX_BASE64
- WINDOWS_SIGNING_PFX_PASSWORD

The protected manual workflow rebuilds/tests the x64 VST3, runs pluginval,
signs the internal executable with Authenticode SHA256 + timestamp, checks
signature trust with Microsoft SignTool, then builds an Inno Setup 6
**Windows x64 Setup.exe** containing the signed VST3. It separately signs the
installer with Authenticode and verifies signature trust. Both the optional
signed VST3 ZIP and signed Setup.exe are re-downloaded for SHA256 integrity
comparison; the published Setup.exe signature is checked again.

Standard, ungated native CI creates clearly named **UNSIGNED-TEST** installers:
a macOS AU/VST3 .pkg and a Windows x64 VST3 Setup.exe. These are only for
local technical tests; they are not signed/notarized and not for customers.

Do not put .p12, .pfx, private keys or passwords in the repository or chat.
Repository secrets are not transferable automatically between projects.

## Commercial gate (default DENY)

The manually audited statuses reside in
`docs/premium-release-gate.json`. At present every DAW/listening
approval is **false**. When tests are actually performed, attach the date,
reviewer and evidence, review changes, and explicitly approve the release.
Only then may the `.github/workflows/commercial-release.yml` manual workflow
produce commercial signed artifacts. Until then, regular CI artifacts
remain explicitly **unsigned technical test builds**.

The version in this repository is currently 0.5.0 and is a development
candidate; do not advertise it as final v1.0 unless it passes all gates.

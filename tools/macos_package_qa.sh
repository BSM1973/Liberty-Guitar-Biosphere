#!/usr/bin/env bash
# Unsigned technical-QA installer. Never label this as a commercial build.
set -euo pipefail
version="$1"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || exit 1
au="build/GuitarBiosphere_artefacts/Release/AU/Guitar Biosphere.component"
vst="build/GuitarBiosphere_artefacts/Release/VST3/Guitar Biosphere.vst3"
for bundle in "$au" "$vst"; do
  test -f "$bundle/Contents/MacOS/Guitar Biosphere" ||
    { echo "::error::Missing native plugin executable in $bundle"; exit 1; }
done
stage="$RUNNER_TEMP/GuitarBiosphere-QA-root"
mkdir -p "$stage/Library/Audio/Plug-Ins/Components" \
         "$stage/Library/Audio/Plug-Ins/VST3" release-output
/usr/bin/ditto "$au" "$stage/Library/Audio/Plug-Ins/Components/Guitar Biosphere.component"
/usr/bin/ditto "$vst" "$stage/Library/Audio/Plug-Ins/VST3/Guitar Biosphere.vst3"
# This is only an unsigned TEST package; fresh ad-hoc signatures repair
# resource seals for loaded AU/VST3 bundles. The commercial workflow later
# replaces these with real Developer ID signatures.
for component in "$stage/Library/Audio/Plug-Ins/Components/Guitar Biosphere.component"                  "$stage/Library/Audio/Plug-Ins/VST3/Guitar Biosphere.vst3"; do
  /usr/bin/codesign --force --deep --sign - "$component"
  /usr/bin/codesign --verify --deep --strict --verbose=2 "$component"
done
pkg="release-output/Guitar-Biosphere-v$version-macOS-ARM64-UNSIGNED-TEST.pkg"
/usr/bin/pkgbuild --root "$stage" \
  --identifier "com.libertymusicsoftware.guitarbiosphere.installer" \
  --version "$version" --install-location / --ownership recommended "$pkg"
test -s "$pkg"
/usr/sbin/pkgutil --payload-files "$pkg" > release-output/macOS-payload.txt
/usr/bin/grep -Fq "Library/Audio/Plug-Ins/Components/Guitar Biosphere.component/Contents/MacOS/Guitar Biosphere" release-output/macOS-payload.txt
/usr/bin/grep -Fq "Library/Audio/Plug-Ins/VST3/Guitar Biosphere.vst3/Contents/MacOS/Guitar Biosphere" release-output/macOS-payload.txt
/usr/bin/shasum -a 256 "$pkg" > release-output/macOS-SHA256.txt
echo "PASS: unsigned technical-QA AU+VST3 installer created and verified: $pkg"

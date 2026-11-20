#!/usr/bin/env bash
# Unsigned AU/VST3 installer for internal tests ONLY.
set -euo pipefail
version="$1"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "Expected version X.Y.Z"; exit 1; }
au="build/GuitarBiosphere_artefacts/Release/AU/Guitar Biosphere.component"
vst="build/GuitarBiosphere_artefacts/Release/VST3/Guitar Biosphere.vst3"
for bundle in "$au" "$vst"; do
  test -f "$bundle/Contents/MacOS/Guitar Biosphere" || {
    echo "Missing plugin binary: $bundle"; exit 1;
  }
done
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT
mkdir -p "$stage/Library/Audio/Plug-Ins/Components" "$stage/Library/Audio/Plug-Ins/VST3" release-output
/usr/bin/ditto "$au" "$stage/Library/Audio/Plug-Ins/Components/Guitar Biosphere.component"
/usr/bin/ditto "$vst" "$stage/Library/Audio/Plug-Ins/VST3/Guitar Biosphere.vst3"
# Local development-only ad-hoc signing: NOT Apple Developer ID, NOT notarized.
for bundle in "$stage/Library/Audio/Plug-Ins/Components/Guitar Biosphere.component" \
              "$stage/Library/Audio/Plug-Ins/VST3/Guitar Biosphere.vst3"; do
  /usr/bin/codesign --force --deep --sign - "$bundle"
  /usr/bin/codesign --verify --deep --verbose=2 "$bundle"
done
pkg="release-output/Guitar-Biosphere-v$version-macOS-ARM64-UNSIGNED-TEST.pkg"
/usr/bin/pkgbuild --root "$stage" \
  --identifier "com.libertymusicsoftware.guitarbiosphere.installer" \
  --version "$version" --install-location / --ownership recommended "$pkg"
payload="$(/usr/sbin/pkgutil --payload-files "$pkg")"
echo "$payload" | /usr/bin/grep -Fq "Library/Audio/Plug-Ins/Components/Guitar Biosphere.component/Contents/MacOS/Guitar Biosphere"
echo "$payload" | /usr/bin/grep -Fq "Library/Audio/Plug-Ins/VST3/Guitar Biosphere.vst3/Contents/MacOS/Guitar Biosphere"
test -s "$pkg"
/usr/bin/shasum -a 256 "$pkg"
echo "UNSIGNED TEST INSTALLER: not notarized; NOT for commercial release."

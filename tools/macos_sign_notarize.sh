#!/usr/bin/env bash
# Fail closed: only Developer ID signed + Apple Accepted + stapled packages.
set -euo pipefail
for key in APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64 APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64 APPLE_P12_PASSWORD APPLE_NOTARY_APPLE_ID APPLE_NOTARY_APP_PASSWORD APPLE_NOTARY_TEAM_ID; do
  if [[ -z "$(printenv "$key" || true)" ]]; then
    echo "::error::Missing Apple signing secret $key; NO commercial artifact"
    exit 1
  fi
done
version="$1"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || exit 1
source_au="build/GuitarBiosphere_artefacts/Release/AU/Guitar Biosphere.component"
source_vst="build/GuitarBiosphere_artefacts/Release/VST3/Guitar Biosphere.vst3"
for bundle in "$source_au" "$source_vst"; do
  test -f "$bundle/Contents/MacOS/Guitar Biosphere" ||
    { echo "::error::Missing plugin executable: $bundle"; exit 1; }
done
keychain="$RUNNER_TEMP/guitar-biosphere-commercial.keychain-db"
password="$(/usr/bin/uuidgen)"
app_p12="$RUNNER_TEMP/guitar-biosphere-app.p12"
install_p12="$RUNNER_TEMP/guitar-biosphere-installer.p12"
cleanup()
{
  /bin/rm -f "$app_p12" "$install_p12"
  /usr/bin/security delete-keychain "$keychain" >/dev/null 2>&1 || true
}
trap cleanup EXIT
/usr/bin/security create-keychain -p "$password" "$keychain"
/usr/bin/security set-keychain-settings -lut 21600 "$keychain"
/usr/bin/security unlock-keychain -p "$password" "$keychain"
/usr/bin/security list-keychains -d user -s "$keychain"
/usr/bin/security default-keychain -s "$keychain"
printf '%s' "$APPLE_DEVELOPER_ID_APPLICATION_P12_BASE64" | /usr/bin/base64 -D > "$app_p12"
printf '%s' "$APPLE_DEVELOPER_ID_INSTALLER_P12_BASE64" | /usr/bin/base64 -D > "$install_p12"
/usr/bin/security import "$app_p12" -k "$keychain" -P "$APPLE_P12_PASSWORD" -T /usr/bin/codesign
/usr/bin/security import "$install_p12" -k "$keychain" -P "$APPLE_P12_PASSWORD" -T /usr/bin/pkgbuild
/usr/bin/security set-key-partition-list -S apple-tool:,apple: -s -k "$password" "$keychain"
app_id="$(/usr/bin/security find-identity -v -p codesigning "$keychain" | /usr/bin/awk -F '"' '/Developer ID Application:/{print $2; exit}')"
installer_id="$(/usr/bin/security find-identity -v "$keychain" | /usr/bin/awk -F '"' '/Developer ID Installer:/{print $2; exit}')"
[[ -n "$app_id" && -n "$installer_id" ]] ||
  { echo "::error::Developer ID Application AND Installer certificates required"; exit 1; }
[[ "$app_id" == *"($APPLE_NOTARY_TEAM_ID)"* && "$installer_id" == *"($APPLE_NOTARY_TEAM_ID)"* ]] ||
  { echo "::error::Developer ID certificates have a different Apple Team ID"; exit 1; }
stage="$RUNNER_TEMP/GuitarBiosphere-package-root"
mkdir -p "$stage/Library/Audio/Plug-Ins/Components" \
         "$stage/Library/Audio/Plug-Ins/VST3" release-output release-validation
for bundle in "$source_au" "$source_vst"; do
  if [[ "$bundle" == *.component ]]; then
    target="$stage/Library/Audio/Plug-Ins/Components/$(basename "$bundle")"
  else
    target="$stage/Library/Audio/Plug-Ins/VST3/$(basename "$bundle")"
  fi
  /usr/bin/ditto "$bundle" "$target"
  /usr/bin/codesign --force --options runtime --timestamp --sign "$app_id" "$target"
  /usr/bin/codesign --verify --strict --deep --verbose=2 "$target"
  signature="$(/usr/bin/codesign --display --verbose=4 "$target" 2>&1)"
  /usr/bin/grep -Fq "Authority=Developer ID Application:" <<< "$signature"
  /usr/bin/grep -q "runtime" <<< "$signature"
done
pkg="release-output/Guitar-Biosphere-v$version-macOS-SIGNED-NOTARIZED.pkg"
/usr/bin/pkgbuild --root "$stage" \
  --identifier "com.libertymusicsoftware.guitarbiosphere.installer" \
  --version "$version" --install-location / --ownership recommended \
  --sign "$installer_id" "$pkg"
/usr/sbin/pkgutil --check-signature "$pkg" | /usr/bin/tee release-validation/installer-signature.txt
/usr/sbin/pkgutil --payload-files "$pkg" > release-validation/payload.txt
/usr/bin/grep -Fq "Library/Audio/Plug-Ins/Components/Guitar Biosphere.component/Contents/MacOS/Guitar Biosphere" release-validation/payload.txt
/usr/bin/grep -Fq "Library/Audio/Plug-Ins/VST3/Guitar Biosphere.vst3/Contents/MacOS/Guitar Biosphere" release-validation/payload.txt
response="$RUNNER_TEMP/biosphere-notary-response.json"
/usr/bin/xcrun notarytool submit "$pkg" \
  --apple-id "$APPLE_NOTARY_APPLE_ID" \
  --password "$APPLE_NOTARY_APP_PASSWORD" \
  --team-id "$APPLE_NOTARY_TEAM_ID" \
  --wait --output-format json > "$response"
status="$(/usr/bin/python3 -c 'import json,sys;print(json.load(open(sys.argv[1])).get("status", "Unknown"))' "$response")"
request_id="$(/usr/bin/python3 -c 'import json,sys;print(json.load(open(sys.argv[1])).get("id", ""))' "$response")"
echo "Apple notarization status: $status"
if [[ -n "$request_id" ]]; then
  /usr/bin/xcrun notarytool log "$request_id" \
    --apple-id "$APPLE_NOTARY_APPLE_ID" \
    --password "$APPLE_NOTARY_APP_PASSWORD" \
    --team-id "$APPLE_NOTARY_TEAM_ID" \
    release-validation/notary-result.json 2>/dev/null || true
fi
[[ "$status" == "Accepted" ]] ||
  { echo "::error::Apple did not accept notarization; no commercial artifact"; exit 1; }
/usr/bin/xcrun stapler staple "$pkg"
/usr/bin/xcrun stapler validate "$pkg" | /usr/bin/tee release-validation/stapler.txt
/usr/sbin/spctl --assess --type install --verbose=2 "$pkg" 2>&1 |
  /usr/bin/tee release-validation/gatekeeper.txt
test -s "$pkg"
/usr/bin/shasum -a 256 "$pkg" | /usr/bin/tee release-validation/SHA256.txt
echo "PASS: Developer ID AU/VST3 and installer, Apple Accepted, stapled and Gatekeeper verified"

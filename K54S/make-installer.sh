#!/bin/bash
# =============================================================================
#  K-54 S – erzeugt einen fertigen Mac-Installer:  K-54 S-<Version>-Installer.dmg
#  Inhalt der DMG: "K-54 S installieren.pkg" (installiert AU, VST3 und App)
#
#  Aufruf im Terminal:   ./make-installer.sh
#  Voraussetzung:        Xcode Command Line Tools + CMake (einmalig)
# =============================================================================
set -e
cd "$(dirname "$0")"
ROOT="$(pwd)"

echo ""
echo "  KELLER54 · K-54 S Installer"
echo "  -------------------------"

# ---------- Werkzeuge pruefen
if ! xcode-select -p >/dev/null 2>&1; then
  echo "Die Xcode Command Line Tools fehlen. Es oeffnet sich gleich ein Fenster zur Installation."
  echo "Danach dieses Skript einfach noch einmal starten."
  xcode-select --install || true
  exit 1
fi
# CMake: vorhandenes nehmen, sonst lokal in den Projektordner laden (kein Homebrew noetig)
if ! command -v cmake >/dev/null 2>&1; then
  for C in /Applications/CMake.app/Contents/bin /opt/homebrew/bin /usr/local/bin "$ROOT/.tools/cmake/CMake.app/Contents/bin"; do
    [ -x "$C/cmake" ] && export PATH="$C:$PATH" && break
  done
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "==> CMake wird in den Projektordner geladen"
  mkdir -p "$ROOT/.tools"
  curl -fL --progress-bar -o "$ROOT/.tools/cmake.tar.gz" \
    "https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5-macos-universal.tar.gz"
  tar -xzf "$ROOT/.tools/cmake.tar.gz" -C "$ROOT/.tools"
  mv "$ROOT/.tools/cmake-3.30.5-macos-universal" "$ROOT/.tools/cmake"
  export PATH="$ROOT/.tools/cmake/CMake.app/Contents/bin:$PATH"
fi
cmake --version | head -1

VERSION=$(grep -m1 'project(K55 VERSION' CMakeLists.txt | sed -E 's/.*VERSION ([0-9.]+).*/\1/')
BUILD="$ROOT/build-installer"
ART="$BUILD/K55_artefacts/Release"
STAGE="$BUILD/pkgroot"
DMGDIR="$BUILD/dmg"
DMG="$ROOT/K-54 S-$VERSION-Installer.dmg"

# ---------- Signieren & Notarisieren (optional, sonst Ad-hoc wie bisher)
#   K54_SIGN_APP        = "Developer ID Application: Marco Franke (TEAMID)"
#   K54_SIGN_INSTALLER  = "Developer ID Installer: Marco Franke (TEAMID)"
#   Notarisieren:  K54_NOTARY_PROFILE (aus "xcrun notarytool store-credentials")
#             oder K54_NOTARY_APPLE_ID + K54_NOTARY_TEAM_ID + K54_NOTARY_PASSWORD (app-spezifisches Passwort)
mkdir -p "$BUILD"
ENT="$BUILD/entitlements.plist"
cat > "$ENT" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>com.apple.security.device.audio-input</key><true/>
</dict></plist>
PLIST
sign_bundle() {
  if [ -n "$K54_SIGN_APP" ]; then
    codesign --force --deep --options runtime --timestamp --entitlements "$ENT" --sign "$K54_SIGN_APP" "$1"
    codesign --verify --strict --deep "$1" || { echo "Signatur von $1 fehlerhaft"; exit 1; }
  else
    codesign --force --deep --sign - "$1" >/dev/null 2>&1 || true   # Ad-hoc-Signatur (noetig fuer Apple Silicon)
  fi
}
notarize() {
  local args=()
  if [ -n "$K54_NOTARY_PROFILE" ]; then args=(--keychain-profile "$K54_NOTARY_PROFILE")
  elif [ -n "$K54_NOTARY_APPLE_ID" ] && [ -n "$K54_NOTARY_PASSWORD" ]; then
    args=(--apple-id "$K54_NOTARY_APPLE_ID" --team-id "$K54_NOTARY_TEAM_ID" --password "$K54_NOTARY_PASSWORD")
  else
    echo "   $(basename "$1"): nicht notarisiert (keine Apple-Zugangsdaten)"; return 0
  fi
  echo "==> Notarisieren bei Apple: $(basename "$1") (meist 1-10 Minuten)"
  local out id status
  out=$(xcrun notarytool submit "$1" "${args[@]}" --wait --timeout 60m --output-format json) || true
  id=$(echo "$out" | plutil -extract id raw - 2>/dev/null)
  status=$(echo "$out" | plutil -extract status raw - 2>/dev/null)
  echo "   Apple sagt: ${status:-?} (${id:-keine ID})"
  if [ "$status" != "Accepted" ]; then
    [ -n "$id" ] && xcrun notarytool log "$id" "${args[@]}" || echo "$out"
    exit 1
  fi
  xcrun stapler staple "$1"
}

# ---------- Bauen (Universal: Apple Silicon + Intel)
echo "==> Kompilieren von K-54 S $VERSION (beim ersten Mal 5-10 Minuten)"
cmake -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DK55_COPY_AFTER_BUILD=OFF >/dev/null
cmake --build "$BUILD" --config Release -j "$(sysctl -n hw.ncpu)"

for B in "AU/K-54 S.component" "VST3/K-54 S.vst3" "Standalone/K-54 S.app"; do
  if [ ! -d "$ART/$B" ]; then echo "Fehler: $ART/$B wurde nicht gebaut."; exit 1; fi
  sign_bundle "$ART/$B"
done

# ---------- Paket-Inhalt zusammenstellen
echo "==> Installationspaket erstellen"
rm -rf "$STAGE" "$DMGDIR" "$BUILD/scripts" "$BUILD/res"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/Components" "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Applications"
ditto "$ART/AU/K-54 S.component"   "$STAGE/Library/Audio/Plug-Ins/Components/K-54 S.component"
ditto "$ART/VST3/K-54 S.vst3"      "$STAGE/Library/Audio/Plug-Ins/VST3/K-54 S.vst3"
ditto "$ART/Standalone/K-54 S.app" "$STAGE/Applications/K-54 S.app"

# Bundles immer an den festen Ort installieren (nicht "umziehen" lassen)
pkgbuild --analyze --root "$STAGE" "$BUILD/component.plist" >/dev/null
i=0
while /usr/libexec/PlistBuddy -c "Print :$i" "$BUILD/component.plist" >/dev/null 2>&1; do
  /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" "$BUILD/component.plist" 2>/dev/null \
    || /usr/libexec/PlistBuddy -c "Add :$i:BundleIsRelocatable bool false" "$BUILD/component.plist"
  i=$((i+1))
done

# Nach der Installation: Audio-Unit-Liste auffrischen, alte Benutzer-Kopien entfernen
mkdir -p "$BUILD/scripts"
cat > "$BUILD/scripts/postinstall" <<'POST'
#!/bin/bash
for H in /Users/*; do
  rm -rf "$H/Library/Audio/Plug-Ins/Components/K-54 S.component" "$H/Library/Audio/Plug-Ins/VST3/K-54 S.vst3" 2>/dev/null
done
# Quarantaene-Kennzeichen entfernen, damit DAW und App ohne Rueckfrage starten
xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/Components/K-54 S.component" "/Library/Audio/Plug-Ins/VST3/K-54 S.vst3" "/Applications/K-54 S.app" 2>/dev/null
killall -9 AudioComponentRegistrar 2>/dev/null
exit 0
POST
chmod +x "$BUILD/scripts/postinstall"

pkgbuild --root "$STAGE" \
         --component-plist "$BUILD/component.plist" \
         --scripts "$BUILD/scripts" \
         --identifier "de.keller54.k54s" \
         --version "$VERSION" \
         --install-location "/" \
         "$BUILD/K54S-core.pkg" >/dev/null

mkdir -p "$BUILD/res"
cp Installer/welcome.html Installer/conclusion.html "$BUILD/res/"
sed "s/@VERSION@/$VERSION/g" Installer/distribution.xml > "$BUILD/distribution.xml"

mkdir -p "$DMGDIR"
PB_SIGN=()
[ -n "$K54_SIGN_INSTALLER" ] && PB_SIGN=(--sign "$K54_SIGN_INSTALLER" --timestamp)
productbuild --distribution "$BUILD/distribution.xml" \
             --resources "$BUILD/res" \
             --package-path "$BUILD" \
             "${PB_SIGN[@]}" \
             "$DMGDIR/K-54 S installieren.pkg" >/dev/null
[ -n "$K54_SIGN_INSTALLER" ] && notarize "$DMGDIR/K-54 S installieren.pkg"

cp Installer/Anleitung.txt "$DMGDIR/Anleitung.txt"
cp "Installer/K-54 S deinstallieren.command" "$DMGDIR/K-54 S deinstallieren.command"
chmod +x "$DMGDIR/K-54 S deinstallieren.command"

# ---------- DMG
echo "==> DMG erstellen"
rm -f "$DMG"
hdiutil create -volname "K-54 S $VERSION" -srcfolder "$DMGDIR" -ov -format UDZO "$DMG" >/dev/null

if [ -n "$K54_SIGN_APP" ]; then
  codesign --force --timestamp --sign "$K54_SIGN_APP" "$DMG"
  notarize "$DMG"
fi

echo ""
echo "Fertig:  $DMG"
echo "Doppelklick auf die DMG, dann auf 'K-54 S installieren.pkg'."
open -R "$DMG" 2>/dev/null || true

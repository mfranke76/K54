#!/bin/bash
# =============================================================================
#  K-54 Analyzer – erzeugt einen fertigen Mac-Installer:  K-54 Analyzer-<Version>-Installer.dmg
#  Inhalt der DMG: "K-54 Analyzer installieren.pkg" (installiert AU, VST3 und App)
#
#  Aufruf im Terminal:   ./make-installer.sh
#  Voraussetzung:        Xcode Command Line Tools + CMake (einmalig)
# =============================================================================
set -e
cd "$(dirname "$0")"
ROOT="$(pwd)"

echo ""
echo "  KELLER54 · K-54 Analyzer Installer"
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

VERSION=$(grep -m1 'project(K54A VERSION' CMakeLists.txt | sed -E 's/.*VERSION ([0-9.]+).*/\1/')
BUILD="$ROOT/build-installer"
ART="$BUILD/K54Analyzer_artefacts/Release"
STAGE="$BUILD/pkgroot"
DMGDIR="$BUILD/dmg"
DMG="$ROOT/K-54 Analyzer-$VERSION-Installer.dmg"

# ---------- Bauen (Universal: Apple Silicon + Intel)
echo "==> Kompilieren von K-54 Analyzer $VERSION (beim ersten Mal 5-10 Minuten)"
cmake -B "$BUILD" -DCMAKE_BUILD_TYPE=Release  >/dev/null
cmake --build "$BUILD" --config Release --target K54Analyzer_AU K54Analyzer_VST3 K54Analyzer_Standalone -j "$(sysctl -n hw.ncpu)"

for B in "AU/K-54 Analyzer.component" "VST3/K-54 Analyzer.vst3" "Standalone/K-54 Analyzer.app"; do
  if [ ! -d "$ART/$B" ]; then echo "Fehler: $ART/$B wurde nicht gebaut."; exit 1; fi
  codesign --force --deep --sign - "$ART/$B" >/dev/null 2>&1 || true   # Ad-hoc-Signatur (noetig fuer Apple Silicon)
done

# ---------- Paket-Inhalt zusammenstellen
echo "==> Installationspaket erstellen"
rm -rf "$STAGE" "$DMGDIR" "$BUILD/scripts" "$BUILD/res"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/Components" "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Applications"
ditto "$ART/AU/K-54 Analyzer.component"   "$STAGE/Library/Audio/Plug-Ins/Components/K-54 Analyzer.component"
ditto "$ART/VST3/K-54 Analyzer.vst3"      "$STAGE/Library/Audio/Plug-Ins/VST3/K-54 Analyzer.vst3"
ditto "$ART/Standalone/K-54 Analyzer.app" "$STAGE/Applications/K-54 Analyzer.app"

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
  rm -rf "$H/Library/Audio/Plug-Ins/Components/K-54 Analyzer.component" "$H/Library/Audio/Plug-Ins/VST3/K-54 Analyzer.vst3" 2>/dev/null
done
killall -9 AudioComponentRegistrar 2>/dev/null
exit 0
POST
chmod +x "$BUILD/scripts/postinstall"

pkgbuild --root "$STAGE" \
         --component-plist "$BUILD/component.plist" \
         --scripts "$BUILD/scripts" \
         --identifier "de.keller54.k54analyzer" \
         --version "$VERSION" \
         --install-location "/" \
         "$BUILD/K54A-core.pkg" >/dev/null

mkdir -p "$BUILD/res"
cp Installer/welcome.html Installer/conclusion.html "$BUILD/res/"
sed "s/@VERSION@/$VERSION/g" Installer/distribution.xml > "$BUILD/distribution.xml"

mkdir -p "$DMGDIR"
productbuild --distribution "$BUILD/distribution.xml" \
             --resources "$BUILD/res" \
             --package-path "$BUILD" \
             "$DMGDIR/K-54 Analyzer installieren.pkg" >/dev/null

cp Installer/Anleitung.txt "$DMGDIR/Anleitung.txt"
cp "Installer/K-54 Analyzer deinstallieren.command" "$DMGDIR/K-54 Analyzer deinstallieren.command"
chmod +x "$DMGDIR/K-54 Analyzer deinstallieren.command"

# ---------- DMG
echo "==> DMG erstellen"
rm -f "$DMG"
hdiutil create -volname "K-54 Analyzer $VERSION" -srcfolder "$DMGDIR" -ov -format UDZO "$DMG" >/dev/null

echo ""
echo "Fertig:  $DMG"
echo "Doppelklick auf die DMG, dann auf 'K-54 Analyzer installieren.pkg'."
open -R "$DMG" 2>/dev/null || true

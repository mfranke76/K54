#!/bin/bash
# =============================================================================
#  K-54 S – nur fuer DIESEN Mac bauen und installieren (zum Testen, nicht oeffentlich)
#  Doppelklick im Finder. Installiert fuer deinen Benutzer, kein Passwort noetig:
#    ~/Library/Audio/Plug-Ins/Components/K-54 S.component   (Logic / GarageBand)
#    ~/Library/Audio/Plug-Ins/VST3/K-54 S.vst3              (Komplete Kontrol / Ableton)
#    ~/Applications/K-54 S.app                              (ohne DAW)
# =============================================================================
cd "$(dirname "$0")"
ROOT="$(pwd)"
LOG="$ROOT/lokal-installieren.log"
exec > >(tee "$LOG") 2>&1
set -e

echo ""
echo "  KELLER54 · K-54 S · lokale Test-Installation"
echo "  ------------------------------------------"

# ---------- Werkzeuge
if ! xcode-select -p >/dev/null 2>&1; then
  echo "Die Xcode Command Line Tools fehlen – es oeffnet sich gleich ein Fenster zur Installation."
  echo "Danach diese Datei einfach noch einmal doppelklicken."
  xcode-select --install || true
  read -n 1 -s -r -p "Taste druecken zum Schliessen ..."; exit 1
fi
for C in /Applications/CMake.app/Contents/bin /opt/homebrew/bin /usr/local/bin "$ROOT/.tools/cmake/CMake.app/Contents/bin" "$ROOT/../K54/.tools/cmake/CMake.app/Contents/bin"; do
  command -v cmake >/dev/null 2>&1 && break
  [ -x "$C/cmake" ] && export PATH="$C:$PATH"
done
if ! command -v cmake >/dev/null 2>&1; then
  echo "==> CMake wird in den Projektordner geladen (einmalig)"
  mkdir -p "$ROOT/.tools"
  curl -fL --progress-bar -o "$ROOT/.tools/cmake.tar.gz" \
    "https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5-macos-universal.tar.gz"
  tar -xzf "$ROOT/.tools/cmake.tar.gz" -C "$ROOT/.tools"
  mv "$ROOT/.tools/cmake-3.30.5-macos-universal" "$ROOT/.tools/cmake"
  export PATH="$ROOT/.tools/cmake/CMake.app/Contents/bin:$PATH"
fi
# JUCE vom K-54 mitbenutzen, falls dort schon geladen (spart den Download)
if [ ! -d "$ROOT/JUCE" ] && [ -f "$ROOT/../K54/JUCE/CMakeLists.txt" ]; then
  ln -s "$ROOT/../K54/JUCE" "$ROOT/JUCE" 2>/dev/null || true
fi

# ---------- Bauen (nur fuer diesen Mac = schneller)
BUILD="$ROOT/build-local"
ART="$BUILD/K55_artefacts/Release"
echo "==> Kompilieren (beim ersten Mal 5–10 Minuten, danach deutlich schneller)"
cmake -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DK55_COPY_AFTER_BUILD=OFF -DK55_NATIVE_ONLY=ON >/dev/null
cmake --build "$BUILD" --config Release -j "$(sysctl -n hw.ncpu)"

for B in "AU/K-54 S.component" "VST3/K-54 S.vst3" "Standalone/K-54 S.app"; do
  [ -d "$ART/$B" ] || { echo "Fehler: $B wurde nicht gebaut (siehe $LOG)"; read -n 1 -s -r -p "Taste druecken ..."; exit 1; }
done

# ---------- Installieren (nur fuer dich)
echo "==> Installieren"
COMP="$HOME/Library/Audio/Plug-Ins/Components"; VST="$HOME/Library/Audio/Plug-Ins/VST3"; APPS="$HOME/Applications"
mkdir -p "$COMP" "$VST" "$APPS"
rm -rf "$COMP/K-54 S.component" "$VST/K-54 S.vst3" "$APPS/K-54 S.app"
# alter Arbeitsname K-55 weg
rm -rf "$COMP/K-55.component" "$VST/K-55.vst3" "$APPS/K-55.app"
ditto "$ART/AU/K-54 S.component"   "$COMP/K-54 S.component"
ditto "$ART/VST3/K-54 S.vst3"      "$VST/K-54 S.vst3"
ditto "$ART/Standalone/K-54 S.app" "$APPS/K-54 S.app"
for B in "$COMP/K-54 S.component" "$VST/K-54 S.vst3" "$APPS/K-54 S.app"; do
  xattr -cr "$B" 2>/dev/null || true
  codesign --force --deep --sign - "$B" >/dev/null 2>&1 || true
done

# ---------- Logic soll das neue Plugin sehen
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo "==> Audio-Unit-Test (auval)"
if auval -v aumu K5S1 Kl54 2>&1 | tail -3 | grep -q "AU VALIDATION SUCCEEDED"; then
  echo "    AU-Test: BESTANDEN"
else
  echo "    AU-Test: nicht bestanden oder noch nicht sichtbar – Details: auval -v aumu K5S1 Kl54"
fi

echo ""
echo "Fertig! K-54 S ist nur auf diesem Mac installiert:"
echo "  Logic Pro:          Software-Instrument > AU-Instrumente > Keller54 > K-54 S"
echo "  Komplete Kontrol:   Preferences > Plug-ins > Rescan, dann K-54 S laden"
echo "  Ohne DAW:           ~/Applications/K-54 S.app"
echo ""
echo "Wieder entfernen:  die drei Dateien oben loeschen (Components, VST3, Applications)."
echo ""
read -n 1 -s -r -p "Taste druecken zum Schliessen ..."

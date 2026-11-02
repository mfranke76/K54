#!/bin/bash
# K-54 – Build fuer macOS (VST3 + AU + Standalone)
# Aufruf im Terminal:  ./build-mac.sh
set -e
cd "$(dirname "$0")"

if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake fehlt. Installieren mit:  brew install cmake   (oder von cmake.org)"
  exit 1
fi
if ! xcode-select -p >/dev/null 2>&1; then
  echo "Xcode Command Line Tools fehlen. Installieren mit:  xcode-select --install"
  exit 1
fi

echo "==> Projekt einrichten (beim ersten Mal wird JUCE geladen, dauert etwas)"
cmake -B build -DCMAKE_BUILD_TYPE=Release

echo "==> Kompilieren"
cmake --build build --config Release -j "$(sysctl -n hw.ncpu)"

echo "==> Audio-Unit-Cache neu laden"
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo ""
echo "Fertig! Installiert nach:"
echo "  VST3: ~/Library/Audio/Plug-Ins/VST3/K-54.vst3"
echo "  AU:   ~/Library/Audio/Plug-Ins/Components/K-54.component"
echo "  App:  build/K54_artefacts/Release/Standalone/K-54.app"
echo ""
echo "AU-Test (optional):  auval -v aumu K54s Kl54"

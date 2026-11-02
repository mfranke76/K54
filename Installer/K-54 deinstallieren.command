#!/bin/bash
echo "K-54 wird entfernt (dein Mac-Passwort wird abgefragt)."
echo "Eigene Presets in ~/Music/Keller54 bleiben erhalten."
echo ""
sudo rm -rf "/Library/Audio/Plug-Ins/Components/K-54.component" \
            "/Library/Audio/Plug-Ins/VST3/K-54.vst3" \
            "/Applications/K-54.app"
rm -rf "$HOME/Library/Audio/Plug-Ins/Components/K-54.component" \
       "$HOME/Library/Audio/Plug-Ins/VST3/K-54.vst3"
sudo pkgutil --forget de.keller54.k54 >/dev/null 2>&1
killall -9 AudioComponentRegistrar 2>/dev/null
echo "Fertig. K-54 wurde entfernt."

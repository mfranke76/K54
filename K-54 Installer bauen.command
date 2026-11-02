#!/bin/bash
# Doppelklick im Finder: baut K-54 und erzeugt die Installer-DMG
cd "$(dirname "$0")"
./make-installer.sh 2>&1 | tee installer-build.log
echo ""
echo "Dieses Fenster kann geschlossen werden."

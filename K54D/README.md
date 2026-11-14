# K-54 D · Drumcomputer · Keller54 · by Frank E.

Viertes Plugin neben K-54, K-54 S und K-54 Analyzer. Gleicher Look (Schwarz/Gold, Regenbogenlinie, FARBIG/schlicht).

## Funktionen
- 12 Spuren (GM-Noten: Kick 36, Snare 38, Clap 39, Rim 37, CH 42, OH 46, Toms 41/45/48, Crash 49, Ride 51, Perc 56)
- Modelle pro Spur (`Source/DrumEngine.h`, `makeRecipe`): Bausteine Körper/Pitch-Hüllkurve, Rauschen, Klick, Metall (6 Rechtecke), Clap-Stöße, Glocke, Sättigung, Lo-Fi. Letztes Modell jeder Spur = Sample
- Regler pro Spur: Level, Tune, Decay, Tone, Snap, Drive, Pan, Room, Mute, Solo
- Sequencer (`PluginProcessor.cpp`): zustandslos aus der DAW-Position (ppq) berechnet → sample-genau, auch bei Sprüngen/Loops. Steps 8–32, Raster 1/8, 1/16, 1/32, 1/8T, 1/16T, Swing, Humanize, Wirbel 1–4x
- Pattern A–D (`Pattern.h`, atomar), Wechsel am Pattern-Ende, MIDI-Noten 24–27 = A–D
- Master: FAT (Low-Shelf 85 Hz + Sättigung), GLUE (Bus-Kompressor), ROOM (Hall-Send), Master-Gain, weicher Schutzbegrenzer
- Presets (`Presets.cpp`): 51 Werks-Presets, Kurzschrift `K:x...x...` (X x o - _ . 1–9, r R q = Wirbel); B/C/D werden aus A erzeugt (Fill, Break, Crash). Eigene Presets: `~/Music/Keller54/K-54 D Presets/*.k54dpreset`
- Samples werden im DAW-Projekt und in eigenen Presets mitgespeichert (16-Bit-WAV, base64)
- UPDATE-Knopf (`Updater.h`, product `k54d`), Fenster 70–150 %

## IDs
Bundle `de.keller54.k54d` · AU `aumu K5D1 Kl54` · Version in `CMakeLists.txt` (`project(K54D VERSION …)`)

## Bauen / Testen
- Linux-Tests: `cmake -B build -G Ninja -DK54D_BUILD_TESTS=ON && ninja -C build K54DTest && xvfb-run -a ./build/K54DTest_artefacts/Release/K54DTest out`
  (Modelle, DAW-Sync sample-genau, Swing, Triolen, Pattern-Wechsel, alle Presets inkl. WAVs, Zustand/Sample, MIDI-Export, Screenshot)
- Mac lokal: Doppelklick `K-54 D lokal installieren.command`
- Mac-Installer: `K-54 D Installer bauen.command` → DMG
- GitHub: `.github/workflows/k54d.yml` – Push auf `main` = Beta-Release `k54d-v<Version>`, Branch `k54d-*` = privater Entwurf `k54d-v<Version>-test`

# K-54 S · Keller54 · by Frank E.

Synthesizer-Plugin (AU / VST3 / App) – Analog (Ladder) · Wavetable · Sample.

## Bauen (Mac)
Doppelklick auf **"K-54 S Installer bauen.command"** → erzeugt `K-54 S-<Version>-Installer.dmg`.
(Beim ersten Mal lädt CMake/JUCE automatisch, 5–10 Minuten.)

## Funktionen
- 3 Oszillatoren: Dreieck / Säge / Rechteck / Puls / **Wavetable** / **Sample**
- 9 Werks-Wavetables + eigene (USER) – Serum-Format (2048 Samples pro Frame) oder aus einem Sample
- Sample: laden (Datei, Drag & Drop aus Finder/DAW), **Editor wie in einer DAW** (Zoom, Auswahl, Zuschneiden, Löschen, Fades, Normalisieren, Rückwärts, Undo), Lautstärke-Regler, **Aufnahme vom Audio-Eingang** (Mikro/Interface, Auto-Start), Start/Ende/Loop/Ping-Pong, Key-Track, One-Shot
- 4-Pol-Ladder-Filter (LP24/LP12/BP/HP), Unison bis 7, Sub, Heat, Mod-Matrix (inkl. WT-Position), Chorus, Delay, Reverb
- **Konzertflügel** (Oszillator-Typ PIANO, Salamander Grand V3 Samples, CC BY 3.0 – A. Holm)
- **Modular:** Mod-Matrix mit 8 Plätzen (14 Quellen inkl. Mod-Env, Pitch-Bend, Mod-Wheel, Aftertouch, Breath, Expression → 20 Ziele)
- **Wheels:** Pitch-/Mod-Wheel der Tastatur werden angezeigt und sind auf dem Bildschirm bedienbar; Bend Up/Down getrennt, Mod-Wheel/Aftertouch direkt auf Vibrato, Cutoff, WT-Position
- **Skalierbares Fenster** (70–150 %, Ecke ziehen oder Größenwahl), **ECO-Modus** für weniger CPU
- **Harmonizer:** Knopf HARMONIE an/aus, Tonart + Skala (leitereigen) oder fester Dur-/Moll-Akkord, Stimmen wählbar (Terz, Quinte, Septime, None, Oktave ±), Lage, Lautstärke
- Main-Pegelanzeige + Output-Gain neben dem Logo, Frank-E.-Porträt im Kopf
- Presets (Untermenüs je Kategorie): Piano (Klassik bis Pop), E-Piano & Keys, Orgel (Kirche, Drawbar, Harmonium), Vintage Mono (Moog-Art), Analog Poly (Diva-Art), Wavetable (Serum-Art), Sample-Vorlagen; eigene Presets mit Kategorie (inkl. Sample/Wavetable-Verweis)

## Ordner
- Eigene Presets: `~/Music/Keller54/K-54 S Presets/<Kategorie>/`
- Aufnahmen: `~/Music/Keller54/K-54 S Samples/`

## Test (Linux/Mac)
`cmake -B build -DK55_BUILD_TESTS=ON && cmake --build build --target K55Test && ./build/K55Test /tmp/k55test`

## GitHub (Windows + Mac automatisch)
Dieser Ordner liegt im Repo `mfranke76/K54` unter `K54S/`. Jede Änderung hier startet
„Actions › K-54 S bauen“ → `K-54-S-<Version>-Setup.exe` (Windows) und
`K-54-S-<Version>-Installer.dmg` (Mac), auch als Release `k54s-v<Version>`.
Neue Version: in `CMakeLists.txt` die Zeile `project(K55 VERSION …)` hochzählen.

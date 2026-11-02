# K-54 – Ladder-Synthesizer von Keller54 · by Frank E.

Analog-inspirierter Synthesizer im Geist der klassischen 70er-Monosynths.
Formate: **VST3, AU (Logic Pro) und Standalone-App** für macOS (Apple Silicon + Intel).

## Klang
- 3 Oszillatoren (Triangle, Saw, Square, Pulse), Range 32' bis 2', Tune ±7 Halbtöne
- Rauschgenerator, Pulsbreite, analoger Drift
- 24-dB-Ladder-Filter mit Emphasis (bis zur Selbstoszillation), Contour, Drive, Key Track
- Je eine ADSR-Hüllkurve für Filter und Lautstärke, Velocity auf Filter und Lautstärke
- LFO (Tri, Saw, Square, Sample & Hold) auf Tonhöhe und Filter, Modrad = Vibrato
- Mono, Legato (8 Stimmen) oder Poly, Glide, Pitch-Bend-Bereich, Sustain-Pedal
- 34 Werks-Presets (Bass, Lead, Brass, Pad, Keys, FX), eigene Presets speichern

## Looper (Reiter LOOPER)
4 Loop-Spuren, taktgenau und automatisch synchron. Jede Spur nimmt wahlweise den K-54, einen externen Audio-Eingang oder beides auf.

**Quelle pro Spur** (Auswahl oben in jeder Spur):
- **SYNTH** – nur der Klang des K-54
- **INPUT** – nur der Audio-Eingang (Mikrofon, Gitarre, Drumcomputer … über dein Audio-Interface)
- **SYNTH + IN** – beides zusammen in einer Spur

**INPUT-Bereich:**
- Pegelanzeige L/R (gelb = gut, orange = laut, rot = kurz vor Übersteuerung)
- **CHANNELS** – Stereo 1/2, Mono In 1, Mono In 2 oder Mono 1+2 (Mono landet auf beiden Seiten)
- **GAIN** – Eingangsverstärkung ±24 dB
- **MONITOR** – Eingang live mithören (in Logic aus lassen, Logic hört selbst mit)

**Wo kommt der Eingang her?**
- **K-54 App (Standalone):** Options › Audio/MIDI Settings › dein Audio-Interface als Input wählen. Beim ersten Start zeigt die App „Audio input is muted“ – auf **Unmute** klicken.
- **Logic Pro:** oben rechts im Plugin-Fenster das **Side Chain**-Menü öffnen und die Audiospur bzw. den Eingang wählen, den du loopen willst.
- Ohne Eingang zeigt der Bereich „KEIN EINGANG“ – die SYNTH-Spuren funktionieren trotzdem normal.

**Tempo:** Sobald der K-54 in einer DAW läuft, gilt **immer das DAW-Tempo** (und die Taktart) – auch wenn die DAW gerade steht und du im K-54 PLAY oder REC drückst. Der Tempo-Regler ist dann ausgeblendet („= DAW“). Nur in der K-54-App ohne DAW gilt der eigene Tempo-Regler.

**Taktquelle (wird automatisch gewählt):**
1. **DAW / Host** – in Logic oder Komplete Kontrol (Host-Transport) folgen die Loops Tempo, Taktart und Songposition. Stoppt die DAW, pausieren die Loops.
2. **MIDI CLOCK** – kommt eine externe MIDI-Clock an (z. B. von einem Drumcomputer in der Standalone-App), werden Tempo, Start, Stop und Continue übernommen.
3. **INTERN** – sonst läuft der eigene Takt mit dem Tempo-Regler; CLICK schaltet ein Metronom ein.

**Bedienung pro Spur (wie beim Looper-Pedal):**
- **REC** – Aufnahme startet am nächsten Taktanfang (Ring blinkt rot = ARMED). Läuft noch gar nichts, startet der interne Takt sofort mit Takt 1.
- Nach der eingestellten **Loop Length** (1, 2, 4 oder 8 Takte) wechselt die Spur automatisch auf **PLAY**.
- Haupttaste erneut: **OVERDUB** (orange) – weitere Schichten dazuspielen; nochmal: zurück auf PLAY.
- **STOP / START** – Spur stumm schalten bzw. wieder starten, **CLEAR** – Spur löschen.
- **STOP ALL / CLEAR ALL** für alle Spuren.
- Der Reiter LOOPER leuchtet rot, solange aufgenommen wird – auch wenn du gerade im SYNTH-Reiter Sounds schraubst.

**Fernsteuerung per MIDI-CC (Wert ≥ 64 = Taste):**

| CC | Funktion |
|---|---|
| 102–105 | Haupttaste Spur 1–4 (REC / OVERDUB / PLAY) |
| 106–109 | STOP/START Spur 1–4 |
| 110 | interner Takt Start/Stop |
| 111 | STOP ALL |

Hinweise: Die Loops werden (noch) nicht mit dem Projekt gespeichert. Maximale Loop-Länge 40 Sekunden pro Spur (8 Takte ab ca. 48 BPM). Ändert sich das Tempo nach der Aufnahme, wird der Loop passend gestreckt (wie ein Tape, Tonhöhe ändert sich mit).

## Drum-Computer (Reiter DRUMS)
8 analog modellierte Stimmen, komplett synthetisch: **Kick, Snare, Clap, Closed Hat, Open Hat, Tom Low, Tom High, Cowbell**.
- Pro Stimme: Level, Tune (±12 Halbtöne), Decay, Tone, Pan, **M** (stumm), **OUT** (eigener Ausgang)
- 16-Schritt-Raster: Klick = an → Akzent (heller, mit Punkt) → aus
- Closed Hat schneidet die Open Hat ab (wie bei den Klassikern)
- Per MIDI spielbar auf **Kanal 10** (General MIDI): 36 Kick, 38 Snare, 39 Clap, 42 Closed Hat, 46 Open Hat, 41/43/45 Tom Low, 47/48/50 Tom High, 56 Cowbell. Kanal 10 erreicht den Synth nicht.

## Step-Sequencer (DRUMS + SEQ)
- **8 Patterns A–H**, Wechsel jeweils am nächsten Takt, COPY kopiert ins nächste Pattern, CLEAR leert
- **PLAY** startet den internen Takt – in der DAW läuft der Sequencer automatisch mit dem Transport („DAW SYNC“), mit externer MIDI-Clock ebenso („MIDI SYNC“)
- Drum-Länge und Synth-Länge getrennt einstellbar (1–16 Schritte, für Polyrhythmen), **SWING** 50–80 %
- **SEQ** (Reiter): 16 Schritte für den K-54-Synth – Ton pro Schritt (ziehen, Mausrad, Doppelklick = Grundton), **ACCENT**, **SLIDE** (Legato; mit Glide am Synth gleitet der Ton wie bei einer 303), ROOT = Grundton, GATE = Notenlänge
- Patterns werden mit dem DAW-Projekt gespeichert; Synth-Presets ändern Drums/Sequencer nicht
- Ab Werk: A Funk, B Four-on-the-floor, C Breakbeat, D Acid – Drums und Synth-Sequencer sind beim Laden aus (DRUMS ON / SEQ ON)

## Einzelausgänge (Reiter OUTS)
14 Stereo-Ausgänge: **Main**, Synth, Kick, Snare, Clap, Closed Hat, Open Hat, Tom Low, Tom High, Cowbell, Loop 1–4.
Jede Quelle ist standardmäßig auf Main; **SEPARAT** (oder OUT bei den Drums) legt sie auf ihren eigenen Ausgang.
Ist der Ausgang in der DAW nicht aktiv, bleibt der Klang automatisch auf Main (roter Punkt im OUTS-Reiter).

- **Logic Pro:** Instrument als *Multi-Output* laden (AU-Instrumente › Keller54 › K-54 › Multi Output), im Mixer beim K-54 auf „+“ klicken
- **Ableton Live:** neue Audiospur, „Audio From“ = K-54-Spur, darunter den Ausgang wählen, Monitor „In“
- **Cubase:** im Instrument-Rack beim K-54 die Ausgänge aktivieren
- **Bitwig:** im Gerät beim K-54 bei den Ausgängen „+“ – jeder Ausgang wird eine eigene Spur

## Bauen auf dem Mac
1. **Xcode Command Line Tools:** im Terminal `xcode-select --install`
2. **CMake:** `brew install cmake` (Homebrew) oder Installer von cmake.org
3. Im Terminal in diesen Ordner wechseln und starten:
   ```
   cd ~/Downloads/K54
   chmod +x build-mac.sh
   ./build-mac.sh
   ```
   Beim ersten Mal lädt das Skript JUCE (ca. 100 MB) und baut 5–10 Minuten.
   Die Plugins werden automatisch in `~/Library/Audio/Plug-Ins/` installiert.

## In Logic Pro
Logic neu starten. K-54 erscheint unter **Instrument › AU-Instrumente › Keller54 › K-54**.

## In Komplete Kontrol mit dem A61
1. Komplete Kontrol öffnen › **Preferences › Plug-ins** › „Rescan“.
2. K-54 unter den Drittanbieter-Plugins (Keller54) aktivieren und laden.
3. Die 8 Drehregler des A61 sind seitenweise belegt:

| Seite | Regler 1–8 |
|---|---|
| 1 Performance | Cutoff · Emphasis · Contour · Drive · Attack · Decay · Release · Glide |
| 2 Filter-Kontur | F Attack · F Decay · F Sustain · F Release · Sustain · Key Track · Vel>Filter · Volume |
| 3 Oszillatoren | Osc1 Wave · Osc1 Range · Osc2 Wave · Osc2 Range · Osc2 Tune · Osc3 Wave · Osc3 Range · Osc3 Tune |
| 4 Mixer / LFO | Osc1 Level · Osc2 Level · Osc3 Level · Noise · Pulse Width · LFO Rate · LFO>Pitch · LFO>Filter |
| 5 Spielweise | LFO Wave · Voice Mode · Bend Range · Vel>Amp · Drift · Loop Tempo · Loop Length · Loop Click |
| 6 Looper | Loop 1–4 Volume · Loop 1–4 Source |
| 7 Input | Input Channels · Input Gain · Input Monitor |
| danach | Pattern, Sequencer, Drums (je Stimme Level/Tune/Decay/Tone/Pan/Mute/Out), Ausgänge |

Modrad am A61 = Vibrato, Pitch-Bend und Sustain-Pedal werden unterstützt.

## Presets
- Werks-Presets: Menü oben rechts, mit `<` `>` durchblättern
- Eigene Presets: **SAVE** › Name eingeben. Gespeichert unter `~/Music/Keller54/K-54 Presets/`
- Doppelklick auf einen Regler setzt ihn auf den Grundwert zurück

## Projektaufbau
- `Source/SynthEngine.h` – Klangerzeugung (Oszillatoren, Ladder-Filter, Stimmen)
- `Source/Parameters.h` – alle Parameter (Reihenfolge = A61-Belegung)
- `Source/Presets.cpp` – Werks-Presets
- `Source/Looper.h` – Looper-Engine mit Host-/MIDI-Clock-Sync (liefert auch den Takt für den Sequencer)
- `Source/Drums.h` – Drum-Computer (8 Stimmen), `Source/Sequencer.h` – Step-Sequencer mit 8 Patterns
- `Source/Updater.h`, `UpdatePanel.h` – UPDATE-Knopf (keller54.de)
- `Source/DrumPanel.h`, `SeqPanel.h`, `OutsPanel.h`, `SeqWidgets.h` – Reiter DRUMS / SEQ / OUTS
- `Source/PluginEditor.cpp`, `LooperPanel.h`, `LookAndFeel.h` – Oberfläche
- `Resources/fonts` – Schriften Syncopate (Apache 2.0) und Monoton (SIL OFL)

Gebaut mit JUCE 8. Für die kostenlose JUCE-Nutzung gelten deren Lizenzbedingungen (juce.com).

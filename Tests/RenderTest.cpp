// Offline-Test: spielt jedes Werks-Preset an, prueft auf Stille/Uebersteuerung/NaN,
// schreibt WAV-Beispiele und einen Screenshot der Oberflaeche.
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? argv[1] : "/tmp/k54test");
    outDir.createDirectory();

    std::unique_ptr<juce::AudioProcessor> base (createPluginFilter());
    auto& proc = dynamic_cast<K54Processor&> (*base);
    const double sr = 48000.0;
    const int block = 256;
    proc.setPlayConfigDetails (2, 2, sr, block);   // Stereo-Eingang + Stereo-Ausgang

    juce::WavAudioFormat wav;
    bool allOk = true;

    for (int pi = 0; pi < proc.getNumPrograms(); ++pi)
    {
        proc.setCurrentProgram (pi);
        proc.prepareToPlay (sr, block);
        const bool poly = juce::roundToInt (proc.apvts.getRawParameterValue ("voiceMode")->load()) == 2;

        const int total = (int) (sr * 3.5);
        juce::AudioBuffer<float> out (2, total);
        out.clear();

        // Ereignisse: poly -> Akkord, sonst kleine Linie
        std::vector<std::tuple<int, int, bool, float>> ev; // sample, note, on, vel
        auto at = [&] (double t) { return (int) (t * sr); };
        if (poly)
        {
            for (int n : { 48, 55, 60, 64 }) ev.push_back ({ at (0.05), n, true, 0.85f });
            for (int n : { 48, 55, 60, 64 }) ev.push_back ({ at (1.8),  n, false, 0.0f });
        }
        else
        {
            const int base = 36;
            const int line[] { 0, 12, 7, 10, 12, 3 };
            for (int i = 0; i < 6; ++i)
            {
                ev.push_back ({ at (0.05 + i * 0.28), base + line[i], true, i % 2 ? 0.6f : 1.0f });
                ev.push_back ({ at (0.05 + i * 0.28 + 0.24), base + line[i], false, 0.0f });
            }
        }

        juce::AudioBuffer<float> buf (2, block);
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = std::min (block, total - pos);
            buf.setSize (2, n, false, false, true);
            juce::MidiBuffer midi;
            for (auto& [s, note, on, vel] : ev)
                if (s >= pos && s < pos + n)
                    midi.addEvent (on ? juce::MidiMessage::noteOn (1, note, vel) : juce::MidiMessage::noteOff (1, note), s - pos);
            proc.processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, buf, ch, 0, n);
        }

        float peak = 0.0f; double sum = 0.0; bool nan = false;
        for (int i = 0; i < total; ++i)
        {
            const float v = out.getSample (0, i);
            if (! std::isfinite (v)) nan = true;
            peak = std::max (peak, std::abs (v));
            sum += (double) v * v;
        }
        const double rmsDb = juce::Decibels::gainToDecibels ((float) std::sqrt (sum / total));
        const bool ok = ! nan && peak > 0.02f && peak <= 1.0f;
        allOk &= ok;
        std::printf ("%-2d %-22s %-6s peak %.3f  rms %6.1f dB  %s\n", pi, proc.getProgramName (pi).toRawUTF8(),
                     poly ? "poly" : "mono", peak, rmsDb, ok ? "OK" : (nan ? "NAN!" : "PRUEFEN"));

        auto file = outDir.getChildFile (juce::String (pi).paddedLeft ('0', 2) + "_" + proc.getProgramName (pi).replaceCharacters (" &", "__") + ".wav");
        file.deleteFile();
        if (auto stream = std::unique_ptr<juce::OutputStream> (file.createOutputStream()))
            if (auto writer = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (stream.get(), sr, 2, 16, {}, 0)))
            {
                stream.release();
                writer->writeFromAudioSampleBuffer (out, 0, total);
            }
    }

    // State speichern/laden
    {
        proc.setCurrentProgram (3);
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        proc.setCurrentProgram (0);
        proc.setStateInformation (mb.getData(), (int) mb.getSize());
        std::printf ("State-Restore: %s\n", proc.getPresetName().toRawUTF8());
    }


    // ======================================================== LOOPER-TESTS
    struct TestPlayHead : juce::AudioPlayHead
    {
        double bpm = 100.0, sr = 48000.0; long long pos = 0; bool playing = true;
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo p;
            p.setIsPlaying (playing);
            p.setBpm (bpm);
            p.setPpqPosition ((double) pos / sr * bpm / 60.0);
            p.setTimeSignature (TimeSignature { 4, 4 });
            return p;
        }
    };

    auto renderSeconds = [&] (double secs, std::function<void (juce::MidiBuffer&, long long, int)> midiFn,
                              TestPlayHead* ph, juce::AudioBuffer<float>* capture) -> double
    {
        const int total = (int) (secs * sr);
        juce::AudioBuffer<float> buf (2, block);
        double sum = 0.0;
        static long long clock = 0;
        if (capture) capture->setSize (2, total);
        for (int p = 0; p < total; p += block)
        {
            const int n = std::min (block, total - p);
            buf.setSize (2, n, false, false, true);
            juce::MidiBuffer midi;
            if (midiFn) midiFn (midi, clock, n);
            proc.processBlock (buf, midi);
            if (ph) ph->pos += n;
            clock += n;
            for (int i = 0; i < n; ++i) sum += (double) buf.getSample (0, i) * buf.getSample (0, i);
            if (capture) for (int ch = 0; ch < 2; ++ch) capture->copyFrom (ch, p, buf, ch, 0, n);
        }
        return juce::Decibels::gainToDecibels ((float) std::sqrt (sum / total), -120.0f);
    };
    auto stateName = [] (LoopState s) { static const char* n[] { "Empty", "Armed", "Recording", "Playing", "Overdub", "Stopped" }; return n[(int) s]; };
    auto setP = [&] (const char* id, float v) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    bool loopOk = true;
    auto check = [&] (bool cond, const char* what) { std::printf ("  [%s] %s\n", cond ? "OK" : "FEHLER", what); loopOk &= cond; };

    // --- A: interner Takt, 1 Takt @120 BPM = 2 s
    {
        std::printf ("Looper A: interner Takt\n");
        proc.setPlayHead (nullptr);
        proc.setCurrentProgram (0);
        proc.prepareToPlay (sr, block);
        setP ("loopBpm", 120.0f); setP ("loopBars", 0);   // 1 Bar
        proc.looper.pressMain (0);
        juce::AudioBuffer<float> rec;
        // Riff im ersten Takt
        renderSeconds (2.0, [&] (juce::MidiBuffer& m, long long c, int n)
        {
            for (int k = 0; k < 4; ++k)
            {
                const long long on = (long long) (k * 0.5 * sr), off = on + (long long) (0.3 * sr);
                if (on >= c && on < c + n)   m.addEvent (juce::MidiMessage::noteOn (1, 36 + k * 3, 0.9f), (int) (on - c));
                if (off >= c && off < c + n) m.addEvent (juce::MidiMessage::noteOff (1, 36 + k * 3), (int) (off - c));
            }
        }, nullptr, &rec);
        // midiFn-Zeit startet bei clock, daher Riff relativ zum Gesamtzaehler -> nur erster Aufruf
        check (proc.looper.getState (0) == LoopState::Playing, "Spur 1 nach 1 Takt auf PLAY");
        check (std::abs (proc.looper.getBpm() - 120.0) < 0.01 && proc.looper.getSource() == ClockSource::Internal, "Quelle INTERN, 120 BPM");
        juce::AudioBuffer<float> play;
        const double db = renderSeconds (2.0, nullptr, nullptr, &play);
        std::printf ("  Wiedergabe-Pegel Takt 2: %.1f dB (Synth stumm)\n", db);
        check (db > -40.0, "Loop ist im zweiten Takt hoerbar");
        // Vergleich Aufnahme vs. Wiedergabe (Mitte des Takts, ohne Naht)
        double num = 0, d1 = 0, d2 = 0;
        for (int i = (int) (0.2 * sr); i < (int) (1.8 * sr); ++i)
        { const double a = rec.getSample (0, i), b = play.getSample (0, i); num += a * b; d1 += a * a; d2 += b * b; }
        const double corr = num / std::sqrt (d1 * d2 + 1e-12);
        std::printf ("  Korrelation Aufnahme/Wiedergabe: %.3f\n", corr);
        check (corr > 0.95, "Wiedergabe entspricht der Aufnahme (taktgenau)");

        // Overdub
        proc.looper.pressMain (0);
        renderSeconds (0.02, nullptr, nullptr, nullptr);
        check (proc.looper.getState (0) == LoopState::Overdub, "Haupttaste -> OVERDUB");
        proc.looper.pressMain (0);
        renderSeconds (0.02, nullptr, nullptr, nullptr);
        check (proc.looper.getState (0) == LoopState::Playing, "nochmal -> PLAY");
        proc.looper.pressStop (0);
        renderSeconds (0.1, nullptr, nullptr, nullptr);
        check (proc.looper.getState (0) == LoopState::Stopped, "STOP -> gestoppt");
        const double silent = renderSeconds (1.0, nullptr, nullptr, nullptr);
        check (silent < -80.0, "gestoppte Spur ist still");
        proc.looper.pressClear (0);
        renderSeconds (0.02, nullptr, nullptr, nullptr);
        check (proc.looper.getState (0) == LoopState::Empty, "CLEAR -> leer");
    }

    // --- B: DAW-Sync, 100 BPM, Spur wird mitten im Takt gedrueckt
    {
        std::printf ("Looper B: DAW / Host-Sync\n");
        TestPlayHead ph; ph.sr = sr; ph.bpm = 100.0;
        proc.setPlayHead (&ph);
        proc.prepareToPlay (sr, block);
        setP ("loopBars", 1);   // 2 Bars
        renderSeconds (60.0 / 100.0 * 1.5, nullptr, &ph, nullptr);     // bis Schlag 1.5
        proc.looper.pressMain (1);
        renderSeconds (0.05, nullptr, &ph, nullptr);
        check (proc.looper.getState (1) == LoopState::Armed, "mitten im Takt: Spur wartet (ARMED)");
        check (proc.looper.getSource() == ClockSource::Host && std::abs (proc.looper.getBpm() - 100.0) < 0.01, "Quelle DAW, 100 BPM");
        // bis kurz nach Schlag 4 (= Takt 2)
        const double toBar2 = (4.0 - 1.5) * 0.6 - 0.05 + 0.01;
        renderSeconds (toBar2, nullptr, &ph, nullptr);
        check (proc.looper.getState (1) == LoopState::Recording, "Aufnahme startet genau am Taktanfang");
        renderSeconds (2 * 4 * 0.6, nullptr, &ph, nullptr);   // 2 Takte
        check (proc.looper.getState (1) == LoopState::Playing && proc.looper.getLengthBars (1) == 2, "nach 2 Takten auf PLAY, Laenge 2 Bars");
        ph.playing = false;
        renderSeconds (0.1, nullptr, &ph, nullptr);
        check (! proc.looper.isRunning(), "DAW stoppt -> Looper pausiert");
        proc.looper.clearAll();
        renderSeconds (0.02, nullptr, &ph, nullptr);
        proc.setPlayHead (nullptr);
    }

    // --- C: externe MIDI-Clock 130 BPM
    {
        std::printf ("Looper C: MIDI-Clock\n");
        proc.prepareToPlay (sr, block);
        const double tickSamples = 60.0 * sr / (130.0 * 24.0);
        long long startClock = -1;
        double nextTick = 0.0;
        bool sentStart = false;
        auto clockFn = [&] (juce::MidiBuffer& m, long long c, int n)
        {
            if (startClock < 0) { startClock = c; nextTick = (double) c; }
            if (! sentStart) { m.addEvent (juce::MidiMessage::midiStart(), 0); sentStart = true; }
            while (nextTick < (double) (c + n))
            {
                m.addEvent (juce::MidiMessage::midiClock(), (int) (nextTick - (double) c));
                nextTick += tickSamples;
            }
        };
        renderSeconds (1.0, clockFn, nullptr, nullptr);
        std::printf ("  erkannt: %.2f BPM, Schlag %.2f\n", proc.looper.getBpm(), proc.looper.getBeat());
        check (proc.looper.getSource() == ClockSource::MidiClock, "Quelle MIDI CLOCK");
        check (std::abs (proc.looper.getBpm() - 130.0) < 0.5, "Tempo 130 BPM erkannt");
        check (std::abs (proc.looper.getBeat() - 130.0 / 60.0) < 0.1, "Songposition laeuft synchron");
        proc.looper.pressMain (2);
        renderSeconds (0.05, clockFn, nullptr, nullptr);
        check (proc.looper.getState (2) == LoopState::Armed, "Spur 3 wartet auf Taktanfang");
        renderSeconds (2.0, clockFn, nullptr, nullptr);
        check (proc.looper.getState (2) == LoopState::Recording, "Aufnahme laeuft im Clock-Takt");
        proc.looper.clearAll();
        renderSeconds (0.02, clockFn, nullptr, nullptr);
    }

    // --- D: Audio-Eingang (Audio-Interface) als Loop-Quelle
    {
        std::printf ("Looper D: Audio-Eingang\n");
        proc.setPlayHead (nullptr);
        proc.prepareToPlay (sr, block);
        setP ("loopBpm", 120.0f); setP ("loopBars", 0);        // 1 Takt = 2 s
        setP ("loopInMode", 0); setP ("loopInGain", 0.0f); setP ("loopMonitor", 0.0f);
        setP ("loop1Src", 1);   // nur Eingang
        setP ("loop2Src", 2);   // Synth + Eingang
        long long phase = 0;
        // Rendert mit Sinus (220 Hz L, 330 Hz R) am Eingang; optional Synth-Noten
        auto renderIn = [&] (double secs, float inLevel, bool notes, juce::AudioBuffer<float>* cap) -> double
        {
            const int total = (int) (secs * sr);
            juce::AudioBuffer<float> b (2, block);
            if (cap) cap->setSize (2, total);
            double sum = 0.0;
            for (int p = 0; p < total; p += block)
            {
                const int n = std::min (block, total - p);
                b.setSize (2, n, false, false, true);
                for (int i = 0; i < n; ++i)
                {
                    const double t = (double) (phase + i) / sr;
                    b.setSample (0, i, inLevel * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * t));
                    b.setSample (1, i, inLevel * (float) std::sin (juce::MathConstants<double>::twoPi * 330.0 * t));
                }
                juce::MidiBuffer midi;
                if (notes && p == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 48, 0.9f), 0);
                if (notes && p + n >= total) midi.addEvent (juce::MidiMessage::noteOff (1, 48), n - 1);
                proc.processBlock (b, midi);
                phase += n;
                for (int i = 0; i < n; ++i) sum += (double) b.getSample (0, i) * b.getSample (0, i);
                if (cap) for (int ch = 0; ch < 2; ++ch) cap->copyFrom (ch, p, b, ch, 0, n);
            }
            return juce::Decibels::gainToDecibels ((float) std::sqrt (sum / total), -120.0f);
        };

        // Spur 1 (INPUT): Eingang aufnehmen, Synth spielt dabei mit -> darf NICHT im Loop landen
        proc.looper.pressMain (0);
        renderIn (2.02, 0.3f, true, nullptr);
        check (proc.looper.getState (0) == LoopState::Playing, "Spur 1 hat den Eingang aufgenommen");
        check (proc.inPeak[0].load() > 0.1f, "Eingangspegel wird angezeigt");
        // Wiedergabe ohne Eingang und ohne Synth
        juce::AudioBuffer<float> playback;
        const double db1 = renderIn (1.9, 0.0f, false, &playback);
        // Frequenzpruefung: Korrelation mit 220 Hz links / 330 Hz rechts (phasenunabhaengig)
        auto toneCorr = [&] (const juce::AudioBuffer<float>& a, int ch, double f)
        {
            double c = 0, sN = 0, e = 0;
            for (int i = (int) (0.1 * sr); i < (int) (1.8 * sr); ++i)
            {
                const double w = juce::MathConstants<double>::twoPi * f * i / sr;
                c += a.getSample (ch, i) * std::cos (w); sN += a.getSample (ch, i) * std::sin (w);
                e += (double) a.getSample (ch, i) * a.getSample (ch, i);
            }
            const int len = (int) (1.7 * sr);
            return std::sqrt ((c * c + sN * sN) * 2.0 / len) / std::sqrt (e + 1e-12);
        };
        const double cL = toneCorr (playback, 0, 220.0), cR = toneCorr (playback, 1, 330.0);
        std::printf ("  Loop 1 Wiedergabe %.1f dB, Ton-Anteil L %.3f / R %.3f\n", db1, cL, cR);
        check (db1 > -20.0, "Loop 1 spielt den aufgenommenen Eingang");
        check (cL > 0.97 && cR > 0.97, "Loop 1 enthaelt nur den Eingang (Stereo L/R korrekt, kein Synth)");

        // Spur 2 (SYNTH + IN): beides
        proc.looper.pressStop (0);
        proc.looper.pressMain (1);
        renderIn (0.11, 0.0f, false, nullptr);   // bis zum naechsten Takt
        check (proc.looper.getState (1) == LoopState::Recording && std::abs (proc.looper.getProgress (1) - 0.015f) < 0.004f,
               "Spur 2 startet exakt am Taktanfang (auch genau auf Blockgrenze)");
        // bis Taktanfang (Takt 3 bei 4 s), dann 1 Takt mit Synth + Eingang
        // Taktanfang (4,0 s) liegt genau auf einer Blockgrenze -> Aufnahme muss dort starten
        renderIn (2.0, 0.3f, true, nullptr);
        renderIn (0.05, 0.0f, false, nullptr);
        check (proc.looper.getState (1) == LoopState::Playing, "Spur 2 hat aufgenommen");
        juce::AudioBuffer<float> pb2;
        renderIn (1.9, 0.0f, false, &pb2);
        const double c2 = toneCorr (pb2, 0, 220.0);
        std::printf ("  Loop 2 Ton-Anteil L %.3f (Rest = Synth)\n", c2);
        check (c2 > 0.15 && c2 < 0.95, "Loop 2 enthaelt Eingang UND Synth");

        // Monitor: Eingang hoerbar ohne Aufnahme
        proc.looper.clearAll();
        setP ("loopMonitor", 1.0f);
        const double dbMon = renderIn (0.5, 0.3f, false, nullptr);
        setP ("loopMonitor", 0.0f);
        const double dbOff = renderIn (0.5, 0.3f, false, nullptr);
        std::printf ("  Monitor an %.1f dB / aus %.1f dB\n", dbMon, dbOff);
        check (dbMon > -20.0 && dbOff < -80.0, "MONITOR schaltet den Eingang hoerbar / stumm");

        // Mono In 2 auf beide Seiten
        setP ("loopInMode", 2);
        renderIn (0.1, 0.3f, false, nullptr);
        setP ("loopMonitor", 1.0f);
        juce::AudioBuffer<float> mono;
        renderIn (1.9, 0.3f, false, &mono);
        setP ("loopMonitor", 0.0f);
        check (toneCorr (mono, 0, 330.0) > 0.97 && toneCorr (mono, 1, 330.0) > 0.97, "MONO IN 2 liegt auf beiden Seiten");
        setP ("loopInMode", 0);
        setP ("loop1Src", 0); setP ("loop2Src", 0);
        proc.looper.clearAll();
        renderIn (0.05, 0.0f, false, nullptr);
    }
    std::printf (loopOk ? "LOOPER OK\n" : "LOOPER: FEHLER\n");
    allOk &= loopOk;


    // ======================================================== DRUMS / SEQUENCER / EINZELAUSGAENGE
    {
        std::printf ("Drums & Sequencer\n");
        bool dOk = true;
        auto chk = [&] (bool c, const char* what) { std::printf ("  [%s] %s\n", c ? "OK" : "FEHLER", what); dOk &= c; };
        proc.setPlayHead (nullptr);
        proc.prepareToPlay (sr, block);

        // --- 1) jede Drum-Stimme per MIDI Kanal 10
        for (int lane = 0; lane < DrumInfo::numLanes; ++lane)
        {
            juce::AudioBuffer<float> b (2, block);
            double sum = 0; float pk = 0; bool nan = false;
            for (int blk = 0; blk < (int) (0.6 * sr / block); ++blk)
            {
                b.clear();
                juce::MidiBuffer m;
                if (blk == 0) m.addEvent (juce::MidiMessage::noteOn (10, DrumInfo::laneNote[lane], 1.0f), 0);
                proc.processBlock (b, m);
                for (int i = 0; i < block; ++i) { const float v = b.getSample (0, i); if (! std::isfinite (v)) nan = true; pk = std::max (pk, std::abs (v)); sum += v * v; }
            }
            std::printf ("    %-10s peak %.2f\n", DrumInfo::names[lane], pk);
            chk (! nan && pk > 0.05f && pk <= 1.0f, (juce::String (DrumInfo::names[lane]) + " klingt (MIDI Kanal 10)").toRawUTF8());
        }

        // --- 2) Kanal 10 erreicht den Synth nicht
        {
            juce::AudioBuffer<float> b (2, block);
            proc.prepareToPlay (sr, block);
            float pk = 0;
            for (int blk = 0; blk < 40; ++blk)
            {
                b.clear(); juce::MidiBuffer m;
                if (blk == 0) m.addEvent (juce::MidiMessage::noteOn (10, 60, 1.0f), 0);   // Note 60 ist keine Drum
                proc.processBlock (b, m);
                pk = std::max (pk, b.getMagnitude (0, 0, block));
            }
            chk (pk < 1e-4f, "Kanal-10-Noten ohne Drum-Zuordnung bleiben still (kein Synth)");
        }

        // --- 3) Sequencer-Timing: Pattern A, nur Kick, interner Takt 120 BPM
        proc.prepareToPlay (sr, block);
        setP ("loopBpm", 120.0f); setP ("seqPattern", 0); setP ("seqSwing", 0.0f); setP ("seqDrumsOn", 1.0f);
        for (int l = 1; l < 8; ++l) setP (ParamID::drum (l, "Mute").toRawUTF8(), 1.0f);
        setP (ParamID::drum (0, "Decay").toRawUTF8(), 0.0f);     // kurze Kick, damit die Fahne nicht stoert
        proc.looper.pressTransportStart();
        const int total = (int) (2.0 * sr);   // 1 Takt
        juce::AudioBuffer<float> rec (2, total);
        {
            juce::AudioBuffer<float> b (2, block);
            for (int pos = 0; pos < total; pos += block)
            {
                const int n = std::min (block, total - pos);
                b.setSize (2, n, false, false, true); b.clear();
                juce::MidiBuffer m; proc.processBlock (b, m);
                rec.copyFrom (0, pos, b, 0, 0, n);
            }
        }
        // Pattern A Kick: "X..x..x...X..x.." -> Schritte 0,3,6,10,13 -> Sekunden 0, .375, .75, 1.25, 1.625
        const double expect[] { 0.0, 0.375, 0.75, 1.25, 1.625 };
        int found = 0; double maxErrMs = 0;
        for (double e : expect)
        {
            const int c = (int) (e * sr);
            int onset = -1;
            for (int i = juce::jmax (0, c - 100); i < juce::jmin (total, c + 2000); ++i)
                if (std::abs (rec.getSample (0, i)) > 0.05f) { onset = i; break; }
            if (onset >= 0) { ++found; maxErrMs = std::max (maxErrMs, std::abs (onset - c) * 1000.0 / sr); }
        }
        std::printf ("    Kick-Schlaege gefunden %d/5, max. Abweichung %.2f ms\n", found, maxErrMs);
        chk (found == 5 && maxErrMs < 1.0, "Drum-Sequencer trifft die 16tel exakt (Pattern A)");
        // keine Schlaege dazwischen: Schritt 9 (1.125 s) ist leer, vorige Kick ist dort abgeklungen
        {
            const int c = (int) (1.125 * sr);
            float after = 0;
            for (int i = c; i < c + 1000; ++i) after = std::max (after, std::abs (rec.getSample (0, i)));
            std::printf ("    Pegel auf leerem Schritt 9: %.3f\n", after);
            chk (after < 0.04f, "kein Schlag auf leerem Schritt");
        }
        proc.looper.pressTransportStop();
        setP (ParamID::drum (0, "Decay").toRawUTF8(), 0.4f);
        { juce::AudioBuffer<float> b (2, block); juce::MidiBuffer m; b.clear(); proc.processBlock (b, m); }

        // --- 4) Synth-Sequencer spielt Noten
        for (int l = 0; l < 8; ++l) setP (ParamID::drum (l, "Mute").toRawUTF8(), 0.0f);
        setP ("seqDrumsOn", 0.0f); setP ("seqSynthOn", 1.0f);
        proc.setCurrentProgram (0);    // Keller Classic Bass
        proc.prepareToPlay (sr, block);
        proc.looper.pressTransportStart();
        double synthSum = 0;
        juce::AudioBuffer<float> seqOut (2, (int) (4.0 * sr));
        {
            juce::AudioBuffer<float> b (2, block);
            for (int pos = 0; pos < seqOut.getNumSamples(); pos += block)
            {
                const int n = std::min (block, seqOut.getNumSamples() - pos);
                b.setSize (2, n, false, false, true); b.clear();
                juce::MidiBuffer m; proc.processBlock (b, m);
                seqOut.copyFrom (0, pos, b, 0, 0, n); seqOut.copyFrom (1, pos, b, 1, 0, n);
                for (int i = 0; i < n; ++i) synthSum += b.getSample (0, i) * b.getSample (0, i);
            }
        }
        const double sdb = juce::Decibels::gainToDecibels ((float) std::sqrt (synthSum / seqOut.getNumSamples()), -120.0f);
        std::printf ("    Synth-Sequencer Pegel %.1f dB\n", sdb);
        chk (sdb > -40.0, "Synth-Sequencer spielt die Bassline");
        proc.looper.pressTransportStop();
        { juce::AudioBuffer<float> b (2, block); juce::MidiBuffer m; b.clear(); proc.processBlock (b, m); proc.processBlock (b, m); }
        // nach Stop keine haengenden Noten
        {
            juce::AudioBuffer<float> b (2, block); float pk = 0;
            for (int blk = 0; blk < (int) (1.5 * sr / block); ++blk) { b.clear(); juce::MidiBuffer m; proc.processBlock (b, m); }
            for (int blk = 0; blk < 10; ++blk) { b.clear(); juce::MidiBuffer m; proc.processBlock (b, m); pk = std::max (pk, b.getMagnitude (0, 0, block)); }
            chk (pk < 1e-3f, "nach Stop keine haengenden Noten");
        }
        setP ("seqSynthOn", 0.0f);

        // Hoerprobe Drums + Bass (Pattern A, 4 Takte, 100 BPM)
        {
            proc.prepareToPlay (sr, block);
            setP ("loopBpm", 100.0f); setP ("seqDrumsOn", 1.0f); setP ("seqSynthOn", 1.0f); setP ("seqSwing", 0.12f);
            proc.looper.pressTransportStart();
            const int len = (int) (4 * 4 * 60.0 / 100.0 * sr);
            juce::AudioBuffer<float> demo (2, len), b (2, block);
            for (int pos = 0; pos < len; pos += block)
            {
                const int n = std::min (block, len - pos);
                b.setSize (2, n, false, false, true); b.clear();
                juce::MidiBuffer m;
                if (pos >= len / 2 && pos - block < len / 2) setP ("seqPattern", 3);   // Wechsel auf D
                proc.processBlock (b, m);
                demo.copyFrom (0, pos, b, 0, 0, n); demo.copyFrom (1, pos, b, 1, 0, n);
            }
            auto f = outDir.getChildFile ("drums_seq_demo.wav"); f.deleteFile();
            if (auto st = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
                if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (st.get(), sr, 2, 16, {}, 0)))
                { st.release(); w->writeFromAudioSampleBuffer (demo, 0, len); }
            chk (demo.getMagnitude (0, len) > 0.1f && demo.getMagnitude (0, len) <= 1.0f, "Demo Drums + Bass ohne Uebersteuerung");
            proc.looper.pressTransportStop();
            setP ("seqDrumsOn", 0.0f); setP ("seqSynthOn", 0.0f); setP ("seqSwing", 0.0f); setP ("seqPattern", 0);
            { juce::AudioBuffer<float> bb (2, block); juce::MidiBuffer m; bb.clear(); proc.processBlock (bb, m); }
        }

        // --- 5) Einzelausgaenge: alle Busse aktiv, Kick + Synth separat
        {
            proc.enableAllBuses();
            proc.prepareToPlay (sr, block);
            const int nch = juce::jmax (proc.getTotalNumInputChannels(), proc.getTotalNumOutputChannels());
            std::printf ("    Busse: %d Ausgaenge, %d Kanaele\n", proc.getBusCount (false), proc.getTotalNumOutputChannels());
            chk (proc.getBusCount (false) == K54Processor::numOutBuses && proc.getTotalNumOutputChannels() == 28, "14 Stereo-Ausgaenge (28 Kanaele)");
            setP (ParamID::drum (0, "Out").toRawUTF8(), 1.0f);
            setP ("outSynth", 1.0f);
            juce::AudioBuffer<float> b (nch, block);
            float mainPk = 0, kickPk = 0, snarePk = 0, synthPk = 0, synthMainPk = 0;
            for (int blk = 0; blk < 60; ++blk)
            {
                b.clear(); juce::MidiBuffer m;
                if (blk == 0) { m.addEvent (juce::MidiMessage::noteOn (10, 36, 1.0f), 0); m.addEvent (juce::MidiMessage::noteOn (1, 48, 1.0f), 0); }
                if (blk == 30) m.addEvent (juce::MidiMessage::noteOn (10, 38, 1.0f), 0);
                proc.processBlock (b, m);
                auto mainB  = proc.getBusBuffer (b, false, 0);
                auto kickB  = proc.getBusBuffer (b, false, K54Processor::busDrum0);
                auto snareB = proc.getBusBuffer (b, false, K54Processor::busDrum0 + 1);
                auto synB   = proc.getBusBuffer (b, false, K54Processor::busSynth);
                if (blk < 30) mainPk = std::max (mainPk, mainB.getMagnitude (0, block));
                else          synthMainPk = std::max (synthMainPk, mainB.getMagnitude (0, block));
                kickPk  = std::max (kickPk,  kickB.getMagnitude (0, block));
                snarePk = std::max (snarePk, snareB.getMagnitude (0, block));
                synthPk = std::max (synthPk, synB.getMagnitude (0, block));
            }
            std::printf ("    Main %.3f | Kick-Out %.3f | Synth-Out %.3f | Snare-Out %.3f | Main (Snare) %.3f\n", mainPk, kickPk, synthPk, snarePk, synthMainPk);
            chk (kickPk > 0.1f && synthPk > 0.05f, "Kick und Synth liegen auf ihren eigenen Ausgaengen");
            chk (mainPk < 1e-4f, "...und nicht mehr auf Main");
            chk (snarePk < 1e-4f && synthMainPk > 0.05f, "Snare (nicht separat) bleibt auf Main");
            setP (ParamID::drum (0, "Out").toRawUTF8(), 0.0f); setP ("outSynth", 0.0f);
            // Bus deaktiviert -> automatisch zurueck auf Main
            setP (ParamID::drum (0, "Out").toRawUTF8(), 1.0f);
            auto layout = proc.getBusesLayout();
            for (int i = 1; i < layout.outputBuses.size(); ++i) layout.outputBuses.getReference (i) = juce::AudioChannelSet::disabled();
            chk (proc.setBusesLayout (layout), "Einzelausgaenge abschaltbar (nur Stereo-Main)");
            proc.prepareToPlay (sr, block);
            juce::AudioBuffer<float> b2 (juce::jmax (proc.getTotalNumInputChannels(), proc.getTotalNumOutputChannels()), block);
            float pk2 = 0;
            for (int blk = 0; blk < 20; ++blk)
            {
                b2.clear(); juce::MidiBuffer m;
                if (blk == 0) m.addEvent (juce::MidiMessage::noteOn (10, 36, 1.0f), 0);
                proc.processBlock (b2, m);
                pk2 = std::max (pk2, b2.getMagnitude (0, 0, block));
            }
            chk (pk2 > 0.1f, "Kick 'separat' ohne aktiven Ausgang faellt auf Main zurueck (nichts geht verloren)");
            setP (ParamID::drum (0, "Out").toRawUTF8(), 0.0f);
        }

        // --- 6) Patterns werden im Zustand gespeichert
        {
            proc.sequencer.setDrum (5, 2, 7, StepSequencer::StepAccent);
            proc.sequencer.setNote (5, 3, 19);
            juce::MemoryBlock mb; proc.getStateInformation (mb);
            proc.sequencer.clearPattern (5);
            proc.setStateInformation (mb.getData(), (int) mb.getSize());
            chk (proc.sequencer.getDrum (5, 2, 7) == StepSequencer::StepAccent && proc.sequencer.getNote (5, 3) == 19, "Patterns werden mit dem Projekt gespeichert");
        }

        // --- 7) DAW steht, PLAY im K-54: Tempo und Taktart kommen trotzdem von der DAW
        {
            TestPlayHead ph; ph.sr = sr; ph.bpm = 93.0; ph.playing = false;
            proc.setPlayHead (&ph);
            proc.prepareToPlay (sr, block);
            setP ("loopBpm", 140.0f);                       // interner Wert soll ignoriert werden
            setP ("seqDrumsOn", 1.0f); setP ("seqPattern", 1); // B: Kick auf jeder Viertel
            for (int l = 1; l < 8; ++l) setP (ParamID::drum (l, "Mute").toRawUTF8(), 1.0f);
            setP (ParamID::drum (0, "Decay").toRawUTF8(), 0.0f);
            proc.looper.pressTransportStart();
            const int total = (int) (sr * 60.0 / 93.0 * 4.0);
            juce::AudioBuffer<float> rec2 (2, total), b (2, block);
            for (int pos = 0; pos < total; pos += block)
            {
                const int n = std::min (block, total - pos);
                b.setSize (2, n, false, false, true); b.clear();
                juce::MidiBuffer m; proc.processBlock (b, m);
                rec2.copyFrom (0, pos, b, 0, 0, n);
            }
            std::printf ("    DAW steht: Tempo %.1f BPM, %d Schlaege/Takt, Quelle %s\n", proc.looper.getBpm(), proc.looper.getBeatsPerBar(),
                         proc.looper.usesHostTempo() ? "DAW-Tempo" : "intern");
            chk (std::abs (proc.looper.getBpm() - 93.0) < 0.01 && proc.looper.usesHostTempo(), "PLAY bei gestoppter DAW laeuft im DAW-Tempo (93 statt 140 BPM)");
            double maxErr = 0; int hitsFound = 0;
            for (int q = 0; q < 4; ++q)
            {
                const int c = (int) (q * 60.0 / 93.0 * sr);
                for (int i = juce::jmax (0, c - 100); i < juce::jmin (total, c + 2000); ++i)
                    if (std::abs (rec2.getSample (0, i)) > 0.05f) { ++hitsFound; maxErr = std::max (maxErr, std::abs (i - c) * 1000.0 / sr); break; }
            }
            std::printf ("    Kick auf den Vierteln bei 93 BPM: %d/4, max. Abweichung %.2f ms\n", hitsFound, maxErr);
            chk (hitsFound == 4 && maxErr < 1.0, "Sequencer-Viertel liegen exakt im DAW-Tempo");
            proc.looper.pressTransportStop();
            { juce::MidiBuffer m; b.clear(); proc.processBlock (b, m); }
            for (int l = 0; l < 8; ++l) setP (ParamID::drum (l, "Mute").toRawUTF8(), 0.0f);
            setP (ParamID::drum (0, "Decay").toRawUTF8(), 0.4f);
            setP ("seqDrumsOn", 0.0f); setP ("seqPattern", 0); setP ("loopBpm", 120.0f);
            proc.setPlayHead (nullptr);
        }
        std::printf (dOk ? "DRUMS OK\n" : "DRUMS: FEHLER\n");
        allOk &= dOk;
    }

    // Looper-Screenshot vorbereiten: interner Takt, 2 Spuren belegt
    {
        proc.prepareToPlay (sr, block);
        setP ("loopBars", 0);
        proc.looper.pressMain (0);
        renderSeconds (2.05, [&] (juce::MidiBuffer& m, long long, int) { static bool on = false; if (! on) { m.addEvent (juce::MidiMessage::noteOn (1, 40, 0.9f), 0); on = true; } }, nullptr, nullptr);
        proc.looper.pressMain (1);
        renderSeconds (2.6, nullptr, nullptr, nullptr);
        proc.looper.pressMain (2);
        renderSeconds (0.5, nullptr, nullptr, nullptr);
        setP ("loop2Src", 1); setP ("loop3Src", 2);
        proc.looper.pressMain (0);   // Overdub auf Spur 1
        renderSeconds (0.3, nullptr, nullptr, nullptr);
    }

    // Screenshot der Oberflaeche
    {
        proc.setCurrentProgram (8);
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        ed->setBounds (0, 0, ed->getWidth(), ed->getHeight());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
        juce::PNGImageFormat png;
        juce::FileOutputStream fos (outDir.getChildFile ("gui.png"));
        fos.setPosition (0); fos.truncate();
        png.writeImageToStream (img, fos);
        if (auto* k54 = dynamic_cast<K54Editor*> (ed.get()))
        {
            k54->setPage (1);
            auto img2 = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
            juce::FileOutputStream fos2 (outDir.getChildFile ("gui_looper.png"));
            fos2.setPosition (0); fos2.truncate();
            png.writeImageToStream (img2, fos2);
            setP ("seqDrumsOn", 1.0f); setP ("seqSynthOn", 1.0f);
            const char* names[] { "", "", "gui_drums.png", "gui_seq.png", "gui_outs.png" };
            for (int pg = 2; pg <= 4; ++pg)
            {
                k54->setPage (pg);
                auto im = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
                juce::FileOutputStream fo (outDir.getChildFile (names[pg]));
                fo.setPosition (0); fo.truncate();
                png.writeImageToStream (im, fo);
            }
        }
        std::printf ("GUI %dx%d gespeichert\n", ed->getWidth(), ed->getHeight());
    }

    std::printf (allOk ? "ALLE PRESETS OK\n" : "EINIGE PRESETS PRUEFEN\n");
    return allOk ? 0 : 1;
}

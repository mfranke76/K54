// K-54 D Offline-Tests:  K54DTest <Ausgabeordner> [schnell]
//  - alle Modelle aller Spuren: klingen, keine NaN, keine Uebersteuerung
//  - alle Werks-Presets 4 Takte im DAW-Tempo, Pegel und Treffer pruefen, WAVs schreiben
//  - DAW-Sync: Schlaege sample-genau an der richtigen Stelle (auch bei krummen Blockgrenzen, Swing, Triolen)
//  - Pattern-Wechsel am Pattern-Ende, Zustand speichern/laden (inkl. Sample), MIDI-Export
//  - Screenshot der Oberflaeche
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

static int failures = 0, checks = 0;
#define CHECK(cond, msg) do { ++checks; if (! (cond)) { ++failures; std::cout << "  FEHLER: " << msg << std::endl; } } while (0)

struct FakeHead : public juce::AudioPlayHead
{
    double bpm = 120.0, ppq = 0.0; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm); p.setPpqPosition (ppq); p.setIsPlaying (playing);
        p.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
        return p;
    }
};

static void setParam (K54DProcessor& p, const juce::String& id, float v)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

/** Nachklang abwarten (DAW gestoppt) */
static void flush (K54DProcessor& p, FakeHead& head)
{
    const bool was = head.playing;
    head.playing = false;
    juce::MidiBuffer midi;
    for (int i = 0; i < 200; ++i) { juce::AudioBuffer<float> b (2, 512); p.processBlock (b, midi); }
    head.playing = was;
}

/** rendert 'seconds' mit laufender DAW ab ppqStart; Bloecke wechselnd gross (krumme Grenzen) */
static juce::AudioBuffer<float> render (K54DProcessor& p, FakeHead& head, double seconds, double sr = 48000.0, double ppqStart = 0.0)
{
    const int total = (int) (seconds * sr);
    juce::AudioBuffer<float> out (2, total);
    out.clear();
    head.ppq = ppqStart;
    const int sizes[] { 512, 300, 1024, 77, 480, 256 };
    int pos = 0, k = 0;
    juce::MidiBuffer midi;
    while (pos < total)
    {
        const int n = juce::jmin (sizes[k++ % 6], total - pos);
        juce::AudioBuffer<float> b (2, n);
        p.processBlock (b, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, b, c, 0, n);
        pos += n;
        head.ppq += n / sr * head.bpm / 60.0;
    }
    return out;
}

static void writeWav (const juce::AudioBuffer<float>& b, const juce::File& f, double sr = 48000.0)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    auto* os = new juce::FileOutputStream (f);
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os, sr, 2, 24, {}, 0));
    if (w) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

static bool finite (const juce::AudioBuffer<float>& b)
{
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (b.getSample (c, i))) return false;
    return true;
}

/** Einsatzzeiten: Stellen, an denen nach Stille (>= gap) der Pegel ueber thr springt */
static std::vector<int> onsets (const juce::AudioBuffer<float>& b, float thr = 0.02f, int gap = 2000)
{
    std::vector<int> o;
    int quiet = gap;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float a = juce::jmax (std::abs (b.getSample (0, i)), std::abs (b.getSample (1, i)));
        if (a > thr) { if (quiet >= gap) o.push_back (i); quiet = 0; }
        else ++quiet;
    }
    return o;
}

static void cleanSlate (K54DProcessor& p)
{
    p.loadFactory (-1);   // Init: leer
    setParam (p, PID::fat, 0.0f); setParam (p, PID::glue, 0.0f); setParam (p, PID::roomSize, 0.0f);
    setParam (p, PID::humanize, 0.0f); setParam (p, PID::swing, 0.0f);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File out (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]) : juce::File::getCurrentWorkingDirectory().getChildFile ("out"));
    const bool quick = argc > 2 && juce::String (argv[2]) == "schnell";
    out.createDirectory();
    const double sr = 48000.0;

    K54DProcessor proc;
    FakeHead head;
    proc.setPlayHead (&head);
    proc.setPlayConfigDetails (0, 2, sr, 512);
    proc.prepareToPlay (sr, 1024);

    // ------------------------------------------------------------------ 1) alle Modelle
    std::cout << "1) Modelle aller Spuren" << std::endl;
    {
        cleanSlate (proc);
        setParam (proc, PID::seqOn, 0.0f);
        head.playing = false;
        juce::AudioBuffer<float> all (2, 0);
        for (int l = 0; l < DrumInfo::numLanes; ++l)
        {
            const auto models = DrumInfo::models (l);
            for (int m = 0; m < models.size() - 1; ++m)
            {
                setParam (proc, PID::lane (l, "model"), (float) m);
                for (float vel : { 1.0f, 0.3f })
                {
                    juce::MidiBuffer midi;
                    midi.addEvent (juce::MidiMessage::noteOn (10, DrumInfo::laneNote[l], vel), 0);
                    const int n = (int) (sr * 0.8);
                    juce::AudioBuffer<float> b (2, n);
                    for (int pos = 0; pos < n; pos += 512)
                    {
                        const int len = juce::jmin (512, n - pos);
                        juce::AudioBuffer<float> blk (2, len);
                        proc.processBlock (blk, midi);
                        midi.clear();
                        for (int c = 0; c < 2; ++c) b.copyFrom (c, pos, blk, c, 0, len);
                    }
                    const float pk = b.getMagnitude (0, n);
                    const float rms = (b.getRMSLevel (0, 0, n) + b.getRMSLevel (1, 0, n)) * 0.5f;
                    const auto name = juce::String (DrumInfo::names[l]) + " / " + models[m] + (vel < 0.5f ? " (leise)" : "");
                    CHECK (finite (b), name + ": NaN/Inf");
                    CHECK (pk > (vel < 0.5f ? 0.01f : 0.08f), name + ": zu leise (" + juce::String (pk, 3) + ")");
                    CHECK (pk <= 1.0f, name + ": uebersteuert");
                    if (vel > 0.5f)
                    {
                        std::cout << "   " << name.paddedRight (' ', 30) << " peak " << juce::String (juce::Decibels::gainToDecibels (pk), 1)
                                  << " dB  rms " << juce::String (juce::Decibels::gainToDecibels (rms), 1) << " dB" << std::endl;
                        // Kurzes Stueck fuer die Hoerprobe
                        const int keep = (int) (sr * 0.45);
                        const int old = all.getNumSamples();
                        all.setSize (2, old + keep, true, true);
                        for (int c = 0; c < 2; ++c) all.copyFrom (c, old, b, c, 0, keep);
                    }
                }
                // ausklingen lassen
                juce::MidiBuffer none; juce::AudioBuffer<float> tail (2, 48000); proc.processBlock (tail, none);
            }
            setParam (proc, PID::lane (l, "model"), 0.0f);
        }
        writeWav (all, out.getChildFile ("alle-modelle.wav"));
        setParam (proc, PID::seqOn, 1.0f);
        head.playing = true;
    }

    // ------------------------------------------------------------------ 2) DAW-Sync
    std::cout << "2) DAW-Sync" << std::endl;
    {
        cleanSlate (proc);
        setParam (proc, PID::lane (0, "model"), 1.0f);    // kurze 808-Kick
        setParam (proc, PID::lane (0, "decay"), 0.0f);
        proc.patterns.setVel (0, 0, 0, 127);
        proc.patterns.setVel (0, 0, 8, 127);
        head.bpm = 120.0;
        // Start bei ppq 0: Schlaege bei 0 und 2 Vierteln (= 1 s)
        auto b = render (proc, head, 2.2, sr, 0.0);
        auto o = onsets (b);
        CHECK (o.size() >= 3, "Sync: zu wenige Schlaege (" + juce::String ((int) o.size()) + ")");
        if (o.size() >= 3)
        {
            std::cout << "   Einsaetze: " << o[0] << ", " << o[1] << ", " << o[2] << " (soll 0, 48000, 96000)" << std::endl;
            CHECK (std::abs (o[0] - 0) <= 2, "Sync: erster Schlag nicht bei 0");
            CHECK (std::abs (o[1] - 48000) <= 2, "Sync: zweiter Schlag nicht bei 1 s");
            CHECK (std::abs (o[2] - 96000) <= 2, "Sync: dritter Schlag nicht bei 2 s");
        }
        // Start mitten im Takt (ppq 0.9) bei 97 BPM: naechster Schlag bei ppq 2.0
        flush (proc, head);
        head.bpm = 97.0;
        b = render (proc, head, 2.0, sr, 0.9);
        o = onsets (b);
        const int expect = (int) ((2.0 - 0.9) * sr * 60.0 / 97.0);
        CHECK (! o.empty() && std::abs (o[0] - expect) <= 2, "Sync bei 97 BPM ab ppq 0.9: " + juce::String (o.empty() ? -1 : o[0]) + " statt " + juce::String (expect));
        if (! o.empty()) std::cout << "   97 BPM ab ppq 0.9: " << o[0] << " (soll " << expect << ")" << std::endl;

        // Swing 100 %: Step 1 (ungerade) um einen halben Step spaeter
        head.bpm = 120.0;
        proc.patterns.clear (0);
        proc.patterns.setVel (0, 0, 1, 127);
        setParam (proc, PID::swing, 1.0f);
        flush (proc, head);
        b = render (proc, head, 0.5, sr, 0.0);
        o = onsets (b);
        const int swingExpect = (int) (0.25 * 1.5 * sr * 0.5);   // 1/16 = 0.25 ppq, +50 %, 0.5 s pro Viertel
        CHECK (! o.empty() && std::abs (o[0] - swingExpect) <= 2, "Swing: " + juce::String (o.empty() ? -1 : o[0]) + " statt " + juce::String (swingExpect));
        setParam (proc, PID::swing, 0.0f);

        // Triolen 1/8T: Step 1 bei 1/3 Viertel
        setParam (proc, PID::res, 3.0f);
        flush (proc, head);
        b = render (proc, head, 0.5, sr, 0.0);
        o = onsets (b);
        const int tripExpect = (int) (sr * 0.5 / 3.0);
        CHECK (! o.empty() && std::abs (o[0] - tripExpect) <= 2, "Triole: " + juce::String (o.empty() ? -1 : o[0]) + " statt " + juce::String (tripExpect));
        setParam (proc, PID::res, 1.0f);

        // Steps 12 bei 1/16: Pattern wiederholt sich nach 3 Vierteln
        proc.patterns.clear (0);
        proc.patterns.setVel (0, 0, 0, 127);
        setParam (proc, PID::steps, 12.0f - 8.0f);
        flush (proc, head);
        b = render (proc, head, 3.2, sr, 0.0);
        o = onsets (b);
        CHECK (o.size() >= 3 && std::abs (o[1] - 72000) <= 2 && std::abs (o[2] - 144000) <= 2, "12 Steps: Wiederholung falsch");
        setParam (proc, PID::steps, 16.0f - 8.0f);

        // Wirbel: 4 Schlaege in einem Step
        proc.patterns.clear (0);
        setParam (proc, PID::lane (4, "model"), 4.0f);
        proc.patterns.setVel (0, 4, 0, 120);
        proc.patterns.setRatchet (0, 4, 0, 4);
        juce::ignoreUnused (proc.getLaneHits (4));
        flush (proc, head);
        const int before = proc.getLaneHits (4);
        render (proc, head, 0.12, sr, 0.0);
        CHECK (proc.getLaneHits (4) - before == 4, "Wirbel 4x: " + juce::String (proc.getLaneHits (4) - before) + " Schlaege");
        proc.patterns.clear (0);
    }

    // ------------------------------------------------------------------ 3) Pattern-Wechsel am Ende
    std::cout << "3) Pattern-Wechsel" << std::endl;
    {
        cleanSlate (proc);
        proc.patterns.setVel (0, 0, 0, 127);    // A: Kick
        proc.patterns.setVel (1, 1, 0, 127);    // B: Snare
        head.bpm = 120.0;
        render (proc, head, 0.3, sr, 0.0);       // laeuft A
        proc.selectPattern (1);
        CHECK (proc.getPlayingPattern() == 0 && proc.getPendingPattern() == 1, "Wechsel sollte warten");
        const int snareBefore = proc.getLaneHits (1);
        render (proc, head, 1.8, sr, 0.3 * 2.0);   // bis Takt-Ende + Anfang
        CHECK (proc.getPlayingPattern() == 1, "Wechsel am Pattern-Ende fehlt");
        CHECK (proc.getLaneHits (1) > snareBefore, "Pattern B spielt nicht");
    }

    // ------------------------------------------------------------------ 4) Werks-Presets
    std::cout << "4) Werks-Presets (" << Presets::numFactory() << ")" << std::endl;
    {
        auto demoDir = out.getChildFile ("presets");
        demoDir.createDirectory();
        for (int i = 0; i < Presets::numFactory(); ++i)
        {
            const auto& fp = Presets::factory (i);
            proc.loadFactory (i);
            head.bpm = juce::String (fp.category).contains ("Jazz") ? 140.0
                     : (juce::String (fp.name).contains ("Trap") || juce::String (fp.name).contains ("Drill")) ? 140.0
                     : juce::String (fp.name).contains ("Drum & Bass") ? 172.0
                     : juce::String (fp.category).contains ("Hip") ? 90.0
                     : juce::String (fp.category).contains ("Schlager") ? 128.0 : 122.0;
            if (juce::String (fp.name).contains ("Ballade")) head.bpm = 72.0;
            if (juce::String (fp.name).contains ("Polka")) head.bpm = 120.0;
            const double stepsPpq = fp.steps * Res::stepPpq (fp.res);
            const double secPerPattern = stepsPpq * 60.0 / head.bpm;
            const int reps = (int) std::ceil (4.0 / secPerPattern);
            // A A A B C D: Grundgroove, Fill, Break, Crash
            juce::AudioBuffer<float> all (2, 0);
            const int seq[] { 0, 0, 1, 2, 3 };
            double ppq = 0.0;
            int hitsBefore = 0;
            for (int l = 0; l < DrumInfo::numLanes; ++l) hitsBefore += proc.getLaneHits (l);
            flush (proc, head);
            for (int part : seq)
            {
                if (quick && part != 0) continue;
                setParam (proc, PID::pattern, (float) part);
                proc.selectPattern (part);
                const int partReps = part == 0 ? juce::jmax (1, reps / 2) : 1;
                auto b = render (proc, head, secPerPattern * partReps, sr, ppq);
                // selectPattern wartet aufs Pattern-Ende: Teile nahtlos aneinander
                ppq += stepsPpq * partReps;
                const int old = all.getNumSamples();
                all.setSize (2, old + b.getNumSamples(), true, true);
                for (int c = 0; c < 2; ++c) all.copyFrom (c, old, b, c, 0, b.getNumSamples());
            }
            int hits = -hitsBefore;
            for (int l = 0; l < DrumInfo::numLanes; ++l) hits += proc.getLaneHits (l);
            const float pk = all.getMagnitude (0, all.getNumSamples());
            const float rms = (all.getRMSLevel (0, 0, all.getNumSamples()) + all.getRMSLevel (1, 0, all.getNumSamples())) * 0.5f;
            const auto name = juce::String (fp.category) + " / " + fp.name;
            CHECK (finite (all), name + ": NaN");
            CHECK (pk <= 1.0f, name + ": uebersteuert");
            CHECK (pk < juce::Decibels::decibelsToGain (-0.3f), name + ": laeuft in den Begrenzer");
            CHECK (rms > juce::Decibels::decibelsToGain (-32.0f), name + ": zu leise (" + juce::String (juce::Decibels::gainToDecibels (rms), 1) + " dB)");
            CHECK (hits > 4, name + ": keine Schlaege");
            std::cout << "   " << name.paddedRight (' ', 42) << juce::String (head.bpm, 0) << " BPM  peak "
                      << juce::String (juce::Decibels::gainToDecibels (pk), 1) << " dB  rms " << juce::String (juce::Decibels::gainToDecibels (rms), 1)
                      << " dB  " << hits << " Schlaege" << std::endl;
            writeWav (all, demoDir.getChildFile (juce::File::createLegalFileName (juce::String (i + 1).paddedLeft ('0', 2) + " " + fp.category + " - " + fp.name) + ".wav"));
        }
    }

    // ------------------------------------------------------------------ 5) Zustand + Sample + MIDI
    std::cout << "5) Zustand, Sample, MIDI-Export" << std::endl;
    {
        proc.loadFactory (3);
        // Test-Sample erzeugen und auf Spur Perc laden
        auto wavFile = out.getChildFile ("test-sample.wav");
        {
            juce::AudioBuffer<float> s (1, 12000);
            for (int i = 0; i < 12000; ++i) s.setSample (0, i, 0.7f * std::sin (i * 0.05f) * std::exp (-i / 3000.0f));
            wavFile.deleteFile();
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (wavFile), 44100.0, 1, 16, {}, 0));
            w->writeFromAudioSampleBuffer (s, 0, 12000);
        }
        CHECK (proc.loadSample (11, wavFile), "Sample laden");
        CHECK (juce::roundToInt (proc.apvts.getRawParameterValue (PID::lane (11, "model"))->load()) == DrumInfo::sampleModel (11), "Modell steht nicht auf Sample");
        proc.patterns.setVel (2, 5, 7, 66);
        proc.patterns.setRatchet (2, 5, 7, 3);
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        K54DProcessor p2;
        p2.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (p2.patterns.vel (2, 5, 7) == 66 && p2.patterns.ratchet (2, 5, 7) == 3, "Pattern nicht wiederhergestellt");
        CHECK (p2.sampleName (11) == "test-sample", "Sample nicht wiederhergestellt");
        CHECK (p2.presetName == proc.presetName, "Preset-Name fehlt");
        // Sample klingt
        p2.setPlayHead (&head);
        p2.setPlayConfigDetails (0, 2, sr, 512);
        p2.prepareToPlay (sr, 512);
        setParam (p2, PID::seqOn, 0.0f);
        juce::MidiBuffer midi; midi.addEvent (juce::MidiMessage::noteOn (10, 56, 1.0f), 0);
        juce::AudioBuffer<float> b (2, 4096);
        p2.processBlock (b, midi);
        CHECK (b.getMagnitude (0, 4096) > 0.05f, "Sample spielt nicht");

        const auto mid = proc.exportMidi (0, out.getChildFile ("pattern-a.mid"));
        juce::MidiFile mf;
        juce::FileInputStream is (mid);
        CHECK (mf.readFrom (is) && mf.getNumTracks() == 1 && mf.getTrack (0)->getNumEvents() > 10, "MIDI-Export leer");

        // eigenes Preset speichern/laden
        CHECK (proc.saveUserPreset ("Test K54D"), "Preset speichern");
        auto pf = proc.userPresetFolder().getChildFile ("Test K54D.k54dpreset");
        CHECK (pf.existsAsFile() && proc.loadUserPreset (pf), "Preset laden");
        pf.deleteFile();
    }

    // ------------------------------------------------------------------ 6) Screenshot
    std::cout << "6) Screenshot" << std::endl;
    {
        for (int i = 0; i < Presets::numFactory(); ++i)
            if (juce::String (Presets::factory (i).name) == "Trap 140") proc.loadFactory (i);
        proc.selectedLane = 4;
        for (bool colorful : { true, false })
        {
            std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
            auto* main = dynamic_cast<K54DMain*> (ed->getChildComponent (0));
            K54View::colorful = colorful;
            if (main != nullptr) { main->setSize (K54DMain::W, K54DMain::H); main->repaint(); }
            ed->setSize (K54DMain::W, K54DMain::H);
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            auto f = out.getChildFile (colorful ? "screenshot.png" : "screenshot-schlicht.png");
            f.deleteFile();
            juce::FileOutputStream os (f);
            juce::PNGImageFormat().writeImageToStream (img, os);
            CHECK (img.getWidth() == K54DMain::W, "Screenshot");
        }
        K54View::colorful = true;
    }

    std::cout << std::endl << (failures == 0 ? "ALLES OK" : "FEHLER") << ": " << checks - failures << " / " << checks << " Pruefungen bestanden" << std::endl;
    return failures == 0 ? 0 : 1;
}

// Offline-Test K-54 S: spielt jedes Werks-Preset an, prueft auf Stille/Uebersteuerung/NaN,
// testet Sample (laden, Loop, One-Shot, Aufnahme), eigene Wavetable, Preset-Speichern,
// und schreibt WAV-Beispiele + Screenshots der Oberflaeche.
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

static void setP (K55Processor& p, const char* id, float v)
{
    if (auto* par = p.apvts.getParameter (id)) par->setValueNotifyingHost (par->convertTo0to1 (v));
}

struct Ev { int at, note; bool on; float vel; };

static juce::AudioBuffer<float> render (K55Processor& proc, double sr, int block, double seconds, const std::vector<Ev>& ev,
                                         const juce::AudioBuffer<float>* input = nullptr)
{
    const int total = (int) (sr * seconds);
    juce::AudioBuffer<float> out (2, total), buf (2, block);
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        buf.setSize (2, n, false, false, true);
        buf.clear();
        if (input != nullptr)
            for (int c = 0; c < 2; ++c)
                for (int s = 0; s < n; ++s)
                    buf.setSample (c, s, pos + s < input->getNumSamples() ? input->getSample (c, pos + s) : 0.0f);
        juce::MidiBuffer midi;
        for (auto& e : ev)
            if (e.at >= pos && e.at < pos + n)
                midi.addEvent (e.on ? juce::MidiMessage::noteOn (1, e.note, e.vel) : juce::MidiMessage::noteOff (1, e.note), e.at - pos);
        proc.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
    }
    return out;
}

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::FileOutputStream> (f.createOutputStream()))
        if (auto* w = wav.createWriterFor (os.get(), sr, 2, 24, {}, 0))
        {
            os.release();
            std::unique_ptr<juce::AudioFormatWriter> wr (w);
            wr->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        }
}

static bool check (const juce::String& name, const juce::AudioBuffer<float>& b, float minPeak = 0.01f)
{
    float pk = 0.0f; double sum = 0.0; bool nan = false;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int s = 0; s < b.getNumSamples(); ++s)
        {
            const float x = b.getSample (c, s);
            if (! std::isfinite (x)) nan = true;
            pk = juce::jmax (pk, std::abs (x));
            sum += (double) x * x;
        }
    const double rms = std::sqrt (sum / (b.getNumSamples() * b.getNumChannels()));
    const bool ok = ! nan && pk >= minPeak && pk <= 1.0f;
    std::printf ("%s %-28s peak %6.3f (%6.1f dB)  rms %6.1f dB%s\n", ok ? "OK  " : "FAIL", name.toRawUTF8(), pk,
                 juce::Decibels::gainToDecibels (pk), juce::Decibels::gainToDecibels ((float) rms), nan ? "  NaN!" : "");
    return ok;
}

static juce::Component* findById (juce::Component* c, const juce::String& id)
{
    if (c->getComponentID() == id) return c;
    for (auto* ch : c->getChildren())
        if (auto* r = findById (ch, id)) return r;
    return nullptr;
}

static void shot (K55Processor& proc, int page, const juce::File& f, std::function<void (juce::Component&)> prepare = {})
{
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* k = dynamic_cast<K55Editor*> (ed.get());
    k->setPage (page);
    for (int i = 0; i < 3; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (40);
    if (prepare) prepare (*ed);
    for (int i = 0; i < 4; ++i) juce::MessageManager::getInstance()->runDispatchLoopUntil (40);
    auto im = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    f.deleteFile();
    juce::FileOutputStream fo (f);
    juce::PNGImageFormat png;
    png.writeImageToStream (im, fo);
}

/** Grundton per normierter Autokorrelation (50..2000 Hz) */
static double estimateHz (const juce::AudioBuffer<float>& b, int from, int len, double sr)
{
    const float* x = b.getReadPointer (0);
    double best = 0.0; int bestLag = 0;
    const int minLag = (int) (sr / 2000.0), maxLag = (int) (sr / 50.0);
    std::vector<double> r ((size_t) maxLag + 2, 0.0);
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double s = 0.0, e1 = 0.0, e2 = 0.0;
        for (int i = 0; i < len; ++i) { const double a = x[from + i], c = x[from + i + lag]; s += a * c; e1 += a * a; e2 += c * c; }
        r[(size_t) lag] = s / (std::sqrt (e1 * e2) + 1e-12);
    }
    // erstes deutliches Maximum (vermeidet Oktavfehler)
    double top = 0.0; for (int lag = minLag; lag <= maxLag; ++lag) top = std::max (top, r[(size_t) lag]);
    for (int lag = minLag + 1; lag < maxLag; ++lag)
        if (r[(size_t) lag] > 0.85 * top && r[(size_t) lag] >= r[(size_t) lag - 1] && r[(size_t) lag] >= r[(size_t) lag + 1]) { bestLag = lag; best = r[(size_t) lag]; break; }
    if (bestLag == 0) return 0.0;
    // parabolische Verfeinerung
    const double a = r[(size_t) bestLag - 1], c = r[(size_t) bestLag + 1];
    const double d = 0.5 * (a - c) / (a - 2 * best + c);
    return sr / (bestLag + d);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? argv[1] : "/tmp/k55test");
    outDir.createDirectory();

    std::unique_ptr<juce::AudioProcessor> base (createPluginFilter());
    auto& proc = dynamic_cast<K55Processor&> (*base);
    const double sr = 48000.0;
    const int block = 256;
    proc.setPlayConfigDetails (2, 2, sr, block);
    auto at = [&] (double t) { return (int) (t * sr); };
    bool allOk = true;
    {
        juce::SharedResourcePointer<PianoSet> pn;
        const auto t0 = juce::Time::getMillisecondCounter();
        const bool ok = pn->waitUntilReady (30000);
        std::printf ("Fluegel-Samples geladen: %s (%u ms)\n", ok ? "ja" : "NEIN", juce::Time::getMillisecondCounter() - t0);
        allOk &= ok;
    }

    // ---------------------------------------------------------------- 1) alle Werks-Presets
    const auto& presets = getFactoryPresets();
    for (int pi = 0; pi < proc.getNumPrograms(); ++pi)
    {
        if (juce::String (presets[(size_t) pi].category) == "Sample") continue;   // brauchen ein Sample (Test 2)
        proc.setCurrentProgram (pi);
        proc.prepareToPlay (sr, block);
        const bool poly = juce::roundToInt (proc.apvts.getRawParameterValue ("voiceMode")->load()) == 2;
        std::vector<Ev> ev;
        if (poly)
        {
            for (int n : { 48, 55, 60, 64 }) ev.push_back ({ at (0.05), n, true, 0.85f });
            for (int n : { 48, 55, 60, 64 }) ev.push_back ({ at (1.8),  n, false, 0.0f });
        }
        else
        {
            const int line[] { 0, 12, 7, 10, 12, 3 };
            for (int i = 0; i < 6; ++i)
            {
                ev.push_back ({ at (0.05 + i * 0.28), 36 + line[i], true, i % 2 ? 0.6f : 1.0f });
                ev.push_back ({ at (0.05 + i * 0.28 + 0.24), 36 + line[i], false, 0.0f });
            }
        }
        auto out = render (proc, sr, block, 3.0, ev);
        allOk &= check (proc.getProgramName (pi), out);
        writeWav (outDir.getChildFile ("preset_" + juce::String (pi).paddedLeft ('0', 2) + "_" + juce::File::createLegalFileName (proc.getProgramName (pi)) + ".wav"), out, sr);
    }

    // ---------------------------------------------------------------- 2) Sample laden & spielen
    // Testsample: 1 s gezupfter Ton (A3 = 220 Hz), Stereo
    auto smpFile = outDir.getChildFile ("testsample.wav");
    {
        juce::AudioBuffer<float> s (2, (int) sr);
        for (int i = 0; i < s.getNumSamples(); ++i)
        {
            const double t = i / sr;
            const float v = (float) (std::sin (2 * juce::MathConstants<double>::pi * 220.0 * t) * 0.6 + std::sin (2 * juce::MathConstants<double>::pi * 440.0 * t) * 0.2) * (float) std::exp (-t * 2.0);
            s.setSample (0, i, v);
            s.setSample (1, i, v * 0.8f);
        }
        writeWav (smpFile, s, sr);
    }
    juce::String err;
    allOk &= proc.loadSampleFile (smpFile, err);
    std::printf ("Sample geladen: %s %s\n", proc.getSample() != nullptr ? "ja" : "NEIN", err.toRawUTF8());

    auto presetIndex = [&] (const char* name) { for (int i = 0; i < proc.getNumPrograms(); ++i) if (proc.getProgramName (i) == name) return i; return 0; };
    for (auto* pn : { "Sample chromatisch", "Sample Loop-Player", "Sample One-Shot", "Sample durch Ladder" })
    {
        proc.setCurrentProgram (presetIndex (pn));
        setP (proc, ParamID::smpRoot, 57);   // A3
        proc.prepareToPlay (sr, block);
        std::vector<Ev> ev { { at (0.02), 57, true, 1.0f }, { at (0.1), 64, true, 0.8f }, { at (0.4), 57, false, 0 }, { at (0.4), 64, false, 0 } };
        auto out = render (proc, sr, block, 2.0, ev);
        allOk &= check (pn, out);
        // One-Shot: nach dem Loslassen (0.4 s) muss es weiterklingen
        if (juce::String (pn) == "Sample One-Shot")
        {
            const float late = out.getMagnitude (0, at (0.6), at (0.1));
            std::printf ("     One-Shot klingt nach Note-Off weiter: %s (%.3f)\n", late > 0.02f ? "ja" : "NEIN", late);
            allOk &= late > 0.02f;
        }
        writeWav (outDir.getChildFile (juce::File::createLegalFileName (pn) + ".wav"), out, sr);
    }
    // Tonhoehe chromatisch: A4 (69) muss ~440 Hz Grundton haben -> Nulldurchgaenge zaehlen
    {
        proc.setCurrentProgram (presetIndex ("Sample chromatisch"));
        setP (proc, ParamID::smpRoot, 57);
        proc.prepareToPlay (sr, block);
        auto out = render (proc, sr, block, 0.5, { { 0, 69, true, 1.0f } });
        int zc = 0;
        for (int s = at (0.1) + 1; s < at (0.3); ++s) if (out.getSample (0, s - 1) < 0 && out.getSample (0, s) >= 0) ++zc;
        const double hz = zc / 0.2;
        std::printf ("     Tonhoehe A4 aus A3-Sample: %.0f Hz (soll ~440)\n", hz);
        allOk &= std::abs (hz - 440.0) < 25.0;
    }

    // Fluegel: A4 muss 440 Hz sein
    {
        proc.setCurrentProgram (presetIndex ("Pop Piano Bright"));
        proc.prepareToPlay (sr, block);
        auto out = render (proc, sr, block, 0.6, { { 0, 69, true, 0.8f } });
        int zc = 0;
        for (int s = at (0.15) + 1; s < at (0.45); ++s) if (out.getSample (0, s - 1) < 0 && out.getSample (0, s) >= 0) ++zc;
        // Grundton per Autokorrelation (Nulldurchgaenge taeuschen bei vielen Obertoenen)
        int bestLag = 0; double best = -1.0;
        for (int lag = (int) (sr / 1000.0); lag < (int) (sr / 150.0); ++lag)
        {
            double c = 0.0;
            for (int s = at (0.15); s < at (0.35); ++s) c += out.getSample (0, s) * out.getSample (0, s + lag);
            if (c > best) { best = c; bestLag = lag; }
        }
        const double hz = sr / bestLag;
        std::printf ("     Fluegel A4: %.0f Hz (soll ~440)\n", hz);
        juce::ignoreUnused (zc);
        allOk &= std::abs (hz - 440.0) < 15.0;
    }
    // Pitch-Bend runter (Bend Down 12): A4 -> A3
    {
        proc.setCurrentProgram (presetIndex ("Init"));
        setP (proc, ParamID::osc2Level, 0.0f);
        setP (proc, ParamID::osc1Wave, 0.0f);
        setP (proc, ParamID::bendDown, 12.0f);
        setP (proc, ParamID::cutoff, 20000.0f);
        setP (proc, ParamID::drift, 0.0f);
        proc.prepareToPlay (sr, block);
        proc.setScreenBend (-1.0f);
        auto out = render (proc, sr, block, 0.5, { { 0, 69, true, 1.0f } });
        int zc = 0;
        for (int s = at (0.1) + 1; s < at (0.4); ++s) if (out.getSample (0, s - 1) < 0 && out.getSample (0, s) >= 0) ++zc;
        std::printf ("     Pitch-Bend -1 bei Bend Down 12: %.0f Hz (soll ~220)\n", zc / 0.3);
        allOk &= std::abs (zc / 0.3 - 220.0) < 15.0;
        proc.setScreenBend (0.0f);
        render (proc, sr, block, 0.05, {});
    }
    // ECO-Modus: gleiche Lautstaerke, weniger Rechenzeit
    {
        proc.setCurrentProgram (presetIndex ("Super Saw Wave"));
        std::vector<Ev> ev;
        for (int n : { 48, 52, 55, 59, 62, 64, 67, 71 }) ev.push_back ({ 0, n, true, 0.9f });
        double tHigh = 0, tEco = 0;
        for (int q = 0; q < 2; ++q)
        {
            setP (proc, ParamID::quality, (float) q);
            proc.prepareToPlay (sr, block);
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            auto out = render (proc, sr, block, 4.0, ev);
            (q == 0 ? tHigh : tEco) = juce::Time::getMillisecondCounterHiRes() - t0;
            allOk &= check (q == 0 ? "8 Noten x 7 Unison HIGH" : "8 Noten x 7 Unison ECO", out);
        }
        std::printf ("     CPU (4 s Audio, 1 Kern): HIGH %.0f ms = %.1f %%, ECO %.0f ms = %.1f %%\n", tHigh, tHigh / 40.0, tEco, tEco / 40.0);
        setP (proc, ParamID::quality, 0.0f);
    }

    // Browser-Vorhoeren: Datei spielt ohne Taste
    {
        proc.prepareToPlay (sr, block);
        proc.previewFile (smpFile);
        for (int i = 0; i < 40 && proc.previewPlaying.load(); ++i) { juce::Thread::sleep (25); render (proc, sr, block, 0.01, {}); }
        auto out = render (proc, sr, block, 0.5, {});
        const float pk = out.getMagnitude (0, 0, out.getNumSamples());
        std::printf ("     Vorhoeren im Browser: %s (%.3f)\n", pk > 0.05f ? "spielt" : "STUMM", pk);
        allOk &= pk > 0.05f;
        proc.stopPreview();
        render (proc, sr, block, 0.05, {});
        const float pk2 = render (proc, sr, block, 0.2, {}).getMagnitude (0, 0, at (0.2));
        std::printf ("     Vorhoeren gestoppt: %s\n", pk2 < 0.01f ? "ja" : "NEIN");
        allOk &= pk2 < 0.01f;
    }

    // ---------------------------------------------------------------- 3) eigene Wavetable aus dem Sample
    proc.sampleToWavetable();
    proc.setCurrentProgram (presetIndex ("Init"));
    setP (proc, ParamID::osc1Wave, ParamChoices::wTable);
    setP (proc, ParamID::osc1Table, ParamChoices::userTable);
    setP (proc, ParamID::osc2Level, 0.0f);
    proc.prepareToPlay (sr, block);
    allOk &= check ("User-Wavetable", render (proc, sr, block, 1.0, { { 0, 48, true, 1.0f }, { at (0.7), 48, false, 0 } }));
    // Wavetable-Aliasing-Test: sehr hoher Ton darf nicht explodieren
    allOk &= check ("Wavetable C8 (Aliasing)", render (proc, sr, block, 0.5, { { 0, 108, true, 1.0f } }), 0.001f);

    // ---------------------------------------------------------------- 4) Aufnahme vom Eingang
    {
        setP (proc, ParamID::inAuto, 1.0f);
        proc.prepareToPlay (sr, block);
        juce::AudioBuffer<float> in (2, at (2.0));
        in.clear();
        for (int s = at (0.5); s < at (1.5); ++s)   // 0,5 s Stille, dann 1 s Ton
        {
            const float v = 0.3f * (float) std::sin (2 * juce::MathConstants<double>::pi * 330.0 * s / sr);
            in.setSample (0, s, v); in.setSample (1, s, v);
        }
        proc.startRecording();
        render (proc, sr, block, 1.7, {}, &in);
        proc.stopRecording();
        render (proc, sr, block, 0.05, {});
        for (int i = 0; i < 20 && proc.getRecState() != K55Processor::recIdle; ++i)
            juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
        auto s = proc.getSample();
        const double len = s != nullptr ? s->length() / s->sampleRate : 0.0;
        std::printf ("     Aufnahme: %s, %.2f s (Auto-Start schneidet Stille ab: soll ~1.2), Datei %s\n",
                     s != nullptr ? s->name.toRawUTF8() : "-", len, s != nullptr && s->file.existsAsFile() ? "gespeichert" : "FEHLT");
        allOk &= s != nullptr && len > 0.9 && len < 1.4 && s->file.existsAsFile();
    }

    // ---------------------------------------------------------------- 5) Preset mit Sample speichern & laden
    {
        proc.setCurrentProgram (presetIndex ("Sample chromatisch"));
        allOk &= proc.loadSampleFile (smpFile, err);
        const bool saved = proc.saveUserPreset ("Test Sample Preset", "Sample");
        proc.clearSample();
        auto file = K55Processor::getUserPresetFolder().getChildFile ("Sample").getChildFile ("Test Sample Preset.k54spreset");
        const bool loaded = proc.loadUserPreset (file);
        const bool back = proc.getSample() != nullptr && proc.getSample()->file == smpFile;
        std::printf ("     Preset gespeichert %s, geladen %s, Sample wieder da %s\n", saved ? "ja" : "NEIN", loaded ? "ja" : "NEIN", back ? "ja" : "NEIN");
        allOk &= saved && loaded && back;
        // Projekt-Zustand
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        proc.clearSample();
        proc.setStateInformation (mb.getData(), (int) mb.getSize());
        std::printf ("     Projekt-Zustand stellt Sample wieder her: %s\n", proc.getSample() != nullptr ? "ja" : "NEIN");
        allOk &= proc.getSample() != nullptr;
        file.deleteFile();
    }

    // ---------------------------------------------------------------- 5a) Sample-Editor (Schneiden wie in einer DAW) + Lautstaerke
    {
        using SE = K55Processor::SampleEdit;
        allOk &= proc.loadSampleFile (smpFile, err);
        const int full = proc.getSample()->length();
        auto len = [&] { return proc.getSample() != nullptr ? proc.getSample()->length() : 0; };
        auto near = [] (int v, double want, double tol) { return std::abs ((double) v - want) <= tol; };
        const double zc = sr * 0.005;

        setP (proc, ParamID::smpStart, 0.1f); setP (proc, ParamID::smpEnd, 0.9f);
        bool ok = proc.editSample (SE::trim, 0.25, 0.5);
        const auto f1 = proc.getSample()->file;
        ok &= near (len(), full * 0.25, 2 * zc) && f1.existsAsFile() && f1 != smpFile && f1.getFileName().contains ("(bearbeitet)");
        ok &= proc.apvts.getRawParameterValue (ParamID::smpStart)->load() < 0.001f && proc.apvts.getRawParameterValue (ParamID::smpEnd)->load() > 0.999f;
        std::printf ("     Zuschneiden 25-50 %%: %d Samples (soll ~%d), Datei %s, Marken zurueckgesetzt\n", len(), full / 4, f1.getFileName().toRawUTF8());
        // Original unveraendert?
        {
            juce::String e2; auto orig = SampleData::load (smpFile, proc.formats, e2);
            ok &= orig != nullptr && orig->length() == full;
        }
        // gespeicherte Datei = Inhalt im Speicher?
        {
            juce::String e2; auto disk = SampleData::load (f1, proc.formats, e2);
            ok &= disk != nullptr && disk->length() == len();
        }
        ok &= proc.undoSampleEdit() && len() == full;
        ok &= proc.redoSampleEdit() && near (len(), full * 0.25, 2 * zc);
        ok &= proc.undoSampleEdit() && len() == full && ! proc.canUndoSampleEdit() && proc.canRedoSampleEdit();
        std::printf ("     Rueckgaengig / Wiederholen: %s\n", ok ? "ok" : "FEHLER");

        // Loeschen: Rest rueckt zusammen, kein Sprung an der Schnittstelle
        ok &= proc.editSample (SE::cut, 0.25, 0.5);
        const int cutAt = (int) (full * 0.25);
        float jump = 0.0f;
        {
            auto& au = proc.getSample()->audio;
            for (int i = juce::jmax (1, cutAt - (int) zc * 2); i < juce::jmin (au.getNumSamples(), cutAt + (int) zc * 2); ++i)
                jump = juce::jmax (jump, std::abs (au.getSample (0, i) - au.getSample (0, i - 1)));
        }
        ok &= near (len(), full * 0.75, 2 * zc) && jump < 0.06f;
        std::printf ("     Loeschen 25-50 %%: %d Samples (soll ~%d), groesster Sprung an der Schnittstelle %.3f\n", len(), full * 3 / 4, jump);
        ok &= proc.getSample()->file == f1;   // weiter dieselbe bearbeitete Datei

        ok &= proc.editSample (SE::silence, 0.0, 0.1);
        const float quiet = proc.getSample()->audio.getMagnitude (0, (int) (len() * 0.01), (int) (len() * 0.08));
        const float preFade = std::abs (proc.getSample()->audio.getMagnitude (0, (int) (len() * 0.15), 64));
        ok &= proc.editSample (SE::fadeIn, 0.15, 0.3);
        const float firstIn = std::abs (proc.getSample()->audio.getMagnitude (0, (int) std::llround (len() * 0.15), 4));
        ok &= preFade > 0.05f;
        ok &= proc.editSample (SE::normalize, 0.0, 0.0);
        float pk = 0.0f; for (int c = 0; c < proc.getSample()->audio.getNumChannels(); ++c) pk = juce::jmax (pk, proc.getSample()->audio.getMagnitude (c, 0, len()));
        auto before = proc.getSample();
        ok &= proc.editSample (SE::reverse, 0.0, 0.0);
        const bool reversed = std::abs (proc.getSample()->audio.getSample (0, 10) - before->audio.getSample (0, len() - 11)) < 1.0e-6f;
        ok &= ! proc.editSample (SE::trim, 0.3, 0.3);    // ohne Auswahl: nichts tun
        std::printf ("     Stille %.4f, Fade-In beginnt bei %.4f, Normalisiert auf %.2f dB, Umkehren %s\n", quiet, firstIn, juce::Decibels::gainToDecibels (pk), reversed ? "ok" : "FEHLER");
        ok &= quiet < 0.001f && firstIn < 0.01f && std::abs (juce::Decibels::gainToDecibels (pk) + 0.3f) < 0.05f && reversed;

        // Projekt merkt sich die bearbeitete Fassung
        {
            const int l0 = len();
            juce::MemoryBlock mb;
            proc.getStateInformation (mb);
            proc.clearSample();
            proc.setStateInformation (mb.getData(), (int) mb.getSize());
            const bool back = proc.getSample() != nullptr && proc.getSample()->file == f1 && len() == l0;
            std::printf ("     Projekt laedt bearbeitete Fassung: %s\n", back ? "ja" : "NEIN");
            ok &= back;
        }

        // Lautstaerke-Regler: -12 dB = ein Viertel des Pegels
        proc.loadSampleFile (smpFile, err);
        proc.setCurrentProgram (presetIndex ("Sample chromatisch"));
        setP (proc, ParamID::smpRoot, 57);
        auto peakAt = [&] (float db)
        {
            setP (proc, ParamID::smpGain, db);
            proc.prepareToPlay (sr, block);
            auto out = render (proc, sr, block, 0.6, { { 0, 57, true, 1.0f }, { at (0.3), 57, false, 0.0f } });
            return out.getMagnitude (0, 0, out.getNumSamples());
        };
        const float p0 = peakAt (0.0f), pm12 = peakAt (-12.0f);
        setP (proc, ParamID::smpGain, 0.0f);
        const float ratioDb = juce::Decibels::gainToDecibels (pm12 / juce::jmax (1.0e-6f, p0));
        std::printf ("     Sample-Lautstaerke -12 dB -> Ausgang %.1f dB leiser (Filter/Drive veraendern es etwas)\n", -ratioDb);
        ok &= ratioDb < -6.0f && ratioDb > -18.0f;

        std::printf ("%s Sample-Editor\n", ok ? "OK  " : "FAIL");
        allOk &= ok;
        f1.deleteFile();
    }

    // ---------------------------------------------------------------- 5b) Harmonizer
    {
        Harmonizer::Settings hs; hs.on = true; hs.key = 0; hs.scale = 0;   // C-Dur, Terz + Quinte
        int res[Harmonizer::maxExtra];
        auto chord = [&] (int n) { juce::String t; const int c = Harmonizer::build (n, hs, res); t << juce::MidiMessage::getMidiNoteName (n, true, true, 3);
                                   for (int i = 0; i < c; ++i) t << " " << juce::MidiMessage::getMidiNoteName (res[i], true, true, 3); return t; };
        const auto c1 = chord (60), c2 = chord (64), c3 = chord (71);
        std::printf ("     C-Dur leitereigen: C -> %s | E -> %s | H -> %s\n", c1.toRawUTF8(), c2.toRawUTF8(), c3.toRawUTF8());
        allOk &= c1 == "C3 E3 G3" && c2 == "E3 G3 B3" && c3 == "B3 D4 F4";
        hs.scale = 7; hs.seventh = true;   // fester Moll7
        const auto c4 = chord (62);
        std::printf ("     Akkord Moll fest + 7 auf D: %s\n", c4.toRawUTF8());
        allOk &= c4 == "D3 F3 A3 C4";

        // im Prozessor: 1 Taste -> 3 Stimmen, nach dem Loslassen alles aus
        proc.setCurrentProgram (presetIndex ("Konzertfluegel"));
        setP (proc, ParamID::harmOn, 1.0f);
        proc.prepareToPlay (sr, block);
        render (proc, sr, block, 0.3, { { 0, 60, true, 0.9f } });
        const int v1 = proc.voicesActive.load();
        // Harmonie waehrend gehaltener Taste ausschalten -> darf nichts haengen lassen
        setP (proc, ParamID::harmOn, 0.0f);
        render (proc, sr, block, 0.05, {});
        render (proc, sr, block, 2.0, { { 0, 60, false, 0.0f } });
        const int v2 = proc.voicesActive.load();
        std::printf ("     Harmonizer: Stimmen bei 1 Taste %d (soll 3), nach Loslassen %d (soll 0)\n", v1, v2);
        allOk &= v1 == 3 && v2 == 0;
        setP (proc, ParamID::harmOn, 1.0f);
        proc.prepareToPlay (sr, block);
        allOk &= check ("Konzertfluegel + Harmonie", render (proc, sr, block, 2.0, { { 0, 60, true, 0.9f }, { at (0.5), 64, true, 0.8f }, { at (1.2), 60, false, 0 }, { at (1.2), 64, false, 0 } }));
    }

    // ---------------------------------------------------------------- 5c) KEYS-Ebene: alle Modelle + Varianten, Tonhoehe, Leslie, Talkbox, CPU
    {
        std::printf ("--- KEYS-Ebene\n");
        auto keysSetup = [&] (int model, int variant)
        {
            proc.setCurrentProgram (0);
            setP (proc, ParamID::synthOn, 0.0f);
            setP (proc, ParamID::keysOn, 1.0f);
            setP (proc, ParamID::keysModel, (float) model);
            setP (proc, ParamID::keysVariant, (float) variant);
            setP (proc, ParamID::keysA, 0.6f);
            proc.prepareToPlay (sr, block);
        };
        for (int m = 0; m < ParamChoices::numKeysModels; ++m)
        {
            const auto& names = ParamChoices::keysVariantNames (m);
            for (int v = 0; v < names.size(); ++v)
            {
                keysSetup (m, v);
                std::vector<Ev> ev;
                for (int n : { 36, 48, 55, 60, 64, 72, 84 }) { ev.push_back ({ at (0.05), n, true, 0.9f }); ev.push_back ({ at (1.2), n, false, 0.0f }); }
                auto out = render (proc, sr, block, 2.2, ev);
                const juce::String nm = ParamChoices::keysModels[m] + " / " + names[v];
                allOk &= check (nm, out, 0.02f);
                // nach dem Loslassen muss es (bis auf Hall) ausklingen
                const float tail = out.getMagnitude (0, at (2.0), at (0.2));
                if (tail > 0.05f) { std::printf ("FAIL     %s klingt nach Note-Off weiter (%.3f)\n", nm.toRawUTF8(), tail); allOk = false; }
                if (v == 0 || m == ParamChoices::kPipe)
                    writeWav (outDir.getChildFile ("keys_" + juce::File::createLegalFileName (nm.replace (" / ", "_")) + ".wav"), out, sr);
            }
        }
        // Tonhoehe: A4 = 440 Hz in jedem Modell (Pfeifenorgel: Welt ohne Temperierung)
        for (int m = 0; m < ParamChoices::numKeysModels; ++m)
        {
            keysSetup (m, m == ParamChoices::kPipe ? 6 : (m == ParamChoices::kTonewheel ? 5 : 0));
            setP (proc, ParamID::keysA, m == ParamChoices::kPipe ? 0.0f : 0.3f);
            setP (proc, ParamID::keysB, 0.0f);
            proc.prepareToPlay (sr, block);
            auto out = render (proc, sr, block, 0.7, { { 0, 69, true, 0.8f } });
            const double hz = estimateHz (out, at (0.3), at (0.08), sr);
            const bool bell = m == ParamChoices::kFmBell;     // Glocken: unharmonisch, Oktave genuegt
            const bool ok = std::abs (hz - 440.0) < 6.0 || (bell && std::abs (hz - 220.0) < 4.0);
            std::printf ("%s Tonhoehe %-16s A4 = %.1f Hz\n", ok ? "OK  " : "FAIL", ParamChoices::keysModels[m].toRawUTF8(), hz);
            allOk &= ok;
        }
        // Leslie langsam -> schnell per Mod-Rad: Modulation muss staerker werden
        {
            proc.setCurrentProgram (presetIndex ("Jazz Drawbars"));
            proc.prepareToPlay (sr, block);
            std::vector<Ev> ev { { at (0.02), 60, true, 0.8f }, { at (0.02), 64, true, 0.8f }, { at (0.02), 67, true, 0.8f } };
            const int total = at (6.0);
            juce::AudioBuffer<float> out (2, total), buf (2, block);
            for (int pos = 0; pos < total; pos += block)
            {
                buf.clear();
                juce::MidiBuffer midi;
                for (auto& e : ev) if (e.at >= pos && e.at < pos + block) midi.addEvent (juce::MidiMessage::noteOn (1, e.note, e.vel), e.at - pos);
                if (pos <= at (3.0) && at (3.0) < pos + block) midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 127), 0);
                proc.processBlock (buf, midi);
                for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, pos, buf, ch, 0, block);
            }
            auto spread = [&] (int a, int len)
            {
                float lo = 10, hi = 0;
                for (int s = a; s < a + len; s += at (0.02)) { const float r = out.getRMSLevel (0, s, at (0.02)); lo = std::min (lo, r); hi = std::max (hi, r); }
                return hi / std::max (lo, 1e-6f);
            };
            const float slow = spread (at (1.0), at (0.6)), fast = spread (at (5.0), at (0.6));
            std::printf ("%s Leslie langsam/schnell: Pegelschwankung %.2f -> %.2f\n", fast > slow ? "OK  " : "FAIL", slow, fast);
            allOk &= check ("Leslie Fahrt", out) && fast > slow;
            writeWav (outDir.getChildFile ("keys_leslie_slow_fast.wav"), out, sr);
        }
        // Talkbox: Mod-Rad von U nach I -> Spektrum wird heller (mehr Nulldurchgaenge)
        {
            proc.setCurrentProgram (presetIndex ("Talk Box Lead"));
            proc.prepareToPlay (sr, block);
            const int total = at (3.0);
            juce::AudioBuffer<float> out (2, total), buf (2, block);
            for (int pos = 0; pos < total; pos += block)
            {
                buf.clear();
                juce::MidiBuffer midi;
                if (pos == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 57, 0.9f), 0);
                const int cc = juce::jlimit (0, 127, (int) (127.0 * pos / total * 1.3));
                midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, cc), 0);
                proc.processBlock (buf, midi);
                for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, pos, buf, ch, 0, block);
            }
            auto zcr = [&] (int a, int len) { int z = 0; for (int s = a + 1; s < a + len; ++s) if ((out.getSample (0, s - 1) < 0) != (out.getSample (0, s) < 0)) ++z; return z; };
            const int u = zcr (at (0.3), at (0.3)), i = zcr (at (2.5), at (0.3));
            std::printf ("%s Talkbox U -> I: Nulldurchgaenge %d -> %d\n", i > u ? "OK  " : "FAIL", u, i);
            allOk &= check ("Talkbox Fahrt", out) && i > u;
            writeWav (outDir.getChildFile ("keys_talkbox_sweep.wav"), out, sr);
        }
        // CPU: Kathedrale Tutti, 10 Toene + Leslie-Orgel + Clav, je 10 s
        for (auto* pn : { "Passau Dom Tutti", "Gospel Leslie", "Funk Clav 72", "FM Ballade Piano", "Tines + Pad" })
        {
            proc.setCurrentProgram (presetIndex (pn));
            proc.prepareToPlay (sr, block);
            std::vector<Ev> ev;
            for (int n : { 36, 43, 48, 52, 55, 60, 64, 67, 72, 76 }) ev.push_back ({ at (0.01), n, true, 0.8f });
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            auto out = render (proc, sr, block, 10.0, ev);
            const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
            std::printf ("     CPU %-20s 10 Toene: %.1f %% Echtzeit\n", pn, ms / 100.0);
            allOk &= check (juce::String (pn) + " (10 Toene)", out);
        }
    }

    // ---------------------------------------------------------------- 6) Screenshots
    proc.setCurrentProgram (presetIndex ("Growl Bass"));
    proc.prepareToPlay (sr, block);
    render (proc, sr, block, 0.3, { { 0, 36, true, 1.0f } });
    shot (proc, 0, outDir.getChildFile ("screen_synth.png"));
    shot (proc, 1, outDir.getChildFile ("screen_mod.png"));
    setP (proc, ParamID::smpLoop, 1.0f);
    setP (proc, ParamID::smpStart, 0.05f);
    setP (proc, ParamID::smpEnd, 0.85f);
    setP (proc, ParamID::smpLoopStart, 0.4f);
    shot (proc, 2, outDir.getChildFile ("screen_sample.png"));
    proc.loadSampleFile (smpFile, err);
    setP (proc, ParamID::smpGain, 4.0f);
    shot (proc, 2, outDir.getChildFile ("screen_sample_edit.png"), [] (juce::Component& ed)
    {
        if (auto* w = dynamic_cast<SampleWaveView*> (findById (&ed, "sampleWave")))
        {
            w->zoomBy (0.5, 0.3);
            w->setSelection (0.22, 0.36);
        }
    });
    setP (proc, ParamID::smpGain, 0.0f);
    shot (proc, 3, outDir.getChildFile ("screen_fx.png"));
    shot (proc, 4, outDir.getChildFile ("screen_harmony.png"));
    proc.setCurrentProgram (presetIndex ("Funk Clav 72"));
    shot (proc, 5, outDir.getChildFile ("screen_keys_clav.png"));
    proc.setCurrentProgram (presetIndex ("Notre-Dame Paris Plenum"));
    shot (proc, 5, outDir.getChildFile ("screen_keys_orgel.png"));

    std::printf ("\n%s\n", allOk ? "ALLE TESTS OK" : "ES GIBT FEHLER");
    return allOk ? 0 : 1;
}

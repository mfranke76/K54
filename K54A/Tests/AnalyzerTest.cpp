// K-54 Analyzer – Offline-Tests (Linux/CI)
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace k54a;

static int failures = 0;

static void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  OK    " : "  FEHLER ") << what << std::endl;
    if (! ok) ++failures;
}

static bool near (float a, float b, float tol) { return std::abs (a - b) <= tol; }

// 997 Hz Sinus (EBU-Testsignale nutzen 1 kHz), Pegel in dBFS, beide Kanäle
static void feedSine (LoudnessMeter& lm, double sr, float dbfs, double seconds, double& phase, double freq = 997.0)
{
    const float amp = juce::Decibels::decibelsToGain (dbfs);
    const int n = (int) (seconds * sr);
    std::vector<float> L (1024), R (1024);
    for (int pos = 0; pos < n; pos += 1024)
    {
        const int m = std::min (1024, n - pos);
        for (int i = 0; i < m; ++i)
        {
            L[(size_t) i] = R[(size_t) i] = amp * (float) std::sin (phase);
            phase += 2.0 * juce::MathConstants<double>::pi * freq / sr;
        }
        lm.process (L.data(), R.data(), m);
    }
}

static void testLoudness (double sr)
{
    std::cout << "Lautheit nach EBU Tech 3341/3342 bei " << sr << " Hz" << std::endl;
    double ph = 0.0;

    { LoudnessMeter lm; lm.prepare (sr); feedSine (lm, sr, -23.0f, 20.0, ph);
      check (near (lm.momentary(), -23.0f, 0.1f) && near (lm.shortTerm(), -23.0f, 0.1f) && near (lm.integrated(), -23.0f, 0.1f),
             "Fall 1: -23 dBFS → M/S/I = " + juce::String (lm.momentary(), 2) + " / " + juce::String (lm.shortTerm(), 2)
             + " / " + juce::String (lm.integrated(), 2)); }

    { LoudnessMeter lm; lm.prepare (sr); feedSine (lm, sr, -33.0f, 20.0, ph);
      check (near (lm.integrated(), -33.0f, 0.1f), "Fall 2: -33 dBFS → I = " + juce::String (lm.integrated(), 2)); }

    { LoudnessMeter lm; lm.prepare (sr);
      feedSine (lm, sr, -36.0f, 10.0, ph); feedSine (lm, sr, -23.0f, 60.0, ph); feedSine (lm, sr, -36.0f, 10.0, ph);
      check (near (lm.integrated(), -23.0f, 0.1f), "Fall 3 (relatives Gate): I = " + juce::String (lm.integrated(), 2)); }

    { LoudnessMeter lm; lm.prepare (sr);
      feedSine (lm, sr, -72.0f, 10.0, ph); feedSine (lm, sr, -36.0f, 10.0, ph); feedSine (lm, sr, -23.0f, 60.0, ph);
      feedSine (lm, sr, -36.0f, 10.0, ph); feedSine (lm, sr, -72.0f, 10.0, ph);
      check (near (lm.integrated(), -23.0f, 0.1f), "Fall 4 (absolutes Gate): I = " + juce::String (lm.integrated(), 2)); }

    { LoudnessMeter lm; lm.prepare (sr);
      feedSine (lm, sr, -20.0f, 20.0, ph); feedSine (lm, sr, -30.0f, 20.0, ph);
      check (near (lm.lra(), 10.0f, 1.0f), "LRA Fall 1 (-20/-30 dBFS): " + juce::String (lm.lra(), 2) + " LU (soll 10 ±1)"); }

    { LoudnessMeter lm; lm.prepare (sr);
      feedSine (lm, sr, -40.0f, 20.0, ph); feedSine (lm, sr, -20.0f, 20.0, ph);
      check (near (lm.lra(), 20.0f, 1.0f), "LRA Fall 3 (-40/-20 dBFS): " + juce::String (lm.lra(), 2) + " LU (soll 20 ±1)"); }

    // True Peak: fs/4 mit 45° Phase → Samples bei 0,707, echte Spitze 1,0
    { LoudnessMeter lm; lm.prepare (sr);
      std::vector<float> L (4096);
      for (int i = 0; i < 4096; ++i)
          L[(size_t) i] = (float) std::sin (juce::MathConstants<double>::halfPi * i + juce::MathConstants<double>::pi / 4.0);
      for (int k = 0; k < 10; ++k) lm.process (L.data(), L.data(), 4096);
      check (near (lm.truePeakDb(), 0.0f, 0.4f), "True Peak fs/4 (Sample-Peak -3 dB): " + juce::String (lm.truePeakDb(), 2) + " dBTP"); }
}

static void pinkish (juce::Random& rnd, float* out, int n, float gain, float* st)
{
    for (int i = 0; i < n; ++i)
    {
        const float w = rnd.nextFloat() * 2.0f - 1.0f;
        st[0] = 0.99886f * st[0] + w * 0.0555179f; st[1] = 0.99332f * st[1] + w * 0.0750759f;
        st[2] = 0.96900f * st[2] + w * 0.1538520f; st[3] = 0.86650f * st[3] + w * 0.3104856f;
        st[4] = 0.55000f * st[4] + w * 0.5329522f; st[5] = -0.7616f * st[5] - w * 0.0168980f;
        out[i] = gain * (st[0] + st[1] + st[2] + st[3] + st[4] + st[5] + w * 0.5362f) * 0.11f;
    }
}

static juce::File writeRefWav (const juce::File& dir)
{
    // 12 s Referenz bei 44,1 kHz: helles Rauschen + Bass-Sinus, laut
    const double sr = 44100.0;
    const int n = (int) (12 * sr);
    juce::AudioBuffer<float> b (2, n);
    juce::Random rnd (7);
    for (int i = 0; i < n; ++i)
    {
        const float bass = 0.35f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 55.0 * i / sr);
        const float hiss = 0.18f * (rnd.nextFloat() * 2.0f - 1.0f);
        b.setSample (0, i, bass + hiss);
        b.setSample (1, i, bass + 0.9f * hiss);
    }
    auto f = dir.getChildFile ("Referenz Test.wav");
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os = std::make_unique<juce::FileOutputStream> (f);
    auto w = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}.withSampleRate (sr).withNumChannels (2).withBitsPerSample (24));
    w->writeFromAudioSampleBuffer (b, 0, n);
    return f;
}

static void setParam (AnalyzerProcessor& p, const char* id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

static void runAudio (AnalyzerProcessor& p, double seconds, float mixGain, juce::Random& rnd, float* st,
                      LoudnessMeter* outMeter = nullptr, double* maxDiff = nullptr)
{
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    const int blocks = (int) (seconds * 48000.0 / 512.0);
    for (int k = 0; k < blocks; ++k)
    {
        pinkish (rnd, buf.getWritePointer (0), 512, mixGain, st);
        buf.copyFrom (1, 0, buf, 0, 0, 512);
        juce::AudioBuffer<float> in;
        if (maxDiff) in.makeCopyOf (buf);
        p.processBlock (buf, midi);
        if (outMeter) outMeter->process (buf.getReadPointer (0), buf.getReadPointer (1), 512);
        if (maxDiff)
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    *maxDiff = std::max (*maxDiff, (double) std::abs (buf.getSample (ch, i) - in.getSample (ch, i)));
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir = argc > 1 ? juce::File (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]))
                                       : juce::File::getCurrentWorkingDirectory();
    outDir.createDirectory();

    testLoudness (48000.0);
    testLoudness (44100.0);
    testLoudness (96000.0);

    std::cout << "Plugin / Referenz / A/B" << std::endl;
    auto refFile = writeRefWav (outDir);

    auto proc = std::make_unique<AnalyzerProcessor>();
    proc->setPlayConfigDetails (2, 2, 48000.0, 512);
    proc->prepareToPlay (48000.0, 512);

    // Laden über den Hintergrund-Thread
    proc->loadReference (refFile);
    for (int i = 0; i < 200 && proc->getRefStatus() != AnalyzerProcessor::refReady; ++i)
        juce::Thread::sleep (25);
    auto* ref = proc->getRef();
    check (ref != nullptr && proc->getRefStatus() == AnalyzerProcessor::refReady, "Referenz geladen (Thread)");
    if (ref == nullptr) return 1;
    check (std::abs (ref->length() - (int64_t) (12 * 48000)) < 64, "Umrechnung 44,1 → 48 kHz: " + juce::String (ref->length()) + " Samples");
    check (ref->integrated > -20.0f && ref->integrated < -5.0f, "Referenz Integrated = " + juce::String (ref->integrated, 2) + " LUFS");
    check (ref->wave.size() == 1600, "Wellenform-Übersicht");

    juce::Random rnd (1);
    float st[6] {};

    // A: Ausgang = Eingang
    setParam (*proc, "sync", 0.0f);
    setParam (*proc, "ab", 0.0f);
    double diff = 0.0;
    runAudio (*proc, 2.0, 0.5f, rnd, st, nullptr, &diff);
    check (diff < 1.0e-12, "A = Mix unverändert (max. Abweichung " + juce::String (diff) + ")");

    // B mit Lautheitsangleich, frei laufend
    proc->resetMeasurements();
    proc->setFreePlaying (true);
    setParam (*proc, "match", 1.0f);
    setParam (*proc, "ab", 1.0f);
    runAudio (*proc, 6.0, 0.5f, rnd, st);   // einschwingen
    LoudnessMeter outM; outM.prepare (48000.0);
    runAudio (*proc, 10.0, 0.5f, rnd, st, &outM);
    const float mixI = proc->mixLoud.i.load();
    check (near (outM.integrated(), mixI, 0.6f), "B angeglichen: Ausgang " + juce::String (outM.integrated(), 2)
           + " LUFS / Mix " + juce::String (mixI, 2) + " LUFS (Angleich " + juce::String (proc->getMatchGainDb(), 2) + " dB)");
    check (proc->isRefRunning(), "Referenz läuft frei");
    check (proc->refLoud.s.load() > -30.0f, "Referenz-Messung live (Short-Term " + juce::String (proc->refLoud.s.load(), 2) + ")");

    // B ohne Angleich = Original-Lautheit
    setParam (*proc, "match", 0.0f);
    runAudio (*proc, 1.0, 0.5f, rnd, st);
    LoudnessMeter outB; outB.prepare (48000.0);
    proc->seekReference (0.0);
    runAudio (*proc, 11.5, 0.5f, rnd, st, &outB);
    check (near (outB.integrated(), ref->integrated, 0.5f), "B ohne Angleich: " + juce::String (outB.integrated(), 2)
           + " LUFS (Datei " + juce::String (ref->integrated, 2) + ")");

    // Ref-Gain
    setParam (*proc, "refGain", -6.0f);
    runAudio (*proc, 0.5, 0.5f, rnd, st);
    LoudnessMeter outG; outG.prepare (48000.0);
    proc->seekReference (0.0);
    runAudio (*proc, 11.5, 0.5f, rnd, st, &outG);
    check (near (outG.integrated(), ref->integrated - 6.0f, 0.5f), "Ref-Gain -6 dB: " + juce::String (outG.integrated(), 2));
    setParam (*proc, "refGain", 0.0f);

    // Zustand speichern / laden
    juce::MemoryBlock state;
    proc->getStateInformation (state);
    auto proc2 = std::make_unique<AnalyzerProcessor>();
    proc2->setPlayConfigDetails (2, 2, 96000.0, 512);
    proc2->setStateInformation (state.getData(), (int) state.getSize());
    proc2->prepareToPlay (96000.0, 512);
    for (int i = 0; i < 200 && proc2->getRefStatus() != AnalyzerProcessor::refReady; ++i)
        juce::Thread::sleep (25);
    auto* ref2 = proc2->getRef();
    check (ref2 != nullptr && std::abs (ref2->sampleRate - 96000.0) < 1.0, "Projekt wiederherstellen: Referenz neu geladen bei 96 kHz");
    check (proc2->apvts.getRawParameterValue ("sync")->load() < 0.5f, "Projekt wiederherstellen: Parameter");
    proc2.reset();

    // Mono-Spur
    {
        auto mono = std::make_unique<AnalyzerProcessor>();
        juce::AudioProcessor::BusesLayout lay;
        lay.inputBuses.add (juce::AudioChannelSet::mono());
        lay.inputBuses.add (juce::AudioChannelSet::disabled());
        lay.outputBuses.add (juce::AudioChannelSet::mono());
        check (mono->setBusesLayout (lay), "Mono-Layout erlaubt");
        mono->prepareToPlay (48000.0, 256);
        juce::AudioBuffer<float> b (1, 256);
        juce::MidiBuffer m;
        for (int k = 0; k < 800; ++k) { pinkish (rnd, b.getWritePointer (0), 256, 0.5f, st); mono->processBlock (b, m); }
        check (mono->mixLoud.s.load() > -40.0f, "Mono-Messung: " + juce::String (mono->mixLoud.s.load(), 2) + " LUFS");
    }

    // ---- 1.1: Tonale Balance der Referenz -------------------------------
    std::cout << "Version 1.1: Bänder, Slots, Abhören" << std::endl;
    check (ref->hasBands && ref->longTermDb.size() == (size_t) SpecMath::numPoints, "Referenz: Langzeit-Spektrum + Bänder berechnet");
    {
        int loudest = 0;
        for (int i = 1; i < SpecMath::numBands; ++i) if (ref->bands[(size_t) i] > ref->bands[(size_t) loudest]) loudest = i;
        check (loudest == 0, juce::String ("Referenz mit 55-Hz-Bass: lautestes Band = ") + SpecMath::bandName (loudest)
               + " (" + juce::String (ref->bands[0], 1) + " dB)");
    }

    // ---- Slot 2 mit anderer Datei ------------------------------------------
    {
        const double fs = 48000.0;
        juce::AudioBuffer<float> b2 (2, (int) (8 * fs));
        for (int i = 0; i < b2.getNumSamples(); ++i)
        {
            const float v = 0.1f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 2000.0 * i / fs);
            b2.setSample (0, i, v); b2.setSample (1, i, v);
        }
        auto f2 = outDir.getChildFile ("Referenz Zwei.wav");
        f2.deleteFile();
        {
            std::unique_ptr<juce::OutputStream> os = std::make_unique<juce::FileOutputStream> (f2);
            juce::WavAudioFormat wav;
            auto w = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}.withSampleRate (fs).withNumChannels (2).withBitsPerSample (24));
            w->writeFromAudioSampleBuffer (b2, 0, b2.getNumSamples());
        }
        proc->loadReference (f2, 1);
        for (int i = 0; i < 200 && proc->getRef (1) == nullptr; ++i) juce::Thread::sleep (25);
        auto* r2 = proc->getRef (1);
        check (r2 != nullptr && proc->getRef (0) == ref, "Slot 2 geladen, Slot 1 unverändert");

        setParam (*proc, "match", 0.0f);
        setParam (*proc, "ab", 1.0f);
        proc->setFreePlaying (true);
        proc->setSlot (1);
        runAudio (*proc, 0.5, 0.5f, rnd, st);
        LoudnessMeter o2; o2.prepare (48000.0);
        runAudio (*proc, 5.0, 0.5f, rnd, st, &o2);
        check (r2 != nullptr && near (o2.integrated(), r2->integrated, 0.5f), "Umschalten auf Slot 2: Ausgang " + juce::String (o2.integrated(), 2)
               + " LUFS (Datei " + juce::String (r2 != nullptr ? r2->integrated : 0.0f, 2) + ")");
        proc->setSlot (0);
        runAudio (*proc, 0.5, 0.5f, rnd, st);
        check (proc->getRef() == ref, "Zurück auf Slot 1");

        // Projekt mit 2 Slots speichern/laden
        juce::MemoryBlock st2;
        proc->getStateInformation (st2);
        auto p3 = std::make_unique<AnalyzerProcessor>();
        p3->setPlayConfigDetails (2, 2, 48000.0, 512);
        p3->setStateInformation (st2.getData(), (int) st2.getSize());
        p3->prepareToPlay (48000.0, 512);
        for (int i = 0; i < 300 && (p3->getRef (0) == nullptr || p3->getRef (1) == nullptr); ++i) juce::Thread::sleep (25);
        check (p3->getRef (0) != nullptr && p3->getRef (1) != nullptr, "Projekt wiederherstellen: beide Slots geladen");
    }

    // ---- Abhören ------------------------------------------------------------
    {
        setParam (*proc, "ab", 0.0f);
        runAudio (*proc, 0.1, 0.5f, rnd, st);
        juce::AudioBuffer<float> b (2, 512);
        juce::MidiBuffer m;
        auto fill = [&] (float l, float r, double freq)
        {
            for (int i = 0; i < 512; ++i)
            {
                const float v = (float) std::sin (2.0 * juce::MathConstants<double>::pi * freq * i / 48000.0);
                b.setSample (0, i, l * v); b.setSample (1, i, r * v);
            }
        };

        setParam (*proc, "monitor", 1.0f);
        fill (0.5f, 0.0f, 1000.0);
        proc->processBlock (b, m);
        check (std::abs (b.getSample (0, 100) - b.getSample (1, 100)) < 1.0e-6f && std::abs (b.getSample (0, 100)) > 0.0f, "Abhören Mono: L = R");
        check (proc->isMonitoringActive(), "Abhören-Warnung aktiv");

        setParam (*proc, "monitor", 2.0f);
        fill (0.5f, 0.5f, 1000.0);
        proc->processBlock (b, m);
        check (b.getMagnitude (0, 512) < 1.0e-6f, "Abhören Side: Mono-Signal verschwindet");

        setParam (*proc, "monitor", 0.0f);
        setParam (*proc, "band", 1.0f);
        float rmsHigh = 0.0f, rmsLow = 0.0f;
        for (int k = 0; k < 40; ++k) { fill (0.5f, 0.5f, 8000.0); proc->processBlock (b, m); rmsHigh = b.getRMSLevel (0, 0, 512); }
        for (int k = 0; k < 40; ++k) { fill (0.5f, 0.5f, 60.0);   proc->processBlock (b, m); rmsLow = b.getRMSLevel (0, 0, 512); }
        check (rmsHigh < 0.002f && rmsLow > 0.2f, "Band-Solo Bass: 8 kHz " + juce::String (juce::Decibels::gainToDecibels (rmsHigh), 1)
               + " dB, 60 Hz " + juce::String (juce::Decibels::gainToDecibels (rmsLow), 1) + " dB");
        setParam (*proc, "band", 0.0f);
        check (! proc->isMonitoringActive(), "Abhören zurückgestellt");
    }

    // ---- 1.2: Live-Referenz über Sidechain ---------------------------------
    {
        auto lp = std::make_unique<AnalyzerProcessor>();
        juce::AudioProcessor::BusesLayout lay;
        lay.inputBuses.add (juce::AudioChannelSet::stereo());
        lay.inputBuses.add (juce::AudioChannelSet::stereo());
        lay.outputBuses.add (juce::AudioChannelSet::stereo());
        check (lp->setBusesLayout (lay), "Sidechain-Layout (Stereo + Stereo-Sidechain) erlaubt");
        lp->prepareToPlay (48000.0, 512);
        setParam (*lp, "refSlot", 4.0f);
        setParam (*lp, "ab", 1.0f);
        setParam (*lp, "match", 0.0f);
        check (lp->isLiveSlot(), "Slot LIVE gewählt");

        juce::AudioBuffer<float> b (4, 512);
        juce::MidiBuffer m;
        double ph = 0.0;
        LoudnessMeter scM, outL;
        scM.prepare (48000.0); outL.prepare (48000.0);
        double maxDiff = 0.0;
        for (int k = 0; k < 600; ++k)
        {
            pinkish (rnd, b.getWritePointer (0), 512, 0.5f, st);
            b.copyFrom (1, 0, b, 0, 0, 512);
            for (int i = 0; i < 512; ++i)
            {
                const float v = 0.25f * (float) std::sin (ph);
                ph += 2.0 * juce::MathConstants<double>::pi * 1000.0 / 48000.0;
                b.setSample (2, i, v); b.setSample (3, i, v);
            }
            juce::AudioBuffer<float> sc (2, 512);
            sc.copyFrom (0, 0, b, 2, 0, 512); sc.copyFrom (1, 0, b, 3, 0, 512);
            lp->processBlock (b, m);
            scM.process (sc.getReadPointer (0), sc.getReadPointer (1), 512);
            outL.process (b.getReadPointer (0), b.getReadPointer (1), 512);
            if (k > 20)
                for (int i = 0; i < 512; ++i)
                    maxDiff = std::max (maxDiff, (double) std::abs (b.getSample (0, i) - sc.getSample (0, i)));
        }
        check (lp->isLiveConnected() && lp->hasLiveSignal() && lp->hasReference(), "Sidechain erkannt, Signal da");
        check (maxDiff < 1.0e-5, "B = Sidechain-Signal (max. Abweichung " + juce::String (maxDiff, 7) + ")");
        check (near (lp->refLoud.i.load(), scM.integrated(), 0.1f), "LIVE-Lautheit " + juce::String (lp->refLoud.i.load(), 2)
               + " LUFS (Sidechain " + juce::String (scM.integrated(), 2) + ")");

        setParam (*lp, "match", 1.0f);
        LoudnessMeter outMatch; outMatch.prepare (48000.0);
        for (int k = 0; k < 900; ++k)
        {
            pinkish (rnd, b.getWritePointer (0), 512, 0.5f, st);
            b.copyFrom (1, 0, b, 0, 0, 512);
            for (int i = 0; i < 512; ++i)
            {
                const float v = 0.25f * (float) std::sin (ph);
                ph += 2.0 * juce::MathConstants<double>::pi * 1000.0 / 48000.0;
                b.setSample (2, i, v); b.setSample (3, i, v);
            }
            lp->processBlock (b, m);
            if (k > 300) outMatch.process (b.getReadPointer (0), b.getReadPointer (1), 512);
        }
        check (near (outMatch.integrated(), lp->mixLoud.i.load(), 0.7f), "LIVE angeglichen: Ausgang " + juce::String (outMatch.integrated(), 2)
               + " LUFS / Mix " + juce::String (lp->mixLoud.i.load(), 2) + " LUFS");

        setParam (*lp, "refSlot", 0.0f);
        for (int k = 0; k < 10; ++k) lp->processBlock (b, m);
        check (! lp->isLiveSlot() && ! lp->hasReference(), "Zurück auf leeren Slot 1: keine Referenz");
    }

    // Oberfläche: Screenshots EINFACH und PRO
    std::cout << "Oberfläche" << std::endl;
    setParam (*proc, "match", 1.0f);
    setParam (*proc, "hold", 0.0f);
    setParam (*proc, "ab", 1.0f);
    proc->setFreePlaying (true);
    proc->resetMeasurements();
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc->createEditor());
        auto* editor = dynamic_cast<AnalyzerEditor*> (ed.get());
        editor->setSize (Content::W, Content::H);
        auto& c = editor->getContent();

        auto snap = [&] (const char* name)
        {
            auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 1.0f);
            auto shot = outDir.getChildFile (name);
            shot.deleteFile();
            juce::FileOutputStream os (shot);
            juce::PNGImageFormat().writeImageToStream (img, os);
            os.flush();
            check (shot.getSize() > 10000, juce::String ("Screenshot ") + name);
        };

        c.setPro (false);
        for (int t = 0; t < 150; ++t) { runAudio (*proc, 1.0 / 30.0, 0.5f, rnd, st); editor->runTick(); }
        snap ("screen_einfach.png");

        c.setPro (true);
        setParam (*proc, "showDiff", 1.0f);
        setParam (*proc, "speed", 3.0f);
        c.setScopeTab (true);
        for (int t = 0; t < 60; ++t) { runAudio (*proc, 1.0 / 30.0, 0.5f, rnd, st); editor->runTick(); }
        snap ("screen_pro.png");
        proc->setSlot (4);
        for (int t = 0; t < 10; ++t) { runAudio (*proc, 1.0 / 30.0, 0.5f, rnd, st); editor->runTick(); }
        snap ("screen_live.png");
        proc->setSlot (0);
        for (int t = 0; t < 10; ++t) { runAudio (*proc, 1.0 / 30.0, 0.5f, rnd, st); editor->runTick(); }
        check (c.isPro() && proc->getProMode(), "PRO-Modus gemerkt");

        // ---- 1.3: Ansicht -------------------------------------------------
        {
            auto ticks = [&] (int n) { for (int t = 0; t < n; ++t) { runAudio (*proc, 1.0 / 30.0, 0.5f, rnd, st); editor->runTick(); } };
            ViewStyle vs;
            vs.specMode = 1;
            c.applyStyle (vs, false);
            ticks (40);
            snap ("screen_balken.png");

            vs = {};
            vs.specMode = 2; vs.theme = 1; vs.meterStyle = 1; vs.range = 1;
            c.applyStyle (vs, false);
            setParam (*proc, "speed", 1.0f);
            {
                // Testsignal mit wechselnden Tönen, damit man im Spektrogramm etwas sieht
                juce::AudioBuffer<float> tb (2, 1600);
                juce::MidiBuffer mb;
                double phs[3] {};
                static const double notes[] { 110.0, 146.8, 196.0, 261.6, 329.6, 440.0, 587.3, 880.0 };
                for (int t = 0; t < 240; ++t)
                {
                    const double f0 = notes[(t / 15) % 8];
                    pinkish (rnd, tb.getWritePointer (0), 1600, 0.12f, st);
                    for (int i = 0; i < 1600; ++i)
                    {
                        float v = tb.getSample (0, i);
                        for (int h = 0; h < 3; ++h)
                        {
                            phs[h] += 2.0 * juce::MathConstants<double>::pi * f0 * (h + 1) / 48000.0;
                            v += (0.25f / (float) (h + 1)) * (float) std::sin (phs[h]);
                        }
                        tb.setSample (0, i, v);
                        tb.setSample (1, i, v);
                    }
                    proc->processBlock (tb, mb);
                    editor->runTick();
                }
            }
            snap ("screen_spektrogramm.png");
            setParam (*proc, "speed", 3.0f);

            vs = {};
            vs.showTonal = vs.showHist = false; vs.theme = 2; vs.meterStyle = 2; vs.line = 2; vs.meterRange = 2;
            c.applyStyle (vs, false);
            ticks (40);
            snap ("screen_gross.png");


            // 1.5: eigene Farben, Felder, Presets
            vs = {};
            vs.theme = ViewStyle::customTheme; vs.customMix = 0xffff3d7f; vs.customRef = 0xff7dff6a; vs.customDiff = 0xfffff066;
            vs.accentMode = 1; vs.customAccent = 0xff00d0ff; vs.specColor = 0; vs.showHist = false; vs.showMeter = false;
            c.applyStyle (vs, false);
            ticks (30);
            snap ("screen_eigene_farben.png");

            vs = ViewPresets::builtIn()[4].second;   // Nur Spektrum
            c.applyStyle (vs, false);
            ticks (30);
            snap ("screen_nur_spektrum.png");

            {
                ViewStyle mine; mine.theme = 3; mine.showLoud = false;
                ViewPresets::saveUser (proc->getSettings(), "Testansicht", mine);
                auto u = ViewPresets::user (proc->getSettings());
                bool found = false;
                for (auto& e : u) found = found || (e.first == "Testansicht" && e.second == mine);
                check (found, "Eigene Ansicht speichern und wieder laden");
                ViewPresets::removeUser (proc->getSettings(), "Testansicht");
                bool gone = true;
                for (auto& e : ViewPresets::user (proc->getSettings())) gone = gone && e.first != "Testansicht";
                check (gone, "Eigene Ansicht löschen");
            }

            vs = {};
            c.applyStyle (vs, false);
            c.showView (true);
            ticks (3);
            snap ("screen_ansicht.png");
            c.showView (false);

            ViewStyle a, b2;
            a.theme = 5; a.specMode = 2; a.hold = 2; a.glow = false; a.customMix = 0xff123456; a.showLoud = false;
            a.save (proc->getSettings());
            b2.load (proc->getSettings());
            check (a == b2, "Ansicht speichern und laden");
            c.applyStyle (ViewStyle {}, true);
            check (c.getStyle() == ViewStyle {}, "Ansicht auf Standard");
        }

        // ---- 1.6: Ladebalken ----------------------------------------------------
        {
            const double fs = 44100.0;
            const int n = (int) (240 * fs);
            juce::AudioBuffer<float> lb (2, n);
            juce::Random r2 (3);
            for (int i = 0; i < n; ++i)
            {
                const float v = 0.2f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * i / fs) + 0.05f * (r2.nextFloat() - 0.5f);
                lb.setSample (0, i, v); lb.setSample (1, i, v);
            }
            auto longF = outDir.getChildFile ("Lange Referenz.wav");
            longF.deleteFile();
            {
                std::unique_ptr<juce::OutputStream> os = std::make_unique<juce::FileOutputStream> (longF);
                juce::WavAudioFormat wav;
                auto w = wav.createWriterFor (os, juce::AudioFormatWriterOptions{}.withSampleRate (fs).withNumChannels (2).withBitsPerSample (24));
                w->writeFromAudioSampleBuffer (lb, 0, n);
            }
            lb.setSize (0, 0);

            proc->setSlot (2);
            proc->loadReference (longF, 2);
            bool seen = false;
            float last = 0.0f;
            bool monotonic = true;
            for (int i = 0; i < 15000 && proc->getRefStatus() != AnalyzerProcessor::refReady; ++i)
            {
                juce::Thread::sleep (2);
                const float pnow = proc->getLoadProgress();
                if (proc->getRefStatus() == AnalyzerProcessor::refLoading)
                {
                    monotonic = monotonic && pnow + 1.0e-6f >= last;
                    last = pnow;
                    if (! seen && pnow > 0.3f)
                    {
                        editor->runTick();
                        snap ("screen_laden.png");
                        seen = true;
                    }
                }
            }
            check (seen, "Ladebalken sichtbar während des Ladens");
            check (monotonic, "Ladefortschritt läuft nur vorwärts");
            check (proc->getRef (2) != nullptr && proc->getLoadProgress() >= 0.999f, "Ladefortschritt am Ende 100 %");
            proc->clearReference (2);
            proc->setSlot (0);
            longF.deleteFile();
            for (int t = 0; t < 40; ++t) { runAudio (*proc, 1.0 / 30.0, 0.5f, rnd, st); editor->runTick(); }
        }

        const auto report = c.buildReport();
        check (report.contains ("Integrated") && report.contains ("TONALE BALANCE"), "Messbericht enthält Lautheit und Tonale Balance");
        outDir.getChildFile ("Bericht.txt").replaceWithText (report);

        c.showHelp (true);
        snap ("screen_help.png");
        c.showHelp (false);
        c.setPro (false);
        setParam (*proc, "speed", 1.0f);
    }

    // Aufräumen + Leistung
    {
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        runAudio (*proc, 20.0, 0.5f, rnd, st);
        const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
        check (ms < 20000.0 * 0.2, "Leistung: 20 s Audio in " + juce::String (ms, 0) + " ms (" + juce::String (ms / 200.0, 2) + " % Echtzeit)");
    }

    proc->clearReference();
    check (proc->getRef() == nullptr, "Referenz entfernt");
    proc.reset();

    std::cout << (failures == 0 ? "ALLE TESTS BESTANDEN" : juce::String (failures) + " FEHLER") << std::endl;
    return failures == 0 ? 0 : 1;
}

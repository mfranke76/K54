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
        lay.outputBuses.add (juce::AudioChannelSet::mono());
        check (mono->setBusesLayout (lay), "Mono-Layout erlaubt");
        mono->prepareToPlay (48000.0, 256);
        juce::AudioBuffer<float> b (1, 256);
        juce::MidiBuffer m;
        for (int k = 0; k < 800; ++k) { pinkish (rnd, b.getWritePointer (0), 256, 0.5f, st); mono->processBlock (b, m); }
        check (mono->mixLoud.s.load() > -40.0f, "Mono-Messung: " + juce::String (mono->mixLoud.s.load(), 2) + " LUFS");
    }

    // Oberfläche: Screenshot
    std::cout << "Oberfläche" << std::endl;
    setParam (*proc, "match", 1.0f);
    setParam (*proc, "hold", 0.0f);
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc->createEditor());
        auto* editor = dynamic_cast<AnalyzerEditor*> (ed.get());
        editor->setSize (Content::W, Content::H);
        for (int t = 0; t < 120; ++t)
        {
            runAudio (*proc, 1.0 / 30.0, 0.5f, rnd, st);
            editor->runTick();
        }
        auto img = editor->getContent().createComponentSnapshot (editor->getContent().getLocalBounds(), true, 1.0f);
        auto shot = outDir.getChildFile ("screen_analyzer.png");
        shot.deleteFile();
        juce::FileOutputStream os (shot);
        juce::PNGImageFormat().writeImageToStream (img, os);
        os.flush();
        check (shot.getSize() > 10000, "Screenshot " + shot.getFullPathName());

        // Hilfe
        editor->getContent().getChildComponent (editor->getContent().getNumChildComponents() - 1)->setVisible (true);
        auto img2 = editor->getContent().createComponentSnapshot (editor->getContent().getLocalBounds(), true, 1.0f);
        auto shot2 = outDir.getChildFile ("screen_help.png");
        shot2.deleteFile();
        juce::FileOutputStream os2 (shot2);
        juce::PNGImageFormat().writeImageToStream (img2, os2);
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

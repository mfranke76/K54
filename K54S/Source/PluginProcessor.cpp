#include "PluginProcessor.h"
#include "PluginEditor.h"

K55Processor::K55Processor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)    // Mikrofon / Interface / Sidechain
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "K55", createParameterLayout())
{
    formats.registerBasicFormats();
    loadFactoryPreset (0);
    startTimerHz (10);
}

K55Processor::~K55Processor()
{
    stopTimer();
    loaderPool.removeAllJobs (true, 3000);
}

bool K55Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    for (auto& in : layouts.inputBuses)
        if (! in.isDisabled() && in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()) return false;
    return true;
}

void K55Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const bool srChanged = std::abs (sampleRate - currentSr) > 1.0 || recBuf.getNumSamples() == 0;
    currentSr = sampleRate;
    readParams();
    engine.setParams (sp);
    engine.prepare (sampleRate, samplesPerBlock);
    harmonizer.reset();
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    revHp[0] = revHp[1] = 0.0f;
    const int n = juce::jmax (samplesPerBlock, 4096);
    synthBuf.setSize (2, n, false, true, false);
    revBuf.setSize (2, n, false, true, false);
    inputBuf.setSize (2, n, false, true, false);
    if (srChanged && recState.load() == recIdle)
        recBuf.setSize (2, (int) (sampleRate * maxRecSeconds) + 16, false, true, false);
    sourcesDirty = true;
}

// ---------------------------------------------------------------- Parameter
void K55Processor::readParams()
{
    auto f = [this] (const char* id) { return raw (id)->load(); };
    auto i = [this] (const char* id) { return juce::roundToInt (raw (id)->load()); };

    sp.cutoff     = f (ParamID::cutoff);
    sp.resonance  = f (ParamID::resonance);
    sp.envAmount  = f (ParamID::envAmount);
    sp.drive      = f (ParamID::drive);
    sp.ampA       = f (ParamID::ampAttack);
    sp.ampD       = f (ParamID::ampDecay);
    sp.ampS       = f (ParamID::ampSustain);
    sp.ampR       = f (ParamID::ampRelease);
    sp.fltA       = f (ParamID::fltAttack);
    sp.fltD       = f (ParamID::fltDecay);
    sp.fltS       = f (ParamID::fltSustain);
    sp.fltR       = f (ParamID::fltRelease);
    sp.glide      = f (ParamID::glide);
    sp.keyTrack   = f (ParamID::keyTrack);
    sp.velFilter  = f (ParamID::velFilter);
    sp.velAmp     = f (ParamID::velAmp);
    sp.masterGain = juce::Decibels::decibelsToGain (f (ParamID::master));

    const char* w[] { ParamID::osc1Wave, ParamID::osc2Wave, ParamID::osc3Wave };
    const char* o[] { ParamID::osc1Oct, ParamID::osc2Oct, ParamID::osc3Oct };
    const char* t[] { ParamID::osc1Tune, ParamID::osc2Tune, ParamID::osc3Tune };
    const char* l[] { ParamID::osc1Level, ParamID::osc2Level, ParamID::osc3Level };
    const char* tb[] { ParamID::osc1Table, ParamID::osc2Table, ParamID::osc3Table };
    const char* ps[] { ParamID::osc1Pos, ParamID::osc2Pos, ParamID::osc3Pos };
    for (int k = 0; k < 3; ++k)
    {
        sp.wave[k]   = i (w[k]);
        sp.octave[k] = i (o[k]);
        sp.tune[k]   = f (t[k]);
        sp.level[k]  = f (l[k]);
        sp.table[k]  = i (tb[k]);
        sp.pos[k]    = f (ps[k]);
    }
    sp.noise      = f (ParamID::noiseLevel);
    sp.pulseWidth = f (ParamID::pulseWidth);
    sp.lfoRate    = f (ParamID::lfoRate);
    sp.lfoPitch   = f (ParamID::lfoPitch);
    sp.lfoFilter  = f (ParamID::lfoFilter);
    sp.lfoWave    = i (ParamID::lfoWave);
    sp.voiceMode  = i (ParamID::voiceMode);
    sp.bendRange  = f (ParamID::bendRange);
    sp.bendDown   = f (ParamID::bendDown);
    sp.mwVibrato  = f (ParamID::mwVibrato);
    sp.mwCutoff   = f (ParamID::mwCutoff);
    sp.mwPos      = f (ParamID::mwPos);
    sp.atVibrato  = f (ParamID::atVibrato);
    sp.atCutoff   = f (ParamID::atCutoff);
    sp.modA       = f (ParamID::modAttack);
    sp.modD       = f (ParamID::modDecay);
    sp.modS       = f (ParamID::modSustain);
    sp.modR       = f (ParamID::modRelease);
    sp.eco        = i (ParamID::quality) == 1;
    sp.drift      = f (ParamID::drift);

    sp.unison     = ParamChoices::unisonValues[juce::jlimit (0, 5, i (ParamID::uniVoices))];
    sp.uniDetune  = f (ParamID::uniDetune);
    sp.uniSpread  = f (ParamID::uniSpread);
    sp.subLevel   = f (ParamID::subLevel);
    sp.fltType    = i (ParamID::fltType);
    sp.heat       = f (ParamID::heat);
    sp.lfo2Rate   = f (ParamID::lfo2Rate);
    sp.lfo2Wave   = i (ParamID::lfo2Wave);
    for (int k = 0; k < SynthParams::slots; ++k)
    {
        sp.modSrc[k] = juce::roundToInt (apvts.getRawParameterValue (ParamID::modSrc (k))->load());
        sp.modDst[k] = juce::roundToInt (apvts.getRawParameterValue (ParamID::modDst (k))->load());
        sp.modAmt[k] = apvts.getRawParameterValue (ParamID::modAmt (k))->load();
    }
    sp.chorusMix   = f (ParamID::chorusMix);
    sp.chorusRate  = f (ParamID::chorusRate);
    sp.chorusDepth = f (ParamID::chorusDepth);
    sp.delayMix    = f (ParamID::delayMix);
    sp.delayBeats  = ParamChoices::delayBeats[juce::jlimit (0, 11, i (ParamID::delayTime))];
    sp.delayFb     = f (ParamID::delayFb);

    sp.smpRoot      = i (ParamID::smpRoot);
    sp.smpStart     = f (ParamID::smpStart);
    sp.smpEnd       = f (ParamID::smpEnd);
    sp.smpLoop      = i (ParamID::smpLoop);
    sp.smpLoopStart = f (ParamID::smpLoopStart);
    sp.smpKeyTrack  = f (ParamID::smpKeyTrack) > 0.5f;
    sp.smpOneShot   = f (ParamID::smpOneShot) > 0.5f;
}

// ---------------------------------------------------------------- Audio
void K55Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    // Bildschirm-Wheels wie echte Wheels einspeisen
    if (uiBendDirty.exchange (false))
        midi.addEvent (juce::MidiMessage::pitchWheel (1, juce::jlimit (0, 16383, juce::roundToInt ((uiBend.load() + 1.0f) * 8192.0f))), 0);
    if (uiModDirty.exchange (false))
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, juce::jlimit (0, 127, juce::roundToInt (uiMod.load() * 127.0f))), 0);

    // Harmonizer: aus einer Taste wird ein Akkord
    {
        Harmonizer::Settings hs;
        auto on = [this] (const char* id) { return raw (id)->load() > 0.5f; };
        hs.on      = on (ParamID::harmOn);
        hs.key     = juce::roundToInt (raw (ParamID::harmKey)->load());
        hs.scale   = juce::roundToInt (raw (ParamID::harmScale)->load());
        hs.voicing = juce::roundToInt (raw (ParamID::harmVoicing)->load());
        hs.third = on (ParamID::harm3); hs.fifth = on (ParamID::harm5); hs.seventh = on (ParamID::harm7);
        hs.ninth = on (ParamID::harm9); hs.octUp = on (ParamID::harmOctUp); hs.octDown = on (ParamID::harmOctDn);
        hs.level = raw (ParamID::harmLevel)->load();
        harmonizer.process (midi, hs);
    }

    // ---- 1) Audio-Eingang sichern, bevor der Synth den Puffer ueberschreibt
    const int numIn = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    if (inputBuf.getNumSamples() < n) inputBuf.setSize (2, n, false, false, true);
    if (synthBuf.getNumSamples() < n) synthBuf.setSize (2, n, false, false, true);
    if (revBuf.getNumSamples() < n)   revBuf.setSize (2, n, false, false, true);
    inputBuf.clear (0, n);
    if (numIn > 0)
    {
        const int mode = juce::roundToInt (raw (ParamID::inMode)->load());
        const float g  = juce::Decibels::decibelsToGain (raw (ParamID::inGain)->load());
        const float* i0 = buffer.getReadPointer (0);
        const float* i1 = buffer.getReadPointer (numIn > 1 ? 1 : 0);
        float* l = inputBuf.getWritePointer (0);
        float* r = inputBuf.getWritePointer (1);
        for (int s = 0; s < n; ++s)
        {
            float a, b;
            switch (mode)
            {
                case 1:  a = b = i0[s]; break;
                case 2:  a = b = i1[s]; break;
                case 3:  a = b = 0.5f * (i0[s] + i1[s]); break;
                default: a = i0[s]; b = i1[s]; break;
            }
            l[s] = a * g;
            r[s] = b * g;
        }
    }
    float inPk = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        const float pk = numIn > 0 ? inputBuf.getMagnitude (ch, 0, n) : 0.0f;
        inPk = juce::jmax (inPk, pk);
        inPeak[ch].store (juce::jmax (pk, inPeak[ch].load()));
    }

    // ---- 2) Aufnahme
    {
        int st = recState.load();
        if (st == recStopping) { recState = recFinished; st = recFinished; }
        if (st == recArmed && inPk > 0.03f) { recState = recRunning; st = recRunning; }   // Auto-Start bei Signal (~ -30 dB)
        if (st == recRunning && recBuf.getNumSamples() > 0)
        {
            const int pos = recPos.load();
            const int todo = juce::jmin (n, recBuf.getNumSamples() - pos);
            for (int ch = 0; ch < 2; ++ch)
                recBuf.copyFrom (ch, pos, inputBuf, ch, 0, todo);
            recPos = pos + todo;
            if (todo < n) recState = recFinished;
        }
    }

    // ---- 3) neue Samples / Wavetables vom Message-Thread uebernehmen
    if (sourcesDirty.load())
    {
        const juce::SpinLock::ScopedTryLockType tl (sourceLock);
        if (tl.isLocked())
        {
            audioSample = pendingSample;     // alte Objekte haelt der ReleasePool -> keine Freigabe hier
            audioWT = pendingWT;
            sourcesDirty = false;
        }
    }
    SourceSet src;
    for (int k = 0; k < ParamChoices::numTables; ++k)
        src.tables[k] = k < (int) factory->tables.size() ? factory->tables[(size_t) k].get() : nullptr;
    src.tables[ParamChoices::userTable] = audioWT != nullptr ? audioWT.get() : factory->tables[0].get();
    src.sample = audioSample.get();
    src.piano = piano->isReady() ? &piano.get() : nullptr;

    readParams();
    engine.setParams (sp);
    engine.setSources (src);
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                engine.setTempo (*bpm);

    // ---- 4) Synth
    juce::AudioBuffer<float> synthView (synthBuf.getArrayOfWritePointers(), 2, n);
    engine.render (synthView, midi);
    voicesActive = engine.activeVoices();
    bendOut = engine.getBend();
    modOut = engine.getModWheel();

    // ---- 5) Hall (Send mit Hochpass, damit der Bass sauber bleibt)
    const float send = raw (ParamID::revSend)->load();
    if (send > 0.0001f)
    {
        juce::AudioBuffer<float> rv (revBuf.getArrayOfWritePointers(), 2, n);
        const float a = std::exp (-juce::MathConstants<float>::twoPi * 150.0f / (float) currentSr);
        for (int ch = 0; ch < 2; ++ch)
        {
            rv.clear (ch, 0, n);
            rv.addFrom (ch, 0, synthView, ch, 0, n, send);
            auto* d = rv.getWritePointer (ch);
            float lp = revHp[ch];
            for (int s = 0; s < n; ++s) { lp = d[s] + (lp - d[s]) * a; d[s] -= lp; }
            revHp[ch] = lp;
        }
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.45f + raw (ParamID::revSize)->load() * 0.53f;
        rp.damping  = 0.1f + raw (ParamID::revDamp)->load() * 0.8f;
        rp.wetLevel = 0.55f;
        rp.dryLevel = 0.0f;
        rp.width    = 1.0f;
        if (rp.roomSize != revParams.roomSize || rp.damping != revParams.damping || rp.wetLevel != revParams.wetLevel || rp.dryLevel != revParams.dryLevel)
        {
            revParams = rp;
            reverb.setParameters (rp);
        }
        reverb.processStereo (rv.getWritePointer (0), rv.getWritePointer (1), n);
        const float ret = raw (ParamID::revLevel)->load();
        for (int ch = 0; ch < 2; ++ch) synthView.addFrom (ch, 0, rv, ch, 0, n, ret);
    }

    // ---- 5b) Browser-Vorhoeren
    if (previewDirty.load())
    {
        const juce::SpinLock::ScopedTryLockType tl (sourceLock);
        if (tl.isLocked()) { audioPreview = pendingPreview; previewPos = 0.0; previewDirty = false; }
    }
    if (audioPreview != nullptr)
    {
        const auto& pv = *audioPreview;
        const double inc = pv.sampleRate / currentSr;
        const double end = (double) pv.length() - 3.0;
        auto* L = synthView.getWritePointer (0);
        auto* R = synthView.getWritePointer (1);
        for (int s = 0; s < n && previewPos < end; ++s)
        {
            const float g = previewPos > end - 256.0 ? (float) ((end - previewPos) / 256.0) * 0.8f : 0.8f;
            L[s] += pv.read (0, previewPos) * g;
            R[s] += pv.read (1, previewPos) * g;
            previewPos += inc;
        }
        if (previewPos >= end) { audioPreview.reset(); previewPlaying = false; }   // Pool haelt eine Kopie -> keine Freigabe hier
    }

    // ---- 6) Mithoeren des Eingangs
    if (numIn > 0 && raw (ParamID::inMonitor)->load() > 0.5f)
        for (int ch = 0; ch < 2; ++ch) synthView.addFrom (ch, 0, inputBuf, ch, 0, n);

    // ---- 7) Ausgang: Gain, weiche Begrenzung, Pegel
    const float og = juce::Decibels::decibelsToGain (raw (ParamID::outGain)->load());
    buffer.clear();
    const int outCh = getTotalNumOutputChannels();
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* d = synthView.getWritePointer (ch);
        float pk = 0.0f;
        for (int s = 0; s < n; ++s)
        {
            float x = d[s] * og;
            const float ax = std::abs (x);
            if (ax > 0.95f) x = std::copysign (0.95f + 0.05f * std::tanh ((ax - 0.95f) * 20.0f), x);
            d[s] = x;
            pk = juce::jmax (pk, ax);                       // vor dem Begrenzer: Anzeige zeigt echte Uebersteuerung
        }
        outPeak[ch].store (juce::jmax (pk, outPeak[ch].load()));
    }
    if (outCh == 1)
    {
        buffer.addFrom  (0, 0, synthView, 0, 0, n, 0.5f);
        buffer.addFrom  (0, 0, synthView, 1, 0, n, 0.5f);
    }
    else
        for (int ch = 0; ch < juce::jmin (2, outCh); ++ch)
            buffer.copyFrom (ch, 0, synthView, ch, 0, n);
}

// ---------------------------------------------------------------- Samples / Wavetables
void K55Processor::publishSources()
{
    pool.add (sample);
    pool.add (userWT);
    {
        const juce::SpinLock::ScopedLockType sl (sourceLock);
        pendingSample = sample;
        pendingWT = userWT;
    }
    sourcesDirty = true;
    ++sourceVersion;
}

bool K55Processor::loadSampleFile (const juce::File& f, juce::String& error)
{
    auto s = SampleData::load (f, formats, error);
    if (s == nullptr) return false;
    sample = s;
    publishSources();
    return true;
}

bool K55Processor::loadWavetableFile (const juce::File& f, juce::String& error)
{
    auto s = SampleData::load (f, formats, error);
    if (s == nullptr) return false;
    auto wt = Wavetable::fromAudio (s->audio, s->name);
    if (wt == nullptr) { error = "Daraus laesst sich keine Wavetable bauen."; return false; }
    userWT = wt;
    userWTFile = f;
    userWTFromSample = false;
    publishSources();
    return true;
}

void K55Processor::sampleToWavetable()
{
    if (sample == nullptr) return;
    if (auto wt = Wavetable::fromAudio (sample->audio, sample->name))
    {
        userWT = wt;
        userWTFile = sample->file;
        userWTFromSample = true;
        publishSources();
    }
}

void K55Processor::previewFile (const juce::File& f)
{
    const int gen = ++previewGen;
    previewPlaying = true;
    loaderPool.addJob ([this, f, gen]
    {
        juce::String err;
        auto s = SampleData::load (f, formats, err, 30.0);     // hoechstens 30 s vorhoeren
        if (gen != previewGen.load()) return;
        if (s == nullptr) { previewPlaying = false; return; }
        pool.add (s);
        {
            const juce::SpinLock::ScopedLockType sl (sourceLock);
            pendingPreview = s;
        }
        previewDirty = true;
    });
}

void K55Processor::stopPreview()
{
    ++previewGen;
    {
        const juce::SpinLock::ScopedLockType sl (sourceLock);
        pendingPreview.reset();
    }
    previewDirty = true;
    previewPlaying = false;
}

void K55Processor::clearSample()
{
    sample.reset();
    publishSources();
}

const Wavetable* K55Processor::getTableForDisplay (int index) const
{
    if (index == ParamChoices::userTable)
        return userWT != nullptr ? userWT.get() : factory->tables[0].get();
    if (juce::isPositiveAndBelow (index, (int) factory->tables.size()))
        return factory->tables[(size_t) index].get();
    return nullptr;
}

// ---------------------------------------------------------------- Aufnahme
void K55Processor::startRecording()
{
    if (recBuf.getNumSamples() == 0) return;
    recPos = 0;
    recState = raw (ParamID::inAuto)->load() > 0.5f ? recArmed : recRunning;
}

void K55Processor::stopRecording()
{
    int expected = recArmed;
    if (recState.compare_exchange_strong (expected, recIdle)) return;     // noch nichts aufgenommen
    expected = recRunning;
    if (recState.compare_exchange_strong (expected, recStopping))
        stopRequestedAt = juce::Time::getMillisecondCounter();
}

void K55Processor::timerCallback()
{
    // Fluegel-Samples erst laden, wenn ein Oszillator auf PIANO steht (spart Speicher und Startzeit)
    if (! pianoWanted)
        for (auto* id : { ParamID::osc1Wave, ParamID::osc2Wave, ParamID::osc3Wave })
            if (juce::roundToInt (raw (id)->load()) == ParamChoices::wPiano) { pianoWanted = true; piano->ensureLoaded(); break; }
    // Falls der Host gerade kein Audio rechnet, beendet der Message-Thread die Aufnahme selbst
    if (recState.load() == recStopping && juce::Time::getMillisecondCounter() - stopRequestedAt > 600)
    {
        int expected = recStopping;
        recState.compare_exchange_strong (expected, recFinished);
    }
    if (recState.load() == recFinished)
        finishRecording();
}

void K55Processor::finishRecording()
{
    const int len = recPos.load();
    recState = recIdle;
    if (len < (int) (currentSr * 0.05)) return;           // kuerzer als 50 ms: verwerfen

    auto s = std::make_shared<SampleData>();
    s->audio.setSize (2, len);
    for (int ch = 0; ch < 2; ++ch) s->audio.copyFrom (ch, 0, recBuf, ch, 0, len);
    const float pk = s->audio.getMagnitude (0, len);
    if (pk > 0.0001f) s->audio.applyGain (0.89f / pk);    // auf -1 dB normalisieren
    // Mono-Eingang? -> als Mono speichern
    bool same = true;
    for (int i = 0; i < len && same; i += 64) same = std::abs (s->audio.getSample (0, i) - s->audio.getSample (1, i)) < 1.0e-6f;
    if (same) s->audio.setSize (1, len, true);
    s->sampleRate = currentSr;
    s->name = "Aufnahme " + juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H-%M-%S");

    // als WAV sichern, damit Projekt und Presets das Sample wiederfinden
    auto folder = getSampleFolder();
    folder.createDirectory();
    auto file = folder.getChildFile (s->name + ".wav");
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::FileOutputStream> (file.createOutputStream()))
    {
        if (auto* w = wav.createWriterFor (os.get(), currentSr, (unsigned) s->audio.getNumChannels(), 24, {}, 0))
        {
            os.release();
            std::unique_ptr<juce::AudioFormatWriter> writer (w);
            writer->writeFromAudioSampleBuffer (s->audio, 0, len);
        }
    }
    s->file = file;
    s->buildPeaks();
    sample = s;
    publishSources();
}

// ---------------------------------------------------------------- Presets
int K55Processor::getNumPrograms() { return (int) getFactoryPresets().size(); }

void K55Processor::setCurrentProgram (int index)
{
    if (juce::isPositiveAndBelow (index, getNumPrograms())) loadFactoryPreset (index);
}

const juce::String K55Processor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, getNumPrograms())) return getFactoryPresets()[(size_t) index].name;
    return {};
}

void K55Processor::setParam (const juce::String& id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void K55Processor::resetToDefaults()
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! ParamID::isGlobalParam (rp->getParameterID()))
                rp->setValueNotifyingHost (rp->getDefaultValue());
}

void K55Processor::loadFactoryPreset (int index)
{
    const auto& list = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) list.size())) return;
    const auto& preset = list[(size_t) index];
    resetToDefaults();
    for (const auto& [id, value] : preset.values)
        setParam (id, value);
    currentProgram = index;
    presetName = preset.name;
    ++presetVersion;
}

juce::File K55Processor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Keller54").getChildFile ("K-54 S Presets");
}

juce::File K55Processor::getSampleFolder()
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Keller54").getChildFile ("K-54 S Samples");
}

juce::Array<juce::File> K55Processor::getUserPresets() const
{
    auto files = getUserPresetFolder().findChildFiles (juce::File::findFiles, true, "*.k54spreset");
    struct Cmp
    {
        static int compareElements (const juce::File& a, const juce::File& b)
        {
            const int c = a.getParentDirectory().getFileName().compareIgnoreCase (b.getParentDirectory().getFileName());
            return c != 0 ? c : a.getFileName().compareIgnoreCase (b.getFileName());
        }
    } cmp;
    files.sort (cmp);
    return files;
}

void K55Processor::writeSourceInfo (juce::ValueTree& tree) const
{
    tree.setProperty ("samplePath", sample != nullptr ? sample->file.getFullPathName() : juce::String(), nullptr);
    tree.setProperty ("wtPath", userWT != nullptr ? userWTFile.getFullPathName() : juce::String(), nullptr);
    tree.setProperty ("wtFromSample", userWTFromSample, nullptr);
}

void K55Processor::restoreSources (const juce::ValueTree& tree)
{
    juce::String err;
    const juce::File sf (tree.getProperty ("samplePath", "").toString());
    if (tree.hasProperty ("samplePath"))
    {
        if (tree.getProperty ("samplePath", "").toString().isEmpty()) sample.reset();
        else if (sf.existsAsFile() && (sample == nullptr || sample->file != sf)) loadSampleFile (sf, err);
    }
    const juce::File wf (tree.getProperty ("wtPath", "").toString());
    if (wf.existsAsFile() && (userWT == nullptr || userWTFile != wf))
    {
        if ((bool) tree.getProperty ("wtFromSample", false) && sample != nullptr && sample->file == wf) sampleToWavetable();
        else loadWavetableFile (wf, err);
    }
    publishSources();
}

bool K55Processor::saveUserPreset (const juce::String& name, const juce::String& category)
{
    auto folder = getUserPresetFolder().getChildFile (juce::File::createLegalFileName (category.isEmpty() ? "User" : category));
    if (! folder.createDirectory()) return false;
    auto state = apvts.copyState();
    state.setProperty ("presetName", name, nullptr);
    writeSourceInfo (state);
    if (auto xml = state.createXml())
    {
        auto file = folder.getChildFile (juce::File::createLegalFileName (name)).withFileExtension ("k54spreset");
        if (xml->writeTo (file))
        {
            presetName = name;
            ++presetVersion;
            return true;
        }
    }
    return false;
}

bool K55Processor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;
    auto tree = juce::ValueTree::fromXml (*xml);
    resetToDefaults();
    for (const auto& child : tree)
        if (child.hasType ("PARAM") && ! ParamID::isGlobalParam (child["id"].toString()))
            if (auto* p = apvts.getParameter (child["id"].toString()))
                p->setValueNotifyingHost (p->convertTo0to1 ((float) child["value"]));
    restoreSources (tree);
    presetName = tree.getProperty ("presetName", file.getFileNameWithoutExtension()).toString();
    ++presetVersion;
    return true;
}

// ---------------------------------------------------------------- Zustand im Projekt
void K55Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("uiScale", uiScale, nullptr);
    writeSourceInfo (state);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void K55Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            presetName = tree.getProperty ("presetName", "").toString();
            currentProgram = (int) tree.getProperty ("program", 0);
            uiScale = juce::jlimit (0.7f, 1.6f, (float) tree.getProperty ("uiScale", 1.0f));
            apvts.replaceState (tree);
            auto copy = tree.createCopy();
            if (juce::MessageManager::getInstance()->isThisTheMessageThread())
                restoreSources (copy);
            else
                juce::MessageManager::callAsync ([safe = juce::WeakReference<K55Processor> (this), copy]
                {
                    if (auto* p = safe.get()) p->restoreSources (copy);
                });
            ++presetVersion;
        }
}

juce::AudioProcessorEditor* K55Processor::createEditor() { return new K55Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new K55Processor(); }

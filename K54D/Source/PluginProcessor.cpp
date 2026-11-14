#include "PluginProcessor.h"
#include "PluginEditor.h"

static const char* laneKeys[11] { "model", "level", "tune", "decay", "tone", "snap", "drive", "pan", "room", "mute", "solo" };

K54DProcessor::K54DProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "K54D", createLayout())
{
    for (int l = 0; l < DrumInfo::numLanes; ++l)
        for (int k = 0; k < 11; ++k)
            lanePtr[(size_t) l].p[k] = apvts.getRawParameterValue (PID::lane (l, laneKeys[k]));
    pSeqOn   = apvts.getRawParameterValue (PID::seqOn);
    pSteps   = apvts.getRawParameterValue (PID::steps);
    pRes     = apvts.getRawParameterValue (PID::res);
    pSwing   = apvts.getRawParameterValue (PID::swing);
    pHuman   = apvts.getRawParameterValue (PID::humanize);
    pPattern = apvts.getRawParameterValue (PID::pattern);
    pFat     = apvts.getRawParameterValue (PID::fat);
    pGlue    = apvts.getRawParameterValue (PID::glue);
    pRoom    = apvts.getRawParameterValue (PID::roomSize);
    pMaster  = apvts.getRawParameterValue (PID::master);

    // Start mit einem Groove, damit sofort etwas klingt
    for (int i = 0; i < Presets::numFactory(); ++i)
        if (juce::String (Presets::factory (i).name) == "909 Classic") { loadFactory (i); break; }
}

bool K54DProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void K54DProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    engine.prepare (sampleRate);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    roomBuf.setSize (2, juce::jmax (512, samplesPerBlock), false, true, true);
    lastFat = -1.0f;
    compEnv = 0.0f; compGain = 1.0f;
    for (int c = 0; c < 2; ++c) shelf.z1[c] = shelf.z2[c] = 0.0f;
}

int K54DProcessor::stepsNow() const { return 8 + juce::roundToInt (pSteps->load()); }

void K54DProcessor::selectPattern (int p)
{
    p = juce::jlimit (0, DrumInfo::numPatterns - 1, p);
    if (running.load()) pendingPattern = p;
    else { currentPattern = p; pendingPattern = -1; }
}

void K54DProcessor::triggerPad (int lane, float velocity)
{
    padVel = velocity;
    padLane = lane;
}

void K54DProcessor::readLaneParams()
{
    bool anySolo = false;
    for (int l = 0; l < DrumInfo::numLanes; ++l) anySolo |= lanePtr[(size_t) l].p[10]->load() > 0.5f;
    for (int l = 0; l < DrumInfo::numLanes; ++l)
    {
        auto& p = lanePtr[(size_t) l].p;
        auto& o = lp[(size_t) l];
        o.model = juce::roundToInt (p[0]->load());
        o.level = p[1]->load(); o.tune = p[2]->load(); o.decay = p[3]->load(); o.tone = p[4]->load();
        o.snap  = p[5]->load(); o.drive = p[6]->load(); o.pan = p[7]->load(); o.room = p[8]->load();
        o.mute  = p[9]->load() > 0.5f || (anySolo && p[10]->load() < 0.5f);
    }
}

void K54DProcessor::addEvent (int offset, int lane, float vel)
{
    if (numEvents < (int) events.size()) events[(size_t) numEvents++] = { offset, lane, vel };
}

void K54DProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (roomBuf.getNumSamples() < n) roomBuf.setSize (2, n, false, false, true);
    roomBuf.clear (0, n);
    numEvents = 0;
    readLaneParams();

    // ------------------------------------------------ MIDI: Noten spielen die Spuren, C0-D#0 waehlen Pattern A-D
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            const int note = m.getNoteNumber();
            if (note >= 24 && note <= 27) { selectPattern (note - 24); continue; }
            const int lane = DrumInfo::laneForNote (note);
            if (lane >= 0) addEvent (meta.samplePosition, lane, m.getFloatVelocity());
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            engine.allOff();
    }
    if (const int pl = padLane.exchange (-1); pl >= 0) addEvent (0, pl, padVel.load());

    // ------------------------------------------------ Pattern-Wahl aus dem Parameter
    const float patParam = pPattern->load();
    if (patParam != lastPatternParam)
    {
        if (lastPatternParam >= 0.0f) selectPattern (juce::roundToInt (patParam));
        else currentPattern = juce::roundToInt (patParam);
        lastPatternParam = patParam;
    }

    // ------------------------------------------------ Sequencer im DAW-Tempo
    double bpm = 120.0, ppqStart = 0.0;
    bool hostRun = false, havePpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            hostRun = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { ppqStart = *q; havePpq = true; }
        }
    bpm = juce::jlimit (20.0, 400.0, bpm);
    hostBpm = bpm;
    hostPlaying = hostRun;

    bool run = false;
    if (hostRun && havePpq) { run = true; internalPpq = ppqStart; }
    else if (internalPlay.load()) { run = true; ppqStart = internalPpq; }
    else internalPpq = 0.0;

    const bool seq = pSeqOn->load() > 0.5f;
    running = run && seq;
    const double samplesPerPpq = sr * 60.0 / bpm;
    const double ppqEnd = ppqStart + n / samplesPerPpq;

    if (run && seq)
    {
        const int steps = stepsNow();
        const double len = Res::stepPpq (juce::roundToInt (pRes->load()));
        const double swingOff = pSwing->load() * 0.5 * len;
        const float human = pHuman->load();
        const auto firstN = (juce::int64) std::floor ((ppqStart - len) / len);
        const auto lastN  = (juce::int64) std::floor (ppqEnd / len);

        for (auto k = firstN; k <= lastN; ++k)
        {
            const double base = (double) k * len + ((k & 1) != 0 ? swingOff : 0.0);
            const int idx = (int) (((k % steps) + steps) % steps);
            if (idx == 0 && base >= ppqStart && base < ppqEnd && pendingPattern.load() >= 0)
                currentPattern = pendingPattern.exchange (-1);
            const int pat = currentPattern.load();

            for (int lane = 0; lane < DrumInfo::numLanes; ++lane)
            {
                const int v = patterns.vel (pat, lane, idx);
                if (v <= 0) continue;
                const int rat = patterns.ratchet (pat, lane, idx);
                for (int h = 0; h < rat; ++h)
                {
                    const double ev = base + h * len / rat;
                    if (ev < ppqStart || ev >= ppqEnd) continue;
                    int off = (int) std::lround ((ev - ppqStart) * samplesPerPpq);
                    float vel = (float) v / 127.0f * (h == 0 ? 1.0f : 0.82f);
                    if (human > 0.0f)
                    {
                        vel *= 1.0f - human * 0.3f * rng.nextFloat();
                        off += (int) (rng.nextFloat() * human * 0.012 * sr);
                    }
                    addEvent (juce::jlimit (0, n - 1, off), lane, juce::jlimit (0.02f, 1.0f, vel));
                }
            }
        }
        const auto cur = (juce::int64) std::floor (ppqStart / len);
        playingStep = (int) (((cur % steps) + steps) % steps);
    }
    else
    {
        playingStep = -1;
        if (pendingPattern.load() >= 0) currentPattern = pendingPattern.exchange (-1);
    }
    if (run && ! hostRun) internalPpq = ppqEnd;

    // ------------------------------------------------ Stimmen rendern
    std::sort (events.begin(), events.begin() + numEvents, [] (const Ev& a, const Ev& b) { return a.offset < b.offset; });
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    float* RL = roomBuf.getWritePointer (0);
    float* RR = roomBuf.getWritePointer (1);
    int pos = 0;
    for (int e = 0; e < numEvents; ++e)
    {
        const auto& ev = events[(size_t) e];
        engine.render (L, R, RL, RR, pos, ev.offset);
        pos = ev.offset;
        const auto& p = lp[(size_t) ev.lane];
        if (! p.mute)
        {
            engine.trigger (ev.lane, ev.vel, p, p.model >= DrumInfo::sampleModel (ev.lane) ? getSample (ev.lane) : nullptr);
            laneHits[(size_t) ev.lane].fetch_add (1);
        }
    }
    engine.render (L, R, RL, RR, pos, n);

    // ------------------------------------------------ Raum (Hall-Send)
    {
        juce::Reverb::Parameters rp;
        const float size = pRoom->load();
        rp.roomSize = 0.25f + 0.7f * size;
        rp.damping  = 0.5f - 0.2f * size;
        rp.wetLevel = 1.0f; rp.dryLevel = 0.0f; rp.width = 1.0f;
        reverb.setParameters (rp);
        reverb.processStereo (RL, RR, n);
        for (int i = 0; i < n; ++i) { L[i] += RL[i] * 0.45f; R[i] += RR[i] * 0.45f; }
    }

    // ------------------------------------------------ FAT: Low-Shelf + Saettigung
    const float fat = pFat->load();
    if (std::abs (fat - lastFat) > 0.001f)
    {
        lastFat = fat;
        // RBJ Low-Shelf 85 Hz, bis +5 dB
        const double A = std::pow (10.0, (fat * 5.0) / 40.0);
        const double w0 = juce::MathConstants<double>::twoPi * 85.0 / sr;
        const double cs = std::cos (w0), sn = std::sin (w0);
        const double alpha = sn / 2.0 * std::sqrt (2.0);
        const double sq = 2.0 * std::sqrt (A) * alpha;
        const double a0 = (A + 1) + (A - 1) * cs + sq;
        shelf.b0 = (float) (A * ((A + 1) - (A - 1) * cs + sq) / a0);
        shelf.b1 = (float) (2 * A * ((A - 1) - (A + 1) * cs) / a0);
        shelf.b2 = (float) (A * ((A + 1) - (A - 1) * cs - sq) / a0);
        shelf.a1 = (float) (-2 * ((A - 1) + (A + 1) * cs) / a0);
        shelf.a2 = (float) (((A + 1) + (A - 1) * cs - sq) / a0);
    }
    buffer.applyGain (0.56f);            // Headroom fuer die Summe (ca. -5 dB)
    const float k = 1.0f + 3.0f * fat;
    const float norm = 1.0f / std::tanh (k);
    for (int c = 0; c < 2; ++c)
    {
        float* d = buffer.getWritePointer (c);
        auto& z1 = shelf.z1[c]; auto& z2 = shelf.z2[c];
        for (int i = 0; i < n; ++i)
        {
            const float x = d[i];
            const float y = shelf.b0 * x + z1;
            z1 = shelf.b1 * x - shelf.a1 * y + z2;
            z2 = shelf.b2 * x - shelf.a2 * y;
            d[i] = (1.0f - 0.6f * fat) * y + 0.6f * fat * std::tanh (k * y) * norm * 0.6f;
        }
    }

    // ------------------------------------------------ GLUE: Bus-Kompressor
    const float glue = pGlue->load();
    const float thr = juce::Decibels::decibelsToGain (-4.0f - glue * 16.0f);
    const float ratio = 1.5f + glue * 3.5f;
    const float makeup = juce::Decibels::decibelsToGain (glue * 3.0f);
    const float att = std::exp (-1.0f / (0.004f * (float) sr));
    const float rel = std::exp (-1.0f / (0.14f * (float) sr));
    const float gSmooth = std::exp (-1.0f / (0.002f * (float) sr));
    const float master = juce::Decibels::decibelsToGain (pMaster->load());
    float pk[2] { 0.0f, 0.0f };
    float minGain = 1.0f;
    for (int i = 0; i < n; ++i)
    {
        const float lvl = juce::jmax (std::abs (L[i]), std::abs (R[i]));
        compEnv = lvl > compEnv ? att * compEnv + (1 - att) * lvl : rel * compEnv + (1 - rel) * lvl;
        float target = 1.0f;
        if (glue > 0.001f && compEnv > thr)
            target = std::pow (compEnv / thr, 1.0f / ratio - 1.0f);
        compGain = gSmooth * compGain + (1 - gSmooth) * target;
        minGain = juce::jmin (minGain, compGain);
        const float g = compGain * (glue > 0.001f ? makeup : 1.0f) * master;
        for (int c = 0; c < 2; ++c)
        {
            float* d = c == 0 ? L : R;
            float y = d[i] * g;
            // weicher Schutz gegen Uebersteuern
            const float a = std::abs (y);
            if (a > 0.92f) y = std::copysign (0.92f + 0.08f * std::tanh ((a - 0.92f) / 0.08f), y);
            d[i] = y;
            pk[c] = juce::jmax (pk[c], std::abs (y));
        }
    }
    grDb = juce::Decibels::gainToDecibels (minGain);
    for (int c = 0; c < 2; ++c) meter[(size_t) c] = pk[c];
    midi.clear();
}

// ============================================================================ Presets
int K54DProcessor::getNumPrograms() { return Presets::numFactory(); }

void K54DProcessor::setCurrentProgram (int index) { loadFactory (index); }

const juce::String K54DProcessor::getProgramName (int index)
{
    const auto& f = Presets::factory (index);
    return juce::String (f.category) + " - " + f.name;
}

void K54DProcessor::loadFactory (int index)
{
    Presets::applyFactory (index, apvts, patterns);
    for (int l = 0; l < DrumInfo::numLanes; ++l) clearSample (l);
    currentFactory = index;
    presetName = Presets::factory (index).name;
    currentPattern = 0; pendingPattern = -1;
    lastPatternParam = 0.0f;
    stateVersion.fetch_add (1);
}

juce::File K54DProcessor::userPresetFolder() const
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Keller54").getChildFile ("K-54 D Presets");
}

juce::Array<juce::File> K54DProcessor::userPresets() const
{
    auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.k54dpreset");
    files.sort();
    return files;
}

bool K54DProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto dir = userPresetFolder();
    dir.createDirectory();
    auto t = stateTree (false);
    t.setProperty ("presetName", name.trim(), nullptr);
    if (auto xml = t.createXml())
        if (xml->writeTo (dir.getChildFile (clean + ".k54dpreset")))
        {
            presetName = name.trim();
            currentFactory = -1;
            return true;
        }
    return false;
}

bool K54DProcessor::loadUserPreset (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
    {
        restoreTree (juce::ValueTree::fromXml (*xml), false);
        presetName = f.getFileNameWithoutExtension();
        currentFactory = -1;
        currentPattern = 0; pendingPattern = -1;
        stateVersion.fetch_add (1);
        return true;
    }
    return false;
}

// ============================================================================ Samples
bool K54DProcessor::loadSample (int lane, const juce::File& file)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0) return false;

    auto s = std::make_shared<DrumSample>();
    const int len = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 8.0));
    s->audio.setSize (2, len + 1);
    s->audio.clear();
    reader->read (&s->audio, 0, len, 0, true, reader->numChannels > 1);
    if (reader->numChannels == 1) s->audio.copyFrom (1, 0, s->audio, 0, 0, len);
    s->sampleRate = reader->sampleRate;
    s->name = file.getFileNameWithoutExtension();
    std::atomic_store (&samples[(size_t) lane], std::shared_ptr<const DrumSample> (s));

    if (auto* prm = apvts.getParameter (PID::lane (lane, "model")))
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) DrumInfo::sampleModel (lane)));
    return true;
}

void K54DProcessor::clearSample (int lane)
{
    std::atomic_store (&samples[(size_t) lane], std::shared_ptr<const DrumSample>());
}

juce::String K54DProcessor::sampleName (int lane) const
{
    auto s = getSample (lane);
    return s != nullptr ? s->name : juce::String();
}

// ============================================================================ MIDI-Export
juce::File K54DProcessor::exportMidi (int pattern, const juce::File& target) const
{
    const int steps = stepsNow();
    const double len = Res::stepPpq (juce::roundToInt (pRes->load()));
    const double swingOff = pSwing->load() * 0.5 * len;
    const int tpq = 960;
    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, "K-54 D " + presetName + " " + juce::String::charToString ((juce::juce_wchar) ('A' + pattern))), 0);
    for (int s = 0; s < steps; ++s)
    {
        const double base = s * len + ((s & 1) != 0 ? swingOff : 0.0);
        for (int l = 0; l < DrumInfo::numLanes; ++l)
        {
            const int v = patterns.vel (pattern, l, s);
            if (v <= 0) continue;
            const int rat = patterns.ratchet (pattern, l, s);
            for (int h = 0; h < rat; ++h)
            {
                const double t = (base + h * len / rat) * tpq;
                const auto vel = (juce::uint8) juce::jlimit (1, 127, h == 0 ? v : juce::roundToInt (v * 0.82f));
                seq.addEvent (juce::MidiMessage::noteOn (10, DrumInfo::laneNote[l], vel), t);
                seq.addEvent (juce::MidiMessage::noteOff (10, DrumInfo::laneNote[l]), t + tpq * len / rat * 0.5);
            }
        }
    }
    seq.addEvent (juce::MidiMessage::endOfTrack(), steps * len * tpq);
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (seq);

    auto file = target;
    if (file == juce::File())
        file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                   .getChildFile (juce::File::createLegalFileName ("K-54 D " + presetName + " " + juce::String::charToString ((juce::juce_wchar) ('A' + pattern))) + ".mid");
    file.deleteFile();
    juce::FileOutputStream os (file);
    if (os.openedOk()) { mf.writeTo (os); os.flush(); }
    return file;
}

// ============================================================================ Zustand
juce::ValueTree K54DProcessor::stateTree (bool withUi)
{
    auto t = apvts.copyState();
    t.removeChild (t.getChildWithName ("PATTERNS"), nullptr);
    t.removeChild (t.getChildWithName ("SAMPLES"), nullptr);
    t.appendChild (patterns.toTree(), nullptr);

    juce::ValueTree smp ("SAMPLES");
    for (int l = 0; l < DrumInfo::numLanes; ++l)
        if (auto s = getSample (l))
        {
            juce::MemoryBlock mb;
            {
                auto* raw = new juce::MemoryOutputStream (mb, false);
                juce::WavAudioFormat wav;
                std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (raw, s->sampleRate, 2, 16, {}, 0));
                if (w != nullptr) w->writeFromAudioSampleBuffer (s->audio, 0, s->audio.getNumSamples() - 1);
                else delete raw;
            }
            juce::ValueTree st ("S");
            st.setProperty ("lane", l, nullptr);
            st.setProperty ("name", s->name, nullptr);
            st.setProperty ("wav", mb.toBase64Encoding(), nullptr);
            smp.appendChild (st, nullptr);
        }
    t.appendChild (smp, nullptr);

    t.setProperty ("presetName", presetName, nullptr);
    t.setProperty ("factory", currentFactory, nullptr);
    if (withUi)
    {
        t.setProperty ("uiScale", uiScale, nullptr);
        t.setProperty ("selectedLane", selectedLane, nullptr);
    }
    return t;
}

void K54DProcessor::restoreTree (const juce::ValueTree& t, bool withUi)
{
    if (! t.isValid()) return;
    auto params = t.createCopy();
    patterns.fromTree (t.getChildWithName ("PATTERNS"));
    for (int l = 0; l < DrumInfo::numLanes; ++l) clearSample (l);
    for (auto st : t.getChildWithName ("SAMPLES"))
    {
        const int l = st.getProperty ("lane", -1);
        if (l < 0 || l >= DrumInfo::numLanes) continue;
        juce::MemoryBlock mb;
        if (! mb.fromBase64Encoding (st.getProperty ("wav").toString())) continue;
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatReader> r (wav.createReaderFor (new juce::MemoryInputStream (mb, false), true));
        if (r == nullptr) continue;
        auto s = std::make_shared<DrumSample>();
        const int len = (int) r->lengthInSamples;
        s->audio.setSize (2, len + 1);
        s->audio.clear();
        r->read (&s->audio, 0, len, 0, true, true);
        s->sampleRate = r->sampleRate;
        s->name = st.getProperty ("name").toString();
        std::atomic_store (&samples[(size_t) l], std::shared_ptr<const DrumSample> (s));
    }
    params.removeChild (params.getChildWithName ("PATTERNS"), nullptr);
    params.removeChild (params.getChildWithName ("SAMPLES"), nullptr);
    if (params.hasType (apvts.state.getType()))
        apvts.replaceState (params);
    presetName = t.getProperty ("presetName", "Init").toString();
    currentFactory = t.getProperty ("factory", -1);
    if (withUi)
    {
        uiScale = (float) t.getProperty ("uiScale", 1.0f);
        selectedLane = juce::jlimit (0, DrumInfo::numLanes - 1, (int) t.getProperty ("selectedLane", 0));
    }
    lastPatternParam = -1.0f;
}

void K54DProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = stateTree (true).createXml())
        copyXmlToBinary (*xml, destData);
}

void K54DProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        restoreTree (juce::ValueTree::fromXml (*xml), true);
        stateVersion.fetch_add (1);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new K54DProcessor(); }

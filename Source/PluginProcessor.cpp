#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessor::BusesProperties K54Processor::makeBuses()
{
    auto b = BusesProperties()
                 .withInput ("Input", juce::AudioChannelSet::stereo(), true);   // Audio-Interface fuer den Looper
    for (int i = 0; i < K54Processor::numOutBuses; ++i)
        b = b.withOutput (K54Processor::outBusName (i), juce::AudioChannelSet::stereo(), i == 0);
    return b;
}

juce::String K54Processor::outBusName (int bus)
{
    if (bus == 0)        return "Main";
    if (bus == busSynth) return "Synth";
    if (bus >= busDrum0 && bus < busDrum0 + DrumInfo::numLanes) return DrumInfo::names[bus - busDrum0];
    if (bus >= busLoop0 && bus < busLoop0 + Looper::numTracks)  return "Loop " + juce::String (bus - busLoop0 + 1);
    return "Out " + juce::String (bus + 1);
}

K54Processor::K54Processor()
    : AudioProcessor (makeBuses()),
      apvts (*this, nullptr, "K54", createParameterLayout())
{
    static const char* what[dNum] { "Level", "Tune", "Decay", "Tone", "Pan", "Mute", "Out" };
    for (int l = 0; l < DrumInfo::numLanes; ++l)
        for (int w = 0; w < dNum; ++w)
            drumP[(size_t) l][(size_t) w] = apvts.getRawParameterValue (ParamID::drum (l, what[w]));
    for (int t = 0; t < Looper::numTracks; ++t)
        outLoopP[(size_t) t] = apvts.getRawParameterValue (ParamID::outLoop (t));
    loadFactoryPreset (0);
}

bool K54Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Main: Mono oder Stereo; Einzelausgaenge: Stereo oder aus; Eingang: Mono, Stereo oder aus
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    for (int i = 1; i < layouts.outputBuses.size(); ++i)
    {
        const auto& set = layouts.outputBuses.getReference (i);
        if (! set.isDisabled() && set != juce::AudioChannelSet::stereo()) return false;
    }
    for (auto& in : layouts.inputBuses)
        if (! in.isDisabled() && in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()) return false;
    return true;
}

bool K54Processor::isOutBusActive (int bus) const
{
    if (auto* b = getBus (false, bus))
        return b->isEnabled() && b->getNumberOfChannels() >= 2;
    return false;
}

void K54Processor::ensureBuffers (int n)
{
    auto sz = [n] (juce::AudioBuffer<float>& b)
    {
        if (b.getNumChannels() != 2 || b.getNumSamples() < n) b.setSize (2, n, false, false, true);
    };
    sz (synthBuf); sz (clickBuf);
    for (auto& b : drumBufs) sz (b);
    for (auto& b : loopBufs) sz (b);
}

void K54Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    readParams();
    engine.setParams (sp);
    engine.prepare (sampleRate, samplesPerBlock);
    looper.prepare (sampleRate, samplesPerBlock);
    sequencer.prepare (sampleRate);
    drums.prepare (sampleRate);
    inputBuf.setSize (2, juce::jmax (samplesPerBlock, 4096), false, true, false);
    ensureBuffers (juce::jmax (samplesPerBlock, 4096));
    hits.clear(); hits.reserve (512);
    synthMidi.ensureSize (4096);
}

void K54Processor::readParams()
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
    sp.wave[0]    = i (ParamID::osc1Wave);
    sp.wave[1]    = i (ParamID::osc2Wave);
    sp.wave[2]    = i (ParamID::osc3Wave);
    sp.octave[0]  = i (ParamID::osc1Oct);
    sp.octave[1]  = i (ParamID::osc2Oct);
    sp.octave[2]  = i (ParamID::osc3Oct);
    sp.tune[0]    = 0.0f;
    sp.tune[1]    = f (ParamID::osc2Tune);
    sp.tune[2]    = f (ParamID::osc3Tune);
    sp.level[0]   = f (ParamID::osc1Level);
    sp.level[1]   = f (ParamID::osc2Level);
    sp.level[2]   = f (ParamID::osc3Level);
    sp.noise      = f (ParamID::noiseLevel);
    sp.pulseWidth = f (ParamID::pulseWidth);
    sp.lfoRate    = f (ParamID::lfoRate);
    sp.lfoPitch   = f (ParamID::lfoPitch);
    sp.lfoFilter  = f (ParamID::lfoFilter);
    sp.lfoWave    = i (ParamID::lfoWave);
    sp.voiceMode  = i (ParamID::voiceMode);
    sp.bendRange  = f (ParamID::bendRange);
    sp.drift      = f (ParamID::drift);
}

void K54Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    const int n = buffer.getNumSamples();

    // ---- Audio-Eingang sichern, bevor der Synth den Puffer ueberschreibt
    const int numIn = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    if (inputBuf.getNumSamples() < n) inputBuf.setSize (2, n, false, false, true);
    inputBuf.clear (0, n);
    if (numIn > 0)
    {
        const int mode = juce::roundToInt (raw (ParamID::loopInMode)->load());
        const float g  = juce::Decibels::decibelsToGain (raw (ParamID::loopInGain)->load());
        const float* i0 = buffer.getReadPointer (0);
        const float* i1 = buffer.getReadPointer (numIn > 1 ? 1 : 0);
        float* l = inputBuf.getWritePointer (0);
        float* r = inputBuf.getWritePointer (1);
        for (int s = 0; s < n; ++s)
        {
            float a = 0.0f, b = 0.0f;
            switch (mode)
            {
                case 1:  a = b = i0[s]; break;                       // Mono In 1
                case 2:  a = b = i1[s]; break;                       // Mono In 2
                case 3:  a = b = 0.5f * (i0[s] + i1[s]); break;      // Mono 1+2
                default: a = i0[s]; b = i1[s]; break;                // Stereo
            }
            l[s] = a * g;
            r[s] = b * g;
        }
    }
    for (int ch = 0; ch < 2; ++ch)
    {
        const float pk = numIn > 0 ? inputBuf.getMagnitude (ch, 0, n) : 0.0f;
        inPeak[ch].store (juce::jmax (pk, inPeak[ch].load() * 0.9f));
    }

    readParams();
    engine.setParams (sp);
    ensureBuffers (n);

    // ---- Looper-/Sequencer-Einstellungen
    Looper::Settings ls;
    ls.internalBpm = raw (ParamID::loopBpm)->load();
    ls.lengthBars  = ParamChoices::loopBarValues[juce::jlimit (0, 3, juce::roundToInt (raw (ParamID::loopBars)->load()))];
    ls.click       = raw (ParamID::loopClick)->load() > 0.5f;
    ls.volume[0]   = raw (ParamID::loop1Vol)->load();
    ls.volume[1]   = raw (ParamID::loop2Vol)->load();
    ls.volume[2]   = raw (ParamID::loop3Vol)->load();
    ls.volume[3]   = raw (ParamID::loop4Vol)->load();
    ls.source[0]   = juce::roundToInt (raw (ParamID::loop1Src)->load());
    ls.source[1]   = juce::roundToInt (raw (ParamID::loop2Src)->load());
    ls.source[2]   = juce::roundToInt (raw (ParamID::loop3Src)->load());
    ls.source[3]   = juce::roundToInt (raw (ParamID::loop4Src)->load());
    ls.monitor     = raw (ParamID::loopMonitor)->load() > 0.5f;

    juce::Optional<juce::AudioPlayHead::PositionInfo> pos;
    if (auto* ph = getPlayHead())
        pos = ph->getPosition();

    // ---- 1) Takt fuer diesen Block (DAW, MIDI-Clock oder intern)
    const ClockInfo clk = looper.beginBlock (midi, pos, ls);

    // ---- 2) MIDI aufteilen: Kanal 10 = Drums, Rest = Synth
    hits.clear();
    synthMidi.clear();
    for (const auto meta : midi)
    {
        const auto& m = meta.getMessage();
        if (m.getChannel() == 10 && (m.isNoteOn() || m.isNoteOff()))
        {
            if (m.isNoteOn())
            {
                const int lane = DrumInfo::laneForNote (m.getNoteNumber());
                if (lane >= 0) hits.push_back ({ lane, meta.samplePosition, m.getFloatVelocity() });
            }
            continue;
        }
        synthMidi.addEvent (m, meta.samplePosition);
    }

    // ---- 3) Step-Sequencer erzeugt Drum-Schlaege und Synth-Noten
    StepSequencer::Settings ss;
    ss.drumsOn     = raw (ParamID::seqDrumsOn)->load() > 0.5f;
    ss.synthOn     = raw (ParamID::seqSynthOn)->load() > 0.5f;
    ss.pattern     = juce::jlimit (0, StepSequencer::numPatterns - 1, juce::roundToInt (raw (ParamID::seqPattern)->load()));
    ss.drumLength  = juce::roundToInt (raw (ParamID::seqDrumLen)->load()) + 1;    // Auswahl 0..15 = 1..16 Schritte
    ss.synthLength = juce::roundToInt (raw (ParamID::seqSynthLen)->load()) + 1;
    ss.swing       = raw (ParamID::seqSwing)->load();
    ss.rootNote    = juce::roundToInt (raw (ParamID::seqRoot)->load());
    ss.gate        = raw (ParamID::seqGate)->load();
    sequencer.process (clk, n, ss, hits, synthMidi);
    std::stable_sort (hits.begin(), hits.end(), [] (const auto& a, const auto& b) { return a.offset < b.offset; });

    // ---- 4) Synth
    synthView.setDataToReferTo (synthBuf.getArrayOfWritePointers(), 2, n);
    engine.render (synthView, synthMidi);

    // ---- 5) Drums
    std::array<DrumLaneParams, DrumInfo::numLanes> dp;
    const float drumGain = juce::Decibels::decibelsToGain (raw (ParamID::drumMaster)->load());
    std::array<juce::AudioBuffer<float>*, DrumInfo::numLanes> drumOuts;
    for (int l = 0; l < DrumInfo::numLanes; ++l)
    {
        auto& d = dp[(size_t) l];
        const auto& P = drumP[(size_t) l];
        d.level = P[dLevel]->load() * drumGain;
        d.tune  = P[dTune]->load();
        d.decay = P[dDecay]->load();
        d.tone  = P[dTone]->load();
        d.pan   = P[dPan]->load();
        d.mute  = P[dMute]->load() > 0.5f;
        drumBufs[(size_t) l].clear (0, n);
        drumViews[(size_t) l].setDataToReferTo (drumBufs[(size_t) l].getArrayOfWritePointers(), 2, n);
        drumOuts[(size_t) l] = &drumViews[(size_t) l];
    }
    drums.render (hits.data(), (int) hits.size(), dp, drumOuts, n);

    // ---- 6) Looper: nimmt den Synth auf, jede Spur in ihren eigenen Puffer
    juce::AudioBuffer<float>* loopOuts[Looper::numTracks];
    for (int t = 0; t < Looper::numTracks; ++t)
    {
        loopBufs[(size_t) t].clear (0, n);
        loopViews[(size_t) t].setDataToReferTo (loopBufs[(size_t) t].getArrayOfWritePointers(), 2, n);
        loopOuts[t] = &loopViews[(size_t) t];
    }
    clickBuf.clear (0, n);
    clickView.setDataToReferTo (clickBuf.getArrayOfWritePointers(), 2, n);
    looper.renderBlock (synthView, numIn > 0 ? &inputBuf : nullptr, loopOuts, &clickView, ls);

    // ---- 7) Verteilen: Main oder Einzelausgang
    buffer.clear();
    auto mainBus = getBusBuffer (buffer, false, 0);
    const int mainCh = mainBus.getNumChannels();
    auto route = [&] (const juce::AudioBuffer<float>& src, bool separate, int bus)
    {
        if (separate && isOutBusActive (bus))
        {
            auto ob = getBusBuffer (buffer, false, bus);
            for (int ch = 0; ch < juce::jmin (2, ob.getNumChannels()); ++ch)
                ob.addFrom (ch, 0, src, ch, 0, n);
            return;
        }
        if (mainCh == 1)
        {
            mainBus.addFrom (0, 0, src, 0, 0, n, 0.5f);
            mainBus.addFrom (0, 0, src, 1, 0, n, 0.5f);
        }
        else
            for (int ch = 0; ch < juce::jmin (2, mainCh); ++ch)
                mainBus.addFrom (ch, 0, src, ch, 0, n);
    };
    route (synthView, raw (ParamID::outSynth)->load() > 0.5f, busSynth);
    for (int l = 0; l < DrumInfo::numLanes; ++l)
        route (drumViews[(size_t) l], drumP[(size_t) l][dOut]->load() > 0.5f, busDrum0 + l);
    for (int t = 0; t < Looper::numTracks; ++t)
        route (loopViews[(size_t) t], outLoopP[(size_t) t]->load() > 0.5f, busLoop0 + t);
    route (clickView, false, 0);
    if (numIn > 0 && ls.monitor)
    {
        juce::AudioBuffer<float> inView (inputBuf.getArrayOfWritePointers(), 2, n);
        route (inView, false, 0);
    }

    // weiche Begrenzung auf allen Ausgaengen
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int s = 0; s < n; ++s) d[s] = Looper::softLimit (d[s]);
    }
}

// ---------------------------------------------------------------- Presets
int K54Processor::getNumPrograms()
{
    return (int) getFactoryPresets().size();
}

void K54Processor::setCurrentProgram (int index)
{
    if (juce::isPositiveAndBelow (index, getNumPrograms()))
        loadFactoryPreset (index);
}

const juce::String K54Processor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, getNumPrograms()))
        return getFactoryPresets()[(size_t) index].name;
    return {};
}

void K54Processor::setParam (const juce::String& id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void K54Processor::resetToDefaults()
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! ParamID::isLooperParam (rp->getParameterID()))
                rp->setValueNotifyingHost (rp->getDefaultValue());
}

void K54Processor::loadFactoryPreset (int index)
{
    const auto& list = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    const auto& preset = list[(size_t) index];
    resetToDefaults();
    for (const auto& [id, value] : preset.values)
        setParam (id, value);

    currentProgram = index;
    presetName = preset.name;
    ++presetVersion;
}

juce::File K54Processor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory)
               .getChildFile ("Keller54").getChildFile ("K-54 Presets");
}

juce::Array<juce::File> K54Processor::getUserPresets() const
{
    auto files = getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.k54preset");
    files.sort();
    return files;
}

bool K54Processor::saveUserPreset (const juce::String& name)
{
    auto folder = getUserPresetFolder();
    if (! folder.createDirectory())
        return false;

    auto state = apvts.copyState();
    state.setProperty ("presetName", name, nullptr);
    if (auto xml = state.createXml())
    {
        auto file = folder.getChildFile (juce::File::createLegalFileName (name)).withFileExtension ("k54preset");
        if (xml->writeTo (file))
        {
            presetName = name;
            ++presetVersion;
            return true;
        }
    }
    return false;
}

bool K54Processor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    auto tree = juce::ValueTree::fromXml (*xml);
    resetToDefaults();
    for (const auto& child : tree)
    {
        if (child.hasType ("PARAM") && ! ParamID::isLooperParam (child["id"].toString()))
            if (auto* p = apvts.getParameter (child["id"].toString()))
                p->setValueNotifyingHost (p->convertTo0to1 ((float) child["value"]));
    }
    presetName = tree.getProperty ("presetName", file.getFileNameWithoutExtension()).toString();
    ++presetVersion;
    return true;
}

// ---------------------------------------------------------------- Zustand
void K54Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("program", currentProgram, nullptr);
    state.removeChild (state.getChildWithName ("PATTERNS"), nullptr);
    state.appendChild (sequencer.toValueTree(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void K54Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            presetName = tree.getProperty ("presetName", "").toString();
            currentProgram = (int) tree.getProperty ("program", 0);
            auto pats = tree.getChildWithName ("PATTERNS");
            if (pats.isValid())
            {
                sequencer.fromValueTree (pats);
                tree.removeChild (pats, nullptr);
            }
            apvts.replaceState (tree);
            ++presetVersion;
        }
}

juce::AudioProcessorEditor* K54Processor::createEditor()
{
    return new K54Editor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new K54Processor();
}

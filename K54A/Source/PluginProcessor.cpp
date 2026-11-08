// K-54 Analyzer · Keller54 · by Frank E.
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace k54a
{
static constexpr int scratchSize = 4096;

//==============================================================================
class AnalyzerProcessor::Loader : public juce::Thread
{
public:
    explicit Loader (AnalyzerProcessor& p) : juce::Thread ("K-54 Analyzer Referenz"), owner (p) {}

    void request (const juce::File& f, double rate)
    {
        {
            const juce::ScopedLock sl (lock);
            job = f;
            jobRate = rate;
            hasJob = true;
        }
        notify();
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            juce::File f;
            double rate = 0.0;
            {
                const juce::ScopedLock sl (lock);
                if (hasJob) { f = job; rate = jobRate; hasJob = false; }
            }

            if (f == juce::File())
            {
                wait (-1);
                continue;
            }

            juce::String error;
            auto stop = [this] { if (threadShouldExit()) return true; const juce::ScopedLock sl (lock); return hasJob; };
            auto data = owner.decodeReference (f, rate, error, stop);

            if (data != nullptr)
                owner.publishReference (std::move (data));
            else if (! stop())
                owner.setStatus (refError, error);
        }
    }

private:
    AnalyzerProcessor& owner;
    juce::CriticalSection lock;
    juce::File job;
    double jobRate = 48000.0;
    bool hasJob = false;
};

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AnalyzerProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<AudioParameterBool> (ParameterID { "ab", 1 }, "A/B (B = Referenz)", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "match", 1 }, "Lautheit angleichen", true));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "refGain", 1 }, "Referenz Gain",
                                                  NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                                                  AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "sync", 1 }, "DAW-Sync", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "showMix", 1 }, "Kurve Mix", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "showRef", 1 }, "Kurve Referenz", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "hold", 1 }, "Peak Hold", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "slope", 1 }, "Neigung",
                                                   StringArray { "0 dB/Okt.", "3 dB/Okt.", "4,5 dB/Okt.", "6 dB/Okt." }, 2));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "speed", 1 }, "Tempo",
                                                   StringArray { "Langsam", "Mittel", "Schnell" }, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "smooth", 1 }, "Glaettung",
                                                   StringArray { "Aus", "1/12 Okt.", "1/6 Okt.", "1/3 Okt." }, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "target", 1 }, "Ziel-Lautheit",
                                                   StringArray { "Kein Ziel", "-14 LUFS  Spotify / YouTube",
                                                                 "-16 LUFS  Apple Music", "-23 LUFS  EBU R128 (TV)",
                                                                 "-9 LUFS  Club / laut" }, 1));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "autoReset", 1 }, "Auto-Reset bei Play", true));
    return l;
}

AnalyzerProcessor::AnalyzerProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "K54A", createLayout())
{
    formatManager.registerBasicFormats();

    pAB        = apvts.getRawParameterValue ("ab");
    pMatch     = apvts.getRawParameterValue ("match");
    pRefGain   = apvts.getRawParameterValue ("refGain");
    pSync      = apvts.getRawParameterValue ("sync");
    pAutoReset = apvts.getRawParameterValue ("autoReset");

    refL.assign (scratchSize, 0.0f);
    refR.assign (scratchSize, 0.0f);
    mixMeter.prepare (48000.0);
    refMeter.prepare (48000.0);

    loader = std::make_unique<Loader> (*this);
    loader->startThread (juce::Thread::Priority::low);
    startTimer (1000);
}

AnalyzerProcessor::~AnalyzerProcessor()
{
    stopTimer();
    loader->signalThreadShouldExit();
    loader->notify();
    loader->stopThread (5000);

    delete activeRef.exchange (nullptr);
    const juce::ScopedLock sl (trashLock);
    for (auto& t : trash) delete t.first;
    trash.clear();
}

bool AnalyzerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void AnalyzerProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;
    hostRate.store (sr);

    mixMeter.prepare (sr);
    refMeter.prepare (sr);
    mixLevels.prepare (sr);
    refLevels.prepare (sr);
    refGain.reset (sr, 0.05);
    refGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pRefGain->load()));
    abPos = pAB->load() > 0.5f ? 1.0f : 0.0f;
    matchInit = false;

    juce::File pending;
    {
        const juce::ScopedLock sl (pendingLock);
        pending = pendingLoad;
        pendingLoad = juce::File();
    }

    if (pending.existsAsFile())
        loadReference (pending);
    else if (auto* r = getRef())
        if (std::abs (r->sampleRate - sr) > 0.5 && r->file.existsAsFile())
            loader->request (r->file, sr);
}

//==============================================================================
void AnalyzerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numIn = getTotalNumInputChannels();
    const int n = buffer.getNumSamples();

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    if (n <= 0 || numIn <= 0)
        return;

    bool playing = false;
    int64_t dawPos = -1;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (auto t = pos->getTimeInSamples())
                dawPos = *t;
        }

    if (dawPos >= 0)
        lastDawPos = dawPos;

    bool reset = resetRequested.exchange (false);
    if (pAutoReset->load() > 0.5f && playing && ! wasPlaying)
        reset = true;
    wasPlaying = playing;
    hostPlaying.store (playing);

    if (reset)
    {
        mixMeter.resetIntegrated();
        refMeter.resetIntegrated();
        mixLevels.maxPeak.store (0.0f);
        refLevels.maxPeak.store (0.0f);
        matchInit = false;
    }

    if (resetRefPos.exchange (false))
    {
        refOffset.store (0);
        freePos = 0;
        matchInit = false;
    }

    const int64_t seek = seekRequest.exchange (-1);
    if (seek >= 0)
    {
        if (pSync->load() > 0.5f) refOffset.store (seek - lastDawPos);
        else                      freePos = seek;
    }

    const bool stereo = numIn > 1;
    int done = 0;
    while (done < n)
    {
        const int m = juce::jmin (n - done, scratchSize);
        float* L = buffer.getWritePointer (0, done);
        float* R = stereo ? buffer.getWritePointer (1, done) : L;
        processChunk (L, R, stereo, m, playing, dawPos >= 0 ? dawPos + done : -1);
        done += m;
    }

    mixLoud.store (mixMeter);
    refLoud.store (refMeter);

    // Anzeige-Position der Referenz (auch bei gestoppter DAW)
    if (auto* ref = getRef())
    {
        const int64_t len = ref->length();
        int64_t p = pSync->load() > 0.5f ? lastDawPos + refOffset.load() : freePos;
        p = juce::jlimit<int64_t> (0, juce::jmax<int64_t> (0, len - 1), p);
        refPosSeconds.store ((double) p / sr);
    }
}

void AnalyzerProcessor::processChunk (float* L, float* R, bool stereo, int n, bool playing, int64_t dawPos)
{
    // ---- Mix messen -------------------------------------------------------
    mixMeter.process (L, stereo ? R : nullptr, n);
    mixLevels.process (L, R, n);
    mixFifo.push (L, R, n);

    // ---- Referenz ---------------------------------------------------------
    RefData* ref = getRef();
    float* rl = refL.data();
    float* rr = refR.data();
    bool active = false;

    if (ref != nullptr && ref->length() > 0)
    {
        const int64_t len = ref->length();
        const float* srcL = ref->audio.getReadPointer (0);
        const float* srcR = ref->audio.getReadPointer (1);

        if (pSync->load() > 0.5f)
        {
            if (playing && dawPos >= 0)
            {
                active = true;
                const int64_t start = dawPos + refOffset.load();
                for (int i = 0; i < n; ++i)
                {
                    const int64_t idx = start + i;
                    const bool inside = idx >= 0 && idx < len;
                    rl[i] = inside ? srcL[idx] : 0.0f;
                    rr[i] = inside ? srcR[idx] : 0.0f;
                }
            }
        }
        else if (freePlaying.load())
        {
            active = true;
            if (freePos < 0 || freePos >= len) freePos = 0;
            for (int i = 0; i < n; ++i)
            {
                rl[i] = srcL[freePos];
                rr[i] = srcR[freePos];
                if (++freePos >= len) freePos = 0;
            }
        }
    }

    refRunning.store (active);

    // ---- Angleich-Gain (alle ~100 ms nachführen) ---------------------------
    matchCounter += n;
    if (matchCounter >= (int) (sr * 0.1))
    {
        matchCounter = 0;
        if (ref != nullptr && ref->integrated > -70.0f)
        {
            const float mixL = mixMeter.hasIntegrated() ? mixMeter.integrated() : mixMeter.shortTerm();
            if (mixL > -60.0f)
            {
                const float target = juce::jlimit (-30.0f, 30.0f, mixL - ref->integrated);
                const float cur = matchGainDb.load();
                matchGainDb.store (matchInit ? cur + 0.35f * (target - cur) : target);
                matchInit = true;
            }
        }
    }

    const float gainDb = pRefGain->load() + (pMatch->load() > 0.5f ? matchGainDb.load() : 0.0f);
    refGain.setTargetValue (juce::Decibels::decibelsToGain (gainDb));

    if (active)
    {
        refMeter.process (rl, rr, n);      // Zahlen = Original-Datei (ohne Gain)
        refLevels.process (rl, rr, n);
        for (int i = 0; i < n; ++i)
        {
            const float g = refGain.getNextValue();
            rl[i] *= g;
            rr[i] *= g;
        }
        refFifo.push (rl, rr, n);          // Kurve = so wie du sie hörst
    }
    else
    {
        refGain.skip (n);
        std::fill (rl, rl + n, 0.0f);
        std::fill (rr, rr + n, 0.0f);
    }

    // ---- A/B mit kurzer Überblendung --------------------------------------
    const float targetAB = (pAB->load() > 0.5f && ref != nullptr) ? 1.0f : 0.0f;
    const float step = (float) (1.0 / (0.015 * sr));

    if (targetAB <= 0.0f && abPos <= 0.0f)
        return; // reiner Mix, nichts zu tun

    for (int i = 0; i < n; ++i)
    {
        if (abPos < targetAB)      abPos = juce::jmin (targetAB, abPos + step);
        else if (abPos > targetAB) abPos = juce::jmax (targetAB, abPos - step);

        const float a = 1.0f - abPos;
        if (stereo)
        {
            L[i] = L[i] * a + rl[i] * abPos;
            R[i] = R[i] * a + rr[i] * abPos;
        }
        else
        {
            L[i] = L[i] * a + 0.5f * (rl[i] + rr[i]) * abPos;
        }
    }
}

//==============================================================================
void AnalyzerProcessor::setStatus (int s, const juce::String& msg)
{
    {
        const juce::ScopedLock sl (msgLock);
        refMessage = msg;
    }
    refStatus.store (s);
    ++refVersion;
}

void AnalyzerProcessor::loadReference (const juce::File& file)
{
    if (! file.existsAsFile())
    {
        setStatus (refError, juce::String::fromUTF8 ("Datei nicht gefunden: ") + file.getFileName());
        return;
    }

    setStatus (refLoading, juce::String::fromUTF8 ("Lade ") + file.getFileName() + " ...");

    const double rate = hostRate.load();
    if (rate <= 0.0)
    {
        const juce::ScopedLock sl (pendingLock);
        pendingLoad = file;
        return;
    }
    loader->request (file, rate);
}

void AnalyzerProcessor::clearReference()
{
    if (auto* old = activeRef.exchange (nullptr))
    {
        const juce::ScopedLock sl (trashLock);
        trash.push_back ({ old, juce::Time::getMillisecondCounter() });
    }
    freePlaying.store (false);
    resetRefPos.store (true);
    setStatus (refEmpty, {});
}

void AnalyzerProcessor::seekReference (double seconds)
{
    const double rate = hostRate.load() > 0.0 ? hostRate.load() : 48000.0;
    seekRequest.store ((int64_t) juce::jmax (0.0, seconds * rate));
}

std::unique_ptr<RefData> AnalyzerProcessor::decodeReference (const juce::File& file, double targetRate,
                                                             juce::String& error, const std::function<bool()>& shouldStop)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr)
    {
        error = juce::String::fromUTF8 ("Dieses Format kann nicht gelesen werden: ") + file.getFileName();
        return nullptr;
    }

    const double fileRate = reader->sampleRate;
    const int64_t len = reader->lengthInSamples;
    if (fileRate <= 0.0 || len <= 0)
    {
        error = juce::String::fromUTF8 ("Die Datei ist leer oder beschädigt.");
        return nullptr;
    }
    if ((double) len / fileRate > 20.0 * 60.0)
    {
        error = juce::String::fromUTF8 ("Die Referenz ist zu lang (höchstens 20 Minuten).");
        return nullptr;
    }

    const int pad = 512;
    juce::AudioBuffer<float> raw (2, (int) len + pad);
    raw.clear();
    reader->read (&raw, 0, (int) len, 0, true, true);
    if (reader->numChannels == 1)
        raw.copyFrom (1, 0, raw, 0, 0, (int) len);

    if (shouldStop()) return nullptr;

    auto data = std::make_unique<RefData>();
    data->file = file;
    data->name = file.getFileName();
    data->fileSampleRate = fileRate;
    data->sampleRate = targetRate;

    if (std::abs (fileRate - targetRate) > 0.5)
    {
        const double ratio = fileRate / targetRate;
        const int outLen = (int) std::floor ((double) len / ratio);
        data->audio.setSize (2, outLen);
        for (int ch = 0; ch < 2; ++ch)
        {
            juce::WindowedSincInterpolator interp;
            interp.process (ratio, raw.getReadPointer (ch), data->audio.getWritePointer (ch), outLen);
            if (shouldStop()) return nullptr;
        }
    }
    else
    {
        data->audio.setSize (2, (int) len);
        for (int ch = 0; ch < 2; ++ch)
            data->audio.copyFrom (ch, 0, raw, ch, 0, (int) len);
    }
    raw.setSize (0, 0);

    const int total = data->audio.getNumSamples();
    data->seconds = (double) total / targetRate;

    // Lautheit der ganzen Datei
    LoudnessMeter lm;
    lm.prepare (targetRate);
    const float* L = data->audio.getReadPointer (0);
    const float* R = data->audio.getReadPointer (1);
    for (int pos = 0; pos < total; pos += 8192)
    {
        lm.process (L + pos, R + pos, juce::jmin (8192, total - pos));
        if ((pos & 0xFFFFF) == 0 && shouldStop()) return nullptr;
    }
    data->integrated = lm.integrated();
    data->truePeak = lm.truePeakDb();
    data->lra = lm.lra();
    data->shortMax = lm.shortTermMax();

    // Wellenform-Übersicht
    const int cols = 1600;
    data->wave.assign (cols, 0.0f);
    for (int c = 0; c < cols; ++c)
    {
        const int a = (int) ((int64_t) total * c / cols);
        const int b = juce::jmax (a + 1, (int) ((int64_t) total * (c + 1) / cols));
        float m = 0.0f;
        for (int i = a; i < b && i < total; ++i)
            m = juce::jmax (m, std::abs (L[i]), std::abs (R[i]));
        data->wave[(size_t) c] = m;
    }

    return data;
}

void AnalyzerProcessor::publishReference (std::unique_ptr<RefData> data)
{
    RefData* fresh = data.release();
    RefData* old = activeRef.exchange (fresh, std::memory_order_acq_rel);

    const bool sameFile = old != nullptr && old->file == fresh->file;
    const bool keep = keepOffset.exchange (false);
    if (! sameFile && ! keep)
        resetRefPos.store (true);

    if (old != nullptr)
    {
        const juce::ScopedLock sl (trashLock);
        trash.push_back ({ old, juce::Time::getMillisecondCounter() });
    }
    setStatus (refReady, {});
}

void AnalyzerProcessor::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounter();
    const juce::ScopedLock sl (trashLock);
    for (auto it = trash.begin(); it != trash.end();)
    {
        if (now - it->second > 2000)
        {
            delete it->first;
            it = trash.erase (it);
        }
        else
            ++it;
    }
}

//==============================================================================
void AnalyzerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto* r = getRef())
        state.setProperty ("refPath", r->file.getFullPathName(), nullptr);
    else
    {
        const juce::ScopedLock sl (pendingLock);
        state.setProperty ("refPath", pendingLoad.getFullPathName(), nullptr);
    }
    state.setProperty ("refOffset", juce::String (refOffset.load()), nullptr);
    state.setProperty ("uiScale", uiScale.load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AnalyzerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    apvts.replaceState (state);

    uiScale.store ((float) (double) state.getProperty ("uiScale", 0.9));
    const auto path = state.getProperty ("refPath").toString();
    const auto offset = state.getProperty ("refOffset").toString().getLargeIntValue();

    if (path.isNotEmpty())
    {
        const juce::File f (path);
        auto* cur = getRef();
        if (cur == nullptr || cur->file != f)
        {
            keepOffset.store (true);
            loadReference (f);
        }
    }
    refOffset.store (offset);
}

juce::AudioProcessorEditor* AnalyzerProcessor::createEditor()
{
    return new AnalyzerEditor (*this);
}

} // namespace k54a

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new k54a::AnalyzerProcessor();
}

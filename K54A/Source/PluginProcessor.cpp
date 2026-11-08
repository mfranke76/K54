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

    void request (const juce::File& f, double rate, int slot)
    {
        {
            const juce::ScopedLock sl (lock);
            // gleicher Slot: ältere Aufträge verwerfen
            for (auto it = jobs.begin(); it != jobs.end();)
                it = it->slot == slot ? jobs.erase (it) : it + 1;
            jobs.push_back ({ f, rate, slot });
        }
        notify();
    }

    bool hasNewerJob (int slot)
    {
        const juce::ScopedLock sl (lock);
        for (auto& j : jobs) if (j.slot == slot) return true;
        return false;
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            Job job;
            bool have = false;
            {
                const juce::ScopedLock sl (lock);
                if (! jobs.empty()) { job = jobs.front(); jobs.pop_front(); have = true; }
            }

            if (! have)
            {
                wait (-1);
                continue;
            }

            owner.loadingSlot.store (job.slot);
            owner.setStatus (refLoading, juce::String::fromUTF8 ("Lade ") + job.file.getFileName() + " ...");

            juce::String error;
            auto stop = [this, &job] { return threadShouldExit() || hasNewerJob (job.slot); };
            auto data = owner.decodeReference (job.file, job.rate, error, stop);

            if (data != nullptr)
                owner.publishReference (std::move (data), job.slot);
            else if (! stop())
                owner.setStatus (refError, error);

            owner.loadingSlot.store (-1);
            ++owner.refVersion;
        }
    }

private:
    struct Job { juce::File file; double rate = 48000.0; int slot = 0; };
    AnalyzerProcessor& owner;
    juce::CriticalSection lock;
    std::deque<Job> jobs;
};

//==============================================================================
static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

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
                                                   StringArray { "Langsam", "Mittel", "Schnell", "Langzeit" }, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "smooth", 1 }, "Glaettung",
                                                   StringArray { "Aus", "1/12 Okt.", "1/6 Okt.", "1/3 Okt." }, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "target", 1 }, "Ziel-Lautheit",
                                                   StringArray { "Kein Ziel", "-14 LUFS  Spotify / YouTube",
                                                                 "-16 LUFS  Apple Music", "-23 LUFS  EBU R128 (TV)",
                                                                 "-9 LUFS  Club / laut", "-24 LUFS  ATSC A/85 (US-TV)" }, 1));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "autoReset", 1 }, "Auto-Reset bei Play", true));

    // ab 1.1
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "showDiff", 2 }, "Differenzkurve", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "channel", 2 }, "Kanal Spektrum",
                                                   StringArray { "L+R (Mid)", "Links", "Rechts", "Side" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "fftRes", 2 }, "Aufloesung",
                                                   StringArray { "Normal", "Hoch", "Sehr hoch" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "monitor", 2 }, "Abhoeren",
                                                   StringArray { "Stereo", "Mono", "Side" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "band", 2 }, "Band-Solo",
                                                   StringArray { "Voller Bereich", "Bass < 150 Hz",
                                                                 u8 ("Mitten 150 Hz – 5 kHz"), u8 ("Höhen > 5 kHz") }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "refSlot", 2 }, "Referenz-Slot",
                                                   StringArray { "1", "2", "3", "4" }, 0));
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
    pSlot      = apvts.getRawParameterValue ("refSlot");
    pMonitor   = apvts.getRawParameterValue ("monitor");
    pBand      = apvts.getRawParameterValue ("band");

    for (int s = 0; s < numSlots; ++s)
    {
        refs[(size_t) s].store (nullptr);
        refOffset[(size_t) s].store (0);
        keepOffset[(size_t) s].store (false);
        resetSlotPos[(size_t) s].store (false);
    }

    juce::PropertiesFile::Options o;
    o.applicationName = "K-54 Analyzer";
    o.filenameSuffix = ".settings";
    o.folderName = "Keller54";
    o.osxLibrarySubFolder = "Application Support";
    o.storageFormat = juce::PropertiesFile::storeAsXML;
    settings = std::make_unique<juce::PropertiesFile> (o);

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

    for (auto& r : refs)
        delete r.exchange (nullptr);
    const juce::ScopedLock sl (trashLock);
    for (auto& t : trash) delete t.first;
    trash.clear();
}

bool AnalyzerProcessor::getProMode() const { return settings->getBoolValue ("pro", false); }

void AnalyzerProcessor::setProMode (bool pro)
{
    settings->setValue ("pro", pro);
    settings->saveIfNeeded();
}

int AnalyzerProcessor::currentSlot() const noexcept
{
    return juce::jlimit (0, numSlots - 1, (int) std::lround (pSlot->load()));
}

void AnalyzerProcessor::setSlot (int slot)
{
    if (auto* prm = apvts.getParameter ("refSlot"))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) juce::jlimit (0, numSlots - 1, slot)));
        prm->endChangeGesture();
    }
    ++refVersion;
}

float AnalyzerProcessor::getRefGainDb() const noexcept
{
    return pRefGain->load() + (pMatch->load() > 0.5f ? matchGainDb.load() : 0.0f);
}

bool AnalyzerProcessor::isMonitoringActive() const noexcept
{
    return pMonitor->load() > 0.5f || pBand->load() > 0.5f;
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
    audioSlot = currentSlot();
    slotFade = 1.0f;
    matchInit = false;

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) scratchSize, 2 };
    using LR = juce::dsp::LinkwitzRileyFilterType;
    lpBass.setType (LR::lowpass);   lpBass.setCutoffFrequency (150.0f);
    hpMid.setType (LR::highpass);   hpMid.setCutoffFrequency (150.0f);
    lpMid.setType (LR::lowpass);    lpMid.setCutoffFrequency (5000.0f);
    hpHigh.setType (LR::highpass);  hpHigh.setCutoffFrequency (5000.0f);
    for (auto* f : { &lpBass, &hpMid, &lpMid, &hpHigh }) f->prepare (spec);

    for (int s = 0; s < numSlots; ++s)
    {
        juce::File pending;
        {
            const juce::ScopedLock sl (pendingLock);
            pending = pendingLoad[(size_t) s];
            pendingLoad[(size_t) s] = juce::File();
        }

        if (pending.existsAsFile())
            loadReference (pending, s);
        else if (auto* r = getRef (s))
            if (std::abs (r->sampleRate - sr) > 0.5 && r->file.existsAsFile())
            {
                keepOffset[(size_t) s].store (true);
                loader->request (r->file, sr, s);
            }
    }
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
        ++resetCount;
    }

    for (int s = 0; s < numSlots; ++s)
        if (resetSlotPos[(size_t) s].exchange (false))
        {
            refOffset[(size_t) s].store (0);
            freePos[(size_t) s] = 0;
            if (s == audioSlot) matchInit = false;
        }

    const int want = currentSlot();
    const int64_t seek = seekRequest.exchange (-1);
    if (seek >= 0)
    {
        if (pSync->load() > 0.5f) refOffset[(size_t) want].store (seek - lastDawPos);
        else                      freePos[(size_t) want] = seek;
    }

    const bool stereo = numIn > 1;
    int done = 0;
    while (done < n)
    {
        const int m = juce::jmin (n - done, scratchSize);
        float* L = buffer.getWritePointer (0, done);
        float* R = stereo ? buffer.getWritePointer (1, done) : L;
        processChunk (L, R, stereo, m, playing, dawPos >= 0 ? dawPos + done : -1);
        applyMonitoring (L, R, stereo, m);
        done += m;
    }

    mixLoud.store (mixMeter);
    refLoud.store (refMeter);

    // Anzeige-Position der Referenz im gewählten Slot (auch bei gestoppter DAW)
    if (auto* ref = getRef (want))
    {
        const int64_t len = ref->length();
        int64_t p = pSync->load() > 0.5f ? lastDawPos + refOffset[(size_t) want].load() : freePos[(size_t) want];
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

    // ---- Slot-Wechsel: alte Referenz kurz ausblenden, dann umschalten -----
    const int want = currentSlot();
    if (want != audioSlot && getRef (audioSlot) == nullptr)
    {
        audioSlot = want;
        slotFade = 0.0f;
        refMeter.resetAll();
        matchInit = false;
    }
    const bool fadingOut = want != audioSlot;
    const float fadeStep = (float) (1.0 / (0.01 * sr));

    // ---- Referenz ---------------------------------------------------------
    RefData* ref = getRef (audioSlot);
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
                const int64_t start = dawPos + refOffset[(size_t) audioSlot].load();
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
            int64_t& fp = freePos[(size_t) audioSlot];
            if (fp < 0 || fp >= len) fp = 0;
            for (int i = 0; i < n; ++i)
            {
                rl[i] = srcL[fp];
                rr[i] = srcR[fp];
                if (++fp >= len) fp = 0;
            }
        }
    }

    refRunning.store (active && ! fadingOut);

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
        if (! fadingOut)
        {
            refMeter.process (rl, rr, n);      // Zahlen = Original-Datei (ohne Gain)
            refLevels.process (rl, rr, n);
        }
        for (int i = 0; i < n; ++i)
        {
            slotFade = fadingOut ? juce::jmax (0.0f, slotFade - fadeStep) : juce::jmin (1.0f, slotFade + fadeStep);
            const float g = refGain.getNextValue() * slotFade;
            rl[i] *= g;
            rr[i] *= g;
        }
        if (! fadingOut)
            refFifo.push (rl, rr, n);          // Kurve = so wie du sie hörst
    }
    else
    {
        refGain.skip (n);
        slotFade = fadingOut ? 0.0f : juce::jmin (1.0f, slotFade + fadeStep * (float) n);
        std::fill (rl, rl + n, 0.0f);
        std::fill (rr, rr + n, 0.0f);
    }

    if (fadingOut && slotFade <= 0.0f)
    {
        audioSlot = want;
        refMeter.resetAll();
        matchInit = false;
    }

    // ---- A/B mit kurzer Überblendung --------------------------------------
    const float targetAB = (pAB->load() > 0.5f && (ref != nullptr || getRef (want) != nullptr)) ? 1.0f : 0.0f;
    const float step = (float) (1.0 / (0.015 * sr));

    if (targetAB <= 0.0f && abPos <= 0.0f)
        return; // reiner Mix

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

void AnalyzerProcessor::applyMonitoring (float* L, float* R, bool stereo, int n)
{
    const int mon = (int) std::lround (pMonitor->load());
    const int band = (int) std::lround (pBand->load());

    if (band != lastBand)
    {
        for (auto* f : { &lpBass, &hpMid, &lpMid, &hpHigh }) f->reset();
        lastBand = band;
    }

    if (stereo && mon > 0)
    {
        for (int i = 0; i < n; ++i)
        {
            const float v = mon == 1 ? 0.5f * (L[i] + R[i]) : 0.5f * (L[i] - R[i]);
            L[i] = R[i] = v;
        }
    }

    if (band <= 0)
        return;

    const int chans = stereo ? 2 : 1;
    for (int ch = 0; ch < chans; ++ch)
    {
        float* d = ch == 0 ? L : R;
        for (int i = 0; i < n; ++i)
        {
            float x = d[i];
            if (band == 1)      x = lpBass.processSample (ch, x);
            else if (band == 2) x = lpMid.processSample (ch, hpMid.processSample (ch, x));
            else                x = hpHigh.processSample (ch, x);
            d[i] = x;
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

void AnalyzerProcessor::loadReference (const juce::File& file, int slot)
{
    slot = juce::jlimit (0, numSlots - 1, slot);
    if (! file.existsAsFile())
    {
        setStatus (refError, u8 ("Datei nicht gefunden: ") + file.getFileName());
        return;
    }

    const double rate = hostRate.load();
    if (rate <= 0.0)
    {
        const juce::ScopedLock sl (pendingLock);
        pendingLoad[(size_t) slot] = file;
        loadingSlot.store (slot);
        setStatus (refLoading, u8 ("Lade ") + file.getFileName() + " ...");
        return;
    }
    loadingSlot.store (slot);
    setStatus (refLoading, u8 ("Lade ") + file.getFileName() + " ...");
    loader->request (file, rate, slot);
}

void AnalyzerProcessor::trashRef (RefData* r)
{
    if (r == nullptr) return;
    const juce::ScopedLock sl (trashLock);
    trash.push_back ({ r, juce::Time::getMillisecondCounter() });
}

void AnalyzerProcessor::clearReference (int slot)
{
    slot = juce::jlimit (0, numSlots - 1, slot);
    trashRef (refs[(size_t) slot].exchange (nullptr));
    {
        const juce::ScopedLock sl (pendingLock);
        pendingLoad[(size_t) slot] = juce::File();
    }
    resetSlotPos[(size_t) slot].store (true);
    bool any = false;
    for (int s = 0; s < numSlots; ++s) any = any || getRef (s) != nullptr;
    if (! any) freePlaying.store (false);
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
        error = u8 ("Dieses Format kann nicht gelesen werden: ") + file.getFileName();
        return nullptr;
    }

    const double fileRate = reader->sampleRate;
    const int64_t len = reader->lengthInSamples;
    if (fileRate <= 0.0 || len <= 0)
    {
        error = u8 ("Die Datei ist leer oder beschädigt.");
        return nullptr;
    }
    if ((double) len / fileRate > 20.0 * 60.0)
    {
        error = u8 ("Die Referenz ist zu lang (höchstens 20 Minuten).");
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
    const float* L = data->audio.getReadPointer (0);
    const float* R = data->audio.getReadPointer (1);

    // Lautheit der ganzen Datei
    LoudnessMeter lm;
    lm.prepare (targetRate);
    for (int pos = 0; pos < total; pos += 8192)
    {
        lm.process (L + pos, R + pos, juce::jmin (8192, total - pos));
        if ((pos & 0xFFFFF) == 0 && shouldStop()) return nullptr;
    }
    data->integrated = lm.integrated();
    data->truePeak = lm.truePeakDb();
    data->lra = lm.lra();
    data->shortMax = lm.shortTermMax();
    data->momentaryMax = lm.momentaryMaxDb();

    // Langzeit-Spektrum der ganzen Datei (für Tonale Balance und Langzeit-Kurve)
    {
        const int order = SpecMath::optimalOrder (targetRate, 0);
        const int size = 1 << order;
        juce::dsp::FFT fft (order);
        float wsum = 1.0f;
        const auto win = SpecMath::hann (size, wsum);
        std::vector<float> buf ((size_t) size * 2);
        std::vector<double> avg ((size_t) size / 2 + 1, 0.0);
        const float scale = 2.0f / wsum;
        int frames = 0, count = 0;

        for (int start = 0; start < total; start += size / 2)
        {
            for (int i = 0; i < size; ++i)
            {
                const int idx = start + i;
                buf[(size_t) i] = idx < total ? 0.5f * (L[idx] + R[idx]) * win[(size_t) i] : 0.0f;
            }
            std::fill (buf.begin() + size, buf.end(), 0.0f);
            fft.performFrequencyOnlyForwardTransform (buf.data(), true);

            double sum = 0.0;
            for (int k = 0; k <= size / 2; ++k)
            {
                const float a = buf[(size_t) k] * scale;
                buf[(size_t) k] = a * a;
                sum += buf[(size_t) k];
            }
            if (sum > 1.0e-7)
            {
                for (int k = 0; k <= size / 2; ++k) avg[(size_t) k] += buf[(size_t) k];
                ++frames;
            }
            if ((++count & 63) == 0 && shouldStop()) return nullptr;
        }

        if (frames > 0)
        {
            std::vector<float> avgF (avg.size());
            for (size_t k = 0; k < avg.size(); ++k) avgF[k] = (float) (avg[k] / frames);
            SpecMath::pointsFromPower (avgF, size, targetRate, 1.0f / 12.0f, false, data->longTermDb);
            data->hasBands = SpecMath::bandsFromPower (avgF, size, targetRate, data->bands);
        }
    }

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

void AnalyzerProcessor::publishReference (std::unique_ptr<RefData> data, int slot)
{
    slot = juce::jlimit (0, numSlots - 1, slot);
    RefData* fresh = data.release();
    RefData* old = refs[(size_t) slot].exchange (fresh, std::memory_order_acq_rel);

    const bool sameFile = old != nullptr && old->file == fresh->file;
    const bool keep = keepOffset[(size_t) slot].exchange (false);
    if (! sameFile && ! keep)
        resetSlotPos[(size_t) slot].store (true);

    trashRef (old);
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
    for (int s = 0; s < numSlots; ++s)
    {
        juce::String path;
        if (auto* r = getRef (s)) path = r->file.getFullPathName();
        else
        {
            const juce::ScopedLock sl (pendingLock);
            path = pendingLoad[(size_t) s].getFullPathName();
        }
        state.setProperty ("refPath" + juce::String (s), path, nullptr);
        state.setProperty ("refOffset" + juce::String (s), juce::String (refOffset[(size_t) s].load()), nullptr);
    }
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
    uiScale.store ((float) (double) state.getProperty ("uiScale", 0.85));

    for (int s = 0; s < numSlots; ++s)
    {
        auto path = state.getProperty ("refPath" + juce::String (s)).toString();
        auto offset = state.getProperty ("refOffset" + juce::String (s)).toString().getLargeIntValue();
        if (s == 0 && path.isEmpty())   // Projekte aus Version 1.0
        {
            path = state.getProperty ("refPath").toString();
            offset = state.getProperty ("refOffset").toString().getLargeIntValue();
        }

        refOffset[(size_t) s].store (offset);
        if (path.isNotEmpty())
        {
            const juce::File f (path);
            auto* cur = getRef (s);
            if (cur == nullptr || cur->file != f)
            {
                keepOffset[(size_t) s].store (true);
                loadReference (f, s);
            }
        }
        else if (getRef (s) != nullptr)
            clearReference (s);
    }
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

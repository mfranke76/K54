// K-54 Analyzer · Keller54 · by Frank E.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include <deque>
#include <vector>
#include "Loudness.h"
#include "Spectrum.h"

namespace k54a
{
//==============================================================================
struct RefData
{
    juce::AudioBuffer<float> audio;   // immer 2 Kanäle, auf Projekt-Samplerate umgerechnet
    double sampleRate = 0.0;
    double fileSampleRate = 0.0;
    juce::File file;
    juce::String name;
    std::vector<float> wave;          // Spitzenwert je Spalte (0..1)
    std::vector<float> longTermDb;    // Langzeit-Spektrum der ganzen Datei (L+R, 640 Punkte, ohne Neigung)
    std::array<float, SpecMath::numBands> bands {};
    bool hasBands = false;
    float integrated = LoudnessMeter::silent;
    float truePeak = LoudnessMeter::silent;
    float lra = 0.0f;
    float shortMax = LoudnessMeter::silent;
    float momentaryMax = LoudnessMeter::silent;
    double seconds = 0.0;

    int64_t length() const noexcept { return audio.getNumSamples(); }
};

//==============================================================================
// Stereo-Paare für Spektrum und Vektorskop (Audio-Thread schreibt, GUI liest)
class StereoFifo
{
public:
    StereoFifo() : bufL ((size_t) capacity, 0.0f), bufR ((size_t) capacity, 0.0f) {}

    void push (const float* L, const float* R, int n) noexcept
    {
        int s1, z1, s2, z2;
        fifo.prepareToWrite (n, s1, z1, s2, z2);
        for (int i = 0; i < z1; ++i) { bufL[(size_t) (s1 + i)] = L[i]; bufR[(size_t) (s1 + i)] = R[i]; }
        for (int i = 0; i < z2; ++i) { bufL[(size_t) (s2 + i)] = L[z1 + i]; bufR[(size_t) (s2 + i)] = R[z1 + i]; }
        fifo.finishedWrite (z1 + z2);
    }

    int pull (float* L, float* R, int maxN) noexcept
    {
        int s1, z1, s2, z2;
        fifo.prepareToRead (maxN, s1, z1, s2, z2);
        for (int i = 0; i < z1; ++i) { L[i] = bufL[(size_t) (s1 + i)]; R[i] = bufR[(size_t) (s1 + i)]; }
        for (int i = 0; i < z2; ++i) { L[z1 + i] = bufL[(size_t) (s2 + i)]; R[z1 + i] = bufR[(size_t) (s2 + i)]; }
        fifo.finishedRead (z1 + z2);
        return z1 + z2;
    }

private:
    static constexpr int capacity = 1 << 17;
    juce::AbstractFifo fifo { capacity };
    std::vector<float> bufL, bufR;
};

//==============================================================================
struct LevelMeter
{
    void prepare (double sr)
    {
        coef = (float) std::exp (-1.0 / (0.3 * (sr > 0 ? sr : 48000.0)));
        msL = msR = xy = 0.0f;
    }

    void process (const float* L, const float* R, int n) noexcept
    {
        float pL = 0.0f, pR = 0.0f;
        const float c1 = 1.0f - coef;
        for (int i = 0; i < n; ++i)
        {
            const float l = L[i], r = R[i];
            pL = std::max (pL, std::abs (l));
            pR = std::max (pR, std::abs (r));
            msL = coef * msL + c1 * l * l;
            msR = coef * msR + c1 * r * r;
            xy  = coef * xy  + c1 * l * r;
        }
        updateMax (peakL, pL);
        updateMax (peakR, pR);
        updateMax (maxPeak, std::max (pL, pR));
        rmsL.store (std::sqrt (msL));
        rmsR.store (std::sqrt (msR));
        const float den = std::sqrt (msL * msR);
        corr.store (den > 1.0e-9f ? juce::jlimit (-1.0f, 1.0f, xy / den) : 0.0f);
    }

    static void updateMax (std::atomic<float>& a, float v) noexcept
    {
        float c = a.load();
        while (v > c && ! a.compare_exchange_weak (c, v)) {}
    }

    std::atomic<float> peakL { 0.0f }, peakR { 0.0f }, rmsL { 0.0f }, rmsR { 0.0f }, corr { 0.0f }, maxPeak { 0.0f };
    float coef = 0.9999f, msL = 0.0f, msR = 0.0f, xy = 0.0f;
};

struct LoudReadout
{
    std::atomic<float> m { LoudnessMeter::silent }, s { LoudnessMeter::silent }, i { LoudnessMeter::silent },
                       lra { 0.0f }, tp { LoudnessMeter::silent }, sMax { LoudnessMeter::silent },
                       mMax { LoudnessMeter::silent };

    void store (const LoudnessMeter& lm) noexcept
    {
        m.store (lm.momentary());
        s.store (lm.shortTerm());
        i.store (lm.integrated());
        lra.store (lm.lra());
        tp.store (lm.truePeakDb());
        sMax.store (lm.shortTermMax());
        mMax.store (lm.momentaryMaxDb());
    }
};

//==============================================================================
class AnalyzerProcessor : public juce::AudioProcessor,
                          private juce::Timer
{
public:
    static constexpr int numSlots = 4;       // Datei-Slots 1–4
    static constexpr int liveSlot = 4;       // Slot 5 = Live-Referenz über Sidechain
    enum RefStatus { refEmpty = 0, refLoading, refReady, refError };

    AnalyzerProcessor();
    ~AnalyzerProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "K-54 Analyzer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    //==== Referenz-Slots =======================================================
    int currentSlot() const noexcept;
    void setSlot (int slot);
    void loadReference (const juce::File& file) { loadReference (file, currentSlot()); }
    void loadReference (const juce::File& file, int slot);
    void clearReference() { clearReference (currentSlot()); }
    void clearReference (int slot);
    void seekReference (double seconds);
    void setFreePlaying (bool shouldPlay) { freePlaying.store (shouldPlay); }
    bool isFreePlaying() const noexcept { return freePlaying.load(); }

    RefData* getRef() const noexcept { return getRef (currentSlot()); }
    RefData* getRef (int slot) const noexcept
    {
        return slot >= 0 && slot < numSlots ? refs[(size_t) slot].load (std::memory_order_acquire) : nullptr;
    }
    bool isLiveSlot() const noexcept { return currentSlot() == liveSlot; }
    bool isLiveConnected() const noexcept { return liveConnected.load(); }
    bool hasLiveSignal() const noexcept { return liveSignal.load(); }
    bool hasReference() const noexcept { return isLiveSlot() ? isLiveConnected() : getRef() != nullptr; }
    int getRefVersion() const noexcept { return refVersion.load(); }
    int getRefStatus() const noexcept { return refStatus.load(); }
    int getLoadingSlot() const noexcept { return loadingSlot.load(); }
    juce::String getRefMessage() const { const juce::ScopedLock sl (msgLock); return refMessage; }
    double getRefPositionSeconds() const noexcept { return refPosSeconds.load(); }
    bool isRefRunning() const noexcept { return refRunning.load(); }
    float getMatchGainDb() const noexcept { return matchGainDb.load(); }
    float getRefGainDb() const noexcept;   // Gain + Angleich, so wie die Referenz klingt
    bool isHostPlaying() const noexcept { return hostPlaying.load(); }
    bool isMonitoringActive() const noexcept;

    void resetMeasurements() { resetRequested.store (true); }
    int getResetCount() const noexcept { return resetCount.load(); }

    float getUiScale() const noexcept { return uiScale.load(); }
    void setUiScale (float s) { uiScale.store (s); }
    bool getProMode() const;
    juce::PropertiesFile& getSettings() { return *settings; }
    void setProMode (bool pro);

    // Lade-Arbeit (läuft im Lade-Thread, auch für Tests direkt aufrufbar)
    std::unique_ptr<RefData> decodeReference (const juce::File& file, double targetRate,
                                              juce::String& error, const std::function<bool()>& shouldStop);
    void publishReference (std::unique_ptr<RefData> data, int slot);

    juce::AudioProcessorValueTreeState apvts;

    StereoFifo mixFifo, refFifo;
    LevelMeter mixLevels, refLevels;
    LoudReadout mixLoud, refLoud;

private:
    class Loader;
    void timerCallback() override;
    void processChunk (float* L, float* R, bool stereo, int n, bool playing, int64_t dawPos,
                       const float* scL, const float* scR);
    void applyMonitoring (float* L, float* R, bool stereo, int n);
    void setStatus (int s, const juce::String& msg);
    void trashRef (RefData* r);

    juce::AudioFormatManager formatManager;
    std::unique_ptr<Loader> loader;
    std::unique_ptr<juce::PropertiesFile> settings;

    std::array<std::atomic<RefData*>, numSlots> refs {};
    std::array<std::atomic<int64_t>, numSlots> refOffset {};
    std::array<std::atomic<bool>, numSlots> keepOffset {};
    std::array<std::atomic<bool>, numSlots> resetSlotPos {};
    std::atomic<int> refVersion { 0 }, refStatus { refEmpty }, loadingSlot { -1 }, resetCount { 0 };
    juce::CriticalSection msgLock, trashLock, pendingLock;
    juce::String refMessage;
    std::vector<std::pair<RefData*, juce::uint32>> trash;
    std::array<juce::File, numSlots> pendingLoad;

    std::atomic<double> hostRate { 0.0 };
    std::atomic<int64_t> seekRequest { -1 };
    std::atomic<bool> freePlaying { false }, refRunning { false }, hostPlaying { false }, resetRequested { false },
                      liveConnected { false }, liveSignal { false };
    std::atomic<double> refPosSeconds { 0.0 };
    std::atomic<float> matchGainDb { 0.0f }, uiScale { 0.85f };

    std::atomic<float>* pAB = nullptr;
    std::atomic<float>* pMatch = nullptr;
    std::atomic<float>* pRefGain = nullptr;
    std::atomic<float>* pSync = nullptr;
    std::atomic<float>* pAutoReset = nullptr;
    std::atomic<float>* pSlot = nullptr;
    std::atomic<float>* pMonitor = nullptr;
    std::atomic<float>* pBand = nullptr;

    LoudnessMeter mixMeter, refMeter;
    juce::LinearSmoothedValue<float> refGain;
    std::vector<float> refL, refR;
    float abPos = 0.0f, slotFade = 1.0f;
    int audioSlot = 0;
    std::array<int64_t, numSlots> freePos {};
    int64_t lastDawPos = 0;
    bool wasPlaying = false, matchInit = false;
    int matchCounter = 0, lastBand = 0;
    double sr = 48000.0;

    juce::dsp::LinkwitzRileyFilter<float> lpBass, hpMid, lpMid, hpHigh;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalyzerProcessor)
};

} // namespace k54a

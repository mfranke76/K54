// K-54 Analyzer · Keller54 · by Frank E.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <vector>
#include "Loudness.h"

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
    float integrated = LoudnessMeter::silent;
    float truePeak = LoudnessMeter::silent;
    float lra = 0.0f;
    float shortMax = LoudnessMeter::silent;
    double seconds = 0.0;

    int64_t length() const noexcept { return audio.getNumSamples(); }
};

//==============================================================================
// Mono-Summe für das Spektrum (Audio-Thread schreibt, GUI liest)
class MonoFifo
{
public:
    MonoFifo() : buf ((size_t) capacity, 0.0f) {}

    void push (const float* L, const float* R, int n) noexcept
    {
        int s1, z1, s2, z2;
        fifo.prepareToWrite (n, s1, z1, s2, z2);
        for (int i = 0; i < z1; ++i) buf[(size_t) (s1 + i)] = 0.5f * (L[i] + R[i]);
        for (int i = 0; i < z2; ++i) buf[(size_t) (s2 + i)] = 0.5f * (L[z1 + i] + R[z1 + i]);
        fifo.finishedWrite (z1 + z2);
    }

    int pull (float* dest, int maxN) noexcept
    {
        int s1, z1, s2, z2;
        fifo.prepareToRead (maxN, s1, z1, s2, z2);
        for (int i = 0; i < z1; ++i) dest[i] = buf[(size_t) (s1 + i)];
        for (int i = 0; i < z2; ++i) dest[z1 + i] = buf[(size_t) (s2 + i)];
        fifo.finishedRead (z1 + z2);
        return z1 + z2;
    }

private:
    static constexpr int capacity = 1 << 17;
    juce::AbstractFifo fifo { capacity };
    std::vector<float> buf;
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
                       lra { 0.0f }, tp { LoudnessMeter::silent };

    void store (const LoudnessMeter& lm) noexcept
    {
        m.store (lm.momentary());
        s.store (lm.shortTerm());
        i.store (lm.integrated());
        lra.store (lm.lra());
        tp.store (lm.truePeakDb());
    }
};

//==============================================================================
class AnalyzerProcessor : public juce::AudioProcessor,
                          private juce::Timer
{
public:
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

    //==== Referenz =============================================================
    void loadReference (const juce::File& file);
    void clearReference();
    void seekReference (double seconds);
    void setFreePlaying (bool shouldPlay) { freePlaying.store (shouldPlay); }
    bool isFreePlaying() const noexcept { return freePlaying.load(); }

    RefData* getRef() const noexcept { return activeRef.load (std::memory_order_acquire); }
    int getRefVersion() const noexcept { return refVersion.load(); }
    int getRefStatus() const noexcept { return refStatus.load(); }
    juce::String getRefMessage() const { const juce::ScopedLock sl (msgLock); return refMessage; }
    double getRefPositionSeconds() const noexcept { return refPosSeconds.load(); }
    bool isRefRunning() const noexcept { return refRunning.load(); }
    float getMatchGainDb() const noexcept { return matchGainDb.load(); }
    bool isHostPlaying() const noexcept { return hostPlaying.load(); }

    void resetMeasurements() { resetRequested.store (true); }

    float getUiScale() const noexcept { return uiScale.load(); }
    void setUiScale (float s) { uiScale.store (s); }

    // Lade-Arbeit (läuft im Lade-Thread, auch für Tests direkt aufrufbar)
    std::unique_ptr<RefData> decodeReference (const juce::File& file, double targetRate,
                                              juce::String& error, const std::function<bool()>& shouldStop);
    void publishReference (std::unique_ptr<RefData> data);

    juce::AudioProcessorValueTreeState apvts;

    MonoFifo mixFifo, refFifo;
    LevelMeter mixLevels, refLevels;
    LoudReadout mixLoud, refLoud;

private:
    class Loader;
    void timerCallback() override;
    void processChunk (float* L, float* R, bool stereo, int n, bool playing, int64_t dawPos);
    void setStatus (int s, const juce::String& msg);

    juce::AudioFormatManager formatManager;
    std::unique_ptr<Loader> loader;

    std::atomic<RefData*> activeRef { nullptr };
    std::atomic<int> refVersion { 0 }, refStatus { refEmpty };
    juce::CriticalSection msgLock, trashLock, pendingLock;
    juce::String refMessage;
    std::vector<std::pair<RefData*, juce::uint32>> trash;
    juce::File pendingLoad;

    std::atomic<double> hostRate { 0.0 };
    std::atomic<int64_t> seekRequest { -1 }, refOffset { 0 };
    std::atomic<bool> freePlaying { false }, refRunning { false }, hostPlaying { false },
                      resetRequested { false }, resetRefPos { false }, keepOffset { false };
    std::atomic<double> refPosSeconds { 0.0 };
    std::atomic<float> matchGainDb { 0.0f }, uiScale { 0.9f };

    std::atomic<float>* pAB = nullptr;
    std::atomic<float>* pMatch = nullptr;
    std::atomic<float>* pRefGain = nullptr;
    std::atomic<float>* pSync = nullptr;
    std::atomic<float>* pAutoReset = nullptr;

    LoudnessMeter mixMeter, refMeter;
    juce::LinearSmoothedValue<float> refGain;
    std::vector<float> refL, refR;
    float abPos = 0.0f;
    int64_t freePos = 0, lastDawPos = 0;
    bool wasPlaying = false, matchInit = false;
    int matchCounter = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalyzerProcessor)
};

} // namespace k54a

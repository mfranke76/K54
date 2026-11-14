#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "Pattern.h"
#include "Presets.h"

// ============================================================================
//  K-54 D · Drumcomputer von Keller54 · by Frank E.
// ============================================================================
class K54DProcessor : public juce::AudioProcessor
{
public:
    K54DProcessor();
    ~K54DProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "K-54 D"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return juce::jmax (0, currentFactory); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ------------------------------------------------------------- fuer die Oberflaeche
    juce::AudioProcessorValueTreeState apvts;
    PatternStore patterns;

    /** Pattern, das gerade spielt / als naechstes kommt (-1 = keins) */
    int  getPlayingPattern() const   { return currentPattern.load(); }
    int  getPendingPattern() const   { return pendingPattern.load(); }
    void selectPattern (int p);      // GUI / Programmwechsel: wechselt am Pattern-Ende (laeuft nichts: sofort)
    int  getPlayingStep() const      { return playingStep.load(); }
    bool isRunning() const           { return running.load(); }
    double getHostBpm() const        { return hostBpm.load(); }
    bool isHostPlaying() const       { return hostPlaying.load(); }

    std::atomic<bool> internalPlay { false };   // PLAY-Knopf (ohne DAW-Transport)
    int  getLaneHits (int lane) const { return laneHits[(size_t) lane].load(); }
    void triggerPad (int lane, float velocity);  // Klick auf den Spurnamen = vorhoeren
    float getMeter (int ch) const { return meter[(size_t) ch].load(); }
    float getGainReduction() const { return grDb.load(); }

    // Presets
    void loadFactory (int index);
    juce::String presetName { "Init" };
    int currentFactory = -1;
    juce::File userPresetFolder() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& f);
    juce::Array<juce::File> userPresets() const;

    // Samples
    bool loadSample (int lane, const juce::File& file);
    void clearSample (int lane);
    juce::String sampleName (int lane) const;
    std::shared_ptr<const DrumSample> getSample (int lane) const { return std::atomic_load (&samples[(size_t) lane]); }

    // MIDI-Export des aktuellen Patterns
    juce::File exportMidi (int pattern, const juce::File& target = {}) const;

    // Oberflaeche
    float uiScale = 1.0f;
    int   selectedLane = 0;
    int   stepsNow() const;
    std::atomic<int> stateVersion { 0 };     // zaehlt bei Preset-/Projekt-Laden hoch (Oberflaeche liest neu)

private:
    juce::ValueTree stateTree (bool withUi);
    void restoreTree (const juce::ValueTree& t, bool withUi);
    void readLaneParams();

    struct Ev { int offset; int lane; float vel; };
    void addEvent (int offset, int lane, float vel);

    DrumEngine engine;
    std::array<LaneParams, DrumInfo::numLanes> lp;
    std::array<std::shared_ptr<const DrumSample>, DrumInfo::numLanes> samples;

    struct LanePtrs { std::atomic<float>* p[11]; };
    std::array<LanePtrs, DrumInfo::numLanes> lanePtr;
    std::atomic<float> *pSeqOn, *pSteps, *pRes, *pSwing, *pHuman, *pPattern, *pFat, *pGlue, *pRoom, *pMaster;

    std::array<Ev, 1024> events;
    int numEvents = 0;

    double sr = 48000.0;
    double internalPpq = 0.0;
    float lastPatternParam = -1.0f;
    std::atomic<int> currentPattern { 0 }, pendingPattern { -1 }, playingStep { -1 };
    std::atomic<bool> running { false }, hostPlaying { false };
    std::atomic<double> hostBpm { 120.0 };
    std::array<std::atomic<int>, DrumInfo::numLanes> laneHits {};
    std::atomic<int> padLane { -1 };
    std::atomic<float> padVel { 0.0f };
    juce::Random rng;

    // Master
    juce::Reverb reverb;
    juce::AudioBuffer<float> roomBuf;
    struct Shelf { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] {}, z2[2] {}; } shelf;
    float lastFat = -1.0f;
    float compEnv = 0.0f, compGain = 1.0f;
    std::array<std::atomic<float>, 2> meter {};
    std::atomic<float> grDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K54DProcessor)
};

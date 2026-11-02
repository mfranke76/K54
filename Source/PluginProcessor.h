#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "SynthEngine.h"
#include "Looper.h"
#include "Drums.h"
#include "Sequencer.h"
#include "Presets.h"

class K54Processor : public juce::AudioProcessor
{
public:
    K54Processor();
    ~K54Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "K-54"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    // Programme = Werks-Presets (fuer Hosts mit Programm-Liste)
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Presets
    void loadFactoryPreset (int index);
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& file);
    static juce::File getUserPresetFolder();
    juce::Array<juce::File> getUserPresets() const;
    juce::String getPresetName() const { return presetName; }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> presetVersion { 0 };    // Oberflaeche fragt das per Timer ab
    juce::MidiKeyboardState keyboardState;   // Bildschirm-Tastatur
    Looper looper;
    StepSequencer sequencer;

    // Ausgaenge: 0 Main, 1 Synth, 2..9 Drums, 10..13 Loops (je Stereo)
    static constexpr int numOutBuses = 14;
    static constexpr int busSynth = 1, busDrum0 = 2, busLoop0 = 10;
    static juce::String outBusName (int bus);
    /** true, wenn der Host diesen Einzelausgang aktiviert hat */
    bool isOutBusActive (int bus) const;
    std::atomic<float> inPeak[2] { 0.0f, 0.0f };   // Eingangspegel fuer die Anzeige
    bool hasAudioInput() const { return getTotalNumInputChannels() > 0; }

private:
    static BusesProperties makeBuses();
    void readParams();
    void resetToDefaults();
    void setParam (const juce::String& id, float plainValue);

    SynthEngine engine;
    SynthParams sp;
    juce::AudioBuffer<float> inputBuf;   // aufbereiteter Audio-Eingang
    juce::AudioBuffer<float> synthBuf, clickBuf;
    std::array<juce::AudioBuffer<float>, DrumInfo::numLanes> drumBufs;
    std::array<juce::AudioBuffer<float>, Looper::numTracks> loopBufs;
    DrumMachine drums;
    std::vector<DrumMachine::Hit> hits;
    juce::MidiBuffer synthMidi;
    void ensureBuffers (int n);
    // Sichten auf die Puffer (ohne Kopie) und zwischengespeicherte Parameter-Zeiger
    juce::AudioBuffer<float> synthView, clickView;
    std::array<juce::AudioBuffer<float>, DrumInfo::numLanes> drumViews;
    std::array<juce::AudioBuffer<float>, Looper::numTracks> loopViews;
    enum { dLevel, dTune, dDecay, dTone, dPan, dMute, dOut, dNum };
    std::array<std::array<std::atomic<float>*, dNum>, DrumInfo::numLanes> drumP {};
    std::array<std::atomic<float>*, Looper::numTracks> outLoopP {};
    int currentProgram = 0;
    juce::String presetName;

    std::atomic<float>* raw (const char* id) { return apvts.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K54Processor)
};

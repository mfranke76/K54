#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "SynthEngine.h"
#include "Presets.h"
#include "Harmonizer.h"

class K55Processor : public juce::AudioProcessor, private juce::Timer
{
public:
    K55Processor();
    ~K55Processor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "K-54 S"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- Presets
    void loadFactoryPreset (int index);
    bool saveUserPreset (const juce::String& name, const juce::String& category);
    bool loadUserPreset (const juce::File& file);
    static juce::File getUserPresetFolder();
    static juce::File getSampleFolder();
    juce::Array<juce::File> getUserPresets() const;
    juce::String getPresetName() const { return presetName; }

    // ---- Samples & Wavetables (nur Message-Thread)
    bool loadSampleFile (const juce::File& f, juce::String& error);
    bool loadWavetableFile (const juce::File& f, juce::String& error);
    void sampleToWavetable();
    void clearSample();
    std::shared_ptr<SampleData> getSample() const { return sample; }

    // ---- Sample-Editor (wie in einer DAW, nur Message-Thread)
    //      a, b = Auswahl als Anteil der Sample-Laenge (0..1); ohne Auswahl a == b
    enum class SampleEdit { trim, cut, silence, fadeIn, fadeOut, normalize, reverse };
    bool editSample (SampleEdit op, double a, double b);
    static bool editNeedsSelection (SampleEdit op) { return op == SampleEdit::trim || op == SampleEdit::cut || op == SampleEdit::silence || op == SampleEdit::fadeIn || op == SampleEdit::fadeOut; }
    bool undoSampleEdit();
    bool redoSampleEdit();
    bool canUndoSampleEdit() const { return ! undoStack.empty(); }
    bool canRedoSampleEdit() const { return ! redoStack.empty(); }
    std::atomic<int> editVersion { 0 };                // zaehlt Bearbeitungen (Ansicht bleibt dann stehen)
    std::shared_ptr<Wavetable>  getUserWavetable() const { return userWT; }
    const Wavetable* getTableForDisplay (int index) const;
    juce::AudioFormatManager formats;
    // Vorhoeren im Browser (spielt direkt auf den Ausgang, Originaltonhoehe)
    void previewFile (const juce::File& f);
    void stopPreview();
    std::atomic<bool> previewPlaying { false };
    std::atomic<int> sourceVersion { 0 };              // Oberflaeche fragt das ab

    // ---- Aufnahme ueber den Audio-Eingang
    enum RecState { recIdle = 0, recArmed, recRunning, recFinished, recStopping };
    void startRecording();
    void stopRecording();
    int  getRecState() const { return recState.load(); }
    double getRecordedSeconds() const { return currentSr > 0 ? (double) recPos.load() / currentSr : 0.0; }
    static constexpr double maxRecSeconds = 60.0;
    bool hasAudioInput() const { return getTotalNumInputChannels() > 0; }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> presetVersion { 0 };
    juce::MidiKeyboardState keyboardState;

    // Pegel fuer die Anzeigen (vom Audio-Thread geschrieben)
    std::atomic<float> outPeak[2] { 0.0f, 0.0f }, inPeak[2] { 0.0f, 0.0f };
    std::atomic<int> voicesActive { 0 };

    // Wheels: Anzeige (vom Audio-Thread) und Bildschirm-Wheels (von der Oberflaeche)
    std::atomic<float> bendOut { 0.0f }, modOut { 0.0f };
    void setScreenBend (float v)  { uiBend = v;  uiBendDirty = true; }
    void setScreenWheel (float v) { uiMod = v;   uiModDirty = true; }
    bool isPianoLoading() const   { return pianoWanted && ! piano->isReady(); }
    float uiScale = 1.0f;
    Harmonizer harmonizer;          // Fenstergroesse (wird im Projekt gespeichert)

private:
    void readParams();
    void resetToDefaults();
    void setParam (const juce::String& id, float plainValue);
    void timerCallback() override;
    void publishSources();
    void finishRecording();
    void restoreSources (const juce::ValueTree& tree);
    void writeSourceInfo (juce::ValueTree& tree) const;
    void applyEditedSample (std::shared_ptr<SampleData> s, bool resetMarkers);
    static bool writeWavFile (const juce::File& f, const juce::AudioBuffer<float>& audio, double sr);
    static bool isEditFile (const juce::File& f);
    std::vector<std::shared_ptr<SampleData>> undoStack, redoStack;
    juce::File editFile;                                // bearbeitete Fassung des aktuellen Samples
    static constexpr size_t maxUndo = 12;

    SynthEngine engine;
    SynthParams sp;
    juce::Reverb reverb;
    juce::Reverb::Parameters revParams;
    juce::AudioBuffer<float> synthBuf, revBuf, inputBuf, recBuf;
    float revHp[2] { 0.0f, 0.0f };
    double currentSr = 44100.0;
    int currentProgram = 0;
    juce::String presetName;

    juce::SharedResourcePointer<FactoryTables> factory;
    juce::SharedResourcePointer<PianoSet> piano;
    bool pianoWanted = false;
    std::atomic<float> uiBend { 0.0f }, uiMod { 0.0f };
    std::atomic<bool> uiBendDirty { false }, uiModDirty { false };
    ReleasePool pool;
    std::shared_ptr<SampleData> sample;                // Message-Thread
    std::shared_ptr<Wavetable>  userWT;
    juce::File userWTFile;
    bool userWTFromSample = false;
    juce::SpinLock sourceLock;
    std::shared_ptr<SampleData> pendingSample;          // geschuetzt durch sourceLock
    std::shared_ptr<Wavetable>  pendingWT;
    std::atomic<bool> sourcesDirty { true };
    std::shared_ptr<SampleData> audioSample;            // nur Audio-Thread
    std::shared_ptr<Wavetable>  audioWT;

    std::atomic<int> recState { recIdle }, recPos { 0 };
    juce::uint32 stopRequestedAt = 0;

    std::atomic<float>* raw (const char* id) { return apvts.getRawParameterValue (id); }

    std::shared_ptr<SampleData> pendingPreview, audioPreview;   // pending: sourceLock
    std::atomic<bool> previewDirty { false };
    std::atomic<int> previewGen { 0 };
    double previewPos = 0.0;
    juce::ThreadPool loaderPool { 1 };                          // zuletzt: wird zuerst beendet

    JUCE_DECLARE_WEAK_REFERENCEABLE (K55Processor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K55Processor)
};

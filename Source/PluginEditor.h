#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "LooperPanel.h"
#include "UpdatePanel.h"
#include "DrumPanel.h"
#include "SeqPanel.h"
#include "OutsPanel.h"

class K54Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit K54Editor (K54Processor&);
    ~K54Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setPage (int page);   // 0 SYNTH, 1 LOOPER, 2 DRUMS, 3 SEQ, 4 OUTS

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    struct Choice
    {
        juce::ComboBox box;
        juce::Label    label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    Knob&   makeKnob   (const char* paramID, const juce::String& caption, bool bipolar = false);
    Choice& makeChoice (const char* paramID, const juce::String& caption);
    Knob&   knob   (const char* id) { return *knobs.at (id); }
    Choice& choice (const char* id) { return *choices.at (id); }

    void placeKnob   (const char* id, juce::Rectangle<int> cell);
    void placeChoice (const char* id, juce::Rectangle<int> cell);
    void drawSection (juce::Graphics&, juce::Rectangle<int> r, const juce::String& title);
    void drawSubtitle (juce::Graphics&, juce::Rectangle<int> r, const juce::String& text);

    void rebuildPresetMenu();
    void presetSelected();
    void stepPreset (int delta);
    void savePresetDialog();
    void timerCallback() override;

    K54Processor& proc;
    K54LookAndFeel lnf;

    std::map<std::string, std::unique_ptr<Knob>>   knobs;
    std::map<std::string, std::unique_ptr<Choice>> choices;

    juce::ComboBox   presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "SAVE" };
    juce::Array<juce::File> userPresets;
    int lastPresetVersion = -1;

    juce::MidiKeyboardComponent keyboard;

    // Reiter
    juce::TextButton synthTab { "SYNTH" }, looperTab { "LOOPER" }, drumsTab { "DRUMS" }, seqTab { "SEQ" }, outsTab { "OUTS" };
    LooperPanel looperPanel;
    DrumPanel drumPanel;
    SeqPanel seqPanel;
    OutsPanel outsPanel;
    int currentPage = 0;

    // Updates von keller54.de
    juce::SharedResourcePointer<K54Updater> updater;
    juce::TextButton updateButton { "UPDATE" };
    UpdatePanel updatePanel { *updater };
    juce::String lastUpdateLabel;

    // Bereiche fuer paint()
    juce::Rectangle<int> secOsc, secFilter, secEnv, secMod, subFEnv, subAEnv, subOut, subLfo, subPlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K54Editor)
};

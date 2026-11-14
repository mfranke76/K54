#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "UpdatePanel.h"

namespace K54D
{
    inline juce::String u8 (const char* t) { return juce::String (juce::CharPointer_UTF8 (t)); }

    /** Farbe einer Spur: bunt (Regenbogen) oder Gold */
    inline juce::Colour laneColour (int lane)
    {
        if (! K54View::colorful) return K54Colours::gold;
        return K54View::rainbow ((float) lane / (float) (DrumInfo::numLanes - 1));
    }

    /** Steps pro Schlag (fuer die Schattierung im Raster) */
    inline int stepsPerBeat (int res)
    {
        switch (res) { case 0: return 2; case 2: return 8; case 3: return 3; case 4: return 6; default: return 4; }
    }
}

// ---------------------------------------------------------------------------
//  Stereo-Pegel neben dem Logo
class OutMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    void setLevels (float l, float r)
    {
        for (int c = 0; c < 2; ++c)
        {
            const float in = c == 0 ? l : r;
            level[c] = juce::jmax (in, level[c] * 0.82f);
            if (in >= hold[c] || ++holdCount[c] > 40) { hold[c] = in; holdCount[c] = 0; }
            if (in >= 0.999f) clip[c] = true;
        }
        repaint();
    }
    void mouseDown (const juce::MouseEvent&) override { clip[0] = clip[1] = false; repaint(); }
    void paint (juce::Graphics& g) override;
private:
    static float norm (float gain) { return juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (gain, -48.0f) + 48.0f) / 54.0f); }
    float level[2] {}, hold[2] {};
    int holdCount[2] {};
    bool clip[2] { false, false };
};

// ---------------------------------------------------------------------------
//  Step-Raster: 12 Spuren x 8..32 Steps
class StepGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit StepGrid (K54DProcessor& p) : proc (p) {}

    static constexpr int headW = 196, rowH = 24;

    std::function<void (int)> onLaneSelected;
    int editPattern = 0;

    juce::Rectangle<int> cellBounds (int lane, int step) const;
    int  stepAtX (int x) const;
    int  laneAtY (int y) const { return juce::jlimit (-1, DrumInfo::numLanes - 1, y / rowH); }
    int  stepsX0() const { return headW + 8; }
    int  stepsW() const  { return getWidth() - stepsX0(); }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override;
    void mouseMove (const juce::MouseEvent& e) override;

    void setPlayStep (int s)    { if (s != playStep) { playStep = s; repaint(); } }
    void setLaneFlash (int lane, float v) { flash[(size_t) lane] = v; }
    int  lastVel = 100;

private:
    void stepMenu (int lane, int step);
    void toggleMuteSolo (int lane, bool solo);
    juce::Rectangle<int> muteBox (int lane) const { return { headW - 50, lane * rowH + 4, 22, rowH - 8 }; }
    juce::Rectangle<int> soloBox (int lane) const { return { headW - 25, lane * rowH + 4, 22, rowH - 8 }; }

    K54DProcessor& proc;
    int playStep = -1;
    int dragLane = -1, dragMode = 0;        // 1 = einschalten, 2 = ausschalten, 3 = Lautstaerke
    int dragStep = -1, dragStartVel = 0;
    float dragStartY = 0;
    std::array<float, DrumInfo::numLanes> flash {};
};

// ---------------------------------------------------------------------------
//  Lautstaerke pro Step der gewaehlten Spur
class VelocityLane : public juce::Component, public juce::SettableTooltipClient
{
public:
    VelocityLane (K54DProcessor& p, StepGrid& g) : proc (p), grid (g)
    {
        setTooltip (K54D::u8 ("Lautstärke pro Step: Balken ziehen = lauter/leiser (nur gesetzte Steps). Doppelklick = 100"));
    }
    int editPattern = 0;
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override { paintAt (e); }
    void mouseDrag (const juce::MouseEvent& e) override { paintAt (e); }
    void mouseDoubleClick (const juce::MouseEvent& e) override;
private:
    void paintAt (const juce::MouseEvent& e);
    K54DProcessor& proc;
    StepGrid& grid;
};

// ---------------------------------------------------------------------------
//  MIDI-Knopf: Ziehen in die DAW = Pattern als MIDI-Clip, Klick = speichern
class MidiDragButton : public juce::TextButton
{
public:
    std::function<juce::File()> makeFile;
    std::function<void()> onSaveClick;
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 6 || makeFile == nullptr) return;
        dragging = true;
        const auto f = makeFile();
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; });
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! dragging && e.getDistanceFromDragStart() < 6 && onSaveClick) onSaveClick();
        dragging = false;
        TextButton::mouseUp (e);
    }
private:
    bool dragging = false;
};

// ---------------------------------------------------------------------------
class K54DMain : public juce::Component,
                 public juce::FileDragAndDropTarget,
                 private juce::Timer
{
public:
    explicit K54DMain (K54DProcessor&);
    ~K54DMain() override;

    static constexpr int W = 1160, H = 720, headerH = 80;
    std::function<void (float)> onScaleChosen;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    struct Knob
    {
        juce::Slider s;
        juce::Label l;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a;
    };

    void timerCallback() override;
    void setupKnob (Knob& k, const juce::String& caption, const juce::String& tip, bool bipolar = false);
    void attachLane (int lane);
    void selectLane (int lane);
    void refreshAll();
    void rebuildPresetMenu();
    void presetChosen();
    void stepPreset (int delta);
    void savePresetDialog();
    void loadSampleDialog();
    void showHelp();
    void velocityTool (int which);
    void setEditPattern (int p);
    juce::AudioParameterChoice* patternParam() const;
    void drawSection (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title);

    K54DProcessor& proc;
    K54LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };
    juce::Image portrait;
    std::unique_ptr<juce::PropertiesFile> settings;

    // Kopf
    OutMeter outMeter;
    juce::Slider masterGain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAtt;
    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "SAVE" };
    juce::TextButton colorButton { "FARBIG" }, helpButton { "?" }, updateButton { "UPDATE" };
    juce::ComboBox sizeBox;

    // Transport
    juce::TextButton seqButton { "SEQ" }, playButton { "PLAY" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqAtt;
    juce::ComboBox stepsBox, resBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stepsAtt, resAtt;
    juce::Label stepsLabel, resLabel, patLabel;
    std::array<juce::TextButton, 4> patButtons;
    juce::TextButton copyButton { "KOPIEREN" }, clearButton { K54D::u8 ("LÖSCHEN") };
    MidiDragButton midiButton;
    juce::Rectangle<int> statusArea;

    // Raster
    StepGrid grid;
    VelocityLane velLane;
    std::array<juce::TextButton, 6> velTools;
    juce::Rectangle<int> velHead;

    // Sound der gewaehlten Spur
    juce::ComboBox modelBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAtt;
    juce::TextButton loadButton { "SAMPLE LADEN" }, clearSampleButton { "X" }, auditionButton { K54D::u8 ("▶") };
    std::array<Knob, 8> laneKnobs;      // Level Tune Decay Tone Snap Drive Pan Room
    std::array<Knob, 2> grooveKnobs;    // Swing Humanize
    std::array<Knob, 3> masterKnobs;    // Fat Glue Room
    juce::Rectangle<int> secSound, secGroove, secMaster, sampleNameArea;

    juce::SharedResourcePointer<K54DUpdater> updater;
    UpdatePanel updatePanel { *updater };

    int lastStateVersion = -1, lastChange = -1, lastPatternShown = -1, lastSteps = -1, lastRes = -1;
    int shownLane = -1;
    std::array<int, DrumInfo::numLanes> lastHits {};
    std::array<float, DrumInfo::numLanes> laneFlash {};
    std::unique_ptr<juce::FileChooser> chooser;
    int blink = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K54DMain)
};

// ---------------------------------------------------------------------------
//  Editor: skaliert die Hauptansicht (70-150 %)
class K54DEditor : public juce::AudioProcessorEditor
{
public:
    explicit K54DEditor (K54DProcessor& p);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (K54Colours::black); }
private:
    void applyScale (float s);
    K54DProcessor& proc;
    K54DMain main;
};

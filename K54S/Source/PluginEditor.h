#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "ModPanel.h"
#include "SamplePanel.h"
#include "HarmonyPanel.h"
#include "KeysPanel.h"
#include "UpdatePanel.h"

// ---------------------------------------------------------------------------
//  Main-Ausgangspegel neben dem Logo (Stereo, Peak-Hold, Clip-Anzeige)
class OutputMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    /** true, wenn sich sichtbar etwas geaendert hat (spart Neuzeichnen) */
    void setLevels (float l, float r)
    {
        const auto now = juce::Time::getMillisecondCounter();
        const float before = level[0] + level[1] + hold[0] + hold[1];
        update (0, l, now);
        update (1, r, now);
        if (std::abs (before - (level[0] + level[1] + hold[0] + hold[1])) > 0.0005f || l >= 1.0f || r >= 1.0f) repaint();
    }
    void mouseDown (const juce::MouseEvent&) override { clip[0] = clip[1] = false; repaint(); }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        auto b = getLocalBounds().toFloat();
        auto bars = b.removeFromTop (b.getHeight() - 12.0f);
        const float bh = (bars.getHeight() - 3.0f) * 0.5f;
        for (int c = 0; c < 2; ++c)
        {
            auto r = juce::Rectangle<float> (bars.getX(), bars.getY() + c * (bh + 3.0f), bars.getWidth(), bh);
            auto clipBox = r.removeFromRight (10.0f);
            r.removeFromRight (3.0f);
            g.setColour (panelHi);
            g.fillRoundedRectangle (r, 3.0f);
            const float f = norm (level[c]);
            juce::ColourGradient grad (goldDim, r.getX(), 0, juce::Colour (0xffE0503A), r.getRight(), 0, false);
            grad.addColour (norm (juce::Decibels::decibelsToGain (-6.0f)), gold);
            grad.addColour (norm (1.0f) - 0.001f, cream);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (r.withWidth (r.getWidth() * f), 3.0f);
            g.setColour (cream);
            g.fillRect (r.getX() + r.getWidth() * norm (hold[c]) - 1.0f, r.getY(), 2.0f, r.getHeight());
            g.setColour (clip[c] ? juce::Colour (0xffE0503A) : panelHi);
            g.fillRoundedRectangle (clipBox, 3.0f);
        }
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 7.0f, 0.05f));
        g.setColour (creamDim.withAlpha (0.6f));
        const float w = bars.getWidth() - 13.0f;
        for (int db : { -36, -24, -12, -6, 0 })
        {
            const float x = bars.getX() + w * norm (juce::Decibels::decibelsToGain ((float) db));
            g.fillRect (x - 0.5f, b.getY(), 1.0f, 3.0f);
            g.drawText (juce::String (db), juce::Rectangle<float> (x - 12.0f, b.getY() + 2.0f, 24.0f, 10.0f), juce::Justification::centred);
        }
    }

private:
    static float norm (float gain) { return juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (gain, -48.0f) + 48.0f) / 54.0f); }
    void update (int c, float in, juce::uint32 now)
    {
        level[c] = juce::jmax (in, level[c] * 0.8f);
        if (level[c] < 0.0005f) level[c] = 0.0f;
        if (in >= hold[c] || now - holdTime[c] > 1500) { hold[c] = in; holdTime[c] = now; }
        if (in >= 1.0f) clip[c] = true;
    }
    float level[2] {}, hold[2] {};
    juce::uint32 holdTime[2] {};
    bool clip[2] { false, false };
};

// ---------------------------------------------------------------------------
//  Pitch- und Mod-Wheel: zeigen die Wheels des Keyboards, lassen sich auch mit der Maus bewegen
class WheelView : public juce::Component, public juce::SettableTooltipClient
{
public:
    WheelView (bool isPitch, std::function<void (float)> send) : pitch (isPitch), onMove (std::move (send)) {}

    void setValue (float v)      // vom Keyboard (nur wenn nicht gerade mit der Maus bewegt)
    {
        if (dragging || std::abs (v - value) < 0.002f) return;
        value = v;
        repaint();
    }
    void mouseDown (const juce::MouseEvent& e) override { dragging = true; startY = e.position.y; startV = value; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const float range = (float) getHeight() - 20.0f;
        const float dv = (startY - e.position.y) / range * (pitch ? 2.0f : 1.0f);
        value = juce::jlimit (pitch ? -1.0f : 0.0f, 1.0f, startV + dv);
        onMove (value);
        repaint();
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        dragging = false;
        if (pitch) { value = 0.0f; onMove (0.0f); repaint(); }   // Pitch-Wheel federt zurueck
    }
    void mouseDoubleClick (const juce::MouseEvent&) override { value = pitch ? 0.0f : value; if (! pitch) { value = 0.0f; onMove (0.0f); } repaint(); }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        auto b = getLocalBounds().toFloat();
        auto lab = b.removeFromBottom (12.0f);
        auto w = b.reduced (2.0f, 1.0f);
        g.setColour (juce::Colour (0xff0E0E11));
        g.fillRoundedRectangle (w, 6.0f);
        // Walze mit Rillen, die sich mit dem Wert bewegen
        {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (w.reduced (2.0f).toNearestInt());
        const float off = value * (pitch ? 0.5f : 1.0f) * w.getHeight() * 0.6f;
        for (float y = w.getY() - 40.0f; y < w.getBottom() + 40.0f; y += 6.0f)
        {
            const float yy = y - std::fmod (off, 6.0f);
            const float d = std::abs (yy - w.getCentreY()) / (w.getHeight() * 0.5f);
            g.setColour (juce::Colour (0xff2C2C33).withAlpha (juce::jlimit (0.0f, 1.0f, 1.0f - d * d)));
            g.fillRect (w.getX() + 3.0f, yy, w.getWidth() - 6.0f, 1.5f);
        }
        // Markierung
        const float my = pitch ? w.getCentreY() - value * (w.getHeight() * 0.5f - 6.0f)
                               : w.getBottom() - 6.0f - value * (w.getHeight() - 12.0f);
        g.setColour (value != 0.0f ? gold : creamDim.withAlpha (0.6f));
        g.fillRoundedRectangle (w.getX() + 3.0f, my - 2.0f, w.getWidth() - 6.0f, 4.0f, 2.0f);
        }
        g.setColour (creamDim.withAlpha (0.7f));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 6.5f, 0.05f));
        g.drawText (pitch ? "PITCH" : "MOD", lab, juce::Justification::centred);
    }

private:
    bool pitch, dragging = false;
    float value = 0.0f, startY = 0.0f, startV = 0.0f;
    std::function<void (float)> onMove;
};

// ---------------------------------------------------------------------------
//  Gesamte Oberflaeche in fester Groesse (1160 x 640); das Editor-Fenster skaliert sie
class K55Main : public juce::Component, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    static constexpr int W = 1160, H = 640;
    explicit K55Main (K55Processor&);
    ~K55Main() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setPage (int page);   // 0 SYNTH, 1 MOD, 2 SAMPLE, 3 FX, 4 HARMONY, 5 KEYS
    std::function<void (float)> onScaleChosen;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragMove (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

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
    void drawSubtitle (juce::Graphics&, juce::Rectangle<int> r, const juce::String& text);

    void rebuildPresetMenu();
    void presetSelected();
    void stepPreset (int delta);
    void savePresetDialog();
    void timerCallback() override;
    int dropTarget (int x, int y) const;

    K55Processor& proc;
    K54LookAndFeel lnf;

    std::map<std::string, std::unique_ptr<Knob>>   knobs;
    std::map<std::string, std::unique_ptr<Choice>> choices;

    juce::ComboBox   presetBox, sizeBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "SAVE" };
    juce::Array<juce::File> userPresets;
    int lastPresetVersion = -1;

    OutputMeter outMeter;
    juce::Slider outGain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAtt;

    juce::MidiKeyboardComponent keyboard;
    WheelView pitchWheel, modWheel;

    juce::TextButton synthTab { "SYNTH" }, modTab { "MOD" }, sampleTab { "SAMPLE" }, fxTab { "FX" }, harmTab { "HARMONY" }, keysTab { "KEYS" };
    juce::TextButton harmButton { ParamChoices::u8 ("\xe2\x99\xab HARMONIE") };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> harmAtt;
    juce::Image portrait;
    ModPanel modPanel;
    HarmonyPanel harmPanel;
    SamplePanel samplePanel;
    FxPanel fxPanel;
    KeysPanel keysPanel;
    juce::SharedResourcePointer<K54SUpdater> updater;
    juce::TextButton updateButton { "UPDATE" };
    UpdatePanel updatePanel { *updater };
    juce::String lastUpdateLabel;
    int currentPage = 0, voices = 0;
    bool pianoLoading = false;
    int lastWave[3] { -1, -1, -1 };
    bool lastEmpty[3] {};

    juce::Rectangle<int> secOsc, secFilter, secEnv, secMod, subFEnv, subAEnv, subOut, subLfo, subPlay, statusArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (K55Main)
};

// ---------------------------------------------------------------------------
//  Plug-in-Fenster: frei skalierbar (70 – 160 %), Seitenverhaeltnis fest
class K55Editor : public juce::AudioProcessorEditor
{
public:
    explicit K55Editor (K55Processor& p);
    void resized() override;
    void setPage (int page) { main.setPage (page); }

private:
    K55Processor& proc;
    K55Main main;
    juce::TooltipWindow tooltips { this, 600 };
    bool ready = false;
};

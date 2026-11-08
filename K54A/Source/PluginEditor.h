// K-54 Analyzer · Keller54 · by Frank E.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "Spectrum.h"

namespace k54a
{
namespace col
{
    const juce::Colour bg      { 0xff0a0a0b };
    const juce::Colour panel   { 0xff131316 };
    const juce::Colour panel2  { 0xff1b1b20 };
    const juce::Colour line    { 0xff2b2b31 };
    const juce::Colour grid    { 0xff222228 };
    const juce::Colour gold    { 0xffd4a537 };
    const juce::Colour goldHi  { 0xfff2cf6b };
    const juce::Colour text    { 0xffe9e4d6 };
    const juce::Colour dim     { 0xff8d877a };
    const juce::Colour ref     { 0xff46c3f0 };
    const juce::Colour good    { 0xff6fd08c };
    const juce::Colour warn    { 0xffef9a3c };
    const juce::Colour red     { 0xffe5484d };
}

juce::String u8s (const char* s);

//==============================================================================
class K54Look : public juce::LookAndFeel_V4
{
public:
    K54Look();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minP, float maxP,
                           juce::Slider::SliderStyle, juce::Slider&) override;
};

//==============================================================================
struct Readouts
{
    float mixM = -100, mixS = -100, mixI = -100, mixLra = 0, mixTp = -100;
    float refM = -100, refS = -100, refI = -100, refLra = 0, refTp = -100;
    bool refLoaded = false, refRunning = false;
    float target = 1000.0f; // 1000 = kein Ziel
};

class SpectrumView : public juce::Component
{
public:
    SpectrumView (SpectrumAnalyser& mix, SpectrumAnalyser& ref) : mixA (mix), refA (ref) {}
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    bool showMix = true, showRef = true, holdOn = false, refLoaded = false;
    float slope = 4.5f;

    static constexpr float topDb = 6.0f, botDb = -90.0f;

private:
    float xForFreq (float f, juce::Rectangle<float> r) const;
    float yForDb (float db, juce::Rectangle<float> r) const;
    juce::Path makePath (const std::vector<float>& v, juce::Rectangle<float> r, bool closed) const;

    SpectrumAnalyser& mixA;
    SpectrumAnalyser& refA;
    juce::Point<float> mouse;
    bool mouseIn = false;
};

class LoudnessPanel : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    Readouts r;
};

class MeterPanel : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void tick (AnalyzerProcessor&);

    struct Bar { float peak = -100, hold = -100, rms = -100; int holdAge = 0; void feed (float pk, float rmsV); };
    Bar mL, mR, rL, rR;
    float mixM = -100, refM = -100, mixCorr = 0, refCorr = 0, mixMax = -100, refMax = -100;
    bool refRunning = false;
};

class HistoryView : public juce::Component
{
public:
    HistoryView() { mix.assign (len, -100.0f); ref.assign (len, -100.0f); }
    void paint (juce::Graphics&) override;
    void push (float m, float r) { mix[(size_t) pos] = m; ref[(size_t) pos] = r; pos = (pos + 1) % len; }
    void clear() { std::fill (mix.begin(), mix.end(), -100.0f); std::fill (ref.begin(), ref.end(), -100.0f); }

    static constexpr int len = 600; // 60 s bei 10 Hz
    float target = 1000.0f;

private:
    std::vector<float> mix, ref;
    int pos = 0;
};

class WaveView : public juce::Component
{
public:
    explicit WaveView (AnalyzerProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { seek (e); }
    void mouseDrag (const juce::MouseEvent& e) override { seek (e); }
    bool dragHover = false;

private:
    void seek (const juce::MouseEvent&);
    AnalyzerProcessor& proc;
};

class HelpOverlay : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { setVisible (false); }
};

//==============================================================================
class Content : public juce::Component
{
public:
    static constexpr int W = 1200, H = 740;
    explicit Content (AnalyzerProcessor&);
    ~Content() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();
    void setDragHover (bool h) { wave.dragHover = h; wave.repaint(); }

private:
    void openFileChooser();
    void setAB (bool b);
    void panel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title) const;

    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;

    AnalyzerProcessor& proc;
    SpectrumAnalyser mixA, refA;
    SpectrumView spectrum { mixA, refA };
    LoudnessPanel loud;
    MeterPanel meters;
    HistoryView history;
    WaveView wave { proc };
    HelpOverlay help;

    juce::TextButton btnA, btnB, btnReset, btnHelp, btnAuto;
    juce::ComboBox cbTarget, cbSlope, cbSpeed, cbSmooth;
    juce::TextButton btnShowMix, btnShowRef, btnHold;
    juce::TextButton btnLoad, btnClear, btnSync, btnPlay, btnMatch;
    juce::Slider sGain;
    juce::Label lbSlope, lbSpeed, lbSmooth, lbGain, lbTarget;

    std::unique_ptr<BA> aShowMix, aShowRef, aHold, aSync, aMatch, aAuto;
    std::unique_ptr<CA> aTarget, aSlope, aSpeed, aSmooth;
    std::unique_ptr<SA> aGain;
    std::unique_ptr<juce::FileChooser> chooser;

    std::vector<float> pullBuf;
    int tickCount = 0, lastRefVersion = -1;
    juce::Rectangle<int> rHeader, rSpec, rLoud, rMeter, rHist, rRef;

    friend class AnalyzerEditor;
};

//==============================================================================
class AnalyzerEditor : public juce::AudioProcessorEditor,
                       public juce::FileDragAndDropTarget,
                       private juce::Timer
{
public:
    explicit AnalyzerEditor (AnalyzerProcessor&);
    ~AnalyzerEditor() override;

    void paint (juce::Graphics& g) override { g.fillAll (col::bg); }
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { content.setDragHover (true); }
    void fileDragExit (const juce::StringArray&) override { content.setDragHover (false); }
    void filesDropped (const juce::StringArray& files, int, int) override;

    Content& getContent() { return content; }
    void runTick() { timerCallback(); }

private:
    void timerCallback() override { content.tick(); }

    AnalyzerProcessor& proc;
    K54Look look;
    Content content;
};

} // namespace k54a

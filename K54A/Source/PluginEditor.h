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
    const juce::Colour diff    { 0xffc9a2f2 };
    const juce::Colour good    { 0xff6fd08c };
    const juce::Colour warn    { 0xffef9a3c };
    const juce::Colour red     { 0xffe5484d };
    const juce::Colour plot    { 0xff0c0c0e };
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
    float mixM = -100, mixS = -100, mixI = -100, mixLra = 0, mixTp = -100, mixSMax = -100, mixMMax = -100;
    float refM = -100, refS = -100, refI = -100, refLra = 0, refTp = -100, refSMax = -100, refMMax = -100;
    bool refLoaded = false, refRunning = false, refLive = false;
    float target = 1000.0f;  // 1000 = kein Ziel
    float tpLimit = -1.0f;
};

class SpectrumView : public juce::Component
{
public:
    SpectrumView (SpectrumAnalyser& mix, SpectrumAnalyser& ref) : mixA (mix), refA (ref) {}
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    bool showMix = true, showRef = true, showDiff = false, holdOn = false, refLoaded = false, longTerm = false;
    juce::String channelName;

    static constexpr float topDb = 6.0f, botDb = -90.0f;

private:
    float xForFreq (float f, juce::Rectangle<float> r) const;
    float yForDb (float db, juce::Rectangle<float> r) const;
    juce::Path makePath (const std::vector<float>& v, juce::Rectangle<float> r, bool closed) const;
    void computeDiff();

    SpectrumAnalyser& mixA;
    SpectrumAnalyser& refA;
    std::vector<float> diff, diffTmp;
    juce::Point<float> mouse;
    bool mouseIn = false;
};

class LoudnessPanel : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    Readouts r;
    bool pro = false;
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

class ScopeView : public juce::Component
{
public:
    ScopeView();
    void paint (juce::Graphics&) override;
    void pushMix (const float* L, const float* R, int n) { push (mL, mR, mPos, L, R, n); }
    void pushRef (const float* L, const float* R, int n) { push (rL, rR, rPos, L, R, n); }
    void clearRef() { std::fill (rL.begin(), rL.end(), 0.0f); std::fill (rR.begin(), rR.end(), 0.0f); }
    bool showRef = true;
    float mixCorr = 0, refCorr = 0;
    bool refRunning = false;

private:
    static constexpr int len = 2048;
    void push (std::vector<float>& a, std::vector<float>& b, int& p, const float* L, const float* R, int n);
    std::vector<float> mL, mR, rL, rR;
    int mPos = 0, rPos = 0;
    float scaleSmooth = 1.0f;
};

class TonalView : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    std::array<float, SpecMath::numBands> mix {}, ref {};
    bool mixValid = false, refValid = false, refLoaded = false, refLive = false, pro = false;
    double mixSeconds = 0.0;
    juce::String channelNote;
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
    static constexpr int W = 1280, H = 800;
    explicit Content (AnalyzerProcessor&);
    ~Content() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();
    void setDragHover (bool h) { wave.dragHover = h; wave.repaint(); }
    void setPro (bool pro);
    bool isPro() const noexcept { return pro; }
    void showHelp (bool s) { help.setVisible (s); if (s) help.toFront (false); }
    void setScopeTab (bool scope);
    juce::String buildReport() const;

private:
    void openFileChooser();
    void exportReport();
    void setAB (bool b);
    void setParam (const char* id, float value);
    void updateVisibility();
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
    ScopeView scope;
    TonalView tonal;
    WaveView wave { proc };
    HelpOverlay help;

    juce::TextButton btnSimple, btnPro, btnA, btnB, btnReset, btnHelp, btnAuto, btnReport;
    juce::TextButton btnTabHist, btnTabScope;
    juce::ComboBox cbTarget, cbSlope, cbSpeed, cbSmooth, cbRes, cbChannel, cbMonitor, cbBand;
    juce::TextButton btnShowMix, btnShowRef, btnShowDiff, btnHold;
    juce::TextButton btnLoad, btnClear, btnSync, btnPlay, btnMatch;
    std::array<juce::TextButton, AnalyzerProcessor::numSlots> btnSlot;
    juce::TextButton btnLive;
    juce::Slider sGain;
    juce::Label lbSlope, lbSpeed, lbSmooth, lbRes, lbChannel, lbGain, lbTarget, lbMonitor;

    std::unique_ptr<BA> aShowMix, aShowRef, aShowDiff, aHold, aSync, aMatch, aAuto;
    std::unique_ptr<CA> aTarget, aSlope, aSpeed, aSmooth, aRes, aChannel, aMonitor, aBand;
    std::unique_ptr<SA> aGain;
    std::unique_ptr<juce::FileChooser> chooser;

    std::vector<float> pullL, pullR, mono;
    int tickCount = 0, lastRefVersion = -1, lastSlot = -1, lastReset = -1, lastRes = -1, lastChannel = -1;
    bool pro = false, scopeTab = true;
    juce::Rectangle<int> rHeader, rSpec, rTonal, rHist, rLoud, rMeter, rRef;

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

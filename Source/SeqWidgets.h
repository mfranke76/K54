#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "LooperPanel.h"   // LoopColours

// ---------------------------------------------------------------------------
//  Kleine Bausteine fuer DRUMS / SEQ / OUTS
// ---------------------------------------------------------------------------

/** Schritt-Taste: aus / an / Akzent, mit Lauflicht */
class StepButton : public juce::Component
{
public:
    std::function<void()> onClick;
    int  state = 0;        // 0 aus, 1 an, 2 Akzent
    bool playing = false, beatStart = false, disabled = false;

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        auto r = getLocalBounds().toFloat().reduced (1.5f);
        juce::Colour c = beatStart ? juce::Colour (0xff222226) : panelHi;
        if (state == 1) c = goldDim;
        if (state == 2) c = gold;
        if (disabled) c = c.withAlpha (0.25f);
        g.setColour (c);
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (playing ? cream : line);
        g.drawRoundedRectangle (r, 3.0f, playing ? 2.0f : 1.0f);
        if (state == 2)
        {
            g.setColour (black.withAlpha (0.6f));
            g.fillEllipse (r.withSizeKeepingCentre (5.0f, 5.0f));
        }
    }
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
};

/** Kleiner Drehregler ohne Textfeld, Wert erscheint beim Drehen */
inline void setupSmallKnob (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
    s.setPopupDisplayEnabled (true, true, nullptr, 1500);
    s.setMouseDragSensitivity (200);
}

/** Umschalter im K-54-Stil (gold wenn an) */
inline void setupToggle (juce::TextButton& b, const juce::String& tip = {})
{
    b.setClickingTogglesState (true);
    if (tip.isNotEmpty()) b.setTooltip (tip);
}

/** Gemeinsame Kopfzeile: PLAY, Pattern A-H, Takt-Anzeige */
class SeqTransportBar : public juce::Component
{
public:
    explicit SeqTransportBar (K54Processor& p) : proc (p)
    {
        addAndMakeVisible (playButton);
        playButton.onClick = [this]
        {
            if (proc.looper.getSource() != ClockSource::Internal) return;   // DAW/Clock steuert
            if (proc.looper.isRunning()) proc.looper.pressTransportStop();
            else                         proc.looper.pressTransportStart();
        };
        for (int i = 0; i < StepSequencer::numPatterns; ++i)
        {
            auto* b = patButtons.add (new juce::TextButton (juce::String::charToString ((juce::juce_wchar) ('A' + i))));
            b->setTooltip ("Pattern " + b->getButtonText() + " (wechselt am naechsten Takt)");
            b->onClick = [this, i]
            {
                if (auto* prm = proc.apvts.getParameter (ParamID::seqPattern))
                    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
                if (onPatternChange) onPatternChange();
            };
            addAndMakeVisible (b);
        }
        addAndMakeVisible (copyButton);
        copyButton.setTooltip ("Aktuelles Pattern in das naechste kopieren");
        copyButton.onClick = [this]
        {
            const int from = editPattern(), to = (from + 1) % StepSequencer::numPatterns;
            proc.sequencer.copyPattern (from, to);
            if (auto* prm = proc.apvts.getParameter (ParamID::seqPattern))
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) to));
            if (onPatternChange) onPatternChange();
        };
        addAndMakeVisible (clearButton);
        clearButton.setTooltip ("Aktuelles Pattern leeren");
        clearButton.onClick = [this] { proc.sequencer.clearPattern (editPattern()); if (onPatternChange) onPatternChange(); };
    }

    std::function<void()> onPatternChange;

    int editPattern() const { return juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::seqPattern)->load()); }

    void refresh()
    {
        const auto src = proc.looper.getSource();
        const bool internal = src == ClockSource::Internal;
        playButton.setButtonText (! internal ? (src == ClockSource::Host ? "DAW SYNC" : "MIDI SYNC")
                                             : (proc.looper.isRunning() ? "STOP" : "PLAY"));
        playButton.setToggleState (proc.looper.isRunning(), juce::dontSendNotification);
        playButton.setEnabled (internal);
        const int p = editPattern(), ap = proc.sequencer.activePattern();
        for (int i = 0; i < patButtons.size(); ++i)
        {
            patButtons[i]->setToggleState (i == p, juce::dontSendNotification);
            patButtons[i]->setColour (juce::TextButton::textColourOffId, i == ap && i != p ? K54Colours::gold : K54Colours::cream);
        }
        repaint (infoArea);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        playButton.setBounds (r.removeFromLeft (104));
        r.removeFromLeft (12);
        for (auto* b : patButtons) { b->setBounds (r.removeFromLeft (28)); r.removeFromLeft (3); }
        r.removeFromLeft (6);
        copyButton.setBounds (r.removeFromLeft (56));
        r.removeFromLeft (4);
        clearButton.setBounds (r.removeFromLeft (60));
        r.removeFromLeft (10);
        infoArea = r.removeFromLeft (150);
        extraArea = r;
    }

    juce::Rectangle<int> getExtraArea() const { return extraArea.translated (getX(), getY()); }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        const auto src = proc.looper.getSource();
        const char* s = src == ClockSource::Host ? "DAW" : (src == ClockSource::MidiClock ? "MIDI CLOCK"
                       : (proc.looper.usesHostTempo() ? "DAW-TEMPO" : "INTERN"));
        g.setColour (proc.looper.isRunning() ? gold : creamDim.withAlpha (0.5f));
        g.fillEllipse (infoArea.withWidth (10).withSizeKeepingCentre (8, 8).toFloat());
        g.setColour (cream);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.5f, 0.12f));
        g.drawText (juce::String (s) + "  " + juce::String (proc.looper.getBpm(), 1) + " BPM",
                    infoArea.withTrimmedLeft (16), juce::Justification::centredLeft);
    }

private:
    K54Processor& proc;
    juce::TextButton playButton { "PLAY" }, copyButton { "COPY" }, clearButton { "CLEAR" };
    juce::OwnedArray<juce::TextButton> patButtons;
    juce::Rectangle<int> infoArea, extraArea;
};

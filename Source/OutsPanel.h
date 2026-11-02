#pragma once
#include "SeqWidgets.h"

// ---------------------------------------------------------------------------
//  Reiter OUTS: jede Quelle auf Main oder eigenen Ausgang in der DAW
// ---------------------------------------------------------------------------
class OutsPanel : public juce::Component, private juce::Timer
{
public:
    explicit OutsPanel (K54Processor& p) : proc (p)
    {
        auto add = [this] (const juce::String& name, const juce::String& paramID, int bus)
        {
            auto* r = rows.add (new Row());
            r->name = name; r->bus = bus; r->paramID = paramID;
            setupToggle (r->button, "Eigener Ausgang " + chanText (bus));
            r->button.setButtonText ("SEPARAT");
            addAndMakeVisible (r->button);
            att.add (new BA (proc.apvts, paramID, r->button));
        };
        add ("SYNTH", ParamID::outSynth, K54Processor::busSynth);
        for (int l = 0; l < DrumInfo::numLanes; ++l)
            add (juce::String (DrumInfo::shortNames[l]), ParamID::drum (l, "Out"), K54Processor::busDrum0 + l);
        for (int t = 0; t < Looper::numTracks; ++t)
            add ("LOOP " + juce::String (t + 1), ParamID::outLoop (t), K54Processor::busLoop0 + t);

        for (auto* b : { &allMain, &allSep }) addAndMakeVisible (b);
        allMain.onClick = [this] { setAll (false); };
        allSep.onClick  = [this] { setAll (true); };
    }

    void visibilityChanged() override { if (isVisible()) startTimerHz (4); else stopTimer(); }

    void resized() override
    {
        auto r = getLocalBounds();
        listArea = r.removeFromLeft (560);
        r.removeFromLeft (12);
        helpArea = r;
        auto l = listArea.reduced (14, 12);
        titleArea = l.removeFromTop (22);
        auto buttons = l.removeFromBottom (30);
        allMain.setBounds (buttons.removeFromLeft (140));
        buttons.removeFromLeft (8);
        allSep.setBounds (buttons.removeFromLeft (160));
        l.removeFromBottom (8);
        const int colW = (l.getWidth() - 16) / 2;
        auto colA = l.removeFromLeft (colW);
        l.removeFromLeft (16);
        auto colB = l;
        const int rowH = juce::jmin (36, colA.getHeight() / 7);
        for (int i = 0; i < rows.size(); ++i)
        {
            auto& col = i < 7 ? colA : colB;
            auto row = col.removeFromTop (rowH).reduced (0, 3);
            rows[i]->area = row;
            rows[i]->button.setBounds (row.removeFromRight (92));
        }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        for (auto a : { listArea, helpArea })
        {
            g.setColour (panel);
            g.fillRoundedRectangle (a.toFloat(), 8.0f);
            g.setColour (line);
            g.drawRoundedRectangle (a.toFloat().reduced (0.5f), 8.0f, 1.0f);
        }
        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.5f, 0.3f));
        g.drawText ("OUTPUTS", titleArea, juce::Justification::centredLeft);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.12f));
        g.setColour (creamDim);
        g.drawText ("AUS = MAIN 1/2   AN = EIGENER AUSGANG", titleArea, juce::Justification::centredRight);

        for (auto* r : rows)
        {
            auto a = r->area;
            const bool sep = r->button.getToggleState();
            const bool active = proc.isOutBusActive (r->bus);
            g.setColour (! sep ? creamDim.withAlpha (0.35f) : (active ? gold : juce::Colour (0xffE0554B)));
            g.fillEllipse (a.removeFromLeft (12).withSizeKeepingCentre (8, 8).toFloat());
            a.removeFromLeft (6);
            g.setColour (cream);
            g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 9.5f, 0.1f));
            g.drawText (r->name, a.removeFromLeft (84), juce::Justification::centredLeft);
            g.setColour (creamDim);
            g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.08f));
            const juce::String where = ! sep ? juce::String ("MAIN 1/2")
                                             : chanText (r->bus) + (active ? "" : "  (IN DAW AUS)");
            g.drawText (where, a.withTrimmedRight (96), juce::Justification::centredLeft);
        }

        // Hilfe fuer die DAWs
        auto h = helpArea.reduced (16, 12);
        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.5f, 0.3f));
        g.drawText ("IN DER DAW", h.removeFromTop (22), juce::Justification::centredLeft);
        h.removeFromTop (6);
        struct Tip { const char* daw; const char* text; };
        static const Tip tips[] {
            { "LOGIC PRO",  "K-54 als \"Multi-Output\" laden (AU-Instrumente > Keller54 > K-54 > Multi Output). Im Mixer beim K-54 auf das \"+\" klicken, dann bekommt jeder Ausgang einen eigenen Kanal." },
            { "ABLETON",    "Neue Audiospur: \"Audio From\" = K-54-Spur, darunter den Ausgang waehlen (z. B. Kick). Monitor auf \"In\"." },
            { "CUBASE",     "Im Instrument-Rack beim K-54 auf den Ausgaenge-Pfeil klicken und die gewuenschten Ausgaenge aktivieren." },
            { "BITWIG",     "Im Geraetefenster des K-54 \"+\" bei den Ausgaengen (Chains) - jeder Ausgang wird eine eigene Spur." },
        };
        for (auto& t : tips)
        {
            g.setColour (cream);
            g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 9.0f, 0.12f));
            g.drawText (t.daw, h.removeFromTop (16), juce::Justification::centredLeft);
            g.setColour (creamDim);
            g.setFont (juce::FontOptions (12.5f));
            g.drawFittedText (t.text, h.removeFromTop (56), juce::Justification::topLeft, 4, 1.0f);
            h.removeFromTop (6);
        }
        g.setColour (creamDim.withAlpha (0.8f));
        g.setFont (juce::FontOptions (11.5f));
        g.drawFittedText ("Roter Punkt: Ausgang ist in der DAW noch nicht aktiv - der Klang bleibt dann auf Main, nichts geht verloren.",
                          h, juce::Justification::topLeft, 3, 1.0f);
    }

private:
    struct Row
    {
        juce::String name, paramID;
        int bus = 0;
        juce::TextButton button;
        juce::Rectangle<int> area;
    };

    static juce::String chanText (int bus)
    {
        return "OUT " + juce::String (bus * 2 + 1) + "/" + juce::String (bus * 2 + 2);
    }

    void setAll (bool sep)
    {
        for (auto* r : rows)
            if (auto* prm = proc.apvts.getParameter (r->paramID))
                prm->setValueNotifyingHost (sep ? 1.0f : 0.0f);
    }

    void timerCallback() override { repaint(); }

    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    K54Processor& proc;
    juce::OwnedArray<Row> rows;
    juce::OwnedArray<BA> att;
    juce::TextButton allMain { "ALLE AUF MAIN" }, allSep { "ALLE SEPARAT" };
    juce::Rectangle<int> listArea, helpArea, titleArea;
};

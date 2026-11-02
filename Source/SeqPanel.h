#pragma once
#include "SeqWidgets.h"

// Senkrechter Tonhoehen-Balken je Schritt (-12 .. +24 Halbtoene), ziehen / Mausrad / Doppelklick = Grundton
class NoteBar : public juce::Component
{
public:
    std::function<void (int)> onChange;
    int value = 0, root = 36;
    bool active = false, playing = false, disabled = false;

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        auto r = getLocalBounds().toFloat().reduced (2.0f, 0.0f);
        g.setColour (black);
        g.fillRoundedRectangle (r, 3.0f);
        const float range = 36.0f;
        auto yFor = [&] (float v) { return r.getBottom() - (v + 12.0f) / range * r.getHeight(); };
        // Oktavlinien
        g.setColour (line);
        for (int o : { 0, 12 }) g.drawHorizontalLine ((int) yFor ((float) o), r.getX(), r.getRight());
        const float y0 = yFor (0.0f), y1 = yFor ((float) value);
        auto bar = juce::Rectangle<float> (r.getX() + 3.0f, juce::jmin (y0, y1) - 2.0f, r.getWidth() - 6.0f, std::abs (y1 - y0) + 4.0f);
        juce::Colour c = active ? gold : creamDim.withAlpha (0.3f);
        if (disabled) c = c.withAlpha (0.25f);
        g.setColour (c.withAlpha (active ? 0.35f : 0.2f));
        g.fillRoundedRectangle (bar, 2.0f);
        g.setColour (c);
        g.fillRoundedRectangle (juce::Rectangle<float> (r.getX() + 3.0f, y1 - 3.0f, r.getWidth() - 6.0f, 6.0f), 2.0f);
        g.setColour (playing ? cream : line);
        g.drawRoundedRectangle (r, 3.0f, playing ? 2.0f : 1.0f);
        // Notenname
        g.setColour (active ? cream : creamDim.withAlpha (0.6f));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.05f));
        g.drawText (juce::MidiMessage::getMidiNoteName (root + value, true, true, 3),
                    getLocalBounds().removeFromBottom (16), juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent& e) override { startVal = value; startY = e.position.y; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const float pxPerSemi = juce::jmax (3.0f, (float) getHeight() / 36.0f);
        set (startVal + juce::roundToInt ((startY - e.position.y) / pxPerSemi));
    }
    void mouseDoubleClick (const juce::MouseEvent&) override { set (0); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (std::abs (w.deltaY) > 0.0001f) set (value + (w.deltaY > 0 ? 1 : -1));
    }

private:
    void set (int v)
    {
        v = juce::jlimit (-12, 24, v);
        if (v != value) { value = v; repaint(); if (onChange) onChange (v); }
    }
    int startVal = 0;
    float startY = 0.0f;
};

// ---------------------------------------------------------------------------
//  Reiter SEQ: 16-Schritt-Sequencer fuer den Synth (Ton, Akzent, Slide)
// ---------------------------------------------------------------------------
class SeqPanel : public juce::Component, private juce::Timer
{
public:
    explicit SeqPanel (K54Processor& p) : proc (p), bar (p)
    {
        addAndMakeVisible (bar);
        bar.onPatternChange = [this] { syncSteps(); };

        setupToggle (seqOn, "Synth-Sequencer an/aus");
        addAndMakeVisible (seqOn);
        bAtt.add (new BA (proc.apvts, ParamID::seqSynthOn, seqOn));

        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (ParamID::seqSynthLen)))
            lenBox.addItemList (c->choices, 1);
        addAndMakeVisible (lenBox);
        cAtt.add (new CA (proc.apvts, ParamID::seqSynthLen, lenBox));

        for (auto* s : { &root, &gate }) { setupSmallKnob (*s); addAndMakeVisible (s); }
        sAtt.add (new SA (proc.apvts, ParamID::seqRoot, root));
        sAtt.add (new SA (proc.apvts, ParamID::seqGate, gate));

        for (int st = 0; st < StepSequencer::numSteps; ++st)
        {
            auto& c = cols[(size_t) st];
            c.on.beatStart = (st % 4) == 0;
            c.on.onClick = [this, st] { toggleFlag (st, StepSequencer::SynOn); };
            c.note.onChange = [this, st] (int v)
            {
                const int pat = bar.editPattern();
                proc.sequencer.setNote (pat, st, v);
                if (! (proc.sequencer.getFlags (pat, st) & StepSequencer::SynOn))   // Ton ziehen schaltet den Schritt ein
                    toggleFlag (st, StepSequencer::SynOn);
            };
            c.acc.onClick   = [this, st] { toggleFlag (st, StepSequencer::SynAccent); };
            c.slide.onClick = [this, st] { toggleFlag (st, StepSequencer::SynSlide); };
            addAndMakeVisible (c.on);
            addAndMakeVisible (c.note);
            addAndMakeVisible (c.acc);
            addAndMakeVisible (c.slide);
        }
        syncSteps();
    }

    void visibilityChanged() override
    {
        if (isVisible()) { syncSteps(); startTimerHz (30); }
        else stopTimer();
    }

    void resized() override
    {
        auto r = getLocalBounds();
        bar.setBounds (r.removeFromTop (30));
        {
            auto e = bar.getExtraArea();
            seqOn.setBounds (e.removeFromLeft (96).withSizeKeepingCentre (96, 30));
            e.removeFromLeft (6);
            lenBox.setBounds (e.removeFromLeft (96).withSizeKeepingCentre (96, 28));
            e.removeFromLeft (6);
            rootLabel = e.removeFromLeft (36);
            root.setBounds (e.removeFromLeft (32).withSizeKeepingCentre (30, 30));
            e.removeFromLeft (4);
            gateLabel = e.removeFromLeft (36);
            gate.setBounds (e.removeFromLeft (32).withSizeKeepingCentre (30, 30));
        }
        r.removeFromTop (10);
        panel = r;
        auto inner = r.reduced (12, 10);
        labels = inner.removeFromLeft (88);
        inner.removeFromLeft (8);
        header = inner.removeFromTop (20);
        const int colW = (inner.getWidth() - 3 * 8) / 16;
        auto cols0 = inner;
        for (int st = 0; st < StepSequencer::numSteps; ++st)
        {
            auto c = cols0.removeFromLeft (colW);
            if (st % 4 == 3) cols0.removeFromLeft (8);
            auto& cc = cols[(size_t) st];
            cc.on.setBounds (c.removeFromTop (30));
            c.removeFromTop (6);
            auto bottom = c.removeFromBottom (58);
            cc.note.setBounds (c);
            bottom.removeFromTop (6);
            cc.acc.setBounds (bottom.removeFromTop (24).reduced (2, 0));
            bottom.removeFromTop (4);
            cc.slide.setBounds (bottom.removeFromTop (24).reduced (2, 0));
        }
        rowOn    = labels.withY (cols[0].on.getY()).withHeight (cols[0].on.getHeight());
        rowNote  = labels.withY (cols[0].note.getY()).withHeight (cols[0].note.getHeight());
        rowAcc   = labels.withY (cols[0].acc.getY()).withHeight (cols[0].acc.getHeight());
        rowSlide = labels.withY (cols[0].slide.getY()).withHeight (cols[0].slide.getHeight());
    }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        g.setColour (K54Colours::panel);
        g.fillRoundedRectangle (panel.toFloat(), 8.0f);
        g.setColour (line);
        g.drawRoundedRectangle (panel.toFloat().reduced (0.5f), 8.0f, 1.0f);

        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.1f));
        g.setColour (creamDim);
        g.drawText ("ROOT", rootLabel, juce::Justification::centredRight);
        g.drawText ("GATE", gateLabel, juce::Justification::centredRight);

        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.0f, 0.25f));
        g.drawText ("SYNTH", labels.withY (header.getY()).withHeight (header.getHeight()), juce::Justification::centredLeft);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.12f));
        g.setColour (cream);
        g.drawText ("STEP", rowOn, juce::Justification::centredLeft);
        g.drawText ("NOTE", rowNote.removeFromTop (20), juce::Justification::centredLeft);
        g.setColour (creamDim.withAlpha (0.7f));
        g.setFont (juce::FontOptions (11.0f));
        g.drawFittedText ("ziehen = Ton\nDoppelklick = Grundton", rowNote.withTrimmedTop (24).withHeight (40), juce::Justification::topLeft, 3);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.12f));
        g.setColour (cream);
        g.drawText ("ACCENT", rowAcc, juce::Justification::centredLeft);
        g.drawText ("SLIDE", rowSlide, juce::Justification::centredLeft);

        for (int st = 0; st < StepSequencer::numSteps; ++st)
        {
            auto b = cols[(size_t) st].on.getBounds();
            g.setColour (st % 4 == 0 ? gold : creamDim.withAlpha (0.6f));
            g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.1f));
            g.drawText (juce::String (st + 1), b.withY (header.getY()).withHeight (header.getHeight()), juce::Justification::centred);
        }
    }

private:
    struct Col
    {
        StepButton on;
        NoteBar note;
        StepButton acc, slide;
    };

    void toggleFlag (int st, uint8_t flag)
    {
        const int pat = bar.editPattern();
        proc.sequencer.setFlags (pat, st, proc.sequencer.getFlags (pat, st) ^ flag);
        syncSteps();
    }

    void syncSteps()
    {
        const int pat = bar.editPattern();
        lastPat = pat;
        const int rootNote = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::seqRoot)->load());
        for (int st = 0; st < StepSequencer::numSteps; ++st)
        {
            auto& c = cols[(size_t) st];
            const auto f = proc.sequencer.getFlags (pat, st);
            c.on.state    = (f & StepSequencer::SynOn) ? 2 : 0;
            c.acc.state   = (f & StepSequencer::SynAccent) ? 2 : 0;
            c.slide.state = (f & StepSequencer::SynSlide) ? 1 : 0;
            c.note.value  = proc.sequencer.getNote (pat, st);
            c.note.root   = rootNote;
            c.note.active = (f & StepSequencer::SynOn) != 0;
            c.on.repaint(); c.acc.repaint(); c.slide.repaint(); c.note.repaint();
        }
        lastRoot = rootNote;
    }

    void timerCallback() override
    {
        bar.refresh();
        const int rootNote = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::seqRoot)->load());
        if (bar.editPattern() != lastPat || rootNote != lastRoot) syncSteps();
        const int cur = proc.sequencer.currentSynthStep();
        const int len = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::seqSynthLen)->load()) + 1;
        if (cur != lastStep || len != lastLen)
        {
            lastStep = cur; lastLen = len;
            for (int st = 0; st < StepSequencer::numSteps; ++st)
            {
                auto& c = cols[(size_t) st];
                const bool pl = st == cur, dis = st >= len;
                c.on.playing = pl; c.note.playing = pl;
                c.on.disabled = dis; c.note.disabled = dis; c.acc.disabled = dis; c.slide.disabled = dis;
                c.on.repaint(); c.note.repaint(); c.acc.repaint(); c.slide.repaint();
            }
        }
    }

    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    K54Processor& proc;
    SeqTransportBar bar;
    juce::TextButton seqOn { "SEQ ON" };
    juce::ComboBox lenBox;
    juce::Slider root, gate;
    std::array<Col, StepSequencer::numSteps> cols;
    juce::OwnedArray<BA> bAtt;
    juce::OwnedArray<SA> sAtt;
    juce::OwnedArray<CA> cAtt;
    juce::Rectangle<int> panel, labels, header, rowOn, rowNote, rowAcc, rowSlide, rootLabel, gateLabel;
    int lastStep = -2, lastLen = -1, lastPat = -1, lastRoot = -1;
};

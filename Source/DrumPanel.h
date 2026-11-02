#pragma once
#include "SeqWidgets.h"

// ---------------------------------------------------------------------------
//  Reiter DRUMS: 8 Spuren x 16 Schritte + Klangregler je Stimme
// ---------------------------------------------------------------------------
class DrumPanel : public juce::Component, private juce::Timer
{
public:
    explicit DrumPanel (K54Processor& p) : proc (p), bar (p)
    {
        addAndMakeVisible (bar);
        bar.onPatternChange = [this] { syncSteps(); };

        setupToggle (drumsOn, "Drum-Sequencer an/aus");
        addAndMakeVisible (drumsOn);
        att.add (new BA (proc.apvts, ParamID::seqDrumsOn, drumsOn));

        for (auto* s : { &swing, &volume }) { setupSmallKnob (*s); addAndMakeVisible (s); }
        sAtt.add (new SA (proc.apvts, ParamID::seqSwing, swing));
        sAtt.add (new SA (proc.apvts, ParamID::drumMaster, volume));
        swing.setDoubleClickReturnValue (true, 0.0);

        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (ParamID::seqDrumLen)))
            lenBox.addItemList (c->choices, 1);
        addAndMakeVisible (lenBox);
        cAtt.add (new CA (proc.apvts, ParamID::seqDrumLen, lenBox));

        static const char* knobNames[5] { "Level", "Tune", "Decay", "Tone", "Pan" };
        for (int l = 0; l < DrumInfo::numLanes; ++l)
        {
            auto& row = rows[(size_t) l];
            setupToggle (row.mute, "Stumm");
            row.mute.setButtonText ("M");
            setupToggle (row.out, "Eigener Ausgang in der DAW");
            row.out.setButtonText ("OUT");
            addAndMakeVisible (row.mute);
            addAndMakeVisible (row.out);
            att.add (new BA (proc.apvts, ParamID::drum (l, "Mute"), row.mute));
            att.add (new BA (proc.apvts, ParamID::drum (l, "Out"), row.out));

            for (int st = 0; st < StepSequencer::numSteps; ++st)
            {
                auto& b = row.steps[(size_t) st];
                b.beatStart = (st % 4) == 0;
                b.onClick = [this, l, st]
                {
                    const int pat = bar.editPattern();
                    const int v = (proc.sequencer.getDrum (pat, l, st) + 1) % 3;   // aus -> an -> Akzent -> aus
                    proc.sequencer.setDrum (pat, l, st, (uint8_t) v);
                    syncSteps();
                };
                addAndMakeVisible (b);
            }
            for (int k = 0; k < 5; ++k)
            {
                auto& s = row.knobs[(size_t) k];
                setupSmallKnob (s);
                if (k == 1 || k == 4) s.getProperties().set ("bipolar", true);
                addAndMakeVisible (s);
                sAtt.add (new SA (proc.apvts, ParamID::drum (l, knobNames[k]), s));
            }
            row.knobs[1].setDoubleClickReturnValue (true, 0.0);
            row.knobs[4].setDoubleClickReturnValue (true, 0.0);
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
            drumsOn.setBounds (e.removeFromLeft (86).withSizeKeepingCentre (86, 30));
            e.removeFromLeft (6);
            lenBox.setBounds (e.removeFromLeft (96).withSizeKeepingCentre (96, 28));
            e.removeFromLeft (6);
            swingLabel = e.removeFromLeft (40);
            swing.setBounds (e.removeFromLeft (32).withSizeKeepingCentre (30, 30));
            e.removeFromLeft (4);
            volLabel = e.removeFromLeft (48);
            volume.setBounds (e.removeFromLeft (32).withSizeKeepingCentre (30, 30));
        }
        r.removeFromTop (10);
        panel = r;
        auto inner = r.reduced (12, 8);
        header = inner.removeFromTop (22);
        const int rowH = inner.getHeight() / DrumInfo::numLanes;
        for (int l = 0; l < DrumInfo::numLanes; ++l)
        {
            auto row = inner.removeFromTop (rowH).reduced (0, 3);
            auto& rr = rows[(size_t) l];
            rr.nameArea = row.removeFromLeft (84);
            rr.mute.setBounds (row.removeFromLeft (28));
            row.removeFromLeft (4);
            rr.out.setBounds (row.removeFromLeft (42));
            row.removeFromLeft (12);
            auto stepsArea = row.removeFromLeft (16 * 32 + 3 * 6);
            for (int st = 0; st < StepSequencer::numSteps; ++st)
            {
                rr.steps[(size_t) st].setBounds (stepsArea.removeFromLeft (32));
                if (st % 4 == 3) stepsArea.removeFromLeft (6);
            }
            row.removeFromLeft (12);
            for (int k = 0; k < 5; ++k)
                rr.knobs[(size_t) k].setBounds (row.removeFromLeft (row.getWidth() / (5 - k)).withSizeKeepingCentre (34, 34));
        }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        g.setColour (panel.isEmpty() ? juce::Colours::transparentBlack : K54Colours::panel);
        g.fillRoundedRectangle (panel.toFloat(), 8.0f);
        g.setColour (line);
        g.drawRoundedRectangle (panel.toFloat().reduced (0.5f), 8.0f, 1.0f);

        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.1f));
        g.setColour (creamDim);
        g.drawText ("SWING", swingLabel, juce::Justification::centredRight);
        g.drawText ("VOLUME", volLabel, juce::Justification::centredRight);

        // Kopf: Schrittnummern und Reglernamen
        const auto& r0 = rows[0];
        for (int st = 0; st < StepSequencer::numSteps; ++st)
        {
            auto b = r0.steps[(size_t) st].getBounds();
            g.setColour (st % 4 == 0 ? gold : creamDim.withAlpha (0.6f));
            g.drawText (juce::String (st + 1), b.withY (header.getY()).withHeight (header.getHeight()), juce::Justification::centred);
        }
        static const char* kn[5] { "LEVEL", "TUNE", "DECAY", "TONE", "PAN" };
        for (int k = 0; k < 5; ++k)
        {
            auto b = r0.knobs[(size_t) k].getBounds();
            g.setColour (creamDim);
            g.drawText (kn[k], b.withY (header.getY()).withHeight (header.getHeight()).expanded (10, 0), juce::Justification::centred);
        }
        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.0f, 0.25f));
        g.drawText ("DRUMS", r0.nameArea.withY (header.getY()).withHeight (header.getHeight()), juce::Justification::centredLeft);

        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 9.5f, 0.12f));
        for (int l = 0; l < DrumInfo::numLanes; ++l)
        {
            g.setColour (cream);
            g.drawText (DrumInfo::shortNames[l], rows[(size_t) l].nameArea, juce::Justification::centredLeft);
        }
    }

private:
    void timerCallback() override
    {
        bar.refresh();
        const int cur = proc.sequencer.currentDrumStep();
        const int len = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::seqDrumLen)->load()) + 1;
        if (cur != lastStep || len != lastLen || bar.editPattern() != lastPat)
        {
            lastStep = cur; lastLen = len;
            if (bar.editPattern() != lastPat) syncSteps();
            for (auto& rr : rows)
                for (int st = 0; st < StepSequencer::numSteps; ++st)
                {
                    auto& b = rr.steps[(size_t) st];
                    const bool pl = st == cur, dis = st >= len;
                    if (pl != b.playing || dis != b.disabled) { b.playing = pl; b.disabled = dis; b.repaint(); }
                }
        }
    }

    void syncSteps()
    {
        const int pat = bar.editPattern();
        lastPat = pat;
        for (int l = 0; l < DrumInfo::numLanes; ++l)
            for (int st = 0; st < StepSequencer::numSteps; ++st)
            {
                auto& b = rows[(size_t) l].steps[(size_t) st];
                const int v = proc.sequencer.getDrum (pat, l, st);
                if (v != b.state) { b.state = v; b.repaint(); }
            }
    }

    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Row
    {
        juce::TextButton mute, out;
        std::array<StepButton, StepSequencer::numSteps> steps;
        std::array<juce::Slider, 5> knobs;
        juce::Rectangle<int> nameArea;
    };

    K54Processor& proc;
    SeqTransportBar bar;
    juce::TextButton drumsOn { "DRUMS ON" };
    juce::Slider swing, volume;
    juce::ComboBox lenBox;
    std::array<Row, DrumInfo::numLanes> rows;
    juce::OwnedArray<BA> att;
    juce::OwnedArray<SA> sAtt;
    juce::OwnedArray<CA> cAtt;
    juce::Rectangle<int> panel, header, swingLabel, volLabel;
    int lastStep = -2, lastLen = -1, lastPat = -1;
};

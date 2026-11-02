#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

namespace LoopColours
{
    inline const juce::Colour rec     { 0xffE0554B };
    inline const juce::Colour dub     { 0xffE8913A };
}

// ---------------------------------------------------------------------------
//  Eine Looper-Spur: Ring mit Fortschritt, Haupttaste, Stop, Clear, Lautstaerke
class LoopTrackStrip : public juce::Component
{
public:
    LoopTrackStrip (K54Processor& p, int index) : proc (p), idx (index)
    {
        for (auto* b : { &mainButton, &stopButton, &clearButton })
            addAndMakeVisible (b);
        mainButton.onClick  = [this] { proc.looper.pressMain (idx); };
        stopButton.onClick  = [this] { proc.looper.pressStop (idx); };
        clearButton.onClick = [this] { proc.looper.pressClear (idx); };

        vol.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        vol.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        vol.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        vol.setColour (juce::Slider::textBoxTextColourId, K54Colours::creamDim);
        vol.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 15);
        vol.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
        vol.setDoubleClickReturnValue (true, 1.0);
        addAndMakeVisible (vol);
        const char* ids[] { ParamID::loop1Vol, ParamID::loop2Vol, ParamID::loop3Vol, ParamID::loop4Vol };
        volAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ids[idx], vol);

        // Aufnahmequelle: Synth, Audio-Eingang oder beides
        sourceBox.addItemList ({ "SYNTH", "INPUT", "SYNTH + IN" }, 1);
        sourceBox.setJustificationType (juce::Justification::centred);
        sourceBox.setTooltip ("Was diese Spur aufnimmt");
        addAndMakeVisible (sourceBox);
        const char* src[] { ParamID::loop1Src, ParamID::loop2Src, ParamID::loop3Src, ParamID::loop4Src };
        srcAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, src[idx], sourceBox);
    }

    void refresh()
    {
        const auto st = proc.looper.getState (idx);
        if (st != lastState)
        {
            lastState = st;
            static const char* mainText[] { "REC", "CANCEL", "REC ...", "OVERDUB", "END DUB", "PLAY" };
            mainButton.setButtonText (mainText[(int) st]);
            mainButton.setEnabled (st != LoopState::Recording);
            stopButton.setButtonText (st == LoopState::Stopped ? "START" : "STOP");
            stopButton.setEnabled (st != LoopState::Empty);
            clearButton.setEnabled (st != LoopState::Empty);
        }
        ++blink;
        repaint (ringArea);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        r.removeFromTop (24);
        sourceBox.setBounds (r.removeFromTop (24));
        r.removeFromTop (4);
        ringArea = r.removeFromTop (118).withSizeKeepingCentre (118, 118);
        r.removeFromTop (8);
        mainButton.setBounds (r.removeFromTop (34));
        r.removeFromTop (6);
        auto row = r.removeFromTop (26);
        stopButton.setBounds (row.removeFromLeft (row.getWidth() / 2 - 3));
        row.removeFromLeft (6);
        clearButton.setBounds (row);
        r.removeFromTop (10);
        volLabelArea = r.removeFromTop (14);
        vol.setBounds (r.removeFromTop (70).withSizeKeepingCentre (80, 70));
    }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        auto b = getLocalBounds().toFloat();
        g.setColour (panel);
        g.fillRoundedRectangle (b, 8.0f);
        g.setColour (line);
        g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);

        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.5f, 0.3f));
        g.drawText ("LOOP " + juce::String (idx + 1), getLocalBounds().reduced (12, 8).removeFromTop (16), juce::Justification::centredLeft);

        g.setColour (creamDim);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.08f));
        g.drawText ("VOLUME", volLabelArea, juce::Justification::centred);

        // Ring
        const auto st = proc.looper.getState (idx);
        const float prog = proc.looper.getProgress (idx);
        auto ring = ringArea.toFloat().reduced (6.0f);
        const float cx = ring.getCentreX(), cy = ring.getCentreY(), rad = ring.getWidth() * 0.5f;
        const float lw = 7.0f;

        juce::Colour col = line;
        juce::String stateText = "EMPTY";
        switch (st)
        {
            case LoopState::Empty:     col = line; stateText = "EMPTY"; break;
            case LoopState::Armed:     col = LoopColours::rec.withAlpha ((blink / 8) % 2 ? 1.0f : 0.35f); stateText = "ARMED"; break;
            case LoopState::Recording: col = LoopColours::rec; stateText = "REC"; break;
            case LoopState::Playing:   col = gold; stateText = "PLAY"; break;
            case LoopState::Overdub:   col = LoopColours::dub; stateText = "DUB"; break;
            case LoopState::Stopped:   col = creamDim.withAlpha (0.5f); stateText = "STOP"; break;
        }

        juce::Path track;
        track.addCentredArc (cx, cy, rad - lw * 0.5f, rad - lw * 0.5f, 0.0f, 0.0f, juce::MathConstants<float>::twoPi, true);
        g.setColour (line);
        g.strokePath (track, juce::PathStrokeType (lw));

        if (st != LoopState::Empty && st != LoopState::Armed)
        {
            juce::Path arc;
            arc.addCentredArc (cx, cy, rad - lw * 0.5f, rad - lw * 0.5f, 0.0f, 0.0f,
                               juce::jmax (0.01f, prog) * juce::MathConstants<float>::twoPi, true);
            g.setColour (col);
            g.strokePath (arc, { lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
        }
        else if (st == LoopState::Armed)
        {
            g.setColour (col);
            g.strokePath (track, juce::PathStrokeType (lw));
        }

        // Pegel als innerer Schein
        const float lvl = juce::jlimit (0.0f, 1.0f, proc.looper.getLevel (idx) * 1.4f);
        const float ir = rad - lw - 8.0f;
        g.setColour (col.withAlpha (0.08f + 0.25f * lvl));
        g.fillEllipse (cx - ir, cy - ir, ir * 2.0f, ir * 2.0f);

        g.setColour (st == LoopState::Empty ? creamDim.withAlpha (0.6f) : cream);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 13.0f, 0.12f));
        g.drawText (stateText, ring.withTrimmedBottom (18), juce::Justification::centred);

        const int bars = proc.looper.getLengthBars (idx);
        if (bars > 0)
        {
            g.setColour (creamDim);
            g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.2f));
            g.drawText (juce::String (bars) + (bars == 1 ? " BAR" : " BARS"), ring.withTrimmedTop (30), juce::Justification::centred);
        }
    }

private:
    K54Processor& proc;
    const int idx;
    juce::TextButton mainButton { "REC" }, stopButton { "STOP" }, clearButton { "CLEAR" };
    juce::Slider vol;
    juce::ComboBox sourceBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> srcAttach;
    juce::Rectangle<int> ringArea, volLabelArea;
    LoopState lastState { (LoopState) -1 };
    int blink = 0;
};

// ---------------------------------------------------------------------------
//  Gesamter Looper-Reiter: 4 Spuren + Sync-Bereich
class LooperPanel : public juce::Component, private juce::Timer
{
public:
    explicit LooperPanel (K54Processor& p) : proc (p)
    {
        for (int i = 0; i < Looper::numTracks; ++i)
        {
            strips[(size_t) i] = std::make_unique<LoopTrackStrip> (p, i);
            addAndMakeVisible (*strips[(size_t) i]);
        }

        lengthBox.addItemList ({ "1 BAR", "2 BARS", "4 BARS", "8 BARS" }, 1);
        addAndMakeVisible (lengthBox);
        lengthAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, ParamID::loopBars, lengthBox);

        bpm.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        bpm.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        bpm.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        bpm.setColour (juce::Slider::textBoxTextColourId, K54Colours::creamDim);
        bpm.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 15);
        bpm.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
        bpm.setMouseDragSensitivity (400);
        bpm.setDoubleClickReturnValue (true, 120.0);
        addAndMakeVisible (bpm);
        bpmAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ParamID::loopBpm, bpm);

        clickButton.setClickingTogglesState (true);
        addAndMakeVisible (clickButton);
        clickAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ParamID::loopClick, clickButton);

        for (auto* b : { &transportButton, &stopAllButton, &clearAllButton })
            addAndMakeVisible (b);
        transportButton.onClick = [this] { proc.looper.pressTransport(); };
        stopAllButton.onClick   = [this] { proc.looper.stopAll(); };
        clearAllButton.onClick  = [this] { proc.looper.clearAll(); };

        // ---- Audio-Eingang
        inModeBox.addItemList ({ "STEREO 1/2", "MONO IN 1", "MONO IN 2", "MONO 1+2" }, 1);
        addAndMakeVisible (inModeBox);
        inModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, ParamID::loopInMode, inModeBox);

        inGain.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        inGain.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        inGain.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        inGain.setColour (juce::Slider::textBoxTextColourId, K54Colours::creamDim);
        inGain.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 15);
        inGain.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
        inGain.getProperties().set ("bipolar", true);
        inGain.setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (inGain);
        inGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ParamID::loopInGain, inGain);

        monitorButton.setClickingTogglesState (true);
        monitorButton.setTooltip ("Eingang live mithoeren");
        addAndMakeVisible (monitorButton);
        monitorAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ParamID::loopMonitor, monitorButton);

        startTimerHz (30);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        auto side = r.removeFromRight (190);
        r.removeFromRight (10);
        inputArea = r.removeFromRight (124);
        r.removeFromRight (10);
        {
            auto s = inputArea.reduced (10);
            s.removeFromTop (24);
            meterArea = s.removeFromTop (112);
            s.removeFromTop (8);
            inModeLabel = s.removeFromTop (14);
            inModeBox.setBounds (s.removeFromTop (26));
            s.removeFromTop (8);
            inGainLabel = s.removeFromTop (14);
            inGain.setBounds (s.removeFromTop (70).withSizeKeepingCentre (84, 70));
            s.removeFromTop (8);
            monitorButton.setBounds (s.removeFromTop (28));
        }
        const int w = (r.getWidth() - 3 * 10) / 4;
        for (int i = 0; i < 4; ++i)
        {
            strips[(size_t) i]->setBounds (r.removeFromLeft (w));
            r.removeFromLeft (10);
        }

        syncArea = side;
        auto s = side.reduced (12);
        s.removeFromTop (24);
        infoArea = s.removeFromTop (92);
        s.removeFromTop (6);
        lengthLabel = s.removeFromTop (14);
        lengthBox.setBounds (s.removeFromTop (26));
        s.removeFromTop (10);
        tempoLabel = s.removeFromTop (14);
        auto row = s.removeFromTop (70);
        bpm.setBounds (row.removeFromLeft (row.getWidth() / 2 + 6));
        row.removeFromLeft (6);
        clickButton.setBounds (row.withSizeKeepingCentre (row.getWidth(), 26).translated (0, -6));
        s.removeFromTop (8);
        transportButton.setBounds (s.removeFromTop (30));
        s.removeFromTop (6);
        auto row2 = s.removeFromTop (26);
        stopAllButton.setBounds (row2.removeFromLeft (row2.getWidth() / 2 - 3));
        row2.removeFromLeft (6);
        clearAllButton.setBounds (row2);
    }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        paintInput (g);
        auto b = syncArea.toFloat();
        g.setColour (panel);
        g.fillRoundedRectangle (b, 8.0f);
        g.setColour (line);
        g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);

        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.5f, 0.3f));
        g.drawText ("SYNC", syncArea.reduced (12, 8).removeFromTop (16), juce::Justification::centredLeft);

        // Quelle
        const auto src = proc.looper.getSource();
        const bool running = proc.looper.isRunning();
        const char* srcText = src == ClockSource::Host ? "DAW / HOST" : (src == ClockSource::MidiClock ? "MIDI CLOCK" : "INTERN");
        auto info = infoArea;
        auto top = info.removeFromTop (16);
        g.setColour (running ? gold : creamDim.withAlpha (0.5f));
        g.fillEllipse (top.removeFromLeft (10).withSizeKeepingCentre (8, 8).toFloat());
        top.removeFromLeft (6);
        g.setColour (cream);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.5f, 0.15f));
        g.drawText (srcText, top, juce::Justification::centredLeft);

        // BPM
        g.setColour (cream);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 30.0f, 0.04f));
        auto bpmRow = info.removeFromTop (40);
        g.drawText (juce::String (proc.looper.getBpm(), 1), bpmRow.withTrimmedRight (40), juce::Justification::bottomLeft);
        g.setColour (creamDim);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.2f));
        g.drawText ("BPM", bpmRow, juce::Justification::bottomRight);

        // Takt.Schlag + Schlag-Punkte
        const double beat = juce::jmax (0.0, proc.looper.getBeat());
        const int bpb = proc.looper.getBeatsPerBar();
        const int bar = (int) std::floor (beat / bpb) + 1;
        const int beatInBar = (int) std::floor (std::fmod (beat, (double) bpb));
        auto beatRow = info.removeFromTop (30).withTrimmedTop (8);
        g.setColour (creamDim);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 10.0f, 0.1f));
        g.drawText ("BAR " + juce::String (running ? bar : 1), beatRow.removeFromLeft (70), juce::Justification::centredLeft);
        const int dots = juce::jmin (bpb, 8);
        const float dw = (float) beatRow.getWidth() / (float) dots;
        for (int i = 0; i < dots; ++i)
        {
            auto d = juce::Rectangle<float> (beatRow.getX() + i * dw, (float) beatRow.getY(), dw, (float) beatRow.getHeight())
                         .withSizeKeepingCentre (9.0f, 9.0f);
            const bool on = running && i == beatInBar;
            g.setColour (on ? (i == 0 ? LoopColours::rec : gold) : line);
            g.fillEllipse (d);
        }

        g.setColour (creamDim);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.08f));
        g.drawText ("LOOP LENGTH", lengthLabel, juce::Justification::centredLeft);
        const bool daw = proc.looper.usesHostTempo();
        g.drawText (daw ? "TEMPO" : "TEMPO (INTERN)", tempoLabel, juce::Justification::centredLeft);
        if (daw)
        {
            auto t = bpm.getBounds().withWidth (tempoLabel.getWidth() - 70);
            g.setColour (gold);
            g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.0f, 0.15f));
            g.drawFittedText ("= DAW", t.removeFromTop (t.getHeight() / 2 + 6), juce::Justification::bottomLeft, 1);
            g.setColour (creamDim);
            g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.08f));
            g.drawFittedText ("FOLGT DEM SONG", t, juce::Justification::topLeft, 1);
        }
    }

    void visibilityChanged() override { if (isVisible()) updateAll(); }

    void paintInput (juce::Graphics& g)
    {
        using namespace K54Colours;
        auto b = inputArea.toFloat();
        g.setColour (panel);
        g.fillRoundedRectangle (b, 8.0f);
        g.setColour (line);
        g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);

        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.5f, 0.3f));
        g.drawText ("INPUT", inputArea.reduced (12, 8).removeFromTop (16), juce::Justification::centredLeft);

        // Pegelanzeige L / R (logarithmisch, -60 .. 0 dB)
        const bool has = proc.hasAudioInput();
        auto m = meterArea;
        auto status = m.removeFromBottom (16);
        const int bw = 16, gap = 10;
        auto bars = m.withSizeKeepingCentre (bw * 2 + gap, m.getHeight());
        for (int ch = 0; ch < 2; ++ch)
        {
            auto bar = juce::Rectangle<int> (bars.getX() + ch * (bw + gap), bars.getY(), bw, bars.getHeight() - 14).toFloat();
            g.setColour (black);
            g.fillRoundedRectangle (bar, 3.0f);
            const float db = juce::Decibels::gainToDecibels (proc.inPeak[ch].load(), -60.0f);
            const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            auto fill = bar.reduced (2.0f).withTrimmedTop ((1.0f - frac) * (bar.getHeight() - 4.0f));
            g.setColour (db > -3.0f ? LoopColours::rec : (db > -12.0f ? LoopColours::dub : gold));
            g.fillRoundedRectangle (fill, 2.0f);
            g.setColour (creamDim);
            g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.1f));
            g.drawText (ch == 0 ? "L" : "R", juce::Rectangle<int> ((int) bar.getX(), (int) bar.getBottom() + 1, bw, 12), juce::Justification::centred);
        }
        g.setColour (has ? creamDim.withAlpha (0.8f) : LoopColours::rec);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.12f));
        g.drawText (has ? "AUDIO IN AKTIV" : "KEIN EINGANG", status, juce::Justification::centred);

        g.setColour (creamDim);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.08f));
        g.drawText ("CHANNELS", inModeLabel, juce::Justification::centredLeft);
        g.drawText ("GAIN", inGainLabel, juce::Justification::centred);
    }

private:
    void timerCallback() override
    {
        if (! isShowing()) return;
        updateAll();
    }

    void updateAll()
    {
        for (auto& s : strips) s->refresh();

        const auto src = proc.looper.getSource();
        const bool internal = src == ClockSource::Internal;
        transportButton.setEnabled (internal);
        transportButton.setButtonText (! internal ? "SYNC EXTERN" : (proc.looper.isRunning() ? "STOP CLOCK" : "START CLOCK"));
        const bool daw = proc.looper.usesHostTempo();
        bpm.setEnabled (internal && ! daw);
        bpm.setVisible (! daw);
        if (daw != lastDawTempo) { lastDawTempo = daw; repaint(); }
        repaint (syncArea);
        repaint (meterArea);
    }

    K54Processor& proc;
    std::array<std::unique_ptr<LoopTrackStrip>, Looper::numTracks> strips;
    juce::ComboBox lengthBox;
    juce::Slider bpm;
    juce::TextButton clickButton { "CLICK" }, transportButton { "START CLOCK" },
                     stopAllButton { "STOP ALL" }, clearAllButton { "CLEAR ALL" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lengthAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> clickAttach;
    juce::ComboBox inModeBox;
    juce::Slider inGain;
    juce::TextButton monitorButton { "MONITOR" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> inModeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monitorAttach;
    juce::Rectangle<int> syncArea, infoArea, lengthLabel, tempoLabel, inputArea, meterArea, inModeLabel, inGainLabel;
    bool lastDawTempo = false;
};

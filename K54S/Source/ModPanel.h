#pragma once
#include "PanelKit.h"

// ---------------------------------------------------------------------------
//  Reiter MOD: LFO 2, Mod-Huellkurve, Matrix mit 8 Plaetzen, Wheels & Spielhilfen
// ---------------------------------------------------------------------------
class ModPanel : public KitPanel
{
public:
    explicit ModPanel (K55Processor& p) : KitPanel (p)
    {
        lfo2Rate = &knob (ParamID::lfo2Rate, "Rate");
        lfo2Wave = &choice (ParamID::lfo2Wave, "Wave");
        mA = &knob (ParamID::modAttack, "Attack");
        mD = &knob (ParamID::modDecay, "Decay");
        mS = &knob (ParamID::modSustain, "Sustain");
        mR = &knob (ParamID::modRelease, "Release");
        for (int k = 0; k < ParamID::numModSlots; ++k)
        {
            src[k] = &choice (ParamID::modSrc (k), {});
            dst[k] = &choice (ParamID::modDst (k), {});
            amt[k] = &bar (ParamID::modAmt (k));
            src[k]->b.onChange = [this] { repaint(); };
            dst[k]->b.onChange = [this] { repaint(); };
        }
        bendUp   = &knob (ParamID::bendRange, "Bend Up");
        bendDn   = &knob (ParamID::bendDown, "Bend Down");
        mwVib    = &knob (ParamID::mwVibrato, "Vibrato");
        mwCut    = &knob (ParamID::mwCutoff, "Cutoff", true);
        mwPos    = &knob (ParamID::mwPos, "WT Pos", true);
        atVib    = &knob (ParamID::atVibrato, "Vibrato");
        atCut    = &knob (ParamID::atCutoff, "Cutoff", true);
        for (auto* k : { mwVib, mwCut, mwPos, atVib, atCut }) k->s.getProperties().set ("accent", true);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        secLeft = r.removeFromLeft (296);   r.removeFromLeft (12);
        secMatrix = r.removeFromLeft (520); r.removeFromLeft (12);
        secWheels = r;
        const int cw = 66;
        {
            auto a = secLeft.reduced (10).withTrimmedTop (24);
            subLfo2 = a.removeFromTop (16);
            auto row = a.removeFromTop (cellH);
            place (*lfo2Rate, row.removeFromLeft (cw));
            placeTight (*lfo2Wave, row.removeFromLeft (110).withTrimmedTop (14));
            a.removeFromTop (8);
            subEnv = a.removeFromTop (16);
            row = a.removeFromTop (cellH);
            const int w4 = row.getWidth() / 4;
            place (*mA, row.removeFromLeft (w4));
            place (*mD, row.removeFromLeft (w4));
            place (*mS, row.removeFromLeft (w4));
            place (*mR, row);
            hintLeft = a.withTrimmedTop (10);
        }
        {
            auto a = secMatrix.reduced (12).withTrimmedTop (24);
            head = a.removeFromTop (16);
            for (int k = 0; k < ParamID::numModSlots; ++k)
            {
                auto row = a.removeFromTop (38).reduced (0, 4);
                rowNum[k] = row.removeFromLeft (22);
                src[k]->b.setBounds (row.removeFromLeft (156));
                row.removeFromLeft (26);
                dst[k]->b.setBounds (row.removeFromLeft (156));
                row.removeFromLeft (12);
                amt[k]->s.setBounds (row);
                src[k]->l.setVisible (false);
                dst[k]->l.setVisible (false);
            }
        }
        {
            auto a = secWheels.reduced (10).withTrimmedTop (24);
            subBend = a.removeFromTop (16);
            auto row = a.removeFromTop (cellH);
            const int w3 = row.getWidth() / 3;
            place (*bendUp, row.removeFromLeft (w3));
            place (*bendDn, row.removeFromLeft (w3));
            a.removeFromTop (4);
            subMw = a.removeFromTop (16);
            row = a.removeFromTop (cellH);
            place (*mwVib, row.removeFromLeft (w3));
            place (*mwCut, row.removeFromLeft (w3));
            place (*mwPos, row);
            a.removeFromTop (4);
            subAt = a.removeFromTop (16);
            row = a.removeFromTop (cellH);
            place (*atVib, row.removeFromLeft (w3));
            place (*atCut, row.removeFromLeft (w3));
        }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        drawSection (g, secLeft, "LFO 2 & MOD ENV");
        drawSection (g, secMatrix, "MOD MATRIX");
        drawSection (g, secWheels, "WHEELS & TOUCH");
        drawSubtitle (g, subLfo2, "LFO 2");
        drawSubtitle (g, subEnv, "MOD ENV (FREI ZUWEISBAR)");
        drawSubtitle (g, subBend, "PITCH WHEEL");
        drawSubtitle (g, subMw, "MOD WHEEL  >");
        drawSubtitle (g, subAt, "AFTERTOUCH  >");

        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.25f));
        g.setColour (creamDim.withAlpha (0.6f));
        auto h = head;
        h.removeFromLeft (22);
        g.drawText ("QUELLE", h.removeFromLeft (156), juce::Justification::centredLeft);
        h.removeFromLeft (26);
        g.drawText ("ZIEL", h.removeFromLeft (156), juce::Justification::centredLeft);
        h.removeFromLeft (12);
        g.drawText ("MENGE", h, juce::Justification::centred);
        for (int k = 0; k < ParamID::numModSlots; ++k)
        {
            const bool used = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::modSrc (k))->load()) != 0
                           && juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::modDst (k))->load()) != 0;
            g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.0f, 0.1f));
            g.setColour (used ? accent2 : creamDim.withAlpha (0.4f));
            g.drawText (juce::String (k + 1), rowNum[k], juce::Justification::centredLeft);
            auto arrow = rowNum[k].withX (src[k]->b.getRight()).withWidth (26);
            g.drawText (ParamChoices::u8 ("\xe2\x86\x92"), arrow, juce::Justification::centred);
        }
        drawHint (g, hintLeft, ParamChoices::u8 ("MODULAR: JEDE QUELLE (LFOS, H\xc3\x9cLLKURVEN, WHEELS, AFTERTOUCH, BREATH, EXPRESSION, VELOCITY) "
                                                  "AUF JEDES ZIEL. DOPPELKLICK SETZT DIE MENGE AUF 0."));
    }

private:
    Knob *lfo2Rate = nullptr, *mA = nullptr, *mD = nullptr, *mS = nullptr, *mR = nullptr;
    Choice* lfo2Wave = nullptr;
    Choice* src[ParamID::numModSlots] {};
    Choice* dst[ParamID::numModSlots] {};
    Knob* amt[ParamID::numModSlots] {};
    Knob *bendUp = nullptr, *bendDn = nullptr, *mwVib = nullptr, *mwCut = nullptr, *mwPos = nullptr, *atVib = nullptr, *atCut = nullptr;
    juce::Rectangle<int> secLeft, secMatrix, secWheels, subLfo2, subEnv, subBend, subMw, subAt, head, hintLeft;
    juce::Rectangle<int> rowNum[ParamID::numModSlots];
};

// ---------------------------------------------------------------------------
//  Reiter FX: Unison & Power, Qualitaet (CPU), Chorus, Delay, Reverb
// ---------------------------------------------------------------------------
class FxPanel : public KitPanel
{
public:
    explicit FxPanel (K55Processor& p) : KitPanel (p)
    {
        uni     = &choice (ParamID::uniVoices, "Unison");
        detune  = &knob (ParamID::uniDetune, "Detune");
        spread  = &knob (ParamID::uniSpread, "Spread");
        sub     = &knob (ParamID::subLevel, "Sub Osc");
        ftype   = &choice (ParamID::fltType, "Filter");
        heat    = &knob (ParamID::heat, "Heat");
        quality = &choice (ParamID::quality, ParamChoices::u8 ("QUALIT\xc3\x84T"));
        chMix   = &knob (ParamID::chorusMix, "Mix");
        chRate  = &knob (ParamID::chorusRate, "Rate");
        chDepth = &knob (ParamID::chorusDepth, "Depth");
        dlMix   = &knob (ParamID::delayMix, "Mix");
        dlTime  = &choice (ParamID::delayTime, "Time");
        dlFb    = &knob (ParamID::delayFb, "Feedback");
        rvSend  = &knob (ParamID::revSend, "Send");
        rvSize  = &knob (ParamID::revSize, "Size");
        rvDamp  = &knob (ParamID::revDamp, "Damp");
        rvLevel = &knob (ParamID::revLevel, "Level");
    }

    void resized() override
    {
        auto r = getLocalBounds();
        secUni = r.removeFromLeft (340); r.removeFromLeft (12);
        secFx  = r;
        const int cw = 70;
        {
            auto a = secUni.reduced (10).withTrimmedTop (24);
            auto row = a.removeFromTop (cellH);
            place (*uni,    row.removeFromLeft (100));
            place (*detune, row.removeFromLeft (cw));
            place (*spread, row.removeFromLeft (cw));
            a.removeFromTop (8);
            row = a.removeFromTop (cellH);
            place (*sub,   row.removeFromLeft (100).withSizeKeepingCentre (cw, cellH));
            place (*ftype, row.removeFromLeft (cw + 30));
            place (*heat,  row.removeFromLeft (cw));
            a.removeFromTop (8);
            subCpu = a.removeFromTop (16);
            row = a.removeFromTop (cellH);
            place (*quality, row.removeFromLeft (120));
            hintCpu = row.reduced (10, 14);
        }
        {
            auto a = secFx.reduced (10).withTrimmedTop (24);
            const int w4 = a.getWidth() / 4;
            subChorus = a.removeFromTop (16);
            auto row = a.removeFromTop (cellH);
            place (*chMix,   row.removeFromLeft (w4));
            place (*chRate,  row.removeFromLeft (w4));
            place (*chDepth, row.removeFromLeft (w4));
            a.removeFromTop (6);
            subDelay = a.removeFromTop (16);
            row = a.removeFromTop (cellH);
            place (*dlMix,  row.removeFromLeft (w4));
            place (*dlTime, row.removeFromLeft (w4).reduced (14, 0).withTrimmedTop (4));
            place (*dlFb,   row.removeFromLeft (w4));
            a.removeFromTop (6);
            subReverb = a.removeFromTop (16);
            row = a.removeFromTop (cellH);
            place (*rvSend,  row.removeFromLeft (w4));
            place (*rvSize,  row.removeFromLeft (w4));
            place (*rvDamp,  row.removeFromLeft (w4));
            place (*rvLevel, row.removeFromLeft (w4));
        }
    }

    void paint (juce::Graphics& g) override
    {
        drawSection (g, secUni, "UNISON & POWER");
        drawSection (g, secFx,  "EFFECTS");
        drawSubtitle (g, subCpu,    "CPU");
        drawSubtitle (g, subChorus, "CHORUS / ENSEMBLE");
        drawSubtitle (g, subDelay,  "DELAY (TEMPO-SYNC)");
        drawSubtitle (g, subReverb, "REVERB");
        drawHint (g, hintCpu, ParamChoices::u8 ("ECO SPART RECHENLEISTUNG (FILTER OHNE \xc3\x9c" "BERABTASTUNG) \xe2\x80\x93 GUT F\xc3\x9c" "R GROSSE PROJEKTE."));
    }

private:
    Choice *uni = nullptr, *ftype = nullptr, *dlTime = nullptr, *quality = nullptr;
    Knob *detune = nullptr, *spread = nullptr, *sub = nullptr, *heat = nullptr;
    Knob *chMix = nullptr, *chRate = nullptr, *chDepth = nullptr, *dlMix = nullptr, *dlFb = nullptr;
    Knob *rvSend = nullptr, *rvSize = nullptr, *rvDamp = nullptr, *rvLevel = nullptr;
    juce::Rectangle<int> secUni, secFx, subCpu, subChorus, subDelay, subReverb, hintCpu;
};

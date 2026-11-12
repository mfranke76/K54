#pragma once
#include "PanelKit.h"

// ---------------------------------------------------------------------------
//  Reiter KEYS: zweite Klangebene (Clav, Tine Piano, Tonewheel, Pfeifenorgel,
//  FM, Blues Harp) mit eigenem Effekt + Talkbox fuer die Synth-Ebene
// ---------------------------------------------------------------------------
class KeysPanel : public KitPanel, private juce::Timer
{
public:
    explicit KeysPanel (K55Processor& p) : KitPanel (p)
    {
        keysOn  = &toggle (ParamID::keysOn,  "KEYS AN",  "KEYS-Ebene ein-/ausschalten (spielt zusammen mit dem Synth)");
        synthOn = &toggle (ParamID::synthOn, "SYNTH AN", "Synth-Ebene ein-/ausschalten (aus = nur die KEYS spielen)");
        model   = &choice (ParamID::keysModel, "Modell");
        oct     = &choice (ParamID::keysOct, "Oktave");
        level   = &knob (ParamID::keysLevel, ParamChoices::u8 ("LAUTST\xc3\x84RKE"));
        tone    = &knob (ParamID::keysTone, "Tone");
        decay   = &knob (ParamID::keysDecay, "Decay");
        ctlA    = &knob (ParamID::keysA, "A");
        ctlB    = &knob (ParamID::keysB, "B");
        vel     = &knob (ParamID::keysVel, "Velocity");
        fx      = &choice (ParamID::keysFx, "Effekt");
        drive   = &knob (ParamID::keysDrive, "Drive");
        fxAmt   = &knob (ParamID::keysFxAmt, "Amount");
        fxRate  = &knob (ParamID::keysFxRate, "Rate");
        wheel   = &choice (ParamID::keysWheel, "Mod-Rad");
        tMix    = &knob (ParamID::talkMix, "Mix");
        tShift  = &knob (ParamID::talkShift, "Formant", true);
        tRate   = &knob (ParamID::talkRate, "Rate");
        tSrc    = &choice (ParamID::talkSrc, "Vokal-Quelle");

        // Variante: Liste haengt vom Modell ab -> eigenes Feld, schreibt den Int-Parameter
        variant.setJustificationType (juce::Justification::centredLeft);
        variant.onChange = [this]
        {
            if (updating) return;
            const int idx = variant.getSelectedItemIndex();
            if (auto* prm = proc.apvts.getParameter (ParamID::keysVariant); prm != nullptr && idx >= 0)
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) idx));
                prm->endChangeGesture();
            }
        };
        addAndMakeVisible (variant);
        variantLabel.setText ("VARIANTE", juce::dontSendNotification);
        variantLabel.setJustificationType (juce::Justification::centred);
        variantLabel.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.08f));
        addAndMakeVisible (variantLabel);

        model->b.setTooltip ("Klangmodell der KEYS-Ebene - alles synthetisch nachgebaut, keine Aufnahmen");
        variant.setTooltip ("Clav: Tonschalter - Tine: Bauart - Tonewheel: Zugriegel - Orgel: Orgelwelt - FM: Patch - Harp: Mikrofon");
        fx->b.setTooltip ("Effekt nur fuer die KEYS-Ebene (Leslie: Mod-Rad oder Rate > 50 % = schnell)");
        wheel->b.setTooltip ("Was das Mod-Rad mit der KEYS-Ebene macht");
        tSrc->b.setTooltip ("Was den Vokal der Talkbox steuert (U - O - A - E - I)");
        refresh (true);
        startTimerHz (10);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        secKeys = r.removeFromLeft (590); r.removeFromLeft (12);
        secFx   = r.removeFromLeft (290); r.removeFromLeft (12);
        secTalk = r;
        const int cw = 76;
        {
            auto a = secKeys.reduced (12).withTrimmedTop (26);
            auto row = a.removeFromTop (cellH);
            auto tg = row.removeFromLeft (112);
            keysOn->b.setBounds (tg.removeFromTop (row.getHeight() / 2).reduced (2, 6));
            synthOn->b.setBounds (tg.reduced (2, 6));
            row.removeFromLeft (8);
            placeTight (*model, row.removeFromLeft (170).withTrimmedTop (12));
            {
                auto cell = row.removeFromLeft (200).withTrimmedTop (12);
                variantLabel.setBounds (cell.removeFromTop (14));
                variant.setBounds (cell.removeFromTop (26).reduced (3, 0));
            }
            placeTight (*oct, row.withTrimmedTop (12));
            a.removeFromTop (6);
            subSound = a.removeFromTop (16);
            a.removeFromTop (4);
            row = a.removeFromTop (cellH);
            for (auto* k : { level, tone, decay, ctlA, ctlB, vel }) place (*k, row.removeFromLeft (cw + 15));
            a.removeFromTop (8);
            hint = a.reduced (4, 0);
        }
        {
            auto a = secFx.reduced (12).withTrimmedTop (26);
            auto row = a.removeFromTop (cellH);
            placeTight (*fx, row.removeFromLeft (150).withTrimmedTop (12));
            place (*drive, row.removeFromLeft (cw));
            a.removeFromTop (6);
            row = a.removeFromTop (cellH);
            place (*fxAmt, row.removeFromLeft (cw));
            place (*fxRate, row.removeFromLeft (cw));
            placeTight (*wheel, row.withTrimmedTop (12));
            a.removeFromTop (8);
            fxHint = a.reduced (4, 0);
        }
        {
            auto a = secTalk.reduced (12).withTrimmedTop (26);
            auto row = a.removeFromTop (cellH);
            place (*tMix, row.removeFromLeft (cw));
            placeTight (*tSrc, row.withTrimmedTop (12));
            a.removeFromTop (6);
            row = a.removeFromTop (cellH);
            place (*tShift, row.removeFromLeft (cw));
            place (*tRate, row.removeFromLeft (cw));
            a.removeFromTop (8);
            talkHint = a.reduced (4, 0);
        }
    }

    void paint (juce::Graphics& g) override
    {
        drawSection (g, secKeys, "KEYS-EBENE");
        drawSection (g, secFx, "KEYS-EFFEKT");
        drawSection (g, secTalk, "TALKBOX (SYNTH)");
        drawSubtitle (g, subSound, "KLANG");
        drawHint (g, hint, modelHint (curModel));
        drawHint (g, fxHint, ParamChoices::u8 (
            "LESLIE: MOD-RAD AUF \xc2\xbbLESLIE SPEED\xc2\xab = LANGSAM/SCHNELL WIE AM ORGEL-SCHALTER. "
            "WAH (ENV) FOLGT DEM ANSCHLAG - DER FUNK-KLASSIKER AUF DEM CLAV. "
            "SWELL = MOD-RAD ALS SCHWELLER (ORGEL), CC11 WIRKT IMMER."));
        drawHint (g, talkHint, ParamChoices::u8 (
            "DIE SYNTH-EBENE \xc2\xbbSPRICHT\xc2\xab: MIX AUFDREHEN, DANN MIT DEM MOD-RAD VON U \xe2\x80\x93 O \xe2\x80\x93 A \xe2\x80\x93 E \xe2\x80\x93 I FAHREN. "
            "HUELLKURVE = JEDER TON \xc2\xbbWAH\xc2\xab-T VON SELBST. FORMANT VERSCHIEBT DIE STIMME TIEFER/HOEHER."));
    }

private:
    static juce::String modelHint (int m)
    {
        using namespace ParamChoices;
        switch (m)
        {
            case kClav:      return u8 ("FUNK CLAV: GESCHLAGENE SAITE MIT TONABNEHMER. VARIANTE = TONSCHALTER, DAMPER = ABGEDAEMPFT/FUNKY, PICKUP = HALS (WEICH) BIS STEG (SPITZ). HART ANSCHLAGEN = MEHR BISS.");
            case kTine:      return u8 ("TINE PIANO: E-PIANO MIT STIMMZUNGEN. BARK = KNURREN BEI HARTEM ANSCHLAG, BELL = GLOCKEN-ANSCHLAG. MIT PHASER ODER AUTO-PAN WIRD ES ZUM 70ER-SOUL-KLANG.");
            case kTonewheel: return u8 ("TONEWHEEL: ZUGRIEGEL-ORGEL. VARIANTE = REGISTRIERUNG (16' 5 1/3' 8' 4' 2 2/3' 2' 1 3/5' 1 1/3' 1'), PERCUSSION = 3. HARMONISCHE BEIM ANSCHLAG, CLICK = TASTENKLICK. DAZU LESLIE + DRIVE.");
            case kPipe:      return u8 ("PFEIFENORGEL: 16 ORGELWELTEN (SYNTHETISCH NACHEMPFUNDEN). REGISTER = WALZE VON LEISE FLOETE BIS TUTTI, WIND = ANSPRACHE UND WINDGERAEUSCH. UNTER C2 KOMMT DAS PEDAL VON SELBST. DUDELSACK: BORDUN AUF A UND VORSCHLAGSNOTEN KOMMEN VON SELBST - AM BESTEN IN A-DUR SPIELEN. HALL IM REITER FX.");
            case kFmPiano:   return u8 ("FM PIANO: 80ER DIGITAL-E-PIANO (2 OPERATOR-STAPEL). INDEX = HELLIGKEIT, ATTACK = METALLISCHER ANSCHLAG. ANSCHLAG STEUERT DEN GLANZ.");
            case kFmBell:    return u8 ("FM BELL: GLOCKEN, CELESTA, MARIMBA, VIBRAPHON. INDEX = OBERTOENE, ATTACK = ZWEITER STAPEL, DECAY = NACHKLANG.");
            case kFmBass:    return u8 ("FM BASS: KNACKIGE 80ER-BAESSE. INDEX = BISS, ATTACK = ANSCHLAG. OKTAVE -1 IST VOREINGESTELLT IN DEN PRESETS.");
            default:         return u8 ("BLUES HARP: MUNDHARMONIKA-STIMME. GROWL = ATEM UND RAUHEIT, BEND = ANBLASEN VON UNTEN. AFTERTOUCH = VIBRATO, MOD-RAD AUF VIBRATO STELLEN.");
        }
    }

    void refresh (bool force)
    {
        const int m = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::keysModel)->load());
        const int v = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::keysVariant)->load());
        if (force || m != curModel)
        {
            curModel = m;
            updating = true;
            variant.clear (juce::dontSendNotification);
            const auto& names = ParamChoices::keysVariantNames (m);
            for (int i = 0; i < names.size(); ++i) variant.addItem (names[i].toUpperCase(), i + 1);
            updating = false;
            ctlA->l.setText (ParamChoices::keysCtrlName (m, false).toUpperCase(), juce::dontSendNotification);
            ctlB->l.setText (ParamChoices::keysCtrlName (m, true).toUpperCase(), juce::dontSendNotification);
            const bool organ = m == ParamChoices::kTonewheel || m == ParamChoices::kPipe;
            vel->s.setEnabled (! organ);
            vel->s.setAlpha (organ ? 0.35f : 1.0f);
            curVariant = -1;
            repaint();
        }
        if (v != curVariant)
        {
            curVariant = v;
            updating = true;
            variant.setSelectedItemIndex (juce::jlimit (0, variant.getNumItems() - 1, v), juce::dontSendNotification);
            updating = false;
        }
    }

    void timerCallback() override { refresh (false); }

    Toggle *keysOn = nullptr, *synthOn = nullptr;
    Choice *model = nullptr, *oct = nullptr, *fx = nullptr, *wheel = nullptr, *tSrc = nullptr;
    Knob *level = nullptr, *tone = nullptr, *decay = nullptr, *ctlA = nullptr, *ctlB = nullptr, *vel = nullptr;
    Knob *drive = nullptr, *fxAmt = nullptr, *fxRate = nullptr, *tMix = nullptr, *tShift = nullptr, *tRate = nullptr;
    juce::ComboBox variant;
    juce::Label variantLabel;
    bool updating = false;
    int curModel = -1, curVariant = -1;
    juce::Rectangle<int> secKeys, secFx, secTalk, subSound, hint, fxHint, talkHint;
};

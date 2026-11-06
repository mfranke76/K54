#pragma once
#include "PanelKit.h"

// ---------------------------------------------------------------------------
//  Reiter HARMONY: automatische Akkorde zu jeder Taste
// ---------------------------------------------------------------------------
class HarmonyPanel : public KitPanel, private juce::Timer
{
public:
    explicit HarmonyPanel (K55Processor& p) : KitPanel (p)
    {
        onBtn   = &toggle (ParamID::harmOn, "HARMONIE AN", "Automatische Harmonien ein-/ausschalten");
        key     = &choice (ParamID::harmKey, "Tonart");
        scale   = &choice (ParamID::harmScale, "Skala");
        voicing = &choice (ParamID::harmVoicing, "Lage");
        level   = &knob (ParamID::harmLevel, ParamChoices::u8 ("LAUTST\xc3\x84RKE"));
        const char* ids[]   { ParamID::harm3, ParamID::harm5, ParamID::harm7, ParamID::harm9, ParamID::harmOctUp, ParamID::harmOctDn };
        const char* names[] { "TERZ", "QUINTE", "SEPTIME", "NONE", "OKTAVE +", "OKTAVE -" };
        const char* tips[]  { "Terz dazu (leitereigen: Dur oder Moll passend zur Tonart)", "Quinte dazu", "Septime dazu (Jazz/Soul-Farbe)",
                              "None dazu (9, offener Klang)", "Grundton eine Oktave hoeher", "Grundton eine Oktave tiefer (Bass-Fundament)" };
        for (int i = 0; i < 6; ++i) voices[i] = &toggle (ids[i], names[i], tips[i]);
        for (auto* c : { key, scale, voicing }) c->b.onChange = [this] { repaint (secChord); };
        startTimerHz (15);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        secHarm = r.removeFromLeft (640); r.removeFromLeft (12);
        secChord = r;
        {
            auto a = secHarm.reduced (14).withTrimmedTop (26);
            auto row = a.removeFromTop (cellH);
            onBtn->b.setBounds (row.removeFromLeft (150).withSizeKeepingCentre (150, 44).translated (0, 4));
            row.removeFromLeft (14);
            placeTight (*key, row.removeFromLeft (100).withTrimmedTop (12));
            placeTight (*scale, row.removeFromLeft (190).withTrimmedTop (12));
            placeTight (*voicing, row.removeFromLeft (160).withTrimmedTop (12));
            a.removeFromTop (6);
            subVoices = a.removeFromTop (16);
            a.removeFromTop (8);
            row = a.removeFromTop (40);
            const int bw = (row.getWidth() - 5 * 8) / 6;
            for (int i = 0; i < 6; ++i) { voices[i]->b.setBounds (row.removeFromLeft (bw)); row.removeFromLeft (8); }
            a.removeFromTop (14);
            row = a.removeFromTop (cellH);
            place (*level, row.removeFromLeft (80));
            hint = row.reduced (16, 6);
        }
        {
            auto a = secChord.reduced (14).withTrimmedTop (26);
            chordText = a.removeFromTop (70);
            a.removeFromTop (8);
            keys = a.removeFromTop (110);
            legend = a.withTrimmedTop (8);
        }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        drawSection (g, secHarm, "HARMONIZER");
        drawSection (g, secChord, "AKKORD");
        drawSubtitle (g, subVoices, "STIMMEN, DIE DAZUKOMMEN");
        drawHint (g, hint, ParamChoices::u8 (
            "JEDE TASTE WIRD ZUM AKKORD. LEITEREIGEN: DIE TERZ IST AUTOMATISCH DUR ODER MOLL \xe2\x80\x93 PASSEND ZUR TONART. "
            "\xc2\xbb" "AKKORD (FEST)\xc2\xab SCHIEBT IMMER DENSELBEN AKKORD MIT. AM BESTEN IM VOICE-MODUS POLY. "
            "DER KNOPF \xc2\xbbHARMONIE\xc2\xab OBEN SCHALTET ES AUCH VON JEDER SEITE AUS EIN UND AUS."));

        const bool on = proc.apvts.getRawParameterValue (ParamID::harmOn)->load() > 0.5f;
        const int root = proc.harmonizer.lastRoot.load();
        juce::Array<int> chord;
        if (root >= 0)
        {
            chord.add (root);
            for (int k = 0; k < proc.harmonizer.lastCount.load() && k < Harmonizer::maxExtra; ++k)
                if (proc.harmonizer.lastNotes[(size_t) k].load() >= 0) chord.add (proc.harmonizer.lastNotes[(size_t) k].load());
        }

        // grosser Akkordname
        g.setColour (on ? cream : creamDim.withAlpha (0.5f));
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 30.0f, 0.08f));
        g.drawText (root >= 0 ? chordName (chord) : juce::String ("--"), chordText.removeFromTop (40), juce::Justification::centredLeft);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 10.0f, 0.12f));
        g.setColour (creamDim);
        juce::StringArray names;
        auto sorted = chord; sorted.sort();
        for (int n : sorted) names.add (juce::MidiMessage::getMidiNoteName (n, true, true, 3));
        g.drawText (on ? names.joinIntoString (ParamChoices::u8 ("  \xc2\xb7  ")) : juce::String ("HARMONIZER IST AUS"),
                    chordText, juce::Justification::centredLeft);

        // Mini-Tastatur: 2 Oktaven, Leitertoene markiert, letzter Akkord leuchtet
        const int keyIdx = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::harmKey)->load());
        const int sc = juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::harmScale)->load());
        int lo = 48;
        if (! sorted.isEmpty()) lo = juce::jlimit (0, 103, (sorted.getFirst() / 12) * 12);
        if (! sorted.isEmpty() && sorted.getLast() >= lo + 24) lo = juce::jlimit (0, 103, sorted.getLast() - 23 - (sorted.getLast() % 12) + 12);
        auto inScale = [&] (int n)
        {
            if (sc >= 6) return false;
            const int rel = ((n - keyIdx) % 12 + 12) % 12;
            for (int s : Harmonizer::scaleSteps (sc)) if (s == rel) return true;
            return false;
        };
        const float ww = (float) keys.getWidth() / 14.0f;
        auto kb = keys.toFloat();
        static const int whiteOf[] { 0, 2, 4, 5, 7, 9, 11 };
        for (int o = 0; o < 2; ++o)
            for (int w = 0; w < 7; ++w)
            {
                const int n = lo + o * 12 + whiteOf[w];
                auto r = juce::Rectangle<float> (kb.getX() + (o * 7 + w) * ww, kb.getY(), ww - 2.0f, kb.getHeight());
                g.setColour (n == root && on ? gold : (chord.contains (n) && on ? accent2 : cream.withAlpha (0.92f)));
                g.fillRoundedRectangle (r, 3.0f);
                if (inScale (n)) { g.setColour (goldDim); g.fillEllipse (r.getCentreX() - 3.0f, r.getBottom() - 12.0f, 6.0f, 6.0f); }
            }
        static const int blackOf[] { 1, 3, 6, 8, 10 };
        static const float blackPos[] { 1, 2, 4, 5, 6 };
        for (int o = 0; o < 2; ++o)
            for (int b = 0; b < 5; ++b)
            {
                const int n = lo + o * 12 + blackOf[b];
                auto r = juce::Rectangle<float> (kb.getX() + (o * 7 + blackPos[b]) * ww - ww * 0.32f, kb.getY(), ww * 0.62f, kb.getHeight() * 0.6f);
                g.setColour (n == root && on ? gold : (chord.contains (n) && on ? accent2 : juce::Colour (0xff121215)));
                g.fillRoundedRectangle (r, 2.0f);
                if (inScale (n)) { g.setColour (goldDim); g.fillEllipse (r.getCentreX() - 2.5f, r.getBottom() - 10.0f, 5.0f, 5.0f); }
            }
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.2f));
        g.setColour (creamDim.withAlpha (0.7f));
        g.drawText (ParamChoices::u8 ("\xe2\x97\x8f LEITERTON    GOLD = GESPIELT    BLAU = HARMONIE"), legend.removeFromTop (14), juce::Justification::centredLeft);
    }

private:
    static juce::String chordName (const juce::Array<int>& notes)
    {
        if (notes.isEmpty()) return "--";
        const int root = notes.getFirst();
        bool has[12] {};
        for (int n : notes) has[((n - root) % 12 + 12) % 12] = true;
        juce::String q;
        if (has[4] && has[8] && ! has[7]) q = "+";
        else if (has[3] && has[6] && ! has[7]) q = "dim";
        else if (has[3]) q = "m";
        else if (has[5] && ! has[4]) q = "sus4";
        else if (has[2] && ! has[4] && ! has[3]) q = "sus2";
        if (has[10]) q += "7"; else if (has[11]) q += "maj7";
        if (has[2] && (has[3] || has[4])) q += (has[10] || has[11]) ? "/9" : "add9";
        if (! has[3] && ! has[4] && has[7] && q.isEmpty()) q = "5";
        return ParamChoices::keys[root % 12] + q;
    }

    void timerCallback() override
    {
        const int v = proc.harmonizer.chordVersion.load();
        const bool on = proc.apvts.getRawParameterValue (ParamID::harmOn)->load() > 0.5f;
        if (v != lastVersion || on != lastOn) { lastVersion = v; lastOn = on; repaint (secChord); }
    }

    Toggle* onBtn = nullptr;
    Toggle* voices[6] {};
    Choice *key = nullptr, *scale = nullptr, *voicing = nullptr;
    Knob* level = nullptr;
    int lastVersion = -1;
    bool lastOn = false;
    juce::Rectangle<int> secHarm, secChord, subVoices, hint, chordText, keys, legend;
};

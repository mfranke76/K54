#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <vector>

// ============================================================================
//  K-54 S Harmonizer: macht aus jeder gespielten Taste einen passenden Akkord
//  (leitereigen in Tonart/Skala oder als fester Dur-/Moll-Akkord)
// ============================================================================
class Harmonizer
{
public:
    struct Settings
    {
        bool on = false;
        int key = 0, scale = 0, voicing = 0;
        bool third = true, fifth = true, seventh = false, ninth = false, octUp = false, octDown = false;
        float level = 0.8f;
    };

    static constexpr int maxExtra = 6;

    void reset() { held.fill ({}); count.fill (0); wasOn = false; }

    /** Ersetzt die Noten im Puffer durch Grundton + Harmonien */
    void process (juce::MidiBuffer& midi, const Settings& st)
    {
        if (! st.on && ! wasOn) return;                                   // aus: MIDI unveraendert

        out.clear();
        if (wasOn && ! st.on)          // ausgeschaltet: haengende Harmonie-Noten loesen
            for (int n = 0; n < 128; ++n)
                for (int k = 0; k < held[(size_t) n].num; ++k)
                    release (held[(size_t) n].notes[(size_t) k], 0, 1);
        if (wasOn && ! st.on)
            for (auto& h : held) h.num = 0;
        wasOn = st.on;

        for (const auto meta : midi)
        {
            const auto& m = meta.getMessage();
            const int pos = meta.samplePosition;
            if (m.isNoteOn() && st.on)
            {
                const int n = m.getNoteNumber();
                auto& h = held[(size_t) n];
                for (int k = 0; k < h.num; ++k) release (h.notes[(size_t) k], pos, m.getChannel());   // gleiche Taste nochmal
                h.num = 0;
                int notes[maxExtra];
                const int cnt = build (n, st, notes);
                press (n, m.getVelocity(), pos, m.getChannel(), true);
                const auto v = (juce::uint8) juce::jlimit (1, 127, juce::roundToInt (m.getVelocity() * st.level));
                for (int k = 0; k < cnt; ++k)
                {
                    press (notes[k], v, pos, m.getChannel(), false);
                    h.notes[(size_t) h.num++] = notes[k];
                }
                // fuer die Anzeige
                lastRoot = n;
                lastCount = cnt;
                for (int k = 0; k < maxExtra; ++k) lastNotes[(size_t) k] = k < cnt ? notes[k] : -1;
                ++chordVersion;
            }
            else if (m.isNoteOff() && (st.on || held[(size_t) m.getNoteNumber()].num > 0))
            {
                const int n = m.getNoteNumber();
                release (n, pos, m.getChannel());
                auto& h = held[(size_t) n];
                for (int k = 0; k < h.num; ++k) release (h.notes[(size_t) k], pos, m.getChannel());
                h.num = 0;
            }
            else if (m.isNoteOn())  out.addEvent (m, pos);
            else if (m.isNoteOff()) release (m.getNoteNumber(), pos, m.getChannel());
            else
            {
                if (m.isAllNotesOff() || m.isAllSoundOff()) { held.fill ({}); count.fill (0); }
                out.addEvent (m, pos);
            }
        }
        midi.swapWith (out);
    }

    /** Akkordtoene fuer eine Taste (ohne den Grundton) */
    static int build (int note, const Settings& st, int* res)
    {
        int c = 0;
        auto add = [&] (int n) { if (n >= 0 && n < 128 && n != note && c < maxExtra) { for (int i = 0; i < c; ++i) if (res[i] == n) return; res[c++] = n; } };
        const bool fixed = st.scale >= 6;
        auto above = [&] (int steps, int fixedSemis) { return fixed ? note + fixedSemis : diatonic (note, st.key, st.scale, steps); };
        const bool minor = st.scale == 7;
        int third = above (2, minor ? 3 : 4), fifth = above (4, 7), sev = above (6, 10), nine = above (8, 14);
        if (st.voicing == 1) third += 12;                                  // offen: Terz eine Oktave hoeher
        if (st.voicing == 2) { third -= 12; fifth -= 12; sev -= 12; nine -= 12; }   // darunter
        if (st.third)   add (third);
        if (st.fifth)   add (fifth);
        if (st.seventh) add (sev);
        if (st.ninth)   add (nine);
        if (st.octUp)   add (note + 12);
        if (st.octDown) add (note - 12);
        return c;
    }

    static const std::vector<int>& scaleSteps (int scale)
    {
        static const std::vector<int> sc[6] {
            { 0, 2, 4, 5, 7, 9, 11 },   // Dur
            { 0, 2, 3, 5, 7, 8, 10 },   // Moll
            { 0, 2, 3, 5, 7, 9, 10 },   // Dorisch
            { 0, 2, 4, 5, 7, 9, 10 },   // Mixolydisch
            { 0, 2, 3, 5, 7, 8, 11 },   // Harmonisch Moll
            { 0, 2, 4, 7, 9 } };        // Pentatonik
        return sc[juce::jlimit (0, 5, scale)];
    }

    /** Note "steps" Leiterstufen ueber "note" (Fremdtoene werden parallel verschoben) */
    static int diatonic (int note, int key, int scale, int steps)
    {
        const auto& s = scaleSteps (scale);
        const int len = (int) s.size();
        const int rel = ((note - key) % 12 + 12) % 12;
        int deg = 0;
        for (int i = 0; i < len; ++i) if (s[(size_t) i] <= rel) deg = i;    // naechste Stufe darunter
        const int offset = rel - s[(size_t) deg];                         // 0 = leitereigen
        const int base = note - offset;
        const int target = deg + steps;
        const int oct = target / len;
        const int semis = s[(size_t) (target % len)] - s[(size_t) deg] + oct * 12;
        return base + semis + offset;
    }

    /** Fuer die Anzeige (Message-Thread liest) */
    std::atomic<int> chordVersion { 0 };
    std::atomic<int> lastRoot { -1 }, lastCount { 0 };
    std::array<std::atomic<int>, maxExtra> lastNotes {};

private:
    struct Held { std::array<int, maxExtra> notes {}; int num = 0; };

    void press (int n, int vel, int pos, int ch, bool)
    {
        if (count[(size_t) n] > 0)            // Ton klingt schon (anderer Akkord): neu anschlagen ohne doppelte Stimme
            out.addEvent (juce::MidiMessage::noteOff (ch, n), pos);
        ++count[(size_t) n];
        out.addEvent (juce::MidiMessage::noteOn (ch, n, (juce::uint8) juce::jlimit (1, 127, vel)), pos);
    }
    void release (int n, int pos, int ch)
    {
        // unbekannte Note (z. B. vor dem Einschalten gedrueckt): Note-Off einfach durchlassen
        if (count[(size_t) n] <= 0 || --count[(size_t) n] == 0) { count[(size_t) n] = 0; out.addEvent (juce::MidiMessage::noteOff (ch, n), pos); }
    }

    std::array<Held, 128> held {};
    std::array<int, 128> count {};
    juce::MidiBuffer out;
    bool wasOn = false;
};

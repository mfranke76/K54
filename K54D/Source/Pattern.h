#pragma once
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>
#include "DrumEngine.h"

// ============================================================================
//  Patterns A-D: pro Spur und Step eine Lautstaerke (0 = aus, 1..127)
//  und ein Wirbel (1 = einfacher Schlag, 2-4 = Roll innerhalb des Steps).
//  Wird von der Oberflaeche geschrieben und im Audio-Thread gelesen (atomar).
// ============================================================================
class PatternStore
{
public:
    static constexpr int P = DrumInfo::numPatterns, L = DrumInfo::numLanes, S = DrumInfo::maxSteps;

    PatternStore() { clearAll(); }

    int  vel (int p, int l, int s) const        { return v[p][l][s].load (std::memory_order_relaxed); }
    int  ratchet (int p, int l, int s) const    { return juce::jmax (1, (int) r[p][l][s].load (std::memory_order_relaxed)); }
    void setVel (int p, int l, int s, int x)    { v[p][l][s].store ((juce::uint8) juce::jlimit (0, 127, x)); bump(); }
    void setRatchet (int p, int l, int s, int x){ r[p][l][s].store ((juce::uint8) juce::jlimit (1, 4, x)); bump(); }

    void clear (int p)
    {
        for (int l = 0; l < L; ++l) clearLane (p, l);
    }
    void clearLane (int p, int l)
    {
        for (int s = 0; s < S; ++s) { v[p][l][s].store (0); r[p][l][s].store (1); }
        bump();
    }
    void clearAll() { for (int p = 0; p < P; ++p) clear (p); }

    void copy (int from, int to)
    {
        if (from == to) return;
        for (int l = 0; l < L; ++l)
            for (int s = 0; s < S; ++s) { v[to][l][s].store (v[from][l][s].load()); r[to][l][s].store (r[from][l][s].load()); }
        bump();
    }

    bool laneHasNotes (int p, int l) const
    {
        for (int s = 0; s < S; ++s) if (vel (p, l, s) > 0) return true;
        return false;
    }

    /** Text: je Step 2 Hex-Ziffern Lautstaerke + 1 Ziffer Wirbel */
    juce::String laneToString (int p, int l) const
    {
        juce::String out;
        for (int s = 0; s < S; ++s)
            out << juce::String::toHexString (vel (p, l, s)).paddedLeft ('0', 2) << juce::String (ratchet (p, l, s));
        return out;
    }
    void laneFromString (int p, int l, const juce::String& txt)
    {
        for (int s = 0; s < S; ++s)
        {
            const auto chunk = txt.substring (s * 3, s * 3 + 3);
            if (chunk.length() < 3) { v[p][l][s].store (0); r[p][l][s].store (1); continue; }
            v[p][l][s].store ((juce::uint8) juce::jlimit (0, 127, chunk.substring (0, 2).getHexValue32()));
            r[p][l][s].store ((juce::uint8) juce::jlimit (1, 4, chunk.substring (2, 3).getIntValue()));
        }
        bump();
    }

    juce::ValueTree toTree() const
    {
        juce::ValueTree t ("PATTERNS");
        for (int p = 0; p < P; ++p)
        {
            juce::ValueTree pt ("P");
            pt.setProperty ("i", p, nullptr);
            for (int l = 0; l < L; ++l)
                if (laneHasNotes (p, l))
                {
                    juce::ValueTree lt ("L");
                    lt.setProperty ("i", l, nullptr);
                    lt.setProperty ("d", laneToString (p, l), nullptr);
                    pt.appendChild (lt, nullptr);
                }
            t.appendChild (pt, nullptr);
        }
        return t;
    }

    void fromTree (const juce::ValueTree& t)
    {
        if (! t.hasType ("PATTERNS")) return;
        clearAll();
        for (auto pt : t)
        {
            const int p = pt.getProperty ("i", -1);
            if (p < 0 || p >= P) continue;
            for (auto lt : pt)
            {
                const int l = lt.getProperty ("i", -1);
                if (l >= 0 && l < L) laneFromString (p, l, lt.getProperty ("d").toString());
            }
        }
    }

    /** zaehlt bei jeder Aenderung hoch (Oberflaeche zeichnet dann neu) */
    int getChangeCount() const { return changes.load(); }

private:
    void bump() { changes.fetch_add (1); }
    std::atomic<juce::uint8> v[P][L][S];
    std::atomic<juce::uint8> r[P][L][S];
    std::atomic<int> changes { 0 };
};

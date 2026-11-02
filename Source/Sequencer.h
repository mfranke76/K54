#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <atomic>
#include <vector>
#include <cmath>
#include "Drums.h"
#include "Looper.h"

// ============================================================================
//  K-54 Step-Sequencer: 8 Patterns (A-H), je 16 Schritte (16tel)
//   - Drum-Spuren: aus / an / Akzent
//   - Synth-Spur:  an, Ton (relativ zum Grundton), Akzent, Slide
//  Laeuft im Takt von DAW, MIDI-Clock oder internem Takt (ClockInfo vom Looper).
// ============================================================================

class StepSequencer
{
public:
    static constexpr int numPatterns = 8, numSteps = 16;
    enum : uint8_t { StepOff = 0, StepOn = 1, StepAccent = 2 };
    enum : uint8_t { SynOn = 1, SynAccent = 2, SynSlide = 4 };

    struct Pattern
    {
        std::array<std::array<std::atomic<uint8_t>, numSteps>, DrumInfo::numLanes> drum;
        std::array<std::atomic<int8_t>,  numSteps> note;    // Halbtoene ueber dem Grundton (-12..+24)
        std::array<std::atomic<uint8_t>, numSteps> flags;   // SynOn | SynAccent | SynSlide
    };

    struct Settings
    {
        bool  drumsOn = false, synthOn = false;
        int   pattern = 0, drumLength = 16, synthLength = 16;
        float swing = 0.0f;        // 0 .. 0.6
        int   rootNote = 36;       // C2
        float gate = 0.5f;         // Anteil eines 16tels
        int   synthChannel = 1;
    };

    StepSequencer() { clearAll(); loadDefaults(); }

    // ---------------------------------------------------------------- Bearbeiten (GUI-Thread)
    uint8_t getDrum (int p, int lane, int step) const { return pat[(size_t) p].drum[(size_t) lane][(size_t) step].load(); }
    void    setDrum (int p, int lane, int step, uint8_t v) { pat[(size_t) p].drum[(size_t) lane][(size_t) step].store (v); }
    int     getNote (int p, int step) const { return pat[(size_t) p].note[(size_t) step].load(); }
    void    setNote (int p, int step, int v) { pat[(size_t) p].note[(size_t) step].store ((int8_t) juce::jlimit (-12, 24, v)); }
    uint8_t getFlags (int p, int step) const { return pat[(size_t) p].flags[(size_t) step].load(); }
    void    setFlags (int p, int step, uint8_t f) { pat[(size_t) p].flags[(size_t) step].store (f); }

    void clearPattern (int p)
    {
        for (auto& lane : pat[(size_t) p].drum) for (auto& s : lane) s.store (StepOff);
        for (auto& n : pat[(size_t) p].note) n.store (0);
        for (auto& f : pat[(size_t) p].flags) f.store (0);
    }
    void clearAll() { for (int p = 0; p < numPatterns; ++p) clearPattern (p); }
    void copyPattern (int from, int to)
    {
        for (int l = 0; l < DrumInfo::numLanes; ++l)
            for (int s = 0; s < numSteps; ++s) setDrum (to, l, s, getDrum (from, l, s));
        for (int s = 0; s < numSteps; ++s) { setNote (to, s, getNote (from, s)); setFlags (to, s, getFlags (from, s)); }
    }

    int  currentDrumStep() const  { return playDrumStep.load(); }
    int  currentSynthStep() const { return playSynthStep.load(); }
    int  activePattern() const    { return activePat.load(); }

    // ---------------------------------------------------------------- Zustand speichern / laden
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree t ("PATTERNS");
        for (int p = 0; p < numPatterns; ++p)
        {
            juce::ValueTree pt ("P");
            pt.setProperty ("i", p, nullptr);
            juce::String d;
            for (int l = 0; l < DrumInfo::numLanes; ++l)
            {
                for (int s = 0; s < numSteps; ++s) d << (int) getDrum (p, l, s);
                d << (l < DrumInfo::numLanes - 1 ? "|" : "");
            }
            pt.setProperty ("drums", d, nullptr);
            juce::StringArray notes, flags;
            for (int s = 0; s < numSteps; ++s) { notes.add (juce::String (getNote (p, s))); flags.add (juce::String ((int) getFlags (p, s))); }
            pt.setProperty ("notes", notes.joinIntoString (","), nullptr);
            pt.setProperty ("flags", flags.joinIntoString (","), nullptr);
            t.appendChild (pt, nullptr);
        }
        return t;
    }

    void fromValueTree (const juce::ValueTree& t)
    {
        if (! t.hasType ("PATTERNS")) return;
        for (const auto& pt : t)
        {
            const int p = (int) pt["i"];
            if (! juce::isPositiveAndBelow (p, numPatterns)) continue;
            clearPattern (p);
            auto lanes = juce::StringArray::fromTokens (pt["drums"].toString(), "|", {});
            for (int l = 0; l < juce::jmin (lanes.size(), DrumInfo::numLanes); ++l)
                for (int s = 0; s < juce::jmin (lanes[l].length(), numSteps); ++s)
                    setDrum (p, l, s, (uint8_t) juce::jlimit (0, 2, (int) lanes[l][s] - (int) '0'));
            auto notes = juce::StringArray::fromTokens (pt["notes"].toString(), ",", {});
            auto flags = juce::StringArray::fromTokens (pt["flags"].toString(), ",", {});
            for (int s = 0; s < numSteps; ++s)
            {
                if (s < notes.size()) setNote (p, s, notes[s].getIntValue());
                if (s < flags.size()) setFlags (p, s, (uint8_t) (flags[s].getIntValue() & 7));
            }
        }
    }

    // ---------------------------------------------------------------- Audio-Thread
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        pendingOffs.clear();
        pendingOffs.reserve (64);
        lastStepFired = -1;
        hasHeld = false;
    }

    /** Erzeugt fuer diesen Block Drum-Schlaege (hits) und Synth-Noten (in synthMidi). */
    void process (const ClockInfo& clk, int numSamples, const Settings& set,
                  std::vector<DrumMachine::Hit>& hits, juce::MidiBuffer& synthMidi)
    {
        const int ch = juce::jlimit (1, 16, set.synthChannel);

        if (! clk.running)
        {
            if (wasRunning) allSynthNotesOff (synthMidi, 0, ch);
            wasRunning = false;
            playDrumStep.store (-1); playSynthStep.store (-1);
            activePat.store (set.pattern);
            return;
        }
        if (! set.synthOn && hasHeld) allSynthNotesOff (synthMidi, 0, ch);

        const double stepLen = 0.25;
        const double b0 = clk.beat0, b1 = clk.beat0 + numSamples * clk.beatPerSample;
        if (clk.jumped || ! wasRunning)
        {
            allSynthNotesOff (synthMidi, 0, ch);
            lastStepFired = (long long) std::ceil (b0 / stepLen - 1e-6) - 1;
        }
        wasRunning = true;

        auto stepBeat = [&] (long long k)
        {
            return (double) k * stepLen + ((k & 1) ? (double) set.swing * stepLen : 0.0);
        };
        auto offsetOf = [&] (double beat)
        {
            return juce::jlimit (0, numSamples - 1, (int) std::floor ((beat - b0) / clk.beatPerSample + 0.5));
        };

        // faellige Note-Offs
        for (size_t i = 0; i < pendingOffs.size();)
        {
            if (pendingOffs[i].beat < b1)
            {
                synthMidi.addEvent (juce::MidiMessage::noteOff (ch, pendingOffs[i].note), offsetOf (juce::jmax (b0, pendingOffs[i].beat)));
                if (pendingOffs[i].note == heldNote) hasHeld = false;
                pendingOffs.erase (pendingOffs.begin() + (long) i);
            }
            else ++i;
        }

        for (long long k = lastStepFired + 1;; ++k)
        {
            const double sb = stepBeat (k);
            if (sb >= b1) break;
            lastStepFired = k;
            if (sb < b0 - 1e-6) continue;                 // verpasst (z. B. Swing-Grenze): nicht nachholen
            const int off = offsetOf (sb);

            // Patternwechsel am Anfang eines Takts (16 Schritte)
            if ((k % numSteps) == 0) activePat.store (set.pattern);
            const int p = activePat.load();

            // Drums
            const int dLen = juce::jlimit (1, numSteps, set.drumLength);
            const int ds = (int) (k % dLen);
            playDrumStep.store (ds);
            if (set.drumsOn)
                for (int l = 0; l < DrumInfo::numLanes; ++l)
                {
                    const auto v = pat[(size_t) p].drum[(size_t) l][(size_t) ds].load (std::memory_order_relaxed);
                    if (v != StepOff)
                        hits.push_back ({ l, off, v == StepAccent ? 1.0f : 0.72f });
                }

            // Synth
            const int sLen = juce::jlimit (1, numSteps, set.synthLength);
            const int ss = (int) (k % sLen);
            playSynthStep.store (ss);
            if (set.synthOn)
            {
                const auto f = pat[(size_t) p].flags[(size_t) ss].load (std::memory_order_relaxed);
                if (f & SynOn)
                {
                    const int note = juce::jlimit (0, 127, set.rootNote + pat[(size_t) p].note[(size_t) ss].load (std::memory_order_relaxed));
                    const float vel = (f & SynAccent) ? 1.0f : 0.7f;
                    // Slide vom vorigen Schritt: neue Note vor dem Note-Off der alten (legato).
                    // Gleicher Ton nach Slide = gebunden (kein neuer Anschlag).
                    bool tied = false;
                    for (size_t i = 0; i < pendingOffs.size();)
                    {
                        auto& po = pendingOffs[i];
                        if (po.slideHold && po.note == note) { tied = true; pendingOffs.erase (pendingOffs.begin() + (long) i); continue; }
                        if (po.slideHold) { po.beat = sb + 1e-5; po.slideHold = false; }
                        ++i;
                    }
                    if (! tied)
                        synthMidi.addEvent (juce::MidiMessage::noteOn (ch, note, vel), off);
                    const bool slide = (f & SynSlide) != 0;
                    const double offBeat = slide ? stepBeat (k + 1) + 1.0e3 : sb + juce::jlimit (0.05, 0.98, (double) set.gate) * stepLen;
                    pendingOffs.push_back ({ note, offBeat, slide });
                    heldNote = note; hasHeld = true;
                }
                else
                {
                    // Pause beendet eine gehaltene Slide-Note
                    for (auto& po : pendingOffs)
                        if (po.slideHold) { po.beat = sb; po.slideHold = false; }
                }
            }
        }

        // Offs, die durch Slide-Ende in diesen Block gefallen sind
        for (size_t i = 0; i < pendingOffs.size();)
        {
            if (pendingOffs[i].beat < b1)
            {
                synthMidi.addEvent (juce::MidiMessage::noteOff (ch, pendingOffs[i].note), offsetOf (juce::jmax (b0, pendingOffs[i].beat)));
                pendingOffs.erase (pendingOffs.begin() + (long) i);
            }
            else ++i;
        }
    }

private:
    struct PendingOff { int note; double beat; bool slideHold; };

    void allSynthNotesOff (juce::MidiBuffer& m, int off, int ch)
    {
        for (auto& po : pendingOffs) m.addEvent (juce::MidiMessage::noteOff (ch, po.note), off);
        pendingOffs.clear();
        hasHeld = false;
    }

    void loadDefaults()
    {
        auto setRow = [this] (int p, int lane, const char* s)
        {
            for (int i = 0; i < numSteps && s[i]; ++i)
                setDrum (p, lane, i, s[i] == 'X' ? StepAccent : (s[i] == 'x' ? StepOn : StepOff));
        };
        auto setSyn = [this] (int p, std::initializer_list<int> notes, const char* flags)
        {
            int i = 0;
            for (int n : notes)
            {
                if (i >= numSteps) break;
                setNote (p, i, n);
                const char f = flags[i];
                setFlags (p, i, f == '.' ? 0 : (uint8_t) (SynOn | (f == 'A' || f == 'S' ? SynAccent : 0) | (f == 's' || f == 'S' ? SynSlide : 0)));
                ++i;
            }
        };
        // A: Funk
        setRow (0, 0, "X..x..x...X..x..");
        setRow (0, 1, "....X.......X...");
        setRow (0, 3, "x.xXx.xXx.xXx.xX");
        setRow (0, 4, "..............x.");
        setRow (0, 7, "..x.......x.....");
        setSyn (0, { 0, 0, 12, 0, 0, 10, 0, 12, 0, 0, 7, 0, 10, 0, 12, 15 }, "A.xs.x.xA.x.x.xA");
        // B: Four on the floor
        setRow (1, 0, "X...X...X...X...");
        setRow (1, 2, "....X.......X...");
        setRow (1, 3, "x.x.x.x.x.x.x.x.");
        setRow (1, 4, "..x...x...x...x.");
        setSyn (1, { 0, 0, 12, 0, 0, 0, 12, 0, 0, 0, 12, 0, 3, 0, 5, 7 }, "x.x.x.x.x.x.x.xs");
        // C: Breakbeat
        setRow (2, 0, "X.........x.x...");
        setRow (2, 1, "....X.......X..x");
        setRow (2, 3, "x.x.x.x.x.x.x.x.");
        setRow (2, 5, "..............x.");
        setSyn (2, { 0, 0, 0, 3, 0, 0, 5, 0, 0, 7, 0, 5, 0, 3, 0, 0 }, "A..x..s.x..x.x..");
        // D: Acid
        setRow (3, 0, "X...X...X...X...");
        setRow (3, 3, "..x...x...x...x.");
        setRow (3, 2, "....x.......x...");
        setSyn (3, { 0, 12, 0, 0, 15, 0, 12, 10, 0, 0, 12, 0, 7, 19, 12, 0 }, "AxsxAxsxAsxxSxxx");
    }

    std::array<Pattern, numPatterns> pat;
    double sr = 48000.0;
    long long lastStepFired = -1;
    bool wasRunning = false, hasHeld = false;
    int heldNote = -1;
    std::vector<PendingOff> pendingOffs;
    std::atomic<int> playDrumStep { -1 }, playSynthStep { -1 }, activePat { 0 };
};

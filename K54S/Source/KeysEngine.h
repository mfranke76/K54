#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include "Effects.h"
#include "Parameters.h"

// ============================================================================
//  K-54 S  ·  KEYS-Ebene (1.2.0)
//  Eigene Klangmodelle neben dem Synth – alles synthetisch, keine Aufnahmen:
//    Funk Clav       – Saite (Karplus-Strong) + Tonabnehmer-Kamm + Tonschalter
//    Tine Piano      – Stimmgabel-Zinke + Glocken-Oberton + nichtlinearer Pickup
//    Tonewheel Organ – 9 Zugriegel, Percussion, Tastenklick (+ Leslie im FX)
//    Pipe Organ      – Register-Pfeifen in 15 Orgelwelten, Crescendo, Wind, Pedal
//    FM Piano/Bell/Bass – 2 x 2-Operator-Stapel mit Feedback
//    Blues Harp      – Durchschlagzunge mit Hand-Formant, Atem und Bend
//  Danach: Tonfilter -> Drive -> Effekt (Phaser/Tremolo/Pan/Chorus/Wah/Leslie)
// ============================================================================

struct KeysParams
{
    bool  on = false;
    int   model = 0, variant = 0, octave = 0;
    float level = 0.5f, tone = 0.5f, decay = 0.5f, a = 0.4f, b = 0.3f, vel = 0.6f, drive = 0.0f;
    int   fx = 0, wheel = 0;
    float fxAmt = 0.5f, fxRate = 0.4f;
    float bendUp = 2.0f, bendDown = 2.0f;
};

// ---------------------------------------------------------------------------
//  Einzyklus-Tabellen mit Oberton-Stufen (kein Aliasing)
namespace KeysTables
{
    constexpr int size = 2048, mask = size - 1, levels = 8;
    constexpr int maxH[levels] { 128, 64, 32, 16, 8, 4, 2, 1 };

    struct Table
    {
        std::vector<float> data;    // levels * (size + 1)
        inline float read (double phase, int level) const noexcept
        {
            const float* d = data.data() + level * (size + 1);
            const double p = phase * size;
            const int i = (int) p;
            const float fr = (float) (p - i);
            return d[i] + (d[i + 1] - d[i]) * fr;
        }
        static int levelFor (double inc) noexcept
        {
            const double allowed = 0.45 / juce::jmax (1.0e-6, inc);
            int l = 0;
            while (l < levels - 1 && maxH[l] > allowed) ++l;
            return l;
        }
    };

    enum Kind { Principal = 0, StoppedFlute, OpenFlute, String, Reed, Clarinet, Tibia, Sine, NumKinds };

    inline float amp (int kind, int n) noexcept
    {
        const float fn = (float) n;
        switch (kind)
        {
            case Principal:    return (n == 2 ? 1.15f : 1.0f) / std::pow (fn, 1.25f) * (n > 24 ? 0.0f : 1.0f);
            case StoppedFlute: return n == 1 ? 1.0f : (n % 2 == 1 ? 0.22f / fn : 0.04f / fn) * (n > 9 ? 0.0f : 1.0f);
            case OpenFlute:    return n == 1 ? 1.0f : (n > 6 ? 0.0f : 0.28f / (fn * fn) * 2.0f);
            case String:       return 1.0f / std::pow (fn, 0.75f) * (n > 40 ? 0.0f : 1.0f);
            case Reed:
            {
                const float form = 1.0f + 1.6f * std::exp (-0.5f * std::pow ((fn - 8.0f) / 3.5f, 2.0f));
                return form / std::pow (fn, 0.62f) * (n > 60 ? 0.0f : 1.0f);
            }
            case Clarinet:     return (n % 2 == 1 ? 1.0f / fn : 0.08f / fn) * (n > 30 ? 0.0f : 1.0f);
            case Tibia:        return n == 1 ? 1.0f : (n == 2 ? 0.05f : (n == 3 ? 0.06f : 0.0f));
            default:           return n == 1 ? 1.0f : 0.0f;
        }
    }

    inline const Table& get (int kind)
    {
        static const std::vector<Table> all = []
        {
            std::vector<Table> t ((size_t) NumKinds);
            for (int k = 0; k < NumKinds; ++k)
            {
                auto& d = t[(size_t) k].data;
                d.assign ((size_t) levels * (size + 1), 0.0f);
                float norm = 1.0f;
                for (int l = 0; l < levels; ++l)
                {
                    float* row = d.data() + l * (size + 1);
                    for (int n = 1; n <= maxH[l]; ++n)
                    {
                        const float a = amp (k, n);
                        if (a == 0.0f) continue;
                        const double ph = (double) n * n * 0.37;          // feste Phasen -> geringere Spitzen
                        for (int i = 0; i < size; ++i)
                            row[i] += a * (float) std::sin (juce::MathConstants<double>::twoPi * n * i / size + ph);
                    }
                    if (l == 0)
                    {
                        float pk = 0.0f;
                        for (int i = 0; i < size; ++i) pk = juce::jmax (pk, std::abs (row[i]));
                        norm = pk > 0.0f ? 1.0f / pk : 1.0f;
                    }
                    for (int i = 0; i < size; ++i) row[i] *= norm;
                    row[size] = row[0];
                }
            }
            return t;
        }();
        return all[(size_t) juce::jlimit (0, NumKinds - 1, kind)];
    }

    inline float sine (double phase) noexcept
    {
        static const std::vector<float> s = []
        {
            std::vector<float> v (size + 1);
            for (int i = 0; i <= size; ++i) v[(size_t) i] = (float) std::sin (juce::MathConstants<double>::twoPi * i / size);
            return v;
        }();
        const double p = phase * size;
        const int i = (int) p;
        const float fr = (float) (p - i);
        return s[(size_t) i] + (s[(size_t) i + 1] - s[(size_t) i]) * fr;
    }
}

// ---------------------------------------------------------------------------
//  Pfeifenorgel: Register und Orgelwelten
namespace PipeOrgan
{
    enum Stop { Bourdon16 = 0, Prinzipal8, Floete8, Gambe8, Oktave4, Floete4, Quinte, Superoktave2, Mixtur, Cymbel,
                Trompete8, Bombarde16, Tuba8, Tibia8, VoxCeleste8, Krummhorn8, NumStops };

    struct StopDef
    {
        int kind;
        float ratios[4];
        int numRanks;
        float gain;
        float speed;          // Ansprache (Anteil pro ms, groesser = schneller)
        bool reed;
    };

    inline const StopDef& stop (int s)
    {
        using namespace KeysTables;
        static const StopDef defs[NumStops] {
            { StoppedFlute, { 0.5f },               1, 0.62f, 0.030f, false },  // Bourdon 16'
            { Principal,    { 1.0f },               1, 0.72f, 0.045f, false },  // Prinzipal 8'
            { OpenFlute,    { 1.0f },               1, 0.62f, 0.035f, false },  // Floete 8'
            { String,       { 1.0f },               1, 0.40f, 0.020f, false },  // Gambe 8'
            { Principal,    { 2.0f },               1, 0.52f, 0.050f, false },  // Oktave 4'
            { OpenFlute,    { 2.0f },               1, 0.42f, 0.040f, false },  // Floete 4'
            { Principal,    { 3.0f },               1, 0.30f, 0.055f, false },  // Quinte 2 2/3'
            { Principal,    { 4.0f },               1, 0.36f, 0.060f, false },  // Superoktave 2'
            { Principal,    { 4.0f, 6.0f, 8.0f },   3, 0.22f, 0.070f, false },  // Mixtur III
            { Principal,    { 8.0f, 12.0f, 16.0f }, 3, 0.15f, 0.080f, false },  // Cymbel III
            { Reed,         { 1.0f },               1, 0.46f, 0.110f, true  },  // Trompete 8'
            { Reed,         { 0.5f },               1, 0.50f, 0.090f, true  },  // Bombarde 16'
            { Reed,         { 1.0f, 1.003f },       2, 0.42f, 0.100f, true  },  // Tuba 8'
            { Tibia,        { 1.0f },               1, 0.80f, 0.030f, false },  // Tibia 8' (Theater)
            { String,       { 1.0f, 1.0035f },      2, 0.30f, 0.020f, false },  // Vox Celeste 8' (schwebend)
            { Clarinet,     { 1.0f },               1, 0.40f, 0.090f, true  },  // Krummhorn 8'
        };
        return defs[juce::jlimit (0, NumStops - 1, s)];
    }

    struct World
    {
        std::vector<int> crescendo;   // Register in der Reihenfolge der Walze
        float chiff, wind, bright, detune, reedPower, tremRate, tremDepth;
        bool  pedal32, wellTempered;
    };

    inline const World& world (int w)
    {
        static const std::vector<World> worlds {
            // Paris Notre-Dame – franzoesisch-symphonisch, maechtige Zungen
            { { Floete8, Prinzipal8, Gambe8, Bourdon16, Oktave4, Floete4, Trompete8, Superoktave2, Mixtur, Bombarde16 }, 0.10f, 0.15f, 0.10f, 0.6f, 1.35f, 0, 0, true,  false },
            // Freiberg Silbermann – saechsischer Barock, hell, deutliche Ansprache
            { { Floete8, Prinzipal8, Oktave4, Floete4, Quinte, Superoktave2, Mixtur, Cymbel, Bourdon16, Trompete8 },   0.55f, 0.25f, 0.35f, 0.9f, 0.80f, 0, 0, false, true  },
            // Leipzig Bach-Barock
            { { Prinzipal8, Floete8, Oktave4, Quinte, Superoktave2, Mixtur, Krummhorn8, Bourdon16, Trompete8, Cymbel }, 0.45f, 0.22f, 0.28f, 0.8f, 0.85f, 0, 0, false, true  },
            // Passau Dom – riesig, viele Farben
            { { Floete8, Prinzipal8, Gambe8, Oktave4, Bourdon16, Floete4, Superoktave2, Mixtur, Trompete8, Tuba8, Bombarde16, Cymbel }, 0.18f, 0.12f, 0.12f, 0.7f, 1.1f, 0, 0, true, false },
            // Koeln Dom
            { { Prinzipal8, Floete8, Bourdon16, Oktave4, Gambe8, Superoktave2, Mixtur, Trompete8, Bombarde16, Tuba8 },   0.15f, 0.10f, 0.05f, 0.6f, 1.15f, 0, 0, true,  false },
            // Ulm Muenster – deutsche Romantik, Streicher und Schwebung
            { { Floete8, Gambe8, VoxCeleste8, Prinzipal8, Bourdon16, Oktave4, Floete4, Mixtur, Trompete8, Bombarde16 }, 0.08f, 0.12f, 0.00f, 0.6f, 1.0f, 0, 0, true,  false },
            // Rom St. Peter – italienisches Ripieno, sanfte Prinzipale, keine Zungen
            { { Prinzipal8, Floete8, Oktave4, Quinte, Superoktave2, Mixtur, Cymbel, Bourdon16 },                         0.20f, 0.20f, 0.18f, 0.7f, 0.0f, 0, 0, false, false },
            // London Westminster – englisch, runde Diapasons, Tuba
            { { Floete8, Prinzipal8, Gambe8, VoxCeleste8, Oktave4, Bourdon16, Superoktave2, Tuba8, Mixtur, Trompete8 }, 0.05f, 0.08f, 0.02f, 0.5f, 1.25f, 0, 0, true,  false },
            // Haarlem St. Bavo – niederlaendisch, brillant
            { { Prinzipal8, Floete8, Oktave4, Quinte, Superoktave2, Mixtur, Cymbel, Trompete8, Bourdon16, Bombarde16 }, 0.40f, 0.20f, 0.40f, 0.8f, 1.0f, 0, 0, false, false },
            // Wien Stephansdom
            { { Floete8, Prinzipal8, Gambe8, Oktave4, Bourdon16, Superoktave2, Mixtur, Trompete8, Bombarde16, Cymbel },   0.15f, 0.12f, 0.12f, 0.6f, 1.1f, 0, 0, true,  false },
            // Hamburg Michel
            { { Floete8, Prinzipal8, Gambe8, VoxCeleste8, Oktave4, Bourdon16, Superoktave2, Mixtur, Trompete8, Tuba8, Bombarde16 }, 0.12f, 0.10f, 0.08f, 0.6f, 1.15f, 0, 0, true, false },
            // Riga Dom – romantische Walcker-Orgel, warm
            { { Floete8, Gambe8, VoxCeleste8, Prinzipal8, Floete4, Bourdon16, Oktave4, Mixtur, Trompete8, Tuba8 },       0.06f, 0.10f, -0.10f, 0.6f, 1.0f, 0, 0, true, false },
            // Philadelphia Wanamaker – riesige Konzertorgel, Streicher-Chor
            { { Gambe8, VoxCeleste8, Floete8, Prinzipal8, Tibia8, Bourdon16, Oktave4, Tuba8, Trompete8, Superoktave2, Mixtur, Bombarde16 }, 0.05f, 0.06f, 0.05f, 0.9f, 1.4f, 0, 0, true, false },
            // Theater-Orgel – Tibia, starker Tremulant
            { { Tibia8, VoxCeleste8, Floete4, Gambe8, Trompete8, Krummhorn8, Bourdon16, Superoktave2 },                  0.00f, 0.05f, 0.00f, 1.0f, 1.0f, 6.6f, 0.42f, false, false },
            // Dorfkirche – klein, etwas verstimmt, hoerbarer Wind
            { { Floete8, Prinzipal8, Floete4, Oktave4, Superoktave2, Mixtur },                                            0.35f, 0.38f, 0.00f, 2.2f, 0.0f, 0, 0, false, false },
        };
        return worlds[(size_t) juce::jlimit (0, (int) worlds.size() - 1, w)];
    }

    /** Werckmeister III, Abweichung von gleichstufig in Cent (A = 0) */
    inline float wellTemperedCents (int note) noexcept
    {
        static const float c[12] { 11.73f, 1.955f, 3.91f, 5.865f, 1.955f, 9.775f, 0.0f, 7.82f, 3.91f, 0.0f, 7.82f, 3.91f };
        return c[((note % 12) + 12) % 12];
    }
}

// ---------------------------------------------------------------------------
//  Tonewheel-Zugriegel
namespace Tonewheel
{
    inline constexpr float footage[9] { 0.5f, 1.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f };
    inline const char* registration (int v)
    {
        static const char* r[] { "888000000", "888800000", "888888888", "800000888", "688600000",
                                 "008000000", "838000000", "888630000", "808800008", "006876540" };
        return r[juce::jlimit (0, 9, v)];
    }
}

// ---------------------------------------------------------------------------
//  FM-Patches
struct FmPatch
{
    float rc1, rm1, idx1, mDec1, cDec1;   // Stapel 1: Traeger-Verhaeltnis, Modulator-Verh., Index, Abklingzeiten
    float rc2, rm2, idx2, mDec2, cDec2;   // Stapel 2 (Anschlag / Glanz)
    float w2, feedback, detuneCents, sustain;
};

inline const FmPatch& fmPatch (int model, int variant)
{
    static const FmPatch piano[] {
        { 1, 1, 1.3f, 0.9f, 3.2f,   1, 14, 2.2f, 0.16f, 0.7f,   0.32f, 0.05f, 3.0f, 0.0f },   // 80er Ballade
        { 1, 1, 1.9f, 0.7f, 2.8f,   1, 14, 3.4f, 0.12f, 0.6f,   0.45f, 0.10f, 4.0f, 0.0f },   // Bright Tine
        { 1, 1, 0.8f, 1.2f, 3.6f,   1, 14, 1.0f, 0.20f, 0.6f,   0.18f, 0.00f, 2.5f, 0.0f },   // Soft Keys
        { 1, 1, 2.4f, 0.35f, 2.0f,  1, 17, 4.0f, 0.06f, 0.4f,   0.50f, 0.20f, 3.0f, 0.0f },   // Hard Funk
    };
    static const FmPatch bell[] {
        { 1, 3.5f, 3.0f, 2.5f, 6.0f, 2.0f, 5.19f, 1.6f, 1.2f, 3.0f, 0.35f, 0.0f, 1.5f, 0.0f },  // Roehrenglocken
        { 1, 7.0f, 1.8f, 1.6f, 4.0f, 3.0f, 1.0f, 1.0f, 0.8f, 2.0f,  0.30f, 0.0f, 2.0f, 0.0f },  // Glas
        { 1, 4.0f, 1.4f, 0.25f, 1.6f, 4.0f, 1.0f, 0.6f, 0.2f, 0.6f, 0.25f, 0.0f, 1.0f, 0.0f },  // Celesta
        { 1, 3.5f, 2.6f, 0.05f, 0.7f, 4.0f, 1.0f, 0.5f, 0.05f, 0.3f,0.20f, 0.0f, 0.0f, 0.0f },  // Marimba
        { 1, 4.0f, 0.9f, 0.6f, 3.5f, 1.0f, 1.0f, 0.0f, 0.5f, 3.5f,  0.30f, 0.0f, 0.0f, 0.0f },  // Vibraphon
    };
    static const FmPatch bass[] {
        { 1, 1, 2.4f, 0.45f, 2.0f,  0.5f, 1, 1.2f, 0.30f, 2.0f,  0.45f, 0.35f, 0.0f, 0.70f },  // Solid
        { 1, 1, 4.0f, 0.09f, 1.2f,  1, 3, 2.0f, 0.04f, 0.5f,     0.35f, 0.25f, 0.0f, 0.55f },  // Punch
        { 1, 3, 2.2f, 0.10f, 0.8f,  0.5f, 1, 0.5f, 0.2f, 0.9f,   0.40f, 0.10f, 0.0f, 0.00f },  // Pluck
        { 1, 0.5f, 3.0f, 0.8f, 3.0f, 1, 1, 1.5f, 0.6f, 3.0f,     0.50f, 0.70f, 6.0f, 0.80f },  // Synth Growl
    };
    if (model == ParamChoices::kFmBell) return bell[juce::jlimit (0, 4, variant)];
    if (model == ParamChoices::kFmBass) return bass[juce::jlimit (0, 3, variant)];
    return piano[juce::jlimit (0, 3, variant)];
}

// ---------------------------------------------------------------------------
//  Gemeinsamer Zustand pro Block (vom Engine an die Stimmen)
struct KeysBlock
{
    KeysParams p;
    double sr = 44100.0;
    float pitchMul = 1.0f;          // Pitch-Bend als Faktor
    float vibSemis = 0.0f;          // Vibrato-Tiefe (Halbtoene)
    float vibPhase = 0.0f;          // Phase des Vibrato-LFO (0..1, Blockanfang)
    double vibInc = 0.0;
    float registerAmt = 0.4f;       // Pfeifenorgel: Walze
    float stopGain[PipeOrgan::NumStops] {};
    float organNorm = 1.0f;
    float tremPhase = 0.0f;
    double tremInc = 0.0;
    float aftertouch = 0.0f;
};

// ---------------------------------------------------------------------------
class KeysVoice
{
public:
    static constexpr int ksSize = 8192;

    void prepare (double sampleRate, int seed)
    {
        sr = sampleRate;
        ks.assign ((size_t) ksSize, 0.0f);
        rng.setSeed (seed);
        for (auto& ph : phases) ph = rng.nextDouble();
    }

    bool isActive() const noexcept     { return active; }
    bool isReleasing() const noexcept  { return active && ! gate; }
    int  getNote() const noexcept      { return note; }
    juce::uint64 getAge() const noexcept { return age; }

    void noteOn (int newNote, int playNote, float velocity, juce::uint64 stamp, const KeysBlock& kb, bool percTrigger)
    {
        note = newNote;
        pitchNote = playNote;
        vel = velocity;
        age = stamp;
        gate = true;
        active = true;
        model = kb.p.model;
        variant = kb.p.variant;
        t = 0.0;
        env = 0.0f;
        relGain = 1.0f;
        percOn = percTrigger;
        noiseLp = 0.0f;
        clickEnv = 1.0f;
        pan = 0.0f;
        const float key = (float) playNote;

        switch (model)
        {
            case ParamChoices::kClav:
            {
                std::fill (ks.begin(), ks.end(), 0.0f);
                ksW = 0;
                ksPrev = 0.0f;
                silentRun = 0;
                excLen = juce::jmax (2, (int) ((1.25f - vel * 0.85f) * 0.0011f * (float) sr));
                excPos = 0;
                pan = juce::jlimit (-0.4f, 0.4f, (key - 60.0f) / 60.0f);
                break;
            }
            case ParamChoices::kTine:
            {
                for (auto& ph : phases) ph = 0.0;
                pan = juce::jlimit (-0.35f, 0.35f, (key - 60.0f) / 70.0f);
                break;
            }
            case ParamChoices::kPipe:
            {
                for (auto& r : rankEnv) r = 0.0f;
                windRnd = 0.0f;
                pan = (playNote % 2 == 0 ? -1.0f : 1.0f) * (0.15f + 0.25f * std::abs (key - 60.0f) / 36.0f);
                break;
            }
            case ParamChoices::kFmPiano: case ParamChoices::kFmBell: case ParamChoices::kFmBass:
            {
                for (auto& ph : phases) ph = 0.0;
                fb1 = 0.0f;
                pan = model == ParamChoices::kFmBass ? 0.0f : juce::jlimit (-0.35f, 0.35f, (key - 60.0f) / 70.0f);
                break;
            }
            case ParamChoices::kHarp:
            {
                harpLp1 = harpLp2 = 0.0f;
                f1.reset(); f2.reset();
                scoop = 1.0f;
                break;
            }
            default: break;
        }
    }

    void noteOff() { gate = false; }
    void kill()    { active = false; gate = false; }

    /** Mono-Signal der Stimme in outL/outR (mit Panorama) addieren */
    void render (float* outL, float* outR, int n, const KeysBlock& kb, juce::Random& noise)
    {
        if (! active) return;
        const auto& p = kb.p;
        const float panL = std::cos ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.41421356f;
        const float panR = std::sin ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.41421356f;
        const double baseHz = 440.0 * std::exp2 (((double) pitchNote - 69.0) / 12.0) * kb.pitchMul;
        const float velA = 1.0f - p.vel + p.vel * vel;
        const double dt = 1.0 / sr;

        for (int s = 0; s < n; ++s)
        {
            float vib = 0.0f;
            if (kb.vibSemis > 0.0001f)
                vib = kb.vibSemis * KeysTables::sine (std::fmod ((double) kb.vibPhase + kb.vibInc * s, 1.0));
            const double hz = vib != 0.0f ? baseHz * std::exp2 (vib / 12.0) : baseHz;
            float y = 0.0f;
            switch (model)
            {
                case ParamChoices::kClav:      y = clav (hz, kb, noise); break;
                case ParamChoices::kTine:      y = tine (hz, kb, noise); break;
                case ParamChoices::kTonewheel: y = tonewheel (hz, kb, noise); break;
                case ParamChoices::kPipe:      y = pipe (hz, kb, noise, s); break;
                case ParamChoices::kHarp:      y = harp (hz, kb, noise); break;
                default:                       y = fm (hz, kb); break;
            }
            if (model != ParamChoices::kTonewheel && model != ParamChoices::kPipe) y *= velA;
            outL[s] += y * panL;
            outR[s] += y * panR;
            t += dt;
            if (! active) break;
        }
    }

private:
    // --------------------------------------------------------------- Clavinet
    float clav (double hz, const KeysBlock& kb, juce::Random& noise) noexcept
    {
        const auto& p = kb.p;
        const double period = juce::jlimit (4.0, (double) ksSize - 4.0, sr / hz);
        const float bright = 0.55f + 0.4f * p.tone;
        const double D = period - 0.5 * (1.0 - bright);
        // Abklingzeit: tief laenger, Daempfer (A) kuerzer, beim Loslassen Filz -> sehr kurz
        const float key = (float) pitchNote;
        float t60 = juce::jlimit (0.35f, 5.0f, 3.2f - (key - 36.0f) * 0.035f) * (0.4f + 1.4f * p.decay) * (1.0f - 0.82f * p.a);
        if (! gate) t60 = 0.045f;
        const float g = std::pow (10.0f, -3.0f / (t60 * (float) hz));

        float exc = 0.0f;
        if (excPos < excLen)
        {
            const float w = std::sin (juce::MathConstants<float>::pi * (float) excPos / (float) excLen);
            exc = (w * w * 0.9f + (noise.nextFloat() - 0.5f) * 0.1f) * (0.4f + 0.6f * vel);
            ++excPos;
        }
        const float out = ksRead (D - 1.0);
        const float avg = 0.5f * (out + ksPrev);
        ksPrev = out;
        const float lp = bright * out + (1.0f - bright) * avg;
        ksPush (lp * g + exc);

        // Tonabnehmer: Kammfilter je nach Position (B = Hals <-> Steg)
        const double pick = period * (0.06 + 0.24 * (1.0 - p.b));
        const float y = out - 0.85f * ksRead (pick);

        // Release-"Thunk": kurzer Klick beim Loslassen
        float thunk = 0.0f;
        if (! gate && relClick > 0.0f)
        {
            thunk = (noise.nextFloat() - 0.5f) * relClick * 0.18f;
            relClick *= 0.985f;
        }
        if (gate) relClick = 0.6f;
        if (! gate)
        {
            relGain *= 0.9993f;
            if (relGain < 0.002f) active = false;
        }
        // ausgeklungen: Energie ueber eine Periode verfolgen (nicht an einem Nulldurchgang abschalten)
        silentRun = std::abs (out) < 1.0e-5f ? silentRun + 1 : 0;
        if (t > 0.05 && silentRun > (int) (period * 2.0) + 64) active = false;
        return (y * 0.85f + thunk) * relGain;
    }

    inline float ksRead (double d) const noexcept
    {
        double rp = (double) ksW - 1.0 - d;
        while (rp < 0.0) rp += ksSize;
        const int i0 = (int) rp;
        const float fr = (float) (rp - i0);
        const float a = ks[(size_t) (i0 & (ksSize - 1))];
        const float b = ks[(size_t) ((i0 + 1) & (ksSize - 1))];
        return a + (b - a) * fr;
    }
    inline void ksPush (float x) noexcept { ks[(size_t) ksW] = x; ksW = (ksW + 1) & (ksSize - 1); }

    // --------------------------------------------------------------- Tine Piano
    float tine (double hz, const KeysBlock& kb, juce::Random& noise) noexcept
    {
        const auto& p = kb.p;
        const bool reedPiano = variant == 4;
        const float key = (float) pitchNote;
        float T = juce::jlimit (0.9f, 9.0f, 7.0f - (key - 36.0f) * 0.085f) * (0.35f + 1.3f * p.decay);
        if (reedPiano) T *= 0.45f;
        const double inc = hz / sr;
        // zweistufiges Abklingen: schneller Abfall der ersten 25 %, dann lang
        const float a1 = (0.75f * std::exp (-6.9f * (float) t / T) + 0.25f * std::exp (-6.9f * (float) t / (T * 0.12f)));
        const float atk = juce::jmin (1.0f, (float) t * 600.0f);
        const float bellRatio = reedPiano ? 0.0f : 7.0f + 0.0035f * (key - 60.0f);
        const float bellAmt = reedPiano ? 0.0f : p.b * (0.25f + 0.9f * vel) * std::exp (-6.9f * (float) t / (0.25f + 0.6f * p.decay));
        const float x = KeysTables::sine (phases[0]) * a1 * atk
                      + (bellAmt > 1.0e-4f ? bellAmt * 0.6f * KeysTables::sine (phases[1]) : 0.0f);
        phases[0] += inc; if (phases[0] >= 1.0) phases[0] -= 1.0;
        if (bellRatio > 0.0f)
        {
            phases[1] += inc * bellRatio; if (phases[1] >= 1.0) phases[1] -= std::floor (phases[1]);
        }

        // Pickup: asymmetrisch -> gerade Obertoene ("Bark"), Reed-Piano symmetrisch und haerter
        static const float biasV[5] { 0.32f, 0.22f, 0.40f, 0.12f, 0.0f };
        const float bias = biasV[juce::jlimit (0, 4, variant)];
        const float k = (reedPiano ? 2.2f : 0.9f) + p.a * vel * (reedPiano ? 4.5f : 3.2f);
        const float tb = std::tanh (k * bias);
        const float norm = 1.0f / (k * (1.0f - tb * tb) + 1.0e-3f);
        float y = (std::tanh (k * (x + bias)) - tb) * norm * 0.42f;

        // Hammer-Thump
        if (t < 0.02)
        {
            const float nz = noise.nextFloat() * 2.0f - 1.0f;
            noiseLp += (nz - noiseLp) * 0.08f;
            y += noiseLp * vel * 0.35f * (1.0f - (float) t / 0.02f);
        }
        return releaseEnv (y, 0.12f);
    }

    // --------------------------------------------------------------- Tonewheel
    float tonewheel (double hz, const KeysBlock& kb, juce::Random& noise) noexcept
    {
        const auto& p = kb.p;
        const char* reg = Tonewheel::registration (variant);
        float y = 0.0f;
        const bool perc = p.a > 0.02f;
        for (int d = 0; d < 9; ++d)
        {
            const int step = reg[d] - '0';
            double f = hz * Tonewheel::footage[d];
            while (f > 5900.0) f *= 0.5;            // Foldback oben
            while (f < 32.0) f *= 2.0;              // und unten
            const double inc = f / sr;
            phases[d] += inc; if (phases[d] >= 1.0) phases[d] -= 1.0;
            if (step == 0) continue;
            float g = std::pow (10.0f, -(8 - step) * 3.0f / 20.0f);
            if (d == 8 && perc) g *= 1.0f - p.a;     // B3: 1' schweigt bei Percussion
            y += KeysTables::sine (phases[d]) * g;
        }
        y *= 0.2f;
        // Percussion (3. Harmonische), nur beim ersten Anschlag (Single Trigger)
        {
            double f = hz * 3.0;
            while (f > 5900.0) f *= 0.5;
            phases[9] += f / sr; if (phases[9] >= 1.0) phases[9] -= 1.0;
            if (perc && percOn)
            {
                const float pd = 0.25f + 0.9f * p.decay;
                y += KeysTables::sine (phases[9]) * p.a * 0.4f * std::exp (-6.9f * (float) t / pd);
            }
        }
        // Tastenklick
        if (p.b > 0.01f && clickEnv > 0.001f)
        {
            const float nz = noise.nextFloat() * 2.0f - 1.0f;
            noiseLp += (nz - noiseLp) * 0.5f;
            y += (nz - noiseLp) * clickEnv * p.b * 0.35f;
            clickEnv *= 0.993f;
        }
        // Orgel: sofort an, fast sofort aus
        const float target = gate ? 1.0f : 0.0f;
        env += (target - env) * (gate ? 0.02f : 0.006f);
        if (! gate && env < 0.0005f) active = false;
        return y * env;
    }

    // --------------------------------------------------------------- Pfeifenorgel
    float pipe (double hz, const KeysBlock& kb, juce::Random& noise, int s) noexcept
    {
        const auto& p = kb.p;
        const auto& w = PipeOrgan::world (variant);
        // Wind: langsames Zittern, Tremulant
        windRnd += ((noise.nextFloat() - 0.5f) - windRnd) * 0.0005f;
        float pitchCents = windRnd * w.wind * p.b * 40.0f;
        float trem = 1.0f;
        if (w.tremDepth > 0.0f)
        {
            const float ph = (float) std::fmod ((double) kb.tremPhase + kb.tremInc * s, 1.0);
            const float sv = KeysTables::sine (ph);
            trem = 1.0f + w.tremDepth * 0.6f * sv;
            pitchCents += sv * w.tremDepth * 14.0f;
        }
        if (w.wellTempered) pitchCents += PipeOrgan::wellTemperedCents (pitchNote);
        const double f0 = hz * std::exp2 (pitchCents / 1200.0);
        const float gateT = gate ? 1.0f : 0.0f;
        const float relSpeed = 0.0016f * (1.4f - p.decay);

        float y = 0.0f, chiff = 0.0f;
        int r = 0;
        bool anyOn = false;
        auto addRank = [&] (int kind, double ratio, float gain, float speed, float detuneCents)
        {
            if (r >= maxRanks) return;
            float& e0 = rankEnv[r];
            if (gain <= 1.0e-4f && e0 <= 1.0e-4f) { e0 = 0.0f; ++r; return; }   // Register aus: nichts rechnen
            double f = f0 * ratio * (detuneCents != 0.0f ? (1.0 + detuneCents * 0.000577623) : 1.0);
            while (ratio > 1.0 && f > 7000.0) f *= 0.5;   // Repetition der Mixturen
            const double inc = f / sr;
            phases[r] += inc; if (phases[r] >= 1.0) phases[r] -= std::floor (phases[r]);
            float& e = rankEnv[r];
            e += (gateT - e) * (gate ? speed * 48000.0f / (float) sr : relSpeed * 48000.0f / (float) sr);
            if (e > 1.0e-4f && gain > 1.0e-4f)
            {
                y += KeysTables::get (kind).read (phases[r], KeysTables::Table::levelFor (inc)) * e * gain;
                anyOn = true;
            }
            ++r;
        };

        for (int st = 0; st < PipeOrgan::NumStops; ++st)
        {
            const float g = kb.stopGain[st];
            const auto& sd = PipeOrgan::stop (st);
            for (int k = 0; k < sd.numRanks; ++k)
                addRank (sd.kind, sd.ratios[k], g, sd.speed, (k > 0 ? 0.0f : (float) ((rankDet (st) - 0.5f) * w.detune)));
        }
        // Pedal: unter C2 automatisch 16' (und 32' in grossen Kathedralen)
        if (pitchNote < 48)
        {
            const float pedal = juce::jlimit (0.0f, 1.0f, (48.0f - (float) pitchNote) / 6.0f);
            addRank (KeysTables::StoppedFlute, 0.5, 0.45f * pedal, 0.02f, 0.0f);
            if (w.pedal32 && kb.registerAmt > 0.45f)
                addRank (KeysTables::StoppedFlute, 0.25, 0.4f * pedal * juce::jlimit (0.0f, 1.0f, (kb.registerAmt - 0.45f) * 4.0f), 0.012f, 0.0f);
        }
        // Chiff (Ansprache-Geraeusch) und Windrauschen
        const float chiffAmt = w.chiff * (0.4f + 1.2f * p.b);
        if (chiffAmt > 0.001f && t < 0.06)
        {
            const float nz = noise.nextFloat() * 2.0f - 1.0f;
            noiseLp += (nz - noiseLp) * 0.35f;
            chiff = noiseLp * chiffAmt * 0.25f * (1.0f - (float) t / 0.06f) * gateT;
        }
        const float windN = (noise.nextFloat() - 0.5f) * w.wind * p.b * 0.02f * (anyOn ? 1.0f : 0.0f);
        if (! gate && ! anyOn && t > 0.01) active = false;
        return (y * kb.organNorm + chiff + windN) * trem;
    }

    float rankDet (int st) const noexcept
    {
        // fester Zufallswert je Register und Taste -> lebendige Schwebung
        const juce::uint32 h = (juce::uint32) (st * 7919 + pitchNote * 104729) * 2654435761u;
        return (float) (h >> 8) / 16777216.0f;
    }

    // --------------------------------------------------------------- FM
    float fm (double hz, const KeysBlock& kb) noexcept
    {
        const auto& p = kb.p;
        const auto& fp = fmPatch (model, variant);
        const float key = (float) pitchNote;
        const float keyScale = juce::jlimit (0.4f, 1.6f, 1.0f - (key - 60.0f) * 0.012f);
        const float dmul = (0.35f + 1.3f * p.decay) * keyScale;
        const float idxMul = (0.3f + 1.4f * p.a) * (0.45f + 0.75f * vel) * (0.6f + 0.8f * p.tone);
        const float tt = (float) t;
        const double det = std::exp2 (fp.detuneCents / 1200.0);

        // Stapel 1
        const float i1 = fp.idx1 * idxMul * std::exp (-6.9f * tt / juce::jmax (0.02f, fp.mDec1 * dmul));
        float c1 = std::exp (-6.9f * tt / juce::jmax (0.05f, fp.cDec1 * dmul));
        c1 = fp.sustain + (1.0f - fp.sustain) * c1;
        const float modOut1 = KeysTables::sine (wrap (phases[1] + fb1 * fp.feedback * 0.15));
        fb1 = modOut1;
        const float car1 = KeysTables::sine (wrap (phases[0] + modOut1 * i1 * 0.159f));

        // Stapel 2 (Anschlag, leicht verstimmt)
        const float i2 = fp.idx2 * idxMul * (0.5f + p.b) * std::exp (-6.9f * tt / juce::jmax (0.02f, fp.mDec2 * dmul));
        const float c2 = std::exp (-6.9f * tt / juce::jmax (0.05f, fp.cDec2 * dmul));
        const float modOut2 = KeysTables::sine (phases[3]);
        const float car2 = KeysTables::sine (wrap (phases[2] + modOut2 * i2 * 0.159f));

        const double inc = hz / sr;
        phases[0] = wrap (phases[0] + inc * fp.rc1);
        phases[1] = wrap (phases[1] + inc * fp.rm1);
        phases[2] = wrap (phases[2] + inc * fp.rc2 * det);
        phases[3] = wrap (phases[3] + inc * fp.rm2 * det);

        const float atk = juce::jmin (1.0f, tt * 800.0f);
        float y = (car1 * c1 + car2 * c2 * fp.w2 * (0.4f + 1.2f * p.b)) * atk * 0.5f;
        if (fp.sustain <= 0.0f && c1 < 0.0005f && c2 < 0.0005f) active = false;
        return releaseEnv (y, model == ParamChoices::kFmBass ? 0.06f : 0.25f + 0.6f * p.decay);
    }

    static inline double wrap (double x) noexcept { x -= std::floor (x); return x; }

    // --------------------------------------------------------------- Blues Harp
    float harp (double hz, const KeysBlock& kb, juce::Random& noise) noexcept
    {
        const auto& p = kb.p;
        // Bend-Scoop beim Anblasen (B) + Aftertouch-Vibrato
        scoop *= (float) std::exp (-1.0 / (0.07 * sr));
        const float bendSemis = -2.0f * p.b * scoop;
        const float atVib = kb.aftertouch * 0.35f * KeysTables::sine (wrap ((double) t * 5.5));
        const double f = hz * std::exp2 ((bendSemis + atVib) / 12.0);
        const double inc = f / sr;
        phases[0] = wrap (phases[0] + inc);
        // Durchschlagzunge: abgerundeter Puls (Tabelle "Clarinet" + "Reed" gemischt)
        const int lvl = KeysTables::Table::levelFor (inc);
        float x = 0.6f * KeysTables::get (KeysTables::Reed).read (phases[0], lvl)
                + 0.5f * KeysTables::get (KeysTables::Clarinet).read (phases[0], lvl);
        // Atem
        const float breath = (noise.nextFloat() - 0.5f) * (0.05f + 0.25f * p.a);
        x += breath;
        // Hand-Formanten
        static const float fA[4] { 950.0f, 800.0f, 1100.0f, 620.0f };
        static const float fB[4] { 2600.0f, 2100.0f, 2900.0f, 1800.0f };
        const int v = juce::jlimit (0, 3, variant);
        f1.set (fA[v] * (0.8f + 0.5f * p.tone), 2.2f, sr);
        f2.set (fB[v] * (0.8f + 0.5f * p.tone), 3.0f, sr);
        float y = 0.55f * x + 0.9f * f1.bandpass (x) + 0.45f * f2.bandpass (x);
        if (v == 1)        // Bullet Mic: Bandbreite eng + Verzerrung
        {
            harpLp1 += (y - harpLp1) * 0.25f;
            harpLp2 += (harpLp1 - harpLp2) * 0.04f;
            y = std::tanh ((harpLp1 - harpLp2) * (2.5f + 3.0f * p.a)) * 0.6f;
        }
        else if (p.a > 0.3f) y = std::tanh (y * (1.0f + (p.a - 0.3f) * 4.0f)) / (1.0f + (p.a - 0.3f));
        const float atk = 1.0f - std::exp (-(float) t / 0.018f);
        return releaseEnv (y * atk * 0.55f, 0.07f);
    }

    /** gemeinsames Loslassen fuer die abklingenden Modelle */
    float releaseEnv (float y, float relSeconds) noexcept
    {
        if (! gate)
        {
            relGain *= (float) std::exp (-6.9 / (juce::jmax (0.01f, relSeconds) * sr));
            if (relGain < 0.0008f) active = false;
        }
        return y * relGain;
    }

    static constexpr int maxRanks = 40;
    double sr = 44100.0, t = 0.0;
    double phases[maxRanks] {};
    float rankEnv[maxRanks] {};
    std::vector<float> ks;
    int ksW = 0, excLen = 0, excPos = 0, silentRun = 0;
    float ksPrev = 0.0f, relClick = 0.0f;
    float env = 0.0f, relGain = 1.0f, noiseLp = 0.0f, clickEnv = 1.0f, windRnd = 0.0f, pan = 0.0f;
    float fb1 = 0.0f, scoop = 0.0f, harpLp1 = 0.0f, harpLp2 = 0.0f;
    SVF f1, f2;
    juce::Random rng;
    bool active = false, gate = false, percOn = false;
    int note = 60, pitchNote = 60, model = 0, variant = 0;
    float vel = 1.0f;
    juce::uint64 age = 0;
};

// ---------------------------------------------------------------------------
//  Rotor-Lautsprecher: Horn + Trommel, Doppler + Amplitude, Hochlaufen
class Leslie
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        hornL.prepare ((int) (sr * 0.02)); hornR.prepare ((int) (sr * 0.02));
        drumL.prepare ((int) (sr * 0.02)); drumR.prepare ((int) (sr * 0.02));
        hornL.clear(); hornR.clear(); drumL.clear(); drumR.clear();
        xo.reset();
        hornHz = 0.83f; drumHz = 0.7f;
    }

    void process (float* L, float* R, int n, bool fast, float depth) noexcept
    {
        const float hTarget = fast ? 6.75f : 0.83f, dTarget = fast ? 5.9f : 0.7f;
        const float hCoef = 1.0f - std::exp (-1.0f / (0.45f * (float) sr));
        const float dCoef = 1.0f - std::exp (-1.0f / (2.8f * (float) sr));
        xo.set (800.0f, 0.6f, sr);
        const float dep = 0.4f + 0.6f * depth;
        for (int s = 0; s < n; ++s)
        {
            hornHz += (hTarget - hornHz) * hCoef;
            drumHz += (dTarget - drumHz) * dCoef;
            hPh += hornHz / sr; if (hPh >= 1.0) hPh -= 1.0;
            dPh += drumHz / sr; if (dPh >= 1.0) dPh -= 1.0;
            const float x = 0.5f * (L[s] + R[s]);
            float lo, bp;
            xo.process (x, lo, bp);
            const float hi = x - lo;
            hornL.push (hi); hornR.push (hi);
            drumL.push (lo); drumR.push (lo);
            const float hs = KeysTables::sine (hPh), hc = KeysTables::sine (std::fmod (hPh + 0.25, 1.0));
            const float ds = KeysTables::sine (dPh), dc = KeysTables::sine (std::fmod (dPh + 0.25, 1.0));
            const float base = 0.0018f * (float) sr, hd = 0.00034f * (float) sr * dep, dd = 0.00012f * (float) sr * dep;
            const float hl = hornL.read (base + hd * hs) * (1.0f + 0.38f * dep * hc);
            const float hr = hornR.read (base - hd * hs) * (1.0f - 0.38f * dep * hc);
            const float dl = drumL.read (base + dd * ds) * (1.0f + 0.22f * dep * dc);
            const float dr = drumR.read (base - dd * ds) * (1.0f - 0.22f * dep * dc);
            L[s] = (hl + dl) * 0.9f;
            R[s] = (hr + dr) * 0.9f;
        }
    }

private:
    double sr = 44100.0, hPh = 0.0, dPh = 0.37;
    float hornHz = 0.83f, drumHz = 0.7f;
    DelayLine hornL, hornR, drumL, drumR;
    SVF xo;
};

// ---------------------------------------------------------------------------
//  Talkbox: drei Formant-Filter, Vokal von U -> O -> A -> E -> I verschiebbar
class TalkBox
{
public:
    void prepare (double sampleRate) { sr = sampleRate; for (auto& f : fL) f.reset(); for (auto& f : fR) f.reset(); vowel = 0.5f; envF = 0.0f; lfo = 0.0; }

    /** source: 0 Mod-Rad, 1 Huellkurve, 2 Aftertouch, 3 Auto-LFO */
    void process (float* L, float* R, int n, float mix, int source, float shift, float rate, float wheel, float aftertouch) noexcept
    {
        if (mix <= 0.0001f) { envF = 0.0f; return; }
        static const float F[5][3] { { 300, 870, 2240 }, { 570, 840, 2410 }, { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 } };
        static const float G[3] { 1.0f, 0.7f, 0.35f };
        const float fShift = std::exp2 (shift * 0.5f);
        for (int s0 = 0; s0 < n; s0 += 16)
        {
            const int m = std::min (16, n - s0);
            // Quelle
            float target = 0.5f;
            switch (source)
            {
                case 1:  target = juce::jlimit (0.0f, 1.0f, envF * 3.0f); break;
                case 2:  target = aftertouch; break;
                case 3:  target = 0.5f + 0.5f * KeysTables::sine (lfo); lfo = std::fmod (lfo + rate * m / sr, 1.0); break;
                default: target = wheel; break;
            }
            vowel += (target - vowel) * 0.12f;
            const float pos = juce::jlimit (0.0f, 3.999f, vowel * 4.0f);
            const int i = (int) pos;
            const float fr = pos - (float) i;
            for (int k = 0; k < 3; ++k)
            {
                const float f = (F[i][k] + (F[i + 1][k] - F[i][k]) * fr) * fShift;
                const float q = f / (k == 0 ? 80.0f : (k == 1 ? 100.0f : 130.0f));
                fL[k].set (f, q, sr);
                fR[k].set (f, q, sr);
            }
            for (int s = s0; s < s0 + m; ++s)
            {
                const float xl = L[s], xr = R[s];
                envF += (std::abs (0.5f * (xl + xr)) - envF) * 0.002f;
                float yl = 0.0f, yr = 0.0f;
                for (int k = 0; k < 3; ++k) { yl += fL[k].bandpass (xl) * G[k]; yr += fR[k].bandpass (xr) * G[k]; }
                L[s] = xl * (1.0f - mix) + yl * 1.6f * mix;
                R[s] = xr * (1.0f - mix) + yr * 1.6f * mix;
            }
        }
    }

private:
    double sr = 44100.0, lfo = 0.0;
    float vowel = 0.5f, envF = 0.0f;
    SVF fL[3], fR[3];
};

// ---------------------------------------------------------------------------
class KeysEngine
{
public:
    static constexpr int numVoices = 20;

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        for (int i = 0; i < numVoices; ++i) voices[(size_t) i].prepare (sampleRate, 9001 + i * 131);
        KeysTables::get (0);   // Tabellen vorab bauen (nicht im Audio-Thread)
        KeysTables::sine (0.0);
        bufL.assign ((size_t) std::max (maxBlock, 64), 0.0f);
        bufR.assign ((size_t) std::max (maxBlock, 64), 0.0f);
        leslie.prepare (sampleRate);
        chorus.prepare (sampleRate);
        for (auto& f : toneL) f.reset();
        for (auto& f : toneR) f.reset();
        wahL.reset(); wahR.reset();
        for (auto& a : apL) a = 0.0f;
        for (auto& a : apR) a = 0.0f;
        fxPhase = 0.0; wahEnv = 0.0f; regSmooth = -1.0f;
        allNotesOff (true);
    }

    void setParams (const KeysParams& p)
    {
        if (p.model != params.model || (! p.on && params.on)) allNotesOff (true);
        params = p;
    }

    void allNotesOff (bool hard)
    {
        for (auto& v : voices) { if (hard) v.kill(); else v.noteOff(); }
        pendingOffs.clear();
        held = 0;
    }

    int activeVoices() const { int c = 0; for (auto& v : voices) c += v.isActive() ? 1 : 0; return c; }
    float getAftertouch() const noexcept { return aftertouch; }
    float getWheel() const noexcept      { return wheel; }

    /** Addiert die KEYS-Ebene in L/R. midi wird nur gelesen. */
    void render (float* L, float* R, int total, const juce::MidiBuffer& midi)
    {
        if ((int) bufL.size() < total) { bufL.resize ((size_t) total); bufR.resize ((size_t) total); }
        std::fill (bufL.begin(), bufL.begin() + total, 0.0f);
        std::fill (bufR.begin(), bufR.begin() + total, 0.0f);

        int pos = 0;
        for (const auto meta : midi)
        {
            const int ev = juce::jlimit (0, total, meta.samplePosition);
            renderRange (pos, ev - pos);
            handleMidi (meta.getMessage());
            pos = ev;
        }
        renderRange (pos, total - pos);

        if (! params.on && activeVoices() == 0 && ! tailActive) return;
        tailActive = params.on;
        postChain (total);
        const float g = 0.38f * params.level * (params.wheel == ParamChoices::kwSwell ? (0.15f + 0.85f * wheel) : 1.0f) * (0.15f + 0.85f * expression);
        for (int s = 0; s < total; ++s) { L[s] += bufL[(size_t) s] * g; R[s] += bufR[(size_t) s] * g; }
    }

private:
    void renderRange (int start, int num)
    {
        while (num > 0)
        {
            const int n = std::min (num, 32);
            KeysBlock kb;
            kb.p = params;
            kb.sr = sr;
            kb.aftertouch = aftertouch;
            const float semis = bend >= 0.0f ? bend * params.bendUp : bend * params.bendDown;
            kb.pitchMul = std::exp2 (semis / 12.0f);
            float vib = 0.0f;
            if (params.wheel == ParamChoices::kwVibrato) vib += wheel * 0.5f;
            if (params.model != ParamChoices::kHarp) vib += aftertouch * 0.3f;
            kb.vibSemis = vib;
            kb.vibPhase = (float) vibPhase;
            kb.vibInc = 5.2 / sr;
            vibPhase = std::fmod (vibPhase + 5.2 * n / sr, 1.0);

            if (params.model == ParamChoices::kPipe)
            {
                const float reg = params.wheel == ParamChoices::kwRegister ? wheel : params.a;
                if (regSmooth < 0.0f) regSmooth = reg;
                regSmooth += (reg - regSmooth) * 0.25f;
                kb.registerAmt = regSmooth;
                const auto& w = PipeOrgan::world (params.variant);
                const int count = (int) w.crescendo.size();
                const float steps = 1.0f + regSmooth * (float) (count - 1);   // mind. 1 Register
                float sum = 0.0f;
                for (int k = 0; k < count; ++k)
                {
                    const float on = juce::jlimit (0.0f, 1.0f, steps - (float) k);
                    const int st = w.crescendo[(size_t) k];
                    const auto& sd = PipeOrgan::stop (st);
                    float g = sd.gain * on * (sd.reed ? w.reedPower : 1.0f);
                    if (sd.numRanks > 1 && ! sd.reed && st != PipeOrgan::VoxCeleste8) g *= 1.0f + w.bright;
                    kb.stopGain[st] = g;
                    sum += g * (float) sd.numRanks;
                }
                kb.organNorm = 0.75f / std::sqrt (juce::jmax (0.6f, sum));
                kb.tremPhase = (float) tremPhase;
                kb.tremInc = w.tremRate / sr;
                tremPhase = std::fmod (tremPhase + w.tremRate * n / sr, 1.0);
            }
            lastBlock = kb;
            for (auto& v : voices)
                v.render (bufL.data() + start, bufR.data() + start, n, kb, noiseRng);
            start += n;
            num -= n;
        }
    }

    void postChain (int n)
    {
        float* L = bufL.data();
        float* R = bufR.data();
        const int m = params.model;
        float tone = params.tone;
        if (params.wheel == ParamChoices::kwTone) tone = juce::jlimit (0.0f, 1.0f, tone * 0.4f + wheel);

        // ---- Tonfilter (linear, deshalb auf der Summe)
        if (m == ParamChoices::kClav)
        {
            // Tonschalter: Brilliant / Treble / Medium / Soft / Brilliant+Medium
            static const float hp[5] { 520.0f, 260.0f, 110.0f, 60.0f, 300.0f };
            static const float lp[5] { 14000.0f, 9000.0f, 4500.0f, 1600.0f, 7000.0f };
            const int v = juce::jlimit (0, 4, params.variant);
            toneL[0].set (hp[v], 0.7f, sr); toneR[0].set (hp[v], 0.7f, sr);
            toneL[1].set (lp[v] * (0.6f + 0.8f * tone), 0.8f, sr); toneR[1].set (lp[v] * (0.6f + 0.8f * tone), 0.8f, sr);
            for (int s = 0; s < n; ++s)
            {
                float l, b;
                toneL[0].process (L[s], l, b); const float hl = L[s] - l;
                toneR[0].process (R[s], l, b); const float hr = R[s] - l;
                L[s] = toneL[1].lowpass (hl) * 1.25f;
                R[s] = toneR[1].lowpass (hr) * 1.25f;
            }
        }
        else if (m != ParamChoices::kHarp)
        {
            const float fc = (m == ParamChoices::kPipe ? 2500.0f : 1800.0f) * std::exp2 (tone * 3.6f);
            if (fc < 18000.0f)
            {
                toneL[1].set (fc, 0.707f, sr); toneR[1].set (fc, 0.707f, sr);
                for (int s = 0; s < n; ++s) { L[s] = toneL[1].lowpass (L[s]); R[s] = toneR[1].lowpass (R[s]); }
            }
        }

        // ---- Drive (Roehren-artig, leicht asymmetrisch)
        if (params.drive > 0.001f)
        {
            const float pre = 1.0f + params.drive * 9.0f;
            const float post = 1.0f / std::tanh (pre * 0.6f + 0.2f) * 0.6f;
            for (int s = 0; s < n; ++s)
            {
                L[s] = (std::tanh (L[s] * pre + 0.08f) - 0.0798f) * post;
                R[s] = (std::tanh (R[s] * pre + 0.08f) - 0.0798f) * post;
            }
        }

        // ---- Effekt
        const float amt = params.fxAmt;
        switch (params.fx)
        {
            case ParamChoices::fxPhaser:
            {
                const double rate = 0.08 + params.fxRate * params.fxRate * 5.0;
                for (int s = 0; s < n; ++s)
                {
                    const float lfoL = 0.5f + 0.5f * KeysTables::sine (fxPhase);
                    const float lfoR = 0.5f + 0.5f * KeysTables::sine (std::fmod (fxPhase + 0.25, 1.0));
                    fxPhase += rate / sr; if (fxPhase >= 1.0) fxPhase -= 1.0;
                    L[s] = phaser (L[s], apL, fbL, lfoL, amt);
                    R[s] = phaser (R[s], apR, fbR, lfoR, amt);
                }
                break;
            }
            case ParamChoices::fxTremolo: case ParamChoices::fxAutoPan:
            {
                const double rate = 1.0 + params.fxRate * 8.0;
                const bool panMode = params.fx == ParamChoices::fxAutoPan;
                for (int s = 0; s < n; ++s)
                {
                    const float sv = KeysTables::sine (fxPhase);
                    fxPhase += rate / sr; if (fxPhase >= 1.0) fxPhase -= 1.0;
                    const float gl = 1.0f - amt * 0.5f * (1.0f + sv);
                    const float gr = panMode ? 1.0f - amt * 0.5f * (1.0f - sv) : gl;
                    L[s] *= gl * (panMode ? 1.3f : 1.0f);
                    R[s] *= gr * (panMode ? 1.3f : 1.0f);
                }
                break;
            }
            case ParamChoices::fxChorus:
                chorus.process (L, R, L, R, n, amt, 0.5f, params.fxRate);
                break;
            case ParamChoices::fxWahEnv: case ParamChoices::fxWahWheel:
            {
                const bool envMode = params.fx == ParamChoices::fxWahEnv;
                for (int s0 = 0; s0 < n; s0 += 16)
                {
                    const int mm = std::min (16, n - s0);
                    float pk = 0.0f;
                    for (int s = s0; s < s0 + mm; ++s) pk = juce::jmax (pk, std::abs (L[s]) + std::abs (R[s]));
                    const float coef = pk > wahEnv ? 0.5f : 0.02f + 0.06f * (1.0f - params.fxRate);
                    wahEnv += (pk - wahEnv) * coef;
                    const float ctl = envMode ? juce::jlimit (0.0f, 1.0f, wahEnv * (1.5f + 6.0f * amt)) : wheel;
                    const float fc = 380.0f * std::exp2 (ctl * 2.8f);
                    wahL.set (fc, 4.0f, sr); wahR.set (fc, 4.0f, sr);
                    const float mix = envMode ? 0.85f : 0.4f + 0.5f * amt;
                    for (int s = s0; s < s0 + mm; ++s)
                    {
                        L[s] = L[s] * (1.0f - mix) + wahL.bandpass (L[s]) * 1.8f * mix;
                        R[s] = R[s] * (1.0f - mix) + wahR.bandpass (R[s]) * 1.8f * mix;
                    }
                }
                break;
            }
            case ParamChoices::fxLeslie:
            {
                const bool fast = params.wheel == ParamChoices::kwLeslie ? wheel > 0.5f : params.fxRate > 0.5f;
                leslie.process (L, R, n, fast, amt);
                break;
            }
            default: break;
        }
    }

    static float phaser (float x, float* ap, float& fb, float lfo, float depth) noexcept
    {
        // 6 Allpaesse 1. Ordnung, Koeffizient folgt dem LFO
        const float a = -0.92f + 0.75f * lfo * (0.4f + 0.6f * depth);
        float y = x + fb * 0.45f;
        for (int i = 0; i < 6; ++i)
        {
            const float o = a * y + ap[i];
            ap[i] = y - a * o;
            y = o;
        }
        fb = y;
        return 0.5f * (x + y * (0.4f + 0.6f * depth)) * 1.2f;
    }

    void handleMidi (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())
        {
            const int note = m.getNoteNumber();
            pendingOffs.erase (std::remove (pendingOffs.begin(), pendingOffs.end(), note), pendingOffs.end());
            if (params.on) noteOn (note, m.getFloatVelocity());
        }
        else if (m.isNoteOff())
        {
            if (sustain) pendingOffs.push_back (m.getNoteNumber());
            else         noteOff (m.getNoteNumber());
        }
        else if (m.isPitchWheel())      bend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
        else if (m.isChannelPressure()) aftertouch = (float) m.getChannelPressureValue() / 127.0f;
        else if (m.isAftertouch())      aftertouch = (float) m.getAfterTouchValue() / 127.0f;
        else if (m.isController())
        {
            const int cc = m.getControllerNumber();
            if (cc == 1)       wheel = (float) m.getControllerValue() / 127.0f;
            else if (cc == 11) expression = (float) m.getControllerValue() / 127.0f;
            else if (cc == 64)
            {
                const bool down = m.getControllerValue() >= 64;
                if (sustain && ! down)
                {
                    auto offs = pendingOffs;
                    pendingOffs.clear();
                    sustain = false;
                    for (int nn : offs) noteOff (nn);
                }
                sustain = down;
            }
            else if (cc == 120 || cc == 123) allNotesOff (cc == 120);
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff (m.isAllSoundOff());
    }

    void noteOn (int note, float vel)
    {
        ++stamp;
        KeysVoice* target = nullptr;
        for (auto& v : voices) if (v.isActive() && v.getNote() == note) { target = &v; break; }
        if (! target) for (auto& v : voices) if (! v.isActive()) { target = &v; break; }
        if (! target) for (auto& v : voices) if (v.isReleasing() && (! target || v.getAge() < target->getAge())) target = &v;
        if (! target) for (auto& v : voices) if (! target || v.getAge() < target->getAge()) target = &v;
        const int play = juce::jlimit (0, 127, note + params.octave * 12);
        const bool percTrigger = held == 0;      // Percussion nur, wenn keine Taste gehalten wird
        lastBlock.p = params;                    // immer das aktuelle Modell (auch vor dem ersten Block)
        target->noteOn (note, play, vel, stamp, lastBlock, percTrigger);
        ++held;
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
            if (v.isActive() && ! v.isReleasing() && v.getNote() == note) { v.noteOff(); held = juce::jmax (0, held - 1); }
    }

    double sr = 44100.0;
    KeysParams params;
    KeysBlock lastBlock;
    std::array<KeysVoice, numVoices> voices;
    std::vector<float> bufL, bufR;
    std::vector<int> pendingOffs;
    juce::Random noiseRng { 777 };
    float bend = 0.0f, wheel = 0.0f, aftertouch = 0.0f, expression = 1.0f, wahEnv = 0.0f, regSmooth = -1.0f;
    double vibPhase = 0.0, tremPhase = 0.0, fxPhase = 0.0;
    bool sustain = false, tailActive = false;
    int held = 0;
    juce::uint64 stamp = 0;
    Leslie leslie;
    Ensemble chorus;
    SVF toneL[2], toneR[2], wahL, wahR;
    float apL[6] {}, apR[6] {}, fbL = 0.0f, fbR = 0.0f;
};

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

// ============================================================================
//  K-54 Drum-Computer: 8 analog modellierte Stimmen (alles synthetisch, keine Samples)
//  Kick · Snare · Clap · Closed Hat · Open Hat · Tom Low · Tom High · Cowbell
// ============================================================================

namespace DrumInfo
{
    inline constexpr int numLanes = 8;
    inline const char* names[numLanes]      { "Kick", "Snare", "Clap", "Closed Hat", "Open Hat", "Tom Low", "Tom High", "Cowbell" };
    inline const char* shortNames[numLanes] { "KICK", "SNARE", "CLAP", "CL HAT", "OP HAT", "TOM L", "TOM H", "COWBELL" };
    // General-MIDI-Noten auf Kanal 10
    inline int laneForNote (int note)
    {
        switch (note)
        {
            case 35: case 36:          return 0;
            case 38: case 40:          return 1;
            case 39:                   return 2;
            case 42: case 44:          return 3;
            case 46:                   return 4;
            case 41: case 43: case 45: return 5;
            case 47: case 48: case 50: return 6;
            case 56: case 37:          return 7;
            default:                   return -1;
        }
    }
    inline constexpr int laneNote[numLanes] { 36, 38, 39, 42, 46, 41, 48, 56 };
}

struct DrumLaneParams
{
    float level = 0.8f, tune = 0.0f, decay = 0.5f, tone = 0.5f, pan = 0.0f;
    bool  mute = false;
};

// Zustandsvariablen-Filter (TPT), liefert LP/BP/HP
class DrumSVF
{
public:
    void set (double sr, float fc, float q)
    {
        fc = juce::jlimit (20.0f, (float) (sr * 0.45), fc);
        g = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
        k = 1.0f / juce::jmax (0.1f, q);
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0.0f; }
    void process (float x, float& lp, float& bp, float& hp)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
private:
    float g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;
};

class DrumVoice
{
public:
    void prepare (double sampleRate, int laneIndex, int seed)
    {
        sr = sampleRate; lane = laneIndex; rng.setSeed (seed);
        f1.reset(); f2.reset(); active = false;
    }

    void trigger (float velocity, const DrumLaneParams& p)
    {
        vel = juce::jlimit (0.05f, 1.0f, velocity);
        t = 0.0; active = true; choke = 1.0f;
        prm = p;
        const float tuneMul = std::exp2 (p.tune / 12.0f);
        phase1 = phase2 = 0.0;
        for (auto& ph : hatPhase) ph = rng.nextDouble();
        f1.reset(); f2.reset();

        switch (lane)
        {
            case 0: base = 48.0f * tuneMul; ampDecay = 0.12f + p.decay * 1.3f; break;                 // Kick
            case 1: base = 180.0f * tuneMul; ampDecay = 0.10f + p.decay * 0.45f;                      // Snare
                    f1.set (sr, 1800.0f, 0.7f); f2.set (sr, 7000.0f * juce::jmin (1.4f, tuneMul), 0.6f); break;
            case 2: base = 1150.0f * tuneMul * (0.75f + 0.6f * p.tone); ampDecay = 0.08f + p.decay * 0.5f; // Clap
                    f1.set (sr, base, 1.6f); break;
            case 3: case 4:                                                                             // Hats
                    base = 1.0f * tuneMul;
                    ampDecay = lane == 3 ? 0.025f + p.decay * 0.14f : 0.12f + p.decay * 0.9f;
                    f1.set (sr, 6500.0f + p.tone * 4500.0f, 0.7f); f2.set (sr, 10000.0f, 0.8f); break;
            case 5: base = 95.0f * tuneMul;  ampDecay = 0.15f + p.decay * 0.7f; break;               // Tom low
            case 6: base = 165.0f * tuneMul; ampDecay = 0.12f + p.decay * 0.6f; break;               // Tom high
            case 7: base = 1.0f * tuneMul;   ampDecay = 0.06f + p.decay * 0.5f;                       // Cowbell
                    f1.set (sr, 850.0f * tuneMul + p.tone * 1200.0f, 1.4f); break;
            default: break;
        }
    }

    void chokeNow() { if (active) chokeFast = true; }
    bool isActive() const { return active; }

    // addiert in l/r ab Sample 'start' bis 'end'
    void render (float* l, float* r, int start, int end)
    {
        if (! active) return;
        const float panL = std::cos ((prm.pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        const float panR = std::sin ((prm.pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        const float gain = prm.level * (0.35f + 0.65f * vel) * 0.9f;
        const double dt = 1.0 / sr;

        for (int s = start; s < end; ++s)
        {
            const float x = next ((float) t);
            t += dt;
            if (chokeFast) { choke *= 0.995f; if (choke < 0.001f) { active = false; chokeFast = false; break; } }
            const float y = x * gain * choke;
            l[s] += y * panL;
            r[s] += y * panR;
            if (t > ampDecay * 8.0 + 0.05) { active = false; break; }
        }
    }

private:
    float noise() { return rng.nextFloat() * 2.0f - 1.0f; }
    static float sq (double ph) { return ph - std::floor (ph) < 0.5 ? 1.0f : -1.0f; }

    float next (float tt)
    {
        const float twoPi = juce::MathConstants<float>::twoPi;
        float lp, bp, hp;
        switch (lane)
        {
            case 0:   // Kick: Sinus mit Pitch-Abfall + Klick, leicht gesaettigt
            {
                const float pitchEnv = std::exp (-tt / (0.025f + 0.02f * prm.tone));
                const float f = base * (1.0f + 5.0f * pitchEnv);
                phase1 += f / sr;
                const float amp = std::exp (-tt / ampDecay);
                float y = std::sin (twoPi * (float) phase1) * amp;
                y += noise() * std::exp (-tt / 0.0015f) * 0.35f * (0.3f + prm.tone);
                return std::tanh (y * (1.3f + prm.tone * 1.5f)) * 0.95f;
            }
            case 1:   // Snare: zwei Toene + gefiltertes Rauschen
            {
                phase1 += base / sr;
                phase2 += base * 1.78f / sr;
                const float bodyEnv = std::exp (-tt / (0.06f + prm.decay * 0.08f));
                const float body = (std::sin (twoPi * (float) phase1) + 0.6f * std::sin (twoPi * (float) phase2)) * bodyEnv;
                f1.process (noise(), lp, bp, hp);
                float lp2, bp2, hp2; f2.process (hp, lp2, bp2, hp2);
                const float nz = lp2 * std::exp (-tt / ampDecay) * 1.6f;
                return body * (0.75f - 0.45f * prm.tone) + nz * (0.35f + 0.75f * prm.tone);
            }
            case 2:   // Clap: drei kurze Rauschstoesse + Fahne
            {
                f1.process (noise(), lp, bp, hp);
                float env = 0.0f;
                for (int i = 0; i < 3; ++i)
                {
                    const float ti = tt - 0.011f * (float) i;
                    if (ti >= 0.0f) env = juce::jmax (env, std::exp (-ti / 0.0045f));
                }
                if (tt >= 0.03f) env = juce::jmax (env, 0.7f * std::exp (-(tt - 0.03f) / ampDecay));
                return bp * env * 2.6f;
            }
            case 3: case 4:   // Hats: sechs metallische Rechtecke + Rauschen, hochgefiltert
            {
                static constexpr float hf[6] { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
                float m = 0.0f;
                for (int i = 0; i < 6; ++i)
                {
                    hatPhase[(size_t) i] += hf[i] * 1.7f * base / sr;
                    m += sq (hatPhase[(size_t) i]);
                }
                m = m * 0.16f + noise() * 0.25f;
                f1.process (m, lp, bp, hp);
                float lp2, bp2, hp2; f2.process (hp, lp2, bp2, hp2);
                const float env = std::exp (-tt / ampDecay);
                return (hp * 0.6f + bp2 * 0.8f) * env * 1.4f;
            }
            case 5: case 6:   // Toms
            {
                const float f = base * (1.0f + 0.6f * std::exp (-tt / 0.05f));
                phase1 += f / sr;
                const float amp = std::exp (-tt / ampDecay);
                float y = std::sin (twoPi * (float) phase1) * amp;
                y += noise() * std::exp (-tt / 0.004f) * 0.25f * prm.tone;
                return std::tanh (y * 1.4f) * 0.9f;
            }
            case 7:   // Cowbell: zwei Rechtecke (540/800 Hz) durch Bandpass
            {
                phase1 += 540.0f * base / sr;
                phase2 += 800.0f * base / sr;
                const float m = (sq (phase1) + sq (phase2)) * 0.5f;
                f1.process (m, lp, bp, hp);
                const float env = 0.6f * std::exp (-tt / 0.012f) + 0.4f * std::exp (-tt / ampDecay);
                return bp * env * 1.6f;
            }
            default: return 0.0f;
        }
    }

    double sr = 48000.0, t = 0.0, phase1 = 0.0, phase2 = 0.0;
    std::array<double, 6> hatPhase {};
    int lane = 0;
    float vel = 1.0f, base = 100.0f, ampDecay = 0.3f, choke = 1.0f;
    bool active = false, chokeFast = false;
    DrumLaneParams prm;
    DrumSVF f1, f2;
    juce::Random rng;
};

class DrumMachine
{
public:
    struct Hit { int lane; int offset; float velocity; };

    void prepare (double sampleRate)
    {
        for (int i = 0; i < DrumInfo::numLanes; ++i)
            voices[(size_t) i].prepare (sampleRate, i, 5400 + i * 31);
    }

    /** hits muessen nach offset sortiert sein; outs[lane] = Stereo-Puffer je Stimme (werden NICHT geloescht) */
    void render (const Hit* hits, int numHits, const std::array<DrumLaneParams, DrumInfo::numLanes>& params,
                 std::array<juce::AudioBuffer<float>*, DrumInfo::numLanes>& outs, int numSamples)
    {
        for (int lane = 0; lane < DrumInfo::numLanes; ++lane)
        {
            auto& v = voices[(size_t) lane];
            float* l = outs[(size_t) lane]->getWritePointer (0);
            float* r = outs[(size_t) lane]->getWritePointer (1);
            int pos = 0;
            for (int h = 0; h < numHits; ++h)
            {
                const auto& hit = hits[h];
                const bool chokesMe = (lane == 4 && hit.lane == 3);   // Closed Hat schneidet Open Hat ab
                if (hit.lane != lane && ! chokesMe) continue;
                const int off = juce::jlimit (0, numSamples, hit.offset);
                v.render (l, r, pos, off);
                pos = off;
                if (chokesMe) v.chokeNow();
                else if (! params[(size_t) lane].mute) v.trigger (hit.velocity, params[(size_t) lane]);
            }
            v.render (l, r, pos, numSamples);
        }
    }

private:
    std::array<DrumVoice, DrumInfo::numLanes> voices;
};

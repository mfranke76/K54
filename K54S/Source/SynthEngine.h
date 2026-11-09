#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include "Effects.h"
#include "SoundSources.h"

// ============================================================================
//  K-54 S Klangengine
//  3 Oszillatoren (Analog / Wavetable / Sample) + Sub + Rauschen
//  -> 4-Pol-Ladder-Filter (TPT, 2x) -> VCA -> Chorus -> Delay
// ============================================================================

struct SynthParams
{
    float cutoff = 1200, resonance = 0.25f, envAmount = 0.45f, drive = 0.2f;
    float ampA = 0.005f, ampD = 0.4f, ampS = 0.8f, ampR = 0.25f;
    float fltA = 0.005f, fltD = 0.35f, fltS = 0.2f, fltR = 0.25f;
    float glide = 0, keyTrack = 0.5f, velFilter = 0.2f, velAmp = 0.3f, masterGain = 0.5f;
    int   wave[3]   { 1, 1, 2 };
    int   table[3]  { 0, 0, 0 };
    float pos[3]    { 0, 0, 0 };
    int   octave[3] { 2, 2, 1 };          // Index 0..4 = 32' .. 2'
    float tune[3]   { 0.0f, 0.07f, -0.05f };
    float level[3]  { 0.8f, 0.6f, 0.0f };
    float noise = 0, pulseWidth = 0.25f;
    float lfoRate = 5, lfoPitch = 0, lfoFilter = 0;
    int   lfoWave = 0, voiceMode = 2;     // 0 Mono, 1 Legato, 2 Poly
    float bendRange = 2, drift = 0.3f;

    // POWER
    int   unison = 1;
    float uniDetune = 0.3f, uniSpread = 0.5f, subLevel = 0.0f, heat = 0.0f;
    int   fltType = 0;
    float lfo2Rate = 0.5f;
    int   lfo2Wave = 0;
    static constexpr int slots = ParamID::numModSlots;
    int   modSrc[slots] {}, modDst[slots] {};
    float modAmt[slots] {};
    float modA = 0.005f, modD = 0.3f, modS = 0.0f, modR = 0.3f;
    // Wheels / Spielhilfen
    float bendDown = 2, mwVibrato = 0.4f, mwCutoff = 0, mwPos = 0, atVibrato = 0, atCutoff = 0;
    bool  eco = false;
    float chorusMix = 0, chorusRate = 0.4f, chorusDepth = 0.5f;
    float delayMix = 0, delayBeats = 0.75f, delayFb = 0.35f;

    // Sample
    int   smpRoot = 60, smpLoop = 0;
    float smpStart = 0, smpEnd = 1, smpLoopStart = 0;
    bool  smpKeyTrack = true, smpOneShot = false;
    float smpGain = 1.0f;                       // Sample-Lautstaerke (linear)
    bool  layerOn = true;                       // Synth-Ebene an (KEYS-Seite)
};

namespace LayerMathLite
{
    inline float smallRatio (float semis) noexcept
    {
        const float x = semis * 0.057762265f;
        return 1.0f + x + 0.5f * x * x;
    }
}

namespace ModSrc { enum { Off, Lfo1, Lfo2, FiltEnv, AmpEnv, ModEnv, Velocity, Wheel, Bend, Aftertouch, Key, Random, Breath, Expression }; }
namespace ModDst { enum { Off, Pitch, Osc1Pitch, Osc2Pitch, Osc3Pitch, Cutoff, Emphasis, Drive, PulseWidth, Volume, Pan, Detune, Noise, Sub,
                          Level1, Level2, Level3, Pos1, Pos2, Pos3, PosAll }; }

/** Was die Oszillatoren in diesem Block lesen (vom Prozessor gesetzt) */
struct SourceSet
{
    const Wavetable* tables[ParamChoices::numTables] {};
    const SampleData* sample = nullptr;
    const PianoSet* piano = nullptr;
};

// ---------------------------------------------------------------------------
class Oscillator
{
public:
    void setRandomPhase (juce::Random& r) { phase = r.nextDouble(); }

    float process (double inc, int wave, float pw) noexcept
    {
        const double t  = phase;
        const double dt = std::min (inc, 0.5);
        float out = 0.0f;
        switch (wave)
        {
            case 0: out = (float) (1.0 - 4.0 * std::abs (t - 0.5)); break;
            case 1: out = (float) (2.0 * t - 1.0 - blep (t, dt)); break;
            default:
            {
                const double w = (wave == 2) ? 0.5 : (double) pw;
                double v = t < w ? 1.0 : -1.0;
                v += blep (t, dt);
                v -= blep (std::fmod (t + 1.0 - w, 1.0), dt);
                out = (float) (v - (2.0 * w - 1.0));
                break;
            }
        }
        advance (inc);
        return out;
    }

    float processTable (double inc, const Wavetable& wt, float pos, int level) noexcept
    {
        const float out = wt.read (phase, pos, level);
        advance (inc);
        return out;
    }

    void advance (double inc) noexcept
    {
        phase += inc;
        if (phase >= 1.0) phase -= std::floor (phase);
    }

private:
    static double blep (double t, double dt) noexcept
    {
        if (t < dt)        { t /= dt;               return t + t - t * t - 1.0; }
        if (t > 1.0 - dt)  { t = (t - 1.0) / dt;    return t * t + t + t + 1.0; }
        return 0.0;
    }
    double phase = 0.0;
};

// ---------------------------------------------------------------------------
class LadderFilter
{
public:
    void prepare (double sampleRate) { fs = sampleRate * 2.0; reset(); }
    void reset() { s[0] = s[1] = s[2] = s[3] = 0.0; }

    /** os = 2: zweifach ueberabgetastet (HIGH), 1: einfach (ECO, spart CPU) */
    float process (float x, float fc, float res, float driveGain, int mode = 0, int os = 2) noexcept
    {
        const double fsNow = os == 2 ? fs : fs * 0.5;
        fc = juce::jlimit (16.0f, (float) (fsNow * 0.42), fc);
        const double g = std::tan (juce::MathConstants<double>::pi * fc / fsNow);
        const double G = g / (1.0 + g);
        const double b = 1.0 / (1.0 + g);
        const double k = res * 4.15;
        const double G2 = G * G, G3 = G2 * G, G4 = G3 * G;
        const double comp = 1.0 + 0.45 * k;

        double acc = 0.0;
        for (int i = 0; i < os; ++i)
        {
            const double S = G3 * b * s[0] + G2 * b * s[1] + G * b * s[2] + b * s[3];
            double u = (x * comp - k * S) / (1.0 + k * G4);
            u = std::tanh (u * driveGain) / std::sqrt (driveGain);
            double y = u, st4[4];
            for (int st = 0; st < 4; ++st)
            {
                const double v = (y - s[st]) * G;
                y = v + s[st];
                s[st] = y + v;
                st4[st] = y;
            }
            switch (mode)
            {
                case 1:  acc += st4[1]; break;
                case 2:  acc += 2.0 * (st4[0] - st4[1]); break;
                case 3:  acc += u - 4.0 * st4[0] + 6.0 * st4[1] - 4.0 * st4[2] + st4[3]; break;
                default: acc += st4[3]; break;
            }
        }
        return (float) (acc / os);
    }

private:
    double fs = 88200.0;
    double s[4] {};
};

// ---------------------------------------------------------------------------
class Voice
{
public:
    static constexpr int maxUnison = 7;

    void prepare (double sampleRate, int seed)
    {
        sr = sampleRate;
        filterL.prepare (sampleRate);
        filterR.prepare (sampleRate);
        ampEnv.setSampleRate (sampleRate);
        fltEnv.setSampleRate (sampleRate);
        modEnv.setSampleRate (sampleRate);
        rng.setSeed (seed);
        for (auto& set : osc) for (auto& o : set) o.setRandomPhase (rng);
        subOsc.setRandomPhase (rng);
        for (int u = 0; u < maxUnison; ++u) uniRand[u] = rng.nextFloat() * 2.0f - 1.0f;
    }

    bool  isActive() const noexcept   { return active; }
    bool  isReleasing() const noexcept{ return active && ! gate; }
    int   getNote() const noexcept    { return note; }
    juce::uint64 getAge() const noexcept { return age; }

    void noteOn (int newNote, float vel, bool retrigger, float glideTime, juce::uint64 stamp, const SynthParams& p, const SourceSet& src)
    {
        const bool wasActive = active;
        note = newNote;
        velocity = vel;
        targetPitch = (float) newNote;
        age = stamp;
        randomVal = rng.nextFloat() * 2.0f - 1.0f;
        if (! wasActive || glideTime <= 0.0001f)
            currentPitch = targetPitch;

        updateEnvelopes (p);
        if (retrigger || ! wasActive)
        {
            ampEnv.noteOn();
            fltEnv.noteOn();
            modEnv.noteOn();
            restartSample (p, src, newNote);
        }
        if (! wasActive)
        {
            filterL.reset();
            filterR.reset();
        }
        active = true;
        gate = true;
        deferredOff = false;
    }

    void setPitchOnly (int newNote) { note = newNote; targetPitch = (float) newNote; }

    void noteOff()
    {
        gate = false;
        if (oneShot && ! allSamplesDone()) { deferredOff = true; return; }   // One-Shot: Sample spielt zu Ende
        ampEnv.noteOff();
        fltEnv.noteOff();
        modEnv.noteOff();
    }

    void kill() { active = false; gate = false; deferredOff = false; ampEnv.reset(); fltEnv.reset(); modEnv.reset(); filterL.reset(); filterR.reset(); }

    void updateEnvelopes (const SynthParams& p)
    {
        ampEnv.setParameters ({ p.ampA, p.ampD, p.ampS, p.ampR });
        fltEnv.setParameters ({ p.fltA, p.fltD, p.fltS, p.fltR });
        modEnv.setParameters ({ p.modA, p.modD, p.modS, p.modR });
    }

    struct BlockMods
    {
        const float* lfo1 = nullptr;
        const float* lfo2 = nullptr;
        float wheel = 0.0f, aftertouch = 0.0f, bend = 0.0f, breath = 0.0f, expression = 1.0f;
    };

    void render (float* outL, float* outR, int n, const SynthParams& p, const BlockMods& bm, const SourceSet& src,
                 float baseCutoff, float pitchMod, float wheelVib, juce::Random& noiseRng)
    {
        if (! active) return;
        updateEnvelopes (p);

        const float glideCoef = p.glide <= 0.0001f ? 1.0f : 1.0f - std::exp (-1.0f / (p.glide * 0.35f * (float) sr));
        const float velA = 1.0f - p.velAmp + p.velAmp * velocity;
        const float envDepthOct = p.envAmount * 6.0f * (1.0f - p.velFilter + p.velFilter * velocity);
        const float keyOct = p.keyTrack * ((float) note - 60.0f) / 12.0f;
        juce::ignoreUnused (wheelVib);
        // Wheels: Modrad/Aftertouch direkt auf Vibrato, Filter und Wavetable-Position
        const float vibSemis = p.lfoPitch * p.lfoPitch * 12.0f + bm.wheel * p.mwVibrato + bm.aftertouch * p.atVibrato;
        const float lfoFiltOct = p.lfoFilter * 3.0f;
        const float wheelOct = (bm.wheel * p.mwCutoff + bm.aftertouch * p.atCutoff) * 4.0f;
        const float wheelPos = bm.wheel * p.mwPos;
        const int os = p.eco ? 1 : 2;

        const int U = juce::jlimit (1, maxUnison, p.unison);
        const float uniGain = 1.0f / std::sqrt ((float) U);

        // Welche Quelle hat jeder Oszillator in diesem Block?
        const Wavetable* wt[3] {};
        const SampleData* smp = src.sample != nullptr && src.sample->length() > 8 ? src.sample : nullptr;
        bool usesSample = false;
        for (int i = 0; i < 3; ++i)
        {
            if (p.wave[i] == ParamChoices::wTable)
                wt[i] = src.tables[juce::jlimit (0, ParamChoices::numTables - 1, p.table[i])];
            if (p.wave[i] == ParamChoices::wSample && smp != nullptr && smp->isStereo()) usesSample = true;
            if (p.wave[i] == ParamChoices::wPiano && pZone[i] != nullptr) usesSample = true;
        }
        const bool stereo = (U > 1 && p.uniSpread > 0.001f) || usesSample;   // Stereo-Quelle oder Unison-Breite

        // Sample-Bereich (in Samples des Originals)
        double sStart = 0, sEnd = 1, sLoop = 0, srcRatio = 1;
        if (smp != nullptr)
        {
            const double len = (double) smp->length() - 2.0;
            sStart = juce::jlimit (0.0, len - 16.0, (double) p.smpStart * len);
            sEnd   = juce::jlimit (sStart + 16.0, len, (double) p.smpEnd * len);
            sLoop  = sStart + (double) juce::jlimit (0.0f, 0.98f, p.smpLoopStart) * (sEnd - sStart);
            srcRatio = smp->sampleRate / sr;
        }

        // nur belegte Matrix-Plaetze rechnen (spart CPU)
        int act[SynthParams::slots], numAct = 0;
        for (int k = 0; k < SynthParams::slots; ++k)
            if (p.modSrc[k] != ModSrc::Off && p.modDst[k] != ModDst::Off && std::abs (p.modAmt[k]) > 0.0001f) act[numAct++] = k;
        const bool anyMod = numAct > 0;
        const float keyVal = ((float) note - 60.0f) / 48.0f;

        for (int s = 0; s < n; ++s)
        {
            currentPitch += (targetPitch - currentPitch) * glideCoef;
            for (int i = 0; i < 3; ++i)
            {
                driftState[i] += (rng.nextFloat() - 0.5f) * 0.004f;
                driftState[i] *= 0.9997f;
            }

            const float fe = fltEnv.getNextSample();
            const float ae = ampEnv.getNextSample();
            const float me = modEnv.getNextSample();
            const float l1 = bm.lfo1[s];
            const float l2 = bm.lfo2[s];

            float mPitch = 0, mO1 = 0, mO2 = 0, mO3 = 0, mCut = 0, mRes = 0, mDrv = 0, mPw = 0, mVol = 0, mPan = 0, mDet = 0, mNoise = 0, mSub = 0;
            float mPos[3] { wheelPos, wheelPos, wheelPos };
            float mLvl[3] { 0, 0, 0 };
            if (anyMod)
            {
                for (int a = 0; a < numAct; ++a)
                {
                    const int k = act[a];
                    float sv = 0.0f;
                    switch (p.modSrc[k])
                    {
                        case ModSrc::Lfo1:       sv = l1; break;
                        case ModSrc::Lfo2:       sv = l2; break;
                        case ModSrc::FiltEnv:    sv = fe; break;
                        case ModSrc::AmpEnv:     sv = ae; break;
                        case ModSrc::ModEnv:     sv = me; break;
                        case ModSrc::Velocity:   sv = velocity; break;
                        case ModSrc::Wheel:      sv = bm.wheel; break;
                        case ModSrc::Bend:       sv = bm.bend; break;
                        case ModSrc::Aftertouch: sv = bm.aftertouch; break;
                        case ModSrc::Key:        sv = keyVal; break;
                        case ModSrc::Random:     sv = randomVal; break;
                        case ModSrc::Breath:     sv = bm.breath; break;
                        case ModSrc::Expression: sv = bm.expression; break;
                        default: break;
                    }
                    const float v = sv * p.modAmt[k];
                    switch (p.modDst[k])
                    {
                        case ModDst::Pitch:      mPitch += v * 12.0f; break;
                        case ModDst::Osc1Pitch:  mO1 += v * 12.0f; break;
                        case ModDst::Osc2Pitch:  mO2 += v * 12.0f; break;
                        case ModDst::Osc3Pitch:  mO3 += v * 12.0f; break;
                        case ModDst::Cutoff:     mCut += v * 5.0f; break;
                        case ModDst::Emphasis:   mRes += v; break;
                        case ModDst::Drive:      mDrv += v; break;
                        case ModDst::Level1:     mLvl[0] += v; break;
                        case ModDst::Level2:     mLvl[1] += v; break;
                        case ModDst::Level3:     mLvl[2] += v; break;
                        case ModDst::PulseWidth: mPw += v * 0.45f; break;
                        case ModDst::Volume:     mVol += v; break;
                        case ModDst::Pan:        mPan += v; break;
                        case ModDst::Detune:     mDet += v; break;
                        case ModDst::Noise:      mNoise += v; break;
                        case ModDst::Sub:        mSub += v; break;
                        case ModDst::Pos1:       mPos[0] += v; break;
                        case ModDst::Pos2:       mPos[1] += v; break;
                        case ModDst::Pos3:       mPos[2] += v; break;
                        case ModDst::PosAll:     mPos[0] += v; mPos[1] += v; mPos[2] += v; break;
                        default: break;
                    }
                }
            }

            const double modSemis  = pitchMod + l1 * vibSemis + mPitch;          // alles ausser der Taste
            const double basePitch = currentPitch + modSemis;
            const double baseFreq  = 440.0 * std::exp2 ((basePitch - 69.0) / 12.0);
            const float pw = juce::jlimit (0.03f, 0.97f, p.pulseWidth + mPw);

            const float drv = juce::jlimit (0.0f, 1.0f, p.drive + mDrv);
            const float driveGain = 1.0f + drv * 5.0f;
            const float inGain = 0.42f * (1.0f + drv * 1.5f);
            float lvl[3];
            for (int i = 0; i < 3; ++i) lvl[i] = juce::jlimit (0.0f, 1.5f, p.level[i] + mLvl[i]);

            double ratio[3];
            float posNow[3];
            int level[3];
            double smpRatio[3];
            for (int i = 0; i < 3; ++i)
            {
                const double extra = (i == 0 ? mO1 : (i == 1 ? mO2 : mO3));
                const double semi = (double) (p.octave[i] - 2) * 12.0 + p.tune[i] + extra + driftState[i] * p.drift * 0.6;
                ratio[i] = std::exp2 (semi / 12.0);
                posNow[i] = juce::jlimit (0.0f, 1.0f, p.pos[i] + mPos[i]);
                level[i] = wt[i] != nullptr ? Wavetable::levelFor (baseFreq * ratio[i] / sr) : 0;
                smpRatio[i] = 1.0;
                if (p.wave[i] == ParamChoices::wSample)
                {
                    const double fromRoot = p.smpKeyTrack ? (basePitch - (double) p.smpRoot) : modSemis;
                    smpRatio[i] = std::exp2 ((fromRoot + semi) / 12.0) * srcRatio;
                }
                else if (p.wave[i] == ParamChoices::wPiano && pZone[i] != nullptr)
                    smpRatio[i] = std::exp2 ((basePitch + semi - (double) pZone[i]->root) / 12.0) * pZone[i]->sampleRate / sr;
            }

            const float det = std::pow (juce::jlimit (0.0f, 1.0f, p.uniDetune + mDet), 1.5f) * 0.7f;
            float mixL = 0.0f, mixR = 0.0f;
            for (int u = 0; u < U; ++u)
            {
                const float upos = U == 1 ? 0.0f : ((float) u / (float) (U - 1)) * 2.0f - 1.0f;
                const double uRatio = U == 1 ? 1.0 : (double) LayerMathLite::smallRatio (upos * det + uniRand[u] * det * 0.15f);
                float suL = 0.0f, suR = 0.0f;
                for (int i = 0; i < 3; ++i)
                {
                    const double inc = baseFreq * ratio[i] * uRatio / sr;
                    auto& o = osc[(size_t) u][(size_t) i];
                    if (lvl[i] <= 0.0001f) { if (u == 0) o.advance (inc); continue; }
                    const int w = p.wave[i];
                    if (w == ParamChoices::wSample || w == ParamChoices::wPiano)
                    {
                        float a = 0.0f, b = 0.0f;
                        if (w == ParamChoices::wSample) { if (smp == nullptr) continue; sampleTick (u, i, smpRatio[i] * uRatio, *smp, p.smpLoop, sStart, sEnd, sLoop, a, b); a *= p.smpGain; b *= p.smpGain; }
                        else { if (pZone[i] == nullptr) continue; sampleTick (u, i, smpRatio[i] * uRatio, *pZone[i], 0, 0.0, (double) pZone[i]->length() - 3.0, 0.0, a, b); }
                        suL += a * lvl[i];
                        suR += b * lvl[i];
                        continue;
                    }
                    float v;
                    if (w == ParamChoices::wTable)
                        v = wt[i] != nullptr ? o.processTable (inc, *wt[i], posNow[i], level[i]) * 0.8f : (o.advance (inc), 0.0f);
                    else
                        v = o.process (inc, w, pw);
                    suL += v * lvl[i];
                    suR += v * lvl[i];
                }
                if (U > 1 && p.uniSpread > 0.001f)
                {
                    const float sp = upos * p.uniSpread;
                    mixL += suL * (1.0f - sp);
                    mixR += suR * (1.0f + sp);
                }
                else
                {
                    mixL += suL;
                    mixR += suR;
                }
            }
            mixL *= uniGain;
            mixR *= uniGain;

            float center = 0.0f;
            const float subLvl = juce::jlimit (0.0f, 1.0f, p.subLevel + mSub);
            const double subInc = baseFreq * ratio[0] * 0.5 / sr;
            if (subLvl > 0.0001f) center += subOsc.process (subInc, 2, 0.5f) * subLvl * 0.9f;
            else                  subOsc.advance (subInc);
            const float noiseLvl = juce::jlimit (0.0f, 1.0f, p.noise + mNoise);
            if (noiseLvl > 0.0001f)
                center += (noiseRng.nextFloat() * 2.0f - 1.0f) * noiseLvl * 0.8f;

            const float fc  = baseCutoff * std::exp2 (fe * envDepthOct + keyOct + l1 * lfoFiltOct + mCut + wheelOct);
            const float res = juce::jlimit (0.0f, 1.0f, p.resonance + mRes);

            float yL, yR;
            if (stereo)
            {
                yL = filterL.process ((mixL + center) * inGain, fc, res, driveGain, p.fltType, os);
                yR = filterR.process ((mixR + center) * inGain, fc, res, driveGain, p.fltType, os);
            }
            else
                yL = yR = filterL.process ((0.5f * (mixL + mixR) + center) * inGain, fc, res, driveGain, p.fltType, os);

            float g = ae * velA;
            if (mVol != 0.0f) g *= juce::jmax (0.0f, 1.0f + mVol);
            if (mPan != 0.0f)
            {
                const float pan = juce::jlimit (-1.0f, 1.0f, mPan);
                const float a = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
                outL[s] += yL * g * std::cos (a) * 1.41421356f;
                outR[s] += yR * g * std::sin (a) * 1.41421356f;
            }
            else
            {
                outL[s] += yL * g;
                outR[s] += yR * g;
            }

            // One-Shot: erst loslassen, wenn das Sample fertig ist
            if (deferredOff && allSamplesDone())
            {
                deferredOff = false;
                ampEnv.noteOff();
                fltEnv.noteOff();
                modEnv.noteOff();
            }
        }

        if (! ampEnv.isActive())
            kill();
    }

private:
    void restartSample (const SynthParams& p, const SourceSet& src, int noteNum)
    {
        oneShot = false;
        bool anyPiano = false;
        for (int i = 0; i < 3; ++i)
        {
            pZone[i] = nullptr;
            if (p.wave[i] == ParamChoices::wPiano && src.piano != nullptr && src.piano->isReady())
            {
                pZone[i] = src.piano->zoneFor (noteNum + (p.octave[i] - 2) * 12 + juce::roundToInt (p.tune[i]));
                anyPiano |= pZone[i] != nullptr && p.level[i] > 0.0001f;
            }
        }
        const SampleData* smp = src.sample;
        double start = 0.0;
        if (smp != nullptr && smp->length() > 8)
            start = juce::jlimit (0.0, (double) smp->length() - 18.0, (double) p.smpStart * (smp->length() - 2));
        bool any = false;
        for (int i = 0; i < 3; ++i)
        {
            const bool isPiano = pZone[i] != nullptr;
            const bool on = (p.wave[i] == ParamChoices::wSample && p.level[i] > 0.0001f && smp != nullptr) || isPiano;
            any |= on && ! isPiano;
            for (int u = 0; u < maxUnison; ++u)
            {
                sPos[u][i] = isPiano ? 0.0 : start;
                sDir[u][i] = 1;
                sDone[u][i] = ! on;
            }
        }
        usedSlots = any || anyPiano;
        oneShot = any && p.smpOneShot && p.smpLoop == 0;
    }

    bool allSamplesDone() const noexcept
    {
        if (! usedSlots) return true;
        for (int i = 0; i < 3; ++i)
            if (! sDone[0][i]) return false;
        return true;
    }

    template <typename Src>
    void sampleTick (int u, int i, double inc, const Src& smp, int loopMode, double start, double end, double loop, float& l, float& r) noexcept
    {
        if (sDone[u][i]) return;
        double& pos = sPos[u][i];
        if (pos < start - 1.0 || pos > end + 1.0) pos = start;        // Sample wurde gewechselt
        l = smp.read (0, pos);
        r = smp.isStereo() ? smp.read (1, pos) : l;

        // Ein-/Ausblenden am Ende (kein Knacken)
        if (loopMode == 0)
        {
            const double left = end - pos;
            if (left < 64.0) { const float f = (float) juce::jmax (0.0, left / 64.0); l *= f; r *= f; }
        }

        pos += inc * sDir[u][i];
        if (loopMode == 0)
        {
            if (pos >= end) sDone[u][i] = true;
        }
        else if (loopMode == 1)
        {
            if (pos >= end) pos = loop + std::fmod (pos - end, juce::jmax (1.0, end - loop));
        }
        else
        {
            if (sDir[u][i] > 0 && pos >= end)  { pos = end - (pos - end); sDir[u][i] = -1; }
            if (sDir[u][i] < 0 && pos <= loop) { pos = loop + (loop - pos); sDir[u][i] = 1; }
        }
    }

    double sr = 44100.0;
    std::array<std::array<Oscillator, 3>, maxUnison> osc;
    Oscillator subOsc;
    LadderFilter filterL, filterR;
    juce::ADSR ampEnv, fltEnv, modEnv;
    const PianoZone* pZone[3] {};
    juce::Random rng;
    float driftState[3] {};
    float uniRand[maxUnison] {};
    double sPos[maxUnison][3] {};
    int sDir[maxUnison][3] {};
    bool sDone[maxUnison][3] {};
    bool usedSlots = false, oneShot = false, deferredOff = false;
    bool active = false, gate = false;
    int note = 60;
    float velocity = 1.0f, randomVal = 0.0f;
    float currentPitch = 60.0f, targetPitch = 60.0f;
    juce::uint64 age = 0;
};

// ---------------------------------------------------------------------------
class SynthEngine
{
public:
    static constexpr int numVoices = 16;

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        for (int i = 0; i < numVoices; ++i)
            voices[(size_t) i].prepare (sampleRate, 5501 + i * 97);
        lfoBuf.assign ((size_t) std::max (maxBlock, 64), 0.0f);
        lfo2Buf.assign ((size_t) std::max (maxBlock, 64), 0.0f);
        cutoffSmooth.reset (sampleRate, 0.03);
        cutoffSmooth.setCurrentAndTargetValue (params.cutoff);
        allNotesOff (true);
        for (auto& d : dc) d = {};
        chorus.prepare (sampleRate);
        delay.prepare (sampleRate);
    }

    void setParams (const SynthParams& p)
    {
        if (p.voiceMode != params.voiceMode) { params = p; allNotesOff (true); }
        params = p;
        cutoffSmooth.setTargetValue (p.cutoff);
    }
    void setSources (const SourceSet& s) { sources = s; }
    void setTempo (double bpm) { tempo = juce::jlimit (20.0, 400.0, bpm); }

    void allNotesOff (bool hard)
    {
        for (auto& v : voices) { if (hard) v.kill(); else v.noteOff(); }
        noteStack.clear();
        pendingOffs.clear();
    }

    int activeVoices() const
    {
        int c = 0;
        for (auto& v : voices) c += v.isActive() ? 1 : 0;
        return c;
    }

    void render (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        const int total = buffer.getNumSamples();
        buffer.clear();
        if ((int) lfoBuf.size() < total)  lfoBuf.resize ((size_t) total);
        if ((int) lfo2Buf.size() < total) lfo2Buf.resize ((size_t) total);

        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);

        int pos = 0;
        for (const auto meta : midi)
        {
            const int evPos = juce::jlimit (0, total, meta.samplePosition);
            renderRange (L, R, pos, evPos - pos);
            handleMidi (meta.getMessage());
            pos = evPos;
        }
        renderRange (L, R, pos, total - pos);

        chorus.process (L, R, L, R, total, params.chorusMix, params.chorusDepth, params.chorusRate);
        delay.process (L, R, total, (float) (params.delayBeats * 60.0 / tempo), params.delayFb, params.delayMix);
    }

private:
    static float lfoShape (int wave, double phase, float sh) noexcept
    {
        switch (wave)
        {
            case 0:  return (float) (1.0 - 4.0 * std::abs (phase - 0.5));
            case 1:  return (float) (1.0 - 2.0 * phase);
            case 2:  return phase < 0.5 ? 1.0f : -1.0f;
            default: return sh;
        }
    }

    void renderRange (float* L, float* R, int start, int num)
    {
        while (num > 0)
        {
            const int n = std::min (num, 32);
            float* outL = L + start;
            float* outR = R + start;

            const double inc1 = params.lfoRate / sr, inc2 = params.lfo2Rate / sr;
            for (int s = 0; s < n; ++s)
            {
                lfoBuf[(size_t) s] = lfoShape (params.lfoWave, lfoPhase, shValue);
                lfoPhase += inc1;
                if (lfoPhase >= 1.0) { lfoPhase -= 1.0; shValue = noiseRng.nextFloat() * 2.0f - 1.0f; }
                lfo2Buf[(size_t) s] = lfoShape (params.lfo2Wave, lfo2Phase, sh2Value);
                lfo2Phase += inc2;
                if (lfo2Phase >= 1.0) { lfo2Phase -= 1.0; sh2Value = noiseRng.nextFloat() * 2.0f - 1.0f; }
            }

            const float baseCutoff = cutoffSmooth.getNextValue();
            cutoffSmooth.skip (n - 1);
            const float wheelVib = 0.0f;
            const float pitchMod = bend >= 0.0f ? bend * params.bendRange : bend * params.bendDown;

            Voice::BlockMods bm;
            bm.lfo1 = lfoBuf.data();
            bm.lfo2 = lfo2Buf.data();
            bm.wheel = modWheel;
            bm.aftertouch = aftertouch;
            bm.bend = bend;
            bm.breath = breath;
            bm.expression = expression;

            for (auto& v : voices)
                v.render (outL, outR, n, params, bm, sources, baseCutoff, pitchMod, wheelVib, noiseRng);

            // DC-Blocker, HEAT-Saettigung, Lautstaerke
            const float pre  = params.masterGain * 1.6f * (1.0f + params.heat * 3.5f);
            const float post = 0.85f / (1.0f + params.heat * 0.9f);
            float* ch[2] { outL, outR };
            for (int c = 0; c < 2; ++c)
            {
                auto& d = dc[c];
                for (int s = 0; s < n; ++s)
                {
                    const float x = ch[c][s];
                    d.y = x - d.x + 0.9995f * d.y;
                    d.x = x;
                    ch[c][s] = std::tanh (d.y * pre) * post;
                }
            }
            start += n;
            num -= n;
        }
    }

    void handleMidi (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())
        {
            const int note = m.getNoteNumber();
            pendingOffs.erase (std::remove (pendingOffs.begin(), pendingOffs.end(), note), pendingOffs.end());
            noteOn (note, m.getFloatVelocity());
        }
        else if (m.isNoteOff())
        {
            if (sustain) pendingOffs.push_back (m.getNoteNumber());
            else         noteOff (m.getNoteNumber());
        }
        else if (m.isPitchWheel())
            bend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
        else if (m.isChannelPressure())
            aftertouch = (float) m.getChannelPressureValue() / 127.0f;
        else if (m.isAftertouch())
            aftertouch = (float) m.getAfterTouchValue() / 127.0f;
        else if (m.isController())
        {
            if (m.getControllerNumber() == 1)
                modWheel = (float) m.getControllerValue() / 127.0f;
            else if (m.getControllerNumber() == 2)
                breath = (float) m.getControllerValue() / 127.0f;
            else if (m.getControllerNumber() == 11)
                expression = (float) m.getControllerValue() / 127.0f;
            else if (m.getControllerNumber() == 64)
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
            else if (m.getControllerNumber() == 123 || m.getControllerNumber() == 120)
                allNotesOff (m.getControllerNumber() == 120);
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            allNotesOff (m.isAllSoundOff());
    }

    void noteOn (int note, float vel)
    {
        if (! params.layerOn) { lastNote = note; return; }   // Synth-Ebene aus: nur die KEYS spielen
        ++stamp;
        if (params.voiceMode == 2)
        {
            Voice* target = nullptr;
            for (auto& v : voices) if (v.isActive() && v.getNote() == note) { target = &v; break; }
            if (! target) for (auto& v : voices) if (! v.isActive()) { target = &v; break; }
            if (! target)
            {
                for (auto& v : voices)
                    if (v.isReleasing() && (! target || v.getAge() < target->getAge())) target = &v;
                if (! target)
                    for (auto& v : voices)
                        if (! target || v.getAge() < target->getAge()) target = &v;
            }
            if (! target->isActive() && params.glide > 0.0001f)
            {
                target->noteOn (lastNote, vel, true, 0.0f, stamp, params, sources);
                target->setPitchOnly (note);
            }
            else
                target->noteOn (note, vel, true, params.glide, stamp, params, sources);
        }
        else
        {
            const bool hadNotes = ! noteStack.empty();
            noteStack.erase (std::remove (noteStack.begin(), noteStack.end(), note), noteStack.end());
            noteStack.push_back (note);
            auto& v = voices[0];
            const bool retrig = (params.voiceMode == 0) || ! hadNotes || ! v.isActive();
            v.noteOn (note, vel, retrig, params.glide, stamp, params, sources);
        }
        lastNote = note;
    }

    void noteOff (int note)
    {
        if (params.voiceMode == 2)
        {
            for (auto& v : voices)
                if (v.isActive() && ! v.isReleasing() && v.getNote() == note) v.noteOff();
        }
        else
        {
            noteStack.erase (std::remove (noteStack.begin(), noteStack.end(), note), noteStack.end());
            auto& v = voices[0];
            if (noteStack.empty())          v.noteOff();
            else if (v.getNote() == note)   v.setPitchOnly (noteStack.back());
        }
    }

    double sr = 44100.0, tempo = 120.0;
    SynthParams params;
    SourceSet sources;
    std::array<Voice, numVoices> voices;
    std::vector<float> lfoBuf, lfo2Buf;
    std::vector<int> noteStack, pendingOffs;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> cutoffSmooth { 1200.0f };
    juce::Random noiseRng { 55 };
    double lfoPhase = 0.0, lfo2Phase = 0.0;
    float shValue = 0.0f, sh2Value = 0.0f, bend = 0.0f, modWheel = 0.0f, aftertouch = 0.0f, breath = 0.0f, expression = 1.0f;
public:
    /** Fuer die Wheel-Anzeige */
    float getBend() const noexcept { return bend; }
    float getModWheel() const noexcept { return modWheel; }
private:
    bool sustain = false;
    int lastNote = 60;
    juce::uint64 stamp = 0;
    struct DcState { float x = 0.0f, y = 0.0f; };
    DcState dc[2];
    Ensemble chorus;
    StereoDelay delay;
};

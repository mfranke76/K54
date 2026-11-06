#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>
#include <algorithm>

// ============================================================================
//  K-54 Effekte: Verzoegerungsleitung, Ensemble/Chorus, Stereo-Delay, Filter
// ============================================================================

class DelayLine
{
public:
    void prepare (int maxSamples)
    {
        buf.assign ((size_t) juce::nextPowerOfTwo (maxSamples + 4), 0.0f);
        mask = (int) buf.size() - 1;
        w = 0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void push (float x) noexcept { buf[(size_t) w] = x; w = (w + 1) & mask; }

    /** Wert von vor d Samples (d >= 0, linear interpoliert); 0 = zuletzt geschrieben */
    float read (float d) const noexcept
    {
        float rp = (float) w - 1.0f - d;
        if (rp < 0.0f) rp += (float) buf.size();
        const int i0 = (int) rp;
        const float fr = rp - (float) i0;
        const float a = buf[(size_t) (i0 & mask)];
        const float b = buf[(size_t) ((i0 + 1) & mask)];
        return a + (b - a) * fr;
    }
    int size() const noexcept { return (int) buf.size(); }

private:
    std::vector<float> buf;
    int mask = 0, w = 0;
};

// ---------------------------------------------------------------------------
//  Zustandsvariablen-Filter (TPT / Zero-Delay-Feedback)
struct SVF
{
    float ic1 = 0.0f, ic2 = 0.0f, k = 1.0f, a1 = 1.0f, a2 = 0.0f, a3 = 0.0f;

    void set (float fc, float q, double sr) noexcept
    {
        fc = juce::jlimit (20.0f, (float) (sr * 0.45), fc);
        const float g = (float) std::tan (juce::MathConstants<double>::pi * fc / sr);
        k  = 1.0f / juce::jmax (0.05f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void reset() noexcept { ic1 = ic2 = 0.0f; }

    inline void process (float v0, float& lp, float& bp) noexcept
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1;
    }
    inline float lowpass (float x) noexcept  { float l, b; process (x, l, b); return l; }
    /** Bandpass mit Spitzenverstaerkung 1 */
    inline float bandpass (float x) noexcept { float l, b; process (x, l, b); return b * k; }
};

// ---------------------------------------------------------------------------
//  Ensemble im Stil der 70er-String-Machines: 3 modulierte Leitungen,
//  langsames + schnelles LFO, 120 Grad versetzt -> breites Stereo
class Ensemble
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        lineL.prepare ((int) (sr * 0.06));
        lineR.prepare ((int) (sr * 0.06));
        reset();
    }
    void reset() { lineL.clear(); lineR.clear(); ph1 = ph2 = 0.0; }

    /** inL/inR duerfen identisch sein (Mono-Quelle). Schreibt nach outL/outR (darf = in sein). */
    void process (const float* inL, const float* inR, float* outL, float* outR, int n,
                  float mix, float depth, float rate) noexcept
    {
        if (mix <= 0.0001f)
        {
            if (outL != inL) std::copy (inL, inL + n, outL);
            if (outR != inR) std::copy (inR, inR + n, outR);
            return;
        }
        const double inc1 = (0.35 + rate * 0.9) / sr;
        const double inc2 = (5.2 + rate * 2.0) / sr;
        const float base = 0.0075f * (float) sr;
        const float amt  = (0.0008f + depth * 0.0042f) * (float) sr;
        const float dry  = 1.0f - mix * 0.5f;
        constexpr double third = 1.0 / 3.0;
        constexpr double twoPi = juce::MathConstants<double>::twoPi;

        for (int s = 0; s < n; ++s)
        {
            const float xl = inL[s], xr = inR[s];
            lineL.push (xl);
            lineR.push (xr);
            float tap[3];
            for (int i = 0; i < 3; ++i)
            {
                const double off = i * third;
                const float m = 0.75f * (float) std::sin (twoPi * (ph1 + off))
                              + 0.25f * (float) std::sin (twoPi * (ph2 + off));
                const float d = base + amt * m;
                tap[i] = i == 0 ? lineL.read (d)
                       : i == 2 ? lineR.read (d)
                                : 0.5f * (lineL.read (d) + lineR.read (d));
            }
            const float wl = (tap[0] + 0.6f * tap[1]) * 0.625f;
            const float wr = (tap[2] + 0.6f * tap[1]) * 0.625f;
            outL[s] = xl * dry + wl * mix;
            outR[s] = xr * dry + wr * mix;

            ph1 += inc1; if (ph1 >= 1.0) ph1 -= 1.0;
            ph2 += inc2; if (ph2 >= 1.0) ph2 -= 1.0;
        }
    }

private:
    double sr = 44100.0, ph1 = 0.0, ph2 = 0.0;
    DelayLine lineL, lineR;
};

// ---------------------------------------------------------------------------
//  Stereo-Delay (Ping-Pong) mit gedaempfter Rueckkopplung
class StereoDelay
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        lineL.prepare ((int) (sr * 2.6));
        lineR.prepare ((int) (sr * 2.6));
        reset();
    }
    void reset() { lineL.clear(); lineR.clear(); lpL = lpR = 0.0f; dCur = -1.0f; }

    void process (float* L, float* R, int n, float timeSec, float feedback, float mix) noexcept
    {
        const float maxD = (float) lineL.size() - 8.0f;
        const float target = juce::jlimit (1.0f, maxD, timeSec * (float) sr);
        if (dCur < 0.0f) dCur = target;
        if (mix <= 0.0001f)
        {
            // Leitungen weiter fuellen, damit beim Aufdrehen kein Muell kommt
            for (int s = 0; s < n; ++s) { lineL.push (0.0f); lineR.push (0.0f); }
            lpL = lpR = 0.0f;
            dCur = target;
            return;
        }
        const float fb = juce::jlimit (0.0f, 0.95f, feedback);
        for (int s = 0; s < n; ++s)
        {
            dCur += (target - dCur) * 0.0008f;
            const float dl = lineL.read (dCur);
            const float dr = lineR.read (dCur);
            lpL += (dl - lpL) * 0.45f;
            lpR += (dr - lpR) * 0.45f;
            const float inMono = 0.5f * (L[s] + R[s]);
            lineL.push (std::tanh (inMono + lpR * fb));
            lineR.push (std::tanh (lpL * fb));
            L[s] += dl * mix;
            R[s] += dr * mix;
        }
    }

private:
    double sr = 44100.0;
    DelayLine lineL, lineR;
    float lpL = 0.0f, lpR = 0.0f, dCur = -1.0f;
};

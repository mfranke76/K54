// K-54 Analyzer · Keller54 · by Frank E.
// Lautheitsmessung nach ITU-R BS.1770-4 / EBU R128 (Momentary, Short-Term, Integrated, LRA, True Peak)
#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace k54a
{
static constexpr double kPi = 3.14159265358979323846;

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    inline double process (double x) noexcept
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() noexcept { z1 = z2 = 0.0; }
};

//==============================================================================
// True-Peak: 4-fach Überabtastung (polyphase FIR) wie in BS.1770 Anhang 2
class TruePeakDetector
{
public:
    static constexpr int taps = 16;

    void prepare (double sampleRate)
    {
        factor = sampleRate < 100000.0 ? 4 : (sampleRate < 200000.0 ? 2 : 1);
        const int N = taps * factor;
        const double c = (N - 1) * 0.5;
        std::array<double, taps * 4> proto {};

        for (int n = 0; n < N; ++n)
        {
            const double x = (n - c) / factor;
            const double sinc = std::abs (x) < 1e-12 ? 1.0 : std::sin (kPi * x) / (kPi * x);
            const double w = 0.42 - 0.5 * std::cos (2.0 * kPi * n / (N - 1))
                                   + 0.08 * std::cos (4.0 * kPi * n / (N - 1));
            proto[(size_t) n] = sinc * w;
        }

        for (int p = 0; p < factor; ++p)
        {
            double sum = 0.0;
            for (int k = 0; k < taps; ++k)
                sum += proto[(size_t) (p + k * factor)];
            for (int k = 0; k < taps; ++k)
                coeffs[(size_t) p][(size_t) k] = (float) (proto[(size_t) (p + k * factor)] / (std::abs (sum) > 1.0e-12 ? sum : 1.0));
        }
        reset();
    }

    void reset() noexcept { hist.fill (0.0f); pos = 0; }

    inline float process (float x) noexcept
    {
        hist[(size_t) pos] = x;
        hist[(size_t) (pos + taps)] = x;
        float m = std::abs (x);

        if (factor > 1)
        {
            const float* h = hist.data() + pos + 1; // ältestes … neuestes (taps Werte)
            for (int p = 0; p < factor; ++p)
            {
                const auto& c = coeffs[(size_t) p];
                float acc = 0.0f;
                for (int k = 0; k < taps; ++k)
                    acc += c[(size_t) k] * h[taps - 1 - k];
                m = std::max (m, std::abs (acc));
            }
        }

        pos = (pos + 1) % taps;
        return m;
    }

private:
    int factor = 4;
    std::array<std::array<float, taps>, 4> coeffs {};
    std::array<float, taps * 2> hist {};
    int pos = 0;
};

//==============================================================================
class LoudnessMeter
{
public:
    static constexpr int numBins = 800; // -70 … +10 LUFS in 0,1-dB-Schritten
    static constexpr float silent = -100.0f;

    void prepare (double sampleRate)
    {
        fs = sampleRate > 0.0 ? sampleRate : 48000.0;
        designKWeighting();
        blockLen = std::max (1, (int) std::lround (fs * 0.1));
        tp[0].prepare (fs);
        tp[1].prepare (fs);
        resetAll();
    }

    void resetAll()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            s1[(size_t) ch].reset();
            s2[(size_t) ch].reset();
            tp[(size_t) ch].reset();
        }
        acc = 0.0;
        accN = 0;
        ring.fill (0.0);
        ringCount = 0;
        ringPos = 0;
        momentaryDb = shortDb = silent;
        resetIntegrated();
    }

    // Integrated, LRA und True-Peak-Maximum neu beginnen
    void resetIntegrated()
    {
        gCount.fill (0); gSum.fill (0.0);
        stCount.fill (0); stSum.fill (0.0);
        integratedDb = silent;
        lraDb = 0.0f;
        tpMax = 0.0f;
        shortMax = silent;
    }

    // R == nullptr → Mono
    void process (const float* L, const float* R, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const double l = s2[0].process (s1[0].process ((double) L[i]));
            double e = l * l;
            float pk = tp[0].process (L[i]);

            if (R != nullptr)
            {
                const double r = s2[1].process (s1[1].process ((double) R[i]));
                e += r * r;
                pk = std::max (pk, tp[1].process (R[i]));
            }

            if (pk > tpMax)
                tpMax = pk;

            acc += e;
            if (++accN >= blockLen)
                finishBlock();
        }
    }

    float momentary() const noexcept  { return momentaryDb; }
    float shortTerm() const noexcept  { return shortDb; }
    float integrated() const noexcept { return integratedDb; }
    float lra() const noexcept        { return lraDb; }
    float shortTermMax() const noexcept { return shortMax; }
    float truePeakDb() const noexcept { return tpMax > 1.0e-6f ? 20.0f * std::log10 (tpMax) : silent; }
    bool hasIntegrated() const noexcept { return integratedDb > -90.0f; }

    static float toLufs (double meanSquare) noexcept
    {
        return meanSquare > 1.0e-20 ? (float) (-0.691 + 10.0 * std::log10 (meanSquare)) : silent;
    }

private:
    void designKWeighting()
    {
        // Stufe 1: High-Shelf (Kopf-Effekt)
        {
            const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            const double K = std::tan (kPi * f0 / fs);
            const double Vh = std::pow (10.0, G / 20.0);
            const double Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            Biquad b;
            b.b0 = (Vh + Vb * K / Q + K * K) / a0;
            b.b1 = 2.0 * (K * K - Vh) / a0;
            b.b2 = (Vh - Vb * K / Q + K * K) / a0;
            b.a1 = 2.0 * (K * K - 1.0) / a0;
            b.a2 = (1.0 - K / Q + K * K) / a0;
            s1[0] = s1[1] = b;
        }
        // Stufe 2: Hochpass (RLB)
        {
            const double f0 = 38.13547087602444, Q = 0.5003270373238773;
            const double K = std::tan (kPi * f0 / fs);
            const double a0 = 1.0 + K / Q + K * K;
            Biquad b;
            b.b0 = 1.0; b.b1 = -2.0; b.b2 = 1.0;
            b.a1 = 2.0 * (K * K - 1.0) / a0;
            b.a2 = (1.0 - K / Q + K * K) / a0;
            s2[0] = s2[1] = b;
        }
    }

    double meanOfLast (int count) const noexcept
    {
        double s = 0.0;
        for (int j = 0; j < count; ++j)
            s += ring[(size_t) ((ringPos - 1 - j + 30) % 30)];
        return s / count;
    }

    static int binOf (float lufs) noexcept
    {
        return std::clamp ((int) std::floor ((lufs + 70.0f) * 10.0f), 0, numBins - 1);
    }

    static int binStart (float threshold) noexcept
    {
        return std::clamp ((int) std::ceil ((threshold + 70.0f) * 10.0f - 0.5f), 0, numBins);
    }

    static float binCenter (int i) noexcept { return -70.0f + ((float) i + 0.5f) * 0.1f; }

    void finishBlock() noexcept
    {
        ring[(size_t) ringPos] = acc / (double) accN;
        ringPos = (ringPos + 1) % 30;
        ringCount = std::min (ringCount + 1, 30);
        acc = 0.0;
        accN = 0;

        if (ringCount >= 4)
        {
            const double m = meanOfLast (4);
            momentaryDb = toLufs (m);
            if (momentaryDb >= -70.0f)
            {
                const int b = binOf (momentaryDb);
                ++gCount[(size_t) b];
                gSum[(size_t) b] += m;
            }
        }

        if (ringCount >= 30)
        {
            const double s = meanOfLast (30);
            shortDb = toLufs (s);
            if (shortDb >= -70.0f)
            {
                const int b = binOf (shortDb);
                ++stCount[(size_t) b];
                stSum[(size_t) b] += s;
                shortMax = std::max (shortMax, shortDb);
            }
        }

        computeIntegrated();
        computeLRA();
    }

    void computeIntegrated() noexcept
    {
        double sum = 0.0; int64_t cnt = 0;
        for (int i = 0; i < numBins; ++i) { sum += gSum[(size_t) i]; cnt += gCount[(size_t) i]; }
        if (cnt == 0) { integratedDb = silent; return; }

        const float thr = toLufs (sum / (double) cnt) - 10.0f;
        sum = 0.0; cnt = 0;
        for (int i = binStart (thr); i < numBins; ++i) { sum += gSum[(size_t) i]; cnt += gCount[(size_t) i]; }
        integratedDb = cnt > 0 ? toLufs (sum / (double) cnt) : silent;
    }

    void computeLRA() noexcept
    {
        double sum = 0.0; int64_t cnt = 0;
        for (int i = 0; i < numBins; ++i) { sum += stSum[(size_t) i]; cnt += stCount[(size_t) i]; }
        if (cnt < 2) { lraDb = 0.0f; return; }

        const float thr = toLufs (sum / (double) cnt) - 20.0f;
        const int start = binStart (thr);
        int64_t total = 0;
        for (int i = start; i < numBins; ++i) total += stCount[(size_t) i];
        if (total < 2) { lraDb = 0.0f; return; }

        auto percentile = [&] (double q)
        {
            const double rank = q * (double) (total - 1);
            int64_t cum = 0;
            for (int i = start; i < numBins; ++i)
            {
                cum += stCount[(size_t) i];
                if ((double) cum > rank)
                    return binCenter (i);
            }
            return binCenter (numBins - 1);
        };

        lraDb = std::max (0.0f, percentile (0.95) - percentile (0.10));
    }

    double fs = 48000.0;
    std::array<Biquad, 2> s1, s2;
    std::array<TruePeakDetector, 2> tp;
    int blockLen = 4800;
    double acc = 0.0;
    int accN = 0;
    std::array<double, 30> ring {};
    int ringCount = 0, ringPos = 0;

    std::array<int64_t, numBins> gCount {}, stCount {};
    std::array<double, numBins> gSum {}, stSum {};

    float momentaryDb = silent, shortDb = silent, integratedDb = silent, lraDb = 0.0f, shortMax = silent;
    float tpMax = 0.0f;
};

} // namespace k54a

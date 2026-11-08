// K-54 Analyzer · FFT-Spektrum 20 Hz – 20 kHz (läuft im GUI-Thread bzw. im Lade-Thread)
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <cmath>

namespace k54a
{
//==============================================================================
struct SpecMath
{
    static constexpr int numPoints = 640;
    static constexpr float floorDb = -130.0f;
    static constexpr float fMin = 20.0f, fMax = 20000.0f;
    static constexpr int numBands = 6;

    static float pointFreq (int p) noexcept
    {
        return fMin * std::pow (fMax / fMin, (float) p / (float) (numPoints - 1));
    }

    static const std::array<float, numBands + 1>& bandEdges()
    {
        static const std::array<float, numBands + 1> e { 20.0f, 60.0f, 250.0f, 800.0f, 2500.0f, 6000.0f, 20000.0f };
        return e;
    }

    static const char* bandName (int b)
    {
        static const char* n[] { "Sub", "Bass", "Untere Mitten", "Mitten", "Präsenz", "Höhen" };
        return n[b];
    }

    static const char* bandRange (int b)
    {
        static const char* r[] { "20–60 Hz", "60–250 Hz", "250–800 Hz", "0,8–2,5 kHz", "2,5–6 kHz", "6–20 kHz" };
        return r[b];
    }

    static int optimalOrder (double sr, int resolution)
    {
        int o = 13 + juce::jlimit (0, 2, resolution);
        if (sr > 60000.0)  ++o;
        if (sr > 130000.0) ++o;
        return juce::jmin (o, 16);
    }

    static std::vector<float> hann (int size, float& sum)
    {
        std::vector<float> w ((size_t) size);
        sum = 0.0f;
        for (int i = 0; i < size; ++i)
        {
            w[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) (size - 1));
            sum += w[(size_t) i];
        }
        return w;
    }

    // Leistungsspektrum (Index 0 … size/2, Amplitude² mit Vollaussteuerung = 1) → 640 Punkte in dB (ohne Neigung)
    template <typename PowerVec>
    static void pointsFromPower (const PowerVec& power, int fftSize, double sr, float smoothOct, bool useMax,
                                 std::vector<float>& outDb)
    {
        outDb.resize (numPoints);
        const float pointOct = std::log2 (fMax / fMin) / (float) (numPoints - 1);
        const float halfBand = std::max (pointOct, smoothOct) * 0.5f;
        const double binHz = sr / (double) fftSize;
        const int maxBin = fftSize / 2;

        for (int p = 0; p < numPoints; ++p)
        {
            const double f = pointFreq (p);
            const double l = f * std::pow (2.0, -halfBand) / binHz;
            const double h = f * std::pow (2.0,  halfBand) / binHz;
            double pw;

            if (h - l < 1.0)
            {
                const double c = f / binHz;
                const int k0 = juce::jlimit (0, maxBin - 1, (int) std::floor (c));
                const double frac = juce::jlimit (0.0, 1.0, c - k0);
                pw = (double) power[(size_t) k0] * (1.0 - frac) + (double) power[(size_t) k0 + 1] * frac;
            }
            else
            {
                const int a = juce::jlimit (0, maxBin, (int) std::ceil (l));
                const int b = juce::jlimit (a, maxBin, (int) std::floor (h));
                if (useMax)
                {
                    pw = 0.0;
                    for (int k = a; k <= b; ++k) pw = std::max (pw, (double) power[(size_t) k]);
                }
                else
                {
                    double s = 0.0;
                    for (int k = a; k <= b; ++k) s += (double) power[(size_t) k];
                    pw = s / (b - a + 1);
                }
            }
            outDb[(size_t) p] = pw > 1.0e-14 ? (float) (10.0 * std::log10 (pw)) : floorDb;
        }
    }

    // Bandpegel relativ zum Gesamtpegel 20 Hz – 20 kHz (lautstärkeunabhängig). false = zu leise
    template <typename PowerVec>
    static bool bandsFromPower (const PowerVec& power, int fftSize, double sr, std::array<float, numBands>& out)
    {
        const double binHz = sr / (double) fftSize;
        const int maxBin = fftSize / 2;
        const auto& e = bandEdges();
        double total = 0.0;
        std::array<double, numBands> band {};

        for (int k = 1; k <= maxBin; ++k)
        {
            const double f = k * binHz;
            if (f < e[0] || f >= e[numBands]) continue;
            const double p = (double) power[(size_t) k];
            total += p;
            for (int b = 0; b < numBands; ++b)
                if (f < e[(size_t) b + 1]) { band[(size_t) b] += p; break; }
        }

        if (total < 1.0e-12) return false;
        for (int b = 0; b < numBands; ++b)
            out[(size_t) b] = band[(size_t) b] > 1.0e-20 ? (float) (10.0 * std::log10 (band[(size_t) b] / total)) : -100.0f;
        return true;
    }
};

//==============================================================================
class SpectrumAnalyser
{
public:
    static constexpr int numPoints = SpecMath::numPoints;
    static constexpr float floorDb = SpecMath::floorDb;
    static constexpr float fMin = SpecMath::fMin, fMax = SpecMath::fMax;

    void prepare (double sr, int resolution = 0)
    {
        sampleRate = sr > 0.0 ? sr : 48000.0;
        res = resolution;
        order = SpecMath::optimalOrder (sampleRate, resolution);
        size = 1 << order;
        fft = std::make_unique<juce::dsp::FFT> (order);
        window = SpecMath::hann (size, windowSum);

        ring.assign ((size_t) size, 0.0f);
        writePos = 0;
        newSamples = 0;
        fftData.assign ((size_t) size * 2, 0.0f);
        power.assign ((size_t) size / 2 + 1, 0.0f);
        avgPow.assign ((size_t) size / 2 + 1, 0.0);
        avgFrames = 0;
        display.assign (numPoints, floorDb);
        hold.assign (numPoints, floorDb);
        target.assign (numPoints, floorDb);
        freq.resize (numPoints);
        octFrom1k.resize (numPoints);
        for (int p = 0; p < numPoints; ++p)
        {
            freq[(size_t) p] = SpecMath::pointFreq (p);
            octFrom1k[(size_t) p] = std::log2 (freq[(size_t) p] / 1000.0f);
        }
    }

    double getSampleRate() const noexcept { return sampleRate; }
    int getResolution() const noexcept { return res; }
    int getFftSize() const noexcept { return size; }

    void push (const float* d, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            ring[(size_t) writePos] = d[i];
            if (++writePos >= size) writePos = 0;
        }
        newSamples += n;
    }

    // speed: 0 = langsam, 1 = mittel, 2 = schnell, 3 = Langzeit (Mittel seit Reset)
    // staticDb: fertige Langzeit-Kurve (z. B. ganze Referenz-Datei), ersetzt bei speed 3 das eigene Mittel
    void update (float slopeDbPerOct, float smoothOct, int speed, bool holdOn,
                 const std::vector<float>* staticDb = nullptr, float staticGainDb = 0.0f)
    {
        const bool fresh = newSamples > 0;
        newSamples = 0;

        if (fresh)
        {
            for (int i = 0; i < size; ++i)
                fftData[(size_t) i] = ring[(size_t) ((writePos + i) % size)] * window[(size_t) i];
            std::fill (fftData.begin() + size, fftData.end(), 0.0f);
            fft->performFrequencyOnlyForwardTransform (fftData.data(), true);

            const float scale = 2.0f / windowSum;
            double frameSum = 0.0;
            for (int k = 0; k <= size / 2; ++k)
            {
                const float a = fftData[(size_t) k] * scale;
                power[(size_t) k] = a * a;
                frameSum += power[(size_t) k];
            }

            if (frameSum > 1.0e-7) // ab etwa -70 dB: Stille zählt nicht ins Mittel
            {
                for (int k = 0; k <= size / 2; ++k) avgPow[(size_t) k] += power[(size_t) k];
                ++avgFrames;
            }
        }

        const bool longTerm = speed >= 3;

        if (longTerm)
        {
            const float sm = std::max (smoothOct, 1.0f / 12.0f);
            if (staticDb != nullptr && (int) staticDb->size() == numPoints)
            {
                for (int p = 0; p < numPoints; ++p)
                    target[(size_t) p] = (*staticDb)[(size_t) p] + staticGainDb;
            }
            else if (avgFrames > 0)
            {
                avgTmp.resize (avgPow.size());
                const double inv = 1.0 / (double) avgFrames;
                for (size_t k = 0; k < avgPow.size(); ++k) avgTmp[k] = (float) (avgPow[k] * inv);
                SpecMath::pointsFromPower (avgTmp, size, sampleRate, sm, false, target);
            }
            else
                std::fill (target.begin(), target.end(), floorDb);
        }
        else if (fresh)
        {
            SpecMath::pointsFromPower (power, size, sampleRate, smoothOct, smoothOct <= 0.0f, target);
        }
        else
        {
            std::fill (target.begin(), target.end(), floorDb);
        }

        for (int p = 0; p < numPoints; ++p)
            if (target[(size_t) p] > floorDb + 1.0f)
                target[(size_t) p] += slopeDbPerOct * octFrom1k[(size_t) p];

        static constexpr float upC[4]   { 0.30f, 0.55f, 0.85f, 0.6f };
        static constexpr float downC[4] { 0.035f, 0.09f, 0.22f, 0.6f };
        const int s = juce::jlimit (0, 3, speed);

        for (int p = 0; p < numPoints; ++p)
        {
            float& d = display[(size_t) p];
            const float t = target[(size_t) p];
            d += (t > d ? upC[s] : downC[s]) * (t - d);
            if (holdOn) hold[(size_t) p] = std::max (hold[(size_t) p], d);
            else        hold[(size_t) p] = d;
        }
        lastFresh = fresh;
    }

    void resetHold() { hold = display; }
    void resetAverage() { std::fill (avgPow.begin(), avgPow.end(), 0.0); avgFrames = 0; }
    int averageFrames() const noexcept { return avgFrames; }
    double averageSeconds() const noexcept { return (double) avgFrames * (double) size / sampleRate; }

    bool bands (std::array<float, SpecMath::numBands>& out) const
    {
        if (avgFrames < 4) return false;
        return SpecMath::bandsFromPower (avgPow, size, sampleRate, out);
    }

    float valueAt (float f) const
    {
        const float pos = std::log (f / fMin) / std::log (fMax / fMin) * (float) (numPoints - 1);
        const int i = juce::jlimit (0, numPoints - 2, (int) pos);
        const float fr = juce::jlimit (0.0f, 1.0f, pos - (float) i);
        return display[(size_t) i] * (1.0f - fr) + display[(size_t) i + 1] * fr;
    }

    std::vector<float> display, hold, freq;
    bool lastFresh = false;

private:
    double sampleRate = 48000.0;
    int res = 0, order = 13, size = 8192;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window, ring, fftData, power, target, octFrom1k, avgTmp;
    std::vector<double> avgPow;
    float windowSum = 1.0f;
    int writePos = 0, newSamples = 0, avgFrames = 0;
};

} // namespace k54a

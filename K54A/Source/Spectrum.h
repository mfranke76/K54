// K-54 Analyzer · FFT-Spektrum 20 Hz – 20 kHz (läuft im GUI-Thread)
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

namespace k54a
{
class SpectrumAnalyser
{
public:
    static constexpr int numPoints = 640;
    static constexpr float floorDb = -130.0f;
    static constexpr float fMin = 20.0f, fMax = 20000.0f;

    void prepare (double sr)
    {
        sampleRate = sr > 0.0 ? sr : 48000.0;
        order = sampleRate > 100000.0 ? 15 : (sampleRate > 60000.0 ? 14 : 13);
        size = 1 << order;
        fft = std::make_unique<juce::dsp::FFT> (order);

        window.assign ((size_t) size, 0.0f);
        windowSum = 0.0f;
        for (int i = 0; i < size; ++i)
        {
            window[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) (size - 1));
            windowSum += window[(size_t) i];
        }

        ring.assign ((size_t) size, 0.0f);
        writePos = 0;
        newSamples = 0;
        fftData.assign ((size_t) size * 2, 0.0f);
        power.assign ((size_t) size / 2 + 1, 0.0f);
        display.assign (numPoints, floorDb);
        hold.assign (numPoints, floorDb);
        target.assign (numPoints, floorDb);
        freq.resize (numPoints);
        octFrom1k.resize (numPoints);
        lo.resize (numPoints);
        hi.resize (numPoints);

        for (int p = 0; p < numPoints; ++p)
        {
            freq[(size_t) p] = fMin * std::pow (fMax / fMin, (float) p / (float) (numPoints - 1));
            octFrom1k[(size_t) p] = std::log2 (freq[(size_t) p] / 1000.0f);
        }
        mapSmoothing = -1.0f;
    }

    double getSampleRate() const noexcept { return sampleRate; }

    void push (const float* d, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            ring[(size_t) writePos] = d[i];
            if (++writePos >= size) writePos = 0;
        }
        newSamples += n;
    }

    // speed: 0 = langsam, 1 = mittel, 2 = schnell
    void update (float slopeDbPerOct, float smoothOct, int speed, bool holdOn)
    {
        if (std::abs (smoothOct - mapSmoothing) > 1.0e-6f)
            buildMap (smoothOct);

        const bool fresh = newSamples > 0;
        newSamples = 0;

        if (fresh)
        {
            for (int i = 0; i < size; ++i)
                fftData[(size_t) i] = ring[(size_t) ((writePos + i) % size)] * window[(size_t) i];
            std::fill (fftData.begin() + size, fftData.end(), 0.0f);

            fft->performFrequencyOnlyForwardTransform (fftData.data(), true);

            const float scale = 2.0f / windowSum;
            for (int k = 0; k <= size / 2; ++k)
            {
                const float a = fftData[(size_t) k] * scale;
                power[(size_t) k] = a * a;
            }

            const int maxBin = size / 2;
            for (int p = 0; p < numPoints; ++p)
            {
                const double l = lo[(size_t) p], h = hi[(size_t) p];
                float pw;

                if (h - l < 1.0)
                {
                    const double c = std::sqrt (l * h);
                    const int k0 = juce::jlimit (0, maxBin - 1, (int) std::floor (c));
                    const float frac = (float) juce::jlimit (0.0, 1.0, c - k0);
                    pw = power[(size_t) k0] * (1.0f - frac) + power[(size_t) k0 + 1] * frac;
                }
                else
                {
                    const int a = juce::jlimit (0, maxBin, (int) std::ceil (l));
                    const int b = juce::jlimit (a, maxBin, (int) std::floor (h));
                    if (mapSmoothing > 0.0f)
                    {
                        double s = 0.0;
                        for (int k = a; k <= b; ++k) s += power[(size_t) k];
                        pw = (float) (s / (b - a + 1));
                    }
                    else
                    {
                        pw = 0.0f;
                        for (int k = a; k <= b; ++k) pw = std::max (pw, power[(size_t) k]);
                    }
                }

                const float db = pw > 1.0e-14f ? 10.0f * std::log10 (pw) : floorDb;
                target[(size_t) p] = db + slopeDbPerOct * octFrom1k[(size_t) p];
            }
        }
        else
        {
            std::fill (target.begin(), target.end(), floorDb);
        }

        static constexpr float upC[3]   { 0.30f, 0.55f, 0.85f };
        static constexpr float downC[3] { 0.035f, 0.09f, 0.22f };
        const int s = juce::jlimit (0, 2, speed);

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

    float peakDisplay() const
    {
        float m = floorDb;
        for (auto v : display) m = std::max (m, v);
        return m;
    }

    // Wert an einer beliebigen Frequenz (für die Maus-Anzeige)
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
    void buildMap (float smoothOct)
    {
        mapSmoothing = smoothOct;
        const float pointOct = std::log2 (fMax / fMin) / (float) (numPoints - 1);
        const float halfBand = std::max (pointOct, smoothOct) * 0.5f;
        const double binHz = sampleRate / (double) size;

        for (int p = 0; p < numPoints; ++p)
        {
            const double f = freq[(size_t) p];
            lo[(size_t) p] = f * std::pow (2.0, -halfBand) / binHz;
            hi[(size_t) p] = f * std::pow (2.0, halfBand) / binHz;
        }
    }

    double sampleRate = 48000.0;
    int order = 13, size = 8192;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window, ring, fftData, power, target, octFrom1k;
    std::vector<double> lo, hi;
    float windowSum = 1.0f, mapSmoothing = -1.0f;
    int writePos = 0, newSamples = 0;
};

} // namespace k54a

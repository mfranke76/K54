#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <complex>
#include <vector>
#include <memory>
#include <cmath>
#include <array>
#include <atomic>
#include "Parameters.h"
#include "BinaryData.h"

/** Kleine Radix-2-FFT (komplex, in-place) fuer die Wavetable-Berechnung */
inline void k55Fft (std::vector<std::complex<double>>& a, bool inverse)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap (a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = 2.0 * juce::MathConstants<double>::pi / (double) len * (inverse ? 1.0 : -1.0);
        const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<double> w (1.0, 0.0);
            for (size_t k = 0; k < len / 2; ++k)
            {
                const auto u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

// ============================================================================
//  K-54 S Klangquellen: Wavetables (bandbegrenzt, je Oktave eine Stufe) + Samples
// ============================================================================

struct Wavetable
{
    static constexpr int N = 2048, order = 11, levels = 11;   // Stufe L: hoechstens 1024 >> L Obertoene
    int numFrames = 0;
    juce::String name;
    std::vector<float> data;                                   // [Frame][Stufe][N + 1]

    const float* frame (int f, int L) const noexcept { return data.data() + ((size_t) f * levels + (size_t) L) * (N + 1); }

    /** Stufe passend zur Frequenz (inc = f / fs), damit nichts ueber Nyquist rutscht */
    static int levelFor (double inc) noexcept
    {
        const double need = 1024.0 * inc / 0.45;
        if (need <= 1.0) return 0;
        return juce::jlimit (0, levels - 1, (int) std::ceil (std::log2 (need)));
    }

    float read (double phase, float pos, int L) const noexcept
    {
        const float fp = juce::jlimit (0.0f, 1.0f, pos) * (float) (numFrames - 1);
        const int f0 = (int) fp;
        const int f1 = juce::jmin (f0 + 1, numFrames - 1);
        const float fr = fp - (float) f0;
        const double x = phase * N;
        const int i = juce::jlimit (0, N - 1, (int) x);
        const float t = (float) (x - i);
        const float* a = frame (f0, L);
        const float va = a[i] + (a[i + 1] - a[i]) * t;
        if (f1 == f0 || fr < 0.0001f) return va;
        const float* b = frame (f1, L);
        const float vb = b[i] + (b[i + 1] - b[i]) * t;
        return va + (vb - va) * fr;
    }

    /** Baut die Tabelle aus Einzelzyklen (je N Samples, Zeitbereich) */
    static std::shared_ptr<Wavetable> fromFrames (const std::vector<std::vector<float>>& frames, const juce::String& nm)
    {
        auto wt = std::make_shared<Wavetable>();
        wt->name = nm;
        wt->numFrames = juce::jmax (1, (int) frames.size());
        wt->data.assign ((size_t) wt->numFrames * levels * (N + 1), 0.0f);

        std::vector<std::complex<double>> spec ((size_t) N), tmp ((size_t) N);
        for (int f = 0; f < (int) frames.size(); ++f)
        {
            for (int s = 0; s < N; ++s) spec[(size_t) s] = { (double) frames[(size_t) f][(size_t) s], 0.0 };
            k55Fft (spec, false);
            spec[0] = {};                                                    // kein Gleichanteil
            float gain = 1.0f;
            for (int L = 0; L < levels; ++L)
            {
                const int H = 1024 >> L;
                std::fill (tmp.begin(), tmp.end(), std::complex<double> {});
                for (int k = 1; k <= H && k < N / 2; ++k)
                {
                    tmp[(size_t) k] = spec[(size_t) k];
                    tmp[(size_t) (N - k)] = spec[(size_t) (N - k)];
                }
                k55Fft (tmp, true);
                if (L == 0)
                {
                    double pk = 0.0;
                    for (auto& c : tmp) pk = juce::jmax (pk, std::abs (c.real()));
                    gain = pk > 1.0e-9 ? (float) (1.0 / pk) : 0.0f;             // jeder Frame gleich laut
                }
                float* d = wt->data.data() + ((size_t) f * levels + (size_t) L) * (N + 1);
                for (int s = 0; s < N; ++s) d[s] = (float) tmp[(size_t) s].real() * gain;
                d[N] = d[0];
            }
        }
        return wt;
    }

    /** Eigene Wavetable aus Audio: Serum-Format (je 2048 Samples ein Frame), Einzelzyklus oder beliebiges Audio */
    static std::shared_ptr<Wavetable> fromAudio (const juce::AudioBuffer<float>& audio, const juce::String& nm)
    {
        const int len = audio.getNumSamples();
        if (len < 16) return {};
        std::vector<float> mono ((size_t) len);
        for (int s = 0; s < len; ++s)
        {
            float v = 0.0f;
            for (int c = 0; c < audio.getNumChannels(); ++c) v += audio.getSample (c, s);
            mono[(size_t) s] = v / (float) juce::jmax (1, audio.getNumChannels());
        }

        std::vector<std::vector<float>> frames;
        auto resampleFrame = [] (const float* src, int n)
        {
            std::vector<float> f ((size_t) N);
            for (int s = 0; s < N; ++s)
            {
                const double x = (double) s * n / N;
                const int i = (int) x;
                const float t = (float) (x - i);
                f[(size_t) s] = src[i % n] + (src[(i + 1) % n] - src[i % n]) * t;
            }
            return f;
        };

        if (len <= 4096 && len % N != 0)                                    // Einzelzyklus (z. B. 600 Samples)
            frames.push_back (resampleFrame (mono.data(), len));
        else
        {
            const int avail = len / N;
            const int count = juce::jlimit (1, 64, avail);
            for (int k = 0; k < count; ++k)
            {
                const int src = count == 1 ? 0 : (int) ((double) k * (avail - 1) / (count - 1));
                frames.emplace_back (mono.begin() + src * N, mono.begin() + (src + 1) * N);
            }
        }
        return fromFrames (frames, nm);
    }
};

// ---------------------------------------------------------------------------
//  Werks-Wavetables (einmal pro Programm, von allen Instanzen geteilt)
class FactoryTables
{
public:
    FactoryTables()
    {
        constexpr int F = 16;
        const double twoPi = juce::MathConstants<double>::twoPi;
        auto make = [&] (const char* nm, auto&& fn)
        {
            std::vector<std::vector<float>> frames ((size_t) F, std::vector<float> ((size_t) Wavetable::N));
            for (int f = 0; f < F; ++f)
            {
                const double t = (double) f / (F - 1);
                for (int s = 0; s < Wavetable::N; ++s)
                    frames[(size_t) f][(size_t) s] = (float) fn ((double) s / Wavetable::N, t);
            }
            tables.push_back (Wavetable::fromFrames (frames, nm));
        };
        auto saw = [] (double x) { return 2.0 * x - 1.0; };
        auto addH = [&] (double x, auto&& amp, int maxH)
        {
            double v = 0.0;
            for (int k = 1; k <= maxH; ++k) { const double a = amp (k); if (a != 0.0) v += a * std::sin (twoPi * k * x); }
            return v;
        };

        // 0 Basic Shapes: Sinus -> Dreieck -> Saege -> Rechteck
        make ("Basic Shapes", [&] (double x, double t)
        {
            const double sine = std::sin (twoPi * x);
            const double tri  = 1.0 - 4.0 * std::abs (std::fmod (x + 0.25, 1.0) - 0.5);
            const double sw   = saw (std::fmod (x + 0.5, 1.0));
            const double sq   = x < 0.5 ? 1.0 : -1.0;
            const double seg = t * 3.0;
            if (seg < 1.0) return sine + (tri - sine) * seg;
            if (seg < 2.0) return tri + (sw - tri) * (seg - 1.0);
            return sw + (sq - sw) * (seg - 2.0);
        });
        // 1 PWM Sweep
        make ("PWM Sweep", [] (double x, double t) { const double w = 0.5 - t * 0.47; return x < w ? 1.0 : -1.0; });
        // 2 Harmonics: Obertoene kommen nach und nach dazu
        make ("Harmonics", [&] (double x, double t)
        {
            const double K = 1.0 + t * 47.0;
            return addH (x, [K] (int k) { return k <= (int) K ? 1.0 / std::sqrt ((double) k) : (k == (int) K + 1 ? (K - std::floor (K)) / std::sqrt ((double) k) : 0.0); }, 49);
        });
        // 3 Vocal: Formanten A - E - I - O - U
        make ("Vocal", [&] (double x, double t)
        {
            static const double fm[5][3] { { 800, 1150, 2900 }, { 400, 1600, 2700 }, { 250, 1750, 2600 }, { 450, 800, 2830 }, { 325, 700, 2530 } };
            const double seg = t * 4.0;
            const int a = juce::jmin (3, (int) seg);
            const double fr = seg - a;
            double F1[3];
            for (int k = 0; k < 3; ++k) F1[k] = fm[a][k] + (fm[a + 1][k] - fm[a][k]) * fr;
            return addH (x, [&] (int k)
            {
                const double hz = 110.0 * k;
                double amp = 0.0;
                for (int q = 0; q < 3; ++q)
                {
                    const double bw = 60.0 + 40.0 * q;
                    amp += std::exp (-0.5 * std::pow ((hz - F1[q]) / bw, 2.0)) * (q == 0 ? 1.0 : (q == 1 ? 0.7 : 0.35));
                }
                return amp / std::sqrt ((double) k);
            }, 40);
        });
        // 4 Digital FM
        make ("Digital FM", [&] (double x, double t) { return std::sin (twoPi * x + t * 7.0 * std::sin (twoPi * 2.0 * x)); });
        // 5 Sync Sweep
        make ("Sync Sweep", [&] (double x, double t) { const double r = 1.0 + t * 7.0; return saw (std::fmod (x * r, 1.0)) * (1.0 - x * 0.3); });
        // 6 Organ Bars: von Floete zu vollem Register
        make ("Organ Bars", [&] (double x, double t)
        {
            const double bars[6][2] { { 1, 1.0 }, { 2, 0.8 }, { 3, 0.6 }, { 4, 0.5 }, { 6, 0.35 }, { 8, 0.3 } };
            double v = 0.0;
            for (int b = 0; b < 6; ++b)
            {
                const double on = juce::jlimit (0.0, 1.0, t * 6.0 - b + 1.0);
                v += bars[b][1] * on * std::sin (twoPi * bars[b][0] * x);
            }
            return v;
        });
        // 7 Bell: Teiltoene wandern nach oben
        make ("Bell", [&] (double x, double t)
        {
            static const int pk[] { 1, 3, 5, 8, 11, 14, 19 };
            double v = 0.0;
            for (int i = 0; i < 7; ++i)
                v += std::sin (twoPi * (pk[i] + (i > 0 ? std::round (t * i * 1.5) : 0)) * x) / (1.0 + i * 0.6);
            return v;
        });
        // 8 Growl: gefaltete Saege (Dubstep-Knurren)
        make ("Growl", [&] (double x, double t)
        {
            const double s = saw (x) * 0.7 + std::sin (twoPi * x) * 0.3;
            return std::sin (juce::MathConstants<double>::pi * s * (1.0 + t * 6.0));
        });
        // 9 Pipe Principal: Register kommen dazu (8' -> 4' -> 2 2/3' -> 2' -> Mixtur)
        make ("Pipe Principal", [&] (double x, double t)
        {
            auto on = [t] (double from) { return juce::jlimit (0.0, 1.0, (t - from) * 5.0); };
            double v = 0.0;
            const double base[] { 1.0, 0.45, 0.3, 0.2, 0.12, 0.1, 0.05, 0.06 };
            for (int k = 1; k <= 8; ++k) v += base[k - 1] * std::sin (twoPi * k * x);
            const double o4 = on (0.1), q = on (0.3), o2 = on (0.5), mix = on (0.7);
            for (int k : { 2, 4, 6, 8 })        v += o4 * 0.35 / std::sqrt ((double) k) * std::sin (twoPi * k * x);
            for (int k : { 3, 6, 9 })           v += q  * 0.25 / std::sqrt ((double) k) * std::sin (twoPi * k * x);
            for (int k : { 4, 8, 12, 16 })      v += o2 * 0.25 / std::sqrt ((double) k) * std::sin (twoPi * k * x);
            for (int k : { 6, 8, 12, 16, 24, 32 }) v += mix * 0.22 / std::sqrt ((double) k) * std::sin (twoPi * k * x);
            return v;
        });
        // 10 Pipe Flute: gedackt (ungerade Obertoene) -> offene Floete
        make ("Pipe Flute", [&] (double x, double t)
        {
            return std::sin (twoPi * x) + (1.0 - t) * (0.15 * std::sin (twoPi * 3 * x) + 0.05 * std::sin (twoPi * 5 * x))
                 + t * (0.25 * std::sin (twoPi * 2 * x) + 0.1 * std::sin (twoPi * 3 * x) + 0.05 * std::sin (twoPi * 4 * x));
        });
        // 11 Pipe Reed: Oboe -> Trompete
        make ("Pipe Reed", [&] (double x, double t)
        {
            return addH (x, [t] (int k)
            {
                const double oboe = std::exp (-0.5 * std::pow ((k - 4.5) / 2.0, 2.0)) + 0.3 / k;
                const double trump = 1.0 / std::pow ((double) k, 0.6);
                return oboe * (1.0 - t) + trump * t;
            }, 40);
        });
        // 12 E-Piano: FM-Glocke, wird mit der Position weicher (Pos per Huellkurve modulieren)
        make ("E-Piano", [&] (double x, double t)
        {
            const double idx = (1.0 - t) * 3.2;
            return std::sin (twoPi * x + idx * std::sin (twoPi * x)) + 0.18 * (1.0 - t) * (1.0 - t) * std::sin (twoPi * 14.0 * x);
        });
    }

    std::vector<std::shared_ptr<Wavetable>> tables;   // Index wie ParamChoices::tables (ohne "User")
};


// ---------------------------------------------------------------------------
//  Konzertfluegel-Samples (Salamander Grand Piano V3, Alexander Holm, CC BY 3.0)
//  30 Zonen (alle 3 Halbtoene), als OGG eingebettet, beim ersten Gebrauch im Hintergrund geladen,
//  als 16 Bit gespeichert (halber Speicher)
struct PianoZone
{
    std::vector<int16_t> d;      // L/R verschachtelt
    int len = 0, root = 60;
    double sampleRate = 44100.0;

    int length() const noexcept { return len; }
    bool isStereo() const noexcept { return true; }
    float read (int ch, double pos) const noexcept
    {
        const int i = (int) pos;
        const float t = (float) (pos - i);
        auto at = [&] (int k) { return (float) d[(size_t) (juce::jlimit (0, len - 1, k) * 2 + ch)] * (1.0f / 32768.0f); };
        const float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + x0;
    }
};

class PianoSet : private juce::Thread
{
public:
    PianoSet() : juce::Thread ("K-54 S Piano") {}
    ~PianoSet() override { stopThread (5000); }

    void ensureLoaded() { if (! started.exchange (true)) startThread(); }
    bool isReady() const noexcept { return ready.load(); }
    /** Zone fuer eine MIDI-Note (nur gueltig, wenn isReady()) */
    const PianoZone* zoneFor (int note) const noexcept
    {
        const int z = map[(size_t) juce::jlimit (0, 127, note)];
        return z >= 0 ? &zones[(size_t) z] : nullptr;
    }
    /** Fuer den Test: wartet, bis alles geladen ist */
    bool waitUntilReady (int ms) { ensureLoaded(); const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) ms;
                                   while (! ready && juce::Time::getMillisecondCounter() < end) juce::Thread::sleep (20); return ready; }

private:
    static int noteFromName (const juce::String& n)    // "piano_Ds4.ogg" -> 63
    {
        auto core = n.fromFirstOccurrenceOf ("piano_", false, false).upToFirstOccurrenceOf (".", false, false);
        if (core.length() < 2) return -1;
        static const juce::String letters ("C D EF G A B");
        int pc = letters.indexOfChar (core[0]);
        int p = 1;
        if (core[1] == 's') { ++pc; ++p; }
        const int oct = core.substring (p).getIntValue();
        return pc < 0 ? -1 : (oct + 1) * 12 + pc;
    }

    void run() override
    {
        juce::OggVorbisAudioFormat ogg;
        std::vector<PianoZone> zs;
        for (int r = 0; r < BinaryData::namedResourceListSize; ++r)
        {
            if (threadShouldExit()) return;
            const juce::String file (BinaryData::getNamedResourceOriginalFilename (BinaryData::namedResourceList[r]));
            if (! file.startsWith ("piano_")) continue;
            int size = 0;
            const char* data = BinaryData::getNamedResource (BinaryData::namedResourceList[r], size);
            std::unique_ptr<juce::AudioFormatReader> rd (ogg.createReaderFor (new juce::MemoryInputStream (data, (size_t) size, false), true));
            if (rd == nullptr) continue;
            PianoZone z;
            z.root = noteFromName (file);
            z.sampleRate = rd->sampleRate;
            z.len = (int) rd->lengthInSamples;
            juce::AudioBuffer<float> b (2, z.len);
            rd->read (&b, 0, z.len, 0, true, true);
            z.d.resize ((size_t) z.len * 2);
            for (int s = 0; s < z.len; ++s)
                for (int c = 0; c < 2; ++c)
                    z.d[(size_t) s * 2 + (size_t) c] = (int16_t) juce::jlimit (-32767, 32767, juce::roundToInt (b.getSample (c, s) * 32767.0f));
            if (z.root > 0) zs.push_back (std::move (z));
        }
        std::sort (zs.begin(), zs.end(), [] (const PianoZone& a, const PianoZone& b) { return a.root < b.root; });
        zones = std::move (zs);
        for (int n = 0; n < 128; ++n)
        {
            int best = -1, dist = 1000;
            for (int z = 0; z < (int) zones.size(); ++z)
            {
                const int d = std::abs (zones[(size_t) z].root - n);
                if (d < dist) { dist = d; best = z; }
            }
            map[(size_t) n] = best;
        }
        ready = true;
    }

    std::vector<PianoZone> zones;
    std::array<int, 128> map {};
    std::atomic<bool> started { false }, ready { false };
};

// ---------------------------------------------------------------------------
//  Geladenes oder aufgenommenes Sample
struct SampleData
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 44100.0;
    juce::String name;
    juce::File file;
    std::vector<float> peaks;                        // fuer die Anzeige: je Punkt max |x|

    int length() const noexcept { return audio.getNumSamples(); }
    bool isStereo() const noexcept { return audio.getNumChannels() > 1; }

    void buildPeaks (int points = 600)
    {
        peaks.assign ((size_t) points, 0.0f);
        const int len = audio.getNumSamples();
        if (len == 0) return;
        for (int p = 0; p < points; ++p)
        {
            const int a = (int) ((int64_t) p * len / points), b = juce::jmax (a + 1, (int) ((int64_t) (p + 1) * len / points));
            float m = 0.0f;
            for (int c = 0; c < audio.getNumChannels(); ++c)
                m = juce::jmax (m, audio.getMagnitude (c, a, juce::jmin (b, len) - a));
            peaks[(size_t) p] = m;
        }
    }

    /** 4-Punkt-Hermite-Interpolation */
    float read (int ch, double pos) const noexcept
    {
        const int len = audio.getNumSamples();
        const float* d = audio.getReadPointer (juce::jmin (ch, audio.getNumChannels() - 1));
        const int i = (int) pos;
        const float t = (float) (pos - i);
        auto at = [&] (int k) { return d[juce::jlimit (0, len - 1, k)]; };
        const float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + x0;
    }

    static constexpr double maxSeconds = 60.0;

    static std::shared_ptr<SampleData> load (const juce::File& f, juce::AudioFormatManager& fm, juce::String& error, double maxSec = maxSeconds)
    {
        std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (f));
        if (reader == nullptr) { error = "Dateiformat wird nicht unterstuetzt: " + f.getFileName(); return {}; }
        const auto len = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * maxSec));
        if (len <= 0) { error = "Die Datei ist leer."; return {}; }
        auto s = std::make_shared<SampleData>();
        const int ch = (int) juce::jlimit (1u, 2u, reader->numChannels);
        s->audio.setSize (ch, len);
        reader->read (&s->audio, 0, len, 0, true, ch > 1);
        s->sampleRate = reader->sampleRate;
        s->name = f.getFileNameWithoutExtension();
        s->file = f;
        s->buildPeaks();
        return s;
    }
};

// ---------------------------------------------------------------------------
//  Haelt Objekte am Leben, bis der Audio-Thread sie nicht mehr benutzt
//  (so wird im Audio-Thread nie Speicher freigegeben)
class ReleasePool : private juce::Timer
{
public:
    ReleasePool() { startTimer (1000); }
    ~ReleasePool() override { stopTimer(); }
    void add (std::shared_ptr<void> p) { const juce::ScopedLock sl (lock); if (p) items.push_back (std::move (p)); }

private:
    void timerCallback() override
    {
        const juce::ScopedLock sl (lock);
        items.erase (std::remove_if (items.begin(), items.end(), [] (const std::shared_ptr<void>& p) { return p.use_count() <= 1; }), items.end());
    }
    std::vector<std::shared_ptr<void>> items;
    juce::CriticalSection lock;
};

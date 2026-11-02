#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cmath>

// ============================================================================
//  K-54 Looper: 4 Spuren, nimmt den Synth-Klang auf, taktgenau synchronisiert.
//  Taktquelle (automatisch, in dieser Reihenfolge):
//    1. DAW / Host-Transport (Tempo, Taktart, Songposition)
//    2. externe MIDI-Clock (Start/Stop/Continue + 24 Ticks pro Viertel)
//    3. interner Takt mit eigenem Tempo (+ optionalem Klick)
// ============================================================================

enum class LoopState : int { Empty = 0, Armed, Recording, Playing, Overdub, Stopped };
enum class ClockSource : int { Internal = 0, MidiClock, Host };

// Takt fuer einen Audio-Block (wird auch vom Step-Sequencer benutzt)
struct ClockInfo
{
    bool   running = false;
    double beat0 = 0.0;          // Schlag (Viertel) am Blockanfang
    double bpm = 120.0;
    double beatPerSample = 0.0;
    int    bpb = 4;              // Schlaege pro Takt
    ClockSource src = ClockSource::Internal;
    bool   jumped = false;       // Neupositionierung / Start in diesem Block
};

class Looper
{
public:
    static constexpr int numTracks = 4;
    static constexpr double maxSeconds = 40.0;

    struct Settings
    {
        double internalBpm = 120.0;
        int    lengthBars  = 4;
        float  volume[numTracks] { 1, 1, 1, 1 };
        int    source[numTracks] { 0, 0, 0, 0 };   // 0 Synth, 1 Eingang, 2 beides
        bool   click = false;
        bool   monitor = false;                      // Eingang live mithoeren
    };

    // ------------------------------------------------ Bedienung (beliebiger Thread)
    void pressMain (int t)  { request (t, ReqMain); }
    void pressStop (int t)  { request (t, ReqStop); }
    void pressClear (int t) { request (t, ReqClear); }
    void stopAll()          { for (int t = 0; t < numTracks; ++t) request (t, ReqHalt); }
    void clearAll()         { for (int t = 0; t < numTracks; ++t) request (t, ReqClear); }
    void pressTransport()   { transportRequest.store (true); }

    // ------------------------------------------------ Anzeige
    LoopState   getState (int t) const      { return (LoopState) tracks[(size_t) t].state.load(); }
    float       getProgress (int t) const   { return tracks[(size_t) t].progress.load(); }
    float       getLevel (int t) const      { return tracks[(size_t) t].level.load(); }
    int         getLengthBars (int t) const { return tracks[(size_t) t].lenBars.load(); }
    ClockSource getSource() const           { return (ClockSource) source.load(); }
    double      getBpm() const              { return bpmDisplay.load(); }
    double      getBeat() const             { return beatDisplay.load(); }
    int         getBeatsPerBar() const      { return bpbDisplay.load(); }
    bool        isRunning() const           { return runningDisplay.load(); }
    /** true, wenn die DAW ein Tempo meldet (dann gilt immer das DAW-Tempo) */
    bool        usesHostTempo() const       { return hostTempo.load(); }
    bool        anyRecording() const
    {
        for (auto& tr : tracks)
            if (tr.state.load() == (int) LoopState::Recording || tr.state.load() == (int) LoopState::Overdub) return true;
        return false;
    }

    // ------------------------------------------------ Audio-Thread
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        const int maxLen = (int) (sampleRate * maxSeconds);
        for (auto& tr : tracks)
        {
            tr.buf.setSize (2, maxLen, false, true, false);
            tr.buf.clear();
            tr.state = (int) LoopState::Empty;
            tr.lenSamples = 0;
            tr.lenBars = 0;
        }
        mix.setSize (2, juce::jmax (maxBlock, 4096), false, true, false);
        internalRunning = false;
        internalBeat = 0.0;
        lastBeatEnd = 0.0;
        lastSampleBeat = -1.0;
        lastTickAbs = -1;
        sampleCounter = 0;
    }

    // ---- Schritt 1: Takt fuer diesen Block bestimmen (MIDI-Clock, Host, intern) und Tasten abarbeiten
    ClockInfo beginBlock (const juce::MidiBuffer& midi, const juce::Optional<juce::AudioPlayHead::PositionInfo>& pos,
                          const Settings& set)
    {
        // MIDI-Clock-Stand zum Blockanfang merken, dann Block-MIDI auswerten
        const long long tickAbsAtStart = lastTickAbs;
        const long ticksAtStart = clockTicks;
        const bool clockWasActive = clockActive();
        handleMidi (midi);

        if (transportRequest.exchange (false))
        {
            internalRunning = ! internalRunning;
            internalBeat = 0.0;
            lastBeatEnd = 0.0;
            lastSampleBeat = -kEps * 2.0;
        }

        // ---------- Taktquelle bestimmen
        // Tempo und Taktart kommen IMMER von der DAW, sobald sie welche meldet -
        // auch wenn sie gerade steht (dann laeuft PLAY/REC im K-54 im DAW-Tempo).
        double hostBpm = 0.0;
        int hostBpb = 0;
        if (pos.hasValue())
        {
            if (auto b = pos->getBpm()) hostBpm = *b;
            if (auto ts = pos->getTimeSignature())
                hostBpb = juce::jmax (1, juce::roundToInt (ts->numerator * 4.0 / juce::jmax (1, ts->denominator)));
        }
        hostTempo.store (hostBpm > 0.0);
        double bpm = hostBpm > 0.0 ? hostBpm : set.internalBpm, beat0 = internalBeat;
        int bpb = hostBpb > 0 ? hostBpb : 4;
        bool running = false;
        ClockSource src = ClockSource::Internal;

        if (pos.hasValue() && pos->getIsPlaying() && pos->getPpqPosition().hasValue() && pos->getBpm().hasValue())
        {
            src = ClockSource::Host;
            beat0 = *pos->getPpqPosition();
            running = true;
            internalRunning = false;
        }
        else if (clockWasActive || clockActive())
        {
            src = ClockSource::MidiClock;
            bpm = clockBpm;
            const double sinceTick = tickAbsAtStart >= 0 ? (double) (sampleCounter - tickAbsAtStart) : 0.0;
            beat0 = (double) juce::jmax (0L, ticksAtStart) / 24.0 + juce::jmin (sinceTick * bpm / (60.0 * sr), 1.0 / 24.0);
            running = clockRunning && ticksAtStart >= 0;
            internalRunning = false;
        }
        else
            running = internalRunning;

        bpm = juce::jlimit (20.0, 400.0, bpm);
        const double beatPerSample = bpm / (60.0 * sr);

        // Sprung (Neupositionierung im Host) -> keine falsche Taktgrenze
        bool jumped = false;
        if (std::abs (beat0 - lastBeatEnd) > 0.25)
        {
            jumped = true;
            lastBeatEnd = beat0;
            lastSampleBeat = beat0 - kEps * 2.0;   // ein Taktanfang genau hier zaehlt noch
        }

        // ---------- Tasten abarbeiten
        for (int t = 0; t < numTracks; ++t)
            applyRequest (t, running, src, bpm, bpb, set.lengthBars);

        if (! running && src == ClockSource::Internal && internalRunning)
        {
            running = true;                 // REC / PLAY hat den internen Takt gestartet
            beat0 = internalBeat;
            jumped = true;
        }

        clk.running = running; clk.beat0 = beat0; clk.bpm = bpm; clk.beatPerSample = beatPerSample;
        clk.bpb = bpb; clk.src = src; clk.jumped = jumped || ! wasRunning;
        wasRunning = running;
        return clk;

    }

    // ---- Schritt 2: aufnehmen und abspielen.
    //  rec  = Aufnahmesignal "Synth", ext = Audio-Eingang (optional)
    //  outs = je Spur ein Stereo-Puffer (nullptr = in interne Summe), clickOut = Ziel fuer den Klick
    void renderBlock (const juce::AudioBuffer<float>& rec, const juce::AudioBuffer<float>* ext,
                      juce::AudioBuffer<float>* const* outs, juce::AudioBuffer<float>* clickOut, const Settings& set)
    {
        const int n = rec.getNumSamples();
        const int nch = rec.getNumChannels();
        if (n == 0 || nch == 0 || tracks[0].buf.getNumSamples() == 0) return;
        if (mix.getNumSamples() < n) mix.setSize (2, n, false, false, true);
        mix.clear (0, n);

        // ---------- Sample-Schleife
        const bool running = clk.running;
        const double beat0 = clk.beat0, bpm = clk.bpm, beatPerSample = clk.beatPerSample;
        const int bpb = clk.bpb;
        const ClockSource src = clk.src;
        float peak[numTracks] {};
        const float* inL = rec.getReadPointer (0);
        const float* inR = rec.getReadPointer (nch > 1 ? 1 : 0);
        const bool hasExt = ext != nullptr && ext->getNumChannels() >= 2 && ext->getNumSamples() >= n;
        const float* exL = hasExt ? ext->getReadPointer (0) : nullptr;
        const float* exR = hasExt ? ext->getReadPointer (1) : nullptr;
        // Ziel je Spur: eigener Puffer (Einzelausgang) oder Summe
        float* oL[numTracks]; float* oR[numTracks];
        for (int t = 0; t < numTracks; ++t)
        {
            auto* b = outs != nullptr ? outs[t] : nullptr;
            oL[t] = b != nullptr ? b->getWritePointer (0) : mix.getWritePointer (0);
            oR[t] = b != nullptr ? b->getWritePointer (1) : mix.getWritePointer (1);
        }
        float* cL = clickOut != nullptr ? clickOut->getWritePointer (0) : mix.getWritePointer (0);
        float* cR = clickOut != nullptr ? clickOut->getWritePointer (1) : mix.getWritePointer (1);

        if (running)
        {
            double prev = lastSampleBeat;          // Schlag des letzten Samples im vorigen Block
            for (int s = 0; s < n; ++s)
            {
                const double beat = beat0 + s * beatPerSample;
                const double barNow  = std::floor ((beat + kEps) / bpb);
                const bool barCrossed = barNow > std::floor ((prev + kEps) / bpb);
                const float sl = inL[s], sr_ = inR[s];
                const float el = hasExt ? exL[s] : 0.0f, er = hasExt ? exR[s] : 0.0f;

                for (int t = 0; t < numTracks; ++t)
                {
                    auto& tr = tracks[(size_t) t];
                    auto st = (LoopState) tr.state.load (std::memory_order_relaxed);

                    if (st == LoopState::Armed && barCrossed)
                    {
                        startRecording (tr, barNow * bpb, bpm, bpb, set.lengthBars);
                        st = LoopState::Recording;
                    }
                    if (st == LoopState::Empty || st == LoopState::Armed || st == LoopState::Stopped || tr.lenSamples <= 0)
                        continue;

                    // Aufnahmequelle dieser Spur
                    const int srcSel = set.source[t];
                    const float xl = (srcSel != 1 ? sl : 0.0f) + (srcSel != 0 ? el : 0.0f);
                    const float xr = (srcSel != 1 ? sr_ : 0.0f) + (srcSel != 0 ? er : 0.0f);

                    double phase = std::fmod (beat - tr.anchor, tr.lenBeats);
                    if (phase < 0.0) phase += tr.lenBeats;
                    const double fpos = phase / tr.lenBeats * tr.lenSamples;
                    const int i0 = juce::jlimit (0, tr.lenSamples - 1, (int) fpos);

                    if (st == LoopState::Recording)
                    {
                        if (beat < tr.anchor) continue;
                        // Luecken (Tempo-Rundung) mit auffuellen, damit keine alten Reste bleiben
                        int from = (tr.recLast >= 0 && i0 > tr.recLast && i0 - tr.recLast < 64) ? tr.recLast + 1 : i0;
                        for (int i = from; i <= i0; ++i)
                        {
                            tr.buf.setSample (0, i, xl);
                            tr.buf.setSample (1, i, xr);
                        }
                        tr.recLast = i0;
                        const double done = beat - tr.anchor;
                        tr.progress.store ((float) juce::jlimit (0.0, 1.0, done / tr.lenBeats), std::memory_order_relaxed);
                        peak[t] = std::max (peak[t], std::abs (xl));
                        if (done + beatPerSample >= tr.lenBeats)
                        {
                            for (int i = i0 + 1; i < tr.lenSamples && i < i0 + 64; ++i)   // letzte Samples schliessen
                            {
                                tr.buf.setSample (0, i, xl);
                                tr.buf.setSample (1, i, xr);
                            }
                            applySeamFade (tr);
                            tr.state.store ((int) LoopState::Playing);
                        }
                        continue;
                    }

                    // Wiedergabe mit linearer Interpolation
                    const int i1 = (i0 + 1) % tr.lenSamples;
                    const float fr = (float) (fpos - (double) i0);
                    const float* bl = tr.buf.getReadPointer (0);
                    const float* br = tr.buf.getReadPointer (1);
                    const float rl = bl[i0] + (bl[i1] - bl[i0]) * fr;
                    const float rr = br[i0] + (br[i1] - br[i0]) * fr;
                    const float g = set.volume[t];
                    oL[t][s] += rl * g;
                    oR[t][s] += rr * g;
                    peak[t] = std::max (peak[t], std::abs (rl * g));

                    if (st == LoopState::Overdub)
                    {
                        tr.buf.setSample (0, i0, bl[i0] + xl);
                        tr.buf.setSample (1, i0, br[i0] + xr);
                    }
                    tr.progress.store ((float) (phase / tr.lenBeats), std::memory_order_relaxed);
                }

                // Metronom-Klick (nur interner Takt)
                if (set.click && src == ClockSource::Internal)
                {
                    const double frac = beat - std::floor (beat);
                    const double tSec = frac / beatPerSample / sr;
                    if (tSec < 0.04)
                    {
                        const bool accent = std::fmod (std::floor (beat + 1e-9), (double) bpb) < 0.5;
                        const float env = (float) std::exp (-tSec * 110.0);
                        const float c = (float) std::sin (juce::MathConstants<double>::twoPi * (accent ? 1760.0 : 1175.0) * tSec) * env * 0.22f;
                        cL[s] += c; cR[s] += c;
                    }
                }
                prev = beat;
            }
            lastBeatEnd = beat0 + n * beatPerSample;
            lastSampleBeat = prev;
            if (src == ClockSource::Internal) internalBeat = lastBeatEnd;
        }

        for (int t = 0; t < numTracks; ++t)
        {
            auto& lv = tracks[(size_t) t].level;
            lv.store (std::max (peak[t], lv.load (std::memory_order_relaxed) * 0.8f), std::memory_order_relaxed);
        }

        source.store ((int) src);
        bpmDisplay.store (bpm);
        beatDisplay.store (running ? lastBeatEnd : beat0);
        bpbDisplay.store (bpb);
        runningDisplay.store (running);
        sampleCounter += n;
    }

    /** Summe aller Spuren, die in renderBlock ohne eigenen Puffer gelandet sind. */
    const juce::AudioBuffer<float>& internalMix() const { return mix; }

    // ---- Kompakt (wie bisher): io = Synth rein, Loops + Monitor raus
    void process (juce::AudioBuffer<float>& io, const juce::AudioBuffer<float>* ext, const juce::MidiBuffer& midi,
                  const juce::Optional<juce::AudioPlayHead::PositionInfo>& pos, const Settings& set)
    {
        const int n = io.getNumSamples(), nch = io.getNumChannels();
        if (n == 0 || nch == 0) return;
        beginBlock (midi, pos, set);
        renderBlock (io, ext, nullptr, nullptr, set);
        const bool hasExt = ext != nullptr && ext->getNumChannels() >= 2 && ext->getNumSamples() >= n;
        for (int ch = 0; ch < nch; ++ch)
        {
            auto* d = io.getWritePointer (ch);
            const float* m = mix.getReadPointer (ch == 1 ? 1 : 0);
            const float* e = (hasExt && set.monitor) ? ext->getReadPointer (ch == 1 ? 1 : 0) : nullptr;
            for (int s = 0; s < n; ++s)
                d[s] = softLimit (d[s] + m[s] + (e != nullptr ? e[s] : 0.0f));
        }
    }

    static float softLimit (float x) noexcept
    {
        const float a = std::abs (x);
        if (a <= 0.85f) return x;
        return std::copysign (0.85f + 0.15f * std::tanh ((a - 0.85f) / 0.15f), x);
    }

    void pressTransportStart() { if (! internalRunning) transportRequest.store (true); }
    void pressTransportStop()  { if (internalRunning)   transportRequest.store (true); }

private:
    enum Request { ReqNone = 0, ReqMain, ReqStop, ReqClear, ReqHalt };

    struct Track
    {
        juce::AudioBuffer<float> buf;
        std::atomic<int>   state { (int) LoopState::Empty };
        std::atomic<int>   request { ReqNone };
        std::atomic<float> progress { 0.0f }, level { 0.0f };
        std::atomic<int>   lenBars { 0 };
        int    lenSamples = 0, recLast = -1;
        double lenBeats = 4.0, anchor = 0.0;
    };

    void request (int t, Request r)
    {
        if (juce::isPositiveAndBelow (t, numTracks))
            tracks[(size_t) t].request.store (r);
    }

    void startRecording (Track& tr, double anchorBeat, double bpm, int bpb, int bars)
    {
        tr.anchor = anchorBeat;
        tr.lenBeats = (double) (bars * bpb);
        tr.lenSamples = juce::jlimit (1, tr.buf.getNumSamples(), (int) std::lround (tr.lenBeats * 60.0 / bpm * sr));
        tr.recLast = -1;
        tr.lenBars.store (bars);
        tr.progress.store (0.0f);
        tr.state.store ((int) LoopState::Recording);
    }

    void applySeamFade (Track& tr)
    {
        const int fade = juce::jmin ((int) (sr * 0.003), tr.lenSamples / 4);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = tr.buf.getWritePointer (ch);
            for (int i = 0; i < fade; ++i)
            {
                const float g = (float) i / (float) fade;
                d[i] *= g;
                d[tr.lenSamples - 1 - i] *= g;
            }
        }
    }

    void clearTrack (Track& tr)
    {
        tr.state.store ((int) LoopState::Empty);
        tr.lenSamples = 0; tr.lenBars.store (0); tr.progress.store (0.0f);
    }

    void applyRequest (int t, bool running, ClockSource src, double bpm, int bpb, int bars)
    {
        auto& tr = tracks[(size_t) t];
        const int req = tr.request.exchange (ReqNone);
        if (req == ReqNone) return;
        const auto st = (LoopState) tr.state.load();

        switch (req)
        {
            case ReqClear: clearTrack (tr); return;

            case ReqHalt:
                if (st == LoopState::Playing || st == LoopState::Overdub) tr.state.store ((int) LoopState::Stopped);
                else if (st == LoopState::Armed || st == LoopState::Recording) clearTrack (tr);
                return;

            case ReqStop:
                if (st == LoopState::Playing || st == LoopState::Overdub) tr.state.store ((int) LoopState::Stopped);
                else if (st == LoopState::Stopped)                        tr.state.store ((int) LoopState::Playing);
                else if (st == LoopState::Armed || st == LoopState::Recording) clearTrack (tr);
                return;

            default: break;
        }

        // Haupttaste wie beim Looper-Pedal: REC -> (automatisch) PLAY -> DUB -> PLAY ...
        switch (st)
        {
            case LoopState::Empty:
                if (! running && src == ClockSource::Internal)
                {
                    // noch kein Takt: interner Takt startet sofort, Aufnahme ab Takt 1
                    internalRunning = true;
                    internalBeat = 0.0;
                    lastBeatEnd = 0.0;
                    lastSampleBeat = -kEps * 2.0;
                    startRecording (tr, 0.0, bpm, bpb, bars);
                }
                else
                    tr.state.store ((int) LoopState::Armed);   // startet am naechsten Taktanfang
                break;
            case LoopState::Armed:     clearTrack (tr); break;
            case LoopState::Recording: break;                  // laeuft bis zur eingestellten Laenge
            case LoopState::Playing:   tr.state.store ((int) LoopState::Overdub); break;
            case LoopState::Overdub:   tr.state.store ((int) LoopState::Playing); break;
            case LoopState::Stopped:   tr.state.store ((int) LoopState::Playing); break;
        }
    }

    void handleMidi (const juce::MidiBuffer& midi)
    {
        for (const auto meta : midi)
        {
            const auto& m = meta.getMessage();
            const long long abs = sampleCounter + meta.samplePosition;
            if (m.isMidiClock())
            {
                if (lastTickAbs >= 0)
                {
                    const double interval = (double) (abs - lastTickAbs);
                    if (interval > 0.0 && interval < sr * 0.5)
                    {
                        const double inst = 60.0 * sr / (interval * 24.0);
                        clockBpm = clockBpm <= 0.0 ? inst : clockBpm * 0.92 + inst * 0.08;
                    }
                }
                lastTickAbs = abs;
                if (clockRunning) ++clockTicks;
            }
            else if (m.isMidiStart())    { clockRunning = true; clockTicks = -1; }  // erster Tick = Schlag 1
            else if (m.isMidiContinue()) { clockRunning = true; }
            else if (m.isMidiStop())     { clockRunning = false; }
            else if (m.isController() && m.getControllerValue() >= 64)
            {
                // Fernsteuerung per MIDI-CC (z. B. Fussschalter oder Pads)
                const int cc = m.getControllerNumber();
                if (cc >= 102 && cc <= 105)      pressMain (cc - 102);
                else if (cc >= 106 && cc <= 109) pressStop (cc - 106);
                else if (cc == 110)              pressTransport();
                else if (cc == 111)              stopAll();
            }
        }
    }

    bool clockActive() const { return lastTickAbs >= 0 && (sampleCounter - lastTickAbs) < (long long) (sr * 0.5) && clockBpm > 0.0; }

    double sr = 48000.0;
    std::array<Track, numTracks> tracks;
    juce::AudioBuffer<float> mix;
    ClockInfo clk;
    bool wasRunning = false;
    std::atomic<bool> transportRequest { false };

    bool   internalRunning = false;
    static constexpr double kEps = 1e-7;
    double internalBeat = 0.0, lastBeatEnd = 0.0, lastSampleBeat = -1.0;

    long long sampleCounter = 0, lastTickAbs = -1;
    long   clockTicks = -1;
    double clockBpm = 0.0;
    bool   clockRunning = false;

    std::atomic<int>    source { 0 }, bpbDisplay { 4 };
    std::atomic<double> bpmDisplay { 120.0 }, beatDisplay { 0.0 };
    std::atomic<bool>   runningDisplay { false }, hostTempo { false };
};

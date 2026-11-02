#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

// ============================================================================
//  K-54 Klangengine
//  3 Oszillatoren (PolyBLEP) + Rauschen -> 4-Pol-Ladder-Filter (TPT, 2x) -> VCA
// ============================================================================

struct SynthParams
{
    float cutoff = 1200, resonance = 0.25f, envAmount = 0.45f, drive = 0.2f;
    float ampA = 0.005f, ampD = 0.4f, ampS = 0.8f, ampR = 0.25f;
    float fltA = 0.005f, fltD = 0.35f, fltS = 0.2f, fltR = 0.25f;
    float glide = 0, keyTrack = 0.5f, velFilter = 0.2f, velAmp = 0.3f, masterGain = 0.5f;
    int   wave[3]   { 1, 1, 2 };
    int   octave[3] { 2, 2, 1 };          // Index 0..4 = 32' .. 2'
    float tune[3]   { 0.0f, 0.07f, -0.05f };
    float level[3]  { 0.8f, 0.6f, 0.0f };
    float noise = 0, pulseWidth = 0.25f;
    float lfoRate = 5, lfoPitch = 0, lfoFilter = 0;
    int   lfoWave = 0, voiceMode = 1;     // 0 Mono, 1 Legato, 2 Poly
    float bendRange = 2, drift = 0.3f;
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
            case 0: // Dreieck
                out = (float) (1.0 - 4.0 * std::abs (t - 0.5));
                break;
            case 1: // Saege
                out = (float) (2.0 * t - 1.0 - blep (t, dt));
                break;
            default: // Rechteck / Puls
            {
                const double w = (wave == 2) ? 0.5 : (double) pw;
                double v = t < w ? 1.0 : -1.0;
                v += blep (t, dt);
                v -= blep (std::fmod (t + 1.0 - w, 1.0), dt);
                out = (float) (v - (2.0 * w - 1.0));   // DC-frei
                break;
            }
        }

        phase += inc;
        if (phase >= 1.0) phase -= std::floor (phase);
        return out;
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
//  4-Pol-Ladder (Zero-Delay-Feedback / TPT) mit Saettigung im Rueckkopplungspfad
class LadderFilter
{
public:
    void prepare (double sampleRate) { fs = sampleRate * 2.0; reset(); }
    void reset() { s[0] = s[1] = s[2] = s[3] = 0.0; }

    // x: Eingang, fc: Cutoff in Hz, res: 0..1, driveGain >= 1
    float process (float x, float fc, float res, float driveGain) noexcept
    {
        fc = juce::jlimit (16.0f, (float) (fs * 0.42), fc);
        const double g = std::tan (juce::MathConstants<double>::pi * fc / fs);
        const double G = g / (1.0 + g);
        const double b = 1.0 / (1.0 + g);
        const double k = res * 4.15;
        const double G2 = G * G, G3 = G2 * G, G4 = G3 * G;
        const double comp = 1.0 + 0.45 * k;            // Bass-Ausgleich bei hoher Resonanz

        double acc = 0.0;
        for (int i = 0; i < 2; ++i)                    // 2-fach Oversampling
        {
            const double S = G3 * b * s[0] + G2 * b * s[1] + G * b * s[2] + b * s[3];
            double u = (x * comp - k * S) / (1.0 + k * G4);
            u = std::tanh (u * driveGain) / std::sqrt (driveGain);

            double y = u;
            for (int st = 0; st < 4; ++st)
            {
                const double v = (y - s[st]) * G;
                y = v + s[st];
                s[st] = y + v;
            }
            acc += y;
        }
        return (float) (acc * 0.5);
    }

private:
    double fs = 88200.0;
    double s[4] {};
};

// ---------------------------------------------------------------------------
class Voice
{
public:
    void prepare (double sampleRate, int seed)
    {
        sr = sampleRate;
        filter.prepare (sampleRate);
        ampEnv.setSampleRate (sampleRate);
        fltEnv.setSampleRate (sampleRate);
        rng.setSeed (seed);
        for (auto& o : osc) o.setRandomPhase (rng);
    }

    bool  isActive() const noexcept   { return active; }
    bool  isReleasing() const noexcept{ return active && ! gate; }
    int   getNote() const noexcept    { return note; }
    juce::uint64 getAge() const noexcept { return age; }

    void noteOn (int newNote, float vel, bool retrigger, float glideTime, juce::uint64 stamp, const SynthParams& p)
    {
        const bool wasActive = active;
        note = newNote;
        velocity = vel;
        targetPitch = (float) newNote;
        age = stamp;

        if (! wasActive || glideTime <= 0.0001f)
            currentPitch = targetPitch;

        updateEnvelopes (p);
        if (retrigger || ! wasActive)
        {
            ampEnv.noteOn();
            fltEnv.noteOn();
        }
        if (! wasActive)
            filter.reset();

        active = true;
        gate = true;
    }

    void setPitchOnly (int newNote) { note = newNote; targetPitch = (float) newNote; }

    void noteOff()
    {
        gate = false;
        ampEnv.noteOff();
        fltEnv.noteOff();
    }

    void kill() { active = false; gate = false; ampEnv.reset(); fltEnv.reset(); filter.reset(); }

    void updateEnvelopes (const SynthParams& p)
    {
        ampEnv.setParameters ({ p.ampA, p.ampD, p.ampS, p.ampR });
        fltEnv.setParameters ({ p.fltA, p.fltD, p.fltS, p.fltR });
    }

    // Rendert additiv in out[0..n)
    void render (float* out, int n, const SynthParams& p, const float* lfo, float baseCutoff,
                 float pitchMod, float wheelVib, juce::Random& noiseRng)
    {
        if (! active) return;
        updateEnvelopes (p);

        const float glideCoef = p.glide <= 0.0001f ? 1.0f
                              : 1.0f - std::exp (-1.0f / (p.glide * 0.35f * (float) sr));
        const float velA = 1.0f - p.velAmp + p.velAmp * velocity;
        const float envDepthOct = p.envAmount * 6.0f * (1.0f - p.velFilter + p.velFilter * velocity);
        const float keyOct = p.keyTrack * ((float) note - 60.0f) / 12.0f;
        const float vibSemis = p.lfoPitch * p.lfoPitch * 12.0f + wheelVib;
        const float lfoFiltOct = p.lfoFilter * 3.0f;
        const float driveGain = 1.0f + p.drive * 5.0f;
        const float inGain = 0.42f * (1.0f + p.drive * 1.5f);

        double octOffset[3];
        for (int i = 0; i < 3; ++i)
            octOffset[i] = (double) (p.octave[i] - 2) * 12.0 + p.tune[i];

        for (int s = 0; s < n; ++s)
        {
            currentPitch += (targetPitch - currentPitch) * glideCoef;

            // analoger "Drift" je Oszillator
            for (int i = 0; i < 3; ++i)
            {
                driftState[i] += (rng.nextFloat() - 0.5f) * 0.004f;
                driftState[i] *= 0.9997f;
            }

            const float l = lfo[s];
            const double basePitch = currentPitch + pitchMod + l * vibSemis;

            float mix = 0.0f;
            for (int i = 0; i < 3; ++i)
            {
                if (p.level[i] <= 0.0001f) { oscDummyAdvance (i, basePitch + octOffset[i]); continue; }
                const double pitch = basePitch + octOffset[i] + driftState[i] * p.drift * 0.6;
                const double freq  = 440.0 * std::exp2 ((pitch - 69.0) / 12.0);
                mix += osc[(size_t) i].process (freq / sr, p.wave[i], p.pulseWidth) * p.level[i];
            }
            if (p.noise > 0.0001f)
                mix += (noiseRng.nextFloat() * 2.0f - 1.0f) * p.noise * 0.8f;

            const float fe = fltEnv.getNextSample();
            const float ae = ampEnv.getNextSample();

            const float fc = baseCutoff * std::exp2 (fe * envDepthOct + keyOct + l * lfoFiltOct);
            const float y  = filter.process (mix * inGain, fc, p.resonance, driveGain);

            out[s] += y * ae * velA;
        }

        if (! ampEnv.isActive())
            kill();
    }

private:
    void oscDummyAdvance (int i, double pitch)
    {
        // stummer Oszillator laeuft frei weiter (wie analog)
        const double freq = 440.0 * std::exp2 ((pitch - 69.0) / 12.0);
        osc[(size_t) i].process (freq / sr, 1, 0.5f);
    }

    double sr = 44100.0;
    std::array<Oscillator, 3> osc;
    LadderFilter filter;
    juce::ADSR ampEnv, fltEnv;
    juce::Random rng;
    float driftState[3] {};
    bool active = false, gate = false;
    int note = 60;
    float velocity = 1.0f;
    float currentPitch = 60.0f, targetPitch = 60.0f;
    juce::uint64 age = 0;
};

// ---------------------------------------------------------------------------
class SynthEngine
{
public:
    static constexpr int numVoices = 8;

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        for (int i = 0; i < numVoices; ++i)
            voices[(size_t) i].prepare (sampleRate, 5401 + i * 97);
        lfoBuf.assign ((size_t) std::max (maxBlock, 64), 0.0f);
        cutoffSmooth.reset (sampleRate, 0.03);
        cutoffSmooth.setCurrentAndTargetValue (params.cutoff);
        allNotesOff (true);
        dcX = dcY = 0.0f;
    }

    void setParams (const SynthParams& p)
    {
        if (p.voiceMode != params.voiceMode)
        {
            params = p;
            allNotesOff (true);
        }
        params = p;
        cutoffSmooth.setTargetValue (p.cutoff);
    }

    void allNotesOff (bool hard)
    {
        for (auto& v : voices) { if (hard) v.kill(); else v.noteOff(); }
        noteStack.clear();
        pendingOffs.clear();
    }

    void render (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        const int total = buffer.getNumSamples();
        buffer.clear();
        if ((int) lfoBuf.size() < total) lfoBuf.resize ((size_t) total);

        int pos = 0;
        for (const auto meta : midi)
        {
            const int evPos = juce::jlimit (0, total, meta.samplePosition);
            renderRange (buffer, pos, evPos - pos);
            handleMidi (meta.getMessage());
            pos = evPos;
        }
        renderRange (buffer, pos, total - pos);

        // Mono -> alle Kanaele
        for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, buffer, 0, 0, total);
    }

private:
    void renderRange (juce::AudioBuffer<float>& buffer, int start, int num)
    {
        while (num > 0)
        {
            const int n = std::min (num, 32);
            float* out = buffer.getWritePointer (0, start);

            // LFO
            for (int s = 0; s < n; ++s)
            {
                const double inc = params.lfoRate / sr;
                float v = 0.0f;
                switch (params.lfoWave)
                {
                    case 0: v = (float) (1.0 - 4.0 * std::abs (lfoPhase - 0.5)); break;
                    case 1: v = (float) (1.0 - 2.0 * lfoPhase); break;
                    case 2: v = lfoPhase < 0.5 ? 1.0f : -1.0f; break;
                    default: v = shValue; break;
                }
                lfoPhase += inc;
                if (lfoPhase >= 1.0) { lfoPhase -= 1.0; shValue = noiseRng.nextFloat() * 2.0f - 1.0f; }
                lfoBuf[(size_t) s] = v;
            }

            const float baseCutoff = cutoffSmooth.getNextValue();
            cutoffSmooth.skip (n - 1);

            // Modrad (CC1) gibt zusaetzliches Vibrato (bis 0,6 Halbtoene)
            const float wheelVib = modWheel * modWheel * 0.6f;

            const float pitchMod = bend * params.bendRange;

            for (auto& v : voices)
                v.render (out, n, params, lfoBuf.data(), baseCutoff, pitchMod, wheelVib, noiseRng);

            // DC-Blocker, Lautstaerke, sanfte Saettigung
            for (int s = 0; s < n; ++s)
            {
                const float x = out[s];
                dcY = x - dcX + 0.9995f * dcY;
                dcX = x;
                out[s] = std::tanh (dcY * params.masterGain * 1.6f) * 0.85f;
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
        else if (m.isController())
        {
            if (m.getControllerNumber() == 1)
                modWheel = (float) m.getControllerValue() / 127.0f;
            else if (m.getControllerNumber() == 64)
            {
                const bool down = m.getControllerValue() >= 64;
                if (sustain && ! down)
                {
                    auto offs = pendingOffs;
                    pendingOffs.clear();
                    sustain = false;
                    for (int n : offs) noteOff (n);
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
        ++stamp;
        if (params.voiceMode == 2)
        {
            Voice* target = nullptr;
            for (auto& v : voices) if (v.isActive() && v.getNote() == note) { target = &v; break; }
            if (! target) for (auto& v : voices) if (! v.isActive()) { target = &v; break; }
            if (! target) // Stehlen: zuerst ausklingende, dann aelteste Stimme
            {
                for (auto& v : voices)
                    if (v.isReleasing() && (! target || v.getAge() < target->getAge())) target = &v;
                if (! target)
                    for (auto& v : voices)
                        if (! target || v.getAge() < target->getAge()) target = &v;
            }
            // Glide im Poly-Modus: Startton = zuletzt gespielte Note
            if (! target->isActive() && params.glide > 0.0001f)
            {
                target->noteOn (lastNote, vel, true, 0.0f, stamp, params);
                target->setPitchOnly (note);
            }
            else
                target->noteOn (note, vel, true, params.glide, stamp, params);
        }
        else
        {
            const bool hadNotes = ! noteStack.empty();
            noteStack.erase (std::remove (noteStack.begin(), noteStack.end(), note), noteStack.end());
            noteStack.push_back (note);
            auto& v = voices[0];
            const bool retrig = (params.voiceMode == 0) || ! hadNotes || ! v.isActive();
            v.noteOn (note, vel, retrig, params.glide, stamp, params);
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
            if (noteStack.empty())
                v.noteOff();
            else if (v.getNote() == note)
                v.setPitchOnly (noteStack.back());   // zurueck zur gehaltenen Note
        }
    }

    double sr = 44100.0;
    SynthParams params;
    std::array<Voice, numVoices> voices;
    std::vector<float> lfoBuf;
    std::vector<int> noteStack, pendingOffs;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> cutoffSmooth { 1200.0f };
    juce::Random noiseRng { 54 };
    double lfoPhase = 0.0;
    float shValue = 0.0f, bend = 0.0f, modWheel = 0.0f;
    bool sustain = false;
    int lastNote = 60;
    juce::uint64 stamp = 0;
    float dcX = 0.0f, dcY = 0.0f;
};

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Reihenfolge der Parameter = Reihenfolge auf den 8 Reglern des
// Komplete Kontrol A61 (je 8 Parameter pro Seite).
namespace ParamID
{
    // Seite 1 – Performance
    inline constexpr const char* cutoff     = "cutoff";
    inline constexpr const char* resonance  = "resonance";
    inline constexpr const char* envAmount  = "envAmount";
    inline constexpr const char* drive      = "drive";
    inline constexpr const char* ampAttack  = "ampAttack";
    inline constexpr const char* ampDecay   = "ampDecay";
    inline constexpr const char* ampRelease = "ampRelease";
    inline constexpr const char* glide      = "glide";
    // Seite 2 – Filter-Huellkurve
    inline constexpr const char* fltAttack  = "fltAttack";
    inline constexpr const char* fltDecay   = "fltDecay";
    inline constexpr const char* fltSustain = "fltSustain";
    inline constexpr const char* fltRelease = "fltRelease";
    inline constexpr const char* ampSustain = "ampSustain";
    inline constexpr const char* keyTrack   = "keyTrack";
    inline constexpr const char* velFilter  = "velFilter";
    inline constexpr const char* master     = "master";
    // Seite 3 – Oszillatoren
    inline constexpr const char* osc1Wave   = "osc1Wave";
    inline constexpr const char* osc1Oct    = "osc1Oct";
    inline constexpr const char* osc2Wave   = "osc2Wave";
    inline constexpr const char* osc2Oct    = "osc2Oct";
    inline constexpr const char* osc2Tune   = "osc2Tune";
    inline constexpr const char* osc3Wave   = "osc3Wave";
    inline constexpr const char* osc3Oct    = "osc3Oct";
    inline constexpr const char* osc3Tune   = "osc3Tune";
    // Seite 4 – Mixer
    inline constexpr const char* osc1Level  = "osc1Level";
    inline constexpr const char* osc2Level  = "osc2Level";
    inline constexpr const char* osc3Level  = "osc3Level";
    inline constexpr const char* noiseLevel = "noiseLevel";
    inline constexpr const char* pulseWidth = "pulseWidth";
    inline constexpr const char* lfoRate    = "lfoRate";
    inline constexpr const char* lfoPitch   = "lfoPitch";
    inline constexpr const char* lfoFilter  = "lfoFilter";
    // Seite 5 – Spielweise
    inline constexpr const char* lfoWave    = "lfoWave";
    inline constexpr const char* voiceMode  = "voiceMode";
    inline constexpr const char* bendRange  = "bendRange";
    inline constexpr const char* velAmp     = "velAmp";
    inline constexpr const char* drift      = "drift";
    // Looper (werden von Presets nicht veraendert)
    inline constexpr const char* loopBpm    = "loopBpm";
    inline constexpr const char* loopBars   = "loopBars";
    inline constexpr const char* loopClick  = "loopClick";
    inline constexpr const char* loop1Vol   = "loop1Vol";
    inline constexpr const char* loop2Vol   = "loop2Vol";
    inline constexpr const char* loop3Vol   = "loop3Vol";
    inline constexpr const char* loop4Vol   = "loop4Vol";
    inline constexpr const char* loop1Src   = "loop1Src";
    inline constexpr const char* loop2Src   = "loop2Src";
    inline constexpr const char* loop3Src   = "loop3Src";
    inline constexpr const char* loop4Src   = "loop4Src";
    inline constexpr const char* loopInMode = "loopInMode";
    inline constexpr const char* loopInGain = "loopInGain";
    inline constexpr const char* loopMonitor= "loopMonitor";
    // Sequencer / Drums / Ausgaenge
    inline constexpr const char* seqPattern  = "seqPattern";
    inline constexpr const char* seqDrumsOn  = "seqDrumsOn";
    inline constexpr const char* seqSynthOn  = "seqSynthOn";
    inline constexpr const char* seqDrumLen  = "seqDrumLen";
    inline constexpr const char* seqSynthLen = "seqSynthLen";
    inline constexpr const char* seqSwing    = "seqSwing";
    inline constexpr const char* seqRoot     = "seqRoot";
    inline constexpr const char* seqGate     = "seqGate";
    inline constexpr const char* drumMaster  = "drumMaster";
    inline constexpr const char* outSynth    = "outSynth";
    inline juce::String drum (int lane, const char* what) { return "drum" + juce::String (lane + 1) + what; }  // Level Tune Decay Tone Pan Mute Out
    inline juce::String outLoop (int t) { return "outLoop" + juce::String (t + 1); }

    /** Parameter, die von Synth-Presets NICHT veraendert werden (Looper, Sequencer, Drums, Ausgaenge). */
    inline bool isGlobalParam (const juce::String& id)
    {
        return id.startsWith ("loop") || id.startsWith ("seq") || id.startsWith ("drum") || id.startsWith ("out");
    }
    inline bool isLooperParam (const juce::String& id) { return isGlobalParam (id); }
}

namespace ParamChoices
{
    inline const juce::StringArray waves   { "Triangle", "Saw", "Square", "Pulse" };
    inline const juce::StringArray octaves { "32'", "16'", "8'", "4'", "2'" };   // -2 .. +2 Oktaven
    inline const juce::StringArray lfoWaves{ "Tri", "Saw", "Square", "S&H" };
    inline const juce::StringArray modes   { "Mono", "Legato", "Poly" };
    inline const juce::StringArray loopBars{ "1 Bar", "2 Bars", "4 Bars", "8 Bars" };
    inline constexpr int loopBarValues[]   { 1, 2, 4, 8 };
    inline const juce::StringArray loopSources { "Synth", "Input", "Synth + In" };
    inline const juce::StringArray inputModes  { "Stereo 1/2", "Mono In 1", "Mono In 2", "Mono 1+2" };
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    using P  = AudioParameterFloat;
    using C  = AudioParameterChoice;
    using PA = AudioParameterFloatAttributes;

    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    int v = 1;

    auto timeRange = [] (float maxS) { NormalisableRange<float> r (0.001f, maxS); r.setSkewForCentre (0.35f); return r; };
    auto secs = PA().withLabel ("s").withStringFromValueFunction ([] (float x, int)
                {
                    return x < 1.0f ? String (roundToInt (x * 1000.0f)) + " ms" : String (x, 2) + " s";
                });
    auto pct  = PA().withStringFromValueFunction ([] (float x, int) { return String (roundToInt (x * 100.0f)) + " %"; });

    NormalisableRange<float> cutoffRange (20.0f, 20000.0f); cutoffRange.setSkewForCentre (1000.0f);
    auto hz = PA().withLabel ("Hz").withStringFromValueFunction ([] (float x, int)
              {
                  return x >= 1000.0f ? String (x / 1000.0f, 2) + " kHz" : String (roundToInt (x)) + " Hz";
              });

    // Seite 1
    p.push_back (std::make_unique<P> (ParameterID { ParamID::cutoff, v },     "Cutoff",     cutoffRange, 1200.0f, hz));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::resonance, v },  "Emphasis",   NormalisableRange<float> (0.0f, 1.0f), 0.25f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::envAmount, v },  "Contour",    NormalisableRange<float> (0.0f, 1.0f), 0.45f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::drive, v },      "Drive",      NormalisableRange<float> (0.0f, 1.0f), 0.2f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::ampAttack, v },  "Attack",     timeRange (10.0f), 0.005f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::ampDecay, v },   "Decay",      timeRange (10.0f), 0.4f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::ampRelease, v }, "Release",    timeRange (10.0f), 0.25f, secs));
    {
        NormalisableRange<float> g (0.0f, 2.0f); g.setSkewForCentre (0.25f);
        p.push_back (std::make_unique<P> (ParameterID { ParamID::glide, v },  "Glide",      g, 0.0f, secs));
    }
    // Seite 2
    p.push_back (std::make_unique<P> (ParameterID { ParamID::fltAttack, v },  "F Attack",   timeRange (10.0f), 0.005f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::fltDecay, v },   "F Decay",    timeRange (10.0f), 0.35f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::fltSustain, v }, "F Sustain",  NormalisableRange<float> (0.0f, 1.0f), 0.2f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::fltRelease, v }, "F Release",  timeRange (10.0f), 0.25f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::ampSustain, v }, "Sustain",    NormalisableRange<float> (0.0f, 1.0f), 0.8f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::keyTrack, v },   "Key Track",  NormalisableRange<float> (0.0f, 1.0f), 0.5f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::velFilter, v },  "Vel>Filter", NormalisableRange<float> (0.0f, 1.0f), 0.2f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::master, v },     "Volume",     NormalisableRange<float> (-36.0f, 6.0f), -6.0f,
                                      PA().withLabel ("dB").withStringFromValueFunction ([] (float x, int) { return String (x, 1) + " dB"; })));
    // Seite 3
    p.push_back (std::make_unique<C> (ParameterID { ParamID::osc1Wave, v },   "Osc1 Wave",  ParamChoices::waves, 1));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::osc1Oct, v },    "Osc1 Range", ParamChoices::octaves, 2));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::osc2Wave, v },   "Osc2 Wave",  ParamChoices::waves, 1));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::osc2Oct, v },    "Osc2 Range", ParamChoices::octaves, 2));
    auto semis = PA().withLabel ("st").withStringFromValueFunction ([] (float x, int) { return (x > 0 ? "+" : "") + String (x, 2) + " st"; });
    p.push_back (std::make_unique<P> (ParameterID { ParamID::osc2Tune, v },   "Osc2 Tune",  NormalisableRange<float> (-7.0f, 7.0f), 0.07f, semis));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::osc3Wave, v },   "Osc3 Wave",  ParamChoices::waves, 2));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::osc3Oct, v },    "Osc3 Range", ParamChoices::octaves, 1));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::osc3Tune, v },   "Osc3 Tune",  NormalisableRange<float> (-7.0f, 7.0f), -0.05f, semis));
    // Seite 4
    p.push_back (std::make_unique<P> (ParameterID { ParamID::osc1Level, v },  "Osc1 Level", NormalisableRange<float> (0.0f, 1.0f), 0.8f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::osc2Level, v },  "Osc2 Level", NormalisableRange<float> (0.0f, 1.0f), 0.6f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::osc3Level, v },  "Osc3 Level", NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::noiseLevel, v }, "Noise",      NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::pulseWidth, v }, "Pulse Width",NormalisableRange<float> (0.05f, 0.5f), 0.25f, pct));
    {
        NormalisableRange<float> r (0.05f, 20.0f); r.setSkewForCentre (3.0f);
        p.push_back (std::make_unique<P> (ParameterID { ParamID::lfoRate, v }, "LFO Rate", r, 5.0f,
                                          PA().withLabel ("Hz").withStringFromValueFunction ([] (float x, int) { return String (x, 2) + " Hz"; })));
    }
    p.push_back (std::make_unique<P> (ParameterID { ParamID::lfoPitch, v },   "LFO>Pitch",  NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::lfoFilter, v },  "LFO>Filter", NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    // Seite 5
    p.push_back (std::make_unique<C> (ParameterID { ParamID::lfoWave, v },    "LFO Wave",   ParamChoices::lfoWaves, 0));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::voiceMode, v },  "Voice Mode", ParamChoices::modes, 1));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::bendRange, v },  "Bend Range", NormalisableRange<float> (1.0f, 12.0f, 1.0f), 2.0f,
                                      PA().withStringFromValueFunction ([] (float x, int) { return String (roundToInt (x)) + " st"; })));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::velAmp, v },     "Vel>Amp",    NormalisableRange<float> (0.0f, 1.0f), 0.3f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::drift, v },      "Drift",      NormalisableRange<float> (0.0f, 1.0f), 0.3f, pct));

    // Looper
    p.push_back (std::make_unique<P> (ParameterID { ParamID::loopBpm, v },    "Loop Tempo", NormalisableRange<float> (40.0f, 220.0f, 0.1f), 120.0f,
                                      PA().withLabel ("BPM").withStringFromValueFunction ([] (float x, int) { return String (x, 1) + " BPM"; })));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::loopBars, v },   "Loop Length", ParamChoices::loopBars, 2));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::loopClick, v }, "Loop Click", false));
    const char* vols[] { ParamID::loop1Vol, ParamID::loop2Vol, ParamID::loop3Vol, ParamID::loop4Vol };
    for (int i = 0; i < 4; ++i)
        p.push_back (std::make_unique<P> (ParameterID { vols[i], v }, "Loop " + String (i + 1) + " Vol",
                                          NormalisableRange<float> (0.0f, 1.5f), 1.0f, pct));

    const char* srcs[] { ParamID::loop1Src, ParamID::loop2Src, ParamID::loop3Src, ParamID::loop4Src };
    for (int i = 0; i < 4; ++i)
        p.push_back (std::make_unique<C> (ParameterID { srcs[i], v }, "Loop " + String (i + 1) + " Source", ParamChoices::loopSources, 0));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::loopInMode, v }, "Input Channels", ParamChoices::inputModes, 0));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::loopInGain, v }, "Input Gain", NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                                      PA().withLabel ("dB").withStringFromValueFunction ([] (float x, int) { return (x > 0 ? "+" : "") + String (x, 1) + " dB"; })));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::loopMonitor, v }, "Input Monitor", false));

    // ---- Step-Sequencer
    p.push_back (std::make_unique<C> (ParameterID { ParamID::seqPattern, v }, "Pattern", StringArray { "A", "B", "C", "D", "E", "F", "G", "H" }, 0));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::seqDrumsOn, v }, "Drum Seq On", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::seqSynthOn, v }, "Synth Seq On", false));
    StringArray stepNames;
    for (int i = 1; i <= 16; ++i) stepNames.add (String (i) + (i == 1 ? " STEP" : " STEPS"));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::seqDrumLen, v },  "Drum Steps",  stepNames, 15));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::seqSynthLen, v }, "Synth Steps", stepNames, 15));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::seqSwing, v }, "Swing", NormalisableRange<float> (0.0f, 0.6f), 0.0f,
                                      PA().withStringFromValueFunction ([] (float x, int) { return String (roundToInt (50.0f + x * 50.0f)) + " %"; })));
    p.push_back (std::make_unique<AudioParameterInt>  (ParameterID { ParamID::seqRoot, v }, "Seq Root", 24, 60, 36,
                                      AudioParameterIntAttributes().withStringFromValueFunction ([] (int x, int) { return MidiMessage::getMidiNoteName (x, true, true, 3); })));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::seqGate, v }, "Seq Gate", NormalisableRange<float> (0.1f, 1.0f), 0.5f, pct));

    // ---- Drum-Computer
    p.push_back (std::make_unique<P> (ParameterID { ParamID::drumMaster, v }, "Drum Volume", NormalisableRange<float> (-36.0f, 6.0f), -3.0f,
                                      PA().withLabel ("dB").withStringFromValueFunction ([] (float x, int) { return String (x, 1) + " dB"; })));
    static const char* dn[8] { "Kick", "Snare", "Clap", "Closed Hat", "Open Hat", "Tom Low", "Tom High", "Cowbell" };
    static const float defLvl[8] { 0.95f, 0.8f, 0.7f, 0.55f, 0.5f, 0.7f, 0.65f, 0.5f };
    static const float defPan[8] { 0.0f, 0.0f, 0.1f, 0.25f, 0.25f, -0.3f, 0.3f, -0.2f };
    for (int i = 0; i < 8; ++i)
    {
        const String nm (dn[i]);
        p.push_back (std::make_unique<P> (ParameterID { ParamID::drum (i, "Level"), v }, nm + " Level", NormalisableRange<float> (0.0f, 1.5f), defLvl[i], pct));
        p.push_back (std::make_unique<P> (ParameterID { ParamID::drum (i, "Tune"),  v }, nm + " Tune",  NormalisableRange<float> (-12.0f, 12.0f), 0.0f, semis));
        p.push_back (std::make_unique<P> (ParameterID { ParamID::drum (i, "Decay"), v }, nm + " Decay", NormalisableRange<float> (0.0f, 1.0f), 0.4f, pct));
        p.push_back (std::make_unique<P> (ParameterID { ParamID::drum (i, "Tone"),  v }, nm + " Tone",  NormalisableRange<float> (0.0f, 1.0f), 0.5f, pct));
        p.push_back (std::make_unique<P> (ParameterID { ParamID::drum (i, "Pan"),   v }, nm + " Pan",   NormalisableRange<float> (-1.0f, 1.0f), defPan[i],
                                          PA().withStringFromValueFunction ([] (float x, int) { return std::abs (x) < 0.01f ? String ("C") : (x < 0 ? "L" : "R") + String (roundToInt (std::abs (x) * 100.0f)); })));
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::drum (i, "Mute"), v }, nm + " Mute", false));
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::drum (i, "Out"),  v }, nm + " Separate Out", false));
    }

    // ---- Einzelausgaenge fuer Synth und Loops
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::outSynth, v }, "Synth Separate Out", false));
    for (int t = 0; t < 4; ++t)
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::outLoop (t), v }, "Loop " + String (t + 1) + " Separate Out", false));

    return { p.begin(), p.end() };
}

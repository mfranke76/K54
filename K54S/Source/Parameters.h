#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// ============================================================================
//  K-54 S Parameter
//  Reihenfolge der ersten Seiten = Reihenfolge auf den 8 Reglern des A61
// ============================================================================
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
    // Seite 2 – Wavetable-Positionen + Filter-Huellkurve
    inline constexpr const char* osc1Pos    = "osc1Pos";
    inline constexpr const char* osc2Pos    = "osc2Pos";
    inline constexpr const char* osc3Pos    = "osc3Pos";
    inline constexpr const char* fltAttack  = "fltAttack";
    inline constexpr const char* fltDecay   = "fltDecay";
    inline constexpr const char* fltSustain = "fltSustain";
    inline constexpr const char* fltRelease = "fltRelease";
    inline constexpr const char* ampSustain = "ampSustain";
    // Seite 3 – Oszillatoren
    inline constexpr const char* osc1Wave   = "osc1Wave";
    inline constexpr const char* osc1Oct    = "osc1Oct";
    inline constexpr const char* osc1Tune   = "osc1Tune";
    inline constexpr const char* osc2Wave   = "osc2Wave";
    inline constexpr const char* osc2Oct    = "osc2Oct";
    inline constexpr const char* osc2Tune   = "osc2Tune";
    inline constexpr const char* osc3Wave   = "osc3Wave";
    inline constexpr const char* osc3Oct    = "osc3Oct";
    inline constexpr const char* osc3Tune   = "osc3Tune";
    inline constexpr const char* osc1Table  = "osc1Table";
    inline constexpr const char* osc2Table  = "osc2Table";
    inline constexpr const char* osc3Table  = "osc3Table";
    // Mixer
    inline constexpr const char* osc1Level  = "osc1Level";
    inline constexpr const char* osc2Level  = "osc2Level";
    inline constexpr const char* osc3Level  = "osc3Level";
    inline constexpr const char* noiseLevel = "noiseLevel";
    inline constexpr const char* pulseWidth = "pulseWidth";
    inline constexpr const char* lfoRate    = "lfoRate";
    inline constexpr const char* lfoPitch   = "lfoPitch";
    inline constexpr const char* lfoFilter  = "lfoFilter";
    // Spielweise
    inline constexpr const char* keyTrack   = "keyTrack";
    inline constexpr const char* velFilter  = "velFilter";
    inline constexpr const char* master     = "master";
    inline constexpr const char* lfoWave    = "lfoWave";
    inline constexpr const char* voiceMode  = "voiceMode";
    inline constexpr const char* bendRange  = "bendRange";
    inline constexpr const char* velAmp     = "velAmp";
    inline constexpr const char* drift      = "drift";
    // POWER
    inline constexpr const char* uniVoices  = "uniVoices";
    inline constexpr const char* uniDetune  = "uniDetune";
    inline constexpr const char* uniSpread  = "uniSpread";
    inline constexpr const char* subLevel   = "subLevel";
    inline constexpr const char* fltType    = "fltType";
    inline constexpr const char* heat       = "heat";
    inline constexpr const char* lfo2Rate   = "lfo2Rate";
    inline constexpr const char* lfo2Wave   = "lfo2Wave";
    // Effekte
    inline constexpr const char* chorusMix  = "chorusMix";
    inline constexpr const char* chorusRate = "chorusRate";
    inline constexpr const char* chorusDepth= "chorusDepth";
    inline constexpr const char* delayMix   = "delayMix";
    inline constexpr const char* delayTime  = "delayTime";
    inline constexpr const char* delayFb    = "delayFb";
    inline constexpr const char* revSend    = "revSend";
    inline constexpr const char* revSize    = "revSize";
    inline constexpr const char* revDamp    = "revDamp";
    inline constexpr const char* revLevel   = "revLevel";
    // Sample
    inline constexpr const char* smpRoot      = "smpRoot";
    inline constexpr const char* smpStart     = "smpStart";
    inline constexpr const char* smpEnd       = "smpEnd";
    inline constexpr const char* smpLoop      = "smpLoop";
    inline constexpr const char* smpLoopStart = "smpLoopStart";
    inline constexpr const char* smpKeyTrack  = "smpKeyTrack";
    inline constexpr const char* smpOneShot   = "smpOneShot";
    inline constexpr const char* smpGain      = "smpGain";
    // Eingang (Aufnahme) + Ausgang – gehoeren nicht zum Preset
    inline constexpr const char* inMode     = "inMode";
    inline constexpr const char* inGain     = "inGain";
    inline constexpr const char* inAuto     = "inAuto";
    inline constexpr const char* inMonitor  = "inMonitor";
    inline constexpr const char* outGain    = "outGain";
    inline constexpr const char* quality    = "quality";
    // Wheels / Spielhilfen
    inline constexpr const char* bendDown   = "bendDown";
    inline constexpr const char* mwVibrato  = "mwVibrato";
    inline constexpr const char* mwCutoff   = "mwCutoff";
    inline constexpr const char* mwPos      = "mwPos";
    inline constexpr const char* atVibrato  = "atVibrato";
    inline constexpr const char* atCutoff   = "atCutoff";
    // Mod-Huellkurve (frei in der Matrix)
    inline constexpr const char* modAttack  = "modAttack";
    inline constexpr const char* modDecay   = "modDecay";
    inline constexpr const char* modSustain = "modSustain";
    inline constexpr const char* modRelease = "modRelease";
    inline constexpr int numModSlots = 8;
    // Harmonizer (Spieleinstellung, nicht Teil der Presets)
    inline constexpr const char* harmOn     = "harmOn";
    inline constexpr const char* harmKey    = "harmKey";
    inline constexpr const char* harmScale  = "harmScale";
    inline constexpr const char* harmVoicing= "harmVoicing";
    inline constexpr const char* harm3      = "harm3";
    inline constexpr const char* harm5      = "harm5";
    inline constexpr const char* harm7      = "harm7";
    inline constexpr const char* harm9      = "harm9";
    inline constexpr const char* harmOctUp  = "harmOctUp";
    inline constexpr const char* harmOctDn  = "harmOctDn";
    inline constexpr const char* harmLevel  = "harmLevel";

    // KEYS-Ebene (1.2.0): Clavinet, Tine-Piano, Tonewheel-Orgel, Pfeifenorgel, FM, Mundharmonika
    inline constexpr const char* synthOn    = "synthOn";
    inline constexpr const char* keysOn     = "keysOn";
    inline constexpr const char* keysModel  = "keysModel";
    inline constexpr const char* keysVariant= "keysVariant";
    inline constexpr const char* keysLevel  = "keysLevel";
    inline constexpr const char* keysOct    = "keysOct";
    inline constexpr const char* keysTone   = "keysTone";
    inline constexpr const char* keysDecay  = "keysDecay";
    inline constexpr const char* keysA      = "keysA";
    inline constexpr const char* keysB      = "keysB";
    inline constexpr const char* keysVel    = "keysVel";
    inline constexpr const char* keysDrive  = "keysDrive";
    inline constexpr const char* keysFx     = "keysFx";
    inline constexpr const char* keysFxAmt  = "keysFxAmt";
    inline constexpr const char* keysFxRate = "keysFxRate";
    inline constexpr const char* keysWheel  = "keysWheel";
    // Talkbox (wirkt auf die Synth-Ebene)
    inline constexpr const char* talkMix    = "talkMix";
    inline constexpr const char* talkSrc    = "talkSrc";
    inline constexpr const char* talkShift  = "talkShift";
    inline constexpr const char* talkRate   = "talkRate";

    inline juce::String modSrc (int slot) { return "mod" + juce::String (slot + 1) + "Src"; }
    inline juce::String modDst (int slot) { return "mod" + juce::String (slot + 1) + "Dst"; }
    inline juce::String modAmt (int slot) { return "mod" + juce::String (slot + 1) + "Amt"; }

    /** Werden von Presets nicht veraendert */
    inline bool isGlobalParam (const juce::String& id)
    {
        return id == inMode || id == inGain || id == inAuto || id == inMonitor || id == outGain || id == quality
            || id.startsWith ("harm");
    }
}

namespace ParamChoices
{
    inline juce::String u8 (const char* t) { return juce::String (juce::CharPointer_UTF8 (t)); }
    // 0..3 analog, 4 Wavetable, 5 Sample, 6 Piano (Flügel-Samples)
    inline const juce::StringArray waves   { "Triangle", "Saw", "Square", "Pulse", "Wavetable", "Sample", "Piano" };
    enum { wTri = 0, wSaw, wSquare, wPulse, wTable, wSample, wPiano };
    inline const juce::StringArray tables  { "Basic Shapes", "PWM Sweep", "Harmonics", "Vocal", "Digital FM",
                                             "Sync Sweep", "Organ Bars", "Bell", "Growl",
                                             "Pipe Principal", "Pipe Flute", "Pipe Reed", "E-Piano", "User" };
    inline constexpr int numTables = 14, userTable = 13;
    inline const juce::StringArray qualities { "High", "Eco" };
    inline const juce::StringArray keys      { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    inline const juce::StringArray harmScales{ "Dur", "Moll", "Dorisch", "Mixolydisch", "Harm. Moll", "Pentatonik",
                                               "Akkord Dur (fest)", "Akkord Moll (fest)" };
    inline const juce::StringArray voicings  { "Eng (darueber)", "Offen", "Darunter" };
    inline const juce::StringArray octaves { "32'", "16'", "8'", "4'", "2'" };
    inline const juce::StringArray lfoWaves{ "Tri", "Saw", "Square", "S&H" };
    inline const juce::StringArray modes   { "Mono", "Legato", "Poly" };
    inline const juce::StringArray inputModes  { "Stereo 1/2", "Mono In 1", "Mono In 2", "Mono 1+2" };
    inline const juce::StringArray loopModes   { "Off", "Loop", "Ping-Pong" };
    inline const juce::StringArray unison      { "1", "2", "3", "4", "5", "7" };
    inline constexpr int unisonValues[]         { 1, 2, 3, 4, 5, 7 };
    inline const juce::StringArray filterTypes { "LP 24", "LP 12", "Band Pass", "High Pass" };
    inline const juce::StringArray delayTimes  { "1/32", "1/16T", "1/16", "1/16.", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4.", "1/2", "1 Bar" };
    inline constexpr float delayBeats[]         { 0.125f, 1.0f / 6.0f, 0.25f, 0.375f, 1.0f / 3.0f, 0.5f, 0.75f, 2.0f / 3.0f, 1.0f, 1.5f, 2.0f, 4.0f };
    inline const juce::StringArray modSources  { "Off", "LFO 1", "LFO 2", "Filter Env", "Amp Env", "Mod Env", "Velocity", "Mod Wheel",
                                                 "Pitch Bend", "Aftertouch", "Key", "Random", "Breath CC2", "Expr CC11" };
    inline const juce::StringArray modDests    { "Off", "Pitch", "Osc1 Pitch", "Osc2 Pitch", "Osc3 Pitch", "Cutoff", "Emphasis", "Drive", "Pulse Width",
                                                 "Volume", "Pan", "Detune", "Noise", "Sub", "Osc1 Level", "Osc2 Level", "Osc3 Level",
                                                 "WT Pos 1", "WT Pos 2", "WT Pos 3", "WT Pos All" };

    // ---------------- KEYS-Ebene
    inline const juce::StringArray keysModels { "Funk Clav", "Tine Piano", "Tonewheel Organ", "Pipe Organ",
                                                "FM Piano", "FM Bell", "FM Bass", "Blues Harp" };
    enum { kClav = 0, kTine, kTonewheel, kPipe, kFmPiano, kFmBell, kFmBass, kHarp, numKeysModels };
    inline const juce::StringArray keysFxTypes { "Off", "Phaser", "Tremolo", "Auto-Pan", "Chorus", "Wah (Env)", "Wah (Mod-Rad)", "Leslie" };
    enum { fxOff = 0, fxPhaser, fxTremolo, fxAutoPan, fxChorus, fxWahEnv, fxWahWheel, fxLeslie };
    inline const juce::StringArray keysWheels  { "Off", "Leslie Speed", "Swell", "Vibrato", "Register", "Tone" };
    enum { kwOff = 0, kwLeslie, kwSwell, kwVibrato, kwRegister, kwTone };
    inline const juce::StringArray keysOctaves { "-2", "-1", "0", "+1", "+2" };
    inline const juce::StringArray talkSources { "Mod-Rad", "Huellkurve", "Aftertouch", "Auto (LFO)" };
    inline constexpr int maxKeysVariants = 15;

    /** Orgelwelten der Pfeifenorgel – synthetisch nachempfunden, keine Aufnahmen */
    inline const juce::StringArray& organWorlds()
    {
        static const juce::StringArray w {
            u8 ("Paris Notre-Dame"), u8 ("Freiberg Silbermann"), u8 ("Leipzig Bach-Barock"), u8 ("Passau Dom"),
            u8 ("K\xc3\xb6ln Dom"), u8 ("Ulm M\xc3\xbcnster"), u8 ("Rom St. Peter"), u8 ("London Westminster"),
            u8 ("Haarlem St. Bavo"), u8 ("Wien Stephansdom"), u8 ("Hamburg Michel"), u8 ("Riga Dom"),
            u8 ("Philadelphia Wanamaker"), u8 ("Theater-Orgel"), u8 ("Dorfkirche") };
        return w;
    }

    /** Namen der Varianten je Modell (Regler VARIANTE auf der KEYS-Seite) */
    inline const juce::StringArray& keysVariantNames (int model)
    {
        static const juce::StringArray clav  { "Brilliant", "Treble", "Medium", "Soft", "Brilliant + Medium" };
        static const juce::StringArray tine  { "Stage", "Suitcase", "Bright Tines", "Mellow", "Reed-Piano" };
        static const juce::StringArray tw    { "888000000  Jazz", "888800000  Gospel", "888888888  Volle Orgel",
                                               "800000888  Pfeife hoch", "688600000  Ballade", "008000000  Weich",
                                               "838000000  Groove", "888630000  Rock", "808800008  Blues", "006876540  Theater" };
        static const juce::StringArray fmp   { "80er Ballade", "Bright Tine", "Soft Keys", "Hard Funk" };
        static const juce::StringArray fmb   { u8 ("R\xc3\xb6hrenglocken"), "Glas", "Celesta", "Marimba", "Vibraphon" };
        static const juce::StringArray fmbs  { "Solid", "Punch", "Pluck", "Synth Growl" };
        static const juce::StringArray harp  { "Clean", "Bullet Mic", "Chromatisch", "Hohle Hand" };
        switch (model)
        {
            case kClav:      return clav;
            case kTine:      return tine;
            case kTonewheel: return tw;
            case kPipe:      return organWorlds();
            case kFmPiano:   return fmp;
            case kFmBell:    return fmb;
            case kFmBass:    return fmbs;
            default:         return harp;
        }
    }

    /** Beschriftung der beiden Modell-Regler A und B */
    inline juce::String keysCtrlName (int model, bool b)
    {
        switch (model)
        {
            case kClav:      return b ? "Pickup"     : "Damper";
            case kTine:      return b ? "Bell"       : "Bark";
            case kTonewheel: return b ? "Click"      : "Percussion";
            case kPipe:      return b ? "Wind"       : "Register";
            case kFmPiano:
            case kFmBell:
            case kFmBass:    return b ? "Attack"     : "Index";
            default:         return b ? "Bend"       : "Growl";
        }
    }
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    using P  = AudioParameterFloat;
    using C  = AudioParameterChoice;
    using B  = AudioParameterBool;
    using PA = AudioParameterFloatAttributes;

    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    const int v = 1;

    auto timeRange = [] (float maxS) { NormalisableRange<float> r (0.001f, maxS); r.setSkewForCentre (0.35f); return r; };
    auto secs = PA().withLabel ("s").withStringFromValueFunction ([] (float x, int)
                { return x < 1.0f ? String (roundToInt (x * 1000.0f)) + " ms" : String (x, 2) + " s"; });
    auto pct  = PA().withStringFromValueFunction ([] (float x, int) { return String (roundToInt (x * 100.0f)) + " %"; });
    auto bip  = PA().withStringFromValueFunction ([] (float x, int) { return (x > 0.005f ? "+" : "") + String (roundToInt (x * 100.0f)) + " %"; });
    auto dbA  = PA().withLabel ("dB").withStringFromValueFunction ([] (float x, int) { return (x > 0.05f ? "+" : "") + String (x, 1) + " dB"; });
    auto semis = PA().withLabel ("st").withStringFromValueFunction ([] (float x, int) { return (x > 0 ? "+" : "") + String (x, 2) + " st"; });
    auto hz   = PA().withLabel ("Hz").withStringFromValueFunction ([] (float x, int)
                { return x >= 1000.0f ? String (x / 1000.0f, 2) + " kHz" : String (roundToInt (x)) + " Hz"; });
    auto hzS  = PA().withLabel ("Hz").withStringFromValueFunction ([] (float x, int) { return String (x, 2) + " Hz"; });
    auto unit = [] { return NormalisableRange<float> (0.0f, 1.0f); };

    NormalisableRange<float> cutoffRange (20.0f, 20000.0f); cutoffRange.setSkewForCentre (1000.0f);

    // Seite 1
    p.push_back (std::make_unique<P> (ParameterID { ParamID::cutoff, v },     "Cutoff",   cutoffRange, 1200.0f, hz));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::resonance, v },  "Emphasis", unit(), 0.25f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::envAmount, v },  "Contour",  unit(), 0.45f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::drive, v },      "Drive",    unit(), 0.2f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::ampAttack, v },  "Attack",   timeRange (10.0f), 0.005f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::ampDecay, v },   "Decay",    timeRange (10.0f), 0.4f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::ampRelease, v }, "Release",  timeRange (10.0f), 0.25f, secs));
    {
        NormalisableRange<float> g (0.0f, 2.0f); g.setSkewForCentre (0.25f);
        p.push_back (std::make_unique<P> (ParameterID { ParamID::glide, v },  "Glide", g, 0.0f, secs));
    }
    // Seite 2
    p.push_back (std::make_unique<P> (ParameterID { ParamID::osc1Pos, v },    "WT Pos 1",  unit(), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::osc2Pos, v },    "WT Pos 2",  unit(), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::osc3Pos, v },    "WT Pos 3",  unit(), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::fltAttack, v },  "F Attack",  timeRange (10.0f), 0.005f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::fltDecay, v },   "F Decay",   timeRange (10.0f), 0.35f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::fltSustain, v }, "F Sustain", unit(), 0.2f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::fltRelease, v }, "F Release", timeRange (10.0f), 0.25f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::ampSustain, v }, "Sustain",   unit(), 0.8f, pct));
    // Seite 3 – Oszillatoren
    const char* wIds[] { ParamID::osc1Wave, ParamID::osc2Wave, ParamID::osc3Wave };
    const char* oIds[] { ParamID::osc1Oct, ParamID::osc2Oct, ParamID::osc3Oct };
    const char* tIds[] { ParamID::osc1Tune, ParamID::osc2Tune, ParamID::osc3Tune };
    const int   wDef[] { 1, 1, 2 }, oDef[] { 2, 2, 1 };
    const float tDef[] { 0.0f, 0.07f, -0.05f };
    for (int i = 0; i < 3; ++i)
    {
        const String n = "Osc" + String (i + 1) + " ";
        p.push_back (std::make_unique<C> (ParameterID { wIds[i], v }, n + "Wave",  ParamChoices::waves, wDef[i]));
        p.push_back (std::make_unique<C> (ParameterID { oIds[i], v }, n + "Range", ParamChoices::octaves, oDef[i]));
        p.push_back (std::make_unique<P> (ParameterID { tIds[i], v }, n + "Tune",  NormalisableRange<float> (-7.0f, 7.0f), tDef[i], semis));
    }
    p.push_back (std::make_unique<C> (ParameterID { ParamID::osc1Table, v }, "Osc1 Table", ParamChoices::tables, 0));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::osc2Table, v }, "Osc2 Table", ParamChoices::tables, 0));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::osc3Table, v }, "Osc3 Table", ParamChoices::tables, 0));
    // Mixer
    p.push_back (std::make_unique<P> (ParameterID { ParamID::osc1Level, v },  "Osc1 Level", unit(), 0.8f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::osc2Level, v },  "Osc2 Level", unit(), 0.6f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::osc3Level, v },  "Osc3 Level", unit(), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::noiseLevel, v }, "Noise",      unit(), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::pulseWidth, v }, "Pulse Width",NormalisableRange<float> (0.05f, 0.5f), 0.25f, pct));
    {
        NormalisableRange<float> r (0.05f, 20.0f); r.setSkewForCentre (3.0f);
        p.push_back (std::make_unique<P> (ParameterID { ParamID::lfoRate, v }, "LFO Rate", r, 5.0f, hzS));
    }
    p.push_back (std::make_unique<P> (ParameterID { ParamID::lfoPitch, v },   "LFO>Pitch",  unit(), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::lfoFilter, v },  "LFO>Filter", unit(), 0.0f, pct));
    // Spielweise
    p.push_back (std::make_unique<P> (ParameterID { ParamID::keyTrack, v },   "Key Track",  unit(), 0.5f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::velFilter, v },  "Vel>Filter", unit(), 0.2f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::master, v },     "Volume",     NormalisableRange<float> (-36.0f, 6.0f), -6.0f, dbA));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::lfoWave, v },    "LFO Wave",   ParamChoices::lfoWaves, 0));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::voiceMode, v },  "Voice Mode", ParamChoices::modes, 2));
    auto stA = PA().withStringFromValueFunction ([] (float x, int) { return String (roundToInt (x)) + " st"; });
    p.push_back (std::make_unique<P> (ParameterID { ParamID::bendRange, v },  "Bend Up",   NormalisableRange<float> (0.0f, 24.0f, 1.0f), 2.0f, stA));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::bendDown, v },   "Bend Down", NormalisableRange<float> (0.0f, 24.0f, 1.0f), 2.0f, stA));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::mwVibrato, v },  "MW>Vibrato", unit(), 0.4f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::mwCutoff, v },   "MW>Cutoff",  NormalisableRange<float> (-1.0f, 1.0f), 0.0f, bip));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::mwPos, v },      "MW>WT Pos",  NormalisableRange<float> (-1.0f, 1.0f), 0.0f, bip));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::atVibrato, v },  "AT>Vibrato", unit(), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::atCutoff, v },   "AT>Cutoff",  NormalisableRange<float> (-1.0f, 1.0f), 0.0f, bip));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::modAttack, v },  "Mod Attack",  timeRange (10.0f), 0.005f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::modDecay, v },   "Mod Decay",   timeRange (10.0f), 0.3f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::modSustain, v }, "Mod Sustain", unit(), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::modRelease, v }, "Mod Release", timeRange (10.0f), 0.3f, secs));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::velAmp, v },     "Vel>Amp",    unit(), 0.3f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::drift, v },      "Drift",      unit(), 0.3f, pct));
    // POWER
    p.push_back (std::make_unique<C> (ParameterID { ParamID::uniVoices, v },  "Unison",     ParamChoices::unison, 0));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::uniDetune, v },  "Uni Detune", unit(), 0.3f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::uniSpread, v },  "Uni Spread", unit(), 0.6f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::subLevel, v },   "Sub Osc",    unit(), 0.0f, pct));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::fltType, v },    "Filter Type", ParamChoices::filterTypes, 0));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::heat, v },       "Heat",       unit(), 0.0f, pct));
    {
        NormalisableRange<float> r (0.02f, 20.0f); r.setSkewForCentre (2.0f);
        p.push_back (std::make_unique<P> (ParameterID { ParamID::lfo2Rate, v }, "LFO2 Rate", r, 0.5f, hzS));
    }
    p.push_back (std::make_unique<C> (ParameterID { ParamID::lfo2Wave, v },   "LFO2 Wave",  ParamChoices::lfoWaves, 0));
    for (int k = 0; k < ParamID::numModSlots; ++k)
    {
        const String pre = "Mod " + String (k + 1) + " ";
        p.push_back (std::make_unique<C> (ParameterID { ParamID::modSrc (k), v }, pre + "Source", ParamChoices::modSources, 0));
        p.push_back (std::make_unique<C> (ParameterID { ParamID::modDst (k), v }, pre + "Dest",   ParamChoices::modDests, 0));
        p.push_back (std::make_unique<P> (ParameterID { ParamID::modAmt (k), v }, pre + "Amount", NormalisableRange<float> (-1.0f, 1.0f), 0.0f, bip));
    }
    // Effekte
    p.push_back (std::make_unique<P> (ParameterID { ParamID::chorusMix, v },  "Chorus Mix",   unit(), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::chorusRate, v }, "Chorus Rate",  unit(), 0.4f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::chorusDepth, v },"Chorus Depth", unit(), 0.5f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::delayMix, v },   "Delay Mix",    unit(), 0.0f, pct));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::delayTime, v },  "Delay Time",   ParamChoices::delayTimes, 6));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::delayFb, v },    "Delay Feedback", NormalisableRange<float> (0.0f, 0.95f), 0.35f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::revSend, v },    "Reverb Send",  unit(), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::revSize, v },    "Reverb Size",  unit(), 0.7f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::revDamp, v },    "Reverb Damp",  unit(), 0.4f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::revLevel, v },   "Reverb Level", NormalisableRange<float> (0.0f, 1.5f), 1.0f, pct));
    // Sample
    auto noteAttr = AudioParameterIntAttributes().withStringFromValueFunction ([] (int x, int) { return MidiMessage::getMidiNoteName (x, true, true, 3); });
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { ParamID::smpRoot, v }, "Sample Root", 0, 127, 60, noteAttr));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::smpStart, v },     "Sample Start", unit(), 0.0f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::smpEnd, v },       "Sample End",   unit(), 1.0f, pct));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::smpLoop, v },      "Sample Loop",  ParamChoices::loopModes, 0));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::smpLoopStart, v }, "Loop Start",   unit(), 0.0f, pct));
    p.push_back (std::make_unique<B> (ParameterID { ParamID::smpKeyTrack, v },  "Sample Key Track", true));
    p.push_back (std::make_unique<B> (ParameterID { ParamID::smpOneShot, v },   "Sample One-Shot", false));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::smpGain, v },      "Sample Volume", NormalisableRange<float> (-24.0f, 24.0f), 0.0f, dbA));
    // Eingang / Ausgang
    p.push_back (std::make_unique<C> (ParameterID { ParamID::inMode, v },    "Input Mode", ParamChoices::inputModes, 0));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::inGain, v },    "Input Gain", NormalisableRange<float> (-24.0f, 24.0f), 0.0f, dbA));
    p.push_back (std::make_unique<B> (ParameterID { ParamID::inAuto, v },    "Rec Auto Start", true));
    p.push_back (std::make_unique<B> (ParameterID { ParamID::inMonitor, v }, "Input Monitor", false));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::outGain, v },   "Output Gain", NormalisableRange<float> (-24.0f, 12.0f), 0.0f, dbA));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::quality, v },   "Quality", ParamChoices::qualities, 0));
    // Harmonizer
    p.push_back (std::make_unique<B> (ParameterID { ParamID::harmOn, v },     "Harmony On", false));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::harmKey, v },    "Harmony Key", ParamChoices::keys, 0));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::harmScale, v },  "Harmony Scale", ParamChoices::harmScales, 0));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::harmVoicing, v },"Harmony Voicing", ParamChoices::voicings, 0));
    p.push_back (std::make_unique<B> (ParameterID { ParamID::harm3, v },      "Harmony 3rd", true));
    p.push_back (std::make_unique<B> (ParameterID { ParamID::harm5, v },      "Harmony 5th", true));
    p.push_back (std::make_unique<B> (ParameterID { ParamID::harm7, v },      "Harmony 7th", false));
    p.push_back (std::make_unique<B> (ParameterID { ParamID::harm9, v },      "Harmony 9th", false));
    p.push_back (std::make_unique<B> (ParameterID { ParamID::harmOctUp, v },  "Harmony Oct Up", false));
    p.push_back (std::make_unique<B> (ParameterID { ParamID::harmOctDn, v },  "Harmony Oct Down", false));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::harmLevel, v },  "Harmony Level", unit(), 0.8f, pct));

    // KEYS-Ebene + Talkbox (1.2.0) – hinten angehaengt, damit alte Projekte und die A61-Seiten gleich bleiben
    p.push_back (std::make_unique<B> (ParameterID { ParamID::synthOn, v },    "Synth Layer On", true));
    p.push_back (std::make_unique<B> (ParameterID { ParamID::keysOn, v },     "Keys On", false));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::keysModel, v },  "Keys Model", ParamChoices::keysModels, 0));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { ParamID::keysVariant, v }, "Keys Variant", 0, ParamChoices::maxKeysVariants - 1, 0,
                 AudioParameterIntAttributes().withStringFromValueFunction ([] (int x, int) { return String (x + 1); })));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::keysLevel, v },  "Keys Volume", NormalisableRange<float> (-36.0f, 6.0f), -6.0f, dbA));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::keysOct, v },    "Keys Octave", ParamChoices::keysOctaves, 2));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::keysTone, v },   "Keys Tone",  unit(), 0.5f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::keysDecay, v },  "Keys Decay", unit(), 0.5f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::keysA, v },      "Keys Ctrl A", unit(), 0.4f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::keysB, v },      "Keys Ctrl B", unit(), 0.3f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::keysVel, v },    "Keys Velocity", unit(), 0.6f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::keysDrive, v },  "Keys Drive", unit(), 0.0f, pct));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::keysFx, v },     "Keys FX", ParamChoices::keysFxTypes, 0));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::keysFxAmt, v },  "Keys FX Amount", unit(), 0.5f, pct));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::keysFxRate, v }, "Keys FX Rate", unit(), 0.4f, pct));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::keysWheel, v },  "Keys Mod Wheel", ParamChoices::keysWheels, 0));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::talkMix, v },    "Talkbox Mix", unit(), 0.0f, pct));
    p.push_back (std::make_unique<C> (ParameterID { ParamID::talkSrc, v },    "Talkbox Source", ParamChoices::talkSources, 0));
    p.push_back (std::make_unique<P> (ParameterID { ParamID::talkShift, v },  "Talkbox Formant", NormalisableRange<float> (-1.0f, 1.0f), 0.0f, bip));
    {
        NormalisableRange<float> r (0.05f, 8.0f); r.setSkewForCentre (1.0f);
        p.push_back (std::make_unique<P> (ParameterID { ParamID::talkRate, v }, "Talkbox Rate", r, 1.0f, hzS));
    }

    return { p.begin(), p.end() };
}

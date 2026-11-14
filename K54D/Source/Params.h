#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DrumEngine.h"

// Parameter-IDs des K-54 D
namespace PID
{
    inline juce::String lane (int i, const char* what) { return "l" + juce::String (i) + "_" + what; }

    inline constexpr const char* seqOn    = "seqOn";
    inline constexpr const char* steps    = "steps";
    inline constexpr const char* res      = "res";
    inline constexpr const char* swing    = "swing";
    inline constexpr const char* humanize = "humanize";
    inline constexpr const char* pattern  = "pattern";
    inline constexpr const char* fat      = "fat";
    inline constexpr const char* glue     = "glue";
    inline constexpr const char* roomSize = "roomSize";
    inline constexpr const char* master   = "master";
}

namespace Res
{
    // Laenge eines Steps in Viertelnoten
    inline const juce::StringArray names { "1/8", "1/16", "1/32", "1/8T", "1/16T" };
    inline double stepPpq (int idx)
    {
        switch (idx)
        {
            case 0: return 0.5;
            case 2: return 0.125;
            case 3: return 1.0 / 3.0;
            case 4: return 1.0 / 6.0;
            default: return 0.25;
        }
    }
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    int v = 1;
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return String (roundToInt (x * 100.0f)) + " %"; });
    auto bip = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return (x > 0.004f ? "+" : "") + String (roundToInt (x * 100.0f)); });
    auto pan = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int)
    {
        const int p = roundToInt (x * 100.0f);
        return p == 0 ? String ("C") : (p < 0 ? "L" + String (-p) : "R" + String (p));
    });
    auto semi = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return (x > 0.04f ? "+" : "") + String (x, 1) + " st"; });

    for (int i = 0; i < DrumInfo::numLanes; ++i)
    {
        const String n = DrumInfo::names[i];
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { PID::lane (i, "model"), v }, n + " Model", DrumInfo::models (i), 0));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::lane (i, "level"), v }, n + " Level", NormalisableRange<float> (0.0f, 1.2f), i == 0 ? 0.9f : 0.75f, pct));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::lane (i, "tune"),  v }, n + " Tune",  NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, semi));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::lane (i, "decay"), v }, n + " Decay", NormalisableRange<float> (0.0f, 1.0f), 0.5f, pct));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::lane (i, "tone"),  v }, n + " Tone",  NormalisableRange<float> (-1.0f, 1.0f), 0.0f, bip));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::lane (i, "snap"),  v }, n + " Snap",  NormalisableRange<float> (0.0f, 1.0f), 0.5f, pct));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::lane (i, "drive"), v }, n + " Drive", NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::lane (i, "pan"),   v }, n + " Pan",   NormalisableRange<float> (-1.0f, 1.0f), 0.0f, pan));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::lane (i, "room"),  v }, n + " Room",  NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
        layout.add (std::make_unique<AudioParameterBool>   (ParameterID { PID::lane (i, "mute"),  v }, n + " Mute", false));
        layout.add (std::make_unique<AudioParameterBool>   (ParameterID { PID::lane (i, "solo"),  v }, n + " Solo", false));
    }

    StringArray stepNames;
    for (int s = 8; s <= DrumInfo::maxSteps; ++s) stepNames.add (String (s));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { PID::seqOn, v }, "Sequencer", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { PID::steps, v }, "Steps", stepNames, 8));     // 16
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { PID::res,   v }, "Resolution", Res::names, 1));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::swing, v }, "Swing", NormalisableRange<float> (0.0f, 1.0f), 0.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return String (50 + roundToInt (x * 25.0f)) + " %"; })));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::humanize, v }, "Humanize", NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { PID::pattern, v }, "Pattern", StringArray { "A", "B", "C", "D" }, 0));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::fat, v }, "Fat", NormalisableRange<float> (0.0f, 1.0f), 0.35f, pct));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::glue, v }, "Glue", NormalisableRange<float> (0.0f, 1.0f), 0.3f, pct));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::roomSize, v }, "Room Size", NormalisableRange<float> (0.0f, 1.0f), 0.4f, pct));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { PID::master, v }, "Master", NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return (x > 0.04f ? "+" : "") + String (x, 1) + " dB"; })));
    return layout;
}

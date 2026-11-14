#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Pattern.h"

// ============================================================================
//  Werks-Presets des K-54 D (Kit + Pattern A Main, B Fill, C Break, D Crash)
// ============================================================================
struct FactoryPreset
{
    const char* category;
    const char* name;
    const char* kit;        // z.B. "K=909 S=909 Kd=.6 Sr=.2"   (Spur-Buchstabe = Modell, Spur+Param = Wert)
    const char* pattern;    // z.B. "K:x...x...x...x... S:....x.......x..."
    int   steps;            // 8..32
    int   res;              // 0 1/8, 1 1/16, 2 1/32, 3 1/8T, 4 1/16T
    float swing, fat, glue, room;
};

namespace Presets
{
    int numFactory();
    const FactoryPreset& factory (int index);
    juce::StringArray categories();

    /** Kit + Patterns + globale Werte setzen */
    void applyFactory (int index, juce::AudioProcessorValueTreeState& apvts, PatternStore& patterns);

    /** Pattern-Text in Pattern p schreiben (Spuren ohne Angabe werden geleert) */
    void parsePattern (const juce::String& text, PatternStore& patterns, int p, int steps);

    /** B Fill, C Break, D Crash aus A erzeugen */
    void makeVariations (PatternStore& patterns, int steps);

    /** Lautstaerke-Zeichen: X x o - _ . 1-9, r R q = Wirbel */
    int velForChar (juce::juce_wchar c, int& ratchet);
}

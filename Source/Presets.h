#pragma once
#include <vector>
#include <utility>

// Werks-Presets. Werte in echten Einheiten (Hz, Sekunden, 0..1, dB).
// Auswahl-Parameter als Index:
//   Wellenform 0 Dreieck, 1 Saege, 2 Rechteck, 3 Puls
//   Range      0 = 32', 1 = 16', 2 = 8', 3 = 4', 4 = 2'
//   Modus      0 Mono, 1 Legato, 2 Poly
//   LFO        0 Dreieck, 1 Saege, 2 Rechteck, 3 S&H
struct FactoryPreset
{
    const char* name;
    const char* category;
    std::vector<std::pair<const char*, float>> values;   // nur Abweichungen vom Grundzustand
};

const std::vector<FactoryPreset>& getFactoryPresets();

#pragma once
#include <vector>
#include <utility>

// Werks-Presets. Werte in echten Einheiten (Hz, Sekunden, 0..1, dB).
// Auswahl-Parameter als Index (siehe Presets.cpp)
struct FactoryPreset
{
    const char* name;
    const char* category;
    std::vector<std::pair<const char*, float>> values;   // nur Abweichungen vom Grundzustand
};

const std::vector<FactoryPreset>& getFactoryPresets();

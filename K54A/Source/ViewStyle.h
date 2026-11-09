// K-54 Analyzer · Ansicht-Einstellungen und Farbschemata (gelten für alle Projekte)
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>

namespace k54a
{
struct ViewStyle
{
    int specMode = 0;     // 0 Kurve, 1 Balken 1/3 Okt., 2 Spektrogramm
    int range = 0;        // dB-Bereich
    int line = 1;         // Linienstärke
    int fill = 2;         // Füllung
    int grid = 1;         // Raster
    bool glow = true;     // Leuchten
    bool peaks = true;    // Spitzen markieren
    int theme = 0;        // Farbschema
    int meterStyle = 0;   // 0 LED Ampel, 1 LED Farbe, 2 Balken glatt
    int meterRange = 0;   // -60 / -48 / -24 dB
    int hold = 1;         // Peak-Hold 1 s / 3 s / unendlich
    int layout = 0;       // 0 Standard, 1 Großes Spektrum

    static constexpr int numThemes = 5;

    float topDb() const    { static const float t[] { 6.0f, 6.0f, 6.0f, 0.0f };      return t[juce::jlimit (0, 3, range)]; }
    float botDb() const    { static const float b[] { -90.0f, -72.0f, -60.0f, -48.0f }; return b[juce::jlimit (0, 3, range)]; }
    float lineWidth() const { static const float w[] { 1.1f, 1.8f, 2.8f };            return w[juce::jlimit (0, 2, line)]; }
    float fillAlpha() const { static const float a[] { 0.0f, 0.18f, 0.38f };          return a[juce::jlimit (0, 2, fill)]; }
    float meterFloor() const { static const float f[] { -60.0f, -48.0f, -24.0f };     return f[juce::jlimit (0, 2, meterRange)]; }
    int holdFrames() const  { static const int h[] { 30, 90, 1 << 28 };               return h[juce::jlimit (0, 2, hold)]; }

    void load (juce::PropertiesFile& p)
    {
        specMode   = p.getIntValue ("v_specMode", specMode);
        range      = p.getIntValue ("v_range", range);
        line       = p.getIntValue ("v_line", line);
        fill       = p.getIntValue ("v_fill", fill);
        grid       = p.getIntValue ("v_grid", grid);
        glow       = p.getBoolValue ("v_glow", glow);
        peaks      = p.getBoolValue ("v_peaks", peaks);
        theme      = juce::jlimit (0, numThemes - 1, p.getIntValue ("v_theme", theme));
        meterStyle = p.getIntValue ("v_meterStyle", meterStyle);
        meterRange = p.getIntValue ("v_meterRange", meterRange);
        hold       = p.getIntValue ("v_hold", hold);
        layout     = p.getIntValue ("v_layout", layout);
    }

    void save (juce::PropertiesFile& p) const
    {
        p.setValue ("v_specMode", specMode);
        p.setValue ("v_range", range);
        p.setValue ("v_line", line);
        p.setValue ("v_fill", fill);
        p.setValue ("v_grid", grid);
        p.setValue ("v_glow", glow);
        p.setValue ("v_peaks", peaks);
        p.setValue ("v_theme", theme);
        p.setValue ("v_meterStyle", meterStyle);
        p.setValue ("v_meterRange", meterRange);
        p.setValue ("v_hold", hold);
        p.setValue ("v_layout", layout);
        p.saveIfNeeded();
    }

    bool operator== (const ViewStyle& o) const
    {
        return specMode == o.specMode && range == o.range && line == o.line && fill == o.fill && grid == o.grid
            && glow == o.glow && peaks == o.peaks && theme == o.theme && meterStyle == o.meterStyle
            && meterRange == o.meterRange && hold == o.hold && layout == o.layout;
    }
};

//==============================================================================
struct Theme
{
    juce::Colour mix, ref, diff;
};

inline int& currentThemeIndex() { static int idx = 0; return idx; }

inline const Theme& theme()
{
    static const Theme themes[ViewStyle::numThemes] {
        { juce::Colour (0xffd4a537), juce::Colour (0xff46c3f0), juce::Colour (0xffc9a2f2) },  // Keller54
        { juce::Colour (0xffff4fd8), juce::Colour (0xff3ef0ff), juce::Colour (0xfff7f06a) },  // Neon
        { juce::Colour (0xff5fd36b), juce::Colour (0xffff9a3c), juce::Colour (0xff9ab8ff) },  // Studio
        { juce::Colour (0xfff2f2f2), juce::Colour (0xffff5a5a), juce::Colour (0xffffd84a) },  // Kontrast
        { juce::Colour (0xff2fe0c2), juce::Colour (0xffff7f6e), juce::Colour (0xffb9a6ff) },  // Ozean
    };
    return themes[juce::jlimit (0, ViewStyle::numThemes - 1, currentThemeIndex())];
}

inline const char* themeName (int i)
{
    static const char* n[] { "Keller54 (Gold / Blau)", "Neon (Magenta / Cyan)", "Studio (Grün / Orange)",
                             "Kontrast (Weiß / Rot)", "Ozean (Türkis / Koralle)" };
    return n[juce::jlimit (0, ViewStyle::numThemes - 1, i)];
}

} // namespace k54a

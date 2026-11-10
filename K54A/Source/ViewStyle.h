// K-54 Analyzer · Ansicht-Einstellungen, Farbschemata und Ansicht-Presets (gelten für alle Projekte)
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
    int theme = 0;        // Farbschema (5 = eigene Farben)
    int meterStyle = 0;   // 0 LED Ampel, 1 LED Farbe, 2 Balken glatt
    int meterRange = 0;   // -60 / -48 / -24 dB
    int hold = 1;         // Peak-Hold 1 s / 3 s / unendlich
    int specColor = 1;    // 0 Themenfarbe, 1 Regenbogen
    bool colorful = true; // farbige Oberfläche

    // ab 1.5
    juce::uint32 customMix = 0xffd4a537, customRef = 0xff46c3f0, customDiff = 0xffc9a2f2, customAccent = 0xffffb03a;
    int accentMode = 0;   // 0 bunt (je Feld), 1 eigene Akzentfarbe
    bool showTonal = true, showHist = true, showLoud = true, showMeter = true;

    static constexpr int numThemes = 6;
    static constexpr int customTheme = 5;

    float topDb() const    { static const float t[] { 6.0f, 6.0f, 6.0f, 0.0f };      return t[juce::jlimit (0, 3, range)]; }
    float botDb() const    { static const float b[] { -90.0f, -72.0f, -60.0f, -48.0f }; return b[juce::jlimit (0, 3, range)]; }
    float lineWidth() const { static const float w[] { 1.1f, 1.8f, 2.8f };            return w[juce::jlimit (0, 2, line)]; }
    float fillAlpha() const { static const float a[] { 0.0f, 0.18f, 0.38f };          return a[juce::jlimit (0, 2, fill)]; }
    float meterFloor() const { static const float f[] { -60.0f, -48.0f, -24.0f };     return f[juce::jlimit (0, 2, meterRange)]; }
    int holdFrames() const  { static const int h[] { 30, 90, 1 << 28 };               return h[juce::jlimit (0, 2, hold)]; }
    bool bottomRow() const  { return showTonal || showHist; }
    bool rightColumn() const { return showLoud || showMeter; }

    static juce::uint32 hexOr (const juce::PropertySet& p, const char* key, juce::uint32 def)
    {
        const auto s = p.getValue (key);
        return s.isEmpty() ? def : (juce::uint32) s.getHexValue64();
    }

    void loadFrom (const juce::PropertySet& p)
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
        specColor  = p.getIntValue ("v_specColor", specColor);
        colorful   = p.getBoolValue ("v_colorful", colorful);
        customMix    = hexOr (p, "v_customMix", customMix);
        customRef    = hexOr (p, "v_customRef", customRef);
        customDiff   = hexOr (p, "v_customDiff", customDiff);
        customAccent = hexOr (p, "v_customAccent", customAccent);
        accentMode = p.getIntValue ("v_accentMode", accentMode);

        if (p.containsKey ("v_showTonal"))
        {
            showTonal = p.getBoolValue ("v_showTonal", true);
            showHist  = p.getBoolValue ("v_showHist", true);
            showLoud  = p.getBoolValue ("v_showLoud", true);
            showMeter = p.getBoolValue ("v_showMeter", true);
        }
        else if (p.getIntValue ("v_layout", 0) == 1)   // Version 1.3/1.4: „Großes Spektrum“
        {
            showTonal = showHist = false;
        }
    }

    void saveTo (juce::PropertySet& p) const
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
        p.setValue ("v_specColor", specColor);
        p.setValue ("v_colorful", colorful);
        p.setValue ("v_customMix", juce::String::toHexString ((juce::int64) customMix));
        p.setValue ("v_customRef", juce::String::toHexString ((juce::int64) customRef));
        p.setValue ("v_customDiff", juce::String::toHexString ((juce::int64) customDiff));
        p.setValue ("v_customAccent", juce::String::toHexString ((juce::int64) customAccent));
        p.setValue ("v_accentMode", accentMode);
        p.setValue ("v_showTonal", showTonal);
        p.setValue ("v_showHist", showHist);
        p.setValue ("v_showLoud", showLoud);
        p.setValue ("v_showMeter", showMeter);
        p.removeValue ("v_layout");
    }

    void load (juce::PropertiesFile& p) { loadFrom (p); }
    void save (juce::PropertiesFile& p) const { saveTo (p); p.saveIfNeeded(); }

    bool operator== (const ViewStyle& o) const
    {
        return specMode == o.specMode && range == o.range && line == o.line && fill == o.fill && grid == o.grid
            && glow == o.glow && peaks == o.peaks && theme == o.theme && meterStyle == o.meterStyle
            && meterRange == o.meterRange && hold == o.hold && specColor == o.specColor && colorful == o.colorful
            && customMix == o.customMix && customRef == o.customRef && customDiff == o.customDiff
            && customAccent == o.customAccent && accentMode == o.accentMode
            && showTonal == o.showTonal && showHist == o.showHist && showLoud == o.showLoud && showMeter == o.showMeter;
    }
    bool operator!= (const ViewStyle& o) const { return ! (*this == o); }
};

//==============================================================================
// Ansicht-Presets: fest eingebaute + eigene (in der Settings-Datei unter „v_presets“)
struct ViewPresets
{
    using List = std::vector<std::pair<juce::String, ViewStyle>>;

    static List builtIn()
    {
        List l;
        l.push_back ({ "Standard", ViewStyle {} });

        ViewStyle mixing;
        mixing.specMode = 1;
        l.push_back ({ "Mixing (Terzbänder)", mixing });

        ViewStyle master;
        master.specColor = 0; master.colorful = false; master.range = 3; master.meterRange = 2;
        master.hold = 2; master.grid = 2; master.fill = 1; master.peaks = false;
        l.push_back ({ "Mastering", master });

        ViewStyle spectro;
        spectro.specMode = 2; spectro.showTonal = spectro.showHist = false;
        l.push_back ({ "Spektrogramm groß", spectro });

        ViewStyle only;
        only.showTonal = only.showHist = only.showLoud = only.showMeter = false;
        l.push_back ({ "Nur Spektrum", only });

        ViewStyle classic;
        classic.specColor = 0; classic.colorful = false; classic.meterStyle = 2; classic.glow = false;
        l.push_back ({ "Klassisch (schlicht)", classic });
        return l;
    }

    static List user (juce::PropertiesFile& p)
    {
        List l;
        if (auto xml = p.getXmlValue ("v_presets"))
            for (auto* e : xml->getChildIterator())
            {
                juce::PropertySet ps;
                ps.restoreFromXml (*e);
                ViewStyle s;
                s.loadFrom (ps);
                l.push_back ({ e->getStringAttribute ("name"), s });
            }
        return l;
    }

    static void store (juce::PropertiesFile& p, const List& l)
    {
        juce::XmlElement root ("PRESETS");
        for (auto& [name, s] : l)
        {
            juce::PropertySet ps;
            s.saveTo (ps);
            auto e = ps.createXml ("PRESET");
            e->setAttribute ("name", name);
            root.addChildElement (e.release());
        }
        p.setValue ("v_presets", &root);
        p.saveIfNeeded();
    }

    static void saveUser (juce::PropertiesFile& p, const juce::String& name, const ViewStyle& s)
    {
        auto l = user (p);
        bool replaced = false;
        for (auto& e : l)
            if (e.first == name) { e.second = s; replaced = true; }
        if (! replaced) l.push_back ({ name, s });
        store (p, l);
    }

    static void removeUser (juce::PropertiesFile& p, const juce::String& name)
    {
        auto l = user (p);
        l.erase (std::remove_if (l.begin(), l.end(), [&] (auto& e) { return e.first == name; }), l.end());
        store (p, l);
    }
};

//==============================================================================
struct Theme
{
    juce::Colour mix, ref, diff;
};

inline int& currentThemeIndex() { static int idx = 0; return idx; }
inline bool& colorfulUi() { static bool c = true; return c; }
inline Theme& customThemeColours() { static Theme t { juce::Colour (0xffd4a537), juce::Colour (0xff46c3f0), juce::Colour (0xffc9a2f2) }; return t; }

// Regenbogen über den Frequenzbereich: 0 = 20 Hz (rot) … 1 = 20 kHz (violett)
inline juce::Colour rainbow (float t, float alpha = 1.0f)
{
    return juce::Colour::fromHSV (0.80f * juce::jlimit (0.0f, 1.0f, t), 0.72f, 1.0f, alpha);
}

inline juce::ColourGradient rainbowGradient (float x0, float x1, float y, float alpha)
{
    juce::ColourGradient g (rainbow (0.0f, alpha), x0, y, rainbow (1.0f, alpha), x1, y, false);
    for (int i = 1; i < 8; ++i)
        g.addColour (i / 8.0, rainbow ((float) i / 8.0f, alpha));
    return g;
}

inline const Theme& theme()
{
    static const Theme themes[5] {
        { juce::Colour (0xffd4a537), juce::Colour (0xff46c3f0), juce::Colour (0xffc9a2f2) },  // Keller54
        { juce::Colour (0xffff4fd8), juce::Colour (0xff3ef0ff), juce::Colour (0xfff7f06a) },  // Neon
        { juce::Colour (0xff5fd36b), juce::Colour (0xffff9a3c), juce::Colour (0xff9ab8ff) },  // Studio
        { juce::Colour (0xfff2f2f2), juce::Colour (0xffff5a5a), juce::Colour (0xffffd84a) },  // Kontrast
        { juce::Colour (0xff2fe0c2), juce::Colour (0xffff7f6e), juce::Colour (0xffb9a6ff) },  // Ozean
    };
    const int i = currentThemeIndex();
    return i == ViewStyle::customTheme ? customThemeColours() : themes[juce::jlimit (0, 4, i)];
}

inline const char* themeName (int i)
{
    static const char* n[] { "Keller54 (Gold / Blau)", "Neon (Magenta / Cyan)", "Studio (Grün / Orange)",
                             "Kontrast (Weiß / Rot)", "Ozean (Türkis / Koralle)", "Eigene Farben" };
    return n[juce::jlimit (0, ViewStyle::numThemes - 1, i)];
}

} // namespace k54a

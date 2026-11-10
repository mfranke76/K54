// K-54 S · ANSICHT: Farbschemata, eigene Farben, farbige Oberflaeche, gespeicherte Ansichten (gelten fuer alle Projekte)
#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "LookAndFeel.h"

struct SynthStyle
{
    int theme = 0;                // 0 Keller54 … 4 Ozean, 5 eigene Farben
    bool colorful = true;         // farbige Oberflaeche
    int accentMode = 0;           // 0 bunt je Sektion, 1 eigene Akzentfarbe
    juce::uint32 customMain = 0xffD4AF6A, customSecond = 0xff6FC3DF, customAccent = 0xffffb03a;

    static constexpr int numThemes = 6, customTheme = 5;

    static const char* themeName (int i)
    {
        static const char* n[] { "Keller54 (Gold / Blau)", "Neon (Magenta / Cyan)", "Studio (Gruen / Orange)",
                                 "Kontrast (Weiss / Rot)", "Ozean (Tuerkis / Koralle)", "Eigene Farben" };
        return n[juce::jlimit (0, numThemes - 1, i)];
    }

    /** setzt die Farben global (K54Colours / K54View) */
    void apply() const
    {
        static const juce::uint32 mains[]   { 0xffD4AF6A, 0xffFF4FD8, 0xff5FD36B, 0xffF2F2F2, 0xff2FE0C2 };
        static const juce::uint32 seconds[] { 0xff6FC3DF, 0xff3EF0FF, 0xffFF9A3C, 0xffFF5A5A, 0xffFF7F6E };
        const auto main   = theme == customTheme ? juce::Colour (customMain)   : juce::Colour (mains[juce::jlimit (0, 4, theme)]);
        const auto second = theme == customTheme ? juce::Colour (customSecond) : juce::Colour (seconds[juce::jlimit (0, 4, theme)]);
        K54Colours::gold = main;
        K54Colours::goldDim = main.withMultipliedBrightness (0.62f);
        K54Colours::accent2 = second;
        K54View::colorful = colorful;
        K54View::accentMode = accentMode;
        K54View::customAccent = juce::Colour (customAccent);
    }

    static juce::uint32 hexOr (const juce::PropertySet& p, const char* key, juce::uint32 def)
    {
        const auto v = p.getValue (key);
        return v.isEmpty() ? def : (juce::uint32) v.getHexValue64();
    }

    void loadFrom (const juce::PropertySet& p)
    {
        theme        = juce::jlimit (0, numThemes - 1, p.getIntValue ("s_theme", theme));
        colorful     = p.getBoolValue ("s_colorful", colorful);
        accentMode   = p.getIntValue ("s_accentMode", accentMode);
        customMain   = hexOr (p, "s_customMain", customMain);
        customSecond = hexOr (p, "s_customSecond", customSecond);
        customAccent = hexOr (p, "s_customAccent", customAccent);
    }

    void saveTo (juce::PropertySet& p) const
    {
        p.setValue ("s_theme", theme);
        p.setValue ("s_colorful", colorful);
        p.setValue ("s_accentMode", accentMode);
        p.setValue ("s_customMain", juce::String::toHexString ((juce::int64) customMain));
        p.setValue ("s_customSecond", juce::String::toHexString ((juce::int64) customSecond));
        p.setValue ("s_customAccent", juce::String::toHexString ((juce::int64) customAccent));
    }

    bool operator== (const SynthStyle& o) const
    {
        return theme == o.theme && colorful == o.colorful && accentMode == o.accentMode
            && customMain == o.customMain && customSecond == o.customSecond && customAccent == o.customAccent;
    }

    // ---- Ansichten ------------------------------------------------------------
    using List = std::vector<std::pair<juce::String, SynthStyle>>;

    static List builtIn()
    {
        List l;
        l.push_back ({ "Standard (farbig)", SynthStyle {} });
        SynthStyle classic; classic.colorful = false;
        l.push_back ({ "Klassisch (schlicht)", classic });
        for (int t = 1; t <= 4; ++t)
        {
            SynthStyle s; s.theme = t;
            l.push_back ({ juce::String (themeName (t)).upToFirstOccurrenceOf (" (", false, false), s });
        }
        return l;
    }

    static List user (juce::PropertiesFile& p)
    {
        List l;
        if (auto xml = p.getXmlValue ("s_presets"))
            for (auto* e : xml->getChildIterator())
            {
                juce::PropertySet ps;
                ps.restoreFromXml (*e);
                SynthStyle s;
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
        p.setValue ("s_presets", &root);
        p.saveIfNeeded();
    }

    static void saveUser (juce::PropertiesFile& p, const juce::String& name, const SynthStyle& s)
    {
        auto l = user (p);
        bool replaced = false;
        for (auto& e : l) if (e.first == name) { e.second = s; replaced = true; }
        if (! replaced) l.push_back ({ name, s });
        store (p, l);
    }

    static void removeUser (juce::PropertiesFile& p, const juce::String& name)
    {
        auto l = user (p);
        l.erase (std::remove_if (l.begin(), l.end(), [&] (auto& e) { return e.first == name; }), l.end());
        store (p, l);
    }

    static std::unique_ptr<juce::PropertiesFile> openSettings()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "K-54 S";
        o.filenameSuffix = ".settings";
        o.folderName = "Keller54";
        o.osxLibrarySubFolder = "Application Support";
        o.storageFormat = juce::PropertiesFile::storeAsXML;
        return std::make_unique<juce::PropertiesFile> (o);
    }
};

//==============================================================================
class K54ColourSwatch : public juce::Component,
                        public juce::SettableTooltipClient,
                        private juce::ChangeListener
{
public:
    ~K54ColourSwatch() override
    {
        if (selector != nullptr)
        {
            selector->removeChangeListener (this);
            if (auto* box = selector->findParentComponentOfClass<juce::CallOutBox>()) box->dismiss();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (colour);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (isMouseOver() ? K54Colours::cream : K54Colours::line.brighter (0.5f));
        g.drawRoundedRectangle (r, 5.0f, 1.2f);
        g.setColour (colour.getPerceivedBrightness() > 0.55f ? juce::Colour (0xff111111) : juce::Colours::white);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 9.0f, 0.1f));
        g.drawText (letter, r, juce::Justification::centred);
    }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

    void mouseDown (const juce::MouseEvent&) override
    {
        auto sel = std::make_unique<juce::ColourSelector> (juce::ColourSelector::showColourAtTop
                                                           | juce::ColourSelector::showSliders
                                                           | juce::ColourSelector::showColourspace);
        sel->setCurrentColour (colour, juce::dontSendNotification);
        sel->setColour (juce::ColourSelector::backgroundColourId, K54Colours::panelHi);
        sel->setSize (250, 290);
        sel->addChangeListener (this);
        selector = sel.get();
        juce::Component* parent = findParentComponentOfClass<juce::AudioProcessorEditor>();
        if (parent == nullptr) parent = getTopLevelComponent();
        juce::CallOutBox::launchAsynchronously (std::move (sel), parent->getLocalArea (this, getLocalBounds()), parent);
    }

    juce::Colour colour { juce::Colours::grey };
    juce::String letter;
    std::function<void (juce::Colour)> onChange;

private:
    void changeListenerCallback (juce::ChangeBroadcaster* src) override
    {
        if (auto* s = dynamic_cast<juce::ColourSelector*> (src))
        {
            colour = s->getCurrentColour().withAlpha (1.0f);
            repaint();
            if (onChange) onChange (colour);
        }
    }
    juce::Component::SafePointer<juce::ColourSelector> selector;
};

//==============================================================================
class SynthViewPanel : public juce::Component
{
public:
    std::function<void (const SynthStyle&)> onChange;
    std::function<void()> onPresetsChanged;

    explicit SynthViewPanel (juce::PropertiesFile& s) : settings (s)
    {
        for (int i = 0; i < SynthStyle::numThemes; ++i) cbTheme.addItem (SynthStyle::themeName (i), i + 1);
        cbSurface.addItem ("Schlicht", 1);
        cbSurface.addItem ("Farbig", 2);
        cbAccent.addItem ("Bunt (je Sektion)", 1);
        cbAccent.addItem ("Eigene Farbe (A)", 2);
        for (auto* cb : { &cbTheme, &cbSurface, &cbAccent })
        {
            cb->onChange = [this] { changed(); };
            addAndMakeVisible (cb);
        }

        const char* letters[] { "H", "Z", "A" };
        const char* tips[] { "Hauptfarbe (Regler, Marken, Schrift)", "Zweitfarbe (Modulation, Loop)", "Akzentfarbe der Sektionen" };
        for (int i = 0; i < 3; ++i)
        {
            auto& sw = swatches[(size_t) i];
            sw.letter = letters[i];
            sw.setTooltip (tips[i]);
            sw.onChange = [this, i] (juce::Colour c)
            {
                if (i == 0) current.customMain = c.getARGB();
                if (i == 1) current.customSecond = c.getARGB();
                if (i == 2) { current.customAccent = c.getARGB(); cbAccent.setSelectedId (2, juce::dontSendNotification); }
                else        cbTheme.setSelectedId (SynthStyle::customTheme + 1, juce::dontSendNotification);
                changed();
            };
            addAndMakeVisible (sw);
        }

        cbPreset.setTextWhenNothingSelected (juce::String::fromUTF8 ("– wählen –"));
        cbPreset.onChange = [this]
        {
            const int idx = cbPreset.getSelectedId() - 1;
            if (idx >= 0 && idx < (int) presets.size())
            {
                setStyle (presets[(size_t) idx].second);
                changed();
            }
        };
        addAndMakeVisible (cbPreset);
        btnSave.setButtonText (juce::String::fromUTF8 ("SPEICHERN …"));
        btnSave.onClick = [this] { askName(); };
        btnDelete.setButtonText (juce::String::fromUTF8 ("LÖSCHEN"));
        btnDelete.onClick = [this]
        {
            const int idx = cbPreset.getSelectedId() - 1;
            if (idx >= numBuiltIn && idx < (int) presets.size())
            {
                SynthStyle::removeUser (settings, presets[(size_t) idx].first);
                refreshPresets();
                if (onPresetsChanged) onPresetsChanged();
            }
        };
        btnDefaults.setButtonText ("STANDARD");
        btnDefaults.onClick = [this] { setStyle (SynthStyle {}); changed(); };
        btnClose.setButtonText ("FERTIG");
        btnClose.setToggleState (true, juce::dontSendNotification);
        btnClose.onClick = [this] { setVisible (false); };
        for (auto* b : { &btnSave, &btnDelete, &btnDefaults, &btnClose }) addAndMakeVisible (b);
        refreshPresets();
    }

    void setStyle (const SynthStyle& s)
    {
        loading = true;
        current = s;
        cbTheme.setSelectedId (s.theme + 1, juce::dontSendNotification);
        cbSurface.setSelectedId (s.colorful ? 2 : 1, juce::dontSendNotification);
        cbAccent.setSelectedId (s.accentMode + 1, juce::dontSendNotification);
        swatches[0].colour = juce::Colour (s.customMain);
        swatches[1].colour = juce::Colour (s.customSecond);
        swatches[2].colour = juce::Colour (s.customAccent);
        for (auto& sw : swatches) sw.repaint();
        loading = false;
        selectMatching();
        repaint();
    }

    SynthStyle getStyle() const
    {
        SynthStyle s = current;
        s.theme = juce::jmax (0, cbTheme.getSelectedId() - 1);
        s.colorful = cbSurface.getSelectedId() == 2;
        s.accentMode = juce::jmax (0, cbAccent.getSelectedId() - 1);
        return s;
    }

    void refreshPresets()
    {
        presets = SynthStyle::builtIn();
        numBuiltIn = (int) presets.size();
        for (auto& p : SynthStyle::user (settings)) presets.push_back (p);
        cbPreset.clear (juce::dontSendNotification);
        for (int i = 0; i < (int) presets.size(); ++i)
        {
            if (i == numBuiltIn) cbPreset.addSectionHeading ("Eigene");
            cbPreset.addItem (presets[(size_t) i].first, i + 1);
        }
        selectMatching();
    }

    void savePreset (const juce::String& name)
    {
        SynthStyle::saveUser (settings, name, getStyle());
        refreshPresets();
        if (onPresetsChanged) onPresetsChanged();
    }

    void mouseDown (const juce::MouseEvent& e) override { if (! box.contains (e.position)) setVisible (false); }

    void resized() override
    {
        box = getLocalBounds().toFloat().withSizeKeepingCentre (560.0f, 360.0f);
        const int x = (int) box.getX() + 28, cx = x + 150, y0 = (int) box.getY() + 64;
        cbTheme.setBounds (cx, y0, 220, 26);
        for (int i = 0; i < 3; ++i) swatches[(size_t) i].setBounds (cx + i * 46, y0 + 34, 40, 26);
        cbSurface.setBounds (cx, y0 + 68, 220, 26);
        cbAccent.setBounds (cx, y0 + 102, 220, 26);
        cbPreset.setBounds (cx, y0 + 166, 220, 26);
        btnSave.setBounds (cx, y0 + 200, 110, 26);
        btnDelete.setBounds (cx + 118, y0 + 200, 102, 26);
        btnDefaults.setBounds (x, (int) box.getBottom() - 46, 110, 30);
        btnClose.setBounds ((int) box.getRight() - 138, (int) box.getBottom() - 46, 110, 30);
    }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        g.fillAll (juce::Colours::black.withAlpha (0.6f));
        g.setColour (panel);
        g.fillRoundedRectangle (box, 10.0f);
        if (K54View::colorful)
        {
            g.setGradientFill (K54View::rainbowGradient (box.getX() + 10.0f, box.getRight() - 10.0f, 0.0f, 0.9f));
            g.fillRect (box.getX() + 10.0f, box.getY(), box.getWidth() - 20.0f, 2.0f);
        }
        g.setColour (gold);
        g.drawRoundedRectangle (box, 10.0f, 1.5f);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 15.0f, 0.2f));
        g.drawText ("ANSICHT", box.reduced (28.0f, 16.0f).withHeight (26.0f), juce::Justification::centredLeft);
        g.setColour (creamDim);
        g.setFont (juce::FontOptions (11.5f));
        g.drawText (juce::String::fromUTF8 ("wirkt sofort · gilt für alle Projekte"), box.reduced (28.0f, 16.0f).withHeight (26.0f),
                    juce::Justification::centredRight);

        const float x = box.getX() + 28.0f, y0 = box.getY() + 64.0f;
        auto label = [&] (const char* t, float y)
        {
            g.setColour (cream.withAlpha (0.85f));
            g.setFont (juce::FontOptions (13.0f));
            g.drawText (juce::String::fromUTF8 (t), juce::Rectangle<float> (x, y, 145.0f, 26.0f), juce::Justification::centredLeft);
        };
        label ("Farbschema", y0);
        label ("Eigene Farben", y0 + 34.0f);
        label ("Oberfläche", y0 + 68.0f);
        label ("Sektions-Farben", y0 + 102.0f);
        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 9.0f, 0.25f));
        g.drawText ("GESPEICHERTE ANSICHTEN", juce::Rectangle<float> (x, y0 + 140.0f, 300.0f, 18.0f), juce::Justification::centredLeft);
        label ("Ansicht", y0 + 166.0f);
    }

private:
    void changed()
    {
        if (loading) return;
        current = getStyle();
        selectMatching();
        if (onChange) onChange (current);
    }

    void selectMatching()
    {
        int match = 0;
        for (int i = (int) presets.size() - 1; i >= 0; --i)
            if (presets[(size_t) i].second == current) match = i + 1;
        cbPreset.setSelectedId (match, juce::dontSendNotification);
        btnDelete.setEnabled (match - 1 >= numBuiltIn);
    }

    void askName()
    {
        auto* aw = new juce::AlertWindow ("Ansicht speichern", juce::String::fromUTF8 ("Name für diese Ansicht:"),
                                          juce::MessageBoxIconType::NoIcon, this);
        aw->addTextEditor ("name", "Meine Ansicht");
        aw->addButton ("Speichern", 1, juce::KeyPress (juce::KeyPress::returnKey));
        aw->addButton ("Abbrechen", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        juce::Component::SafePointer<SynthViewPanel> safe (this);
        aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, aw] (int r)
        {
            if (safe == nullptr || r != 1) return;
            const auto n = aw->getTextEditorContents ("name").trim();
            if (n.isNotEmpty()) safe->savePreset (n);
        }), true);
    }

    juce::PropertiesFile& settings;
    juce::ComboBox cbTheme, cbSurface, cbAccent, cbPreset;
    std::array<K54ColourSwatch, 3> swatches;
    juce::TextButton btnSave, btnDelete, btnDefaults, btnClose;
    juce::Rectangle<float> box;
    SynthStyle current;
    SynthStyle::List presets;
    int numBuiltIn = 0;
    bool loading = false;
};

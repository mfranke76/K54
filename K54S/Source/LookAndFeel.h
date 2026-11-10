#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

// Farben aus dem Keller54-Logo (Schwarz / Gold / Creme)
namespace K54Colours
{
    inline const juce::Colour black    { 0xff09090B };
    inline const juce::Colour panel    { 0xff131316 };
    inline const juce::Colour panelHi  { 0xff1D1D22 };
    inline const juce::Colour line     { 0xff26262C };
    inline const juce::Colour knobBody { 0xff202026 };
    // ab 1.4 umschaltbar (ANSICHT): Hauptfarbe, gedimmte Hauptfarbe, Zweitfarbe
    inline juce::Colour accent2  { 0xff6FC3DF };   // Modulation / Loop (kuehles Blau)
    inline juce::Colour gold     { 0xffD4AF6A };
    inline juce::Colour goldDim  { 0xff9A7430 };
    inline const juce::Colour cream    { 0xffF3EEE4 };
    inline const juce::Colour creamDim { 0xffB9B2A4 };
}

struct K54Fonts
{
    static juce::Typeface::Ptr syncopateBold()
    {
        static auto t = juce::Typeface::createSystemTypefaceFor (BinaryData::SyncopateBold_ttf, BinaryData::SyncopateBold_ttfSize);
        return t;
    }
    static juce::Typeface::Ptr syncopate()
    {
        static auto t = juce::Typeface::createSystemTypefaceFor (BinaryData::SyncopateRegular_ttf, BinaryData::SyncopateRegular_ttfSize);
        return t;
    }
    static juce::Typeface::Ptr monoton()
    {
        static auto t = juce::Typeface::createSystemTypefaceFor (BinaryData::MonotonRegular_ttf, BinaryData::MonotonRegular_ttfSize);
        return t;
    }
    static juce::Font make (juce::Typeface::Ptr tf, float height, float kerning = 0.0f)
    {
        return juce::Font (juce::FontOptions (tf).withHeight (height).withKerningFactor (kerning));
    }
};

// Keller54-Signet: Pegelanzeige mit goldener Spitze (aus dem Logo)
inline void drawKellerMeter (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour bars, juce::Colour peak)
{
    const float sc = std::min (area.getWidth(), area.getHeight()) / 64.0f;
    const auto o = area.getCentre() - juce::Point<float> (32.0f * sc, 32.0f * sc);
    auto P = [&] (float x, float y) { return o + juce::Point<float> (x * sc, y * sc); };

    const float bx[] { 11, 18, 25, 32, 39, 46, 53 };
    const float by[] { 42, 30, 27, 27, 24, 36, 48 };
    juce::PathStrokeType stroke (3.4f * sc, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path p;
    for (int i = 0; i < 7; ++i)
    {
        p.startNewSubPath (P (bx[i], 58));
        p.lineTo (P (bx[i], by[i]));
    }
    g.setColour (bars);
    g.strokePath (p, stroke);

    juce::Path pk;
    pk.startNewSubPath (P (25, 21)); pk.lineTo (P (25, 18));
    pk.startNewSubPath (P (32, 21)); pk.lineTo (P (32, 6));
    g.setColour (peak);
    g.strokePath (pk, stroke);
}

// Ansicht (ANSICHT-Fenster): farbige Oberflaeche und Akzentfarben der Sektionen
namespace K54View
{
    inline bool colorful = true;
    inline int accentMode = 0;                       // 0 bunt je Sektion, 1 eigene Akzentfarbe
    inline juce::Colour customAccent { 0xffffb03a };

    inline juce::Colour rainbow (float t, float alpha = 1.0f)
    {
        return juce::Colour::fromHSV (0.80f * juce::jlimit (0.0f, 1.0f, t), 0.70f, 1.0f, alpha);
    }

    inline juce::ColourGradient rainbowGradient (float x0, float x1, float y, float alpha)
    {
        juce::ColourGradient g (rainbow (0.0f, alpha), x0, y, rainbow (1.0f, alpha), x1, y, false);
        for (int i = 1; i < 8; ++i) g.addColour (i / 8.0, rainbow ((float) i / 8.0f, alpha));
        return g;
    }

    inline juce::Colour sectionAccent (const juce::String& title)
    {
        if (accentMode == 1) return customAccent;
        static const juce::uint32 pal[] { 0xffffb03a, 0xffff6f91, 0xff5fe08a, 0xffb58cff, 0xff4fd2ff, 0xffffe066, 0xffff8a5c, 0xff6fe0d0 };
        return juce::Colour (pal[(juce::uint32) title.hashCode() % 8u]);
    }
}

// Sektion im K-54 S-Look: weicher Verlauf, goldene Marke vor dem Titel
inline void drawK55Section (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    using namespace K54Colours;
    auto b = r.toFloat();
    if (K54View::colorful)
    {
        const auto acc = K54View::sectionAccent (title);
        g.setGradientFill (juce::ColourGradient (panelHi.interpolatedWith (acc, 0.12f).withAlpha (0.9f), 0.0f, b.getY(),
                                                 panel, 0.0f, b.getY() + 110.0f, false));
        g.fillRoundedRectangle (b, 10.0f);
        g.setColour (acc.withAlpha (0.32f));
        g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);
        juce::ColourGradient top (acc.withAlpha (0.0f), b.getX(), 0.0f, acc.withAlpha (0.0f), b.getRight(), 0.0f, false);
        top.addColour (0.15, acc.withAlpha (0.9f));
        top.addColour (0.6, acc.withAlpha (0.3f));
        g.setGradientFill (top);
        g.fillRect (b.getX() + 10.0f, b.getY(), b.getWidth() - 20.0f, 2.0f);
        g.setColour (acc);
        g.fillRoundedRectangle (b.getX() + 12.0f, b.getY() + 12.0f, 3.0f, 12.0f, 1.5f);
        g.setColour (cream);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.0f, 0.28f));
        g.drawText (title, juce::Rectangle<float> (b.getX() + 22.0f, b.getY() + 8.0f, b.getWidth() - 30.0f, 20.0f), juce::Justification::centredLeft);
        return;
    }
    g.setGradientFill (juce::ColourGradient (panelHi.withAlpha (0.75f), 0.0f, b.getY(), panel, 0.0f, b.getY() + 90.0f, false));
    g.fillRoundedRectangle (b, 10.0f);
    g.setColour (line.withAlpha (0.8f));
    g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);
    g.setColour (gold);
    g.fillRoundedRectangle (b.getX() + 12.0f, b.getY() + 12.0f, 3.0f, 12.0f, 1.5f);
    g.setColour (cream);
    g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.0f, 0.28f));
    g.drawText (title, juce::Rectangle<float> (b.getX() + 22.0f, b.getY() + 8.0f, b.getWidth() - 30.0f, 20.0f), juce::Justification::centredLeft);
}

class K54LookAndFeel : public juce::LookAndFeel_V4
{
public:
    K54LookAndFeel() { refreshColours(); }

    // nach einem Farbwechsel (ANSICHT) erneut aufrufen
    void refreshColours()
    {
        using namespace K54Colours;
        setColour (juce::Slider::textBoxTextColourId,        creamDim);
        setColour (juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId,  juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId,   goldDim);
        setColour (juce::Label::textColourId,                creamDim);
        setColour (juce::ComboBox::backgroundColourId,       panelHi);
        setColour (juce::ComboBox::outlineColourId,          line);
        setColour (juce::ComboBox::textColourId,             cream);
        setColour (juce::ComboBox::arrowColourId,            gold);
        setColour (juce::ComboBox::focusedOutlineColourId,   goldDim);
        setColour (juce::PopupMenu::backgroundColourId,      panelHi);
        setColour (juce::PopupMenu::textColourId,            cream);
        setColour (juce::PopupMenu::headerTextColourId,      gold);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, goldDim);
        setColour (juce::PopupMenu::highlightedTextColourId, black);
        setColour (juce::TextButton::buttonColourId,         panelHi);
        setColour (juce::TextButton::buttonOnColourId,       goldDim);
        setColour (juce::TextButton::textColourOffId,        cream);
        setColour (juce::TextButton::textColourOnId,         black);
        setColour (juce::AlertWindow::backgroundColourId,    panel);
        setColour (juce::AlertWindow::textColourId,          cream);
        setColour (juce::AlertWindow::outlineColourId,       gold);
        setColour (juce::TextEditor::backgroundColourId,     panelHi);
        setColour (juce::TextEditor::textColourId,           cream);
        setColour (juce::TextEditor::outlineColourId,        line);
        setColour (juce::TextEditor::focusedOutlineColourId, gold);
        setColour (juce::CaretComponent::caretColourId,      gold);
    }

    // Moderner, flacher Regler: Ring mit Leuchten, ruhiger Knopf, kurzer Zeiger
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        using namespace K54Colours;
        auto b = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (2.0f);
        const float r  = std::min (b.getWidth(), b.getHeight()) * 0.5f;
        const float cx = b.getCentreX(), cy = b.getCentreY();
        const float lineW = std::max (2.5f, r * 0.1f);
        const float arcR = r - lineW * 0.5f - 1.0f;
        const float angle = startAngle + pos * (endAngle - startAngle);
        const bool on = s.isEnabled();

        juce::Path track;
        track.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (line);
        g.strokePath (track, { lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

        const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
        const juce::Colour col = s.getProperties().contains ("accent") ? accent2 : gold;
        const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
        if (std::abs (angle - from) > 0.001f)
        {
            juce::Path val;
            val.addCentredArc (cx, cy, arcR, arcR, 0.0f, std::min (from, angle), std::max (from, angle), true);
            if (on)
            {
                g.setColour (col.withAlpha (0.18f));
                g.strokePath (val, { lineW * 2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
            }
            g.setColour (on ? col : goldDim.withAlpha (0.5f));
            g.strokePath (val, { lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
        }

        const float kr = r - lineW - 4.0f;
        g.setColour (knobBody);
        g.fillEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), cx, cy - kr,
                                                 juce::Colours::transparentWhite, cx, cy + kr * 0.4f, false));
        g.fillEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f);

        juce::Path ptr;
        const float pw = std::max (2.0f, kr * 0.13f);
        ptr.addRoundedRectangle (-pw * 0.5f, -kr + 3.0f, pw, kr * 0.42f, pw * 0.5f);
        g.setColour (on ? cream : creamDim.withAlpha (0.5f));
        g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (cx, cy));
    }

    // Wertanzeige unter den Reglern: ohne Kasten, gleiche Schrift
    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.04f));
        l->setColour (juce::Label::textColourId, K54Colours::creamDim);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineWhenEditingColourId, K54Colours::goldDim);
        l->setJustificationType (juce::Justification::centred);
        return l;
    }

    // Balken-Regler (Mod-Matrix): bipolar von der Mitte aus
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style != juce::Slider::LinearBar)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, minPos, maxPos, style, s);
            return;
        }
        using namespace K54Colours;
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (0.5f, 3.0f);
        g.setColour (panelHi);
        g.fillRoundedRectangle (r, 4.0f);
        const float mid = r.getCentreX();
        const float pos = juce::jlimit (r.getX(), r.getRight(), sliderPos);
        auto fill = juce::Rectangle<float> (std::min (mid, pos), r.getY(), std::abs (pos - mid), r.getHeight());
        g.setColour (s.isEnabled() ? accent2.withAlpha (0.85f) : line);
        g.fillRoundedRectangle (fill, 3.0f);
        g.setColour (line.brighter (0.3f));
        g.fillRect (mid - 0.5f, r.getY() + 2.0f, 1.0f, r.getHeight() - 4.0f);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        using namespace K54Colours;
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (box.hasKeyboardFocus (false) ? goldDim : line);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);

        juce::Path arrow;
        const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
        g.setColour (gold);
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override   { return K54Fonts::make (K54Fonts::syncopate(), 9.5f, 0.02f); }
    juce::Font getPopupMenuFont() override                   { return K54Fonts::make (K54Fonts::syncopate(), 11.0f, 0.02f); }
    juce::Font getLabelFont (juce::Label& l) override        { return l.getFont(); }
    juce::Font getTextButtonFont (juce::TextButton& b, int) override
    {
        // einzelne Knoepfe koennen eine eigene Schriftgroesse haben (Property "fontSize")
        const float size = b.getProperties().contains ("fontSize") ? (float) b.getProperties()["fontSize"] : 10.0f;
        return K54Fonts::make (K54Fonts::syncopateBold(), size, size > 12.0f ? 0.0f : (size < 9.5f ? 0.04f : 0.12f));
    }
    juce::Font getAlertWindowMessageFont() override          { return K54Fonts::make (K54Fonts::syncopate(), 12.0f); }
    juce::Font getAlertWindowTitleFont() override            { return K54Fonts::make (K54Fonts::syncopateBold(), 14.0f, 0.1f); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (6, 1, box.getWidth() - 24, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        using namespace K54Colours;
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        g.setColour (! b.isEnabled() ? panel : (on || down ? (on ? gold : goldDim) : (over ? juce::Colour (0xff26262A) : panelHi)));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (on ? gold : (over ? goldDim : line));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setFont (getTextButtonFont (b, b.getHeight()));
        auto col = b.findColour (b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId);
        g.setColour (b.isEnabled() ? col : col.withAlpha (0.35f));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 2), juce::Justification::centred, 1, 0.8f);
    }
};

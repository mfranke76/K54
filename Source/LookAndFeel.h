#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

// Farben aus dem Keller54-Logo (Schwarz / Gold / Creme)
namespace K54Colours
{
    inline const juce::Colour black    { 0xff0A0A0B };
    inline const juce::Colour panel    { 0xff151517 };
    inline const juce::Colour panelHi  { 0xff1C1C1F };
    inline const juce::Colour line     { 0xff2A2A2E };
    inline const juce::Colour gold     { 0xffD4AF6A };
    inline const juce::Colour goldDim  { 0xff9A7430 };
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

class K54LookAndFeel : public juce::LookAndFeel_V4
{
public:
    K54LookAndFeel()
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

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        using namespace K54Colours;
        auto b = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (2.0f);
        const float r  = std::min (b.getWidth(), b.getHeight()) * 0.5f;
        const float cx = b.getCentreX(), cy = b.getCentreY();
        const float arcR = r - 2.0f;
        const float lineW = std::max (2.0f, r * 0.09f);
        const float angle = startAngle + pos * (endAngle - startAngle);

        juce::Path track;
        track.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (line);
        g.strokePath (track, { lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

        const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
        const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
        if (std::abs (angle - from) > 0.001f)
        {
            juce::Path val;
            val.addCentredArc (cx, cy, arcR, arcR, 0.0f, std::min (from, angle), std::max (from, angle), true);
            g.setColour (s.isEnabled() ? gold : goldDim);
            g.strokePath (val, { lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
        }

        const float kr = r - lineW - 4.0f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff303035), cx, cy - kr,
                                                 juce::Colour (0xff111113), cx, cy + kr, false));
        g.fillEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f);
        g.setColour (juce::Colour (0xff3C3C42));
        g.drawEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f, 1.0f);

        juce::Path ptr;
        const float pw = std::max (2.0f, kr * 0.12f);
        ptr.addRoundedRectangle (-pw * 0.5f, -kr + 3.0f, pw, kr * 0.5f, pw * 0.5f);
        g.setColour (cream);
        g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (cx, cy));
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
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return K54Fonts::make (K54Fonts::syncopateBold(), 10.0f, 0.12f); }
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

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

/** Umschalt-Knopf im K-54 S-Look */
inline void setupToggle (juce::TextButton& b, const juce::String& tip)
{
    b.setClickingTogglesState (true);
    if (tip.isNotEmpty()) b.setTooltip (tip);
}

// ---------------------------------------------------------------------------
//  Bausteine fuer die Reiter POWER und SAMPLE (gleicher Look wie die SYNTH-Seite)
// ---------------------------------------------------------------------------
class KitPanel : public juce::Component
{
public:
    explicit KitPanel (K55Processor& p) : proc (p) {}

protected:
    static constexpr int cellH = 84;

    struct Knob
    {
        juce::Slider s;
        juce::Label  l;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a;
    };
    struct Choice
    {
        juce::ComboBox b;
        juce::Label    l;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> a;
    };
    struct Toggle
    {
        juce::TextButton b;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> a;
    };

    Knob& knob (const juce::String& id, const juce::String& caption, bool bipolar = false)
    {
        auto* k = knobs.add (new Knob());
        auto& s = k->s;
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxTextColourId, K54Colours::creamDim);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 15);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
        s.setMouseDragSensitivity (220);
        s.getProperties().set ("bipolar", bipolar);
        addAndMakeVisible (s);
        k->a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s);
        if (auto* param = proc.apvts.getParameter (id))
            s.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        setupLabel (k->l, caption);
        return *k;
    }

    Choice& choice (const juce::String& id, const juce::String& caption)
    {
        auto* c = choices.add (new Choice());
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
            for (int i = 0; i < param->choices.size(); ++i)
                c->b.addItem (param->choices[i].toUpperCase(), i + 1);
        c->b.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (c->b);
        c->a = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, c->b);
        setupLabel (c->l, caption);
        return *c;
    }

    Toggle& toggle (const juce::String& id, const juce::String& text, const juce::String& tip = {})
    {
        auto* t = toggles.add (new Toggle());
        t->b.setButtonText (text);
        setupToggle (t->b, tip);
        addAndMakeVisible (t->b);
        t->a = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, t->b);
        return *t;
    }

    static void place (Knob& k, juce::Rectangle<int> cell)
    {
        k.l.setBounds (cell.removeFromTop (14));
        k.s.setBounds (cell);
    }
    static void place (Choice& c, juce::Rectangle<int> cell)
    {
        c.l.setBounds (cell.removeFromTop (14));
        c.b.setBounds (cell.withSizeKeepingCentre (cell.getWidth() - 6, 26).translated (0, -8));
    }

    /** Auswahlfeld kompakt: Beschriftung (14) + Feld (26) */
    static void placeTight (Choice& c, juce::Rectangle<int> cell)
    {
        c.l.setBounds (cell.removeFromTop (14));
        c.b.setBounds (cell.removeFromTop (26).reduced (3, 0));
    }

    /** Modrad-Anzeige: senkrechter Balken mit Wert */
    static void drawWheelMeter (juce::Graphics& g, juce::Rectangle<int> r, float value, juce::Colour col)
    {
        using namespace K54Colours;
        auto b = r.toFloat();
        g.setColour (panelHi);
        g.fillRoundedRectangle (b, 4.0f);
        g.setColour (line);
        g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
        auto fill = b.reduced (3.0f);
        fill = fill.removeFromBottom (fill.getHeight() * juce::jlimit (0.0f, 1.0f, value));
        g.setColour (col);
        g.fillRoundedRectangle (fill, 3.0f);
        g.setColour (cream);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 9.0f, 0.1f));
        g.drawText (juce::String (juce::roundToInt (value * 100.0f)) + "%", r.translated (0, r.getHeight() + 2).withHeight (14).expanded (14, 0), juce::Justification::centred);
    }

    static juce::String u8 (const char* t) { return juce::String (juce::CharPointer_UTF8 (t)); }

    static void drawSection (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title) { drawK55Section (g, r, title); }

    /** Kompakter Balken-Regler (Mod-Matrix) */
    Knob& bar (const juce::String& id)
    {
        auto* k = knobs.add (new Knob());
        auto& s = k->s;
        s.setSliderStyle (juce::Slider::LinearBar);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 15);
        s.setColour (juce::Slider::textBoxTextColourId, K54Colours::cream);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (s);
        k->a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s);
        if (auto* param = proc.apvts.getParameter (id))
            s.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        return *k;
    }

    static void drawSubtitle (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text)
    {
        using namespace K54Colours;
        g.setColour (creamDim.withAlpha (0.7f));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.25f));
        g.drawText (text, r, juce::Justification::centredLeft);
        const int textW = (int) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text) + 8;
        g.setColour (line);
        g.fillRect (r.getX() + textW, r.getCentreY(), r.getWidth() - textW, 1);
    }

    static void drawHint (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text)
    {
        juce::AttributedString as;
        as.setJustification (juce::Justification::topLeft);
        as.setWordWrap (juce::AttributedString::byWord);
        as.setLineSpacing (4.0f);
        as.append (text, K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.06f), K54Colours::creamDim.withAlpha (0.75f));
        juce::TextLayout tl;
        tl.createLayout (as, (float) r.getWidth());
        tl.draw (g, r.toFloat());
    }

    K55Processor& proc;
    juce::OwnedArray<Knob>   knobs;
    juce::OwnedArray<Choice> choices;
    juce::OwnedArray<Toggle> toggles;

private:
    void setupLabel (juce::Label& l, const juce::String& caption)
    {
        l.setText (caption.toUpperCase(), juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.08f));
        addAndMakeVisible (l);
    }
};

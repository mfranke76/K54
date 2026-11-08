// K-54 Analyzer · Keller54 · by Frank E.
#include "PluginEditor.h"

namespace k54a
{
juce::String u8s (const char* s) { return juce::String::fromUTF8 (s); }

static juce::Font font (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

static juce::String fmt (float v, int dec = 1, bool sign = false)
{
    if (v <= -90.0f || std::isnan (v)) return u8s ("–");
    auto s = juce::String (v, dec).replaceCharacter ('.', ',');
    if (sign && v > 0.0f) s = "+" + s;
    return s;
}

static juce::String mmss (double sec)
{
    const int t = (int) std::floor (juce::jmax (0.0, sec));
    return juce::String (t / 60) + ":" + juce::String (t % 60).paddedLeft ('0', 2);
}

static void setAccent (juce::Button& b, juce::Colour c) { b.getProperties().set ("accent", (int) c.getARGB()); }

static const float kTargets[]  { 1000.0f, -14.0f, -16.0f, -23.0f, -9.0f, -24.0f };
static const float kTpLimits[] { -1.0f, -1.0f, -1.0f, -1.0f, -0.3f, -2.0f };
static const char* kBandPhrase[] { "Sub-Bass", "Bass", "untere Mitten", "Mitten", "Präsenz", "Höhen" };

//==============================================================================
K54Look::K54Look()
{
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::ComboBox::backgroundColourId, col::panel2);
    setColour (juce::ComboBox::outlineColourId, col::line);
    setColour (juce::ComboBox::arrowColourId, col::gold);
    setColour (juce::PopupMenu::backgroundColourId, col::panel2);
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::gold);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff111111));
    setColour (juce::Label::textColourId, col::dim);
    setColour (juce::Slider::textBoxTextColourId, col::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::highlightColourId, col::gold.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, col::gold);
    setColour (juce::TooltipWindow::backgroundColourId, col::panel2);
    setColour (juce::TooltipWindow::textColourId, col::text);
    setColour (juce::TooltipWindow::outlineColourId, col::gold);
}

void K54Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    auto& props = b.getProperties();
    const juce::Colour accent = props.contains ("accent") ? juce::Colour ((juce::uint32) (int) props["accent"]) : col::gold;
    const bool on = b.getToggleState();

    juce::Colour fill = on ? accent : col::panel2;
    if (over) fill = fill.brighter (on ? 0.08f : 0.18f);
    if (down) fill = fill.darker (0.15f);
    if (! b.isEnabled()) fill = fill.withMultipliedAlpha (0.35f);

    g.setColour (fill);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (on ? accent.brighter (0.25f) : (over ? accent.withAlpha (0.6f) : col::line));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    if ((bool) props.getWithDefault ("dot", false))
    {
        g.setColour (on ? juce::Colour (0xff0e0e0e) : col::ref);
        g.fillEllipse (r.getRight() - 9.0f, r.getY() + 4.0f, 5.0f, 5.0f);
    }
}

void K54Look::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool on = b.getToggleState();
    auto c = on ? juce::Colour (0xff0e0e0e) : col::text;
    if (! b.isEnabled()) c = c.withMultipliedAlpha (0.4f);
    g.setColour (c);
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 2), juce::Justification::centred, 1);
}

juce::Font K54Look::getTextButtonFont (juce::TextButton& b, int)
{
    return font (b.getProperties().contains ("big") ? 16.0f : 12.0f, true);
}

void K54Look::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    const bool alert = (bool) box.getProperties().getWithDefault ("alert", false);
    g.setColour ((alert ? col::warn.withAlpha (0.25f) : col::panel2).brighter (box.isMouseOver (true) ? 0.12f : 0.0f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (alert ? col::warn : col::line);
    g.drawRoundedRectangle (r, 4.0f, alert ? 1.5f : 1.0f);

    juce::Path p;
    const float cx = (float) w - 11.0f, cy = (float) h * 0.5f;
    p.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
    g.setColour (alert ? col::warn : col::gold);
    g.fillPath (p);
}

juce::Font K54Look::getComboBoxFont (juce::ComboBox&) { return font (12.0f); }
juce::Font K54Look::getPopupMenuFont() { return font (13.0f); }

void K54Look::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 0, box.getWidth() - 20, box.getHeight());
    label.setFont (getComboBoxFont (box));
}

void K54Look::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cy = (float) y + (float) h * 0.5f;
    auto track = juce::Rectangle<float> ((float) x, cy - 2.0f, (float) w, 4.0f);
    g.setColour (col::line);
    g.fillRoundedRectangle (track, 2.0f);

    const float zero = (float) x + (float) w * (float) s.valueToProportionOfLength (0.0);
    g.setColour (col::gold);
    g.fillRoundedRectangle (juce::Rectangle<float> (juce::jmin (zero, pos), cy - 2.0f, std::abs (pos - zero), 4.0f), 2.0f);
    g.setColour (col::dim);
    g.fillRect (zero - 0.5f, cy - 6.0f, 1.0f, 12.0f);

    g.setColour (col::goldHi);
    g.fillEllipse (pos - 7.0f, cy - 7.0f, 14.0f, 14.0f);
    g.setColour (col::bg);
    g.fillEllipse (pos - 2.5f, cy - 2.5f, 5.0f, 5.0f);
}

//==============================================================================
// SPEKTRUM
float SpectrumView::xForFreq (float f, juce::Rectangle<float> r) const
{
    return r.getX() + r.getWidth() * std::log (f / SpecMath::fMin) / std::log (SpecMath::fMax / SpecMath::fMin);
}

float SpectrumView::yForDb (float db, juce::Rectangle<float> r) const
{
    const float t = (juce::jlimit (botDb - 2.0f, topDb, db) - topDb) / (botDb - topDb);
    return r.getY() + t * r.getHeight();
}

juce::Path SpectrumView::makePath (const std::vector<float>& v, juce::Rectangle<float> r, bool closed) const
{
    juce::Path p;
    const int n = (int) v.size();
    for (int i = 0; i < n; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) (n - 1);
        const float y = yForDb (v[(size_t) i], r);
        if (i == 0) p.startNewSubPath (x, y);
        else        p.lineTo (x, y);
    }
    if (closed)
    {
        p.lineTo (r.getRight(), r.getBottom() + 2.0f);
        p.lineTo (r.getX(), r.getBottom() + 2.0f);
        p.closeSubPath();
    }
    return p;
}

// Mix minus Referenz, auf 1/3 Oktave geglättet und um den mittleren Abstand (100 Hz – 10 kHz) bereinigt
void SpectrumView::computeDiff()
{
    const int n = SpecMath::numPoints;
    diffTmp.assign ((size_t) n, 0.0f);
    diff.assign ((size_t) n, 0.0f);
    for (int i = 0; i < n; ++i)
        diffTmp[(size_t) i] = mixA.display[(size_t) i] - refA.display[(size_t) i];

    const int half = (int) std::round ((double) (n - 1) / std::log2 (1000.0) / 6.0); // ±1/6 Okt.
    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f; int c = 0;
        for (int k = std::max (0, i - half); k <= std::min (n - 1, i + half); ++k) { s += diffTmp[(size_t) k]; ++c; }
        diff[(size_t) i] = s / (float) c;
    }

    float mean = 0.0f; int c = 0;
    for (int i = 0; i < n; ++i)
    {
        const float f = SpecMath::pointFreq (i);
        if (f >= 100.0f && f <= 10000.0f) { mean += diff[(size_t) i]; ++c; }
    }
    mean /= (float) juce::jmax (1, c);
    for (auto& d : diff) d -= mean;
}

void SpectrumView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (col::plot);
    g.fillRoundedRectangle (b, 4.0f);

    const bool drawDiff = showDiff && refLoaded && showMix;
    auto plot = b.withTrimmedLeft (34.0f).withTrimmedBottom (20.0f).withTrimmedTop (6.0f)
                 .withTrimmedRight (drawDiff ? 34.0f : 8.0f);

    static const float minor[] { 30, 40, 60, 70, 80, 90, 300, 400, 600, 700, 800, 900,
                                 3000, 4000, 6000, 7000, 8000, 9000 };
    static const float major[] { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    static const char* labels[] { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

    g.setColour (col::grid.withAlpha (0.55f));
    for (auto f : minor)
        g.drawVerticalLine ((int) xForFreq (f, plot), plot.getY(), plot.getBottom());

    g.setFont (font (11.0f));
    for (int i = 0; i < 10; ++i)
    {
        const float x = xForFreq (major[i], plot);
        g.setColour (col::grid.brighter (0.25f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setColour (col::dim);
        auto tr = juce::Rectangle<float> (x - 20.0f, plot.getBottom() + 3.0f, 40.0f, 14.0f);
        if (i == 0) tr = tr.withX (x - 2.0f);
        if (i == 9) tr = tr.withX (x - 38.0f);
        g.drawText (labels[i], tr, i == 0 ? juce::Justification::centredLeft
                                          : (i == 9 ? juce::Justification::centredRight : juce::Justification::centred));
    }

    for (int db = 0; db >= (int) botDb; db -= 12)
    {
        const float y = yForDb ((float) db, plot);
        g.setColour (db == 0 ? col::line.brighter (0.4f) : col::grid.brighter (0.15f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (col::dim);
        g.drawText (juce::String (db), juce::Rectangle<float> (0.0f, y - 7.0f, 30.0f, 14.0f), juce::Justification::centredRight);
    }

    if (drawDiff) computeDiff();

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (plot.toNearestInt());

        if (showRef && refLoaded)
        {
            g.setColour (col::ref.withAlpha (0.07f));
            g.fillPath (makePath (refA.display, plot, true));
            if (holdOn)
            {
                g.setColour (col::ref.withAlpha (0.45f));
                g.strokePath (makePath (refA.hold, plot, false), juce::PathStrokeType (0.9f));
            }
        }

        if (showMix)
        {
            juce::ColourGradient grad (col::gold.withAlpha (0.38f), 0.0f, plot.getY(),
                                       col::gold.withAlpha (0.03f), 0.0f, plot.getBottom(), false);
            g.setGradientFill (grad);
            g.fillPath (makePath (mixA.display, plot, true));
            g.setColour (col::gold);
            g.strokePath (makePath (mixA.display, plot, false), juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
            if (holdOn)
            {
                g.setColour (col::goldHi.withAlpha (0.55f));
                g.strokePath (makePath (mixA.hold, plot, false), juce::PathStrokeType (0.9f));
            }
        }

        if (showRef && refLoaded)
        {
            g.setColour (col::ref);
            g.strokePath (makePath (refA.display, plot, false), juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
        }

        if (drawDiff)
        {
            // eigene Skala ±12 dB um die Mitte
            const float mid = plot.getCentreY(), span = plot.getHeight() * 0.5f;
            auto yD = [&] (float d) { return mid - juce::jlimit (-12.0f, 12.0f, d) / 12.0f * span; };
            g.setColour (col::diff.withAlpha (0.35f));
            g.drawHorizontalLine ((int) mid, plot.getX(), plot.getRight());
            juce::Path p;
            for (int i = 0; i < (int) diff.size(); ++i)
            {
                const float x = plot.getX() + plot.getWidth() * (float) i / (float) (diff.size() - 1);
                if (i == 0) p.startNewSubPath (x, yD (diff[0]));
                else        p.lineTo (x, yD (diff[(size_t) i]));
            }
            g.setColour (col::diff);
            g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
        }
    }

    if (drawDiff)
    {
        const float mid = plot.getCentreY(), span = plot.getHeight() * 0.5f;
        g.setFont (font (10.0f, true));
        g.setColour (col::diff);
        for (int d : { 12, 6, 0, -6, -12 })
            g.drawText ((d > 0 ? "+" : "") + juce::String (d), juce::Rectangle<float> (plot.getRight() + 3.0f, mid - (float) d / 12.0f * span - 6.0f, 30.0f, 12.0f),
                        juce::Justification::centredLeft);
        g.drawText ("DIFF", juce::Rectangle<float> (plot.getRight() + 3.0f, plot.getY(), 30.0f, 12.0f), juce::Justification::centredLeft);
    }

    // Hinweise oben links
    {
        juce::String info;
        if (longTerm) info << u8s ("LANGZEIT-MITTEL");
        if (channelName.isNotEmpty()) info << (info.isNotEmpty() ? u8s ("  ·  ") : juce::String()) << channelName;
        if (drawDiff) info << (info.isNotEmpty() ? u8s ("  ·  ") : juce::String()) << u8s ("Lila = Mix minus Referenz (+ = Mix hat mehr)");
        if (info.isNotEmpty())
        {
            g.setFont (font (11.0f, true));
            g.setColour (col::text.withAlpha (0.7f));
            g.drawText (info, plot.reduced (8.0f, 4.0f).withHeight (16.0f), juce::Justification::centredLeft);
        }
    }

    if (! showMix && ! (showRef && refLoaded))
    {
        g.setColour (col::dim);
        g.setFont (font (14.0f));
        g.drawText (u8s ("Beide Kurven ausgeblendet – oben MIX oder REFERENZ einschalten"), plot, juce::Justification::centred);
    }

    if (mouseIn && plot.contains (mouse))
    {
        const float t = (mouse.x - plot.getX()) / plot.getWidth();
        const float f = SpecMath::fMin * std::pow (SpecMath::fMax / SpecMath::fMin, t);
        g.setColour (col::text.withAlpha (0.25f));
        g.drawVerticalLine ((int) mouse.x, plot.getY(), plot.getBottom());

        static const char* names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "H" };
        const int midi = (int) std::lround (69.0 + 12.0 * std::log2 (f / 440.0));
        const juce::String note = juce::String (names[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 2);
        const juce::String fs = f < 1000.0f ? juce::String (juce::roundToInt (f)) + " Hz"
                                            : juce::String (f / 1000.0f, 2).replaceCharacter ('.', ',') + " kHz";

        juce::String txt = fs + "  " + note;
        if (showMix) txt += "   Mix " + fmt (mixA.valueAt (f)) + " dB";
        if (showRef && refLoaded) txt += "   Ref " + fmt (refA.valueAt (f)) + " dB";
        if (drawDiff && ! diff.empty())
        {
            const float pos = t * (float) (diff.size() - 1);
            txt += u8s ("   Δ ") + fmt (diff[(size_t) juce::jlimit (0, (int) diff.size() - 1, (int) pos)], 1, true) + " dB";
        }

        g.setFont (font (12.0f, true));
        const float tw = (float) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), txt) + 16.0f;
        float tx = mouse.x + 10.0f;
        if (tx + tw > plot.getRight()) tx = mouse.x - tw - 10.0f;
        auto box = juce::Rectangle<float> (tx, plot.getY() + 24.0f, tw, 22.0f);
        g.setColour (col::panel2.withAlpha (0.92f));
        g.fillRoundedRectangle (box, 4.0f);
        g.setColour (col::line);
        g.drawRoundedRectangle (box, 4.0f, 1.0f);
        g.setColour (col::text);
        g.drawText (txt, box, juce::Justification::centred);
    }
}

void SpectrumView::mouseMove (const juce::MouseEvent& e) { mouse = e.position; mouseIn = true; }
void SpectrumView::mouseExit (const juce::MouseEvent&) { mouseIn = false; }

//==============================================================================
// LAUTHEIT
void LoudnessPanel::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    const float labW = 130.0f;
    const float colW = (w - labW - 14.0f) * 0.5f;
    const float cMixX = labW, cRefX = labW + colW + 6.0f;

    g.setFont (font (12.0f, true));
    g.setColour (col::gold);
    g.drawText ("MIX", juce::Rectangle<float> (cMixX, 0, colW, 16), juce::Justification::centredRight);
    g.setColour (col::ref);
    g.drawText ("REFERENZ", juce::Rectangle<float> (cRefX, 0, colW, 16), juce::Justification::centredRight);

    const bool hasTarget = r.target < 100.0f;
    const float dMix = r.mixI > -90.0f && hasTarget ? r.mixI - r.target : -1000.0f;
    const float dRef = r.refLoaded && r.refI > -90.0f && hasTarget ? r.refI - r.target : -1000.0f;
    const float plrMix = r.mixI > -90.0f && r.mixTp > -90.0f ? r.mixTp - r.mixI : -1000.0f;
    const float plrRef = r.refLoaded && r.refI > -90.0f && r.refTp > -90.0f ? r.refTp - r.refI : -1000.0f;

    enum Kind { normal, lu, tp, delta, plr };
    struct Row { const char* label; const char* sub; float mix; float ref; float h; float fs; Kind kind; bool proOnly; };

    const float refI = r.refLoaded ? r.refI : -100.0f;
    const Row rows[] {
        { "Integrated",    pro ? "LUFS" : "LUFS · ganzer Song",        r.mixI,    refI,                                      pro ? 44.0f : 50.0f, 30, normal, false },
        { "Short-Term",    pro ? "LUFS · 3 s" : "LUFS · letzte 3 Sekunden", r.mixS, r.refRunning ? r.refS : -100.0f,     pro ? 26.0f : 36.0f, 19, normal, false },
        { "Momentary",     "LUFS · 0,4 s",                              r.mixM,    r.refRunning ? r.refM : -100.0f,         26, 17, normal, true },
        { "Short-Term max","LUFS",                                      r.mixSMax, r.refLoaded ? r.refSMax : -100.0f,       24, 15, normal, true },
        { "Momentary max", "LUFS",                                      r.mixMMax, r.refLoaded ? r.refMMax : -100.0f,       24, 15, normal, true },
        { "LRA",           "LU · Lautheitsumfang",                     r.mixLra,  r.refLoaded ? r.refLra : -100.0f,        24, 15, lu, true },
        { "True Peak max", pro ? "dBTP" : "dBTP · echte Spitze",        r.mixTp,   r.refLoaded ? r.refTp : -100.0f,         pro ? 24.0f : 36.0f, pro ? 15.0f : 19.0f, tp, false },
        { "PLR",           "LU · Peak zu Lautheit",                    plrMix,    plrRef,                                   24, 15, plr, true },
        { "zum Ziel",      pro ? "LU" : "LU · + zu laut, − zu leise",  dMix,      dRef,                                     pro ? 24.0f : 36.0f, pro ? 15.0f : 19.0f, delta, false },
    };

    float y = 20.0f;
    for (auto& row : rows)
    {
        if (row.proOnly && ! pro) continue;

        g.setColour (col::line.withAlpha (0.6f));
        g.drawHorizontalLine ((int) y, 0.0f, w);

        const bool compact = row.h < 30.0f;
        g.setColour (col::text.withAlpha (0.85f));
        g.setFont (font (compact ? 11.5f : 12.5f, true));
        const juce::String lab = row.kind == delta ? u8s ("Δ zum Ziel") : juce::String (row.label);
        if (compact)
        {
            g.drawText (lab, juce::Rectangle<float> (10, y + 1, labW - 10, row.h - 2), juce::Justification::centredLeft);
        }
        else
        {
            g.drawText (lab, juce::Rectangle<float> (10, y + 5, labW, 15), juce::Justification::centredLeft);
            g.setColour (col::dim);
            g.setFont (font (10.5f));
            g.drawText (u8s (row.sub), juce::Rectangle<float> (10, y + 20, labW + 30, 12), juce::Justification::centredLeft);
        }

        auto value = [&] (float v, juce::Rectangle<float> area, juce::Colour base)
        {
            juce::Colour c = base;
            juce::String s;
            if (row.kind == delta || row.kind == plr)
            {
                if (v < -900.0f) { s = u8s ("–"); c = col::dim; }
                else if (row.kind == delta)
                {
                    s = fmt (v, 1, true);
                    c = std::abs (v) <= 1.0f ? col::good : col::warn;
                }
                else
                    s = fmt (v, 1);
            }
            else
            {
                s = row.kind == lu && v < -90.0f ? u8s ("–") : fmt (v);
                if (row.kind == tp && v > -90.0f)
                    c = v > 0.0f ? col::red : (v > r.tpLimit ? col::warn : base);
                if (v <= -90.0f) c = col::dim;
            }
            g.setColour (c);
            g.setFont (font (row.fs, true));
            g.drawText (s, area, juce::Justification::centredRight);
        };

        value (row.mix, juce::Rectangle<float> (cMixX, y + 1, colW, row.h - 2), col::text);
        value (row.ref, juce::Rectangle<float> (cRefX, y + 1, colW, row.h - 2), col::ref.brighter (0.35f));
        y += row.h;
    }

    g.setColour (col::dim);
    g.setFont (font (10.5f));
    g.drawText (u8s ("Referenz: Integrated, Max, LRA, TP = ganze Datei"), juce::Rectangle<float> (10, y + 4, w - 20, 14),
                juce::Justification::centredLeft);

    if (pro)
        return;

    // EINFACH: Einschätzung in klaren Worten
    struct Tip { juce::String text; juce::Colour c; };
    std::vector<Tip> tips;
    if (r.mixI <= -90.0f)
        tips.push_back ({ u8s ("Starte die Wiedergabe – nach ein paar Sekunden steht hier, wie dein Mix dasteht."), col::dim });
    else
    {
        if (hasTarget)
        {
            const float dl = r.mixI - r.target;
            if (std::abs (dl) <= 1.0f)
                tips.push_back ({ u8s ("Lautheit passt zum Ziel."), col::good });
            else if (dl > 0.0f)
                tips.push_back ({ u8s ("Dein Mix ist ") + fmt (dl, 1) + u8s (" LU lauter als das Ziel. Die Plattform dreht ihn leiser – "
                                  "mehr Lautheit bringt dort nichts, kostet aber Dynamik."), col::warn });
            else
                tips.push_back ({ u8s ("Dein Mix ist ") + fmt (-dl, 1) + u8s (" LU leiser als das Ziel. Soll er so laut wirken wie "
                                  "andere Songs, mit Limiter oder Gain etwas anheben."), col::goldHi });
        }
        if (r.mixTp > r.tpLimit && r.mixTp > -90.0f)
            tips.push_back ({ u8s ("True Peak über ") + fmt (r.tpLimit) + u8s (" dBTP: Ceiling am Limiter senken, sonst kann es "
                              "beim Umwandeln in MP3/AAC verzerren."), col::warn });
        if (r.refLoaded && r.refI > -90.0f)
        {
            const float dr = r.refI - r.mixI;
            tips.push_back ({ std::abs (dr) < 1.0f ? u8s ("Dein Mix ist ungefähr so laut wie die Referenz.")
                                                   : u8s ("Die Referenz ist ") + fmt (std::abs (dr), 1) + u8s (" LU ")
                                                     + (dr > 0 ? u8s ("lauter") : u8s ("leiser")) + u8s (" als dein Mix."),
                              col::text.withAlpha (0.85f) });
        }
    }

    float ty = y + 26.0f;
    g.setColour (col::line.withAlpha (0.6f));
    g.drawHorizontalLine ((int) ty - 4, 0.0f, w);
    g.setColour (col::gold);
    g.setFont (font (11.5f, true));
    g.drawText (u8s ("EINSCHÄTZUNG"), juce::Rectangle<float> (10, ty, w - 20, 14), juce::Justification::centredLeft);
    ty += 18.0f;
    for (auto& t : tips)
    {
        g.setColour (t.c);
        g.setFont (font (12.0f));
        const auto rect = juce::Rectangle<float> (10, ty, w - 20, 32);
        g.drawFittedText (t.text, rect.toNearestInt(), juce::Justification::topLeft, 2, 1.0f);
        ty += 34.0f;
        if (ty > (float) getHeight() - 20.0f) break;
    }
}

//==============================================================================
// PEGEL
void MeterPanel::Bar::feed (float pk, float rmsV)
{
    peak = juce::jmax (pk, peak - 1.0f);
    if (pk >= hold) { hold = pk; holdAge = 0; }
    else if (++holdAge > 45) hold = juce::jmax (peak, hold - 1.2f);
    rms = rmsV;
}

void MeterPanel::tick (AnalyzerProcessor& p)
{
    auto db = [] (float v) { return juce::Decibels::gainToDecibels (v, -100.0f); };
    mL.feed (db (p.mixLevels.peakL.exchange (0.0f)), db (p.mixLevels.rmsL.load()));
    mR.feed (db (p.mixLevels.peakR.exchange (0.0f)), db (p.mixLevels.rmsR.load()));
    refRunning = p.isRefRunning();
    rL.feed (db (p.refLevels.peakL.exchange (0.0f)), refRunning ? db (p.refLevels.rmsL.load()) : -100.0f);
    rR.feed (db (p.refLevels.peakR.exchange (0.0f)), refRunning ? db (p.refLevels.rmsR.load()) : -100.0f);
    mixM = p.mixLoud.m.load();
    refM = refRunning ? p.refLoud.m.load() : -100.0f;
    mixCorr = p.mixLevels.corr.load();
    refCorr = refRunning ? p.refLevels.corr.load() : 0.0f;
    mixMax = db (p.mixLevels.maxPeak.load());
    refMax = db (p.refLevels.maxPeak.load());
}

void MeterPanel::paint (juce::Graphics& g)
{
    const float top = 34.0f, bottom = (float) getHeight() - 50.0f, hgt = bottom - top;
    auto yOf = [&] (float db) { return top + (1.0f - (juce::jlimit (-60.0f, 3.0f, db) + 60.0f) / 63.0f) * hgt; };

    auto drawBar = [&] (float x, float w, const Bar& b, juce::Colour c)
    {
        g.setColour (col::plot);
        g.fillRect (juce::Rectangle<float> (x, top, w, hgt));
        if (b.rms > -60.0f)
        {
            const float yr = yOf (b.rms);
            juce::ColourGradient grad (c.darker (0.3f), 0, bottom, c, 0, top, false);
            g.setGradientFill (grad);
            g.fillRect (juce::Rectangle<float> (x, yr, w, bottom - yr));
        }
        if (b.peak > -60.0f)
        {
            const float yp = yOf (b.peak);
            g.setColour (c.withAlpha (0.25f));
            g.fillRect (juce::Rectangle<float> (x, yp, w, bottom - yp));
            g.setColour (b.peak > 0.0f ? col::red : c.brighter (0.5f).withAlpha (0.9f));
            g.fillRect (juce::Rectangle<float> (x, yp, w, juce::jmin (3.0f, bottom - yp)));
        }
        if (b.hold > -60.0f)
        {
            g.setColour (b.hold > 0.0f ? col::red : col::text);
            g.fillRect (juce::Rectangle<float> (x, yOf (b.hold) - 0.5f, w, 1.5f));
        }
    };

    auto drawLufs = [&] (float x, float w, float v, juce::Colour c)
    {
        g.setColour (col::plot);
        g.fillRect (juce::Rectangle<float> (x, top, w, hgt));
        if (v > -60.0f)
        {
            const float y = yOf (v);
            g.setColour (c.withAlpha (0.75f));
            g.fillRect (juce::Rectangle<float> (x, y, w, bottom - y));
            g.setColour (c.brighter (0.6f));
            g.fillRect (juce::Rectangle<float> (x, y, w, 2.0f));
        }
    };

    auto group = [&] (float gx, const char* title, const Bar& l, const Bar& r, float m, float mx, float corr,
                      juce::Colour c, bool active)
    {
        g.setFont (font (12.0f, true));
        g.setColour (c);
        g.drawText (title, juce::Rectangle<float> (gx, 0, 120, 14), juce::Justification::centredLeft);
        g.setFont (font (11.0f));
        g.setColour (mx > 0.0f ? col::red : col::text.withAlpha (active ? 0.9f : 0.4f));
        g.drawText (u8s ("Peak ") + fmt (mx), juce::Rectangle<float> (gx, 15, 70, 14), juce::Justification::centredLeft);

        drawBar (gx, 22.0f, l, c);
        drawBar (gx + 26.0f, 22.0f, r, c);
        drawLufs (gx + 58.0f, 24.0f, m, c);

        g.setFont (font (10.0f, true));
        g.setColour (col::dim);
        g.drawText ("L", juce::Rectangle<float> (gx, bottom + 2, 22, 12), juce::Justification::centred);
        g.drawText ("R", juce::Rectangle<float> (gx + 26.0f, bottom + 2, 22, 12), juce::Justification::centred);
        g.drawText ("LUFS M", juce::Rectangle<float> (gx + 48.0f, bottom + 2, 44, 12), juce::Justification::centred);
        g.setColour (col::text.withAlpha (active ? 0.9f : 0.4f));
        g.drawText (fmt (m), juce::Rectangle<float> (gx + 70.0f, 15, 42, 14), juce::Justification::centredRight);

        const float cy = bottom + 22.0f;
        auto cr = juce::Rectangle<float> (gx, cy, 130.0f, 8.0f);
        g.setColour (col::plot);
        g.fillRoundedRectangle (cr, 3.0f);
        g.setColour (col::line);
        g.drawVerticalLine ((int) cr.getCentreX(), cr.getY() - 2, cr.getBottom() + 2);
        if (active)
        {
            const float cx = cr.getX() + (corr + 1.0f) * 0.5f * cr.getWidth();
            g.setColour (corr < 0.0f ? col::red : (corr < 0.3f ? col::warn : col::good));
            g.fillRoundedRectangle (juce::Rectangle<float> (cx - 3.0f, cy - 2.0f, 6.0f, 12.0f), 2.0f);
        }
        g.setColour (col::dim);
        g.setFont (font (9.5f));
        g.drawText ("-1", juce::Rectangle<float> (gx, cy + 10, 20, 11), juce::Justification::centredLeft);
        g.drawText (u8s ("Korrelation ") + (active ? juce::String (corr, 2).replaceCharacter ('.', ',') : u8s ("–")),
                    juce::Rectangle<float> (gx + 16, cy + 10, 98, 11), juce::Justification::centred);
        g.drawText ("+1", juce::Rectangle<float> (gx + 110, cy + 10, 20, 11), juce::Justification::centredRight);
    };

    group (12.0f, "MIX", mL, mR, mixM, mixMax, mixCorr, col::gold, true);
    group ((float) getWidth() - 142.0f, "REFERENZ", rL, rR, refM, refRunning ? refMax : -100.0f, refCorr, col::ref, refRunning);

    const float sx = (float) getWidth() * 0.5f - 22.0f;
    g.setFont (font (10.0f));
    for (int db : { 0, -6, -12, -18, -24, -30, -36, -48, -60 })
    {
        const float y = yOf ((float) db);
        g.setColour (col::line);
        g.drawHorizontalLine ((int) y, sx, sx + 6.0f);
        g.setColour (col::dim);
        g.drawText (juce::String (db), juce::Rectangle<float> (sx + 8.0f, y - 6.0f, 30.0f, 12.0f), juce::Justification::centredLeft);
    }
}

//==============================================================================
// VERLAUF
void HistoryView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (col::plot);
    g.fillRoundedRectangle (b, 4.0f);
    auto plot = b.withTrimmedLeft (34.0f).withTrimmedRight (8.0f).reduced (0.0f, 6.0f);
    const float topL = 0.0f, botL = -42.0f;
    auto yOf = [&] (float v) { return plot.getY() + (topL - juce::jlimit (botL, topL, v)) / (topL - botL) * plot.getHeight(); };

    g.setFont (font (10.0f));
    for (int l = -6; l >= -42; l -= 12)
    {
        const float y = yOf ((float) l);
        g.setColour (col::grid.brighter (0.15f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (col::dim);
        g.drawText (juce::String (l), juce::Rectangle<float> (0, y - 6, 30, 12), juce::Justification::centredRight);
    }

    if (target < 100.0f)
    {
        const float y = yOf (target);
        juce::Path dash;
        dash.startNewSubPath (plot.getX(), y);
        dash.lineTo (plot.getRight(), y);
        const float pattern[] { 5.0f, 4.0f };
        juce::Path out;
        juce::PathStrokeType (1.0f).createDashedStroke (out, dash, pattern, 2);
        g.setColour (col::good.withAlpha (0.7f));
        g.fillPath (out);
        g.setFont (font (10.0f, true));
        g.drawText (u8s ("Ziel ") + fmt (target, 0), juce::Rectangle<float> (plot.getRight() - 70, y - 13, 66, 12),
                    juce::Justification::centredRight);
    }

    auto drawLine = [&] (const std::vector<float>& v, juce::Colour c, float th)
    {
        juce::Path p;
        bool open = false;
        for (int i = 0; i < len; ++i)
        {
            const float val = v[(size_t) ((pos + i) % len)];
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) (len - 1);
            if (val <= -70.0f) { open = false; continue; }
            if (! open) { p.startNewSubPath (x, yOf (val)); open = true; }
            else          p.lineTo (x, yOf (val));
        }
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (th, juce::PathStrokeType::curved));
    };

    drawLine (ref, col::ref, 1.6f);
    drawLine (mix, col::gold, 1.8f);
}

//==============================================================================
// STEREO / VEKTORSKOP
ScopeView::ScopeView()
{
    mL.assign (len, 0.0f); mR.assign (len, 0.0f);
    rL.assign (len, 0.0f); rR.assign (len, 0.0f);
}

void ScopeView::push (std::vector<float>& a, std::vector<float>& b, int& p, const float* L, const float* R, int n)
{
    const int start = juce::jmax (0, n - len);
    for (int i = start; i < n; ++i)
    {
        a[(size_t) p] = L[i];
        b[(size_t) p] = R[i];
        p = (p + 1) % len;
    }
}

void ScopeView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (col::plot);
    g.fillRoundedRectangle (b, 4.0f);

    const float sz = b.getHeight() - 8.0f;
    auto sq = juce::Rectangle<float> (b.getX() + 6.0f, b.getY() + 4.0f, sz, sz);
    const float cx = sq.getCentreX(), cy = sq.getCentreY(), rad = sz * 0.5f;

    g.setColour (col::grid.brighter (0.2f));
    g.drawLine (cx, sq.getY(), cx, sq.getBottom(), 1.0f);
    g.drawLine (sq.getX(), cy, sq.getRight(), cy, 1.0f);
    g.setColour (col::grid);
    g.drawLine (cx - rad * 0.707f, cy - rad * 0.707f, cx + rad * 0.707f, cy + rad * 0.707f, 1.0f);
    g.drawLine (cx - rad * 0.707f, cy + rad * 0.707f, cx + rad * 0.707f, cy - rad * 0.707f, 1.0f);
    g.drawEllipse (sq.reduced (1.0f), 1.0f);
    g.setFont (font (9.5f, true));
    g.setColour (col::dim);
    g.drawText ("M", juce::Rectangle<float> (cx - 8, sq.getY() + 1, 16, 10), juce::Justification::centred);
    g.drawText ("L", juce::Rectangle<float> (sq.getX() + 6, sq.getY() + 6, 12, 10), juce::Justification::centred);
    g.drawText ("R", juce::Rectangle<float> (sq.getRight() - 18, sq.getY() + 6, 12, 10), juce::Justification::centred);

    // Auto-Skalierung
    float peak = 1.0e-4f;
    for (int i = 0; i < len; ++i) peak = juce::jmax (peak, std::abs (mL[(size_t) i]), std::abs (mR[(size_t) i]));
    if (showRef && refRunning)
        for (int i = 0; i < len; ++i) peak = juce::jmax (peak, std::abs (rL[(size_t) i]), std::abs (rR[(size_t) i]));
    const float wantScale = 0.9f / peak;
    scaleSmooth += (wantScale < scaleSmooth ? 0.5f : 0.05f) * (wantScale - scaleSmooth);
    scaleSmooth = juce::jlimit (0.5f, 200.0f, scaleSmooth);

    auto dots = [&] (const std::vector<float>& L, const std::vector<float>& R, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.45f));
        for (int i = 0; i < len; i += 2)
        {
            const float l = L[(size_t) i] * scaleSmooth, r = R[(size_t) i] * scaleSmooth;
            const float x = cx + (r - l) * 0.7071f * rad;
            const float y = cy - (l + r) * 0.7071f * rad;
            if (sq.contains (x, y)) g.fillRect (x, y, 1.5f, 1.5f);
        }
    };
    if (showRef && refRunning) dots (rL, rR, col::ref);
    dots (mL, mR, col::gold);

    // Kennzahlen
    auto stats = [&] (const std::vector<float>& L, const std::vector<float>& R, float& sm)
    {
        double m = 0.0, s = 0.0;
        for (int i = 0; i < len; ++i)
        {
            const double a = 0.5 * (L[(size_t) i] + R[(size_t) i]), d = 0.5 * (L[(size_t) i] - R[(size_t) i]);
            m += a * a; s += d * d;
        }
        sm = (m > 1.0e-12 && s > 1.0e-14) ? (float) (10.0 * std::log10 (s / m)) : -100.0f;
        return m > 1.0e-10;
    };

    float smMix = -100.0f, smRef = -100.0f;
    const bool mixOn = stats (mL, mR, smMix);
    const bool refOn = refRunning && stats (rL, rR, smRef);

    auto tx = juce::Rectangle<float> (sq.getRight() + 12.0f, b.getY() + 6.0f, b.getRight() - sq.getRight() - 18.0f, 16.0f);
    auto row = [&] (const juce::String& label, const juce::String& v1, const juce::String& v2)
    {
        g.setFont (font (11.0f, true));
        g.setColour (col::text.withAlpha (0.85f));
        g.drawText (label, tx, juce::Justification::centredLeft);
        g.setColour (col::gold);
        g.drawText (v1, tx.withTrimmedRight (52.0f), juce::Justification::centredRight);
        g.setColour (col::ref);
        g.drawText (v2, tx, juce::Justification::centredRight);
        tx.translate (0.0f, 20.0f);
    };

    g.setFont (font (10.5f, true));
    g.setColour (col::gold);
    g.drawText ("MIX", tx.withTrimmedRight (52.0f), juce::Justification::centredRight);
    g.setColour (col::ref);
    g.drawText ("REF", tx, juce::Justification::centredRight);
    tx.translate (0.0f, 18.0f);

    row (u8s ("Korrelation"), mixOn ? juce::String (mixCorr, 2).replaceCharacter ('.', ',') : u8s ("–"),
         refOn ? juce::String (refCorr, 2).replaceCharacter ('.', ',') : u8s ("–"));
    row (u8s ("Side/Mid"), mixOn ? fmt (smMix) + " dB" : u8s ("–"), refOn ? fmt (smRef) + " dB" : u8s ("–"));

    g.setFont (font (10.0f));
    g.setColour (col::dim);
    g.drawFittedText (u8s ("Senkrecht = Mono. Breiter = mehr Stereo. Korrelation unter 0 = Mono-Probleme."),
                      tx.withHeight (30.0f).toNearestInt(), juce::Justification::topLeft, 2);
}

//==============================================================================
// TONALE BALANCE
void TonalView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (col::plot);
    g.fillRoundedRectangle (b, 4.0f);

    if (! refLoaded)
    {
        g.setColour (col::text.withAlpha (0.75f));
        g.setFont (font (13.0f, true));
        g.drawFittedText (u8s ("Lade unten eine Referenz – dann zeigt dir dieses Feld, wo dein Mix mehr oder weniger "
                               "Bass, Mitten oder Höhen hat als der Song, an dem du dich orientierst."),
                          b.reduced (20.0f, 10.0f).toNearestInt(), juce::Justification::centred, 4);
        return;
    }

    std::array<float, SpecMath::numBands> d {};
    const bool valid = mixValid && refValid;
    for (int i = 0; i < SpecMath::numBands; ++i) d[(size_t) i] = valid ? mix[(size_t) i] - ref[(size_t) i] : 0.0f;

    // Tipp-Zeile
    juce::String tip;
    juce::Colour tipCol = col::text;
    if (! mixValid)
    {
        tip = u8s ("Spiel deinen Mix ein paar Sekunden ab – dann erscheint der Vergleich.");
        tipCol = col::dim;
    }
    else if (! refValid)
    {
        tip = u8s ("Referenz wird gemessen …");
        tipCol = col::dim;
    }
    else
    {
        int worst = 0;
        for (int i = 1; i < SpecMath::numBands; ++i)
            if (std::abs (d[(size_t) i]) > std::abs (d[(size_t) worst])) worst = i;
        const float v = d[(size_t) worst];
        if (std::abs (v) < 1.5f)
        {
            tip = u8s ("Sehr nah an der Referenz – die Klangbalance passt.");
            tipCol = col::good;
        }
        else
        {
            tip = u8s ("Dein Mix hat ") + fmt (std::abs (v), 1) + u8s (" dB ") + (v > 0 ? u8s ("mehr ") : u8s ("weniger "))
                + u8s (kBandPhrase[worst]) + u8s (" als die Referenz.");
            tipCol = std::abs (v) < 3.0f ? col::goldHi : col::warn;
        }
    }

    g.setFont (font (12.5f, true));
    g.setColour (tipCol);
    g.drawText (tip, b.reduced (10.0f, 4.0f).withHeight (18.0f), juce::Justification::centredLeft);
    g.setFont (font (10.0f));
    g.setColour (col::dim);
    juce::String meas = u8s ("Mix ") + mmss (mixSeconds) + u8s (" · Ref: ganzer Song");
    if (channelNote.isNotEmpty()) meas = channelNote;
    g.drawText (meas, b.reduced (10.0f, 4.0f).withHeight (18.0f), juce::Justification::centredRight);

    // Balken
    auto area = b.reduced (10.0f, 0.0f).withTrimmedTop (26.0f).withTrimmedBottom (32.0f);
    const float colW = area.getWidth() / (float) SpecMath::numBands;
    const float mid = area.getCentreY(), span = area.getHeight() * 0.5f;
    float maxAbs = 0.0f;
    if (valid) for (auto v : d) maxAbs = juce::jmax (maxAbs, std::abs (v));
    const float range = maxAbs <= 5.0f ? 6.0f : (maxAbs <= 10.0f ? 12.0f : 18.0f);
    const float bodySpan = span - 14.0f;   // Platz für die Zahl über/unter dem Balken
    auto yOf = [&] (float v) { return mid - juce::jlimit (-range, range, v) / range * bodySpan; };

    for (float l : { -range, -range * 0.5f, 0.0f, range * 0.5f, range })
    {
        g.setColour (std::abs (l) < 0.01f ? col::line.brighter (0.4f) : col::grid);
        g.drawHorizontalLine ((int) yOf (l), area.getX(), area.getRight());
    }
    g.setFont (font (9.5f));
    g.setColour (col::dim);
    g.drawText ("+" + juce::String ((int) range), juce::Rectangle<float> (area.getX(), yOf (range) - 11.0f, 24, 10), juce::Justification::bottomLeft);
    g.drawText ("-" + juce::String ((int) range), juce::Rectangle<float> (area.getX(), yOf (-range) + 1.0f, 24, 10), juce::Justification::topLeft);

    // ±1,5 dB „passt“-Zone
    g.setColour (col::good.withAlpha (0.07f));
    g.fillRect (juce::Rectangle<float> (area.getX(), yOf (1.5f), area.getWidth(), yOf (-1.5f) - yOf (1.5f)));

    for (int i = 0; i < SpecMath::numBands; ++i)
    {
        const float x = area.getX() + colW * (float) i;
        const float bw = juce::jmin (46.0f, colW * 0.42f);
        const float bx = x + (colW - bw) * 0.5f;

        if (valid)
        {
            const float v = d[(size_t) i];
            const juce::Colour c = std::abs (v) < 1.5f ? col::good : (std::abs (v) < 3.0f ? col::goldHi : col::warn);
            const float y0 = yOf (0.0f), y1 = yOf (v);
            g.setColour (c.withAlpha (0.85f));
            g.fillRoundedRectangle (juce::Rectangle<float> (bx, juce::jmin (y0, y1), bw, juce::jmax (2.0f, std::abs (y1 - y0))), 2.0f);
            g.setFont (font (11.5f, true));
            g.setColour (c);
            const float ty = v >= 0.0f ? y1 - 14.0f : y1 + 1.0f;
            g.drawText (fmt (v, 1, true), juce::Rectangle<float> (x, ty, colW, 13), juce::Justification::centred);
        }

        g.setFont (font (11.0f, true));
        g.setColour (col::text.withAlpha (0.85f));
        g.drawText (u8s (SpecMath::bandName (i)), juce::Rectangle<float> (x, area.getBottom() + 4, colW, 13), juce::Justification::centred);
        g.setFont (font (9.5f));
        g.setColour (col::dim);
        g.drawText (u8s (SpecMath::bandRange (i)), juce::Rectangle<float> (x, area.getBottom() + 17, colW, 12), juce::Justification::centred);
    }
}

//==============================================================================
// WELLENFORM REFERENZ
void WaveView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (col::plot);
    g.fillRoundedRectangle (b, 4.0f);

    auto* ref = proc.getRef();
    const int status = proc.getRefStatus();
    const bool loadingHere = status == AnalyzerProcessor::refLoading && proc.getLoadingSlot() == proc.currentSlot();

    if (ref == nullptr)
    {
        juce::Path border;
        border.addRoundedRectangle (b.reduced (1.5f), 4.0f);
        juce::Path dashed;
        const float pattern[] { 6.0f, 5.0f };
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, border, pattern, 2);
        g.setColour (dragHover ? col::gold : col::line.brighter (0.3f));
        g.fillPath (dashed);

        g.setFont (font (14.0f, true));
        g.setColour (dragHover ? col::goldHi : col::text.withAlpha (0.75f));
        juce::String msg = u8s ("Slot ") + juce::String (proc.currentSlot() + 1)
                         + u8s (": Referenz-Track hierher ziehen oder LADEN klicken");
        if (loadingHere) msg = proc.getRefMessage();
        g.drawText (msg, b.withTrimmedBottom (18), juce::Justification::centred);
        g.setFont (font (11.0f));
        const bool err = status == AnalyzerProcessor::refError;
        g.setColour (err ? col::red : col::dim);
        g.drawText (err ? proc.getRefMessage() : u8s ("WAV · AIFF · FLAC · MP3 · OGG (Mac auch M4A) · bis zu 4 Dateien auf einmal"),
                    b.withTrimmedTop (34), juce::Justification::centred);
        return;
    }

    auto wr = b.reduced (6.0f, 4.0f).withTrimmedTop (18.0f);
    const float mid = wr.getCentreY();
    const int n = (int) ref->wave.size();
    const double posSec = proc.getRefPositionSeconds();
    const float playX = wr.getX() + wr.getWidth() * (float) (ref->seconds > 0 ? posSec / ref->seconds : 0.0);

    for (int x = 0; x < (int) wr.getWidth(); ++x)
    {
        const int i0 = (int) ((float) x / wr.getWidth() * (float) n);
        const int i1 = juce::jmax (i0 + 1, (int) ((float) (x + 1) / wr.getWidth() * (float) n));
        float m = 0.0f;
        for (int i = i0; i < i1 && i < n; ++i) m = juce::jmax (m, ref->wave[(size_t) i]);
        const float h = juce::jmin (1.0f, m) * wr.getHeight() * 0.5f;
        const float px = wr.getX() + (float) x;
        g.setColour (px < playX ? col::ref.withAlpha (0.85f) : col::ref.withAlpha (0.38f));
        g.fillRect (px, mid - h, 1.0f, juce::jmax (1.0f, 2.0f * h));
    }

    g.setColour (col::goldHi);
    g.fillRect (playX - 1.0f, wr.getY() - 2.0f, 2.0f, wr.getHeight() + 4.0f);

    g.setFont (font (12.5f, true));
    g.setColour (col::text);
    auto head = b.reduced (8.0f, 2.0f).withHeight (18.0f);
    g.drawText (juce::String (proc.currentSlot() + 1) + u8s (" · ") + ref->name, head.withWidth (head.getWidth() * 0.5f),
                juce::Justification::centredLeft, true);
    g.setFont (font (11.5f));
    g.setColour (col::dim);
    const juce::String info = mmss (posSec) + " / " + mmss (ref->seconds) + u8s ("   ·   ") + fmt (ref->integrated)
                            + " LUFS   " + fmt (ref->truePeak) + " dBTP   LRA " + fmt (ref->lra);
    g.drawText (loadingHere ? proc.getRefMessage() : info, head, juce::Justification::centredRight);

    if (dragHover)
    {
        g.setColour (col::gold);
        g.drawRoundedRectangle (b.reduced (1.0f), 4.0f, 2.0f);
    }
}

void WaveView::seek (const juce::MouseEvent& e)
{
    auto* ref = proc.getRef();
    if (ref == nullptr) return;
    auto wr = getLocalBounds().toFloat().reduced (6.0f, 4.0f);
    const double t = juce::jlimit (0.0, 1.0, (double) ((e.position.x - wr.getX()) / wr.getWidth()));
    proc.seekReference (t * ref->seconds);
}

//==============================================================================
void HelpOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.88f));

    juce::AttributedString as;
    as.setLineSpacing (2.5f);
    auto add = [&] (const char* t, float h, juce::Colour c, bool bold = false) { as.append (u8s (t), font (h, bold), c); };

    add ("So funktioniert der K-54 Analyzer\n\n", 22, col::gold, true);
    add ("EINFACH / PRO  ", 13.5f, col::goldHi, true);
    add ("EINFACH zeigt nur das Wichtigste mit Tipps in klaren Worten. PRO blendet alle Werkzeuge für Mix- und "
         "Mastering-Engineers ein. Der Analyzer merkt sich deine Wahl.\n\n", 13.5f, col::text);
    add ("Einsetzen  ", 13.5f, col::goldHi, true);
    add ("Als letztes Plugin auf den Master (Stereo-Ausgang). Gemessen wird immer dein Signal am Eingang.\n\n", 13.5f, col::text);
    add ("Spektrum 20 Hz – 20 kHz  ", 13.5f, col::goldHi, true);
    add ("Gold = Mix, Blau = Referenz, jede Kurve einzeln ein/aus. Maus darüber = Frequenz, Ton, Pegel. "
         "PRO: DIFFERENZ zeigt Mix minus Referenz (lila, eigene Skala rechts). TEMPO \"Langzeit\" mittelt seit dem letzten Reset – "
         "die Referenz zeigt dann das Mittel über den ganzen Song. KANAL: L+R, Links, Rechts oder Side. "
         "AUFLÖSUNG: mehr FFT-Punkte für feine Bässe. NEIGUNG 4,5 dB/Okt.: ausgewogene Mixe liegen ungefähr waagerecht.\n\n", 13.5f, col::text);
    add ("Tonale Balance  ", 13.5f, col::goldHi, true);
    add ("Vergleicht 6 Frequenzbereiche deines Mixes mit der Referenz, unabhängig von der Lautstärke. "
         "Grün = passt (±1,5 dB), Gelb = etwas, Orange = deutlich. + heißt: dein Mix hat dort mehr.\n\n", 13.5f, col::text);
    add ("Lautheit (EBU R128 / ITU BS.1770)  ", 13.5f, col::goldHi, true);
    add ("Integrated = ganzer Song, Short-Term = 3 s, Momentary = 0,4 s, LRA = Lautheitsumfang, True Peak = echte Spitzen, "
         "PLR = True Peak minus Integrated (wie viel Dynamik übrig ist). Beim DAW-Start wird neu gemessen (AUTO-RESET).\n\n", 13.5f, col::text);
    add ("Referenz & A/B  ", 13.5f, col::goldHi, true);
    add ("4 Slots (1–4): LADEN oder Dateien ins Fenster ziehen. Oben A (Mix) / B (Referenz). LAUTHEIT ANGLEICHEN macht die "
         "Referenz so laut wie deinen Mix. DAW-SYNC an: Referenz läuft mit der DAW, Klick in die Wellenform setzt die Stelle. "
         "DAW-SYNC aus: PLAY/STOP, Schleife.\n\n", 13.5f, col::text);
    add ("Abhören (PRO)  ", 13.5f, col::goldHi, true);
    add ("Mono, Side oder ein Frequenzbereich solo – für Mix und Referenz. Ist etwas davon aktiv, leuchtet es orange: "
         "vor dem Bouncen wieder auf Stereo / Voller Bereich stellen!\n\n", 13.5f, col::text);
    add ("Klick irgendwo, um die Hilfe zu schließen.", 12, col::dim);

    juce::TextLayout tl;
    tl.createLayout (as, 880.0f);
    auto box = getLocalBounds().toFloat().withSizeKeepingCentre (940.0f, tl.getHeight() + 52.0f);
    g.setColour (col::panel);
    g.fillRoundedRectangle (box, 8.0f);
    g.setColour (col::gold);
    g.drawRoundedRectangle (box, 8.0f, 1.5f);
    tl.draw (g, box.reduced (30.0f, 26.0f));
}

//==============================================================================
Content::Content (AnalyzerProcessor& p) : proc (p)
{
    auto& vts = proc.apvts;

    auto setupToggle = [this] (juce::TextButton& b, const juce::String& text, juce::Colour accent)
    {
        b.setButtonText (text);
        b.setClickingTogglesState (true);
        setAccent (b, accent);
        addAndMakeVisible (b);
    };

    auto setupLabel = [this] (juce::Label& lb, const char* text, juce::Justification j = juce::Justification::centredRight)
    {
        lb.setText (u8s (text), juce::dontSendNotification);
        lb.setFont (font (10.5f, true));
        lb.setJustificationType (j);
        addAndMakeVisible (lb);
    };

    auto setupCombo = [this, &vts] (juce::ComboBox& cb, const char* id, std::unique_ptr<CA>& att)
    {
        cb.addItemList (vts.getParameter (id)->getAllValueStrings(), 1);
        addAndMakeVisible (cb);
        att = std::make_unique<CA> (vts, id, cb);
    };

    // Modus
    btnSimple.setButtonText ("EINFACH");
    btnPro.setButtonText ("PRO");
    btnSimple.onClick = [this] { setPro (false); };
    btnPro.onClick = [this] { setPro (true); };
    btnSimple.setTooltip (u8s ("Nur das Wichtigste, mit Tipps"));
    btnPro.setTooltip (u8s ("Alle Werkzeuge für Mix- und Mastering-Engineers"));
    addAndMakeVisible (btnSimple);
    addAndMakeVisible (btnPro);

    // A/B
    btnA.setButtonText (u8s ("A  ·  MIX"));
    btnB.setButtonText (u8s ("B  ·  REFERENZ"));
    for (auto* b : { &btnA, &btnB }) { b->getProperties().set ("big", true); addAndMakeVisible (*b); }
    setAccent (btnA, col::gold);
    setAccent (btnB, col::ref);
    btnA.onClick = [this] { setAB (false); };
    btnB.onClick = [this] { setAB (true); };

    // Kopf rechts
    setupLabel (lbTarget, "ZIEL");
    setupCombo (cbTarget, "target", aTarget);
    btnReport.setButtonText ("BERICHT");
    btnReport.setTooltip (u8s ("Messbericht (Lautheit, True Peak, Tonale Balance) als Textdatei speichern"));
    btnReport.onClick = [this] { exportReport(); };
    addAndMakeVisible (btnReport);
    btnReset.setButtonText ("RESET");
    btnReset.setTooltip (u8s ("Messung neu starten"));
    btnReset.onClick = [this] { proc.resetMeasurements(); };
    addAndMakeVisible (btnReset);
    btnHelp.setButtonText ("?");
    btnHelp.onClick = [this] { showHelp (true); };
    addAndMakeVisible (btnHelp);

    // Spektrum-Leiste
    setupToggle (btnShowMix, "MIX", col::gold);
    setupToggle (btnShowRef, "REFERENZ", col::ref);
    setupToggle (btnShowDiff, "DIFFERENZ", col::diff);
    setupToggle (btnHold, "HOLD", col::goldHi);
    aShowMix  = std::make_unique<BA> (vts, "showMix", btnShowMix);
    aShowRef  = std::make_unique<BA> (vts, "showRef", btnShowRef);
    aShowDiff = std::make_unique<BA> (vts, "showDiff", btnShowDiff);
    aHold     = std::make_unique<BA> (vts, "hold", btnHold);
    btnShowDiff.setTooltip (u8s ("Mix minus Referenz als lila Kurve"));

    setupLabel (lbSlope, "NEIGUNG");     setupCombo (cbSlope, "slope", aSlope);
    setupLabel (lbSpeed, "TEMPO");       setupCombo (cbSpeed, "speed", aSpeed);
    setupLabel (lbSmooth, "GLÄTTUNG");   setupCombo (cbSmooth, "smooth", aSmooth);
    setupLabel (lbRes, "AUFLÖSUNG");     setupCombo (cbRes, "fftRes", aRes);
    setupLabel (lbChannel, "KANAL");     setupCombo (cbChannel, "channel", aChannel);

    setupToggle (btnAuto, "AUTO-RESET", col::gold);
    aAuto = std::make_unique<BA> (vts, "autoReset", btnAuto);
    btnAuto.setTooltip (u8s ("Integrated/LRA/True Peak bei jedem DAW-Start neu messen"));

    // Stereo-Bereich
    btnTabHist.setButtonText ("VERLAUF");
    btnTabScope.setButtonText ("STEREO");
    btnTabHist.onClick = [this] { setScopeTab (false); };
    btnTabScope.onClick = [this] { setScopeTab (true); };
    addAndMakeVisible (btnTabHist);
    addAndMakeVisible (btnTabScope);

    // Referenz-Leiste
    for (int s = 0; s < AnalyzerProcessor::numSlots; ++s)
    {
        auto& b = btnSlot[(size_t) s];
        b.setButtonText (juce::String (s + 1));
        setAccent (b, col::ref);
        b.setTooltip (u8s ("Referenz-Slot ") + juce::String (s + 1));
        b.onClick = [this, s] { proc.setSlot (s); };
        addAndMakeVisible (b);
    }
    btnLoad.setButtonText (u8s ("LADEN …"));
    btnLoad.onClick = [this] { openFileChooser(); };
    addAndMakeVisible (btnLoad);
    btnClear.setButtonText ("ENTFERNEN");
    btnClear.onClick = [this]
    {
        proc.clearReference();
        if (proc.apvts.getRawParameterValue ("ab")->load() > 0.5f) setAB (false);
    };
    addAndMakeVisible (btnClear);

    setupToggle (btnSync, "DAW-SYNC", col::gold);
    aSync = std::make_unique<BA> (vts, "sync", btnSync);
    btnPlay.setButtonText ("PLAY");
    btnPlay.onClick = [this] { proc.setFreePlaying (! proc.isFreePlaying()); };
    addAndMakeVisible (btnPlay);

    setupToggle (btnMatch, "LAUTHEIT ANGLEICHEN", col::ref);
    aMatch = std::make_unique<BA> (vts, "match", btnMatch);

    setupLabel (lbGain, "GAIN", juce::Justification::centredLeft);
    sGain.setSliderStyle (juce::Slider::LinearHorizontal);
    sGain.setTextBoxStyle (juce::Slider::TextBoxRight, false, 66, 22);
    sGain.setDoubleClickReturnValue (true, 0.0);
    sGain.setColour (juce::Slider::textBoxTextColourId, col::text);
    sGain.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    sGain.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (sGain);
    aGain = std::make_unique<SA> (vts, "refGain", sGain);
    sGain.textFromValueFunction = [] (double v) { return fmt ((float) v, 1, true) + " dB"; };
    sGain.updateText();

    setupLabel (lbMonitor, "ABHÖREN", juce::Justification::centredLeft);
    setupCombo (cbMonitor, "monitor", aMonitor);
    setupCombo (cbBand, "band", aBand);
    cbMonitor.setTooltip (u8s ("Wirkt auf den Ausgang (Mix und Referenz). Vor dem Bouncen auf Stereo stellen!"));
    cbBand.setTooltip (u8s ("Nur einen Frequenzbereich hören. Vor dem Bouncen auf Voller Bereich stellen!"));

    addAndMakeVisible (spectrum);
    addAndMakeVisible (tonal);
    addAndMakeVisible (loud);
    addAndMakeVisible (meters);
    addChildComponent (history);
    addChildComponent (scope);
    addAndMakeVisible (wave);
    addChildComponent (help);

    pullL.resize (1 << 17);
    pullR.resize (1 << 17);
    mono.resize (1 << 17);
    pro = proc.getProMode();
    setSize (W, H);
    updateVisibility();
}

Content::~Content() = default;

void Content::setParam (const char* id, float value)
{
    if (auto* prm = proc.apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
        prm->endChangeGesture();
    }
}

void Content::setPro (bool p)
{
    pro = p;
    proc.setProMode (p);
    if (! p)
    {
        // EINFACH: alles, was man nicht sieht, auf neutral stellen
        setParam ("monitor", 0.0f);
        setParam ("band", 0.0f);
        setParam ("channel", 0.0f);
        setParam ("showDiff", 0.0f);
        setParam ("hold", 0.0f);
    }
    updateVisibility();
    resized();
    repaint();
}

void Content::setScopeTab (bool s)
{
    scopeTab = s;
    updateVisibility();
}

void Content::updateVisibility()
{
    btnSimple.setToggleState (! pro, juce::dontSendNotification);
    btnPro.setToggleState (pro, juce::dontSendNotification);

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &btnShowDiff, &btnHold, &lbSlope, &cbSlope, &lbSpeed, &cbSpeed, &lbSmooth, &cbSmooth,
             &lbRes, &cbRes, &lbChannel, &cbChannel, &btnReport, &btnTabHist, &btnTabScope,
             &lbMonitor, &cbMonitor, &cbBand })
        c->setVisible (pro);

    const bool showScope = pro && scopeTab;
    scope.setVisible (showScope);
    history.setVisible (! showScope);
    btnTabHist.setToggleState (! showScope, juce::dontSendNotification);
    btnTabScope.setToggleState (showScope, juce::dontSendNotification);
    loud.pro = pro;
    tonal.pro = pro;
    loud.repaint();
}

void Content::setAB (bool b)
{
    if (b && proc.getRef() == nullptr)
        b = false;
    setParam ("ab", b ? 1.0f : 0.0f);
    if (b && proc.apvts.getRawParameterValue ("sync")->load() < 0.5f && ! proc.isFreePlaying())
        proc.setFreePlaying (true);
}

void Content::openFileChooser()
{
    chooser = std::make_unique<juce::FileChooser> (u8s ("Referenz-Track laden (Slot ") + juce::String (proc.currentSlot() + 1) + ")",
                                                   juce::File(),
                                                   "*.wav;*.wave;*.aif;*.aiff;*.flac;*.mp3;*.ogg;*.m4a;*.aac;*.caf");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.existsAsFile())
                                  proc.loadReference (f);
                          });
}

juce::String Content::buildReport() const
{
    auto line = [] (const juce::String& label, const juce::String& v) { return label.paddedRight (' ', 22) + v + "\n"; };
    const auto& r = loud.r;
    juce::String s;
    s << u8s ("K-54 Analyzer – Messbericht\n");
    s << u8s ("Keller54 · by Frank E.\n");
    s << juce::Time::getCurrentTime().formatted ("%d.%m.%Y %H:%M") << "\n";
    s << line (u8s ("Samplerate"), juce::String ((int) proc.getSampleRate()) + " Hz");
    s << line (u8s ("Ziel"), r.target < 100.0f ? fmt (r.target, 0) + " LUFS / " + fmt (r.tpLimit) + " dBTP" : u8s ("kein Ziel"));
    s << "\n" << u8s ("MIX\n");
    s << line ("Integrated", fmt (r.mixI) + " LUFS");
    s << line ("LRA", fmt (r.mixLra) + " LU");
    s << line ("True Peak max", fmt (r.mixTp) + " dBTP");
    s << line ("Short-Term max", fmt (r.mixSMax) + " LUFS");
    s << line ("Momentary max", fmt (r.mixMMax) + " LUFS");
    s << line ("PLR", (r.mixI > -90.0f && r.mixTp > -90.0f ? fmt (r.mixTp - r.mixI) : u8s ("–")) + " LU");
    if (r.target < 100.0f && r.mixI > -90.0f)
        s << line (u8s ("Abstand zum Ziel"), fmt (r.mixI - r.target, 1, true) + " LU");

    if (auto* ref = proc.getRef())
    {
        s << "\n" << u8s ("REFERENZ  (Slot ") << (proc.currentSlot() + 1) << ")\n";
        s << line ("Datei", ref->name);
        s << line (u8s ("Länge"), mmss (ref->seconds));
        s << line ("Integrated", fmt (ref->integrated) + " LUFS");
        s << line ("LRA", fmt (ref->lra) + " LU");
        s << line ("True Peak max", fmt (ref->truePeak) + " dBTP");
        s << line ("Short-Term max", fmt (ref->shortMax) + " LUFS");
        s << line ("PLR", fmt (ref->truePeak - ref->integrated) + " LU");
        s << line (u8s ("Angleich beim Hören"), fmt (proc.getRefGainDb(), 1, true) + " dB");

        if (tonal.mixValid && tonal.refValid)
        {
            s << "\n" << u8s ("TONALE BALANCE  (Mix minus Referenz, + = Mix hat mehr)\n");
            for (int i = 0; i < SpecMath::numBands; ++i)
                s << line (u8s (SpecMath::bandName (i)) + " (" + u8s (SpecMath::bandRange (i)) + ")",
                           fmt (tonal.mix[(size_t) i] - tonal.ref[(size_t) i], 1, true) + " dB");
        }
    }
    s << "\n" << u8s ("Lautheit nach ITU-R BS.1770-4 / EBU R128, LRA nach EBU Tech 3342.\n");
    return s;
}

void Content::exportReport()
{
    const auto text = buildReport();
    const auto def = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                         .getChildFile ("K-54 Analyzer Bericht " + juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H-%M") + ".txt");
    chooser = std::make_unique<juce::FileChooser> (u8s ("Messbericht speichern"), def, "*.txt");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [text] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f != juce::File())
                                  f.withFileExtension ("txt").replaceWithText (text, false, false, "\n");
                          });
}

void Content::resized()
{
    rHeader = { 0, 0, W, 58 };
    rSpec   = { 12, 66, 880, 400 };
    rTonal  = { 12, 474, 520, 172 };
    rHist   = { 540, 474, 352, 172 };
    rLoud   = { 904, 66, 364, 330 };
    rMeter  = { 904, 404, 364, 242 };
    rRef    = { 12, 654, 1256, 134 };

    btnSimple.setBounds (258, 17, 76, 26);
    btnPro.setBounds (338, 17, 52, 26);
    btnA.setBounds (466, 11, 170, 36);
    btnB.setBounds (642, 11, 200, 36);
    lbTarget.setBounds (850, 15, 46, 28);
    cbTarget.setBounds (900, 15, 196, 28);
    btnReport.setBounds (1102, 15, 74, 28);
    btnReset.setBounds (1182, 15, 56, 28);
    btnHelp.setBounds (1242, 15, 26, 28);
    if (! pro) btnReset.setBounds (1102, 15, 136, 28);

    const int sx = rSpec.getX(), sy = rSpec.getY() + 7;
    btnShowMix.setBounds (sx + 104, sy, 60, 24);
    btnShowRef.setBounds (sx + 168, sy, 92, 24);
    btnShowDiff.setBounds (sx + 264, sy, 92, 24);
    btnHold.setBounds (sx + 360, sy, 56, 24);

    const int y2 = rSpec.getY() + 36;
    lbSlope.setBounds (sx + 4, y2, 66, 24);    cbSlope.setBounds (sx + 74, y2, 98, 24);
    lbSpeed.setBounds (sx + 172, y2, 52, 24);  cbSpeed.setBounds (sx + 228, y2, 92, 24);
    lbSmooth.setBounds (sx + 320, y2, 72, 24); cbSmooth.setBounds (sx + 396, y2, 96, 24);
    lbRes.setBounds (sx + 492, y2, 80, 24);    cbRes.setBounds (sx + 576, y2, 92, 24);
    lbChannel.setBounds (sx + 668, y2, 52, 24); cbChannel.setBounds (sx + 724, y2, 146, 24);

    const int plotTop = pro ? 66 : 38;
    spectrum.setBounds (rSpec.getX() + 8, rSpec.getY() + plotTop, rSpec.getWidth() - 16, rSpec.getHeight() - plotTop - 8);

    tonal.setBounds (rTonal.getX() + 8, rTonal.getY() + 32, rTonal.getWidth() - 16, rTonal.getHeight() - 40);
    btnTabHist.setBounds (rHist.getRight() - 160, rHist.getY() + 7, 74, 22);
    btnTabScope.setBounds (rHist.getRight() - 82, rHist.getY() + 7, 72, 22);
    history.setBounds (rHist.getX() + 8, rHist.getY() + 32, rHist.getWidth() - 16, rHist.getHeight() - 40);
    scope.setBounds (history.getBounds());

    btnAuto.setBounds (rLoud.getRight() - 100, rLoud.getY() + 7, 90, 22);
    loud.setBounds (rLoud.getX() + 4, rLoud.getY() + 34, rLoud.getWidth() - 8, rLoud.getHeight() - 38);
    meters.setBounds (rMeter.getX() + 4, rMeter.getY() + 30, rMeter.getWidth() - 8, rMeter.getHeight() - 34);

    const int rx = rRef.getX(), ry = rRef.getY();
    for (int s = 0; s < AnalyzerProcessor::numSlots; ++s)
        btnSlot[(size_t) s].setBounds (rx + 12 + s * 34, ry + 32, 30, 28);
    btnLoad.setBounds (rx + 12, ry + 66, 132, 28);
    btnClear.setBounds (rx + 12, ry + 100, 132, 24);
    wave.setBounds (rx + 156, ry + 10, 724, 114);

    const int x0 = rLoud.getX();
    btnSync.setBounds (x0, ry + 8, 150, 26);
    btnPlay.setBounds (x0 + 158, ry + 8, 92, 26);
    lbGain.setBounds (x0, ry + 38, 40, 26);
    sGain.setBounds (x0 + 40, ry + 38, 324, 26);
    btnMatch.setBounds (x0, ry + 68, 190, 26);
    lbMonitor.setBounds (x0, ry + 100, 64, 24);
    cbMonitor.setBounds (x0 + 66, ry + 100, 96, 24);
    cbBand.setBounds (x0 + 168, ry + 100, 186, 24);

    help.setBounds (getLocalBounds());
}

void Content::panel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title) const
{
    g.setColour (col::panel);
    g.fillRoundedRectangle (r.toFloat(), 6.0f);
    g.setColour (col::line);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 6.0f, 1.0f);
    if (title.isNotEmpty())
    {
        g.setColour (col::gold);
        g.setFont (font (12.0f, true));
        g.drawText (title, r.getX() + 12, r.getY() + 7, r.getWidth() - 24, 22, juce::Justification::centredLeft);
    }
}

void Content::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);

    juce::ColourGradient hg (juce::Colour (0xff17140c), 0, 0, col::bg, 0, (float) rHeader.getBottom(), false);
    g.setGradientFill (hg);
    g.fillRect (rHeader);
    g.setColour (col::gold.withAlpha (0.35f));
    g.drawHorizontalLine (rHeader.getBottom() - 1, 0.0f, (float) W);

    g.setColour (col::gold);
    g.setFont (font (32.0f, true));
    g.drawText ("K-54", 18, 8, 90, 40, juce::Justification::centredLeft);
    g.setColour (col::goldHi);
    g.setFont (font (17.0f, true));
    g.drawText ("ANALYZER", 108, 9, 140, 22, juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (font (11.0f));
    g.drawText (u8s ("Keller54 · by Frank E."), 108, 30, 150, 16, juce::Justification::centredLeft);

    if (proc.isMonitoringActive())
    {
        juce::StringArray parts;
        const int mon = (int) proc.apvts.getRawParameterValue ("monitor")->load();
        const int band = (int) proc.apvts.getRawParameterValue ("band")->load();
        if (mon == 1) parts.add ("MONO");
        if (mon == 2) parts.add ("SIDE");
        if (band > 0) parts.add (proc.apvts.getParameter ("band")->getCurrentValueAsText().toUpperCase());
        g.setColour (col::warn);
        g.setFont (font (10.5f, true));
        g.drawText (u8s ("ABHÖREN: ") + parts.joinIntoString (u8s (" · ")) + u8s (" – vor dem Bouncen zurückstellen"),
                    466, 47, 376, 11, juce::Justification::centred);
    }

    panel (g, rSpec, u8s ("SPEKTRUM"));
    g.setColour (col::dim);
    g.setFont (font (10.0f));
    if (! pro)
        g.drawText (u8s ("20 Hz – 20 kHz"), rSpec.getX() + 12, rSpec.getY() + 24, 90, 12, juce::Justification::centredLeft);
    else
        g.drawText (u8s ("20 Hz – 20 kHz"), rSpec.getX() + 430, rSpec.getY() + 13, 120, 12, juce::Justification::centredLeft);

    panel (g, rTonal, u8s ("TONALE BALANCE  ·  Mix gegenüber Referenz"));
    panel (g, rHist, pro && scopeTab ? u8s ("STEREO") : u8s ("VERLAUF  ·  Short-Term, 60 s"));
    panel (g, rLoud, u8s ("LAUTHEIT"));
    panel (g, rMeter, u8s ("PEGEL"));
    panel (g, rRef, {});

    g.setColour (col::ref);
    g.setFont (font (12.0f, true));
    g.drawText ("REFERENZ", rRef.getX() + 12, rRef.getY() + 8, 132, 20, juce::Justification::centredLeft);

    const bool match = proc.apvts.getRawParameterValue ("match")->load() > 0.5f;
    g.setFont (font (11.5f));
    g.setColour (match && proc.getRef() != nullptr ? col::text : col::dim);
    const juce::String m = proc.getRef() == nullptr ? u8s ("–")
                         : (match ? u8s ("Angleich ") + fmt (proc.getMatchGainDb(), 1, true) + " dB" : u8s ("aus"));
    g.drawText (m, rLoud.getX() + 198, rRef.getY() + 68, 166, 26, juce::Justification::centredLeft);
}

void Content::tick()
{
    auto& vts = proc.apvts;
    double s = proc.getSampleRate();
    if (s <= 0.0) s = 48000.0;

    const int res = juce::jlimit (0, 2, (int) vts.getRawParameterValue ("fftRes")->load());
    const int channel = juce::jlimit (0, 3, (int) vts.getRawParameterValue ("channel")->load());
    const int slot = proc.currentSlot();

    if (std::abs (mixA.getSampleRate() - s) > 0.5 || mixA.display.empty() || res != lastRes || channel != lastChannel)
    {
        mixA.prepare (s, res);
        refA.prepare (s, res);
        lastRes = res;
        lastChannel = channel;
    }

    if (lastReset != proc.getResetCount())
    {
        lastReset = proc.getResetCount();
        mixA.resetAverage();
        refA.resetAverage();
        mixA.resetHold();
        refA.resetHold();
        history.clear();
    }

    if (lastRefVersion != proc.getRefVersion() || lastSlot != slot)
    {
        lastRefVersion = proc.getRefVersion();
        if (lastSlot != slot) scope.clearRef();
        lastSlot = slot;
        refA.prepare (s, res);
    }

    auto feed = [&] (StereoFifo& fifo, SpectrumAnalyser& a, bool isMix)
    {
        for (int got; (got = fifo.pull (pullL.data(), pullR.data(), (int) pullL.size())) > 0;)
        {
            for (int i = 0; i < got; ++i)
            {
                const float l = pullL[(size_t) i], r = pullR[(size_t) i];
                mono[(size_t) i] = channel == 1 ? l : channel == 2 ? r : channel == 3 ? 0.5f * (l - r) : 0.5f * (l + r);
            }
            a.push (mono.data(), got);
            if (isMix) scope.pushMix (pullL.data(), pullR.data(), got);
            else       scope.pushRef (pullL.data(), pullR.data(), got);
        }
    };
    feed (proc.mixFifo, mixA, true);
    feed (proc.refFifo, refA, false);

    static const float slopes[] { 0.0f, 3.0f, 4.5f, 6.0f };
    static const float smooths[] { 0.0f, 1.0f / 12.0f, 1.0f / 6.0f, 1.0f / 3.0f };

    const int slopeIdx  = juce::jlimit (0, 3, (int) vts.getRawParameterValue ("slope")->load());
    const int smoothIdx = juce::jlimit (0, 3, (int) vts.getRawParameterValue ("smooth")->load());
    const int speedIdx  = juce::jlimit (0, 3, (int) vts.getRawParameterValue ("speed")->load());
    const int targetIdx = juce::jlimit (0, 5, (int) vts.getRawParameterValue ("target")->load());
    const bool hold = vts.getRawParameterValue ("hold")->load() > 0.5f;

    auto* ref = proc.getRef();
    const bool useStatic = ref != nullptr && channel == 0 && speedIdx == 3 && ! ref->longTermDb.empty();

    mixA.update (slopes[slopeIdx], smooths[smoothIdx], speedIdx, hold);
    refA.update (slopes[slopeIdx], smooths[smoothIdx], speedIdx, hold,
                 useStatic ? &ref->longTermDb : nullptr, proc.getRefGainDb());

    static const char* chNames[] { "", "KANAL LINKS", "KANAL RECHTS", "SIDE (L-R)" };
    spectrum.showMix = vts.getRawParameterValue ("showMix")->load() > 0.5f;
    spectrum.showRef = vts.getRawParameterValue ("showRef")->load() > 0.5f;
    spectrum.showDiff = pro && vts.getRawParameterValue ("showDiff")->load() > 0.5f;
    spectrum.holdOn = hold;
    spectrum.refLoaded = ref != nullptr;
    spectrum.longTerm = speedIdx == 3;
    spectrum.channelName = u8s (chNames[channel]);

    // Tonale Balance
    tonal.refLoaded = ref != nullptr;
    tonal.mixValid = mixA.averageSeconds() >= 2.0 && mixA.bands (tonal.mix);
    tonal.mixSeconds = mixA.averageSeconds();
    if (ref != nullptr && channel == 0 && ref->hasBands)
    {
        tonal.ref = ref->bands;
        tonal.refValid = true;
        tonal.channelNote = {};
    }
    else
    {
        tonal.refValid = refA.averageSeconds() >= 2.0 && refA.bands (tonal.ref);
        tonal.channelNote = channel != 0 ? u8s ("Kanal: ") + u8s (chNames[channel]).toLowerCase() : juce::String();
    }

    // Lautheit
    auto& r = loud.r;
    r.mixM = proc.mixLoud.m.load(); r.mixS = proc.mixLoud.s.load(); r.mixI = proc.mixLoud.i.load();
    r.mixLra = proc.mixLoud.lra.load(); r.mixTp = proc.mixLoud.tp.load();
    r.mixSMax = proc.mixLoud.sMax.load(); r.mixMMax = proc.mixLoud.mMax.load();
    r.refLoaded = ref != nullptr;
    r.refRunning = proc.isRefRunning();
    r.refM = proc.refLoud.m.load(); r.refS = proc.refLoud.s.load();
    if (ref != nullptr)
    {
        r.refI = ref->integrated; r.refLra = ref->lra; r.refTp = ref->truePeak;
        r.refSMax = ref->shortMax; r.refMMax = ref->momentaryMax;
    }
    r.target = kTargets[targetIdx];
    r.tpLimit = kTpLimits[targetIdx];
    history.target = r.target;

    meters.tick (proc);
    scope.mixCorr = meters.mixCorr;
    scope.refCorr = meters.refCorr;
    scope.refRunning = meters.refRunning;
    scope.showRef = spectrum.showRef;

    if (++tickCount % 3 == 0)
        history.push (r.mixS > -70.0f ? r.mixS : -100.0f, r.refRunning ? r.refS : -100.0f);

    // Knöpfe
    const bool ab = vts.getRawParameterValue ("ab")->load() > 0.5f && ref != nullptr;
    btnA.setToggleState (! ab, juce::dontSendNotification);
    btnB.setToggleState (ab, juce::dontSendNotification);
    btnB.setEnabled (ref != nullptr);
    btnClear.setEnabled (ref != nullptr);
    const bool sync = vts.getRawParameterValue ("sync")->load() > 0.5f;
    btnPlay.setEnabled (! sync && ref != nullptr);
    btnPlay.setToggleState (! sync && proc.isFreePlaying(), juce::dontSendNotification);
    btnPlay.setButtonText (! sync && proc.isFreePlaying() ? "STOP" : "PLAY");

    for (int i = 0; i < AnalyzerProcessor::numSlots; ++i)
    {
        auto& b = btnSlot[(size_t) i];
        b.setToggleState (i == slot, juce::dontSendNotification);
        const bool loaded = proc.getRef (i) != nullptr;
        if ((bool) b.getProperties().getWithDefault ("dot", false) != loaded)
        {
            b.getProperties().set ("dot", loaded);
            b.repaint();
        }
    }

    const bool monAlert = vts.getRawParameterValue ("monitor")->load() > 0.5f;
    const bool bandAlert = vts.getRawParameterValue ("band")->load() > 0.5f;
    if ((bool) cbMonitor.getProperties().getWithDefault ("alert", false) != monAlert)
        { cbMonitor.getProperties().set ("alert", monAlert); cbMonitor.repaint(); }
    if ((bool) cbBand.getProperties().getWithDefault ("alert", false) != bandAlert)
        { cbBand.getProperties().set ("alert", bandAlert); cbBand.repaint(); }

    spectrum.repaint();
    tonal.repaint();
    loud.repaint();
    meters.repaint();
    if (history.isVisible()) history.repaint();
    if (scope.isVisible()) scope.repaint();
    wave.repaint();
    repaint (rRef.withTrimmedLeft (rLoud.getX() - rRef.getX()));
    repaint (rHeader.withTrimmedLeft (460).withWidth (390));
}

//==============================================================================
AnalyzerEditor::AnalyzerEditor (AnalyzerProcessor& p)
    : AudioProcessorEditor (p), proc (p), content (p)
{
    setLookAndFeel (&look);
    addAndMakeVisible (content);

    setResizable (true, true);
    setResizeLimits ((int) (Content::W * 0.55), (int) (Content::H * 0.55), (int) (Content::W * 1.5), (int) (Content::H * 1.5));
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) Content::W / (double) Content::H);

    const float scale = juce::jlimit (0.55f, 1.5f, proc.getUiScale());
    setSize (juce::roundToInt (Content::W * scale), juce::roundToInt (Content::H * scale));
    startTimerHz (30);
}

AnalyzerEditor::~AnalyzerEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void AnalyzerEditor::resized()
{
    const float s = (float) getWidth() / (float) Content::W;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, Content::W, Content::H);
    proc.setUiScale (s);
}

static bool isAudioFile (const juce::File& f)
{
    return f.hasFileExtension ("wav;wave;aif;aiff;flac;mp3;ogg;m4a;aac;caf");
}

bool AnalyzerEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (isAudioFile (juce::File (f)))
            return true;
    return false;
}

void AnalyzerEditor::filesDropped (const juce::StringArray& files, int, int)
{
    content.setDragHover (false);
    int slot = proc.currentSlot();
    for (auto& f : files)
    {
        const juce::File file (f);
        if (isAudioFile (file) && slot < AnalyzerProcessor::numSlots)
            proc.loadReference (file, slot++);
    }
}

} // namespace k54a

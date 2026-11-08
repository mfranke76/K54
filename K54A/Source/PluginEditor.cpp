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
    g.setColour (col::panel2.brighter (box.isMouseOver (true) ? 0.12f : 0.0f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (col::line);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    juce::Path p;
    const float cx = (float) w - 11.0f, cy = (float) h * 0.5f;
    p.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
    g.setColour (col::gold);
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
    return r.getX() + r.getWidth() * std::log (f / SpectrumAnalyser::fMin) / std::log (SpectrumAnalyser::fMax / SpectrumAnalyser::fMin);
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

void SpectrumView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0c0c0e));
    g.fillRoundedRectangle (b, 4.0f);

    auto plot = b.withTrimmedLeft (34.0f).withTrimmedBottom (20.0f).withTrimmedTop (6.0f).withTrimmedRight (8.0f);

    // Raster Frequenz
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

    // Raster dB
    for (int db = 0; db >= (int) botDb; db -= 12)
    {
        const float y = yForDb ((float) db, plot);
        g.setColour (db == 0 ? col::line.brighter (0.4f) : col::grid.brighter (0.15f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (col::dim);
        g.drawText (juce::String (db), juce::Rectangle<float> (0.0f, y - 7.0f, 30.0f, 14.0f), juce::Justification::centredRight);
    }

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
    }

    if (! showMix && ! (showRef && refLoaded))
    {
        g.setColour (col::dim);
        g.setFont (font (14.0f));
        g.drawText (u8s ("Beide Kurven ausgeblendet – oben MIX oder REFERENZ einschalten"), plot, juce::Justification::centred);
    }

    // Maus-Anzeige
    if (mouseIn && plot.contains (mouse))
    {
        const float t = (mouse.x - plot.getX()) / plot.getWidth();
        const float f = SpectrumAnalyser::fMin * std::pow (SpectrumAnalyser::fMax / SpectrumAnalyser::fMin, t);
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

        g.setFont (font (12.0f, true));
        const float tw = (float) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), txt) + 16.0f;
        float tx = mouse.x + 10.0f;
        if (tx + tw > plot.getRight()) tx = mouse.x - tw - 10.0f;
        auto box = juce::Rectangle<float> (tx, plot.getY() + 6.0f, tw, 22.0f);
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
    const float cMixX = 116.0f, cRefX = 214.0f, cW = w - 214.0f - 10.0f;
    const float mixW = 92.0f;

    g.setFont (font (12.0f, true));
    g.setColour (col::gold);
    g.drawText ("MIX", juce::Rectangle<float> (cMixX, 0, mixW, 16), juce::Justification::centredRight);
    g.setColour (col::ref);
    g.drawText ("REFERENZ", juce::Rectangle<float> (cRefX, 0, cW, 16), juce::Justification::centredRight);

    const bool hasTarget = r.target < 100.0f;
    const float dMix = r.mixI > -90.0f && hasTarget ? r.mixI - r.target : -1000.0f;
    const float dRef = r.refLoaded && r.refI > -90.0f && hasTarget ? r.refI - r.target : -1000.0f;

    struct Row { const char* label; const char* unit; float mix; float ref; float h; float fs; int kind; };
    const Row rows[] {
        { "Integrated",    "LUFS", r.mixI,   r.refLoaded ? r.refI : -100.0f,                    48, 30, 0 },
        { "Short-Term",    "LUFS", r.mixS,   r.refRunning ? r.refS : -100.0f,                   30, 19, 0 },
        { "Momentary",     "LUFS", r.mixM,   r.refRunning ? r.refM : -100.0f,                   30, 19, 0 },
        { "LRA",           "LU",   r.mixLra, r.refLoaded ? r.refLra : -100.0f,                  28, 16, 1 },
        { "True Peak max", "dBTP", r.mixTp,  r.refLoaded ? r.refTp : -100.0f,                   28, 16, 2 },
        { "zum Ziel",      "LU",   dMix,     dRef,                                              28, 16, 3 },
    };

    float y = 20.0f;
    for (auto& row : rows)
    {
        g.setColour (col::line.withAlpha (0.6f));
        g.drawHorizontalLine ((int) y, 0.0f, w);

        g.setColour (col::text.withAlpha (0.85f));
        g.setFont (font (12.5f, true));
        const juce::String lab = row.kind == 3 ? u8s ("Δ zum Ziel") : juce::String (row.label);
        g.drawText (lab, juce::Rectangle<float> (10, y + 4, 110, 15), juce::Justification::centredLeft);
        g.setColour (col::dim);
        g.setFont (font (10.5f));
        g.drawText (row.unit, juce::Rectangle<float> (10, y + 18, 100, 12), juce::Justification::centredLeft);

        auto value = [&] (float v, juce::Rectangle<float> area, juce::Colour base)
        {
            juce::Colour c = base;
            juce::String s;
            if (row.kind == 3)
            {
                if (v < -900.0f) { s = u8s ("–"); c = col::dim; }
                else
                {
                    s = fmt (v, 1, true);
                    c = std::abs (v) <= 1.0f ? col::good : col::warn;
                }
            }
            else
            {
                s = row.kind == 1 && v < -90.0f ? u8s ("–") : fmt (v);
                if (row.kind == 2 && v > -90.0f)
                    c = v > 0.0f ? col::red : (v > -1.0f ? col::warn : base);
                if (v <= -90.0f) c = col::dim;
            }
            g.setColour (c);
            g.setFont (font (row.fs, true));
            g.drawText (s, area, juce::Justification::centredRight);
        };

        value (row.mix, juce::Rectangle<float> (cMixX, y + 2, mixW, row.h - 4), col::text);
        value (row.ref, juce::Rectangle<float> (cRefX, y + 2, cW, row.h - 4), col::ref.brighter (0.35f));
        y += row.h;
    }

    g.setColour (col::dim);
    g.setFont (font (10.5f));
    g.drawText (u8s ("Referenz: I, LRA, TP = ganze Original-Datei"), juce::Rectangle<float> (10, y + 4, w - 20, 14),
                juce::Justification::centredLeft);
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
    const float top = 34.0f, bottom = (float) getHeight() - 52.0f, hgt = bottom - top;
    auto yOf = [&] (float db) { return top + (1.0f - (juce::jlimit (-60.0f, 3.0f, db) + 60.0f) / 63.0f) * hgt; };

    auto drawBar = [&] (float x, float w, const Bar& b, juce::Colour c)
    {
        auto r = juce::Rectangle<float> (x, top, w, hgt);
        g.setColour (juce::Colour (0xff0c0c0e));
        g.fillRect (r);
        if (b.rms > -60.0f)
        {
            const float yr = yOf (b.rms);
            juce::ColourGradient grad (c.darker (0.3f), 0, bottom, c, 0, top, false);
            grad.addColour ((double) ((yOf (-6.0f) - top) / hgt), c);
            g.setGradientFill (grad);
            g.fillRect (juce::Rectangle<float> (x, yr, w, bottom - yr));
        }
        if (b.peak > -60.0f)
        {
            g.setColour (b.peak > 0.0f ? col::red : c.brighter (0.5f).withAlpha (0.9f));
            const float yp = yOf (b.peak);
            g.fillRect (juce::Rectangle<float> (x, yp, w, juce::jmin (3.0f, bottom - yp)));
            g.setColour (c.withAlpha (0.25f));
            g.fillRect (juce::Rectangle<float> (x, yp, w, bottom - yp));
        }
        if (b.hold > -60.0f)
        {
            g.setColour (b.hold > 0.0f ? col::red : col::text);
            g.fillRect (juce::Rectangle<float> (x, yOf (b.hold) - 0.5f, w, 1.5f));
        }
    };

    auto drawLufs = [&] (float x, float w, float v, juce::Colour c)
    {
        auto r = juce::Rectangle<float> (x, top, w, hgt);
        g.setColour (juce::Colour (0xff0c0c0e));
        g.fillRect (r);
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

        drawBar (gx, 20.0f, l, c);
        drawBar (gx + 24.0f, 20.0f, r, c);
        drawLufs (gx + 54.0f, 22.0f, m, c);

        g.setFont (font (10.0f, true));
        g.setColour (col::dim);
        g.drawText ("L", juce::Rectangle<float> (gx, bottom + 2, 20, 12), juce::Justification::centred);
        g.drawText ("R", juce::Rectangle<float> (gx + 24.0f, bottom + 2, 20, 12), juce::Justification::centred);
        g.drawText ("M", juce::Rectangle<float> (gx + 54.0f, bottom + 2, 22, 12), juce::Justification::centred);
        g.setColour (col::text.withAlpha (active ? 0.9f : 0.4f));
        g.drawText (fmt (m), juce::Rectangle<float> (gx + 44.0f, 15, 42, 14), juce::Justification::centredRight);

        // Korrelation
        const float cy = bottom + 22.0f;
        auto cr = juce::Rectangle<float> (gx, cy, 112.0f, 8.0f);
        g.setColour (juce::Colour (0xff0c0c0e));
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
        g.drawText (u8s ("Korrelation"), juce::Rectangle<float> (gx + 20, cy + 10, 72, 11), juce::Justification::centred);
        g.drawText ("+1", juce::Rectangle<float> (gx + 92, cy + 10, 20, 11), juce::Justification::centredRight);
    };

    group (12.0f, "MIX", mL, mR, mixM, mixMax, mixCorr, col::gold, true);
    group ((float) getWidth() - 124.0f, "REFERENZ", rL, rR, refM, refRunning ? refMax : -100.0f, refCorr, col::ref, refRunning);

    // Skala in der Mitte
    const float sx = 12.0f + 80.0f;
    g.setFont (font (10.0f));
    for (int db : { 0, -6, -12, -18, -24, -30, -36, -48, -60 })
    {
        const float y = yOf ((float) db);
        g.setColour (col::line);
        g.drawHorizontalLine ((int) y, sx + 2.0f, sx + 8.0f);
        g.setColour (col::dim);
        g.drawText (juce::String (db), juce::Rectangle<float> (sx + 10.0f, y - 6.0f, 30.0f, 12.0f), juce::Justification::centredLeft);
    }
}

//==============================================================================
// VERLAUF
void HistoryView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0c0c0e));
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
// WELLENFORM REFERENZ
void WaveView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0c0c0e));
    g.fillRoundedRectangle (b, 4.0f);

    auto* ref = proc.getRef();
    const int status = proc.getRefStatus();

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
        juce::String msg = u8s ("Referenz-Track hierher ziehen oder LADEN klicken");
        if (status == AnalyzerProcessor::refLoading) msg = proc.getRefMessage();
        g.drawText (msg, b.withTrimmedBottom (18), juce::Justification::centred);
        g.setFont (font (11.0f));
        g.setColour (status == AnalyzerProcessor::refError ? col::red : col::dim);
        g.drawText (status == AnalyzerProcessor::refError ? proc.getRefMessage()
                                                          : u8s ("WAV · AIFF · FLAC · MP3 · OGG (Mac auch M4A)"),
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

    // Kopfzeile
    g.setFont (font (12.5f, true));
    g.setColour (col::text);
    auto head = b.reduced (8.0f, 2.0f).withHeight (18.0f);
    g.drawText (ref->name, head.withWidth (head.getWidth() * 0.55f), juce::Justification::centredLeft, true);
    g.setFont (font (11.5f));
    g.setColour (col::dim);
    const juce::String info = mmss (posSec) + " / " + mmss (ref->seconds) + u8s ("   ·   ") + fmt (ref->integrated)
                            + " LUFS   " + fmt (ref->truePeak) + " dBTP";
    g.drawText (status == AnalyzerProcessor::refLoading ? proc.getRefMessage() : info, head, juce::Justification::centredRight);

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
    g.fillAll (juce::Colours::black.withAlpha (0.86f));

    juce::AttributedString as;
    as.setLineSpacing (3.0f);
    auto add = [&] (const char* t, float h, juce::Colour c, bool bold = false) { as.append (u8s (t), font (h, bold), c); };

    add ("So funktioniert der K-54 Analyzer\n\n", 22, col::gold, true);
    add ("Einsetzen  ", 14, col::goldHi, true);
    add ("Als letztes Plugin auf den Master (Stereo-Ausgang) legen. Er misst immer dein Signal am Eingang.\n\n", 14, col::text);
    add ("Spektrum 20 Hz – 20 kHz  ", 14, col::goldHi, true);
    add ("Gold = dein Mix, Blau = Referenz. Mit den Knöpfen MIX und REFERENZ blendest du jede Kurve ein oder aus. "
         "NEIGUNG 4,5 dB/Okt. lässt einen ausgewogenen Mix ungefähr waagerecht aussehen. GLÄTTUNG macht die Kurven ruhiger, "
         "HOLD zeigt die Spitzen. Maus über das Spektrum = Frequenz, Ton und Pegel.\n\n", 14, col::text);
    add ("Lautheit (EBU R128 / ITU BS.1770)  ", 14, col::goldHi, true);
    add ("Integrated = Durchschnitt über den ganzen Song. Short-Term = letzte 3 s, Momentary = letzte 0,4 s. "
         "LRA = Lautheitsumfang. True Peak = echte Spitzen zwischen den Samples – für Streaming unter -1 dBTP bleiben. "
         "Beim Start der DAW wird neu gemessen (AUTO-RESET), sonst mit RESET.\n\n", 14, col::text);
    add ("Referenz  ", 14, col::goldHi, true);
    add ("LADEN oder Datei in das Fenster ziehen. Mit A (Mix) und B (Referenz) oben schaltest du beim Hören hin und her.\n"
         "LAUTHEIT ANGLEICHEN bringt die Referenz auf die Lautheit deines Mixes – so vergleichst du Klang statt Lautstärke. "
         "Die Zahlen der Referenz zeigen trotzdem immer die Original-Datei.\n"
         "DAW-SYNC an: Die Referenz läuft mit der DAW mit. Klick in die Wellenform = Referenz an diese Stelle setzen.\n"
         "DAW-SYNC aus: Die Referenz läuft frei mit PLAY/STOP und fängt am Ende wieder vorne an.\n\n", 14, col::text);
    add ("Klick irgendwo, um die Hilfe zu schließen.", 12, col::dim);

    juce::TextLayout tl;
    tl.createLayout (as, 760.0f);
    auto box = getLocalBounds().toFloat().withSizeKeepingCentre (820.0f, tl.getHeight() + 52.0f);
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

    // A/B
    btnA.setButtonText (u8s ("A  ·  MIX"));
    btnB.setButtonText (u8s ("B  ·  REFERENZ"));
    for (auto* b : { &btnA, &btnB }) { b->getProperties().set ("big", true); addAndMakeVisible (*b); }
    setAccent (btnA, col::gold);
    setAccent (btnB, col::ref);
    btnA.onClick = [this] { setAB (false); };
    btnB.onClick = [this] { setAB (true); };
    btnA.setTooltip ("Mix hören");
    btnB.setTooltip ("Referenz hören");

    // Kopf rechts
    lbTarget.setText ("ZIEL", juce::dontSendNotification);
    lbTarget.setFont (font (11.0f, true));
    lbTarget.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (lbTarget);
    cbTarget.addItemList (vts.getParameter ("target")->getAllValueStrings(), 1);
    addAndMakeVisible (cbTarget);
    aTarget = std::make_unique<CA> (vts, "target", cbTarget);

    btnReset.setButtonText ("RESET");
    btnReset.onClick = [this] { proc.resetMeasurements(); history.clear(); mixA.resetHold(); refA.resetHold(); };
    addAndMakeVisible (btnReset);
    btnHelp.setButtonText ("?");
    btnHelp.onClick = [this] { help.setVisible (true); help.toFront (false); };
    addAndMakeVisible (btnHelp);

    // Spektrum-Leiste
    setupToggle (btnShowMix, "MIX", col::gold);
    setupToggle (btnShowRef, "REFERENZ", col::ref);
    setupToggle (btnHold, "HOLD", col::goldHi);
    aShowMix = std::make_unique<BA> (vts, "showMix", btnShowMix);
    aShowRef = std::make_unique<BA> (vts, "showRef", btnShowRef);
    aHold    = std::make_unique<BA> (vts, "hold", btnHold);

    auto setupCombo = [this, &vts] (juce::ComboBox& cb, juce::Label& lb, const char* label, const char* id,
                                    std::unique_ptr<CA>& att)
    {
        lb.setText (u8s (label), juce::dontSendNotification);
        lb.setFont (font (10.5f, true));
        lb.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (lb);
        cb.addItemList (vts.getParameter (id)->getAllValueStrings(), 1);
        addAndMakeVisible (cb);
        att = std::make_unique<CA> (vts, id, cb);
    };
    setupCombo (cbSlope, lbSlope, "NEIGUNG", "slope", aSlope);
    setupCombo (cbSpeed, lbSpeed, "TEMPO", "speed", aSpeed);
    setupCombo (cbSmooth, lbSmooth, "GLÄTTUNG", "smooth", aSmooth);

    setupToggle (btnAuto, "AUTO-RESET", col::gold);
    aAuto = std::make_unique<BA> (vts, "autoReset", btnAuto);
    btnAuto.setTooltip (u8s ("Integrated/LRA/True Peak bei jedem DAW-Start neu messen"));

    // Referenz-Leiste
    btnLoad.setButtonText (u8s ("LADEN …"));
    btnLoad.onClick = [this] { openFileChooser(); };
    addAndMakeVisible (btnLoad);
    btnClear.setButtonText ("ENTFERNEN");
    btnClear.onClick = [this] { proc.clearReference(); if (proc.apvts.getRawParameterValue ("ab")->load() > 0.5f) setAB (false); };
    addAndMakeVisible (btnClear);

    setupToggle (btnSync, "DAW-SYNC", col::gold);
    aSync = std::make_unique<BA> (vts, "sync", btnSync);
    btnPlay.setButtonText ("PLAY");
    btnPlay.onClick = [this] { proc.setFreePlaying (! proc.isFreePlaying()); };
    addAndMakeVisible (btnPlay);

    setupToggle (btnMatch, "LAUTHEIT ANGLEICHEN", col::ref);
    aMatch = std::make_unique<BA> (vts, "match", btnMatch);

    lbGain.setText ("GAIN", juce::dontSendNotification);
    lbGain.setFont (font (10.5f, true));
    addAndMakeVisible (lbGain);
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

    addAndMakeVisible (spectrum);
    addAndMakeVisible (loud);
    addAndMakeVisible (meters);
    addAndMakeVisible (history);
    addAndMakeVisible (wave);
    addChildComponent (help);

    pullBuf.resize (1 << 17);
    setSize (W, H);
}

Content::~Content() = default;

void Content::setAB (bool b)
{
    if (b && proc.getRef() == nullptr)
        b = false;
    if (auto* prm = proc.apvts.getParameter ("ab"))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (b ? 1.0f : 0.0f);
        prm->endChangeGesture();
    }
    if (b && proc.apvts.getRawParameterValue ("sync")->load() < 0.5f && ! proc.isFreePlaying())
        proc.setFreePlaying (true);
}

void Content::openFileChooser()
{
    chooser = std::make_unique<juce::FileChooser> (u8s ("Referenz-Track laden"), juce::File(),
                                                   "*.wav;*.wave;*.aif;*.aiff;*.flac;*.mp3;*.ogg;*.m4a;*.aac;*.caf");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.existsAsFile())
                                  proc.loadReference (f);
                          });
}

void Content::resized()
{
    rHeader = { 0, 0, W, 58 };
    rSpec   = { 12, 66, 834, 424 };
    rHist   = { 12, 498, 834, 116 };
    rLoud   = { 858, 66, 330, 270 };
    rMeter  = { 858, 344, 330, 270 };
    rRef    = { 12, 622, 1176, 106 };

    btnA.setBounds (372, 11, 190, 36);
    btnB.setBounds (570, 11, 230, 36);
    lbTarget.setBounds (818, 15, 40, 28);
    cbTarget.setBounds (862, 15, 196, 28);
    btnReset.setBounds (1066, 15, 72, 28);
    btnHelp.setBounds (1146, 15, 40, 28);

    const int sx = rSpec.getX(), sy = rSpec.getY() + 7;
    btnShowMix.setBounds (sx + 100, sy, 64, 24);
    btnShowRef.setBounds (sx + 170, sy, 96, 24);
    btnHold.setBounds (sx + 272, sy, 56, 24);
    lbSlope.setBounds (sx + 334, sy, 62, 24);
    cbSlope.setBounds (sx + 398, sy, 98, 24);
    lbSpeed.setBounds (sx + 498, sy, 48, 24);
    cbSpeed.setBounds (sx + 548, sy, 84, 24);
    lbSmooth.setBounds (sx + 634, sy, 70, 24);
    cbSmooth.setBounds (sx + 706, sy, 118, 24);
    spectrum.setBounds (rSpec.getX() + 8, rSpec.getY() + 38, rSpec.getWidth() - 16, rSpec.getHeight() - 46);

    history.setBounds (rHist.getX() + 8, rHist.getY() + 28, rHist.getWidth() - 16, rHist.getHeight() - 36);

    btnAuto.setBounds (rLoud.getRight() - 100, rLoud.getY() + 7, 90, 22);
    loud.setBounds (rLoud.getX() + 4, rLoud.getY() + 34, rLoud.getWidth() - 8, rLoud.getHeight() - 38);
    meters.setBounds (rMeter.getX() + 4, rMeter.getY() + 30, rMeter.getWidth() - 8, rMeter.getHeight() - 34);

    const int rx = rRef.getX(), ry = rRef.getY();
    btnLoad.setBounds (rx + 12, ry + 32, 118, 30);
    btnClear.setBounds (rx + 12, ry + 68, 118, 26);
    wave.setBounds (rx + 142, ry + 10, 700, 86);
    const int x0 = rLoud.getX();
    btnSync.setBounds (x0, ry + 10, 150, 26);
    btnPlay.setBounds (x0 + 158, ry + 10, 92, 26);
    lbGain.setBounds (x0, ry + 42, 40, 26);
    sGain.setBounds (x0 + 40, ry + 42, 290, 26);
    btnMatch.setBounds (x0, ry + 72, 190, 26);

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
        g.drawText (title, r.getX() + 12, r.getY() + 7, 300, 24, juce::Justification::centredLeft);
    }
}

void Content::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);

    // Kopf
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
    g.drawText ("ANALYZER", 108, 9, 160, 22, juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (font (11.0f));
    g.drawText (u8s ("Keller54 · by Frank E."), 108, 30, 200, 16, juce::Justification::centredLeft);

    panel (g, rSpec, u8s ("SPEKTRUM"));
    g.setColour (col::dim);
    g.setFont (font (10.0f));
    g.drawText (u8s ("20 Hz – 20 kHz"), rSpec.getX() + 12, rSpec.getY() + 24, 90, 12, juce::Justification::centredLeft);

    panel (g, rHist, u8s ("VERLAUF  ·  Short-Term LUFS, letzte 60 s"));
    panel (g, rLoud, u8s ("LAUTHEIT"));
    panel (g, rMeter, u8s ("PEGEL"));
    panel (g, rRef, {});

    g.setColour (col::ref);
    g.setFont (font (12.0f, true));
    g.drawText ("REFERENZ", rRef.getX() + 12, rRef.getY() + 8, 120, 20, juce::Justification::centredLeft);

    // Angleich-Anzeige
    const bool match = proc.apvts.getRawParameterValue ("match")->load() > 0.5f;
    g.setFont (font (11.5f));
    g.setColour (match && proc.getRef() != nullptr ? col::text : col::dim);
    const juce::String m = proc.getRef() == nullptr ? u8s ("–")
                         : (match ? u8s ("Angleich ") + fmt (proc.getMatchGainDb(), 1, true) + " dB" : u8s ("aus"));
    g.drawText (m, rLoud.getX() + 196, rRef.getY() + 72, 134, 26, juce::Justification::centredLeft);

}

void Content::tick()
{
    double s = proc.getSampleRate();
    if (s <= 0.0) s = 48000.0;
    if (std::abs (mixA.getSampleRate() - s) > 0.5 || mixA.display.empty())
    {
        mixA.prepare (s);
        refA.prepare (s);
    }

    for (int got; (got = proc.mixFifo.pull (pullBuf.data(), (int) pullBuf.size())) > 0;)
        mixA.push (pullBuf.data(), got);
    for (int got; (got = proc.refFifo.pull (pullBuf.data(), (int) pullBuf.size())) > 0;)
        refA.push (pullBuf.data(), got);

    auto& vts = proc.apvts;
    static const float slopes[] { 0.0f, 3.0f, 4.5f, 6.0f };
    static const float smooths[] { 0.0f, 1.0f / 12.0f, 1.0f / 6.0f, 1.0f / 3.0f };
    static const float targets[] { 1000.0f, -14.0f, -16.0f, -23.0f, -9.0f };

    const int slopeIdx  = juce::jlimit (0, 3, (int) vts.getRawParameterValue ("slope")->load());
    const int smoothIdx = juce::jlimit (0, 3, (int) vts.getRawParameterValue ("smooth")->load());
    const int speedIdx  = juce::jlimit (0, 2, (int) vts.getRawParameterValue ("speed")->load());
    const int targetIdx = juce::jlimit (0, 4, (int) vts.getRawParameterValue ("target")->load());
    const bool hold = vts.getRawParameterValue ("hold")->load() > 0.5f;

    mixA.update (slopes[slopeIdx], smooths[smoothIdx], speedIdx, hold);
    refA.update (slopes[slopeIdx], smooths[smoothIdx], speedIdx, hold);

    auto* ref = proc.getRef();
    spectrum.showMix = vts.getRawParameterValue ("showMix")->load() > 0.5f;
    spectrum.showRef = vts.getRawParameterValue ("showRef")->load() > 0.5f;
    spectrum.holdOn = hold;
    spectrum.refLoaded = ref != nullptr;
    spectrum.slope = slopes[slopeIdx];

    // Lautheit
    auto& r = loud.r;
    r.mixM = proc.mixLoud.m.load(); r.mixS = proc.mixLoud.s.load(); r.mixI = proc.mixLoud.i.load();
    r.mixLra = proc.mixLoud.lra.load(); r.mixTp = proc.mixLoud.tp.load();
    r.refLoaded = ref != nullptr;
    r.refRunning = proc.isRefRunning();
    r.refM = proc.refLoud.m.load(); r.refS = proc.refLoud.s.load();
    if (ref != nullptr) { r.refI = ref->integrated; r.refLra = ref->lra; r.refTp = ref->truePeak; }
    r.target = targets[targetIdx];
    history.target = r.target;

    meters.tick (proc);

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
    btnPlay.setTooltip (sync ? u8s ("Nur ohne DAW-SYNC: Referenz frei abspielen") : u8s ("Referenz frei abspielen / anhalten"));

    if (lastRefVersion != proc.getRefVersion())
    {
        lastRefVersion = proc.getRefVersion();
        refA.prepare (s);
    }

    spectrum.repaint();
    loud.repaint();
    meters.repaint();
    history.repaint();
    wave.repaint();
    repaint (rRef.withTrimmedLeft (rLoud.getX() - rRef.getX()));
}

//==============================================================================
AnalyzerEditor::AnalyzerEditor (AnalyzerProcessor& p)
    : AudioProcessorEditor (p), proc (p), content (p)
{
    setLookAndFeel (&look);
    addAndMakeVisible (content);

    setResizable (true, true);
    setResizeLimits ((int) (Content::W * 0.6), (int) (Content::H * 0.6), (int) (Content::W * 1.6), (int) (Content::H * 1.6));
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) Content::W / (double) Content::H);

    const float scale = juce::jlimit (0.6f, 1.6f, proc.getUiScale());
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

bool AnalyzerEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;wave;aif;aiff;flac;mp3;ogg;m4a;aac;caf"))
            return true;
    return false;
}

void AnalyzerEditor::filesDropped (const juce::StringArray& files, int, int)
{
    content.setDragHover (false);
    for (auto& f : files)
    {
        const juce::File file (f);
        if (file.hasFileExtension ("wav;wave;aif;aiff;flac;mp3;ogg;m4a;aac;caf"))
        {
            proc.loadReference (file);
            return;
        }
    }
}

} // namespace k54a

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

    juce::ColourGradient gloss (fill.brighter (on ? 0.25f : 0.10f), 0.0f, r.getY(), fill.darker (on ? 0.12f : 0.05f), 0.0f, r.getBottom(), false);
    g.setGradientFill (gloss);
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
static juce::String fmtFreq (float f)
{
    return f < 1000.0f ? juce::String (juce::roundToInt (f)) + " Hz"
                       : juce::String (f / 1000.0f, f < 10000.0f ? 2 : 1).replaceCharacter ('.', ',') + " kHz";
}

float SpectrumView::xForFreq (float f, juce::Rectangle<float> r) const
{
    return r.getX() + r.getWidth() * std::log (f / SpecMath::fMin) / std::log (SpecMath::fMax / SpecMath::fMin);
}

float SpectrumView::yForDb (float db, juce::Rectangle<float> r) const
{
    const float t = (juce::jlimit (botDb - 2.0f, topDb, db) - topDb) / (botDb - topDb);
    return r.getY() + t * r.getHeight();
}

juce::Colour SpectrumView::gridCol (juce::Colour base) const
{
    return gridLevel == 0 ? base.withAlpha (0.45f) : (gridLevel == 2 ? base.brighter (0.45f) : base);
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

void SpectrumView::setStyle (const ViewStyle& s)
{
    const int oldMode = mode;
    mode = juce::jlimit (0, 2, s.specMode);
    topDb = s.topDb();
    botDb = s.botDb();
    lineW = s.lineWidth();
    fillA = s.fillAlpha();
    gridLevel = s.grid;
    glow = s.glow;
    peaks = s.peaks;
    rainbowCol = s.specColor == 1;

    // Farbskala fürs Spektrogramm aus der Mix-Farbe
    const auto c = theme().mix;
    juce::ColourGradient grad (col::plot, 0.0f, 0.0f, juce::Colours::white, 1.0f, 0.0f, false);
    if (rainbowCol)
    {
        grad.addColour (0.14, juce::Colour (0xff1b1060));
        grad.addColour (0.34, juce::Colour (0xff7a1fa2));
        grad.addColour (0.54, juce::Colour (0xffe0344a));
        grad.addColour (0.72, juce::Colour (0xffff8a2a));
        grad.addColour (0.88, juce::Colour (0xffffe14a));
    }
    else
    {
        grad.addColour (0.18, col::plot.interpolatedWith (c, 0.10f));
        grad.addColour (0.45, c.darker (0.85f));
        grad.addColour (0.72, c);
        grad.addColour (0.90, c.brighter (0.7f));
    }
    for (int i = 0; i < 256; ++i)
        lut[(size_t) i] = grad.getColourAtPosition ((double) i / 255.0);

    if (oldMode != mode) clearHistory();
    repaint();
}

void SpectrumView::clearHistory()
{
    if (sgram.isValid()) sgram.clear (sgram.getBounds(), col::plot);
    barHoldMix.fill (-130.0f);
    barHoldAge.fill (0);
}

void SpectrumView::computeBars (const std::vector<float>& src, std::array<float, numBars>& out) const
{
    const int n = (int) src.size();
    const float perOct = (float) (n - 1) / std::log2 (SpecMath::fMax / SpecMath::fMin);
    for (int b = 0; b < numBars; ++b)
    {
        const float fc = 1000.0f * std::pow (2.0f, (float) (b - 17) / 3.0f);
        const float lo = std::log2 (fc * std::pow (2.0f, -1.0f / 6.0f) / SpecMath::fMin) * perOct;
        const float hi = std::log2 (fc * std::pow (2.0f,  1.0f / 6.0f) / SpecMath::fMin) * perOct;
        const int a = juce::jlimit (0, n - 1, (int) std::ceil (lo));
        const int e = juce::jlimit (a, n - 1, (int) std::floor (hi));
        double sum = 0.0;
        for (int i = a; i <= e; ++i) sum += std::pow (10.0, src[(size_t) i] / 10.0);
        const double mean = sum / (double) (e - a + 1);
        out[(size_t) b] = mean > 1.0e-13 ? (float) (10.0 * std::log10 (mean)) : -130.0f;
    }
}

void SpectrumView::advance (int holdFrames)
{
    if (mixA.display.empty()) return;

    if (mode == 1)
    {
        computeBars (mixA.display, barMix);
        if (! refA.display.empty()) computeBars (refA.display, barRef);
        for (int b = 0; b < numBars; ++b)
        {
            if (barMix[(size_t) b] >= barHoldMix[(size_t) b]) { barHoldMix[(size_t) b] = barMix[(size_t) b]; barHoldAge[(size_t) b] = 0; }
            else if (++barHoldAge[(size_t) b] > holdFrames)    barHoldMix[(size_t) b] = juce::jmax (barMix[(size_t) b], barHoldMix[(size_t) b] - 0.8f);
        }
    }
    else if (mode == 2)
    {
        const int w = SpecMath::numPoints, h = 240;
        if (! sgram.isValid())
        {
            sgram = juce::Image (juce::Image::RGB, w, h, true);
            sgram.clear (sgram.getBounds(), col::plot);
        }
        sgram.moveImageSection (0, 1, 0, 0, w, h - 1);
        juce::Image::BitmapData bd (sgram, 0, 0, w, 1, juce::Image::BitmapData::writeOnly);
        for (int x = 0; x < w; ++x)
        {
            const float t = juce::jlimit (0.0f, 1.0f, (mixA.instant[(size_t) x] - botDb) / (topDb - 18.0f - botDb));
            bd.setPixelColour (x, 0, lut[(size_t) juce::roundToInt (t * 255.0f)]);
        }
    }
}

// Mix minus Referenz, auf 1/3 Oktave geglättet und um den mittleren Abstand (100 Hz – 10 kHz) bereinigt
void SpectrumView::computeDiff()
{
    const int n = SpecMath::numPoints;
    diffTmp.assign ((size_t) n, 0.0f);
    diff.assign ((size_t) n, 0.0f);
    for (int i = 0; i < n; ++i)
        diffTmp[(size_t) i] = mixA.display[(size_t) i] - refA.display[(size_t) i];

    const int half = (int) std::round ((double) (n - 1) / std::log2 (1000.0) / 6.0);
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

void SpectrumView::strokeCurve (juce::Graphics& g, const juce::Path& p, juce::Colour c, float width) const
{
    if (glow)
    {
        g.setColour (c.withAlpha (0.08f));
        g.strokePath (p, juce::PathStrokeType (width * 6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c.withAlpha (0.20f));
        g.strokePath (p, juce::PathStrokeType (width * 2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SpectrumView::strokeCurveGrad (juce::Graphics& g, const juce::Path& p, const juce::ColourGradient& grad, float width) const
{
    if (glow)
    {
        auto g1 = grad; g1.multiplyOpacity (0.10f);
        g.setGradientFill (g1);
        g.strokePath (p, juce::PathStrokeType (width * 6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        auto g2 = grad; g2.multiplyOpacity (0.25f);
        g.setGradientFill (g2);
        g.strokePath (p, juce::PathStrokeType (width * 2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setGradientFill (grad);
    g.strokePath (p, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SpectrumView::drawCurves (juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto& th = theme();

    if (showRef && refLoaded)
    {
        if (fillA > 0.0f)
        {
            g.setColour (th.ref.withAlpha (fillA * 0.22f));
            g.fillPath (makePath (refA.display, r, true));
        }
        if (holdOn)
        {
            g.setColour (th.ref.withAlpha (0.45f));
            g.strokePath (makePath (refA.hold, r, false), juce::PathStrokeType (0.9f));
        }
    }

    if (showMix)
    {
        if (rainbowCol)
        {
            if (fillA > 0.0f)
            {
                const auto area = makePath (mixA.display, r, true);
                g.setGradientFill (rainbowGradient (r.getX(), r.getRight(), r.getY(), juce::jmin (1.0f, fillA * 1.6f)));
                g.fillPath (area);
                juce::ColourGradient fade (col::plot.withAlpha (0.0f), 0.0f, r.getY(), col::plot.withAlpha (0.92f), 0.0f, r.getBottom(), false);
                g.setGradientFill (fade);
                g.fillPath (area);
            }
            strokeCurveGrad (g, makePath (mixA.display, r, false), rainbowGradient (r.getX(), r.getRight(), r.getY(), 1.0f), lineW * 1.1f);
        }
        else
        {
            if (fillA > 0.0f)
            {
                juce::ColourGradient grad (th.mix.withAlpha (fillA), 0.0f, r.getY(),
                                           th.mix.withAlpha (fillA * 0.06f), 0.0f, r.getBottom(), false);
                g.setGradientFill (grad);
                g.fillPath (makePath (mixA.display, r, true));
            }
            strokeCurve (g, makePath (mixA.display, r, false), th.mix, lineW);
        }
        if (holdOn)
        {
            g.setColour (th.mix.brighter (0.4f).withAlpha (0.6f));
            g.strokePath (makePath (mixA.hold, r, false), juce::PathStrokeType (0.9f));
        }
    }

    if (showRef && refLoaded)
    {
        const auto p = makePath (refA.display, r, false);
        if (rainbowCol)
        {
            g.setColour (col::bg.withAlpha (0.85f));
            g.strokePath (p, juce::PathStrokeType (lineW + 3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        strokeCurve (g, p, th.ref, lineW * 1.15f);
    }
}

void SpectrumView::drawBars (juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto& th = theme();
    const float segH = 4.0f, gap = 1.5f;

    for (int b = 0; b < numBars; ++b)
    {
        const float fc = 1000.0f * std::pow (2.0f, (float) (b - 17) / 3.0f);
        const float xl = juce::jmax (r.getX(), xForFreq (fc * std::pow (2.0f, -1.0f / 6.0f), r));
        const float xr = juce::jmin (r.getRight(), xForFreq (fc * std::pow (2.0f, 1.0f / 6.0f), r));
        const auto col = juce::Rectangle<float> (xl + 1.5f, r.getY(), juce::jmax (2.0f, xr - xl - 3.0f), r.getHeight());

        if (showRef && refLoaded)
        {
            const float yr = yForDb (barRef[(size_t) b], r);
            g.setColour (th.ref.withAlpha (0.10f));
            g.fillRect (col.withTop (yr));
        }

        if (showMix)
        {
            const float ym = yForDb (barMix[(size_t) b], r);
            for (float y = r.getBottom() - segH; y >= r.getY(); y -= segH + gap)
            {
                const bool lit = y + segH * 0.5f >= ym;
                const float frac = (r.getBottom() - y) / r.getHeight();
                const auto base = rainbowCol ? rainbow ((float) b / (float) (numBars - 1)) : th.mix;
                const auto c = base.interpolatedWith (base.brighter (0.8f), frac * frac);
                g.setColour (lit ? c : base.withAlpha (0.06f));
                g.fillRect (col.getX(), y, col.getWidth(), segH);
            }
            const float yh = yForDb (barHoldMix[(size_t) b], r);
            if (barHoldMix[(size_t) b] > botDb)
            {
                g.setColour (juce::Colours::white.withAlpha (0.85f));
                g.fillRect (col.getX(), yh - 1.0f, col.getWidth(), 2.0f);
            }
        }

        if (showRef && refLoaded)
        {
            const float yr = yForDb (barRef[(size_t) b], r);
            const auto rc = th.ref;
            if (glow)
            {
                g.setColour (rc.withAlpha (0.25f));
                g.fillRect (col.getX() - 1.0f, yr - 3.0f, col.getWidth() + 2.0f, 6.0f);
            }
            g.setColour (rc);
            g.fillRect (col.getX(), yr - 1.5f, col.getWidth(), 3.0f);
        }
    }
}

void SpectrumView::drawSpectrogram (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (col::plot);
    g.fillRect (r);
    if (sgram.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);
        g.drawImage (sgram, r, juce::RectanglePlacement::stretchToFit);
    }
    g.setColour (col::line);
    g.drawRect (r, 1.0f);

    // Zeitachse (30 Zeilen pro Sekunde)
    const float rowsPerSec = 30.0f, total = 240.0f;
    g.setFont (font (10.0f));
    for (int sec = 2; sec <= 8; sec += 2)
    {
        const float y = r.getY() + r.getHeight() * ((float) sec * rowsPerSec / total);
        g.setColour (col::text.withAlpha (0.18f));
        g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
        g.setColour (col::dim);
        g.drawText ("-" + juce::String (sec) + " s", juce::Rectangle<float> (r.getX() - 34.0f, y - 6.0f, 30.0f, 12.0f),
                    juce::Justification::centredRight);
    }
    g.setColour (col::text.withAlpha (0.7f));
    g.setFont (font (10.5f, true));
    g.drawText (u8s ("SPEKTROGRAMM · Mix · neueste Zeile oben"), r.reduced (8.0f, 4.0f).withHeight (14.0f),
                juce::Justification::centredLeft);

    // Farblegende
    auto leg = juce::Rectangle<float> (r.getRight() - 130.0f, r.getY() + 6.0f, 110.0f, 8.0f);
    for (int i = 0; i < (int) leg.getWidth(); ++i)
    {
        g.setColour (lut[(size_t) juce::jlimit (0, 255, i * 255 / (int) leg.getWidth())]);
        g.fillRect (leg.getX() + (float) i, leg.getY(), 1.0f, leg.getHeight());
    }
    g.setColour (col::dim);
    g.setFont (font (9.5f));
    g.drawText (juce::String ((int) botDb), leg.withY (leg.getBottom() + 1.0f).withHeight (11.0f), juce::Justification::centredLeft);
    g.drawText (juce::String ((int) topDb - 18) + " dB", leg.withY (leg.getBottom() + 1.0f).withHeight (11.0f), juce::Justification::centredRight);
}

void SpectrumView::drawPeakLabels (juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto& v = mixA.display;
    const int n = (int) v.size(), win = 21;
    if (n < 2 * win + 1) return;

    std::vector<std::pair<float, int>> cand;
    for (int i = win; i < n - win; ++i)
    {
        const float x = v[(size_t) i];
        if (x < botDb + 10.0f) continue;
        bool isMax = true;
        float lo = x;
        for (int k = i - win; k <= i + win && isMax; ++k)
        {
            if (v[(size_t) k] > x) isMax = false;
            lo = juce::jmin (lo, v[(size_t) k]);
        }
        if (isMax && x - lo > 5.0f) cand.push_back ({ x, i });
    }
    std::sort (cand.begin(), cand.end(), [] (auto& a, auto& b) { return a.first > b.first; });

    std::vector<float> used;
    int count = 0;
    for (auto& c : cand)
    {
        const float x = r.getX() + r.getWidth() * (float) c.second / (float) (n - 1);
        bool clash = false;
        for (auto u : used) clash = clash || std::abs (u - x) < 80.0f;
        if (clash) continue;
        used.push_back (x);

        const float y = yForDb (c.first, r);
        juce::Path tri;
        tri.addTriangle (x - 4.0f, y - 9.0f, x + 4.0f, y - 9.0f, x, y - 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillPath (tri);

        const auto txt = fmtFreq (SpecMath::pointFreq (c.second));
        g.setFont (font (10.5f, true));
        const float tw = (float) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), txt) + 10.0f;
        auto pill = juce::Rectangle<float> (x - tw * 0.5f, juce::jmax (r.getY() + 2.0f, y - 27.0f), tw, 16.0f);
        pill = pill.withX (juce::jlimit (r.getX(), r.getRight() - tw, pill.getX()));
        g.setColour (col::bg.withAlpha (0.8f));
        g.fillRoundedRectangle (pill, 8.0f);
        g.setColour (rainbowCol ? rainbow ((float) c.second / (float) (n - 1)) : theme().mix.brighter (0.3f));
        g.drawRoundedRectangle (pill, 8.0f, 1.2f);
        g.setColour (col::text);
        g.drawText (txt, pill, juce::Justification::centred);
        if (++count >= 3) break;
    }
}

void SpectrumView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    {
        juce::ColourGradient bgGrad (juce::Colour (0xff111114), 0.0f, b.getY(), juce::Colour (0xff08080a), 0.0f, b.getBottom(), false);
        g.setGradientFill (bgGrad);
        g.fillRoundedRectangle (b, 4.0f);
    }

    const bool drawDiff = showDiff && refLoaded && showMix && mode != 1;
    auto plot = b.withTrimmedLeft (34.0f).withTrimmedBottom (20.0f).withTrimmedTop (6.0f)
                 .withTrimmedRight (drawDiff ? 34.0f : 8.0f);

    auto curveR = plot;
    juce::Rectangle<float> sgR;
    if (mode == 2)
    {
        curveR = plot.withHeight (plot.getHeight() * 0.36f);
        sgR = plot.withTrimmedTop (curveR.getHeight() + 6.0f);
    }

    // Frequenz-Raster
    static const float minor[] { 30, 40, 60, 70, 80, 90, 300, 400, 600, 700, 800, 900,
                                 3000, 4000, 6000, 7000, 8000, 9000 };
    static const float major[] { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    static const char* labels[] { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

    if (gridLevel > 0 || mode != 2)
    {
        g.setColour (gridCol (col::grid.withAlpha (0.55f)));
        for (auto f : minor)
            g.drawVerticalLine ((int) xForFreq (f, plot), curveR.getY(), curveR.getBottom());
    }

    g.setFont (font (11.0f));
    for (int i = 0; i < 10; ++i)
    {
        const float x = xForFreq (major[i], plot);
        g.setColour (gridCol (col::grid.brighter (0.25f)));
        g.drawVerticalLine ((int) x, curveR.getY(), curveR.getBottom());
        g.setColour (col::dim);
        auto tr = juce::Rectangle<float> (x - 20.0f, plot.getBottom() + 3.0f, 40.0f, 14.0f);
        if (i == 0) tr = tr.withX (x - 2.0f);
        if (i == 9) tr = tr.withX (x - 38.0f);
        g.drawText (labels[i], tr, i == 0 ? juce::Justification::centredLeft
                                          : (i == 9 ? juce::Justification::centredRight : juce::Justification::centred));
    }

    // dB-Raster
    const float span = topDb - botDb;
    const int step = mode == 2 ? 24 : (span >= 70.0f ? 12 : 6);
    for (int db = (int) topDb - ((int) topDb % step + step) % step; db >= (int) botDb; db -= step)
    {
        const float y = yForDb ((float) db, curveR);
        if (y < curveR.getY() - 0.5f) continue;
        g.setColour (db == 0 ? gridCol (col::line.brighter (0.4f)) : gridCol (col::grid.brighter (0.15f)));
        g.drawHorizontalLine ((int) y, curveR.getX(), curveR.getRight());
        g.setColour (col::dim);
        g.drawText (juce::String (db), juce::Rectangle<float> (0.0f, y - 7.0f, 30.0f, 14.0f), juce::Justification::centredRight);
    }

    // Bereichsnamen
    if (mode != 2 && gridLevel > 0)
    {
        static const float edges[] { 20, 60, 250, 800, 2500, 6000, 20000 };
        static const char* names[] { "SUB", "BASS", "TIEFE MITTEN", "MITTEN", "PRÄSENZ", "BRILLANZ" };
        g.setFont (font (9.5f, true));
        for (int i = 0; i < 6; ++i)
        {
            const float x0 = xForFreq (edges[i], curveR), x1 = xForFreq (edges[i + 1], curveR);
            g.setColour (col::text.withAlpha (0.16f));
            g.drawText (u8s (names[i]), juce::Rectangle<float> (x0, curveR.getBottom() - 15.0f, x1 - x0, 12.0f),
                        juce::Justification::centred);
            if (i > 0)
            {
                g.setColour (col::text.withAlpha (0.06f));
                g.drawVerticalLine ((int) x0, curveR.getY(), curveR.getBottom());
            }
        }
    }

    if (drawDiff) computeDiff();

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (curveR.toNearestInt().expanded (0, 2));

        if (mode == 1) drawBars (g, curveR);
        else           drawCurves (g, curveR);

        if (drawDiff)
        {
            const float mid = curveR.getCentreY(), half = curveR.getHeight() * 0.5f;
            auto yD = [&] (float d) { return mid - juce::jlimit (-12.0f, 12.0f, d) / 12.0f * half; };
            g.setColour (theme().diff.withAlpha (0.35f));
            g.drawHorizontalLine ((int) mid, curveR.getX(), curveR.getRight());
            juce::Path p;
            for (int i = 0; i < (int) diff.size(); ++i)
            {
                const float x = curveR.getX() + curveR.getWidth() * (float) i / (float) (diff.size() - 1);
                if (i == 0) p.startNewSubPath (x, yD (diff[0]));
                else        p.lineTo (x, yD (diff[(size_t) i]));
            }
            strokeCurve (g, p, theme().diff, juce::jmax (1.6f, lineW));
        }

        if (peaks && showMix && mode != 1)
            drawPeakLabels (g, curveR);
    }

    if (mode == 2)
        drawSpectrogram (g, sgR);

    if (drawDiff)
    {
        const float mid = curveR.getCentreY(), half = curveR.getHeight() * 0.5f;
        g.setFont (font (10.0f, true));
        g.setColour (theme().diff);
        for (int d : { 12, 6, 0, -6, -12 })
        {
            if (mode == 2 && std::abs (d) == 6) continue;
            g.drawText ((d > 0 ? "+" : "") + juce::String (d),
                        juce::Rectangle<float> (curveR.getRight() + 3.0f, mid - (float) d / 12.0f * half - 6.0f, 30.0f, 12.0f),
                        juce::Justification::centredLeft);
        }
    }

    // Hinweise oben links
    {
        juce::String info;
        auto add = [&info] (const juce::String& t) { if (info.isNotEmpty()) info << u8s ("  ·  "); info << t; };
        if (mode == 1) add (u8s ("TERZBÄNDER"));
        if (longTerm) add (u8s ("LANGZEIT-MITTEL"));
        if (channelName.isNotEmpty()) add (channelName);
        if (drawDiff) add (u8s ("Lila = Mix minus Referenz (+ = Mix hat mehr)"));
        if (info.isNotEmpty())
        {
            g.setFont (font (11.0f, true));
            g.setColour (col::text.withAlpha (0.7f));
            g.drawText (info, curveR.reduced (8.0f, 4.0f).withHeight (16.0f), juce::Justification::centredLeft);
        }
    }

    if (! showMix && ! (showRef && refLoaded))
    {
        g.setColour (col::dim);
        g.setFont (font (14.0f));
        g.drawText (u8s ("Beide Kurven ausgeblendet – oben MIX oder REFERENZ einschalten"), curveR, juce::Justification::centred);
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

        juce::String txt = fmtFreq (f) + "  " + note;
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
        auto box = juce::Rectangle<float> (tx, curveR.getY() + 24.0f, tw, 22.0f);
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
    g.setColour (theme().mix);
    g.drawText ("MIX", juce::Rectangle<float> (cMixX, 0, colW, 16), juce::Justification::centredRight);
    g.setColour (theme().ref);
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
        value (row.ref, juce::Rectangle<float> (cRefX, y + 1, colW, row.h - 2), theme().ref.brighter (0.35f));
        y += row.h;
    }

    g.setColour (col::dim);
    g.setFont (font (10.5f));
    g.drawText (r.refLive ? u8s ("Referenz LIVE: gemessen seit dem letzten Reset")
                          : u8s ("Referenz: Integrated, Max, LRA, TP = ganze Datei"),
                juce::Rectangle<float> (10, y + 4, w - 20, 14), juce::Justification::centredLeft);

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
void MeterPanel::Bar::feed (float pk, float rmsV, int holdFrames)
{
    peak = juce::jmax (pk, peak - 1.0f);
    if (pk >= hold) { hold = pk; holdAge = 0; }
    else if (++holdAge > holdFrames) hold = juce::jmax (peak, hold - 1.2f);
    rms = rmsV;
}

void MeterPanel::tick (AnalyzerProcessor& p)
{
    auto db = [] (float v) { return juce::Decibels::gainToDecibels (v, -100.0f); };
    mL.feed (db (p.mixLevels.peakL.exchange (0.0f)), db (p.mixLevels.rmsL.load()), holdFrames);
    mR.feed (db (p.mixLevels.peakR.exchange (0.0f)), db (p.mixLevels.rmsR.load()), holdFrames);
    refRunning = p.isRefRunning();
    rL.feed (db (p.refLevels.peakL.exchange (0.0f)), refRunning ? db (p.refLevels.rmsL.load()) : -100.0f, holdFrames);
    rR.feed (db (p.refLevels.peakR.exchange (0.0f)), refRunning ? db (p.refLevels.rmsR.load()) : -100.0f, holdFrames);
    mixM = p.mixLoud.m.load();
    refM = refRunning ? p.refLoud.m.load() : -100.0f;
    mixCorr = p.mixLevels.corr.load();
    refCorr = refRunning ? p.refLevels.corr.load() : 0.0f;
    mixMax = db (p.mixLevels.maxPeak.load());
    refMax = db (p.refLevels.maxPeak.load());
}

juce::Colour MeterPanel::segColour (float db, juce::Colour c) const
{
    if (db > 0.0f) return col::red;
    if (style == 0)   // Ampel
    {
        if (db > -6.0f)  return col::warn;
        if (db > -18.0f) return juce::Colour (0xffe8c547);
        return juce::Colour (0xff4fc36b);
    }
    const float t = juce::jlimit (0.0f, 1.0f, (db - floorDb) / -floorDb);
    return c.darker (0.35f).interpolatedWith (c.brighter (0.55f), t);
}

void MeterPanel::drawMeter (juce::Graphics& g, juce::Rectangle<float> r, const Bar& b, juce::Colour c) const
{
    const float top = r.getY(), bottom = r.getBottom(), hgt = r.getHeight(), hi = 3.0f;
    auto yOf  = [&] (float db) { return top + (1.0f - (juce::jlimit (floorDb, hi, db) - floorDb) / (hi - floorDb)) * hgt; };
    auto dbAt = [&] (float y)  { return floorDb + (1.0f - (y - top) / hgt) * (hi - floorDb); };

    g.setColour (col::plot);
    g.fillRect (r);

    if (style == 2)   // Balken glatt
    {
        if (b.rms > floorDb)
        {
            const float yr = yOf (b.rms);
            juce::ColourGradient grad (c.darker (0.3f), 0, bottom, c.brighter (0.2f), 0, top, false);
            g.setGradientFill (grad);
            g.fillRect (juce::Rectangle<float> (r.getX(), yr, r.getWidth(), bottom - yr));
        }
        if (b.peak > floorDb)
        {
            const float yp = yOf (b.peak);
            g.setColour (c.withAlpha (0.25f));
            g.fillRect (juce::Rectangle<float> (r.getX(), yp, r.getWidth(), bottom - yp));
            g.setColour (b.peak > 0.0f ? col::red : c.brighter (0.5f).withAlpha (0.9f));
            g.fillRect (juce::Rectangle<float> (r.getX(), yp, r.getWidth(), juce::jmin (3.0f, bottom - yp)));
        }
    }
    else              // LED-Segmente
    {
        const float segH = 3.0f, gap = 1.0f;
        for (float y = bottom - segH; y >= top - 0.5f; y -= segH + gap)
        {
            const float sdb = dbAt (y + segH * 0.5f);
            const auto sc = segColour (sdb, c);
            if (sdb <= b.rms)       g.setColour (sc);
            else if (sdb <= b.peak) g.setColour (sc.withAlpha (0.5f));
            else                    g.setColour (sc.withAlpha (0.07f));
            g.fillRect (r.getX(), y, r.getWidth(), segH);
        }
    }

    if (b.hold > floorDb)
    {
        g.setColour (b.hold > 0.0f ? col::red : col::text);
        g.fillRect (juce::Rectangle<float> (r.getX(), yOf (b.hold) - 1.0f, r.getWidth(), 2.0f));
    }
}

void MeterPanel::paint (juce::Graphics& g)
{
    const float top = 34.0f, bottom = (float) getHeight() - 50.0f, hgt = bottom - top, hi = 3.0f;
    auto yOf = [&] (float db) { return top + (1.0f - (juce::jlimit (floorDb, hi, db) - floorDb) / (hi - floorDb)) * hgt; };

    auto group = [&] (float gx, const char* title, const Bar& l, const Bar& r, float m, float mx, float corr,
                      juce::Colour c, bool active)
    {
        g.setFont (font (12.0f, true));
        g.setColour (c);
        g.drawText (title, juce::Rectangle<float> (gx, 0, 120, 14), juce::Justification::centredLeft);
        g.setFont (font (11.0f));
        g.setColour (mx > 0.0f ? col::red : col::text.withAlpha (active ? 0.9f : 0.4f));
        g.drawText (u8s ("Peak ") + fmt (mx), juce::Rectangle<float> (gx, 15, 70, 14), juce::Justification::centredLeft);

        drawMeter (g, { gx, top, 22.0f, hgt }, l, c);
        drawMeter (g, { gx + 26.0f, top, 22.0f, hgt }, r, c);
        Bar lufs;
        lufs.rms = lufs.peak = m;
        drawMeter (g, { gx + 58.0f, top, 24.0f, hgt }, lufs, c);

        g.setFont (font (10.0f, true));
        g.setColour (col::dim);
        g.drawText ("L", juce::Rectangle<float> (gx, bottom + 2, 22, 12), juce::Justification::centred);
        g.drawText ("R", juce::Rectangle<float> (gx + 26.0f, bottom + 2, 22, 12), juce::Justification::centred);
        g.drawText ("LUFS M", juce::Rectangle<float> (gx + 48.0f, bottom + 2, 44, 12), juce::Justification::centred);
        g.setColour (col::text.withAlpha (active ? 0.9f : 0.4f));
        g.drawText (fmt (m), juce::Rectangle<float> (gx + 70.0f, 15, 42, 14), juce::Justification::centredRight);

        const float cy = bottom + 22.0f;
        auto cr = juce::Rectangle<float> (gx, cy, 130.0f, 8.0f);
        juce::ColourGradient cg (col::red.withAlpha (0.35f), cr.getX(), 0, col::good.withAlpha (0.35f), cr.getRight(), 0, false);
        cg.addColour (0.5, col::plot);
        g.setGradientFill (cg);
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

    group (12.0f, "MIX", mL, mR, mixM, mixMax, mixCorr, theme().mix, true);
    group ((float) getWidth() - 142.0f, "REFERENZ", rL, rR, refM, refRunning ? refMax : -100.0f, refCorr, theme().ref, refRunning);

    static const std::vector<int> s60 { 0, -6, -12, -18, -24, -30, -36, -48, -60 };
    static const std::vector<int> s48 { 0, -3, -6, -9, -12, -18, -24, -30, -36, -48 };
    static const std::vector<int> s24 { 0, -3, -6, -9, -12, -15, -18, -21, -24 };
    const auto& scale = floorDb <= -59.0f ? s60 : (floorDb <= -47.0f ? s48 : s24);

    const float sx = (float) getWidth() * 0.5f - 22.0f;
    g.setFont (font (10.0f));
    for (int db : scale)
    {
        const float y = yOf ((float) db);
        g.setColour (db == 0 ? col::red.withAlpha (0.7f) : col::line);
        g.drawHorizontalLine ((int) y, sx, sx + 6.0f);
        g.setColour (col::dim);
        g.drawText (juce::String (db), juce::Rectangle<float> (sx + 8.0f, y - 6.0f, 30.0f, 12.0f), juce::Justification::centredLeft);
    }
    g.setFont (font (9.0f));
    g.setColour (col::dim.withAlpha (0.7f));
    g.drawText (u8s ("Klick = Reset"), juce::Rectangle<float> ((float) getWidth() * 0.5f - 36.0f, bottom + 3.0f, 72.0f, 11.0f),
                juce::Justification::centred);
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

    drawLine (ref, theme().ref, 1.6f);
    drawLine (mix, theme().mix, 1.8f);
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
    if (showRef && refRunning) dots (rL, rR, theme().ref);
    dots (mL, mR, theme().mix);

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
        g.setColour (theme().mix);
        g.drawText (v1, tx.withTrimmedRight (52.0f), juce::Justification::centredRight);
        g.setColour (theme().ref);
        g.drawText (v2, tx, juce::Justification::centredRight);
        tx.translate (0.0f, 20.0f);
    };

    g.setFont (font (10.5f, true));
    g.setColour (theme().mix);
    g.drawText ("MIX", tx.withTrimmedRight (52.0f), juce::Justification::centredRight);
    g.setColour (theme().ref);
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
        g.drawFittedText (refLive ? u8s ("LIVE-Referenz: Noch kein Sidechain-Signal. Wähle in deiner DAW eine Spur oder einen "
                                         "Eingang als Sidechain für den K-54 Analyzer.")
                                  : u8s ("Lade unten eine Referenz – dann zeigt dir dieses Feld, wo dein Mix mehr oder weniger "
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
    juce::String meas = u8s ("Mix ") + mmss (mixSeconds) + (refLive ? u8s (" · Ref: live") : u8s (" · Ref: ganzer Song"));
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
            const auto barR = juce::Rectangle<float> (bx, juce::jmin (y0, y1), bw, juce::jmax (2.0f, std::abs (y1 - y0)));
            if (colorfulUi())
            {
                const auto bc = rainbow ((float) i / (float) (SpecMath::numBands - 1));
                juce::ColourGradient bg (bc.brighter (0.35f), 0.0f, barR.getY(), bc.darker (0.25f), 0.0f, barR.getBottom(), false);
                g.setGradientFill (bg);
                g.fillRoundedRectangle (barR, 3.0f);
                g.setColour (c);
                g.drawRoundedRectangle (barR, 3.0f, 1.2f);
            }
            else
            {
                g.setColour (c.withAlpha (0.85f));
                g.fillRoundedRectangle (barR, 2.0f);
            }
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

    if (proc.isLiveSlot())
    {
        const bool conn = proc.isLiveConnected();
        const bool sig = conn && proc.hasLiveSignal();
        g.setColour (sig ? theme().ref : col::line.brighter (0.3f));
        g.drawRoundedRectangle (b.reduced (1.0f), 4.0f, sig ? 2.0f : 1.2f);

        auto r = b.reduced (14.0f, 8.0f);
        g.setFont (font (15.0f, true));
        g.setColour (sig ? theme().ref : col::text);
        const float pulse = sig ? 0.6f + 0.4f * (float) std::sin (juce::Time::getMillisecondCounter() * 0.006) : 0.25f;
        g.setColour ((sig ? col::red : col::dim).withAlpha (pulse));
        g.fillEllipse (r.getX(), r.getY() + 5.0f, 10.0f, 10.0f);
        g.setColour (sig ? theme().ref : col::text);
        g.drawText (u8s ("LIVE-REFERENZ  ·  Sidechain"), r.withTrimmedLeft (16.0f).withHeight (20.0f), juce::Justification::centredLeft);

        g.setFont (font (12.5f, true));
        g.setColour (sig ? col::good : (conn ? theme().mix.brighter (0.4f) : col::warn));
        g.drawText (sig ? u8s ("Signal da – wird live gemessen, nichts wird aufgenommen")
                        : (conn ? u8s ("Sidechain verbunden – warte auf Signal") : u8s ("Kein Sidechain verbunden")),
                    r.withHeight (20.0f), juce::Justification::centredRight);

        g.setFont (font (12.0f));
        g.setColour (col::text.withAlpha (0.8f));
        g.drawFittedText (u8s ("Logic: oben rechts im Plugin-Fenster bei „Side Chain“ eine Spur, einen Bus oder einen Eingang wählen.\n"
                               "Ableton: im Gerät den Sidechain-Bereich aufklappen und die Quelle wählen.  "
                               "Cubase: im Plugin-Fenster Sidechain aktivieren und eine Spur dorthin senden.\n"
                               "Andere Programme (z. B. Streaming-App) über ein virtuelles Audiogerät wie BlackHole in die DAW holen – "
                               "nur live messen, nicht aufnehmen."),
                          r.withTrimmedTop (26.0f).toNearestInt(), juce::Justification::topLeft, 4, 1.0f);
        return;
    }

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
        g.setColour (dragHover ? theme().mix.brighter (0.4f) : col::text.withAlpha (0.75f));
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
        g.setColour (px < playX ? theme().ref.withAlpha (0.85f) : theme().ref.withAlpha (0.38f));
        g.fillRect (px, mid - h, 1.0f, juce::jmax (1.0f, 2.0f * h));
    }

    g.setColour (theme().mix.brighter (0.4f));
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
// ANSICHT
ColourSwatch::~ColourSwatch()
{
    if (selector != nullptr)
    {
        selector->removeChangeListener (this);
        if (auto* box = selector->findParentComponentOfClass<juce::CallOutBox>())
            box->dismiss();
    }
}

void ColourSwatch::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (colour);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (isMouseOver() ? col::text : col::line.brighter (0.4f));
    g.drawRoundedRectangle (r, 4.0f, 1.2f);
    g.setColour (colour.getPerceivedBrightness() > 0.55f ? juce::Colour (0xff111111) : juce::Colours::white);
    g.setFont (font (11.0f, true));
    g.drawText (letter, r, juce::Justification::centred);
}

void ColourSwatch::mouseDown (const juce::MouseEvent&)
{
    auto sel = std::make_unique<juce::ColourSelector> (juce::ColourSelector::showColourAtTop
                                                       | juce::ColourSelector::showSliders
                                                       | juce::ColourSelector::showColourspace);
    sel->setCurrentColour (colour, juce::dontSendNotification);
    sel->setColour (juce::ColourSelector::backgroundColourId, col::panel2);
    sel->setSize (250, 290);
    sel->addChangeListener (this);
    selector = sel.get();

    juce::Component* parent = findParentComponentOfClass<juce::AudioProcessorEditor>();
    if (parent == nullptr) parent = getTopLevelComponent();
    juce::CallOutBox::launchAsynchronously (std::move (sel), parent->getLocalArea (this, getLocalBounds()), parent);
}

void ColourSwatch::changeListenerCallback (juce::ChangeBroadcaster* src)
{
    if (auto* s = dynamic_cast<juce::ColourSelector*> (src))
    {
        colour = s->getCurrentColour().withAlpha (1.0f);
        repaint();
        if (onChange) onChange (colour);
    }
}

//==============================================================================
ViewPanel::ViewPanel()
{
    auto combo = [this] (juce::ComboBox& cb, std::initializer_list<const char*> items)
    {
        int id = 1;
        for (auto* t : items) cb.addItem (u8s (t), id++);
        cb.onChange = [this] { changed(); };
        addAndMakeVisible (cb);
    };
    auto toggle = [this] (juce::TextButton& b)
    {
        b.setClickingTogglesState (true);
        b.onClick = [this, &b] { b.setButtonText (b.getToggleState() ? "AN" : "AUS"); changed(); };
        addAndMakeVisible (b);
    };

    combo (cbMode,  { "Kurve", "Balken (Terzbänder)", "Spektrogramm" });
    combo (cbSpecColor, { "Themenfarbe", "Regenbogen" });
    combo (cbRange, { "96 dB  (+6 bis -90)", "78 dB  (+6 bis -72)", "66 dB  (+6 bis -60)", "48 dB  (0 bis -48)" });
    combo (cbLine,  { "Dünn", "Normal", "Dick" });
    combo (cbFill,  { "Aus", "Leicht", "Kräftig" });
    combo (cbGrid,  { "Dezent", "Normal", "Kräftig" });
    cbTheme.onChange = [this] { changed(); };
    for (int i = 0; i < ViewStyle::numThemes; ++i) cbTheme.addItem (u8s (themeName (i)), i + 1);
    addAndMakeVisible (cbTheme);
    combo (cbSurface, { "Schlicht", "Farbig" });
    combo (cbAccent, { "Bunt (je Feld)", "Eigene Farbe (A)" });
    combo (cbMeter, { "LED Ampel", "LED Themenfarbe", "Balken glatt" });
    combo (cbMeterRange, { "60 dB", "48 dB", "24 dB (Mastering)" });
    combo (cbHold,  { "1 Sekunde", "3 Sekunden", "Bis Klick / Reset" });
    for (auto* b : { &btnGlow, &btnPeaks, &btnShowTonal, &btnShowHist, &btnShowLoud, &btnShowMeter })
        toggle (*b);

    // eigene Farben
    const char* letters[] { "M", "R", "D", "A" };
    const char* tips[] { "Farbe Mix", "Farbe Referenz", "Farbe Differenzkurve", "Akzentfarbe der Felder" };
    for (int i = 0; i < 4; ++i)
    {
        auto& sw = swatches[(size_t) i];
        sw.letter = letters[i];
        sw.setTooltip (u8s (tips[i]));
        sw.onChange = [this, i] (juce::Colour c)
        {
            const auto argb = c.getARGB();
            if (i == 0) current.customMix = argb;
            if (i == 1) current.customRef = argb;
            if (i == 2) current.customDiff = argb;
            if (i == 3) { current.customAccent = argb; cbAccent.setSelectedItemIndex (1, juce::dontSendNotification); }
            else        cbTheme.setSelectedItemIndex (ViewStyle::customTheme, juce::dontSendNotification);
            changed();
        };
        swatchRow.addAndMakeVisible (sw);
    }
    addAndMakeVisible (swatchRow);

    // Ansicht-Presets
    cbPreset.setTextWhenNothingSelected (u8s ("– wählen –"));
    cbPreset.onChange = [this]
    {
        const int idx = cbPreset.getSelectedId() - 1;
        if (idx >= 0 && idx < (int) presetList.size())
        {
            setStyle (presetList[(size_t) idx].second);
            changed();
            cbPreset.setSelectedId (idx + 1, juce::dontSendNotification);
        }
        btnDelete.setEnabled (idx >= numBuiltIn);
    };
    addAndMakeVisible (cbPreset);
    btnSave.setButtonText (u8s ("SPEICHERN …"));
    btnSave.onClick = [this] { askPresetName(); };
    btnDelete.setButtonText (u8s ("LÖSCHEN"));
    btnDelete.onClick = [this]
    {
        const int idx = cbPreset.getSelectedId() - 1;
        if (settings != nullptr && idx >= numBuiltIn && idx < (int) presetList.size())
        {
            ViewPresets::removeUser (*settings, presetList[(size_t) idx].first);
            refreshPresets();
            if (onPresetsChanged) onPresetsChanged();
        }
    };
    presetBtnRow.addAndMakeVisible (btnSave);
    presetBtnRow.addAndMakeVisible (btnDelete);
    addAndMakeVisible (presetBtnRow);

    btnDefaults.setButtonText ("STANDARD");
    btnDefaults.onClick = [this] { setStyle (ViewStyle {}); changed(); };
    addAndMakeVisible (btnDefaults);
    btnClose.setButtonText ("FERTIG");
    btnClose.setToggleState (true, juce::dontSendNotification);
    btnClose.onClick = [this] { setVisible (false); };
    addAndMakeVisible (btnClose);

    cols[0] = { { "#SPEKTRUM" }, { "Darstellung", &cbMode }, { "Farbe", &cbSpecColor }, { "dB-Bereich", &cbRange },
                { "Linienstärke", &cbLine }, { "Füllung", &cbFill }, { "Raster", &cbGrid }, { "Leuchten", &btnGlow },
                { "Spitzen zeigen", &btnPeaks } };
    cols[1] = { { "#FARBEN" }, { "Farbschema", &cbTheme }, { "Eigene Farben", &swatchRow }, { "Oberfläche", &cbSurface },
                { "Feld-Farben", &cbAccent }, { "#PEGEL" }, { "Stil", &cbMeter }, { "Bereich", &cbMeterRange },
                { "Peak-Hold", &cbHold } };
    cols[2] = { { "#FELDER ZEIGEN" }, { "Tonale Balance", &btnShowTonal }, { "Verlauf / Stereo", &btnShowHist },
                { "Lautheit", &btnShowLoud }, { "Pegel", &btnShowMeter }, { "#GESPEICHERTE ANSICHTEN" },
                { "Ansicht", &cbPreset }, { "", &presetBtnRow } };
}

void ViewPanel::setSettings (juce::PropertiesFile* s)
{
    settings = s;
    refreshPresets();
}

void ViewPanel::refreshPresets()
{
    presetList = ViewPresets::builtIn();
    numBuiltIn = (int) presetList.size();
    if (settings != nullptr)
        for (auto& p : ViewPresets::user (*settings))
            presetList.push_back (p);

    cbPreset.clear (juce::dontSendNotification);
    for (int i = 0; i < (int) presetList.size(); ++i)
    {
        if (i == numBuiltIn) cbPreset.addSectionHeading (u8s ("Eigene"));
        cbPreset.addItem (presetList[(size_t) i].first, i + 1);
    }
    selectMatchingPreset();
}

void ViewPanel::selectMatchingPreset()
{
    int match = 0;
    for (int i = (int) presetList.size() - 1; i >= 0; --i)
        if (presetList[(size_t) i].second == current) match = i + 1;
    cbPreset.setSelectedId (match, juce::dontSendNotification);
    btnDelete.setEnabled (match - 1 >= numBuiltIn);
}

void ViewPanel::askPresetName()
{
    auto* aw = new juce::AlertWindow (u8s ("Ansicht speichern"), u8s ("Name für diese Ansicht:"), juce::MessageBoxIconType::NoIcon, this);
    const int idx = cbPreset.getSelectedId() - 1;
    aw->addTextEditor ("name", idx >= numBuiltIn && idx < (int) presetList.size() ? presetList[(size_t) idx].first : u8s ("Meine Ansicht"));
    aw->addButton (u8s ("Speichern"), 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton (u8s ("Abbrechen"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<ViewPanel> safe (this);
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, aw] (int result)
    {
        if (safe == nullptr || result != 1) return;
        const auto name = aw->getTextEditorContents ("name").trim();
        if (name.isNotEmpty()) safe->savePreset (name);
    }), true);
}

void ViewPanel::savePreset (const juce::String& name)
{
    if (settings == nullptr) return;
    ViewPresets::saveUser (*settings, name, getStyle());
    refreshPresets();
    if (onPresetsChanged) onPresetsChanged();
}

void ViewPanel::setStyle (const ViewStyle& s)
{
    loading = true;
    current = s;
    cbMode.setSelectedItemIndex (s.specMode, juce::dontSendNotification);
    cbSpecColor.setSelectedItemIndex (s.specColor, juce::dontSendNotification);
    cbRange.setSelectedItemIndex (s.range, juce::dontSendNotification);
    cbLine.setSelectedItemIndex (s.line, juce::dontSendNotification);
    cbFill.setSelectedItemIndex (s.fill, juce::dontSendNotification);
    cbGrid.setSelectedItemIndex (s.grid, juce::dontSendNotification);
    cbTheme.setSelectedItemIndex (s.theme, juce::dontSendNotification);
    cbSurface.setSelectedItemIndex (s.colorful ? 1 : 0, juce::dontSendNotification);
    cbAccent.setSelectedItemIndex (s.accentMode, juce::dontSendNotification);
    cbMeter.setSelectedItemIndex (s.meterStyle, juce::dontSendNotification);
    cbMeterRange.setSelectedItemIndex (s.meterRange, juce::dontSendNotification);
    cbHold.setSelectedItemIndex (s.hold, juce::dontSendNotification);

    auto setT = [] (juce::TextButton& b, bool on) { b.setToggleState (on, juce::dontSendNotification); b.setButtonText (on ? "AN" : "AUS"); };
    setT (btnGlow, s.glow);
    setT (btnPeaks, s.peaks);
    setT (btnShowTonal, s.showTonal);
    setT (btnShowHist, s.showHist);
    setT (btnShowLoud, s.showLoud);
    setT (btnShowMeter, s.showMeter);

    swatches[0].colour = juce::Colour (s.customMix);
    swatches[1].colour = juce::Colour (s.customRef);
    swatches[2].colour = juce::Colour (s.customDiff);
    swatches[3].colour = juce::Colour (s.customAccent);
    for (auto& sw : swatches) sw.repaint();
    loading = false;
    selectMatchingPreset();
    repaint();
}

ViewStyle ViewPanel::getStyle() const
{
    ViewStyle s = current;
    s.specMode   = juce::jmax (0, cbMode.getSelectedItemIndex());
    s.specColor  = juce::jmax (0, cbSpecColor.getSelectedItemIndex());
    s.range      = juce::jmax (0, cbRange.getSelectedItemIndex());
    s.line       = juce::jmax (0, cbLine.getSelectedItemIndex());
    s.fill       = juce::jmax (0, cbFill.getSelectedItemIndex());
    s.grid       = juce::jmax (0, cbGrid.getSelectedItemIndex());
    s.theme      = juce::jmax (0, cbTheme.getSelectedItemIndex());
    s.colorful   = cbSurface.getSelectedItemIndex() != 0;
    s.accentMode = juce::jmax (0, cbAccent.getSelectedItemIndex());
    s.meterStyle = juce::jmax (0, cbMeter.getSelectedItemIndex());
    s.meterRange = juce::jmax (0, cbMeterRange.getSelectedItemIndex());
    s.hold       = juce::jmax (0, cbHold.getSelectedItemIndex());
    s.glow       = btnGlow.getToggleState();
    s.peaks      = btnPeaks.getToggleState();
    s.showTonal  = btnShowTonal.getToggleState();
    s.showHist   = btnShowHist.getToggleState();
    s.showLoud   = btnShowLoud.getToggleState();
    s.showMeter  = btnShowMeter.getToggleState();
    return s;
}

void ViewPanel::changed()
{
    if (loading) return;
    current = getStyle();
    selectMatchingPreset();
    repaint();
    if (onChange) onChange (current);
}

void ViewPanel::resized()
{
    box = getLocalBounds().toFloat().withSizeKeepingCentre (1010.0f, 470.0f);
    labels.clear();
    headers.clear();

    for (int c = 0; c < 3; ++c)
    {
        const float x = box.getX() + 28.0f + (float) c * 330.0f;
        float y = box.getY() + 66.0f;
        for (auto& r : cols[(size_t) c])
        {
            if (r.comp == nullptr)
                headers.push_back ({ u8s (r.label + 1), { x, y + 6.0f, 300.0f, 20.0f } });
            else
            {
                labels.push_back ({ u8s (r.label), { x, y + 2.0f, 120.0f, 26.0f } });
                const bool isToggle = dynamic_cast<juce::TextButton*> (r.comp) != nullptr;
                r.comp->setBounds ((int) x + 124, (int) y + 3, isToggle ? 70 : 176, 26);
            }
            y += 34.0f;
        }
    }

    for (int i = 0; i < 4; ++i)
        swatches[(size_t) i].setBounds (i * 44, 0, 38, 26);
    btnSave.setBounds (0, 0, 104, 26);
    btnDelete.setBounds (110, 0, 66, 26);

    btnDefaults.setBounds ((int) box.getX() + 28, (int) box.getBottom() - 46, 110, 30);
    btnClose.setBounds ((int) box.getRight() - 138, (int) box.getBottom() - 46, 110, 30);
}

void ViewPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.6f));
    g.setColour (col::panel);
    g.fillRoundedRectangle (box, 8.0f);
    if (colorfulUi())
    {
        g.setGradientFill (rainbowGradient (box.getX() + 10.0f, box.getRight() - 10.0f, 0.0f, 0.9f));
        g.fillRect (box.getX() + 10.0f, box.getY(), box.getWidth() - 20.0f, 2.0f);
    }
    g.setColour (col::gold);
    g.drawRoundedRectangle (box, 8.0f, 1.5f);

    g.setFont (font (20.0f, true));
    g.drawText ("ANSICHT", box.reduced (28.0f, 16.0f).withHeight (24.0f), juce::Justification::centredLeft);
    g.setFont (font (11.5f));
    g.setColour (col::dim);
    g.drawText (u8s ("Wirkt sofort · gilt für alle Projekte · eigene Ansichten rechts speichern"),
                box.reduced (28.0f, 16.0f).withHeight (24.0f), juce::Justification::centredRight);

    g.setColour (col::line);
    for (int c = 1; c < 3; ++c)
        g.drawVerticalLine ((int) (box.getX() + 28.0f + (float) c * 330.0f - 14.0f), box.getY() + 70.0f, box.getBottom() - 62.0f);

    for (auto& h : headers)
    {
        g.setColour (col::goldHi);
        g.setFont (font (12.0f, true));
        g.drawText (h.first, h.second, juce::Justification::centredLeft);
    }
    for (auto& l : labels)
    {
        g.setColour (col::text.withAlpha (0.85f));
        g.setFont (font (12.5f));
        g.drawText (l.first, l.second, juce::Justification::centredLeft);
    }
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
    add ("ANSICHT  ", 13.5f, col::goldHi, true);
    add ("Darstellung als Kurve, Terzband-Balken oder Spektrogramm, dB-Bereich, Linien, Füllung, Leuchten, Spitzen-Markierung, "
         "5 Farbschemata, Pegel als LED oder Balken mit wählbarem Bereich und Peak-Hold, Layout \"Großes Spektrum\". "
         "Klick auf die Pegel setzt die Spitzen zurück.\n\n", 13.5f, col::text);
    add ("Schnellumschalter  ", 13.5f, col::goldHi, true);
    add ("Rechts im Spektrum-Feld: gespeicherte Ansicht wählen, KURVE / BALKEN / SPEKTRO und FARBIG. Unter ANSICHT: eigene Farben "
         "(Mix, Referenz, Differenz, Akzent), Felder ein- und ausblenden und eigene Ansichten speichern.\n\n", 13.5f, col::text);
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
    add ("LIVE-Referenz  ", 13.5f, col::goldHi, true);
    add ("Slot LIVE misst, was am Sidechain-Eingang ankommt – eine andere Spur, ein externer Player oder eine App über "
         "ein virtuelles Audiogerät. Alles läuft wie bei einer Datei (A/B, Angleich, Tonale Balance), es wird nichts aufgenommen.\n\n",
         13.5f, col::text);
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
    btnLive.setButtonText ("LIVE");
    setAccent (btnLive, col::ref);
    btnLive.setTooltip (u8s ("Live-Referenz über den Sidechain-Eingang"));
    btnLive.onClick = [this] { proc.setSlot (AnalyzerProcessor::liveSlot); };
    addAndMakeVisible (btnLive);
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

    btnView.setButtonText ("ANSICHT");
    btnView.setTooltip (u8s ("Darstellung, Farben, Pegel und Layout einstellen"));
    btnView.onClick = [this] { showView (true); };
    addAndMakeVisible (btnView);
    viewPanel.onChange = [this] (const ViewStyle& st) { applyStyle (st, true); };
    viewPanel.onPresetsChanged = [this] { refreshQuickPresets(); };
    viewPanel.setSettings (&proc.getSettings());
    addChildComponent (viewPanel);

    // Schnellumschalter im Spektrum-Feld
    auto quick = [this] (juce::TextButton& b, const char* text, const char* tip, std::function<void (ViewStyle&)> fn)
    {
        b.setButtonText (u8s (text));
        b.setTooltip (u8s (tip));
        setAccent (b, col::goldHi);
        b.onClick = [this, fn]
        {
            auto st = style;
            fn (st);
            applyStyle (st, true);
        };
        addAndMakeVisible (b);
    };
    quick (btnQCurve, "KURVE", "Spektrum als Kurve", [] (ViewStyle& st) { st.specMode = 0; });
    quick (btnQBars, "BALKEN", "Spektrum als Terzband-Balken", [] (ViewStyle& st) { st.specMode = 1; });
    quick (btnQSpectro, "SPEKTRO", "Spektrogramm mit Kurve", [] (ViewStyle& st) { st.specMode = 2; });
    quick (btnQColor, "FARBIG", "Farbig (Regenbogen) oder schlicht",
           [] (ViewStyle& st) { const bool on = ! (st.colorful && st.specColor == 1); st.colorful = on; st.specColor = on ? 1 : 0; });
    cbQuickPreset.setTextWhenNothingSelected (u8s ("Eigene Ansicht"));
    cbQuickPreset.setTooltip (u8s ("Gespeicherte Ansicht wählen"));
    cbQuickPreset.onChange = [this]
    {
        const int id = cbQuickPreset.getSelectedId();
        if (id == 999) { refreshQuickPresets(); showView (true); return; }
        auto list = ViewPresets::builtIn();
        for (auto& up : ViewPresets::user (proc.getSettings())) list.push_back (up);
        if (id >= 1 && id <= (int) list.size())
            applyStyle (list[(size_t) id - 1].second, true);
    };
    addAndMakeVisible (cbQuickPreset);

    meters.onClick = [this]
    {
        proc.mixLevels.maxPeak.store (0.0f);
        proc.refLevels.maxPeak.store (0.0f);
        meters.resetHolds();
    };

    pullL.resize (1 << 17);
    pullR.resize (1 << 17);
    mono.resize (1 << 17);
    pro = proc.getProMode();
    setSize (W, H);
    style.load (proc.getSettings());
    applyStyle (style, false);
}

void Content::applyStyle (const ViewStyle& s, bool save)
{
    style = s;
    if (save) style.save (proc.getSettings());
    customThemeColours() = { juce::Colour (s.customMix), juce::Colour (s.customRef), juce::Colour (s.customDiff) };
    currentThemeIndex() = s.theme;
    colorfulUi() = s.colorful;
    spectrum.setStyle (s);
    meters.style = s.meterStyle;
    meters.floorDb = s.meterFloor();
    meters.holdFrames = s.holdFrames();
    bigSpec = ! s.bottomRow();

    btnQCurve.setToggleState (s.specMode == 0, juce::dontSendNotification);
    btnQBars.setToggleState (s.specMode == 1, juce::dontSendNotification);
    btnQSpectro.setToggleState (s.specMode == 2, juce::dontSendNotification);
    btnQColor.setToggleState (s.colorful && s.specColor == 1, juce::dontSendNotification);
    if (viewPanel.isVisible()) viewPanel.setStyle (s);
    refreshQuickPresets();

    const auto& th = theme();
    for (auto* b : { &btnA, &btnShowMix }) setAccent (*b, th.mix);
    for (auto* b : { &btnB, &btnShowRef, &btnMatch, &btnLive }) setAccent (*b, th.ref);
    for (auto& b : btnSlot) setAccent (b, th.ref);
    setAccent (btnShowDiff, th.diff);

    updateVisibility();
    resized();
    repaint();
}

juce::Colour Content::accent (juce::Colour def) const
{
    return style.accentMode == 1 ? juce::Colour (style.customAccent) : def;
}

void Content::refreshQuickPresets()
{
    cbQuickPreset.clear (juce::dontSendNotification);
    auto list = ViewPresets::builtIn();
    const int numBuiltIn = (int) list.size();
    for (auto& p : ViewPresets::user (proc.getSettings())) list.push_back (p);
    int match = 0;
    for (int i = 0; i < (int) list.size(); ++i)
    {
        if (i == numBuiltIn) cbQuickPreset.addSectionHeading (u8s ("Eigene"));
        cbQuickPreset.addItem (list[(size_t) i].first, i + 1);
        if (match == 0 && list[(size_t) i].second == style) match = i + 1;
    }
    cbQuickPreset.addSeparator();
    cbQuickPreset.addItem (u8s ("Einstellen …"), 999);
    cbQuickPreset.setSelectedId (match, juce::dontSendNotification);
}

void Content::showView (bool s)
{
    if (s) viewPanel.setStyle (style);
    viewPanel.setVisible (s);
    if (s) viewPanel.toFront (false);
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
    scope.setVisible (showScope && style.showHist);
    history.setVisible (! showScope && style.showHist);
    tonal.setVisible (style.showTonal);
    btnTabHist.setVisible (pro && style.showHist);
    btnTabScope.setVisible (pro && style.showHist);
    loud.setVisible (style.showLoud);
    btnAuto.setVisible (style.showLoud);
    meters.setVisible (style.showMeter);
    btnTabHist.setToggleState (! showScope, juce::dontSendNotification);
    btnTabScope.setToggleState (showScope, juce::dontSendNotification);
    loud.pro = pro;
    tonal.pro = pro;
    loud.repaint();
}

void Content::setAB (bool b)
{
    if (b && ! proc.hasReference())
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

    if (proc.isLiveSlot() && r.refLoaded)
    {
        s << "\n" << u8s ("REFERENZ  (LIVE über Sidechain, gemessen seit Reset)\n");
        s << line ("Integrated", fmt (r.refI) + " LUFS");
        s << line ("LRA", fmt (r.refLra) + " LU");
        s << line ("True Peak max", fmt (r.refTp) + " dBTP");
        s << line ("Short-Term max", fmt (r.refSMax) + " LUFS");
        if (tonal.mixValid && tonal.refValid)
        {
            s << "\n" << u8s ("TONALE BALANCE  (Mix minus Referenz, + = Mix hat mehr)\n");
            for (int i = 0; i < SpecMath::numBands; ++i)
                s << line (u8s (SpecMath::bandName (i)) + " (" + u8s (SpecMath::bandRange (i)) + ")",
                           fmt (tonal.mix[(size_t) i] - tonal.ref[(size_t) i], 1, true) + " dB");
        }
    }
    else if (auto* ref = proc.getRef())
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
    const int leftW = style.rightColumn() ? 880 : 1256;
    rSpec   = { 12, 66, leftW, style.bottomRow() ? 400 : 580 };
    if (style.showTonal && style.showHist)
    {
        const int tw = (leftW - 8) * 520 / 872;
        rTonal = { 12, 474, tw, 172 };
        rHist  = { 20 + tw, 474, leftW - tw - 8, 172 };
    }
    else
    {
        rTonal = { 12, 474, leftW, 172 };
        rHist  = { 12, 474, leftW, 172 };
    }
    if (style.showLoud && style.showMeter)
    {
        rLoud  = { 904, 66, 364, 330 };
        rMeter = { 904, 404, 364, 242 };
    }
    else
    {
        rLoud  = { 904, 66, 364, 580 };
        rMeter = { 904, 66, 364, 580 };
    }
    rRef    = { 12, 654, 1256, 134 };

    btnSimple.setBounds (258, 17, 76, 26);
    btnView.setBounds (396, 17, 64, 26);
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

    {
        const int qr = rSpec.getRight() - 10;
        btnQColor.setBounds (qr - 62, sy, 62, 24);
        btnQSpectro.setBounds (qr - 62 - 8 - 66, sy, 66, 24);
        btnQBars.setBounds (qr - 62 - 8 - 66 - 2 - 62, sy, 62, 24);
        btnQCurve.setBounds (qr - 62 - 8 - 66 - 2 - 62 - 2 - 58, sy, 58, 24);
        cbQuickPreset.setBounds (btnQCurve.getX() - 8 - 150, sy, 150, 24);
    }

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
        btnSlot[(size_t) s].setBounds (rx + 12 + s * 26, ry + 32, 23, 28);
    btnLive.setBounds (rx + 116, ry + 32, 44, 28);
    btnLoad.setBounds (rx + 12, ry + 66, 148, 28);
    btnClear.setBounds (rx + 12, ry + 100, 148, 24);
    wave.setBounds (rx + 168, ry + 10, 712, 114);

    const int x0 = 904;
    btnSync.setBounds (x0, ry + 8, 150, 26);
    btnPlay.setBounds (x0 + 158, ry + 8, 92, 26);
    lbGain.setBounds (x0, ry + 38, 40, 26);
    sGain.setBounds (x0 + 40, ry + 38, 324, 26);
    btnMatch.setBounds (x0, ry + 68, 190, 26);
    lbMonitor.setBounds (x0, ry + 100, 64, 24);
    cbMonitor.setBounds (x0 + 66, ry + 100, 96, 24);
    cbBand.setBounds (x0 + 168, ry + 100, 186, 24);

    help.setBounds (getLocalBounds());
    viewPanel.setBounds (getLocalBounds());
}

void Content::panel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, juce::Colour accent) const
{
    const auto rf = r.toFloat();
    if (! colorfulUi())
    {
        g.setColour (col::panel);
        g.fillRoundedRectangle (rf, 6.0f);
        g.setColour (col::line);
        g.drawRoundedRectangle (rf.reduced (0.5f), 6.0f, 1.0f);
        if (title.isNotEmpty())
        {
            g.setColour (col::gold);
            g.setFont (font (12.0f, true));
            g.drawText (title, r.getX() + 12, r.getY() + 7, r.getWidth() - 24, 22, juce::Justification::centredLeft);
        }
        return;
    }

    juce::ColourGradient bg (col::panel.interpolatedWith (accent, 0.10f), 0.0f, rf.getY(),
                             col::panel, 0.0f, rf.getY() + juce::jmin (160.0f, rf.getHeight()), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (rf, 7.0f);
    g.setColour (accent.withAlpha (0.30f));
    g.drawRoundedRectangle (rf.reduced (0.5f), 7.0f, 1.0f);
    juce::ColourGradient topLine (accent.withAlpha (0.0f), rf.getX(), 0.0f, accent.withAlpha (0.0f), rf.getRight(), 0.0f, false);
    topLine.addColour (0.15, accent.withAlpha (0.9f));
    topLine.addColour (0.6, accent.withAlpha (0.35f));
    g.setGradientFill (topLine);
    g.fillRect (rf.getX() + 8.0f, rf.getY(), rf.getWidth() - 16.0f, 2.0f);

    if (title.isNotEmpty())
    {
        g.setColour (accent);
        g.fillRoundedRectangle (rf.getX() + 12.0f, rf.getY() + 12.0f, 4.0f, 13.0f, 2.0f);
        g.setFont (font (12.0f, true));
        g.drawText (title, r.getX() + 22, r.getY() + 7, r.getWidth() - 34, 22, juce::Justification::centredLeft);
    }
}

void Content::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);
    const bool colorful = colorfulUi();

    if (colorful)
    {
        juce::ColourGradient glowA (juce::Colour (0x5a3a1a6a), (float) W * 0.12f, 40.0f, juce::Colour (0x003a1a6a), (float) W * 0.55f, (float) H * 0.7f, true);
        g.setGradientFill (glowA);
        g.fillAll();
        juce::ColourGradient glowB (juce::Colour (0x40104a5a), (float) W * 0.92f, (float) H * 0.95f, juce::Colour (0x00104a5a), (float) W * 0.45f, (float) H * 0.3f, true);
        g.setGradientFill (glowB);
        g.fillAll();
    }

    juce::ColourGradient hg (colorful ? juce::Colour (0xff1d1428) : juce::Colour (0xff17140c), 0, 0,
                             colorful ? juce::Colour (0x000a0a0b) : col::bg, 0, (float) rHeader.getBottom(), false);
    g.setGradientFill (hg);
    g.fillRect (rHeader);
    if (colorful)
    {
        g.setGradientFill (rainbowGradient (0.0f, (float) W, 0.0f, 0.85f));
        g.fillRect (0.0f, (float) rHeader.getBottom() - 2.0f, (float) W, 2.0f);
        juce::ColourGradient lg (col::goldHi, 18.0f, 10.0f, juce::Colour (0xffff6a3a), 100.0f, 46.0f, false);
        g.setGradientFill (lg);
    }
    else
    {
        g.setColour (col::gold.withAlpha (0.35f));
        g.drawHorizontalLine (rHeader.getBottom() - 1, 0.0f, (float) W);
        g.setColour (col::gold);
    }
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

    panel (g, rSpec, u8s ("SPEKTRUM"), accent (juce::Colour (0xffffb03a)));
    g.setColour (col::dim);
    g.setFont (font (10.0f));
    if (! pro)
        g.drawText (u8s ("20 Hz – 20 kHz"), rSpec.getX() + 12, rSpec.getY() + 24, 90, 12, juce::Justification::centredLeft);

    if (style.showTonal)
        panel (g, rTonal, u8s ("TONALE BALANCE  ·  Mix gegenüber Referenz"), accent (juce::Colour (0xffff6f91)));
    if (style.showHist)
        panel (g, rHist, pro && scopeTab ? u8s ("STEREO") : u8s ("VERLAUF  ·  Short-Term, 60 s"), accent (juce::Colour (0xff5fe08a)));
    if (style.showLoud)
        panel (g, rLoud, u8s ("LAUTHEIT"), accent (juce::Colour (0xffb58cff)));
    if (style.showMeter)
        panel (g, rMeter, u8s ("PEGEL"), accent (juce::Colour (0xff4fd2ff)));
    panel (g, rRef, {}, accent (theme().ref));

    g.setColour (theme().ref);
    g.setFont (font (12.0f, true));
    g.drawText ("REFERENZ", rRef.getX() + 12, rRef.getY() + 8, 132, 20, juce::Justification::centredLeft);

    const bool match = proc.apvts.getRawParameterValue ("match")->load() > 0.5f;
    g.setFont (font (11.5f));
    g.setColour (match && proc.hasReference() ? col::text : col::dim);
    const juce::String m = ! proc.hasReference() ? u8s ("–")
                         : (match ? u8s ("Angleich ") + fmt (proc.getMatchGainDb(), 1, true) + " dB" : u8s ("aus"));
    g.drawText (m, 904 + 198, rRef.getY() + 68, 166, 26, juce::Justification::centredLeft);
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
    const bool live = proc.isLiveSlot();
    const bool refPresent = ref != nullptr || (live && proc.isLiveConnected());
    const bool useStatic = ref != nullptr && channel == 0 && speedIdx == 3 && ! ref->longTermDb.empty();

    mixA.wantInstant = spectrum.mode == 2;
    mixA.update (slopes[slopeIdx], smooths[smoothIdx], speedIdx, hold);
    refA.update (slopes[slopeIdx], smooths[smoothIdx], speedIdx, hold,
                 useStatic ? &ref->longTermDb : nullptr, proc.getRefGainDb());

    static const char* chNames[] { "", "KANAL LINKS", "KANAL RECHTS", "SIDE (L-R)" };
    spectrum.showMix = vts.getRawParameterValue ("showMix")->load() > 0.5f;
    spectrum.showRef = vts.getRawParameterValue ("showRef")->load() > 0.5f;
    spectrum.showDiff = pro && vts.getRawParameterValue ("showDiff")->load() > 0.5f;
    spectrum.holdOn = hold;
    spectrum.refLoaded = refPresent;
    spectrum.longTerm = speedIdx == 3;
    spectrum.channelName = u8s (chNames[channel]);

    // Tonale Balance
    tonal.refLoaded = refPresent;
    tonal.refLive = live;
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
        if (live && channel == 0) tonal.channelNote = u8s ("Mix ") + mmss (tonal.mixSeconds) + u8s (" · Ref live ") + mmss (refA.averageSeconds());
    }

    // Lautheit
    auto& r = loud.r;
    r.mixM = proc.mixLoud.m.load(); r.mixS = proc.mixLoud.s.load(); r.mixI = proc.mixLoud.i.load();
    r.mixLra = proc.mixLoud.lra.load(); r.mixTp = proc.mixLoud.tp.load();
    r.mixSMax = proc.mixLoud.sMax.load(); r.mixMMax = proc.mixLoud.mMax.load();
    r.refLoaded = refPresent;
    r.refLive = live;
    r.refRunning = proc.isRefRunning();
    r.refM = proc.refLoud.m.load(); r.refS = proc.refLoud.s.load();
    if (live)
    {
        r.refI = proc.refLoud.i.load(); r.refLra = proc.refLoud.lra.load(); r.refTp = proc.refLoud.tp.load();
        r.refSMax = proc.refLoud.sMax.load(); r.refMMax = proc.refLoud.mMax.load();
    }
    else if (ref != nullptr)
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
    const bool ab = vts.getRawParameterValue ("ab")->load() > 0.5f && refPresent;
    btnA.setToggleState (! ab, juce::dontSendNotification);
    btnB.setToggleState (ab, juce::dontSendNotification);
    btnB.setEnabled (refPresent);
    btnClear.setEnabled (ref != nullptr && ! live);
    btnLoad.setEnabled (! live);
    btnSync.setEnabled (! live);
    btnLive.setToggleState (live, juce::dontSendNotification);
    if ((bool) btnLive.getProperties().getWithDefault ("dot", false) != proc.isLiveConnected())
    {
        btnLive.getProperties().set ("dot", proc.isLiveConnected());
        btnLive.repaint();
    }
    const bool sync = vts.getRawParameterValue ("sync")->load() > 0.5f;
    btnPlay.setEnabled (! sync && ref != nullptr && ! live);
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

    spectrum.advance (style.holdFrames());
    spectrum.repaint();
    if (tonal.isVisible()) tonal.repaint();
    loud.repaint();
    meters.repaint();
    if (history.isVisible()) history.repaint();
    if (scope.isVisible()) scope.repaint();
    wave.repaint();
    repaint (rRef.withTrimmedLeft (904 - rRef.getX()));
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
    if (slot >= AnalyzerProcessor::numSlots)
    {
        slot = 0;
        proc.setSlot (0);
    }
    for (auto& f : files)
    {
        const juce::File file (f);
        if (isAudioFile (file) && slot < AnalyzerProcessor::numSlots)
            proc.loadReference (file, slot++);
    }
}

} // namespace k54a

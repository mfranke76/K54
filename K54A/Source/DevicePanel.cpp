// K-54 Analyzer · Keller54 · by Frank E.
#include "DevicePanel.h"
#include "PluginEditor.h"

namespace k54a
{
static juce::Font dfont (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

static const devsim::Model& model (int d)
{
    const auto& ms = devsim::models();
    return ms[(size_t) juce::jlimit (0, (int) ms.size() - 1, d)];
}

juce::Colour deviceGroupColour (const juce::String& group)
{
    if (! colorfulUi()) return col::gold;
    if (group == "STUDIO")                   return col::gold;
    if (group == "HANDY & COMPUTER")         return col::ref;
    if (group == u8s ("KOPFHÖRER"))          return col::diff;
    if (group == "ZUHAUSE")                  return col::good;
    if (group == "AUTO")                     return col::warn;
    return juce::Colour (0xffff5fa8);   // LIVE
}

//==============================================================================
// Strich-Symbole der Geräte (in einem quadratischen Feld)
void drawDeviceIcon (juce::Graphics& g, int icon, juce::Rectangle<float> area, juce::Colour c, float st)
{
    using namespace devsim;
    auto r = area.withSizeKeepingCentre (juce::jmin (area.getWidth(), area.getHeight()),
                                         juce::jmin (area.getWidth(), area.getHeight()));
    const float x = r.getX(), y = r.getY(), s = r.getWidth();
    auto X = [&] (float f) { return x + f * s; };
    auto Y = [&] (float f) { return y + f * s; };
    auto rr = [&] (float x0, float y0, float x1, float y1, float corner)
    {
        g.drawRoundedRectangle (X (x0), Y (y0), X (x1) - X (x0), Y (y1) - Y (y0), corner * s, st);
    };
    auto circ = [&] (float cx, float cy, float rad, bool fill = false)
    {
        if (fill) g.fillEllipse (X (cx) - rad * s, Y (cy) - rad * s, 2 * rad * s, 2 * rad * s);
        else      g.drawEllipse (X (cx) - rad * s, Y (cy) - rad * s, 2 * rad * s, 2 * rad * s, st);
    };
    auto line = [&] (float x0, float y0, float x1, float y1) { g.drawLine (X (x0), Y (y0), X (x1), Y (y1), st); };
    auto speaker = [&] (float x0, float y0, float x1, float y1)
    {
        rr (x0, y0, x1, y1, 0.04f);
        const float w = x1 - x0, cx = 0.5f * (x0 + x1);
        circ (cx, y0 + 0.22f * (y1 - y0), w * 0.13f);
        circ (cx, y0 + 0.64f * (y1 - y0), w * 0.30f);
    };
    auto car = [&]
    {
        juce::Path p;
        p.startNewSubPath (X (0.06f), Y (0.66f));
        p.lineTo (X (0.06f), Y (0.52f));
        p.quadraticTo (X (0.08f), Y (0.45f), X (0.22f), Y (0.43f));
        p.lineTo (X (0.33f), Y (0.30f));
        p.quadraticTo (X (0.37f), Y (0.27f), X (0.44f), Y (0.27f));
        p.lineTo (X (0.66f), Y (0.27f));
        p.quadraticTo (X (0.71f), Y (0.27f), X (0.75f), Y (0.31f));
        p.lineTo (X (0.85f), Y (0.43f));
        p.quadraticTo (X (0.94f), Y (0.45f), X (0.94f), Y (0.53f));
        p.lineTo (X (0.94f), Y (0.66f));
        p.lineTo (X (0.82f), Y (0.66f));
        p.startNewSubPath (X (0.62f), Y (0.66f));
        p.lineTo (X (0.38f), Y (0.66f));
        p.startNewSubPath (X (0.18f), Y (0.66f));
        p.lineTo (X (0.06f), Y (0.66f));
        g.strokePath (p, juce::PathStrokeType (st, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        circ (0.28f, 0.66f, 0.09f);
        circ (0.72f, 0.66f, 0.09f);
        line (0.50f, 0.29f, 0.50f, 0.43f);
        line (0.24f, 0.43f, 0.84f, 0.43f);
    };

    g.setColour (c);
    switch (icon)
    {
        case icStudio:
            speaker (0.08f, 0.22f, 0.40f, 0.80f);
            speaker (0.60f, 0.22f, 0.92f, 0.80f);
            break;
        case icMonitor:   speaker (0.28f, 0.12f, 0.72f, 0.88f); break;
        case icCube:
            rr (0.20f, 0.20f, 0.80f, 0.80f, 0.05f);
            circ (0.5f, 0.5f, 0.20f);
            circ (0.5f, 0.5f, 0.06f, true);
            break;
        case icPhone:
            rr (0.32f, 0.08f, 0.68f, 0.92f, 0.07f);
            line (0.44f, 0.15f, 0.56f, 0.15f);
            line (0.45f, 0.85f, 0.55f, 0.85f);
            break;
        case icPhoneWide:
            rr (0.08f, 0.32f, 0.92f, 0.68f, 0.07f);
            line (0.15f, 0.44f, 0.15f, 0.56f);
            line (0.85f, 0.44f, 0.85f, 0.56f);
            break;
        case icTablet:
            rr (0.18f, 0.10f, 0.82f, 0.90f, 0.06f);
            circ (0.5f, 0.84f, 0.025f, true);
            break;
        case icLaptop:
        {
            rr (0.20f, 0.22f, 0.80f, 0.62f, 0.03f);
            juce::Path p;
            p.startNewSubPath (X (0.20f), Y (0.66f));
            p.lineTo (X (0.80f), Y (0.66f));
            p.lineTo (X (0.92f), Y (0.76f));
            p.lineTo (X (0.08f), Y (0.76f));
            p.closeSubPath();
            g.strokePath (p, juce::PathStrokeType (st, juce::PathStrokeType::curved));
            break;
        }
        case icTelephone:
        {
            juce::Path p;
            p.addCentredArc (X (0.5f), Y (0.62f), 0.32f * s, 0.30f * s, 0.0f, -1.25f, 1.25f, true);
            g.strokePath (p, juce::PathStrokeType (st * 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.fillRoundedRectangle (X (0.10f), Y (0.42f), 0.18f * s, 0.12f * s, 0.03f * s);
            g.fillRoundedRectangle (X (0.72f), Y (0.42f), 0.18f * s, 0.12f * s, 0.03f * s);
            break;
        }
        case icEarbud:
        {
            circ (0.30f, 0.26f, 0.10f);
            circ (0.70f, 0.26f, 0.10f);
            juce::Path p;
            p.startNewSubPath (X (0.30f), Y (0.36f));
            p.quadraticTo (X (0.32f), Y (0.60f), X (0.50f), Y (0.66f));
            p.quadraticTo (X (0.68f), Y (0.60f), X (0.70f), Y (0.36f));
            p.startNewSubPath (X (0.50f), Y (0.66f));
            p.quadraticTo (X (0.46f), Y (0.80f), X (0.52f), Y (0.92f));
            g.strokePath (p, juce::PathStrokeType (st, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            break;
        }
        case icTws:
            circ (0.30f, 0.36f, 0.12f);
            rr (0.25f, 0.46f, 0.35f, 0.74f, 0.04f);
            circ (0.70f, 0.36f, 0.12f);
            rr (0.65f, 0.46f, 0.75f, 0.74f, 0.04f);
            break;
        case icHeadphone:
        {
            juce::Path p;
            p.addCentredArc (X (0.5f), Y (0.56f), 0.34f * s, 0.38f * s, 0.0f, -1.5708f, 1.5708f, true);
            g.strokePath (p, juce::PathStrokeType (st, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.fillRoundedRectangle (X (0.10f), Y (0.50f), 0.14f * s, 0.30f * s, 0.05f * s);
            g.fillRoundedRectangle (X (0.76f), Y (0.50f), 0.14f * s, 0.30f * s, 0.05f * s);
            break;
        }
        case icBtBox:
            rr (0.08f, 0.32f, 0.92f, 0.70f, 0.18f);
            circ (0.27f, 0.51f, 0.11f);
            circ (0.73f, 0.51f, 0.11f);
            circ (0.5f, 0.51f, 0.025f, true);
            break;
        case icSmart:
            rr (0.30f, 0.14f, 0.70f, 0.88f, 0.12f);
            line (0.36f, 0.24f, 0.64f, 0.24f);
            for (int i = 0; i < 4; ++i)
                line (0.38f, 0.42f + 0.1f * (float) i, 0.62f, 0.42f + 0.1f * (float) i);
            break;
        case icTv:
            rr (0.08f, 0.20f, 0.92f, 0.70f, 0.03f);
            line (0.50f, 0.70f, 0.50f, 0.80f);
            line (0.32f, 0.82f, 0.68f, 0.82f);
            break;
        case icRadio:
            rr (0.08f, 0.36f, 0.92f, 0.84f, 0.06f);
            line (0.30f, 0.36f, 0.36f, 0.24f);
            line (0.36f, 0.24f, 0.64f, 0.24f);
            line (0.64f, 0.24f, 0.70f, 0.36f);
            line (0.74f, 0.36f, 0.90f, 0.08f);
            circ (0.32f, 0.60f, 0.15f);
            rr (0.58f, 0.46f, 0.84f, 0.56f, 0.02f);
            circ (0.64f, 0.70f, 0.04f);
            circ (0.78f, 0.70f, 0.04f);
            break;
        case icHifi:
            rr (0.30f, 0.06f, 0.70f, 0.94f, 0.04f);
            circ (0.5f, 0.18f, 0.06f);
            circ (0.5f, 0.42f, 0.13f);
            circ (0.5f, 0.74f, 0.13f);
            break;
        case icCar:
            car();
            break;
        case icCarSub:
            car();
            for (int i = 0; i < 3; ++i)
            {
                juce::Path p;
                const float rad = (0.08f + 0.06f * (float) i) * s;
                p.addCentredArc (X (0.5f), Y (0.86f), rad, rad * 0.6f, 0.0f, 2.2f, 4.08f, true);
                g.strokePath (p, juce::PathStrokeType (st * 0.9f));
            }
            break;
        case icClub:
            speaker (0.06f, 0.30f, 0.32f, 0.92f);
            speaker (0.68f, 0.30f, 0.94f, 0.92f);
            line (0.50f, 0.08f, 0.40f, 0.62f);
            line (0.50f, 0.08f, 0.50f, 0.66f);
            line (0.50f, 0.08f, 0.60f, 0.62f);
            circ (0.5f, 0.08f, 0.04f, true);
            break;
        default: break;
    }
}

//==============================================================================
juce::Rectangle<float> DeviceButton::powerArea() const
{
    auto r = getLocalBounds().toFloat();
    return r.removeFromRight (r.getHeight()).reduced (6.0f);
}

void DeviceButton::refresh()
{
    const auto s = proc.getDeviceSettings();
    const bool lim = proc.getDeviceSim().getLimiterDb() > 1.0f || proc.getDeviceSim().isClipping();
    if (s.device != device || s.on != on || lim != limiting)
    {
        device = s.device;
        on = s.on;
        limiting = lim;
        repaint();
    }
}

void DeviceButton::mouseDown (const juce::MouseEvent& e)
{
    if (device > 0 && powerArea().expanded (4.0f).contains (e.position))
    {
        if (onToggle) onToggle (! on);
    }
    else if (onOpen)
        onOpen();
}

void DeviceButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const auto& m = model (device);
    const bool active = on && device > 0;
    const juce::Colour acc = active ? col::warn : col::gold;

    juce::ColourGradient bg (col::panel2.brighter (hover ? 0.12f : 0.05f), 0, r.getY(), col::panel2.darker (0.3f), 0, r.getBottom(), false);
    if (active)
        bg = juce::ColourGradient (col::warn.withAlpha (0.30f).overlaidWith (col::panel2.withAlpha (0.5f)), 0, r.getY(),
                                   col::warn.withAlpha (0.14f), 0, r.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (active ? col::warn : (hover ? col::gold.withAlpha (0.7f) : col::line));
    g.drawRoundedRectangle (r, 5.0f, active ? 1.4f : 1.0f);

    auto ir = r.withWidth (r.getHeight()).reduced (5.0f);
    drawDeviceIcon (g, device > 0 ? m.icon : devsim::icHeadphone, ir, active ? col::warn : col::goldHi, 1.4f);

    auto tr = r.withTrimmedLeft (r.getHeight() - 2.0f).withTrimmedRight (device > 0 ? r.getHeight() - 4.0f : 6.0f);
    g.setColour (col::dim);
    g.setFont (dfont (9.5f, true));
    g.drawText (u8s (device > 0 ? (active ? "ABHÖREN AUF" : "SIMULATION AUS") : "ABHÖR-SIMULATION"),
                tr.withHeight (r.getHeight() * 0.48f).withTrimmedTop (3.0f), juce::Justification::bottomLeft);
    g.setColour (active ? juce::Colours::white : col::text);
    g.setFont (dfont (12.5f, true));
    g.drawFittedText (device > 0 ? u8s (m.name) : u8s ("Gerät wählen …"),
                      tr.withTrimmedTop (r.getHeight() * 0.48f).toNearestInt(), juce::Justification::topLeft, 1, 0.8f);

    if (device > 0)
    {
        auto p = powerArea();
        g.setColour (active ? col::warn : col::dim.withAlpha (0.6f));
        if (active)
        {
            g.setColour (col::warn.withAlpha (0.25f));
            g.fillEllipse (p.expanded (2.0f));
            g.setColour (limiting ? col::red : col::warn);
        }
        juce::Path pw;
        pw.addCentredArc (p.getCentreX(), p.getCentreY(), p.getWidth() * 0.36f, p.getWidth() * 0.36f, 0.0f, 0.6f, 5.68f, true);
        g.strokePath (pw, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (p.getCentreX(), p.getY() + 1.0f, p.getCentreX(), p.getCentreY(), 1.8f);
    }
    juce::ignoreUnused (acc);
}

//==============================================================================
void DeviceTile::paint (juce::Graphics& g)
{
    const auto& m = model (index);
    const auto gc = deviceGroupColour (u8s (m.group));
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool lit = selected && (simOn || index == 0);

    g.setColour (selected ? gc.withAlpha (lit ? 0.22f : 0.10f) : (hover ? col::panel2.brighter (0.08f) : col::panel2));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (selected ? gc : (hover ? gc.withAlpha (0.6f) : col::line));
    g.drawRoundedRectangle (r, 6.0f, selected ? 1.8f : 1.0f);

    drawDeviceIcon (g, m.icon, r.withHeight (r.getHeight() * 0.56f).reduced (6.0f, 6.0f).translated (0, 3.0f),
                    selected ? gc.brighter (0.2f) : (hover ? col::text : col::text.withAlpha (0.75f)), 1.5f);
    g.setColour (selected ? juce::Colours::white : col::text.withAlpha (0.9f));
    g.setFont (dfont (11.0f, selected));
    g.drawFittedText (u8s (m.name), r.withTrimmedTop (r.getHeight() * 0.58f).reduced (4.0f, 0.0f).toNearestInt(),
                      juce::Justification::centredTop, 2, 0.85f);
}

//==============================================================================
void ResponseView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::plot);
    g.fillRoundedRectangle (r, 5.0f);
    auto p = r.reduced (26.0f, 10.0f).withTrimmedBottom (8.0f);
    const float top = 18.0f, bot = -30.0f;
    auto fx = [&] (double f) { return p.getX() + (float) (std::log10 (f / 20.0) / 3.0) * p.getWidth(); };
    auto dy = [&] (float db) { return p.getY() + (top - juce::jlimit (bot, top, db)) / (top - bot) * p.getHeight(); };

    g.setFont (dfont (9.5f));
    for (float db : { 12.0f, 0.0f, -12.0f, -24.0f })
    {
        g.setColour (std::abs (db) < 0.1f ? col::line.brighter (0.4f) : col::grid);
        g.drawHorizontalLine ((int) dy (db), p.getX(), p.getRight());
        g.setColour (col::dim);
        g.drawText ((db > 0 ? "+" : "") + juce::String ((int) db), (int) r.getX(), (int) dy (db) - 6, 22, 12, juce::Justification::centredRight);
    }
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
    {
        g.setColour (col::grid);
        g.drawVerticalLine ((int) fx (f), p.getY(), p.getBottom());
        g.setColour (col::dim);
        const juce::String t = f >= 1000.0 ? juce::String ((int) (f / 1000.0)) + "k" : juce::String ((int) f);
        g.drawText (t, (int) fx (f) - 16, (int) p.getBottom() + 1, 32, 12, juce::Justification::centred);
    }

    if (device <= 0)
    {
        g.setColour (col::gold);
        g.drawLine (p.getX(), dy (0), p.getRight(), dy (0), 2.0f);
        g.setColour (col::dim);
        g.setFont (dfont (11.0f));
        g.drawText (u8s ("Original: linear, keine Veränderung"), p.withTrimmedTop (4.0f), juce::Justification::centredTop);
        return;
    }

    const auto gc = deviceGroupColour (u8s (model (device).group));
    juce::Path path, fill;
    const int pts = 160;
    for (int i = 0; i <= pts; ++i)
    {
        const double f = 20.0 * std::pow (1000.0, (double) i / pts);
        const float db = DeviceSim::responseDb (device, f, volume, trimDb);
        const float x = fx (f), y = dy (db);
        if (i == 0) { path.startNewSubPath (x, y); fill.startNewSubPath (x, p.getBottom()); }
        else path.lineTo (x, y);
        fill.lineTo (x, y);
    }
    fill.lineTo (p.getRight(), p.getBottom());
    fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (gc.withAlpha (on ? 0.35f : 0.12f), 0, p.getY(), gc.withAlpha (0.0f), 0, p.getBottom(), false));
    g.fillPath (fill);
    g.setColour (on ? gc : gc.withAlpha (0.45f));
    g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));

    g.setColour (col::dim);
    g.setFont (dfont (9.5f));
    g.drawText (u8s ("Frequenzgang Gerät (dB) · Raum und Geräusch nicht eingezeichnet"), p.withHeight (12.0f),
                juce::Justification::topRight);
}

//==============================================================================
DevicePanel::DevicePanel (AnalyzerProcessor& p) : proc (p)
{
    setWantsKeyboardFocus (true);
    const int n = devsim::numModels();
    for (int i = 0; i < n; ++i)
    {
        auto t = std::make_unique<DeviceTile>();
        t->index = i;
        t->onClick = [this, i] { selectDevice (i); };
        addAndMakeVisible (*t);
        tiles.push_back (std::move (t));
    }
    addAndMakeVisible (response);

    btnOn.setClickingTogglesState (false);
    btnOn.getProperties().set ("accent", (int) col::warn.getARGB());
    btnOn.onClick = [this] { if (setParam && device > 0) setParam ("devOn", on ? 0.0f : 1.0f); };
    addAndMakeVisible (btnOn);

    btnMatch.getProperties().set ("accent", (int) col::ref.getARGB());
    btnMatch.onClick = [this]
    {
        if (setParam) setParam ("devMatch", proc.getDeviceSettings().match ? 0.0f : 1.0f);
    };
    addAndMakeVisible (btnMatch);

    setupSegment (segVol, "devVol", { "LEISE", "NORMAL", "LAUT" });
    setupSegment (segNoise, "devNoise", { "AUS", "LEISE", "LAUT" });
    setupSegment (segSeat, "devSeat", { "FAHRER", "MITTE", "BEIFAHRER" });

    btnClose.setButtonText (u8s ("SCHLIESSEN"));
    btnClose.onClick = [this] { setVisible (false); };
    addAndMakeVisible (btnClose);

    segVol.b[0].setTooltip (u8s ("Leise hören: Bass und Höhen wirken schwächer, Umgebung lauter"));
    segVol.b[2].setTooltip (u8s ("Laut aufgedreht: Limiter und Zerre des Geräts greifen stärker"));
    btnMatch.setTooltip (u8s ("Gerät auf gleiche Lautheit bringen – damit du den Klang vergleichst, nicht die Lautstärke"));
    refresh();
}

void DevicePanel::setupSegment (Segment& sg, const char* id, std::initializer_list<const char*> names)
{
    int i = 0;
    for (auto* nm : names)
    {
        auto& b = sg.b[(size_t) i];
        b.setButtonText (u8s (nm));
        b.getProperties().set ("accent", (int) col::gold.getARGB());
        b.onClick = [this, id, i] { if (setParam) setParam (id, (float) i); };
        addAndMakeVisible (b);
        ++i;
    }
}

void DevicePanel::selectDevice (int d)
{
    if (! setParam) return;
    setParam ("device", (float) d);
    if (d > 0) setParam ("devOn", 1.0f);
}

bool DevicePanel::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { setVisible (false); return true; }
    if (k == juce::KeyPress::spaceKey && device > 0 && setParam) { setParam ("devOn", on ? 0.0f : 1.0f); return true; }
    const int n = devsim::numModels();
    if (k == juce::KeyPress::rightKey || k == juce::KeyPress::downKey) { selectDevice ((device + 1) % n); return true; }
    if (k == juce::KeyPress::leftKey || k == juce::KeyPress::upKey)    { selectDevice ((device + n - 1) % n); return true; }
    return false;
}

void DevicePanel::refresh()
{
    const auto s = proc.getDeviceSettings();
    const auto& m = model (s.device);
    const bool changed = s.device != device || s.on != on;
    device = s.device;
    on = s.on;
    isCar = m.car;
    noiseLevel = s.noise;

    for (auto& t : tiles)
    {
        const bool sel = t->index == device;
        if (t->selected != sel || t->simOn != on) { t->selected = sel; t->simOn = on; t->repaint(); }
    }

    response.device = device;
    response.volume = s.volume;
    response.on = on;
    response.trimDb = s.match ? proc.getDeviceSim().getTrimDb (device) : 0.0f;
    response.repaint();

    btnOn.setEnabled (device > 0);
    btnOn.setToggleState (on && device > 0, juce::dontSendNotification);
    btnOn.setButtonText (device <= 0 ? u8s ("KEIN GERÄT") : on ? u8s ("AN – GERÄT HÖREN") : u8s ("AUS – ORIGINAL HÖREN"));
    btnMatch.setToggleState (s.match, juce::dontSendNotification);
    btnMatch.setButtonText (s.match ? "AN" : "AUS");
    segVol.set (s.volume);
    segNoise.set (s.noise);
    segSeat.set (s.seat);
    for (int i = 0; i < 3; ++i)
    {
        segNoise.b[(size_t) i].setButtonText (u8s (devsim::noiseLevelName (m.noise, i)).toUpperCase());  // nur ASCII
        segNoise.b[(size_t) i].setEnabled (m.noise != devsim::nzNone || i == 0);
        segSeat.b[(size_t) i].setVisible (isCar);
    }

    const float lim = proc.getDeviceSim().getLimiterDb();
    const bool clip = proc.getDeviceSim().isClipping();
    if (changed || std::abs (lim - limiterDb) > 0.2f || clip != clipping)
    {
        limiterDb = lim;
        clipping = clip;
        repaint();
    }
}

void DevicePanel::resized()
{
    box = getLocalBounds().toFloat().withSizeKeepingCentre (1170.0f, 690.0f);
    const float x0 = box.getX() + 24.0f, y0 = box.getY() + 70.0f;
    const float tileW = 112.0f, tileH = 76.0f, gap = 6.0f, labelW = 122.0f;

    groupLabels.clear();
    juce::String lastGroup;
    int row = -1, colI = 0;
    for (auto& t : tiles)
    {
        const juce::String grp = u8s (model (t->index).group);
        if (grp != lastGroup)
        {
            lastGroup = grp;
            ++row;
            colI = 0;
            groupLabels.push_back ({ grp, { x0, y0 + (float) row * (tileH + 6.0f), labelW - 8.0f, tileH } });
        }
        t->setBounds ((int) (x0 + labelW + (float) colI * (tileW + gap)), (int) (y0 + (float) row * (tileH + 6.0f)),
                      (int) tileW, (int) tileH);
        ++colI;
    }

    const float rx = box.getX() + 770.0f, rw = box.getRight() - 24.0f - rx;
    infoArea = { rx, y0, rw, 166.0f };
    response.setBounds ((int) rx, (int) (y0 + 170.0f), (int) rw, 130);

    optArea = { rx, y0 + 312.0f, rw, 220.0f };
    const int ox = (int) rx + 136, ow = (int) rw - 136, rowH = 34;
    int y = (int) optArea.getY();
    rowOn    = { (int) rx, y, (int) rw, rowH }; btnOn.setBounds (ox, y + 3, ow, 28); y += rowH;
    rowVol   = { (int) rx, y, (int) rw, rowH };
    for (int i = 0; i < 3; ++i) segVol.b[(size_t) i].setBounds (ox + i * (ow / 3), y + 3, ow / 3 - 3, 26);
    y += rowH;
    rowNoise = { (int) rx, y, (int) rw, rowH };
    for (int i = 0; i < 3; ++i) segNoise.b[(size_t) i].setBounds (ox + i * (ow / 3), y + 3, ow / 3 - 3, 26);
    y += rowH;
    rowSeat  = { (int) rx, y, (int) rw, rowH };
    for (int i = 0; i < 3; ++i) segSeat.b[(size_t) i].setBounds (ox + i * (ow / 3), y + 3, ow / 3 - 3, 26);
    y += rowH;
    rowMatch = { (int) rx, y, (int) rw, rowH }; btnMatch.setBounds (ox, y + 3, 70, 26);

    btnClose.setBounds ((int) box.getRight() - 140, (int) box.getBottom() - 46, 116, 30);
}

void DevicePanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.62f));
    g.setColour (col::panel);
    g.fillRoundedRectangle (box, 8.0f);
    if (colorfulUi())
    {
        g.setGradientFill (rainbowGradient (box.getX() + 10.0f, box.getRight() - 10.0f, 0.0f, 0.9f));
        g.fillRect (box.getX() + 10.0f, box.getY(), box.getWidth() - 20.0f, 2.0f);
    }
    g.setColour (col::gold);
    g.drawRoundedRectangle (box, 8.0f, 1.5f);

    auto head = box.reduced (24.0f, 16.0f).withHeight (26.0f);
    g.setFont (dfont (20.0f, true));
    g.drawText (u8s ("ABHÖR-SIMULATION"), head, juce::Justification::centredLeft);
    g.setFont (dfont (11.5f));
    g.setColour (col::dim);
    g.drawText (u8s ("So klingt dein Mix (und die Referenz) auf anderen Geräten · Messwerte bleiben unverändert"),
                head, juce::Justification::centredRight);

    for (auto& gl : groupLabels)
    {
        const auto gc = deviceGroupColour (gl.first);
        g.setColour (gc);
        g.fillRoundedRectangle (gl.second.getX(), gl.second.getCentreY() - 8.0f, 4.0f, 16.0f, 2.0f);
        g.setColour (col::text.withAlpha (0.9f));
        g.setFont (dfont (11.5f, true));
        g.drawFittedText (gl.first, gl.second.withTrimmedLeft (10.0f).toNearestInt(), juce::Justification::centredLeft, 2);
    }

    g.setColour (col::line);
    g.drawVerticalLine ((int) infoArea.getX() - 18, box.getY() + 70.0f, box.getBottom() - 62.0f);

    // Info zum gewählten Gerät
    const auto& m = model (device);
    const auto gc = deviceGroupColour (u8s (m.group));
    auto ia = infoArea;
    auto iconR = ia.removeFromLeft (58.0f).withHeight (58.0f);
    g.setColour (gc.withAlpha (0.14f));
    g.fillRoundedRectangle (iconR, 8.0f);
    drawDeviceIcon (g, m.icon, iconR.reduced (8.0f), gc.brighter (0.2f), 1.8f);
    ia.removeFromLeft (12.0f);
    g.setColour (col::text);
    g.setFont (dfont (19.0f, true));
    g.drawText (u8s (m.name), ia.removeFromTop (26.0f), juce::Justification::centredLeft);
    g.setColour (gc);
    g.setFont (dfont (10.5f, true));
    g.drawText (u8s (m.group), ia.removeFromTop (18.0f), juce::Justification::centredLeft);

    auto txt = infoArea.withTrimmedTop (66.0f);
    juce::AttributedString as;
    as.setLineSpacing (1.5f);
    as.append (u8s (m.info) + "\n", dfont (12.5f), col::text.withAlpha (0.9f));
    as.append (u8s ("Darauf achten: "), dfont (12.5f, true), col::goldHi);
    as.append (u8s (m.tip), dfont (12.5f), col::text.withAlpha (0.9f));
    juce::TextLayout tl;
    tl.createLayout (as, txt.getWidth());
    tl.draw (g, txt);

    // Optionen
    auto label = [&] (juce::Rectangle<int> r, const juce::String& t, bool enabled = true)
    {
        g.setColour (enabled ? col::text.withAlpha (0.85f) : col::dim.withAlpha (0.5f));
        g.setFont (dfont (11.0f, true));
        g.drawText (t, r.withWidth (132), juce::Justification::centredLeft);
    };
    label (rowOn, "SIMULATION");
    label (rowVol, u8s ("LAUTSTÄRKE"));
    label (rowNoise, u8s (devsim::noiseLabel (m.noise)), m.noise != devsim::nzNone);
    label (rowSeat, "SITZPLATZ", isCar);
    if (! isCar)
    {
        g.setColour (col::dim.withAlpha (0.6f));
        g.setFont (dfont (11.0f));
        g.drawText ("nur im Auto", rowSeat.withTrimmedLeft (136), juce::Justification::centredLeft);
    }
    label (rowMatch, "PEGEL ANGLEICHEN");

    // Status: Limiter / Zerre
    {
        auto st = rowMatch.translated (0, 38).withHeight (40).withTrimmedLeft (0).toFloat();
        juce::String t1;
        juce::Colour c = col::good;
        if (device <= 0 || ! on)
        {
            t1 = device <= 0 ? u8s ("Wähle links ein Gerät – Klick auf „Original“ schaltet die Simulation ab.")
                             : u8s ("Simulation aus: du hörst das Original (Leertaste schaltet um).");
            c = col::dim;
        }
        else if (clipping)
        {
            t1 = u8s ("Lautsprecher übersteuert – so verzerrt es auf dem Gerät bei dieser Lautstärke.");
            c = col::red;
        }
        else if (limiterDb > 1.0f)
        {
            t1 = u8s ("Eingebauter Limiter drückt ") + juce::String (limiterDb, 1).replaceCharacter ('.', ',') + " dB";
            c = col::warn;
        }
        else
            t1 = u8s ("Gerät spielt sauber.");
        if (device > 0 && on)
        {
            g.setColour (c);
            g.fillEllipse (st.getX(), st.getY() + 6.0f, 9.0f, 9.0f);
        }
        g.setColour (device > 0 && on ? c.brighter (0.3f) : c);
        g.setFont (dfont (11.5f));
        g.drawFittedText (t1, st.withTrimmedLeft (device > 0 && on ? 16.0f : 0.0f).toNearestInt(), juce::Justification::topLeft, 2);
    }

    g.setColour (col::warn.withAlpha (0.9f));
    g.setFont (dfont (11.5f));
    g.drawText (u8s ("Nur zum Abhören – vor dem Bouncen ausschalten! (Knopf oben leuchtet orange)  ·  Pfeiltasten: Gerät wechseln · Leertaste: an/aus"),
                juce::Rectangle<float> (box.getX() + 24.0f, box.getBottom() - 46.0f, box.getWidth() - 190.0f, 30.0f),
                juce::Justification::centredLeft);
}

} // namespace k54a

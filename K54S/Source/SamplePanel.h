#pragma once
#include "PanelKit.h"
#include "SampleBrowser.h"

// ---------------------------------------------------------------------------
//  Kleine Pegelanzeige (senkrecht, Stereo) fuer den Eingang
class MiniMeter : public juce::Component
{
public:
    void setLevels (float l, float r)
    {
        auto fall = [] (float& v, float in) { v = juce::jmax (in, v * 0.82f); };
        fall (lv, l); fall (rv, r);
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        auto b = getLocalBounds().toFloat();
        g.setColour (panelHi);
        g.fillRoundedRectangle (b, 3.0f);
        auto bar = [&] (juce::Rectangle<float> r, float v)
        {
            const float db = juce::Decibels::gainToDecibels (v, -60.0f);
            const float f = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            auto fill = r.removeFromBottom (r.getHeight() * f);
            g.setColour (db > -1.0f ? juce::Colour (0xffE0503A) : (db > -12.0f ? gold : goldDim));
            g.fillRect (fill);
        };
        auto in = b.reduced (3.0f);
        const float w = (in.getWidth() - 2.0f) * 0.5f;
        bar (in.removeFromLeft (w), lv);
        in.removeFromLeft (2.0f);
        bar (in, rv);
    }
private:
    float lv = 0.0f, rv = 0.0f;
};

// ---------------------------------------------------------------------------
//  Sample-Editor wie in einer DAW: Wellenform mit Zoom, Auswahl, Start-/End-/Loop-Marken
//    - Marken S / E / LOOP mit der Maus ziehen
//    - sonst ziehen = Bereich auswaehlen (Shift = Auswahl erweitern, Doppelklick = alles)
//    - Mausrad / Pinch = Zoom an der Maus, waagerecht wischen (oder Shift + Rad) = verschieben
//    - Tasten: Entf = Auswahl loeschen, Cmd/Strg+Z = rueckgaengig, Cmd/Strg+Shift+Z = wiederholen,
//              Cmd/Strg+A = alles auswaehlen, Esc = Auswahl aufheben
class SampleWaveView : public juce::Component
{
public:
    explicit SampleWaveView (K55Processor& p) : proc (p)
    {
        setComponentID ("sampleWave");
        lastSample = proc.getSample().get();
        setWantsKeyboardFocus (true);
        setMouseClickGrabsKeyboardFocus (true);
    }

    // ---- Ansicht / Auswahl (Anteile 0..1 der Sample-Laenge)
    bool hasSelection() const noexcept   { return selB - selA > 0.00001; }
    double getSelStart() const noexcept  { return selA; }
    double getSelEnd() const noexcept    { return selB; }
    void clearSelection()                { selA = selB = 0.0; changed(); }
    void setSelection (double a, double b) { syncSample(); selA = juce::jlimit (0.0, 1.0, juce::jmin (a, b)); selB = juce::jlimit (0.0, 1.0, juce::jmax (a, b)); changed(); }
    void selectAll()                     { syncSample(); if (proc.getSample() != nullptr) { selA = 0.0; selB = 1.0; changed(); } }
    bool isZoomed() const noexcept       { return viewLen < 0.999; }

    void zoomAll()                       { viewStart = 0.0; viewLen = 1.0; changed(); }
    void zoomBy (double factor, double anchor = -1.0)
    {
        auto s = proc.getSample();
        if (s == nullptr) return;
        checkSampleChanged (s.get());
        if (anchor < 0.0) anchor = hasSelection() ? (selA + selB) * 0.5 : viewStart + viewLen * 0.5;
        const double minLen = juce::jmin (1.0, 64.0 / juce::jmax (64.0, (double) s->length()));   // bis ca. 64 Samples breit
        const double newLen = juce::jlimit (minLen, 1.0, viewLen * factor);
        const double rel = (anchor - viewStart) / viewLen;
        viewStart = anchor - rel * newLen;
        viewLen = newLen;
        clampView();
        changed();
    }
    void zoomToSelection()
    {
        if (! hasSelection()) return;
        const double pad = (selB - selA) * 0.05;
        viewStart = selA - pad; viewLen = juce::jmax (selB - selA + 2.0 * pad, 0.0005);
        clampView(); changed();
    }

    /** Bearbeitung ausfuehren (Auswahl oder ganzes Sample) */
    bool edit (K55Processor::SampleEdit op)
    {
        const bool sel = hasSelection();
        if (K55Processor::editNeedsSelection (op) && ! sel) return false;
        const double a = sel ? selA : 0.0, b = sel ? selB : 0.0;
        const auto before = proc.getSample();
        const double oldLen = before != nullptr ? (double) before->length() : 1.0;
        if (! proc.editSample (op, a, b)) return false;
        const auto after = proc.getSample();
        const double newLen = after != nullptr ? (double) after->length() : 1.0;
        lastSample = after.get();
        if (op == K55Processor::SampleEdit::trim)
        {
            selA = selB = 0.0; zoomAll();
        }
        else if (op == K55Processor::SampleEdit::cut)
        {
            // Ansicht bleibt an der Schnittstelle stehen
            const double cutPos = a * oldLen / newLen;
            viewLen = juce::jmin (1.0, viewLen * oldLen / newLen);
            viewStart = cutPos - viewLen * 0.5;
            clampView();
            selA = selB = 0.0;
        }
        changed();
        return true;
    }
    bool undo() { const bool ok = proc.undoSampleEdit(); if (ok) afterHistoryStep(); return ok; }
    bool redo() { const bool ok = proc.redoSampleEdit(); if (ok) afterHistoryStep(); return ok; }

    /** setzt Start/Ende-Marken auf die Auswahl (zerstoerungsfrei) */
    void markersFromSelection()
    {
        if (! hasSelection()) return;
        setMarker (ParamID::smpStart, (float) selA);
        setMarker (ParamID::smpEnd, (float) selB);
        changed();
    }

    std::function<void()> onEmptyClick, onStateChanged;
    bool dragOver = false;

    // ------------------------------------------------------------------ Zeichnen
    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0E0E10));
        g.fillRoundedRectangle (b, 6.0f);

        auto s = proc.getSample();
        if (s == nullptr || s->length() == 0)
        {
            g.setColour (dragOver ? gold : line);
            g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, dragOver ? 2.0f : 1.0f);
            g.setColour (creamDim.withAlpha (0.6f));
            g.setFont (K54Fonts::make (K54Fonts::syncopate(), 10.0f, 0.15f));
            g.drawFittedText (ParamChoices::u8 ("AUDIODATEI HIER HINEINZIEHEN (FINDER / DAW)\nODER \xc2\xbbLADEN\xc2\xab / \xc2\xbbREC\xc2\xab DR\xc3\x9c" "CKEN"),
                              getLocalBounds().reduced (10), juce::Justification::centred, 3);
            return;
        }
        checkSampleChanged (s.get());

        auto inner = waveArea();
        const float gain = juce::Decibels::decibelsToGain (value (ParamID::smpGain), -60.0f);
        const float mid = inner.getCentreY(), h = inner.getHeight() * 0.5f;
        updateColumns (*s, (int) inner.getWidth());

        const float st = value (ParamID::smpStart), en = juce::jmax (st + 0.001f, value (ParamID::smpEnd));
        const float ls = st + value (ParamID::smpLoopStart) * (en - st);
        const bool loop = juce::roundToInt (value (ParamID::smpLoop)) > 0;

        // Nulllinie + 0-dB-Linien
        g.setColour (line.withAlpha (0.6f));
        g.fillRect (inner.getX(), mid, inner.getWidth(), 1.0f);

        // Wellenform (Spalten mit Min/Max, mit Sample-Lautstaerke skaliert)
        juce::Path wave, clip;
        const int W = (int) columns.size();
        for (int x = 0; x < W; ++x)
        {
            float lo = columns[(size_t) x].first * gain, hi = columns[(size_t) x].second * gain;
            const float px = inner.getX() + (float) x;
            if (hi > 1.0f || lo < -1.0f) clip.addRectangle (px, inner.getY(), 1.0f, inner.getHeight());
            hi = juce::jlimit (-1.0f, 1.0f, hi); lo = juce::jlimit (-1.0f, 1.0f, lo);
            const float y0 = mid - hi * h, y1 = mid - lo * h;
            wave.addRectangle (px, y0, 1.0f, juce::jmax (1.0f, y1 - y0));
        }
        g.setColour (creamDim.withAlpha (0.35f));
        g.fillPath (wave);
        {
            juce::Graphics::ScopedSaveState ss (g);
            const float x0 = juce::jmax (inner.getX(), xFor (st)), x1 = juce::jmin (inner.getRight(), xFor (en));
            if (x1 > x0)
            {
                g.reduceClipRegion (juce::Rectangle<float> (x0, b.getY(), x1 - x0, b.getHeight()).toNearestInt());
                g.setColour (gold);
                g.fillPath (wave);
            }
        }
        if (! clip.isEmpty())
        {
            g.setColour (juce::Colour (0x40E0503A));
            g.fillPath (clip);
        }

        // Auswahl
        if (hasSelection())
        {
            const float x0 = juce::jmax (inner.getX(), xFor ((float) selA)), x1 = juce::jmin (inner.getRight(), xFor ((float) selB));
            if (x1 > x0)
            {
                g.setColour (cream.withAlpha (0.12f));
                g.fillRect (x0, b.getY() + 1.0f, x1 - x0, b.getHeight() - 2.0f);
                g.setColour (cream.withAlpha (0.7f));
                if (xFor ((float) selA) >= inner.getX()) g.fillRect (x0, b.getY() + 1.0f, 1.0f, b.getHeight() - 2.0f);
                if (xFor ((float) selB) <= inner.getRight()) g.fillRect (x1 - 1.0f, b.getY() + 1.0f, 1.0f, b.getHeight() - 2.0f);
            }
        }

        // Marken
        auto marker = [&] (float pos, juce::Colour c, const char* t)
        {
            const float x = xFor (pos);
            if (x < inner.getX() - 1.0f || x > inner.getRight() + 1.0f) return;
            g.setColour (c);
            g.fillRect (x - 1.0f, b.getY() + 2.0f, 2.0f, b.getHeight() - 4.0f);
            g.fillRect (x - 1.0f, b.getY() + 2.0f, 8.0f, 2.0f);
            g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 8.0f, 0.1f));
            g.drawText (t, juce::Rectangle<float> (x + 3.0f, b.getY() + 5.0f, 40.0f, 12.0f), juce::Justification::centredLeft);
        };
        marker (st, cream, "S");
        marker (en, cream, "E");
        if (loop) marker (ls, juce::Colour (0xff6FC3DF), "LOOP");

        // Uebersicht oben, wenn gezoomt
        if (isZoomed())
        {
            auto ov = juce::Rectangle<float> (inner.getX(), b.getY() + 2.0f, inner.getWidth(), 3.0f);
            g.setColour (line);
            g.fillRect (ov);
            g.setColour (gold);
            g.fillRect (ov.getX() + ov.getWidth() * (float) viewStart, ov.getY(), juce::jmax (3.0f, ov.getWidth() * (float) viewLen), ov.getHeight());
        }

        // Infozeile
        g.setColour (creamDim);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.1f));
        const double secs = (double) s->length() / s->sampleRate;
        juce::String info = s->name.toUpperCase() + "   " + juce::String (secs, 2) + " S   "
                          + (s->isStereo() ? "STEREO" : "MONO") + "   " + juce::String (juce::roundToInt (s->sampleRate)) + " HZ";
        const float db = value (ParamID::smpGain);
        if (std::abs (db) > 0.05f) info << "   " << (db > 0 ? "+" : "") << juce::String (db, 1) << " DB";
        g.drawText (info, getLocalBounds().reduced (10, 4).removeFromBottom (14), juce::Justification::centredRight);
        juce::String left;
        if (hasSelection())
            left << ParamChoices::u8 ("AUSWAHL ") << fmtTime (selA * secs) << " - " << fmtTime (selB * secs)
                 << "  (" << fmtTime ((selB - selA) * secs) << ")";
        else if (isZoomed())
            left << "ZOOM " << fmtTime (viewStart * secs) << " - " << fmtTime ((viewStart + viewLen) * secs);
        if (left.isNotEmpty())
            g.drawText (left, getLocalBounds().reduced (10, 4).removeFromBottom (14), juce::Justification::centredLeft);

        g.setColour (dragOver ? gold : (hasKeyboardFocus (false) ? goldDim : line));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, dragOver ? 2.0f : 1.0f);
    }

    // ------------------------------------------------------------------ Maus
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (proc.getSample() == nullptr) { if (onEmptyClick) onEmptyClick(); return; }
        const float x = (float) e.x;
        dragging = nullptr;
        selecting = false;

        if (! e.mods.isShiftDown())
        {
            const float st = value (ParamID::smpStart), en = value (ParamID::smpEnd);
            const float ls = st + value (ParamID::smpLoopStart) * (en - st);
            const bool loop = juce::roundToInt (value (ParamID::smpLoop)) > 0;
            float best = 7.0f;
            auto test = [&] (float pos, const char* id) { const float d = std::abs (xFor (pos) - x); if (d < best) { best = d; dragging = id; } };
            test (st, ParamID::smpStart);
            test (en, ParamID::smpEnd);
            if (loop) test (ls, ParamID::smpLoopStart);
            // Marken lassen sich auch oben an der kleinen Fahne greifen
            if (dragging == nullptr && e.y < 18)
            {
                best = 12.0f;
                test (st, ParamID::smpStart); test (en, ParamID::smpEnd);
                if (loop) test (ls, ParamID::smpLoopStart);
            }
        }
        if (dragging != nullptr)
        {
            if (auto* p = proc.apvts.getParameter (dragging)) p->beginChangeGesture();
            mouseDrag (e);
            return;
        }
        const double pos = posAt (x);
        if (e.mods.isShiftDown() && hasSelection())
            anchor = std::abs (pos - selA) < std::abs (pos - selB) ? selB : selA;
        else
            anchor = pos;
        selecting = true;
        mouseDrag (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (selecting)
        {
            autoScroll ((float) e.x);
            const double pos = juce::jlimit (0.0, 1.0, posAt ((float) e.x));
            selA = juce::jmin (anchor, pos);
            selB = juce::jmax (anchor, pos);
            changed();
            return;
        }
        if (dragging == nullptr) return;
        float pos = (float) juce::jlimit (0.0, 1.0, posAt ((float) e.x));
        const float st = value (ParamID::smpStart), en = value (ParamID::smpEnd);
        if (dragging == ParamID::smpStart)          pos = juce::jmin (pos, en - 0.001f);
        else if (dragging == ParamID::smpEnd)       pos = juce::jmax (pos, st + 0.001f);
        else                                        pos = juce::jlimit (0.0f, 0.98f, (pos - st) / juce::jmax (0.001f, en - st));
        if (auto* p = proc.apvts.getParameter (dragging)) p->setValueNotifyingHost (p->convertTo0to1 (pos));
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging != nullptr)
            if (auto* p = proc.apvts.getParameter (dragging)) p->endChangeGesture();
        dragging = nullptr;
        if (selecting && (xFor ((float) selB) - xFor ((float) selA)) < 3.0f) clearSelection();
        selecting = false;
    }

    void mouseDoubleClick (const juce::MouseEvent&) override { selectAll(); }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (proc.getSample() == nullptr) return;
        const bool horizontal = std::abs (w.deltaX) > std::abs (w.deltaY) || e.mods.isShiftDown();
        if (horizontal)
        {
            const float d = std::abs (w.deltaX) > std::abs (w.deltaY) ? w.deltaX : w.deltaY;
            viewStart -= (double) d * viewLen * 0.5;
            clampView(); changed();
        }
        else if (std::abs (w.deltaY) > 0.0f)
            zoomBy (std::pow (0.5, (double) w.deltaY * 2.5), posAt ((float) e.x));
    }

    void mouseMagnify (const juce::MouseEvent& e, float scale) override
    {
        if (scale > 0.0f) zoomBy (1.0 / scale, posAt ((float) e.x));
    }

    bool keyPressed (const juce::KeyPress& k) override
    {
        using SE = K55Processor::SampleEdit;
        const bool cmd = k.getModifiers().isCommandDown();
        const int code = k.getKeyCode();
        if (cmd && (code == 'Z' || code == 'z')) return k.getModifiers().isShiftDown() ? redo() || true : undo() || true;
        if (cmd && (code == 'Y' || code == 'y')) return redo() || true;
        if (cmd && (code == 'A' || code == 'a')) { selectAll(); return true; }
        if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey) { edit (SE::cut); return true; }
        if (code == juce::KeyPress::escapeKey) { clearSelection(); return true; }
        if (code == '+' || code == '=') { zoomBy (0.5); return true; }
        if (code == '-') { zoomBy (2.0); return true; }
        return false;
    }

    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override   { repaint(); }
    void resized() override { columnsKey = {}; }

private:
    juce::Rectangle<float> waveArea() const { return getLocalBounds().toFloat().reduced (6.0f, 10.0f).withTrimmedBottom (8.0f); }
    float value (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    double posAt (float x) const
    {
        auto inner = waveArea();
        return viewStart + viewLen * (double) ((x - inner.getX()) / inner.getWidth());
    }
    float xFor (float pos) const
    {
        auto inner = waveArea();
        return inner.getX() + inner.getWidth() * (float) (((double) pos - viewStart) / viewLen);
    }
    void clampView()
    {
        viewLen = juce::jlimit (0.00001, 1.0, viewLen);
        viewStart = juce::jlimit (0.0, 1.0 - viewLen, viewStart);
    }
    void autoScroll (float x)
    {
        auto inner = waveArea();
        if (! isZoomed()) return;
        if (x < inner.getX())          { viewStart -= viewLen * 0.03; clampView(); }
        else if (x > inner.getRight()) { viewStart += viewLen * 0.03; clampView(); }
    }
    void setMarker (const char* id, float v)
    {
        if (auto* p = proc.apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (v));
            p->endChangeGesture();
        }
    }
    void changed()
    {
        repaint();
        if (onStateChanged) onStateChanged();
    }
    void afterHistoryStep()
    {
        lastSample = proc.getSample().get();
        selA = selB = 0.0;
        clampView();
        changed();
    }
    void syncSample() { checkSampleChanged (proc.getSample().get()); }
    /** neues Sample (geladen, aufgenommen) -> Ansicht zuruecksetzen */
    void checkSampleChanged (const SampleData* s)
    {
        if (s == lastSample) return;
        lastSample = s;
        selA = selB = 0.0;
        viewStart = 0.0; viewLen = 1.0;
        if (onStateChanged) onStateChanged();
    }
    static juce::String fmtTime (double sec)
    {
        if (sec < 10.0) return juce::String (sec, 3) + " S";
        return juce::String (sec, 2) + " S";
    }

    /** Min/Max je Bildschirmspalte, nur neu berechnet wenn sich Ansicht oder Sample aendern */
    void updateColumns (const SampleData& s, int width)
    {
        const std::tuple<const void*, double, double, int> key { &s, viewStart, viewLen, width * 100000 + (proc.sourceVersion.load() % 100000) };
        if (key == columnsKey) return;
        columnsKey = key;
        columns.assign ((size_t) juce::jmax (0, width), { 0.0f, 0.0f });
        const int len = s.length(), chs = s.audio.getNumChannels();
        if (len == 0 || width <= 0) return;
        const double first = viewStart * len, perPx = viewLen * len / width;
        for (int x = 0; x < width; ++x)
        {
            const double p0 = first + perPx * x, p1 = p0 + perPx;
            float lo = 0.0f, hi = 0.0f;
            if (perPx >= 1.0)
            {
                const int a = juce::jlimit (0, len - 1, (int) p0), b = juce::jlimit (a + 1, len, (int) p1);
                lo = 1.0f; hi = -1.0f;
                for (int c = 0; c < chs; ++c)
                {
                    auto r = s.audio.findMinMax (c, a, b - a);
                    lo = juce::jmin (lo, r.getStart()); hi = juce::jmax (hi, r.getEnd());
                }
            }
            else
            {
                // stark gezoomt: zwischen den Samples interpolieren (Linie statt Balken)
                const double q0 = juce::jlimit (0.0, (double) len - 1.0, p0), q1 = juce::jlimit (0.0, (double) len - 1.0, p1);
                lo = 1.0f; hi = -1.0f;
                for (int c = 0; c < chs; ++c)
                {
                    const float v0 = s.read (c, q0), v1 = s.read (c, q1);
                    lo = juce::jmin (lo, v0, v1); hi = juce::jmax (hi, v0, v1);
                }
            }
            columns[(size_t) x] = { lo, hi };
        }
    }

    K55Processor& proc;
    const char* dragging = nullptr;
    bool selecting = false;
    double anchor = 0.0, selA = 0.0, selB = 0.0;
    double viewStart = 0.0, viewLen = 1.0;
    const SampleData* lastSample = nullptr;
    std::vector<std::pair<float, float>> columns;
    std::tuple<const void*, double, double, int> columnsKey { nullptr, -1.0, -1.0, -1 };
};

// ---------------------------------------------------------------------------
//  Wavetable-Ansicht: Frames gestaffelt, aktuelle Position in Gold
class WavetableView : public juce::Component
{
public:
    void setTable (const Wavetable* t, float position, bool enabled)
    {
        if (t != table || std::abs (position - pos) > 0.001f || enabled != active)
        {
            table = t; pos = position; active = enabled;
            repaint();
        }
    }
    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0E0E10));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (dragOver ? gold : line);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, dragOver ? 2.0f : 1.0f);
        if (table == nullptr || table->numFrames == 0) return;

        const int F = table->numFrames;
        const int shown = juce::jmin (F, 16);
        const float dx = 4.0f, dy = 4.0f;
        auto area = b.reduced (10.0f, 10.0f);
        const float w = area.getWidth() - dx * (shown - 1), h = area.getHeight() - dy * (shown - 1);
        auto drawFrame = [&] (float fpos, float ox, float oy, juce::Colour c, float thick)
        {
            juce::Path p;
            const int pts = 160;
            for (int i = 0; i <= pts; ++i)
            {
                const float v = table->read ((double) i / pts * 0.9999, fpos, 0);
                const float x = area.getX() + ox + w * (float) i / pts;
                const float y = area.getY() + oy + h * 0.5f - v * h * 0.42f;
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            g.setColour (c);
            g.strokePath (p, juce::PathStrokeType (thick));
        };
        for (int k = shown - 1; k >= 0; --k)
        {
            const float fpos = shown == 1 ? 0.0f : (float) k / (float) (shown - 1);
            drawFrame (fpos, dx * (float) k, dy * (float) (shown - 1 - k), creamDim.withAlpha (0.18f), 1.0f);
        }
        const float k = pos * (float) (shown - 1);
        drawFrame (pos, dx * k, dy * ((float) (shown - 1) - k), active ? gold : goldDim, 2.0f);

        g.setColour (creamDim);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.1f));
        g.drawText (table->name.toUpperCase() + ParamChoices::u8 ("  \xc2\xb7  ") + juce::String (F) + ParamChoices::u8 (" FRAMES  \xc2\xb7  POS ") + juce::String (juce::roundToInt (pos * 100.0f)) + " %",
                    getLocalBounds().reduced (10, 4).removeFromBottom (14), juce::Justification::centredRight);
    }
    bool dragOver = false;
private:
    const Wavetable* table = nullptr;
    float pos = 0.0f;
    bool active = true;
};

// ---------------------------------------------------------------------------
//  Reiter SAMPLE: laden, aufnehmen, bearbeiten, Wavetable
class SamplePanel : public KitPanel, private juce::Timer
{
public:
    explicit SamplePanel (K55Processor& p) : KitPanel (p), wave (p), browser (p)
    {
        start   = &knob (ParamID::smpStart, "Start");
        end     = &knob (ParamID::smpEnd, "End");
        loopSt  = &knob (ParamID::smpLoopStart, "Loop Pos");
        loop    = &choice (ParamID::smpLoop, "Loop");
        root    = &knob (ParamID::smpRoot, "Root Key");
        keyTrk  = &toggle (ParamID::smpKeyTrack, "KEY TRACK", "An: Sample folgt der Tastatur (chromatisch). Aus: immer Originaltonhoehe");
        oneShot = &toggle (ParamID::smpOneShot, "ONE-SHOT", "Sample spielt nach dem Loslassen der Taste zu Ende (nur ohne Loop)");
        volume  = &knob (ParamID::smpGain, "Volume", true);
        volume->s.setTooltip ("Lautstaerke des Samples (-24 bis +24 dB). Doppelklick = 0 dB");

        // Bearbeiten wie in einer DAW
        using SE = K55Processor::SampleEdit;
        struct EB { juce::TextButton* b; SE op; const char* tip; };
        for (auto& x : { EB { &trimBtn,  SE::trim,      "Nur die Auswahl behalten (Rest wegschneiden)" },
                         EB { &cutBtn,   SE::cut,       "Auswahl herausschneiden, der Rest rueckt zusammen (Taste Entf)" },
                         EB { &muteBtn,  SE::silence,   "Auswahl stumm schalten" },
                         EB { &fadeInBtn, SE::fadeIn,   "Auswahl einblenden" },
                         EB { &fadeOutBtn, SE::fadeOut, "Auswahl ausblenden" },
                         EB { &normBtn,  SE::normalize, "Auswahl (oder ganzes Sample) auf volle Lautstaerke bringen (-0,3 dB Spitze)" },
                         EB { &revBtn,   SE::reverse,   "Auswahl (oder ganzes Sample) rueckwaerts" } })
        {
            addAndMakeVisible (x.b);
            x.b->setTooltip (ParamChoices::u8 (x.tip));
            const auto op = x.op;
            x.b->onClick = [this, op] { wave.edit (op); wave.grabKeyboardFocus(); updateEditButtons(); };
        }
        for (auto* b : { &undoBtn, &redoBtn, &zoomOutBtn, &zoomInBtn, &zoomAllBtn, &selToMarkBtn })
            addAndMakeVisible (b);
        undoBtn.setTooltip (ParamChoices::u8 ("R\xc3\xbc" "ckg\xc3\xa4ngig (Cmd/Strg + Z)"));
        redoBtn.setTooltip ("Wiederholen (Cmd/Strg + Shift + Z)");
        zoomInBtn.setTooltip ("Hineinzoomen (auch Mausrad / Trackpad)");
        zoomOutBtn.setTooltip ("Herauszoomen");
        zoomAllBtn.setTooltip ("Ganzes Sample zeigen; mit Auswahl: auf die Auswahl zoomen");
        selToMarkBtn.setTooltip ("Start- und End-Marke auf die Auswahl setzen (das Sample bleibt unveraendert)");
        undoBtn.onClick     = [this] { wave.undo(); updateEditButtons(); };
        redoBtn.onClick     = [this] { wave.redo(); updateEditButtons(); };
        zoomInBtn.onClick   = [this] { wave.zoomBy (0.5); };
        zoomOutBtn.onClick  = [this] { wave.zoomBy (2.0); };
        zoomAllBtn.onClick  = [this] { if (wave.hasSelection() && ! wave.isZoomed()) wave.zoomToSelection(); else wave.zoomAll(); };
        selToMarkBtn.onClick= [this] { wave.markersFromSelection(); };
        wave.onStateChanged = [this] { updateEditButtons(); };
        for (auto* b : { &trimBtn, &cutBtn, &muteBtn, &fadeInBtn, &fadeOutBtn, &normBtn, &revBtn, &zoomAllBtn, &selToMarkBtn })
            b->getProperties().set ("fontSize", 8.0f);
        for (auto* b : { &undoBtn, &redoBtn, &zoomOutBtn, &zoomInBtn })
            b->getProperties().set ("fontSize", 15.0f);

        inMode  = &choice (ParamID::inMode, "Eingang");
        inGain  = &knob (ParamID::inGain, "Gain", true);
        inAuto  = &toggle (ParamID::inAuto, "AUTO-START", "Aufnahme startet erst, wenn ein Signal kommt");
        inMon   = &toggle (ParamID::inMonitor, "MONITOR", "Eingang mithoeren");

        for (auto* b : { &loadBtn, &recBtn, &stopBtn, &toWtBtn, &clearBtn, &wtLoadBtn, &osc1Btn, &osc2Btn, &osc3Btn })
            addAndMakeVisible (b);
        loadBtn.setTooltip ("Audiodatei laden (WAV, AIFF, FLAC, MP3 ...)");
        recBtn.setTooltip ("Vom Audio-Eingang aufnehmen (Mikrofon / Interface, max. 60 s)");
        toWtBtn.setTooltip ("Aus dem Sample eine eigene Wavetable machen (Tabelle USER)");
        wtLoadBtn.setTooltip ("Wavetable-Datei laden (z. B. Serum-Format: je 2048 Samples ein Frame)");
        loadBtn.onClick  = [this] { chooseFile (false); };
        wtLoadBtn.onClick= [this] { chooseFile (true); };
        recBtn.onClick   = [this] { proc.startRecording(); };
        stopBtn.onClick  = [this] { proc.stopRecording(); };
        toWtBtn.onClick  = [this] { proc.sampleToWavetable(); };
        clearBtn.onClick = [this] { proc.clearSample(); };
        for (auto* b : { &osc1Btn, &osc2Btn, &osc3Btn })
        {
            b->setClickingTogglesState (true);
            b->setRadioGroupId (551);
        }
        osc1Btn.setToggleState (true, juce::dontSendNotification);
        osc1Btn.onClick = [this] { viewOsc = 0; };
        osc2Btn.onClick = [this] { viewOsc = 1; };
        osc3Btn.onClick = [this] { viewOsc = 2; };

        addAndMakeVisible (wave);
        wave.onEmptyClick = [this] { chooseFile (false); };
        addAndMakeVisible (wtView);
        addAndMakeVisible (inMeter);
        addChildComponent (browser);
        browser.onLoad = [this] (const juce::File& f) { loadFile (f, false); };
        for (auto* b : { &browseTab, &inputTab })
        {
            b->setClickingTogglesState (true);
            b->setRadioGroupId (552);
            addAndMakeVisible (b);
        }
        browseTab.setTooltip ("Alle Samples und Loops: Logic Apple Loops, Sound Library, Sample-Platten, eigene Ordner");
        inputTab.setTooltip ("Aufnahme-Eingang und Wavetable der Oszillatoren");
        browseTab.onClick = [this] { if (browseTab.getToggleState()) setRightPage (0); };
        inputTab.onClick  = [this] { if (inputTab.getToggleState())  setRightPage (1); };
        setRightPage (0);   // Browser zuerst
        startTimerHz (20);
    }

    void setRightPage (int pg)
    {
        rightPage = pg;
        browseTab.setToggleState (pg == 0, juce::dontSendNotification);
        inputTab.setToggleState (pg == 1, juce::dontSendNotification);
        browser.setVisible (pg == 0);
        const bool in = pg == 1;
        for (juce::Component* c : std::initializer_list<juce::Component*> { &inMode->b, &inMode->l, &inGain->s, &inGain->l, &inMeter, &inAuto->b, &inMon->b,
                                                                             &osc1Btn, &osc2Btn, &osc3Btn, &wtLoadBtn, &wtView })
            c->setVisible (in);
        repaint();
    }

    /** Fuer Drag & Drop aus dem Editor: liegt der Punkt ueber der Wavetable-Ansicht? */
    bool isOverWavetable (juce::Point<int> pInPanel) const { return wtView.isVisible() && wtView.getBounds().contains (pInPanel); }
    void setDragHighlight (int which)   // 0 aus, 1 Sample, 2 Wavetable
    {
        wave.dragOver = which == 1; wtView.dragOver = which == 2;
        wave.repaint(); wtView.repaint();
    }

    void resized() override
    {
        auto r = getLocalBounds();
        secSample = r.removeFromLeft (712);
        r.removeFromLeft (12);
        secRight = r;

        {
            auto a = secSample.reduced (10).withTrimmedTop (22);
            auto row = a.removeFromTop (28);
            loadBtn.setBounds (row.removeFromLeft (84));  row.removeFromLeft (6);
            recBtn.setBounds (row.removeFromLeft (84));   row.removeFromLeft (6);
            stopBtn.setBounds (row.removeFromLeft (70));  row.removeFromLeft (6);
            toWtBtn.setBounds (row.removeFromLeft (130)); row.removeFromLeft (6);
            clearBtn.setBounds (row.removeFromLeft (76));
            recInfo = row.reduced (10, 0);
            a.removeFromTop (8);
            wave.setBounds (a.removeFromTop (138));
            a.removeFromTop (6);
            {
                auto er = a.removeFromTop (24);
                auto put = [&er] (juce::Component& c, int w) { c.setBounds (er.removeFromLeft (w)); er.removeFromLeft (4); };
                put (trimBtn, 80); put (cutBtn, 58); put (muteBtn, 50); put (fadeInBtn, 56); put (fadeOutBtn, 64);
                put (normBtn, 44); put (revBtn, 50);
                auto right = er;
                zoomAllBtn.setBounds (right.removeFromRight (50)); right.removeFromRight (4);
                zoomInBtn.setBounds (right.removeFromRight (26));  right.removeFromRight (4);
                zoomOutBtn.setBounds (right.removeFromRight (26)); right.removeFromRight (8);
                redoBtn.setBounds (right.removeFromRight (28));    right.removeFromRight (4);
                undoBtn.setBounds (right.removeFromRight (28));
                selToMarkBtn.setBounds (right.removeFromLeft (juce::jmax (0, right.getWidth() - 8)));
            }
            a.removeFromTop (4);
            row = a.removeFromTop (cellH);
            const int cw = 66;
            place (*start,  row.removeFromLeft (cw));
            place (*end,    row.removeFromLeft (cw));
            place (*loop,   row.removeFromLeft (104));
            place (*loopSt, row.removeFromLeft (cw));
            place (*root,   row.removeFromLeft (cw + 6));
            place (*volume, row.removeFromLeft (cw + 6));
            row.removeFromLeft (8);
            auto tg = row.removeFromLeft (120).withTrimmedTop (14);
            keyTrk->b.setBounds (tg.removeFromTop (26));
            tg.removeFromTop (8);
            oneShot->b.setBounds (tg.removeFromTop (26));
            a.removeFromTop (4);
            hint = a;
        }
        {
            auto tabs = secRight.reduced (10, 8).removeFromTop (22);
            inputTab.setBounds (tabs.removeFromRight (150));
            tabs.removeFromRight (6);
            browseTab.setBounds (tabs.removeFromRight (100));
            browser.setBounds (secRight.reduced (10).withTrimmedTop (28));
            auto a = secRight.reduced (10).withTrimmedTop (28);
            subIn = a.removeFromTop (16);
            auto row = a.removeFromTop (cellH);
            placeTight (*inMode, row.removeFromLeft (128).withTrimmedTop (8));
            place (*inGain, row.removeFromLeft (66));
            inMeter.setBounds (row.removeFromLeft (26).reduced (2, 6));
            row.removeFromLeft (10);
            auto tg = row.withTrimmedTop (14);
            inAuto->b.setBounds (tg.removeFromTop (26));
            tg.removeFromTop (8);
            inMon->b.setBounds (tg.removeFromTop (26));
            a.removeFromTop (6);
            subWt = a.removeFromTop (16);
            row = a.removeFromTop (26);
            osc1Btn.setBounds (row.removeFromLeft (62)); row.removeFromLeft (4);
            osc2Btn.setBounds (row.removeFromLeft (62)); row.removeFromLeft (4);
            osc3Btn.setBounds (row.removeFromLeft (62));
            wtLoadBtn.setBounds (row.removeFromRight (110));
            a.removeFromTop (8);
            wtView.setBounds (a.removeFromTop (150));
            hintWt = a.withTrimmedTop (4);
        }
    }

    void paint (juce::Graphics& g) override
    {
        drawSection (g, secSample, "SAMPLE");
        drawSection (g, secRight,  rightPage == 0 ? "LOOPS" : "INPUT");
        if (rightPage == 1)
        {
            drawSubtitle (g, subIn, ParamChoices::u8 ("AUFNAHME-EINGANG"));
            drawSubtitle (g, subWt, "WAVETABLE DES OSZILLATORS");
        }

        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 9.5f, 0.12f));
        g.setColour (recColour());
        g.drawText (recText(), recInfo, juce::Justification::centredLeft);

        drawHint (g, hint, ParamChoices::u8 (
            "SPIELEN: AUF DER SYNTH-SEITE BEI EINEM OSZILLATOR \xc2\xbbSAMPLE\xc2\xab W\xc3\x84HLEN. "
            "MARKEN S / E / LOOP ZIEHEN, SONST ZIEHEN = AUSWAHL (DOPPELKLICK = ALLES). MAUSRAD / TRACKPAD = ZOOM, SEITLICH WISCHEN = VERSCHIEBEN. "
            "SCHNITTE LANDEN AUF NULLDURCHG\xc3\x84NGEN, DAS ORIGINAL BLEIBT ERHALTEN (\xc2\xbb(BEARBEITET)\xc2\xab IM ORDNER K-54 S SAMPLES)."));
        if (rightPage == 1)
        drawHint (g, hintWt, proc.hasAudioInput()
            ? ParamChoices::u8 ("TABELLE \xc2\xbbUSER\xc2\xab = DEINE EIGENE WAVETABLE (DATEI ODER \xc2\xbb\xe2\x86\x92 WAVETABLE\xc2\xab). IN LOGIC DEN EINGANG \xc3\x9c" "BER \xc2\xbbSIDECHAIN\xc2\xab OBEN IM PLUG-IN-FENSTER W\xc3\x84HLEN.")
            : ParamChoices::u8 ("KEIN AUDIO-EINGANG AKTIV: IN DER DAW DEN SIDECHAIN-EINGANG W\xc3\x84HLEN, IN DER APP UNTER OPTIONS \xe2\x86\x92 AUDIO DAS INTERFACE."));
    }

private:
    juce::String recText() const
    {
        switch (proc.getRecState())
        {
            case K55Processor::recArmed:   return ParamChoices::u8 ("\xe2\x97\x8f WARTE AUF SIGNAL ...");
            case K55Processor::recRunning: return ParamChoices::u8 ("\xe2\x97\x8f AUFNAHME ") + juce::String (proc.getRecordedSeconds(), 1) + " S";
            case K55Processor::recStopping:
            case K55Processor::recFinished: return "SPEICHERE ...";
            default: break;
        }
        return lastError;
    }
    juce::Colour recColour() const
    {
        const int st = proc.getRecState();
        if (st == K55Processor::recRunning) return juce::Colour (0xffE0503A);
        if (st == K55Processor::recArmed) return (juce::Time::getMillisecondCounter() / 400) % 2 ? juce::Colour (0xffE0503A) : K54Colours::creamDim;
        return K54Colours::creamDim;
    }

    void chooseFile (bool wavetable)
    {
        chooser = std::make_unique<juce::FileChooser> (wavetable ? "Wavetable laden" : "Sample laden",
                                                        lastFolder.isDirectory() ? lastFolder : juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                        proc.formats.getWildcardForAllFormats());
        juce::Component::SafePointer<SamplePanel> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe, wavetable] (const juce::FileChooser& fc)
        {
            if (safe == nullptr || fc.getResult() == juce::File()) return;
            safe->loadFile (fc.getResult(), wavetable);
        });
    }

public:
    void loadFile (const juce::File& f, bool wavetable)
    {
        juce::String err;
        lastFolder = f.getParentDirectory();
        const bool ok = wavetable ? proc.loadWavetableFile (f, err) : proc.loadSampleFile (f, err);
        lastError = ok ? juce::String() : err.toUpperCase();
        repaint();
    }

private:
    void timerCallback() override
    {
        const int st = proc.getRecState();
        recBtn.setToggleState (st == K55Processor::recArmed || st == K55Processor::recRunning, juce::dontSendNotification);
        stopBtn.setEnabled (st == K55Processor::recArmed || st == K55Processor::recRunning);
        const bool hasSample = proc.getSample() != nullptr;
        toWtBtn.setEnabled (hasSample);
        clearBtn.setEnabled (hasSample);
        if (hasSample != lastHasSample) { lastHasSample = hasSample; updateEditButtons(); }
        recBtn.setEnabled (proc.hasAudioInput());
        inMeter.setLevels (proc.inPeak[0].exchange (0.0f), proc.inPeak[1].exchange (0.0f));
        if (st != lastRec || st == K55Processor::recRunning || st == K55Processor::recArmed) { lastRec = st; repaint (recInfo); }

        const int sv = proc.sourceVersion.load();
        if (sv != lastSource) { lastSource = sv; wave.repaint(); }

        // Wavetable-Ansicht fuer den gewaehlten Oszillator
        static const char* wIds[] { ParamID::osc1Wave, ParamID::osc2Wave, ParamID::osc3Wave };
        static const char* tIds[] { ParamID::osc1Table, ParamID::osc2Table, ParamID::osc3Table };
        static const char* pIds[] { ParamID::osc1Pos, ParamID::osc2Pos, ParamID::osc3Pos };
        const int w = juce::roundToInt (proc.apvts.getRawParameterValue (wIds[viewOsc])->load());
        const int t = juce::roundToInt (proc.apvts.getRawParameterValue (tIds[viewOsc])->load());
        const float p = proc.apvts.getRawParameterValue (pIds[viewOsc])->load();
        wtView.setTable (proc.getTableForDisplay (t), p, w == ParamChoices::wTable);
        if (sv != lastWtSource) { lastWtSource = sv; wtView.repaint(); }
        if (isShowing()) wave.repaint();
    }

    void updateEditButtons()
    {
        const bool has = proc.getSample() != nullptr, sel = has && wave.hasSelection();
        for (auto* b : { &trimBtn, &cutBtn, &muteBtn, &fadeInBtn, &fadeOutBtn, &selToMarkBtn }) b->setEnabled (sel);
        for (auto* b : { &normBtn, &revBtn, &zoomInBtn, &zoomOutBtn, &zoomAllBtn }) b->setEnabled (has);
        undoBtn.setEnabled (proc.canUndoSampleEdit());
        redoBtn.setEnabled (proc.canRedoSampleEdit());
        zoomAllBtn.setButtonText (sel && ! wave.isZoomed() ? "AUSW." : "ALLES");
    }

    SampleWaveView wave;
    WavetableView wtView;
    juce::TextButton trimBtn { "ZUSCHNEIDEN" }, cutBtn { ParamChoices::u8 ("L\xc3\x96SCHEN") }, muteBtn { "STILLE" },
                     fadeInBtn { "FADE IN" }, fadeOutBtn { "FADE OUT" }, normBtn { "NORM" }, revBtn { ParamChoices::u8 ("\xe2\x87\x86 REV") },
                     undoBtn { ParamChoices::u8 ("\xe2\x86\xb6") }, redoBtn { ParamChoices::u8 ("\xe2\x86\xb7") },
                     zoomOutBtn { ParamChoices::u8 ("\xe2\x88\x92") }, zoomInBtn { "+" }, zoomAllBtn { "ALLES" },
                     selToMarkBtn { "S/E SETZEN" };
    bool lastHasSample = false;
    MiniMeter inMeter;
    juce::TextButton loadBtn { "LADEN" }, recBtn { ParamChoices::u8 ("\xe2\x97\x8f REC") }, stopBtn { "STOP" }, toWtBtn { ParamChoices::u8 ("\xe2\x86\x92 WAVETABLE") },
                     clearBtn { "LEEREN" }, wtLoadBtn { "WT LADEN" }, osc1Btn { "OSC 1" }, osc2Btn { "OSC 2" }, osc3Btn { "OSC 3" };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File lastFolder;
    juce::String lastError;
    int viewOsc = 0, lastRec = -1, lastSource = -1, lastWtSource = -1;

    Knob *start = nullptr, *end = nullptr, *loopSt = nullptr, *root = nullptr, *inGain = nullptr, *volume = nullptr;
    Choice *loop = nullptr, *inMode = nullptr;
    Toggle *keyTrk = nullptr, *oneShot = nullptr, *inAuto = nullptr, *inMon = nullptr;
    juce::Rectangle<int> secSample, secRight, subIn, subWt, recInfo, hint, hintWt;
    SampleBrowser browser;
    juce::TextButton browseTab { "BROWSER" }, inputTab { "INPUT & WT" };
    int rightPage = 0;
};

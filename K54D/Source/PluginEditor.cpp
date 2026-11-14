#include "PluginEditor.h"

using namespace K54D;

// ============================================================================ OutMeter
void OutMeter::paint (juce::Graphics& g)
{
    using namespace K54Colours;
    auto b = getLocalBounds().toFloat();
    auto bars = b.removeFromTop (b.getHeight() - 12.0f);
    const float bh = (bars.getHeight() - 3.0f) * 0.5f;
    for (int c = 0; c < 2; ++c)
    {
        auto r = juce::Rectangle<float> (bars.getX(), bars.getY() + c * (bh + 3.0f), bars.getWidth(), bh);
        auto clipBox = r.removeFromRight (10.0f);
        r.removeFromRight (3.0f);
        g.setColour (panelHi);
        g.fillRoundedRectangle (r, 3.0f);
        juce::ColourGradient grad (goldDim, r.getX(), 0, juce::Colour (0xffE0503A), r.getRight(), 0, false);
        grad.addColour (norm (juce::Decibels::decibelsToGain (-6.0f)), gold);
        grad.addColour (norm (1.0f) - 0.001f, cream);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r.withWidth (r.getWidth() * norm (level[c])), 3.0f);
        g.setColour (cream);
        g.fillRect (r.getX() + r.getWidth() * norm (hold[c]) - 1.0f, r.getY(), 2.0f, r.getHeight());
        g.setColour (clip[c] ? juce::Colour (0xffE0503A) : panelHi);
        g.fillRoundedRectangle (clipBox, 3.0f);
    }
    g.setFont (K54Fonts::make (K54Fonts::syncopate(), 7.0f, 0.05f));
    g.setColour (creamDim.withAlpha (0.6f));
    const float w = bars.getWidth() - 13.0f;
    for (int db : { -36, -24, -12, -6, 0 })
    {
        const float x = bars.getX() + w * norm (juce::Decibels::decibelsToGain ((float) db));
        g.fillRect (x - 0.5f, b.getY(), 1.0f, 3.0f);
        g.drawText (juce::String (db), juce::Rectangle<float> (x - 12.0f, b.getY() + 2.0f, 24.0f, 10.0f), juce::Justification::centred);
    }
}

// ============================================================================ StepGrid
juce::Rectangle<int> StepGrid::cellBounds (int lane, int step) const
{
    const int steps = proc.stepsNow();
    const float cw = (float) stepsW() / (float) steps;
    const int x0 = stepsX0() + juce::roundToInt (step * cw);
    const int x1 = stepsX0() + juce::roundToInt ((step + 1) * cw);
    return { x0, lane * rowH, x1 - x0, rowH };
}

int StepGrid::stepAtX (int x) const
{
    if (x < stepsX0()) return -1;
    const int steps = proc.stepsNow();
    return juce::jlimit (0, steps - 1, (int) ((float) (x - stepsX0()) / ((float) stepsW() / (float) steps)));
}

void StepGrid::paint (juce::Graphics& g)
{
    using namespace K54Colours;
    const int steps = proc.stepsNow();
    const int spb = stepsPerBeat (juce::roundToInt (proc.apvts.getRawParameterValue (PID::res)->load()));
    const int sel = proc.selectedLane;

    for (int l = 0; l < DrumInfo::numLanes; ++l)
    {
        const auto row = juce::Rectangle<int> (0, l * rowH, getWidth(), rowH);
        const auto col = laneColour (l);
        const bool muted = proc.apvts.getRawParameterValue (PID::lane (l, "mute"))->load() > 0.5f;
        const bool solo  = proc.apvts.getRawParameterValue (PID::lane (l, "solo"))->load() > 0.5f;

        if (l == sel)
        {
            g.setColour (col.withAlpha (0.10f));
            g.fillRoundedRectangle (row.toFloat().reduced (0.0f, 1.0f), 5.0f);
        }

        // --- Spurkopf: LED, Name, Modell, M / S
        auto head = row.withWidth (headW).reduced (4, 0);
        const float fl = flash[(size_t) l];
        g.setColour (fl > 0.02f ? col.withAlpha (0.35f + 0.65f * fl) : line);
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ (float) head.getX() + 6.0f, (float) row.getCentreY() }));
        g.setColour (muted ? creamDim.withAlpha (0.4f) : (l == sel ? cream : cream.withAlpha (0.85f)));
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 9.5f, 0.08f));
        g.drawText (DrumInfo::shortNames[l], head.getX() + 16, row.getY(), 70, rowH, juce::Justification::centredLeft);
        const int model = juce::roundToInt (proc.apvts.getRawParameterValue (PID::lane (l, "model"))->load());
        auto mname = DrumInfo::models (l)[model].toUpperCase();
        if (model == DrumInfo::sampleModel (l))
        {
            const auto sn = proc.sampleName (l);
            mname = sn.isNotEmpty() ? sn.toUpperCase() : juce::String ("(LEER)");
        }
        g.setColour (l == sel ? col : creamDim.withAlpha (0.7f));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 7.5f, 0.06f));
        g.drawText (mname, head.getX() + 84, row.getY(), headW - 140, rowH, juce::Justification::centredLeft, true);

        for (int k = 0; k < 2; ++k)
        {
            const auto box = (k == 0 ? muteBox (l) : soloBox (l)).toFloat();
            const bool on = k == 0 ? muted : solo;
            g.setColour (on ? (k == 0 ? juce::Colour (0xffE0503A) : gold) : panelHi);
            g.fillRoundedRectangle (box, 3.0f);
            g.setColour (on ? black : creamDim.withAlpha (0.8f));
            g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 8.0f));
            g.drawText (k == 0 ? "M" : "S", box, juce::Justification::centred);
        }

        // --- Steps
        for (int s = 0; s < steps; ++s)
        {
            auto c = cellBounds (l, s);
            if ((s / spb) % 2 == 1)
            {
                g.setColour (juce::Colours::white.withAlpha (0.025f));
                g.fillRect (c);
            }
            auto cell = c.toFloat().reduced (1.5f, 2.5f);
            const int v = proc.patterns.vel (editPattern, l, s);
            g.setColour (s % spb == 0 ? panelHi.brighter (0.08f) : panelHi);
            g.fillRoundedRectangle (cell, 3.0f);
            if (v > 0)
            {
                const float f = (float) v / 127.0f;
                const auto c2 = muted ? creamDim.withAlpha (0.35f) : col;
                g.setColour (c2.withAlpha (0.25f + 0.25f * f));
                g.fillRoundedRectangle (cell, 3.0f);
                auto fill = cell.reduced (1.0f);
                fill = fill.removeFromBottom (juce::jmax (3.0f, fill.getHeight() * f));
                g.setColour (c2.withAlpha (0.55f + 0.45f * f));
                g.fillRoundedRectangle (fill, 2.0f);
                const int rat = proc.patterns.ratchet (editPattern, l, s);
                if (rat > 1)
                {
                    g.setColour (black.withAlpha (0.8f));
                    for (int k = 0; k < rat; ++k)
                        g.fillEllipse (cell.getX() + 3.0f + k * 4.5f, cell.getY() + 2.5f, 3.0f, 3.0f);
                }
            }
        }
    }

    // --- Laufposition
    if (playStep >= 0 && playStep < steps && proc.getPlayingPattern() == editPattern)
    {
        auto c = cellBounds (0, playStep);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRect (c.getX(), 0, c.getWidth(), DrumInfo::numLanes * rowH);
        g.setColour (gold);
        g.fillRect (c.getX() + 2, 0, c.getWidth() - 4, 2);
    }
}

void StepGrid::toggleMuteSolo (int lane, bool solo)
{
    if (auto* p = proc.apvts.getParameter (PID::lane (lane, solo ? "solo" : "mute")))
        p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
    repaint();
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    const int lane = laneAtY (e.y);
    if (lane < 0) return;
    dragMode = 0;

    if (e.x < headW)
    {
        if (muteBox (lane).contains (e.getPosition()))      toggleMuteSolo (lane, false);
        else if (soloBox (lane).contains (e.getPosition())) toggleMuteSolo (lane, true);
        else
        {
            if (onLaneSelected) onLaneSelected (lane);
            proc.triggerPad (lane, 0.85f);
        }
        return;
    }

    const int step = stepAtX (e.x);
    if (step < 0) return;
    if (onLaneSelected && lane != proc.selectedLane) onLaneSelected (lane);

    if (e.mods.isPopupMenu()) { stepMenu (lane, step); return; }

    dragLane = lane; dragStep = step;
    const int v = proc.patterns.vel (editPattern, lane, step);
    if (e.mods.isShiftDown() || e.mods.isAltDown())
    {
        dragMode = 3;
        dragStartVel = v > 0 ? v : lastVel;
        dragStartY = (float) e.y;
        if (v == 0) proc.patterns.setVel (editPattern, lane, step, dragStartVel);
    }
    else if (v > 0)
    {
        dragMode = 2;
        proc.patterns.setVel (editPattern, lane, step, 0);
        proc.patterns.setRatchet (editPattern, lane, step, 1);
    }
    else
    {
        dragMode = 1;
        proc.patterns.setVel (editPattern, lane, step, lastVel);
        if (! proc.isRunning()) proc.triggerPad (lane, (float) lastVel / 127.0f);
    }
    repaint();
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragLane < 0) return;
    if (dragMode == 3)
    {
        const int v = juce::jlimit (1, 127, dragStartVel + juce::roundToInt ((dragStartY - (float) e.y) * 1.2f));
        proc.patterns.setVel (editPattern, dragLane, dragStep, v);
        lastVel = v;
        repaint();
        return;
    }
    const int step = stepAtX (e.x);
    if (step < 0) return;
    if (dragMode == 1 && proc.patterns.vel (editPattern, dragLane, step) == 0)
        proc.patterns.setVel (editPattern, dragLane, step, lastVel);
    else if (dragMode == 2)
        proc.patterns.setVel (editPattern, dragLane, step, 0);
    repaint();
}

void StepGrid::mouseUp (const juce::MouseEvent&) { dragLane = -1; dragMode = 0; }

void StepGrid::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int lane = laneAtY (e.y), step = stepAtX (e.x);
    if (lane < 0 || step < 0) return;
    const int v = proc.patterns.vel (editPattern, lane, step);
    if (v <= 0) return;
    const int nv = juce::jlimit (1, 127, v + (w.deltaY > 0 ? 6 : -6));
    proc.patterns.setVel (editPattern, lane, step, nv);
    lastVel = nv;
    repaint();
}

void StepGrid::mouseMove (const juce::MouseEvent& e)
{
    if (e.x < headW)
        setTooltip (u8 ("Klick auf den Namen: Spur wählen und vorhören · M = stumm · S = solo"));
    else
        setTooltip (u8 ("Klick = Step an/aus · ziehen = mehrere · Shift + ziehen oder Mausrad = lauter/leiser · Rechtsklick = Lautstärke, Wirbel"));
}

void StepGrid::stepMenu (int lane, int step)
{
    juce::PopupMenu m;
    const int v = proc.patterns.vel (editPattern, lane, step);
    const int rat = proc.patterns.ratchet (editPattern, lane, step);
    m.addSectionHeader (juce::String (DrumInfo::names[lane]) + " - Step " + juce::String (step + 1));
    const std::pair<int, const char*> vels[] { { 127, "Akzent (127)" }, { 100, "Normal (100)" }, { 72, "Mittel (72)" }, { 42, "Ghost (42)" }, { 22, "Ganz leise (22)" } };
    for (auto& p : vels) m.addItem (p.first, u8 (p.second), true, v == p.first);
    m.addSeparator();
    for (int r = 1; r <= 4; ++r) m.addItem (1000 + r, r == 1 ? u8 ("Einzelschlag") : "Wirbel " + juce::String (r) + "x", true, v > 0 && rat == r);
    m.addSeparator();
    m.addItem (999, u8 ("Step löschen"), v > 0);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
        [this, lane, step] (int r)
        {
            if (r <= 0) return;
            if (r == 999) { proc.patterns.setVel (editPattern, lane, step, 0); proc.patterns.setRatchet (editPattern, lane, step, 1); }
            else if (r > 1000)
            {
                if (proc.patterns.vel (editPattern, lane, step) == 0) proc.patterns.setVel (editPattern, lane, step, lastVel);
                proc.patterns.setRatchet (editPattern, lane, step, r - 1000);
            }
            else { proc.patterns.setVel (editPattern, lane, step, r); lastVel = r; }
            repaint();
        });
}

// ============================================================================ VelocityLane
void VelocityLane::paint (juce::Graphics& g)
{
    using namespace K54Colours;
    const int lane = proc.selectedLane;
    const int steps = proc.stepsNow();
    const auto col = laneColour (lane);
    auto area = getLocalBounds().toFloat();
    g.setColour (panel.brighter (0.03f));
    g.fillRoundedRectangle (area, 6.0f);
    // Hilfslinien 25 / 50 / 75 / 100 %
    g.setColour (line.withAlpha (0.6f));
    for (float f : { 0.25f, 0.5f, 0.75f })
        g.fillRect (area.getX(), area.getBottom() - 4.0f - (area.getHeight() - 8.0f) * f, area.getWidth(), 1.0f);

    for (int s = 0; s < steps; ++s)
    {
        auto c = grid.cellBounds (0, s).withY (0).withHeight (getHeight()).translated (-grid.stepsX0() + 0, 0).toFloat();
        c = c.withX (c.getX()).reduced (3.0f, 4.0f);
        const int v = proc.patterns.vel (editPattern, lane, s);
        if (v <= 0)
        {
            g.setColour (line);
            g.fillRect (c.getX(), c.getBottom() - 2.0f, c.getWidth(), 2.0f);
            continue;
        }
        const float f = (float) v / 127.0f;
        auto bar = c.withTop (c.getBottom() - c.getHeight() * f);
        g.setGradientFill (juce::ColourGradient (col, 0, bar.getY(), col.withAlpha (0.45f), 0, bar.getBottom(), false));
        g.fillRoundedRectangle (bar, 2.5f);
        g.setColour (cream.withAlpha (0.85f));
        g.fillRect (bar.getX(), bar.getY(), bar.getWidth(), 2.0f);
        if (c.getWidth() > 20.0f)
        {
            g.setColour (cream.withAlpha (0.6f));
            g.setFont (K54Fonts::make (K54Fonts::syncopate(), 7.0f));
            g.drawText (juce::String (v), bar.withHeight (12.0f).translated (0, -13.0f).withY (juce::jmax (c.getY(), bar.getY() - 13.0f)), juce::Justification::centred);
        }
    }
}

void VelocityLane::paintAt (const juce::MouseEvent& e)
{
    const int step = grid.stepAtX (e.x + grid.stepsX0());
    if (step < 0) return;
    const int lane = proc.selectedLane;
    if (proc.patterns.vel (editPattern, lane, step) <= 0) return;
    const float f = 1.0f - (float) (e.y - 4) / (float) juce::jmax (1, getHeight() - 8);
    const int v = juce::jlimit (1, 127, juce::roundToInt (f * 127.0f));
    proc.patterns.setVel (editPattern, lane, step, v);
    grid.lastVel = v;
    repaint();
    grid.repaint();
}

void VelocityLane::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int step = grid.stepAtX (e.x + grid.stepsX0());
    if (step < 0 || proc.patterns.vel (editPattern, proc.selectedLane, step) <= 0) return;
    proc.patterns.setVel (editPattern, proc.selectedLane, step, 100);
    repaint(); grid.repaint();
}

// ============================================================================ Main
K54DMain::K54DMain (K54DProcessor& p) : proc (p), grid (p), velLane (p, grid)
{
    setLookAndFeel (&lnf);
    setOpaque (true);

    juce::PropertiesFile::Options o;
    o.applicationName = "K-54 D"; o.folderName = "Keller54"; o.filenameSuffix = "settings"; o.osxLibrarySubFolder = "Application Support";
    settings = std::make_unique<juce::PropertiesFile> (o);
    K54View::colorful = settings->getBoolValue ("colorful", true);
    portrait = juce::ImageCache::getFromMemory (BinaryData::frank_portrait_png, BinaryData::frank_portrait_pngSize);

    // ---- Kopf
    addAndMakeVisible (outMeter);
    outMeter.setTooltip ("Main-Ausgang L/R - Klick setzt die Clip-Anzeige zurueck");
    masterGain.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    masterGain.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 13);
    masterGain.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    masterGain.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
    masterGain.setMouseDragSensitivity (220);
    masterGain.setTooltip (u8 ("Gesamtlautstärke (Master)"));
    addAndMakeVisible (masterGain);
    masterAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, PID::master, masterGain);
    masterGain.setDoubleClickReturnValue (true, 0.0);

    for (auto* b : { &prevButton, &nextButton, &saveButton, &colorButton, &helpButton, &updateButton })
        addAndMakeVisible (b);
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (+1); };
    saveButton.onClick = [this] { savePresetDialog(); };
    saveButton.setTooltip ("Kit + alle vier Patterns als eigenes Preset speichern");
    addAndMakeVisible (presetBox);
    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.onChange = [this] { presetChosen(); };
    presetBox.setTooltip (u8 ("Presets nach Stil: von 808 und 909 bis Jazz, House, Disco, Schlager, Hip-Hop ..."));
    rebuildPresetMenu();

    colorButton.setClickingTogglesState (true);
    colorButton.setToggleState (K54View::colorful, juce::dontSendNotification);
    colorButton.setTooltip (u8 ("Farbige oder schlichte Oberfläche"));
    colorButton.onClick = [this]
    {
        K54View::colorful = colorButton.getToggleState();
        settings->setValue ("colorful", K54View::colorful);
        settings->saveIfNeeded();
        repaint();
    };
    helpButton.setTooltip ("Kurze Hilfe");
    helpButton.onClick = [this] { showHelp(); };
    updateButton.setTooltip ("Neue Version von keller54.de laden - deine Presets und Samples bleiben erhalten");
    updateButton.onClick = [this] { updatePanel.setVisible (true); updatePanel.toFront (true); };
    updatePanel.onClose = [this] { updatePanel.setVisible (false); };
    addChildComponent (updatePanel);
    updater->checkNow (false);

    addAndMakeVisible (sizeBox);
    for (int pct : { 70, 85, 100, 115, 130, 150 }) sizeBox.addItem (juce::String (pct) + " %", pct);
    sizeBox.setText (juce::String (juce::roundToInt (proc.uiScale * 100.0f)) + " %", juce::dontSendNotification);
    sizeBox.setTooltip (u8 ("Fenstergröße"));
    sizeBox.onChange = [this] { if (onScaleChosen && sizeBox.getSelectedId() > 0) onScaleChosen ((float) sizeBox.getSelectedId() / 100.0f); };

    // ---- Transport
    seqButton.setClickingTogglesState (true);
    seqButton.setTooltip (u8 ("Sequencer an: das Pattern läuft synchron zur DAW. Aus: nur per MIDI-Noten spielen (GM-Belegung, Kick = C1)"));
    addAndMakeVisible (seqButton);
    seqAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, PID::seqOn, seqButton);
    playButton.setClickingTogglesState (true);
    playButton.setTooltip (u8 ("Ohne DAW-Wiedergabe abspielen (im DAW-Tempo). Läuft die DAW, folgt der K-54 D automatisch ihrem Takt."));
    playButton.onClick = [this] { proc.internalPlay = playButton.getToggleState(); };
    addAndMakeVisible (playButton);

    auto setupSmallLabel = [this] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.2f));
        l.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (l);
    };
    setupSmallLabel (stepsLabel, "STEPS");
    setupSmallLabel (resLabel, "RASTER");
    setupSmallLabel (patLabel, "PATTERN");
    for (int s = 8; s <= DrumInfo::maxSteps; ++s) stepsBox.addItem (juce::String (s), s - 7);
    stepsBox.setTooltip (u8 ("Pattern-Länge: 8 bis 32 Steps"));
    addAndMakeVisible (stepsBox);
    stepsAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, PID::steps, stepsBox);
    resBox.addItemList (Res::names, 1);
    resBox.setTooltip (u8 ("Notenwert eines Steps (T = Triole, z.B. für Swing, Shuffle und Jazz)"));
    addAndMakeVisible (resBox);
    resAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, PID::res, resBox);

    const char* patTips[4] { "A - Grundgroove", "B - mit Fill am Ende", "C - Break (ohne Kick und Snare)", "D - mit Crash auf der Eins" };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = patButtons[(size_t) i];
        b.setButtonText (juce::String::charToString ((juce::juce_wchar) ('A' + i)));
        b.setTooltip (u8 (patTips[i]) + u8 (" · wechselt am Pattern-Ende · MIDI: C0, C#0, D0, D#0"));
        b.onClick = [this, i] { setEditPattern (i); };
        addAndMakeVisible (b);
    }
    copyButton.setTooltip ("Pattern kopieren");
    copyButton.onClick = [this]
    {
        juce::PopupMenu m;
        const int cur = grid.editPattern;
        const auto from = juce::String::charToString ((juce::juce_wchar) ('A' + cur));
        for (int t = 0; t < 4; ++t)
            if (t != cur) m.addItem (1 + t, "Pattern " + from + " nach " + juce::String::charToString ((juce::juce_wchar) ('A' + t)) + " kopieren");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&copyButton), [this, cur] (int r)
        {
            if (r > 0) { proc.patterns.copy (cur, r - 1); setEditPattern (r - 1); }
        });
    };
    clearButton.setTooltip (u8 ("Spur oder Pattern löschen, Varianten neu erzeugen"));
    clearButton.onClick = [this]
    {
        juce::PopupMenu m;
        const auto pat = juce::String::charToString ((juce::juce_wchar) ('A' + grid.editPattern));
        m.addItem (1, u8 ("Spur ") + DrumInfo::names[proc.selectedLane] + u8 (" löschen"));
        m.addItem (2, u8 ("Pattern ") + pat + u8 (" löschen"));
        m.addSeparator();
        m.addItem (3, u8 ("B, C und D neu aus A erzeugen (Fill, Break, Crash)"));
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&clearButton), [this] (int r)
        {
            if (r == 1) proc.patterns.clearLane (grid.editPattern, proc.selectedLane);
            if (r == 2) proc.patterns.clear (grid.editPattern);
            if (r == 3) Presets::makeVariations (proc.patterns, proc.stepsNow());
            grid.repaint(); velLane.repaint();
        });
    };
    addAndMakeVisible (copyButton);
    addAndMakeVisible (clearButton);

    midiButton.setButtonText (u8 ("MIDI ⇣"));
    midiButton.setTooltip (u8 ("In die DAW ziehen = Pattern als MIDI-Clip · Klick = als .mid speichern"));
    midiButton.makeFile = [this] { return proc.exportMidi (grid.editPattern); };
    midiButton.onSaveClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("MIDI speichern",
            juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile (proc.exportMidi (grid.editPattern).getFileName()), "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
            [this] (const juce::FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (f != juce::File()) proc.exportMidi (grid.editPattern, f.withFileExtension ("mid"));
            });
    };
    addAndMakeVisible (midiButton);

    // ---- Raster
    grid.onLaneSelected = [this] (int l) { selectLane (l); };
    addAndMakeVisible (grid);
    addAndMakeVisible (velLane);
    const char* toolNames[6] { "+ LAUTER", "", "GLEICH", "BETONT", "ANSTIEG", "ZUFALL" };
    const char* toolTips[6] { "Alle Steps der Spur lauter", "Alle Steps der Spur leiser", "Alle Steps gleich laut (100)",
                              "Schlag betont, Zwischenschlaege leiser", "Von leise nach laut (fuer Fills)", "Leicht zufaellig - klingt lebendiger" };
    for (int i = 0; i < 6; ++i)
    {
        auto& b = velTools[(size_t) i];
        b.setButtonText (i == 1 ? u8 ("− LEISER") : juce::String (toolNames[i]));
        b.getProperties().set ("fontSize", 7.5f);
        b.setTooltip (u8 (toolTips[i]));
        b.onClick = [this, i] { velocityTool (i); };
        addAndMakeVisible (b);
    }

    // ---- Sound
    modelBox.setTooltip (u8 ("Klang-Modell der Spur (808, 909, Linn-Stil, akustisch ...) oder eigenes Sample"));
    addAndMakeVisible (modelBox);
    loadButton.setTooltip (u8 ("WAV/AIFF/MP3/FLAC für diese Spur laden - oder Datei direkt auf eine Spur ziehen"));
    loadButton.onClick = [this] { loadSampleDialog(); };
    clearSampleButton.setTooltip ("Sample entfernen");
    clearSampleButton.onClick = [this]
    {
        proc.clearSample (proc.selectedLane);
        if (auto* prm = proc.apvts.getParameter (PID::lane (proc.selectedLane, "model"))) prm->setValueNotifyingHost (0.0f);
        repaint(); grid.repaint();
    };
    auditionButton.setTooltip (u8 ("Vorhören"));
    auditionButton.onClick = [this] { proc.triggerPad (proc.selectedLane, 0.9f); };
    for (auto* b : { &loadButton, &clearSampleButton, &auditionButton }) addAndMakeVisible (b);

    const char* laneCaps[8] { "Level", "Tune", "Decay", "Tone", "Snap", "Drive", "Pan", "Room" };
    const juce::String tips[8] { u8 ("Lautstärke der ganzen Spur"), u8 ("Stimmung in Halbtönen"), u8 ("Ausklingen: kurz bis lang"),
                                 u8 ("dunkler / heller"), u8 ("Anschlag, Klick und Snare-Teppich"), u8 ("Sättigung - dicker und fetter"),
                                 u8 ("Stereo-Position"), u8 ("Anteil im Raumhall") };
    for (int i = 0; i < 8; ++i) setupKnob (laneKnobs[(size_t) i], laneCaps[i], tips[i], i == 1 || i == 3 || i == 6);
    setupKnob (grooveKnobs[0], "Swing", u8 ("Swing: jeder zweite Step später (50 % = gerade)"));
    setupKnob (grooveKnobs[1], "Human", u8 ("Humanize: kleine Zufallsschwankungen in Timing und Lautstärke"));
    setupKnob (masterKnobs[0], "Fat", u8 ("Fett: Bass-Anhebung und analoge Sättigung auf der Summe"));
    setupKnob (masterKnobs[1], "Glue", u8 ("Bus-Kompressor: klebt den Groove zusammen"));
    setupKnob (masterKnobs[2], "Room", u8 ("Größe des Raumhalls (Anteil pro Spur mit ROOM)"));
    const char* gIds[2] { PID::swing, PID::humanize };
    const char* mIds[3] { PID::fat, PID::glue, PID::roomSize };
    for (int i = 0; i < 2; ++i) grooveKnobs[(size_t) i].a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, gIds[i], grooveKnobs[(size_t) i].s);
    for (int i = 0; i < 3; ++i) masterKnobs[(size_t) i].a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, mIds[i], masterKnobs[(size_t) i].s);

    setSize (W, H);
    refreshAll();
    startTimerHz (30);
}

K54DMain::~K54DMain()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void K54DMain::setupKnob (Knob& k, const juce::String& caption, const juce::String& tip, bool bipolar)
{
    auto& s = k.s;
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 15);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
    s.setMouseDragSensitivity (220);
    s.getProperties().set ("bipolar", bipolar);
    s.setTooltip (tip);
    addAndMakeVisible (s);
    k.l.setText (caption.toUpperCase(), juce::dontSendNotification);
    k.l.setJustificationType (juce::Justification::centred);
    k.l.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.08f));
    addAndMakeVisible (k.l);
}

void K54DMain::attachLane (int lane)
{
    static const char* ids[8] { "level", "tune", "decay", "tone", "snap", "drive", "pan", "room" };
    for (int i = 0; i < 8; ++i)
    {
        auto& k = laneKnobs[(size_t) i];
        k.a.reset();
        const auto id = PID::lane (lane, ids[i]);
        k.a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.s);
        if (auto* prm = proc.apvts.getParameter (id))
            k.s.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    }
    modelAtt.reset();
    modelBox.clear (juce::dontSendNotification);
    const auto names = DrumInfo::models (lane);
    for (int i = 0; i < names.size(); ++i) modelBox.addItem (names[i].toUpperCase(), i + 1);
    modelAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, PID::lane (lane, "model"), modelBox);
    modelBox.onChange = [this] { grid.repaint(); repaint (sampleNameArea); };
    shownLane = lane;
}

void K54DMain::selectLane (int lane)
{
    proc.selectedLane = juce::jlimit (0, DrumInfo::numLanes - 1, lane);
    attachLane (proc.selectedLane);
    grid.repaint();
    velLane.repaint();
    repaint (secSound);
    repaint (velHead);
}

juce::AudioParameterChoice* K54DMain::patternParam() const
{
    return dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (PID::pattern));
}

void K54DMain::setEditPattern (int p)
{
    if (auto* prm = patternParam())
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) p));
    proc.selectPattern (p);
    grid.editPattern = p; velLane.editPattern = p;
    grid.repaint(); velLane.repaint();
}

void K54DMain::refreshAll()
{
    rebuildPresetMenu();
    const int p = patternParam() != nullptr ? patternParam()->getIndex() : 0;
    grid.editPattern = p; velLane.editPattern = p;
    selectLane (proc.selectedLane);
    lastStateVersion = proc.stateVersion.load();
    repaint();
}

// ---------------------------------------------------------------- Presets
void K54DMain::rebuildPresetMenu()
{
    presetBox.clear (juce::dontSendNotification);
    auto* root = presetBox.getRootMenu();
    root->addItem (900, "INIT (LEER)");
    for (auto& cat : Presets::categories())
    {
        juce::PopupMenu sub;
        for (int i = 0; i < Presets::numFactory(); ++i)
            if (cat == Presets::factory (i).category)
                sub.addItem (1 + i, juce::String (Presets::factory (i).name).toUpperCase(), true, proc.currentFactory == i);
        root->addSubMenu (cat.toUpperCase(), sub);
    }
    const auto user = proc.userPresets();
    if (! user.isEmpty())
    {
        juce::PopupMenu sub;
        for (int i = 0; i < user.size(); ++i) sub.addItem (1000 + i, user[i].getFileNameWithoutExtension().toUpperCase());
        root->addSubMenu ("EIGENE", sub);
    }
    if (proc.currentFactory >= 0) presetBox.setSelectedId (1 + proc.currentFactory, juce::dontSendNotification);
    else presetBox.setText (proc.presetName.toUpperCase(), juce::dontSendNotification);
}

void K54DMain::presetChosen()
{
    const int id = presetBox.getSelectedId();
    if (id <= 0) return;
    if (id == 900) proc.loadFactory (-1);
    else if (id >= 1000)
    {
        const auto user = proc.userPresets();
        if (juce::isPositiveAndBelow (id - 1000, user.size())) proc.loadUserPreset (user[id - 1000]);
    }
    else proc.loadFactory (id - 1);
    refreshAll();
}

void K54DMain::stepPreset (int delta)
{
    const int n = Presets::numFactory();
    const int cur = proc.currentFactory < 0 ? (delta > 0 ? -1 : 0) : proc.currentFactory;
    proc.loadFactory (((cur + delta) % n + n) % n);
    refreshAll();
}

void K54DMain::savePresetDialog()
{
    auto* w = new juce::AlertWindow ("PRESET SPEICHERN", u8 ("Name für dein Preset (Kit + Patterns A-D + Samples):"), juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.presetName == "Init" ? juce::String() : proc.presetName);
    w->addButton ("SPEICHERN", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("ABBRECHEN", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
    {
        if (r == 1 && proc.saveUserPreset (w->getTextEditorContents ("name"))) rebuildPresetMenu();
    }), true);
}

void K54DMain::loadSampleDialog()
{
    chooser = std::make_unique<juce::FileChooser> ("Sample laden",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f.existsAsFile() && proc.loadSample (proc.selectedLane, f)) { grid.repaint(); repaint(); }
        });
}

void K54DMain::showHelp()
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "K-54 D - SO GEHT'S", u8 (
        "Preset oben rechts wählen und in der DAW auf Play drücken - der K-54 D läuft immer im Takt deiner DAW.\n\n"
        "RASTER: Klick = Step an/aus, ziehen = mehrere. Shift + ziehen oder Mausrad = lauter/leiser. "
        "Rechtsklick = Lautstärke und Wirbel (Hi-Hat-Rolls).\n\n"
        "LAUTSTÄRKE PRO STEP: unten die Balken der gewählten Spur ziehen. Mit BETONT, ANSTIEG usw. geht es schneller.\n\n"
        "STEPS: 8 bis 32. RASTER: 1/16, 1/8, Triolen für Swing und Jazz.\n\n"
        "PATTERN A-D: A Grundgroove, B mit Fill, C Break, D mit Crash. Wechsel immer am Pattern-Ende. "
        "MIDI-Noten C0 bis D#0 schalten um.\n\n"
        "SOUND: Spur anklicken, Modell wählen (808, 909, akustisch ...) oder eigenes Sample laden bzw. Datei auf die Spur ziehen.\n\n"
        "MIDI-Noten (GM) spielen die Spuren auch direkt: Kick C1, Snare D1, Clap D#1, Hi-Hat F#1 ...\n\n"
        "MIDI ⇣ in die DAW ziehen = Pattern als MIDI-Clip."));
}

void K54DMain::velocityTool (int which)
{
    const int lane = proc.selectedLane, steps = proc.stepsNow(), p = grid.editPattern;
    const int spb = stepsPerBeat (juce::roundToInt (proc.apvts.getRawParameterValue (PID::res)->load()));
    juce::Random rnd;
    int first = -1, last = -1;
    for (int s = 0; s < steps; ++s) if (proc.patterns.vel (p, lane, s) > 0) { if (first < 0) first = s; last = s; }
    for (int s = 0; s < steps; ++s)
    {
        const int v = proc.patterns.vel (p, lane, s);
        if (v <= 0) continue;
        int nv = v;
        switch (which)
        {
            case 0: nv = v + 12; break;
            case 1: nv = v - 12; break;
            case 2: nv = 100; break;
            case 3: nv = s % spb == 0 ? 118 : (s % spb == spb / 2 ? 88 : 62); break;
            case 4: nv = last > first ? 45 + (127 - 45) * (s - first) / (last - first) : 100; break;
            case 5: nv = v + rnd.nextInt ({ -14, 15 }); break;
            default: break;
        }
        proc.patterns.setVel (p, lane, s, juce::jlimit (1, 127, nv));
    }
    grid.repaint(); velLane.repaint();
}

// ---------------------------------------------------------------- Drag & Drop
bool K54DMain::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;mp3;flac;ogg")) return true;
    return false;
}

void K54DMain::filesDropped (const juce::StringArray& files, int x, int y)
{
    int lane = proc.selectedLane;
    const auto gp = grid.getLocalPoint (this, juce::Point<int> (x, y));
    if (grid.getLocalBounds().contains (gp)) lane = juce::jmax (0, grid.laneAtY (gp.y));
    for (auto& f : files)
        if (proc.loadSample (lane, juce::File (f)))
        {
            selectLane (lane);
            proc.triggerPad (lane, 0.9f);
            break;
        }
    repaint();
}

// ---------------------------------------------------------------- Timer
void K54DMain::timerCallback()
{
    if (proc.stateVersion.load() != lastStateVersion) refreshAll();

    outMeter.setLevels (proc.getMeter (0), proc.getMeter (1));

    const int pp = patternParam() != nullptr ? patternParam()->getIndex() : 0;
    if (pp != grid.editPattern) { grid.editPattern = pp; velLane.editPattern = pp; grid.repaint(); velLane.repaint(); }

    ++blink;
    const int playing = proc.getPlayingPattern(), pending = proc.getPendingPattern();
    for (int i = 0; i < 4; ++i)
    {
        auto& b = patButtons[(size_t) i];
        b.setToggleState (i == grid.editPattern, juce::dontSendNotification);
        const bool mark = proc.isRunning() && ((i == playing) || (i == pending && (blink / 6) % 2 == 0));
        b.setColour (juce::TextButton::textColourOffId, mark ? K54Colours::gold : K54Colours::cream);
    }
    playButton.setToggleState (proc.internalPlay.load(), juce::dontSendNotification);

    bool flashChanged = false;
    for (int l = 0; l < DrumInfo::numLanes; ++l)
    {
        const int h = proc.getLaneHits (l);
        if (h != lastHits[(size_t) l]) { lastHits[(size_t) l] = h; laneFlash[(size_t) l] = 1.0f; flashChanged = true; }
        else if (laneFlash[(size_t) l] > 0.0f) { laneFlash[(size_t) l] = juce::jmax (0.0f, laneFlash[(size_t) l] - 0.12f); flashChanged = true; }
        grid.setLaneFlash (l, laneFlash[(size_t) l]);
    }

    const int change = proc.patterns.getChangeCount();
    const int steps = proc.stepsNow();
    const int res = juce::roundToInt (proc.apvts.getRawParameterValue (PID::res)->load());
    if (change != lastChange || steps != lastSteps || res != lastRes)
    {
        lastChange = change; lastSteps = steps; lastRes = res;
        grid.repaint(); velLane.repaint();
    }
    else if (flashChanged)
        grid.repaint (0, 0, StepGrid::headW, grid.getHeight());
    grid.setPlayStep (proc.isRunning() ? proc.getPlayingStep() : -1);

    if (proc.selectedLane != shownLane) selectLane (proc.selectedLane);
    repaint (statusArea);
}

// ---------------------------------------------------------------- Layout
void K54DMain::resized()
{
    // Kopf
    outMeter.setBounds (244, 22, 176, 36);
    masterGain.setBounds (428, 8, 60, 64);
    {
        auto r = juce::Rectangle<int> (W - 14 - 350, 12, 350, 28);
        saveButton.setBounds (r.removeFromRight (60));
        r.removeFromRight (6);
        nextButton.setBounds (r.removeFromRight (28));
        r.removeFromRight (4);
        prevButton.setBounds (r.removeFromLeft (28));
        r.removeFromLeft (4);
        presetBox.setBounds (r);
        auto r2 = juce::Rectangle<int> (W - 14 - 350, 46, 350, 22);
        updateButton.setBounds (r2.removeFromRight (84));
        r2.removeFromRight (6);
        sizeBox.setBounds (r2.removeFromRight (78));
        r2.removeFromRight (6);
        helpButton.setBounds (r2.removeFromRight (28));
        r2.removeFromRight (6);
        colorButton.setBounds (r2.removeFromRight (74));
    }
    updatePanel.setBounds (getLocalBounds());

    // Transport
    {
        auto r = juce::Rectangle<int> (20, headerH + 10, W - 40, 26);
        seqButton.setBounds (r.removeFromLeft (54)); r.removeFromLeft (6);
        playButton.setBounds (r.removeFromLeft (64)); r.removeFromLeft (10);
        statusArea = r.removeFromLeft (232); r.removeFromLeft (2);
        stepsLabel.setBounds (r.removeFromLeft (52)); r.removeFromLeft (4);
        stepsBox.setBounds (r.removeFromLeft (62)); r.removeFromLeft (10);
        resLabel.setBounds (r.removeFromLeft (58)); r.removeFromLeft (4);
        resBox.setBounds (r.removeFromLeft (72)); r.removeFromLeft (10);
        patLabel.setBounds (r.removeFromLeft (64)); r.removeFromLeft (4);
        for (auto& b : patButtons) { b.setBounds (r.removeFromLeft (32)); r.removeFromLeft (4); }
        r.removeFromLeft (8);
        midiButton.setBounds (r.removeFromRight (84)); r.removeFromRight (6);
        clearButton.setBounds (r.removeFromRight (84)); r.removeFromRight (6);
        copyButton.setBounds (r.removeFromRight (90));
    }

    // Raster + Lautstaerke pro Step
    const int gy = headerH + 46;
    grid.setBounds (20, gy, W - 40, DrumInfo::numLanes * StepGrid::rowH);
    const int vy = grid.getBottom() + 8;
    velHead = juce::Rectangle<int> (20, vy, StepGrid::headW, 84);
    velLane.setBounds (20 + grid.stepsX0(), vy, grid.stepsW(), 84);
    {
        auto r = velHead.withTrimmedTop (30).reduced (2, 0);
        for (int row = 0; row < 2; ++row)
        {
            auto line = r.removeFromTop (24);
            r.removeFromTop (3);
            const int bw = (line.getWidth() - 8) / 3;
            for (int c = 0; c < 3; ++c)
            {
                velTools[(size_t) (row * 3 + c)].setBounds (line.removeFromLeft (bw));
                line.removeFromLeft (4);
            }
        }
    }

    // Sektionen unten
    const int sy = vy + 84 + 10;
    const int sh = H - 26 - sy;
    secSound  = { 20, sy, 664, sh };
    secGroove = { secSound.getRight() + 10, sy, 172, sh };
    secMaster = { secGroove.getRight() + 10, sy, W - 20 - (secGroove.getRight() + 10), sh };
    {
        auto r = secSound.reduced (14, 0).withTrimmedTop (34);
        auto top = r.removeFromTop (26);
        modelBox.setBounds (top.removeFromLeft (170));
        top.removeFromLeft (8);
        auditionButton.setBounds (top.removeFromLeft (34));
        top.removeFromLeft (8);
        loadButton.setBounds (top.removeFromLeft (124));
        top.removeFromLeft (6);
        clearSampleButton.setBounds (top.removeFromLeft (26));
        top.removeFromLeft (10);
        sampleNameArea = top;
        r.removeFromTop (8);
        const int kw = r.getWidth() / 8;
        for (auto& k : laneKnobs)
        {
            auto cell = r.removeFromLeft (kw);
            k.l.setBounds (cell.removeFromTop (14));
            k.s.setBounds (cell.withHeight (juce::jmin (cell.getHeight(), 72)));
        }
    }
    auto placeRow = [] (juce::Rectangle<int> sec, Knob* ks, int n)
    {
        auto r = sec.reduced (10, 0).withTrimmedTop (68);
        const int kw = r.getWidth() / n;
        for (int i = 0; i < n; ++i)
        {
            auto cell = r.removeFromLeft (kw);
            ks[i].l.setBounds (cell.removeFromTop (14));
            ks[i].s.setBounds (cell.withHeight (juce::jmin (cell.getHeight(), 72)));
        }
    };
    placeRow (secGroove, grooveKnobs.data(), 2);
    placeRow (secMaster, masterKnobs.data(), 3);
}

void K54DMain::drawSection (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    drawK55Section (g, r, title);
}

// ---------------------------------------------------------------- Zeichnen
void K54DMain::paint (juce::Graphics& g)
{
    using namespace K54Colours;
    g.fillAll (black);

    // ----- Kopfzeile wie K-54 S: Logo | Pegel + Gain | Portrait | Name | Presets
    drawKellerMeter (g, { 18.0f, 16.0f, 44.0f, 44.0f }, cream, gold);
    g.setColour (cream);
    g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 19.0f, 0.14f));
    g.drawText ("KELLER", 70, 22, 120, 34, juce::Justification::centredLeft);
    const int kellerW = (int) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), "KELLER");
    g.setColour (gold);
    g.setFont (K54Fonts::make (K54Fonts::monoton(), 36.0f));
    g.drawText ("54", 70 + kellerW + 8, 18, 70, 42, juce::Justification::centredLeft);

    g.setColour (creamDim.withAlpha (0.7f));
    g.setFont (K54Fonts::make (K54Fonts::syncopate(), 7.5f, 0.3f));
    g.drawText ("MAIN OUT", 244, 9, 120, 12, juce::Justification::centredLeft);

    if (portrait.isValid())
        g.drawImage (portrait, juce::Rectangle<float> (514.0f, 12.0f, 56.0f, 56.0f),
                     juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);

    g.setColour (cream);
    g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 28.0f, 0.18f));
    g.drawText ("K-54 D", juce::Rectangle<int> (586, 8, 210, 36), juce::Justification::centredLeft);
    g.setColour (gold);
    g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.3f));
    g.drawText ("BY FRANK E.", juce::Rectangle<int> (588, 42, 200, 12), juce::Justification::centredLeft);
    g.setColour (creamDim.withAlpha (0.7f));
    g.setFont (K54Fonts::make (K54Fonts::syncopate(), 7.0f, 0.25f));
    g.drawText (u8 ("DRUM COMPUTER \xc2\xb7 808 BIS HEUTE"), juce::Rectangle<int> (588, 56, 210, 12), juce::Justification::centredLeft);

    if (K54View::colorful)
    {
        g.setGradientFill (K54View::rainbowGradient (0.0f, (float) W, 0.0f, 0.85f));
        g.fillRect (0, headerH - 2, W, 2);
    }
    else
    {
        g.setColour (goldDim.withAlpha (0.6f));
        g.fillRect (0, headerH - 1, W, 1);
    }

    // ----- Status (Tempo / Sync)
    {
        juce::String st;
        const auto bpm = juce::String (proc.getHostBpm(), proc.getHostBpm() == std::floor (proc.getHostBpm()) ? 0 : 1) + " BPM";
        if (proc.isHostPlaying())          st = "DAW " + bpm + u8 (" \xc2\xb7 SYNC");
        else if (proc.internalPlay.load()) st = "PLAY " + bpm;
        else                               st = u8 ("STOP \xc2\xb7 ") + bpm;
        g.setColour (panelHi);
        g.fillRoundedRectangle (statusArea.toFloat(), 4.0f);
        g.setColour (proc.isRunning() ? gold : creamDim);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 8.5f, 0.12f));
        g.drawText (st, statusArea.reduced (8, 0), juce::Justification::centredLeft);
        if (proc.isRunning())
        {
            const int steps = proc.stepsNow();
            const int s = juce::jmax (0, proc.getPlayingStep());
            g.setColour (creamDim);
            g.drawText (juce::String (s + 1) + "/" + juce::String (steps), statusArea.reduced (8, 0), juce::Justification::centredRight);
        }
    }

    // ----- Kopf der Lautstaerke-Zeile
    {
        const int lane = proc.selectedLane;
        g.setColour (K54D::laneColour (lane));
        g.fillRoundedRectangle ((float) velHead.getX() + 2.0f, (float) velHead.getY() + 6.0f, 3.0f, 12.0f, 1.5f);
        g.setColour (cream);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 9.0f, 0.18f));
        g.drawText (u8 ("LAUTSTÄRKE PRO STEP"), velHead.getX() + 10, velHead.getY() + 2, velHead.getWidth(), 14, juce::Justification::centredLeft);
        g.setColour (K54D::laneColour (lane));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.18f));
        g.drawText (juce::String (DrumInfo::names[lane]).toUpperCase() + u8 ("  \xc2\xb7  PATTERN ") + juce::String::charToString ((juce::juce_wchar) ('A' + grid.editPattern)),
                    velHead.getX() + 10, velHead.getY() + 15, velHead.getWidth(), 12, juce::Justification::centredLeft);
    }

    // ----- Sektionen
    drawSection (g, secSound, u8 ("SOUND  \xc2\xb7  ") + juce::String (DrumInfo::names[proc.selectedLane]).toUpperCase());
    drawSection (g, secGroove, "GROOVE");
    drawSection (g, secMaster, "MASTER");
    {
        const int lane = proc.selectedLane;
        const bool isSmp = juce::roundToInt (proc.apvts.getRawParameterValue (PID::lane (lane, "model"))->load()) == DrumInfo::sampleModel (lane);
        const auto sn = proc.sampleName (lane);
        g.setColour (sn.isNotEmpty() ? (isSmp ? K54D::laneColour (lane) : creamDim) : creamDim.withAlpha (0.6f));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.06f));
        g.drawText (sn.isNotEmpty() ? sn.toUpperCase() : u8 ("KEIN SAMPLE \xc2\xb7 DATEI AUF EINE SPUR ZIEHEN"), sampleNameArea, juce::Justification::centredLeft, true);
    }
    {
        // Gain-Reduction im Master
        const float gr = juce::jlimit (0.0f, 12.0f, -proc.getGainReduction());
        auto r = juce::Rectangle<float> ((float) secMaster.getX() + 14.0f, (float) secMaster.getY() + 40.0f, (float) secMaster.getWidth() - 28.0f, 6.0f);
        g.setColour (panelHi);
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (gold);
        g.fillRoundedRectangle (r.withWidth (r.getWidth() * gr / 12.0f), 3.0f);
        g.setColour (creamDim.withAlpha (0.7f));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 7.0f, 0.2f));
        g.drawText ("GLUE GR " + juce::String (gr, 1) + " DB", r.translated (0, 8).withHeight (12).toNearestInt(), juce::Justification::centredLeft);
    }
    {
        auto r = juce::Rectangle<float> ((float) secGroove.getX() + 14.0f, (float) secGroove.getY() + 40.0f, (float) secGroove.getWidth() - 28.0f, 20.0f);
        g.setColour (creamDim.withAlpha (0.6f));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 7.0f, 0.12f));
        g.drawFittedText (u8 ("TIMING UND GEFÜHL"), r.toNearestInt(), juce::Justification::centredLeft, 1);
    }

    // ----- Fusszeile
    auto f = juce::Rectangle<int> (20, H - 24, W - 40, 20);
    g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.2f));
    g.setColour (creamDim.withAlpha (0.65f));
    g.drawText (u8 ("KELLER54  \xc2\xb7  RECORDING \xc2\xb7 MIXING \xc2\xb7 PRODUCING"), f, juce::Justification::centredLeft);
    g.drawText (u8 ("K-54 D V") + JucePlugin_VersionString + u8 ("  \xc2\xb7  FRANK E.  \xc2\xb7  ALLE KLÄNGE SYNTHETISCH ERZEUGT"), f, juce::Justification::centredRight);
}

// ============================================================================ Editor
K54DEditor::K54DEditor (K54DProcessor& p) : AudioProcessorEditor (&p), proc (p), main (p)
{
    const float startScale = juce::jlimit (0.7f, 1.6f, proc.uiScale);
    addAndMakeVisible (main);
    main.onScaleChosen = [this] (float sc) { applyScale (sc); };
    setResizable (false, false);
    applyScale (startScale);
}

void K54DEditor::applyScale (float s)
{
    proc.uiScale = s;
    setSize (juce::roundToInt (K54DMain::W * s), juce::roundToInt (K54DMain::H * s));
}

void K54DEditor::resized()
{
    const float sc = (float) getWidth() / (float) K54DMain::W;
    main.setBounds (0, 0, K54DMain::W, K54DMain::H);
    main.setTransform (juce::AffineTransform::scale (sc));
}

juce::AudioProcessorEditor* K54DProcessor::createEditor() { return new K54DEditor (*this); }

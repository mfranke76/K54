#include "PluginEditor.h"

namespace
{
    constexpr int W = K55Main::W, H = K55Main::H;
    constexpr int tabH = 30;
    constexpr int headerH = 78, keyboardH = 70, footerH = 28;
    constexpr int cellW = 58, cellH = 84, oscRowH = 92;

    const char* waveIds[]  { ParamID::osc1Wave,  ParamID::osc2Wave,  ParamID::osc3Wave };
    const char* tableIds[] { ParamID::osc1Table, ParamID::osc2Table, ParamID::osc3Table };
    const char* posIds[]   { ParamID::osc1Pos,   ParamID::osc2Pos,   ParamID::osc3Pos };
    const char* octIds[]   { ParamID::osc1Oct,   ParamID::osc2Oct,   ParamID::osc3Oct };
    const char* tuneIds[]  { ParamID::osc1Tune,  ParamID::osc2Tune,  ParamID::osc3Tune };
    const char* levelIds[] { ParamID::osc1Level, ParamID::osc2Level, ParamID::osc3Level };

    bool isAudioFile (const juce::String& f)
    {
        return juce::File (f).hasFileExtension ("wav;wave;aif;aiff;flac;mp3;ogg;m4a;caf;aac");
    }
}

K55Main::K55Main (K55Processor& p)
    : proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      pitchWheel (true,  [&p] (float v) { p.setScreenBend (v); }),
      modWheel   (false, [&p] (float v) { p.setScreenWheel (v); }),
      modPanel (p), harmPanel (p), samplePanel (p), fxPanel (p), keysPanel (p)
{
    setLookAndFeel (&lnf);
    setOpaque (true);

    // Reiter
    int idx = 0;
    for (auto* t : { &synthTab, &modTab, &sampleTab, &fxTab, &harmTab, &keysTab })
    {
        t->setClickingTogglesState (true);
        t->setRadioGroupId (55);
        addAndMakeVisible (t);
        t->onClick = [this, t, idx] { if (t->getToggleState()) setPage (idx); };
        ++idx;
    }
    synthTab.setTooltip ("Oszillatoren (Analog / Wavetable / Sample / Piano), Ladder-Filter, Huellkurven");
    modTab.setTooltip ("Modulations-Matrix (8 Plaetze), LFO 2, Mod-Huellkurve, Pitch-/Mod-Wheel, Aftertouch");
    sampleTab.setTooltip ("Samples laden, aufnehmen, bearbeiten - und eigene Wavetables");
    fxTab.setTooltip ("Unison, Sub, Saettigung, ECO-Modus, Chorus, Delay, Hall");
    addChildComponent (modPanel);
    addChildComponent (harmPanel);
    harmTab.setTooltip ("Harmonizer: Tonart, Skala und welche Stimmen dazukommen");
    addAndMakeVisible (harmButton);
    harmButton.setClickingTogglesState (true);
    harmButton.setTooltip ("Automatische Harmonien an/aus (Einstellungen im Reiter HARMONY)");
    harmAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ParamID::harmOn, harmButton);
    portrait = juce::ImageCache::getFromMemory (BinaryData::frank_portrait_png, BinaryData::frank_portrait_pngSize);
    addChildComponent (samplePanel);
    addChildComponent (fxPanel);
    addChildComponent (keysPanel);
    keysTab.setTooltip ("KEYS-Ebene: Funk Clav, Tine Piano, Tonewheel + Leslie, Kirchenorgeln der Welt, FM, Blues Harp - und Talkbox");
    // UPDATE-Knopf (rechts in der Reiterzeile) + Update-Fenster
    addAndMakeVisible (updateButton);
    updateButton.setTooltip ("Neue Version von keller54.de laden - deine Einstellungen, Presets und Samples bleiben erhalten");
    updateButton.onClick = [this] { updatePanel.setVisible (true); updatePanel.toFront (true); };
    updatePanel.onClose = [this] { updatePanel.setVisible (false); };
    addChildComponent (updatePanel);
    updater->checkNow (false);   // leise im Hintergrund, hoechstens alle 6 Stunden
    addAndMakeVisible (pitchWheel);
    addAndMakeVisible (modWheel);
    pitchWheel.setTooltip ("Pitch-Wheel (folgt deinem Keyboard, mit der Maus ziehbar)");
    modWheel.setTooltip ("Mod-Wheel (CC1) - Ziele im Reiter MOD");

    // Fenstergroesse
    addAndMakeVisible (sizeBox);
    for (int pct : { 70, 85, 100, 115, 130, 150 }) sizeBox.addItem (juce::String (pct) + " %", pct);
    sizeBox.setText (juce::String (juce::roundToInt (proc.uiScale * 100.0f)) + " %", juce::dontSendNotification);
    sizeBox.setTooltip ("Fenstergroesse (oder unten rechts an der Ecke ziehen)");
    sizeBox.onChange = [this] { if (onScaleChosen && sizeBox.getSelectedId() > 0) onScaleChosen ((float) sizeBox.getSelectedId() / 100.0f); };

    // Oszillatoren
    for (int i = 0; i < 3; ++i)
    {
        makeChoice (waveIds[i], "Wave");
        makeChoice (tableIds[i], "Table");
        makeKnob (posIds[i], "WT Pos");
        makeChoice (octIds[i], "Range");
        makeKnob (tuneIds[i], "Tune", true);
        makeKnob (levelIds[i], "Level");
    }
    makeKnob (ParamID::noiseLevel, "Noise");
    makeKnob (ParamID::pulseWidth, "Pulse W");
    makeKnob (ParamID::drift, "Drift");

    // Filter
    makeKnob (ParamID::cutoff, "Cutoff");
    makeKnob (ParamID::resonance, "Emphasis");
    makeKnob (ParamID::envAmount, "Contour");
    makeKnob (ParamID::drive, "Drive");
    makeKnob (ParamID::keyTrack, "Key Trk");
    makeKnob (ParamID::velFilter, "Velocity");

    // Huellkurven
    makeKnob (ParamID::fltAttack, "Attack");  makeKnob (ParamID::fltDecay, "Decay");
    makeKnob (ParamID::fltSustain, "Sustain"); makeKnob (ParamID::fltRelease, "Release");
    makeKnob (ParamID::ampAttack, "Attack");  makeKnob (ParamID::ampDecay, "Decay");
    makeKnob (ParamID::ampSustain, "Sustain"); makeKnob (ParamID::ampRelease, "Release");
    makeKnob (ParamID::velAmp, "Velocity");
    makeKnob (ParamID::bendRange, "Bend");
    makeKnob (ParamID::master, "Volume");

    // Modulation / Spielweise
    makeKnob (ParamID::lfoRate, "Rate");
    makeChoice (ParamID::lfoWave, "Wave");
    makeKnob (ParamID::lfoPitch, "> Pitch");
    makeKnob (ParamID::lfoFilter, "> Filter");
    makeKnob (ParamID::glide, "Glide");
    makeChoice (ParamID::voiceMode, "Mode");

    // Presets
    for (auto* b : { &prevButton, &nextButton, &saveButton })
        addAndMakeVisible (b);
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (+1); };
    saveButton.onClick = [this] { savePresetDialog(); };
    saveButton.setTooltip ("Aktuellen Klang als eigenes Preset speichern (mit Sample/Wavetable)");
    addAndMakeVisible (presetBox);
    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.onChange = [this] { presetSelected(); };
    rebuildPresetMenu();

    // Ausgangspegel + Gain neben dem Logo
    addAndMakeVisible (outMeter);
    outMeter.setTooltip ("Main-Ausgang L/R  -  Klick setzt die Clip-Anzeige zurueck");
    outGain.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outGain.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 13);
    outGain.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    outGain.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
    outGain.setMouseDragSensitivity (220);
    outGain.getProperties().set ("bipolar", true);
    outGain.setTooltip ("Output Gain (Main)");
    addAndMakeVisible (outGain);
    outGainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ParamID::outGain, outGain);
    outGain.setDoubleClickReturnValue (true, 0.0);

    // Tastatur
    keyboard.setAvailableRange (24, 108);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, K54Colours::cream);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff111113));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff8A8478));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, K54Colours::gold);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, K54Colours::gold.withAlpha (0.3f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, K54Colours::panelHi);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, K54Colours::gold);
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    // ANSICHT
    viewSettings = SynthStyle::openSettings();
    viewStyle.loadFrom (*viewSettings);
    viewPanel = std::make_unique<SynthViewPanel> (*viewSettings);
    viewPanel->onChange = [this] (const SynthStyle& s) { applyView (s, true); };
    addChildComponent (*viewPanel);
    viewButton.setTooltip (juce::String::fromUTF8 ("Farbschema, eigene Farben, farbige oder schlichte Oberfläche, Ansichten speichern"));
    viewButton.onClick = [this] { showViewPanel (true); };
    addAndMakeVisible (viewButton);
    colorButton.setTooltip (juce::String::fromUTF8 ("Farbige oder schlichte Oberfläche"));
    colorButton.onClick = [this] { auto s = viewStyle; s.colorful = ! s.colorful; applyView (s, true); };
    addAndMakeVisible (colorButton);

    setSize (W, H);
    setPage (0);
    applyView (viewStyle, false);
    startTimerHz (25);
}

void K55Main::applyView (const SynthStyle& s, bool save)
{
    viewStyle = s;
    s.apply();
    if (save)
    {
        viewStyle.saveTo (*viewSettings);
        viewSettings->saveIfNeeded();
    }
    lnf.refreshColours();
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, K54Colours::gold);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, K54Colours::gold.withAlpha (0.3f));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, K54Colours::gold);
    colorButton.setToggleState (s.colorful, juce::dontSendNotification);
    if (viewPanel != nullptr && viewPanel->isVisible()) viewPanel->setStyle (s);
    sendLookAndFeelChange();
    repaint();
}

K55Main::~K55Main()
{
    setLookAndFeel (nullptr);
}

// --------------------------------------------------------------- Bausteine
K55Main::Knob& K55Main::makeKnob (const char* id, const juce::String& caption, bool bipolar)
{
    auto k = std::make_unique<Knob>();
    auto& s = k->slider;
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 66, 15);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
    s.setMouseDragSensitivity (220);
    s.getProperties().set ("bipolar", bipolar);
    addAndMakeVisible (s);
    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s);
    if (auto* param = proc.apvts.getParameter (id))
        s.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    k->label.setText (caption.toUpperCase(), juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.08f));
    addAndMakeVisible (k->label);
    auto& ref = *k;
    knobs[id] = std::move (k);
    return ref;
}

K55Main::Choice& K55Main::makeChoice (const char* id, const juce::String& caption)
{
    auto c = std::make_unique<Choice>();
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        for (int i = 0; i < param->choices.size(); ++i)
            c->box.addItem (param->choices[i].toUpperCase(), i + 1);
    c->box.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (c->box);
    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, c->box);
    c->label.setText (caption.toUpperCase(), juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);
    c->label.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.08f));
    addAndMakeVisible (c->label);
    auto& ref = *c;
    choices[id] = std::move (c);
    return ref;
}

void K55Main::placeKnob (const char* id, juce::Rectangle<int> cell)
{
    auto& k = knob (id);
    k.label.setBounds (cell.removeFromTop (14));
    k.slider.setBounds (cell);
}

void K55Main::placeChoice (const char* id, juce::Rectangle<int> cell)
{
    auto& c = choice (id);
    c.label.setBounds (cell.removeFromTop (14));
    c.box.setBounds (cell.withSizeKeepingCentre (cell.getWidth(), 26).translated (0, -8));
}

// --------------------------------------------------------------- Layout
void K55Main::resized()
{
    {
        auto r = juce::Rectangle<int> (W - 20 - 350, 25, 350, 28);
        saveButton.setBounds (r.removeFromRight (64));
        r.removeFromRight (8);
        nextButton.setBounds (r.removeFromRight (28));
        r.removeFromRight (4);
        prevButton.setBounds (r.removeFromLeft (28));
        r.removeFromLeft (4);
        presetBox.setBounds (r);
    }
    outMeter.setBounds (244, 22, 176, 36);
    outGain.setBounds (428, 8, 60, 64);

    {
        int x = 14;
        for (auto* t : { &synthTab, &modTab, &sampleTab, &fxTab, &harmTab, &keysTab })
        {
            t->setBounds (x, headerH + 10, 92, 24);
            x += 97;
        }
        harmButton.setBounds (x + 8, headerH + 10, 130, 24);
    }
    sizeBox.setBounds (W - 14 - 90, headerH + 10, 90, 24);
    updateButton.setBounds (W - 14 - 90 - 8 - 130, headerH + 10, 130, 24);
    viewButton.setBounds (harmButton.getRight() + 8, headerH + 10, 84, 24);
    colorButton.setBounds (viewButton.getRight() + 6, headerH + 10, 72, 24);
    statusArea = { 330, H - footerH, 280, footerH };   // VOICES jetzt in der Fusszeile
    updatePanel.setBounds (getLocalBounds());
    if (viewPanel != nullptr) viewPanel->setBounds (getLocalBounds());

    const int top = headerH + 12 + tabH, bodyH = H - headerH - tabH - keyboardH - footerH - 24;
    modPanel.setBounds (14, top, W - 28, bodyH);
    harmPanel.setBounds (14, top, W - 28, bodyH);
    samplePanel.setBounds (14, top, W - 28, bodyH);
    fxPanel.setBounds (14, top, W - 28, bodyH);
    keysPanel.setBounds (14, top, W - 28, bodyH);
    secOsc    = { 14,  top, 480, bodyH };
    secFilter = { 506, top, 206, bodyH };
    secEnv    = { 724, top, 258, bodyH };
    secMod    = { 994, top, 152, bodyH };

    // OSZILLATOREN: Nummer | Wave | Table | Pos | Range | Tune | Level
    {
        auto area = secOsc.reduced (10).withTrimmedTop (24);
        for (int i = 0; i < 3; ++i)
        {
            auto row = area.removeFromTop (oscRowH);
            row.removeFromLeft (30);
            placeChoice (waveIds[i],  row.removeFromLeft (92).reduced (3, 0).withTrimmedTop (14).withHeight (56));
            placeChoice (tableIds[i], row.removeFromLeft (104).reduced (3, 0).withTrimmedTop (14).withHeight (56));
            placeKnob (posIds[i], row.removeFromLeft (cellW).withHeight (cellH));
            placeChoice (octIds[i],   row.removeFromLeft (60).reduced (3, 0).withTrimmedTop (14).withHeight (56));
            placeKnob (tuneIds[i], row.removeFromLeft (cellW).withHeight (cellH));
            placeKnob (levelIds[i], row.removeFromLeft (cellW).withHeight (cellH));
        }
        area.removeFromTop (6);
        auto row = area.removeFromTop (cellH);
        row.removeFromLeft (30);
        placeKnob (ParamID::pulseWidth, row.removeFromLeft (92).withSizeKeepingCentre (66, cellH));
        placeKnob (ParamID::drift,      row.removeFromLeft (104).withSizeKeepingCentre (66, cellH));
        row.removeFromLeft (cellW + 60 + cellW);
        placeKnob (ParamID::noiseLevel, row.removeFromLeft (cellW));
    }

    // FILTER
    {
        auto area = secFilter.reduced (10).withTrimmedTop (24);
        placeKnob (ParamID::cutoff, area.removeFromTop (146).withSizeKeepingCentre (126, 146));
        area.removeFromTop (8);
        auto row = area.removeFromTop (cellH);
        const int w3 = row.getWidth() / 3;
        placeKnob (ParamID::resonance, row.removeFromLeft (w3));
        placeKnob (ParamID::envAmount, row.removeFromLeft (w3));
        placeKnob (ParamID::drive, row);
        area.removeFromTop (8);
        row = area.removeFromTop (cellH).withSizeKeepingCentre (w3 * 2, cellH);
        placeKnob (ParamID::keyTrack, row.removeFromLeft (w3));
        placeKnob (ParamID::velFilter, row);
    }

    // HUELLKURVEN + AUSGANG
    {
        auto area = secEnv.reduced (10).withTrimmedTop (24);
        subFEnv = area.removeFromTop (16);
        auto row = area.removeFromTop (cellH);
        const int w4 = row.getWidth() / 4;
        placeKnob (ParamID::fltAttack,  row.removeFromLeft (w4));
        placeKnob (ParamID::fltDecay,   row.removeFromLeft (w4));
        placeKnob (ParamID::fltSustain, row.removeFromLeft (w4));
        placeKnob (ParamID::fltRelease, row);
        area.removeFromTop (10);
        subAEnv = area.removeFromTop (16);
        row = area.removeFromTop (cellH);
        placeKnob (ParamID::ampAttack,  row.removeFromLeft (w4));
        placeKnob (ParamID::ampDecay,   row.removeFromLeft (w4));
        placeKnob (ParamID::ampSustain, row.removeFromLeft (w4));
        placeKnob (ParamID::ampRelease, row);
        area.removeFromTop (10);
        subOut = area.removeFromTop (16);
        row = area.removeFromTop (cellH);
        placeKnob (ParamID::velAmp, row.removeFromLeft (w4));
        placeKnob (ParamID::bendRange, row.removeFromLeft (w4));
        placeKnob (ParamID::master, row.withSizeKeepingCentre (w4 + 20, cellH));
    }

    // MODULATION / SPIELWEISE
    {
        auto area = secMod.reduced (8).withTrimmedTop (24);
        subLfo = area.removeFromTop (16);
        auto row = area.removeFromTop (cellH);
        const int half = row.getWidth() / 2;
        placeKnob (ParamID::lfoRate, row.removeFromLeft (half));
        placeChoice (ParamID::lfoWave, row.withTrimmedTop (14).withHeight (56).reduced (2, 0));
        row = area.removeFromTop (cellH);
        placeKnob (ParamID::lfoPitch, row.removeFromLeft (half));
        placeKnob (ParamID::lfoFilter, row);
        area.removeFromTop (10);
        subPlay = area.removeFromTop (16);
        row = area.removeFromTop (cellH);
        placeKnob (ParamID::glide, row.removeFromLeft (half));
        placeChoice (ParamID::voiceMode, row.withTrimmedTop (14).withHeight (56).reduced (2, 0));
    }

    const int ky = H - footerH - keyboardH - 4;
    pitchWheel.setBounds (14, ky, 34, keyboardH);
    modWheel.setBounds (52, ky, 34, keyboardH);
    keyboard.setBounds (94, ky, W - 108, keyboardH);
    keyboard.setKeyWidth ((float) (W - 108) / 50.0f);   // 24..108 = 50 weisse Tasten
    keyboard.setLowestVisibleKey (24);
}

// --------------------------------------------------------------- Zeichnen
void K55Main::drawSubtitle (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text)
{
    g.setColour (K54Colours::creamDim.withAlpha (0.7f));
    g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.25f));
    g.drawText (text, r, juce::Justification::centredLeft);
    const int textW = (int) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text) + 8;
    g.setColour (K54Colours::line);
    g.fillRect (r.getX() + textW, r.getCentreY(), r.getWidth() - textW, 1);
}

void K55Main::paint (juce::Graphics& g)
{
    using namespace K54Colours;
    g.fillAll (black);

    // ----- Kopfzeile: Logo | Pegel + Gain | Modellname | Presets
    {
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

        // Frank E. als schlichte Linienzeichnung (schwarz auf weiss)
        if (portrait.isValid())
        {
            const juce::Rectangle<float> pr (514.0f, 12.0f, 56.0f, 56.0f);
            g.setOpacity (1.0f);
            g.drawImage (portrait, pr, juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
        }

        g.setColour (cream);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 28.0f, 0.18f));
        g.drawText ("K-54 S", juce::Rectangle<int> (586, 8, 200, 36), juce::Justification::centredLeft);
        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.3f));
        g.drawText ("BY FRANK E.", juce::Rectangle<int> (588, 42, 200, 12), juce::Justification::centredLeft);
        g.setColour (creamDim.withAlpha (0.7f));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 7.0f, 0.25f));
        g.drawText (ParamChoices::u8 ("ANALOG \xc2\xb7 WAVETABLE \xc2\xb7 SAMPLE"), juce::Rectangle<int> (588, 56, 210, 12), juce::Justification::centredLeft);

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
    }

    // Status in der Reiterzeile
    g.setColour (creamDim.withAlpha (0.7f));
    g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.25f));
    g.drawText ((pianoLoading ? ParamChoices::u8 ("FL\xc3\x9cGEL L\xc3\x84" "DT ...   ") : juce::String())
                + "VOICES " + juce::String (voices) + " / " + juce::String (SynthEngine::numVoices),
                statusArea, juce::Justification::centredRight);

    if (currentPage == 0)
    {
        drawK55Section (g, secOsc,    "OSCILLATORS");
        drawK55Section (g, secFilter, "LADDER FILTER");
        drawK55Section (g, secEnv,    "CONTOURS");
        drawK55Section (g, secMod,    "MOD");
        drawSubtitle (g, subFEnv, "FILTER");
        drawSubtitle (g, subAEnv, "LOUDNESS");
        drawSubtitle (g, subOut,  "OUTPUT");
        drawSubtitle (g, subLfo,  "LFO");
        drawSubtitle (g, subPlay, "PLAY");

        for (int i = 0; i < 3; ++i)
        {
            auto r = juce::Rectangle<int> (secOsc.getX() + 8, secOsc.getY() + 34 + i * oscRowH + 14, 30, cellH - 14);
            g.setColour (cream);
            g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 11.0f, 0.1f));
            g.drawText (juce::String (i + 1), r.removeFromTop (r.getHeight() / 2 + 6), juce::Justification::centredBottom);
            g.setColour (creamDim.withAlpha (0.6f));
            g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.2f));
            g.drawText ("OSC", r, juce::Justification::centredTop);
        }
    }

    // ----- Fusszeile
    {
        auto f = juce::Rectangle<int> (20, H - footerH, W - 40, footerH);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.2f));
        g.setColour (creamDim.withAlpha (0.65f));
        g.drawText (ParamChoices::u8 ("KELLER54  \xc2\xb7  RECORDING \xc2\xb7 MIXING \xc2\xb7 PRODUCING"), f, juce::Justification::centredLeft);
        g.drawText (ParamChoices::u8 ("K-54 S V") + JucePlugin_VersionString + ParamChoices::u8 ("  \xc2\xb7  FRANK E.  \xc2\xb7  PIANO: SALAMANDER GRAND V3, A. HOLM, CC BY 3.0"), f, juce::Justification::centredRight);
    }
}

// --------------------------------------------------------------- Drag & Drop (Finder / DAW)
bool K55Main::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files) if (isAudioFile (f)) return true;
    return false;
}

int K55Main::dropTarget (int x, int y) const
{
    if (currentPage == 2 && samplePanel.isOverWavetable ({ x - samplePanel.getX(), y - samplePanel.getY() })) return 2;
    return 1;
}

void K55Main::fileDragMove (const juce::StringArray&, int x, int y)
{
    samplePanel.setDragHighlight (dropTarget (x, y));
}

void K55Main::fileDragExit (const juce::StringArray&)
{
    samplePanel.setDragHighlight (0);
}

void K55Main::filesDropped (const juce::StringArray& files, int x, int y)
{
    samplePanel.setDragHighlight (0);
    for (auto& f : files)
        if (isAudioFile (f))
        {
            const bool wt = dropTarget (x, y) == 2;
            samplePanel.loadFile (juce::File (f), wt);
            setPage (2);
            return;
        }
}

// --------------------------------------------------------------- Presets
void K55Main::rebuildPresetMenu()
{
    // uebersichtlich: je Kategorie ein Untermenue
    presetBox.clear (juce::dontSendNotification);
    auto* root = presetBox.getRootMenu();
    const auto& list = getFactoryPresets();
    juce::StringArray cats;
    for (auto& pr : list) cats.addIfNotAlreadyThere (pr.category);
    for (auto& cat : cats)
    {
        juce::PopupMenu sub;
        for (size_t i = 0; i < list.size(); ++i)
            if (cat == list[i].category)
                sub.addItem ((int) i + 1, juce::String (list[i].name).toUpperCase());
        if (cat == "Init") root->addItem (1, "INIT");
        else root->addSubMenu (cat.toUpperCase(), sub);
    }

    userPresets = proc.getUserPresets();
    if (! userPresets.isEmpty())
    {
        root->addSeparator();
        juce::PopupMenu mine, sub;
        juce::String lastCat;
        auto flush = [&] { if (lastCat.isNotEmpty()) mine.addSubMenu (lastCat.toUpperCase(), sub); sub = {}; };
        for (int i = 0; i < userPresets.size(); ++i)
        {
            auto dir = userPresets[i].getParentDirectory();
            const auto cat = dir == K55Processor::getUserPresetFolder() ? juce::String ("User") : dir.getFileName();
            if (cat != lastCat) { flush(); lastCat = cat; }
            sub.addItem (1000 + i, userPresets[i].getFileNameWithoutExtension().toUpperCase());
        }
        flush();
        root->addSubMenu (ParamChoices::u8 ("\xe2\x98\x85 MEINE PRESETS"), mine);
    }
    presetBox.setText (proc.getPresetName().toUpperCase(), juce::dontSendNotification);
}

void K55Main::presetSelected()
{
    const int id = presetBox.getSelectedId();
    if (id >= 1000)
    {
        if (juce::isPositiveAndBelow (id - 1000, userPresets.size()))
            proc.loadUserPreset (userPresets[id - 1000]);
    }
    else if (id > 0)
        proc.loadFactoryPreset (id - 1);
}

void K55Main::stepPreset (int delta)
{
    juce::Array<int> ids;
    for (int i = 0; i < presetBox.getNumItems(); ++i) ids.add (presetBox.getItemId (i));
    if (ids.isEmpty()) return;
    int idx = ids.indexOf (presetBox.getSelectedId());
    if (idx < 0)
        for (int i = 0; i < presetBox.getNumItems(); ++i)
            if (presetBox.getItemText (i).equalsIgnoreCase (proc.getPresetName())) { idx = i; break; }
    idx = (idx < 0) ? 0 : (idx + delta + ids.size()) % ids.size();
    presetBox.setSelectedId (ids[idx], juce::sendNotificationSync);
}

void K55Main::savePresetDialog()
{
    static const juce::StringArray cats { "Bass", "Lead", "Pad", "Keys", "Pluck", "Brass", "FX", "Sample", "User" };
    auto* w = new juce::AlertWindow ("PRESET SPEICHERN", "Name und Kategorie fuer dein Preset:", juce::MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel (&lnf);
    w->addTextEditor ("name", proc.getPresetName());
    w->addComboBox ("cat", cats, "Kategorie");
    w->addButton ("Speichern", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Abbrechen", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<K55Main> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (result != 1 || safe == nullptr) return;
        const auto name = w->getTextEditorContents ("name").trim();
        auto* cb = w->getComboBoxComponent ("cat");
        const auto cat = cb != nullptr ? cb->getText() : juce::String ("User");
        if (name.isEmpty()) return;
        if (safe->proc.saveUserPreset (name, cat))
        {
            safe->rebuildPresetMenu();
            for (int i = 0; i < safe->userPresets.size(); ++i)
                if (safe->userPresets[i].getFileNameWithoutExtension() == juce::File::createLegalFileName (name)
                    && safe->userPresets[i].getParentDirectory().getFileName() == juce::File::createLegalFileName (cat))
                    safe->presetBox.setSelectedId (1000 + i, juce::dontSendNotification);
        }
    }), true);
}

void K55Main::setPage (int page)
{
    currentPage = page;
    synthTab.setToggleState (page == 0, juce::dontSendNotification);
    modTab.setToggleState (page == 1, juce::dontSendNotification);
    sampleTab.setToggleState (page == 2, juce::dontSendNotification);
    fxTab.setToggleState (page == 3, juce::dontSendNotification);
    harmTab.setToggleState (page == 4, juce::dontSendNotification);
    keysTab.setToggleState (page == 5, juce::dontSendNotification);
    keysPanel.setVisible (page == 5);
    harmPanel.setVisible (page == 4);
    for (auto& [id, k] : knobs)   { k->slider.setVisible (page == 0); k->label.setVisible (page == 0); }
    for (auto& [id, c] : choices) { c->box.setVisible (page == 0);    c->label.setVisible (page == 0); }
    modPanel.setVisible (page == 1);
    samplePanel.setVisible (page == 2);
    fxPanel.setVisible (page == 3);
    repaint();
}

void K55Main::timerCallback()
{
    outMeter.setLevels (proc.outPeak[0].exchange (0.0f), proc.outPeak[1].exchange (0.0f));

    pitchWheel.setValue (proc.bendOut.load());
    modWheel.setValue (proc.modOut.load());

    const int v = proc.voicesActive.load();
    const bool pl = proc.isPianoLoading();
    if (v != voices || pl != pianoLoading) { voices = v; pianoLoading = pl; repaint (statusArea); }

    // UPDATE-Knopf: gold, sobald eine neue Version bereitsteht
    {
        const auto st = updater->get();
        const juce::String label = st.updateAvailable ? "UPDATE " + st.latest : juce::String ("UPDATE");
        if (label != lastUpdateLabel)
        {
            lastUpdateLabel = label;
            updateButton.setButtonText (label);
            updateButton.setClickingTogglesState (false);
            updateButton.setToggleState (st.updateAvailable, juce::dontSendNotification);
        }
    }

    // Table / Pos nur aktiv, wenn der Oszillator auf WAVETABLE steht (nur bei Aenderung anfassen)
    for (int i = 0; i < 3; ++i)
    {
        const int w = juce::roundToInt (proc.apvts.getRawParameterValue (waveIds[i])->load());
        const bool emptyNow = w == ParamChoices::wSample && proc.getSample() == nullptr;
        if (w == lastWave[i] && emptyNow == lastEmpty[i]) continue;
        lastWave[i] = w;
        lastEmpty[i] = emptyNow;
        const bool wt = w == ParamChoices::wTable;
        choice (tableIds[i]).box.setEnabled (wt);
        choice (tableIds[i]).box.setAlpha (wt ? 1.0f : 0.35f);
        knob (posIds[i]).slider.setEnabled (wt);
        knob (posIds[i]).slider.setAlpha (wt ? 1.0f : 0.35f);
        const bool smp = w == ParamChoices::wSample;
        const bool empty = smp && proc.getSample() == nullptr;
        choice (waveIds[i]).box.setColour (juce::ComboBox::textColourId, empty ? juce::Colour (0xffE0503A) : K54Colours::cream);
        choice (waveIds[i]).box.setTooltip (empty ? "Noch kein Sample geladen - Reiter SAMPLE" : juce::String());
    }

    const int pv = proc.presetVersion.load();
    if (pv != lastPresetVersion)
    {
        lastPresetVersion = pv;
        const auto name = proc.getPresetName();
        int found = 0;
        for (int i = 0; i < presetBox.getNumItems(); ++i)
            if (presetBox.getItemText (i).equalsIgnoreCase (name)) { found = presetBox.getItemId (i); break; }
        if (found != 0) presetBox.setSelectedId (found, juce::dontSendNotification);
        else            presetBox.setText (name.toUpperCase(), juce::dontSendNotification);
    }
}

// ---------------------------------------------------------------- Fenster (skalierbar)
K55Editor::K55Editor (K55Processor& p) : AudioProcessorEditor (&p), proc (p), main (p)
{
    const float startScale = juce::jlimit (0.7f, 1.6f, proc.uiScale);   // vor setResizeLimits merken
    addAndMakeVisible (main);
    main.onScaleChosen = [this] (float sc) { setSize (juce::roundToInt (K55Main::W * sc), juce::roundToInt (K55Main::H * sc)); };
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (K55Main::W * 0.7f), juce::roundToInt (K55Main::H * 0.7f),
                     juce::roundToInt (K55Main::W * 1.6f), juce::roundToInt (K55Main::H * 1.6f));
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) K55Main::W / K55Main::H);
    setSize (juce::roundToInt (K55Main::W * startScale), juce::roundToInt (K55Main::H * startScale));
    ready = true;
    proc.uiScale = startScale;
}

void K55Editor::resized()
{
    const float sc = (float) getWidth() / (float) K55Main::W;
    main.setBounds (0, 0, K55Main::W, K55Main::H);
    main.setTransform (juce::AffineTransform::scale (sc));
    if (ready) proc.uiScale = sc;
}

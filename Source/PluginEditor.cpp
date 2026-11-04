#include "PluginEditor.h"

namespace
{
    constexpr int W = 1040, H = 614;
    constexpr int tabH = 30;
    constexpr int headerH = 78, keyboardH = 64, footerH = 28;
    constexpr int cellW = 66, cellH = 84;
}

K54Editor::K54Editor (K54Processor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      looperPanel (p), drumPanel (p), seqPanel (p), outsPanel (p)
{
    setLookAndFeel (&lnf);

    // Reiter SYNTH / LOOPER
    for (auto* t : { &synthTab, &looperTab, &drumsTab, &seqTab, &outsTab })
    {
        t->setClickingTogglesState (true);
        t->setRadioGroupId (54);
        addAndMakeVisible (t);
    }
    synthTab.setToggleState (true, juce::dontSendNotification);
    synthTab.onClick  = [this] { if (synthTab.getToggleState())  setPage (0); };
    looperTab.onClick = [this] { if (looperTab.getToggleState()) setPage (1); };
    drumsTab.onClick  = [this] { if (drumsTab.getToggleState())  setPage (2); };
    seqTab.onClick    = [this] { if (seqTab.getToggleState())    setPage (3); };
    outsTab.onClick   = [this] { if (outsTab.getToggleState())   setPage (4); };
    addChildComponent (looperPanel);
    addChildComponent (drumPanel);
    addChildComponent (seqPanel);
    addChildComponent (outsPanel);

    // UPDATE-Knopf (oben rechts in der Reiterzeile) + Update-Fenster
    addAndMakeVisible (updateButton);
    updateButton.setTooltip ("Neue Version von keller54.de laden");
    updateButton.onClick = [this] { updatePanel.setVisible (true); updatePanel.toFront (false); };
    addChildComponent (updatePanel);
    updatePanel.onClose = [this] { updatePanel.setVisible (false); };
    updater->checkNow (false);   // leise im Hintergrund, hoechstens alle 6 Stunden

    // Oszillatoren
    makeChoice (ParamID::osc1Wave, "Wave");  makeChoice (ParamID::osc1Oct, "Range");
    makeChoice (ParamID::osc2Wave, "Wave");  makeChoice (ParamID::osc2Oct, "Range");
    makeChoice (ParamID::osc3Wave, "Wave");  makeChoice (ParamID::osc3Oct, "Range");
    makeKnob (ParamID::osc2Tune, "Tune", true);
    makeKnob (ParamID::osc3Tune, "Tune", true);
    makeKnob (ParamID::osc1Level, "Level");
    makeKnob (ParamID::osc2Level, "Level");
    makeKnob (ParamID::osc3Level, "Level");
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
    makeKnob (ParamID::master, "Volume");

    // Modulation / Spielweise
    makeKnob (ParamID::lfoRate, "Rate");
    makeChoice (ParamID::lfoWave, "Wave");
    makeKnob (ParamID::lfoPitch, "> Pitch");
    makeKnob (ParamID::lfoFilter, "> Filter");
    makeKnob (ParamID::glide, "Glide");
    makeChoice (ParamID::voiceMode, "Mode");
    makeKnob (ParamID::bendRange, "Bend");

    // Presets
    for (auto* b : { &prevButton, &nextButton, &saveButton })
        addAndMakeVisible (b);
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (+1); };
    saveButton.onClick = [this] { savePresetDialog(); };
    addAndMakeVisible (presetBox);
    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.onChange = [this] { presetSelected(); };
    rebuildPresetMenu();

    // Tastatur
    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (17.0f);
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

    setSize (W, H);
    startTimerHz (15);
}

K54Editor::~K54Editor()
{
    setLookAndFeel (nullptr);
}

// --------------------------------------------------------------- Bausteine
K54Editor::Knob& K54Editor::makeKnob (const char* id, const juce::String& caption, bool bipolar)
{
    auto k = std::make_unique<Knob>();
    auto& s = k->slider;
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    s.setColour (juce::Slider::textBoxTextColourId, K54Colours::creamDim);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, cellW, 15);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
    s.setVelocityBasedMode (false);
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

K54Editor::Choice& K54Editor::makeChoice (const char* id, const juce::String& caption)
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

void K54Editor::placeKnob (const char* id, juce::Rectangle<int> cell)
{
    auto& k = knob (id);
    k.label.setBounds (cell.removeFromTop (14));
    k.slider.setBounds (cell);
}

void K54Editor::placeChoice (const char* id, juce::Rectangle<int> cell)
{
    auto& c = choice (id);
    c.label.setBounds (cell.removeFromTop (14));
    c.box.setBounds (cell.withSizeKeepingCentre (cell.getWidth(), 26).translated (0, -8));
}

// --------------------------------------------------------------- Layout
void K54Editor::resized()
{
    // Kopfzeile: Preset-Browser rechts
    {
        auto r = juce::Rectangle<int> (W - 20 - 380, 25, 380, 28);
        saveButton.setBounds (r.removeFromRight (64));
        r.removeFromRight (8);
        nextButton.setBounds (r.removeFromRight (28));
        r.removeFromRight (4);
        prevButton.setBounds (r.removeFromLeft (28));
        r.removeFromLeft (4);
        presetBox.setBounds (r);
    }

    {
        int x = 14;
        for (auto* t : { &synthTab, &looperTab, &drumsTab, &seqTab, &outsTab })
        {
            t->setBounds (x, headerH + 10, 104, 24);
            x += 110;
        }
    }
    updateButton.setBounds (W - 14 - 170, headerH + 10, 170, 24);
    updatePanel.setBounds (getLocalBounds());

    const int top = headerH + 12 + tabH, bodyH = H - headerH - tabH - keyboardH - footerH - 24;
    looperPanel.setBounds (14, top, W - 28, bodyH);
    drumPanel.setBounds (14, top, W - 28, bodyH);
    seqPanel.setBounds (14, top, W - 28, bodyH);
    outsPanel.setBounds (14, top, W - 28, bodyH);
    secOsc    = { 14,  top, 346, bodyH };
    secFilter = { 372, top, 214, bodyH };
    secEnv    = { 598, top, 250, bodyH };
    secMod    = { 860, top, 166, bodyH };

    // OSZILLATOREN: je Zeile Nummer | Wave | Range | Tune | Level
    {
        auto area = secOsc.reduced (10).withTrimmedTop (22);
        const char* waves[]  { ParamID::osc1Wave, ParamID::osc2Wave, ParamID::osc3Wave };
        const char* octs[]   { ParamID::osc1Oct,  ParamID::osc2Oct,  ParamID::osc3Oct };
        const char* tunes[]  { nullptr,           ParamID::osc2Tune, ParamID::osc3Tune };
        const char* levels[] { ParamID::osc1Level, ParamID::osc2Level, ParamID::osc3Level };

        for (int i = 0; i < 3; ++i)
        {
            auto row = area.removeFromTop (cellH);
            row.removeFromLeft (36);                                   // "OSC n" (paint)
            placeChoice (waves[i], row.removeFromLeft (88).reduced (3, 0).withTrimmedTop (14).withHeight (56));
            placeChoice (octs[i],  row.removeFromLeft (66).reduced (3, 0).withTrimmedTop (14).withHeight (56));
            auto tuneCell = row.removeFromLeft (cellW);
            if (tunes[i] != nullptr) placeKnob (tunes[i], tuneCell);
            placeKnob (levels[i], row.removeFromLeft (cellW));
        }
        area.removeFromTop (6);
        auto row = area.removeFromTop (cellH);
        row.removeFromLeft (36);
        placeKnob (ParamID::pulseWidth, row.removeFromLeft (88).withSizeKeepingCentre (cellW, cellH));
        placeKnob (ParamID::drift,      row.removeFromLeft (66));
        row.removeFromLeft (cellW);
        placeKnob (ParamID::noiseLevel, row.removeFromLeft (cellW));
    }

    // FILTER: grosser Cutoff, darunter 3 + 2
    {
        auto area = secFilter.reduced (10).withTrimmedTop (22);
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
        auto area = secEnv.reduced (10).withTrimmedTop (22);
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
        placeKnob (ParamID::master, row.withSizeKeepingCentre (cellW + 20, cellH));
    }

    // MODULATION / SPIELWEISE
    {
        auto area = secMod.reduced (8).withTrimmedTop (22);
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

    keyboard.setBounds (14, H - footerH - keyboardH - 4, W - 28, keyboardH);
    keyboard.setKeyWidth ((float) (W - 28) / 50.0f);   // 24..108 = 50 weisse Tasten
    keyboard.setLowestVisibleKey (24);
}

// --------------------------------------------------------------- Zeichnen
void K54Editor::drawSection (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    g.setColour (K54Colours::panel);
    g.fillRoundedRectangle (r.toFloat(), 8.0f);
    g.setColour (K54Colours::line);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 8.0f, 1.0f);

    auto t = r.reduced (12, 8).removeFromTop (16);
    g.setColour (K54Colours::gold);
    g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 10.5f, 0.3f));
    g.drawText (title, t, juce::Justification::centredLeft);
}

void K54Editor::drawSubtitle (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text)
{
    g.setColour (K54Colours::creamDim.withAlpha (0.7f));
    g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.25f));
    g.drawText (text, r, juce::Justification::centredLeft);
    const int textW = (int) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text) + 8;
    g.setColour (K54Colours::line);
    g.fillRect (r.getX() + textW, r.getCentreY(), r.getWidth() - textW, 1);
}

void K54Editor::paint (juce::Graphics& g)
{
    using namespace K54Colours;
    g.fillAll (black);

    // ----- Kopfzeile
    {
        // Keller54-Logo (klein): Signet + KELLER + 54
        drawKellerMeter (g, { 18.0f, 16.0f, 44.0f, 44.0f }, cream, gold);
        g.setColour (cream);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 19.0f, 0.14f));
        g.drawText ("KELLER", 70, 22, 120, 34, juce::Justification::centredLeft);
        const int kellerW = (int) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), "KELLER");
        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::monoton(), 36.0f));
        g.drawText ("54", 70 + kellerW + 8, 18, 70, 42, juce::Justification::centredLeft);

        // Modellname + Kuenstlername
        g.setColour (cream);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 28.0f, 0.18f));
        g.drawText ("K-54", juce::Rectangle<int> (0, 10, W, 38), juce::Justification::centred);
        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 10.0f, 0.45f));
        g.drawText ("BY FRANK E.", juce::Rectangle<int> (0, 50, W, 16), juce::Justification::centred);

        g.setColour (goldDim.withAlpha (0.6f));
        g.fillRect (0, headerH - 1, W, 1);
    }

    // ----- Sektionen (nur auf der SYNTH-Seite)
    if (currentPage == 0)
    {
    drawSection (g, secOsc,    "OSCILLATORS");
    drawSection (g, secFilter, "LADDER FILTER");
    drawSection (g, secEnv,    "CONTOURS");
    drawSection (g, secMod,    "MOD");
    drawSubtitle (g, subFEnv, "FILTER");
    drawSubtitle (g, subAEnv, "LOUDNESS");
    drawSubtitle (g, subOut,  "OUTPUT");
    drawSubtitle (g, subLfo,  "LFO");
    drawSubtitle (g, subPlay, "PLAY");

    // Oszillator-Nummern
    g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 11.0f, 0.1f));
    for (int i = 0; i < 3; ++i)
    {
        auto r = juce::Rectangle<int> (secOsc.getX() + 10, secOsc.getY() + 32 + i * cellH + 14, 36, cellH - 14);
        g.setColour (cream);
        g.drawText (juce::String (i + 1), r.removeFromTop (r.getHeight() / 2 + 6), juce::Justification::centredBottom);
        g.setColour (creamDim.withAlpha (0.6f));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.0f, 0.2f));
        g.drawText ("OSC", r, juce::Justification::centredTop);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 11.0f, 0.1f));
    }
    }

    // ----- Fusszeile
    {
        auto f = juce::Rectangle<int> (20, H - footerH, W - 40, footerH);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.35f));
        g.setColour (creamDim.withAlpha (0.65f));
        g.drawText (juce::CharPointer_UTF8 ("KELLER54  \xc2\xb7  RECORDING \xc2\xb7 MIXING \xc2\xb7 PRODUCING"), f, juce::Justification::centredLeft);
        g.drawText (juce::String ("K-54 V") + JucePlugin_VersionString + juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7  SOUND DESIGN FRANK E.")), f, juce::Justification::centredRight);
    }
}

// --------------------------------------------------------------- Presets
void K54Editor::rebuildPresetMenu()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& list = getFactoryPresets();
    juce::String lastCat;
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (lastCat != list[i].category)
        {
            if (lastCat.isNotEmpty()) presetBox.addSeparator();
            presetBox.addSectionHeading (juce::String (list[i].category).toUpperCase());
            lastCat = list[i].category;
        }
        presetBox.addItem (juce::String (list[i].name).toUpperCase(), (int) i + 1);
    }

    userPresets = proc.getUserPresets();
    if (! userPresets.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("USER");
        for (int i = 0; i < userPresets.size(); ++i)
            presetBox.addItem (userPresets[i].getFileNameWithoutExtension().toUpperCase(), 1000 + i);
    }
    presetBox.setText (proc.getPresetName().toUpperCase(), juce::dontSendNotification);
}

void K54Editor::presetSelected()
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

void K54Editor::stepPreset (int delta)
{
    // Reihenfolge: alle Werks-Presets, danach eigene
    juce::Array<int> ids;
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        ids.add (presetBox.getItemId (i));
    if (ids.isEmpty()) return;

    int idx = ids.indexOf (presetBox.getSelectedId());
    if (idx < 0)
    {
        // Text ohne Auswahl -> ueber den Namen suchen
        for (int i = 0; i < presetBox.getNumItems(); ++i)
            if (presetBox.getItemText (i).equalsIgnoreCase (proc.getPresetName())) { idx = i; break; }
    }
    idx = (idx < 0) ? 0 : (idx + delta + ids.size()) % ids.size();
    presetBox.setSelectedId (ids[idx], juce::sendNotificationSync);
}

void K54Editor::savePresetDialog()
{
    auto* w = new juce::AlertWindow ("PRESET SPEICHERN", "Name fuer dein Preset:", juce::MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel (&lnf);
    w->addTextEditor ("name", proc.getPresetName());
    w->addButton ("Speichern", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Abbrechen", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<K54Editor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (result != 1 || safe == nullptr) return;
        const auto name = w->getTextEditorContents ("name").trim();
        if (name.isEmpty()) return;
        if (safe->proc.saveUserPreset (name))
        {
            safe->rebuildPresetMenu();
            for (int i = 0; i < safe->userPresets.size(); ++i)
                if (safe->userPresets[i].getFileNameWithoutExtension() == juce::File::createLegalFileName (name))
                    safe->presetBox.setSelectedId (1000 + i, juce::dontSendNotification);
        }
    }), true);
}

void K54Editor::setPage (int page)
{
    currentPage = page;
    synthTab.setToggleState (page == 0, juce::dontSendNotification);
    looperTab.setToggleState (page == 1, juce::dontSendNotification);
    drumsTab.setToggleState (page == 2, juce::dontSendNotification);
    seqTab.setToggleState (page == 3, juce::dontSendNotification);
    outsTab.setToggleState (page == 4, juce::dontSendNotification);
    for (auto& [id, k] : knobs)   { k->slider.setVisible (page == 0); k->label.setVisible (page == 0); }
    for (auto& [id, c] : choices) { c->box.setVisible (page == 0);    c->label.setVisible (page == 0); }
    looperPanel.setVisible (page == 1);
    drumPanel.setVisible (page == 2);
    seqPanel.setVisible (page == 3);
    outsPanel.setVisible (page == 4);
    repaint();
}

void K54Editor::timerCallback()
{
    // UPDATE-Knopf: gold, sobald eine neue Version bereitsteht
    {
        const auto st = updater->get();
        const juce::String label = st.updateAvailable ? "UPDATE " + st.latest : juce::String ("UPDATE");
        if (label != lastUpdateLabel)
        {
            lastUpdateLabel = label;
            updateButton.setButtonText (label);
            updateButton.setToggleState (st.updateAvailable, juce::dontSendNotification);
        }
    }

    // LOOPER-Reiter leuchtet rot, solange eine Spur aufnimmt
    const bool rec = proc.looper.anyRecording();
    looperTab.setColour (juce::TextButton::textColourOffId, rec ? LoopColours::rec : K54Colours::cream);
    looperTab.setColour (juce::TextButton::textColourOnId,  rec ? juce::Colour (0xff5A0F0A) : K54Colours::black);

    const int v = proc.presetVersion.load();
    if (v != lastPresetVersion)
    {
        lastPresetVersion = v;
        const auto name = proc.getPresetName();
        int found = 0;
        for (int i = 0; i < presetBox.getNumItems(); ++i)
            if (presetBox.getItemText (i).equalsIgnoreCase (name)) { found = presetBox.getItemId (i); break; }
        if (found != 0) presetBox.setSelectedId (found, juce::dontSendNotification);
        else            presetBox.setText (name.toUpperCase(), juce::dontSendNotification);
    }
}

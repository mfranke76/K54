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
//  Wellenform mit Start-, End- und Loop-Marken (ziehbar)
class SampleWaveView : public juce::Component
{
public:
    explicit SampleWaveView (K55Processor& p) : proc (p) {}

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0E0E10));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (dragOver ? gold : line);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, dragOver ? 2.0f : 1.0f);

        auto s = proc.getSample();
        if (s == nullptr || s->peaks.empty())
        {
            g.setColour (creamDim.withAlpha (0.6f));
            g.setFont (K54Fonts::make (K54Fonts::syncopate(), 10.0f, 0.15f));
            g.drawFittedText (ParamChoices::u8 ("AUDIODATEI HIER HINEINZIEHEN (FINDER / DAW)\nODER \xc2\xbbLADEN\xc2\xab / \xc2\xbbREC\xc2\xab DR\xc3\x9c" "CKEN"),
                              getLocalBounds().reduced (10), juce::Justification::centred, 3);
            return;
        }

        const float st = value (ParamID::smpStart), en = juce::jmax (st + 0.001f, value (ParamID::smpEnd));
        const float ls = st + value (ParamID::smpLoopStart) * (en - st);
        const bool loop = juce::roundToInt (value (ParamID::smpLoop)) > 0;
        auto inner = b.reduced (6.0f, 8.0f);
        const float mid = inner.getCentreY(), h = inner.getHeight() * 0.5f;
        const int pts = (int) s->peaks.size();

        // ausserhalb Start..Ende abgedunkelt
        juce::Path wave;
        for (int i = 0; i < pts; ++i)
        {
            const float x = inner.getX() + inner.getWidth() * (float) i / (float) (pts - 1);
            const float v = juce::jmin (1.0f, s->peaks[(size_t) i]) * h;
            wave.addRectangle (x, mid - v, juce::jmax (1.0f, inner.getWidth() / (float) pts), v * 2.0f + 0.5f);
        }
        g.setColour (creamDim.withAlpha (0.35f));
        g.fillPath (wave);
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (juce::Rectangle<float> (xFor (st), b.getY(), xFor (en) - xFor (st), b.getHeight()).toNearestInt());
            g.setColour (gold);
            g.fillPath (wave);
        }
        auto marker = [&] (float pos, juce::Colour c, const char* t)
        {
            const float x = xFor (pos);
            g.setColour (c);
            g.fillRect (x - 1.0f, b.getY() + 2.0f, 2.0f, b.getHeight() - 4.0f);
            g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 8.0f, 0.1f));
            g.drawText (t, juce::Rectangle<float> (x + 3.0f, b.getY() + 3.0f, 40.0f, 12.0f), juce::Justification::centredLeft);
        };
        marker (st, cream, "S");
        marker (en, cream, "E");
        if (loop) marker (ls, juce::Colour (0xff6FC3DF), "LOOP");

        g.setColour (creamDim);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.1f));
        const juce::String info = s->name.toUpperCase() + "   " + juce::String ((double) s->length() / s->sampleRate, 2) + " S   "
                                + (s->isStereo() ? "STEREO" : "MONO") + "   " + juce::String (juce::roundToInt (s->sampleRate)) + " HZ";
        g.drawText (info, getLocalBounds().reduced (10, 4).removeFromBottom (14), juce::Justification::centredRight);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (proc.getSample() == nullptr) { if (onEmptyClick) onEmptyClick(); return; }
        const float st = value (ParamID::smpStart), en = value (ParamID::smpEnd);
        const float ls = st + value (ParamID::smpLoopStart) * (en - st);
        const float x = (float) e.x;
        const bool loop = juce::roundToInt (value (ParamID::smpLoop)) > 0;
        float best = 1.0e9f;
        dragging = nullptr;
        auto test = [&] (float pos, const char* id) { const float d = std::abs (xFor (pos) - x); if (d < best) { best = d; dragging = id; } };
        test (st, ParamID::smpStart);
        test (en, ParamID::smpEnd);
        if (loop) test (ls, ParamID::smpLoopStart);
        if (best > 14.0f) dragging = x < (xFor (st) + xFor (en)) * 0.5f ? ParamID::smpStart : ParamID::smpEnd;
        if (auto* p = proc.apvts.getParameter (dragging)) p->beginChangeGesture();
        mouseDrag (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging == nullptr) return;
        auto inner = getLocalBounds().toFloat().reduced (6.0f, 8.0f);
        float pos = juce::jlimit (0.0f, 1.0f, ((float) e.x - inner.getX()) / inner.getWidth());
        const float st = value (ParamID::smpStart), en = value (ParamID::smpEnd);
        if (dragging == ParamID::smpStart)          pos = juce::jmin (pos, en - 0.005f);
        else if (dragging == ParamID::smpEnd)       pos = juce::jmax (pos, st + 0.005f);
        else                                        pos = juce::jlimit (0.0f, 0.98f, (pos - st) / juce::jmax (0.001f, en - st));
        if (auto* p = proc.apvts.getParameter (dragging)) p->setValueNotifyingHost (p->convertTo0to1 (pos));
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (auto* p = proc.apvts.getParameter (dragging != nullptr ? dragging : "")) p->endChangeGesture();
        dragging = nullptr;
    }

    std::function<void()> onEmptyClick;
    bool dragOver = false;

private:
    float value (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    float xFor (float pos) const
    {
        auto inner = getLocalBounds().toFloat().reduced (6.0f, 8.0f);
        return inner.getX() + inner.getWidth() * pos;
    }
    K55Processor& proc;
    const char* dragging = nullptr;
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
            wave.setBounds (a.removeFromTop (150));
            a.removeFromTop (8);
            row = a.removeFromTop (cellH);
            const int cw = 70;
            place (*start,  row.removeFromLeft (cw));
            place (*end,    row.removeFromLeft (cw));
            place (*loop,   row.removeFromLeft (110));
            place (*loopSt, row.removeFromLeft (cw));
            place (*root,   row.removeFromLeft (cw + 10));
            row.removeFromLeft (10);
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
            "SO SPIELST DU DAS SAMPLE: AUF DER SYNTH-SEITE BEI EINEM OSZILLATOR \xc2\xbbSAMPLE\xc2\xab W\xc3\x84HLEN (ODER EIN PRESET AUS \xc2\xbbSAMPLE\xc2\xab). "
            "KEY TRACK AN = CHROMATISCH \xc3\x9c" "BER DIE TASTATUR, AUS = LOOP/ONE-SHOT IN ORIGINALTONH\xc3\x96HE. MARKEN S / E / LOOP MIT DER MAUS ZIEHEN."));
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

    SampleWaveView wave;
    WavetableView wtView;
    MiniMeter inMeter;
    juce::TextButton loadBtn { "LADEN" }, recBtn { ParamChoices::u8 ("\xe2\x97\x8f REC") }, stopBtn { "STOP" }, toWtBtn { ParamChoices::u8 ("\xe2\x86\x92 WAVETABLE") },
                     clearBtn { "LEEREN" }, wtLoadBtn { "WT LADEN" }, osc1Btn { "OSC 1" }, osc2Btn { "OSC 2" }, osc3Btn { "OSC 3" };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File lastFolder;
    juce::String lastError;
    int viewOsc = 0, lastRec = -1, lastSource = -1, lastWtSource = -1;

    Knob *start = nullptr, *end = nullptr, *loopSt = nullptr, *root = nullptr, *inGain = nullptr;
    Choice *loop = nullptr, *inMode = nullptr;
    Toggle *keyTrk = nullptr, *oneShot = nullptr, *inAuto = nullptr, *inMon = nullptr;
    juce::Rectangle<int> secSample, secRight, subIn, subWt, recInfo, hint, hintWt;
    SampleBrowser browser;
    juce::TextButton browseTab { "BROWSER" }, inputTab { "INPUT & WT" };
    int rightPage = 0;
};

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Updater.h"
#include "LookAndFeel.h"

// Ueberlagerndes UPDATE-Fenster im K-54 (mit Profilbild von keller54.de)
class UpdatePanel : public juce::Component, private juce::Timer
{
public:
    explicit UpdatePanel (K54Updater& u) : updater (u)
    {
        for (auto* b : { &actionButton, &accountButton, &closeButton, &siteButton })
            addAndMakeVisible (b);

        closeButton.onClick   = [this] { if (onClose) onClose(); };
        siteButton.onClick    = [this] { juce::URL (updater.accountPage()).launchInDefaultBrowser(); };
        accountButton.onClick = [this]
        {
            const auto s = updater.get();
            if (s.phase == K54Updater::Phase::WaitingForBrowser) updater.cancel();
            else if (s.loggedIn)                                 updater.logout();
            else                                                 updater.login();
        };
        actionButton.onClick = [this]
        {
            const auto s = updater.get();
            if (s.phase == K54Updater::Phase::Downloading)       updater.cancel();
            else if (s.updateAvailable && s.loggedIn)            updater.download();
            else if (s.updateAvailable && ! s.loggedIn)          updater.login();
            else                                                 updater.checkNow (true);
        };
        setInterceptsMouseClicks (true, true);
    }

    std::function<void()> onClose;

    void visibilityChanged() override
    {
        if (isVisible())
        {
            if (! updater.busy() && updater.get().phase != K54Updater::Phase::Done)
                updater.checkNow (true);
            startTimerHz (10);
            refresh();
        }
        else
            stopTimer();
    }

    void resized() override
    {
        card = getLocalBounds().withSizeKeepingCentre (620, 400);
        auto r = card.reduced (28);
        closeButton.setBounds (card.getRight() - 48, card.getY() + 14, 34, 30);
        r.removeFromTop (140);   // Kopf mit Bild (paint)
        notesArea = r.removeFromTop (92);
        r.removeFromTop (8);
        progressArea = r.removeFromTop (8);
        r.removeFromTop (8);
        statusArea = r.removeFromTop (20);
        r.removeFromTop (14);
        auto row = r.removeFromTop (38);
        actionButton.setBounds (row.removeFromRight (280));
        row.removeFromRight (10);
        accountButton.setBounds (row.removeFromRight (150));
        siteButton.setBounds (row.removeFromLeft (110).withSizeKeepingCentre (110, 30));
    }

    void paint (juce::Graphics& g) override
    {
        using namespace K54Colours;
        g.fillAll (black.withAlpha (0.82f));

        auto cf = card.toFloat();
        g.setColour (panel);
        g.fillRoundedRectangle (cf, 12.0f);
        g.setColour (goldDim);
        g.drawRoundedRectangle (cf.reduced (0.5f), 12.0f, 1.0f);

        const auto s = updater.get();
        auto head = card.reduced (28).removeFromTop (120);

        // Profilbild (rund) oder Keller54-Signet
        auto pic = head.removeFromLeft (112).withSizeKeepingCentre (104, 104).toFloat();
        if (s.avatar.isValid())
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip; clip.addEllipse (pic);
            g.reduceClipRegion (clip);
            g.drawImage (s.avatar, pic, juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
        }
        else
        {
            g.setColour (black);
            g.fillEllipse (pic);
            drawKellerMeter (g, pic.reduced (22.0f), cream, gold);
        }
        g.setColour (gold);
        g.drawEllipse (pic, 2.5f);

        head.removeFromLeft (20);
        g.setColour (gold);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.5f, 0.35f));
        g.drawText (juce::String (juce::CharPointer_UTF8 ("KELLER54 \xc2\xb7 BY FRANK E.")), head.removeFromTop (22), juce::Justification::bottomLeft);
        g.setColour (cream);
        g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 22.0f, 0.14f));
        g.drawText ("K-54 UPDATER", head.removeFromTop (36), juce::Justification::centredLeft);

        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 10.0f, 0.1f));
        g.setColour (creamDim);
        auto vrow = head.removeFromTop (22);
        g.drawText ("INSTALLIERT  " + s.installed, vrow.removeFromLeft (180), juce::Justification::centredLeft);
        g.setColour (s.updateAvailable ? gold : creamDim);
        g.drawText ("VERFUEGBAR  " + (s.latest.isNotEmpty() ? s.latest : juce::String ("-")), vrow, juce::Justification::centredLeft);

        g.setColour (creamDim);
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.0f, 0.12f));
        juce::String acc = s.loggedIn ? "ANGEMELDET: " + s.account.toUpperCase() : "NICHT ANGEMELDET";
        g.drawText (acc, head.removeFromTop (22), juce::Justification::centredLeft);

        // Notizen / Anmeldecode
        g.setColour (line);
        g.fillRect (notesArea.getX(), notesArea.getY(), notesArea.getWidth(), 1);
        auto na = notesArea.withTrimmedTop (10);
        if (s.phase == K54Updater::Phase::WaitingForBrowser && s.verifyCode.isNotEmpty())
        {
            g.setColour (creamDim);
            g.setFont (juce::FontOptions (14.0f));
            g.drawText ("Im Browser hat sich keller54.de geoeffnet. Code vergleichen und bestaetigen:", na.removeFromTop (22), juce::Justification::centredLeft);
            g.setColour (gold);
            g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 30.0f, 0.25f));
            g.drawText (s.verifyCode, na, juce::Justification::centredLeft);
        }
        else
        {
            g.setColour (creamDim);
            g.setFont (juce::FontOptions (14.0f));
            const auto txt = s.notes.isNotEmpty() ? "Neu in " + s.latest + ":\n" + s.notes
                                                  : juce::String ("Neue Versionen des K-54 gibt es kostenlos auf keller54.de. "
                                                                  "Einmal anmelden, dann reicht ein Klick zum Aktualisieren.");
            g.drawFittedText (txt, na, juce::Justification::topLeft, 5, 1.0f);
        }

        // Fortschritt
        if (s.phase == K54Updater::Phase::Downloading || s.phase == K54Updater::Phase::Installing)
        {
            auto pa = progressArea.toFloat();
            g.setColour (black);
            g.fillRoundedRectangle (pa, 4.0f);
            g.setColour (gold);
            g.fillRoundedRectangle (pa.withWidth (pa.getWidth() * (float) juce::jlimit (0.0, 1.0, s.progress)), 4.0f);
        }

        g.setColour (s.phase == K54Updater::Phase::Error ? juce::Colour (0xffE0554B) : cream);
        g.setFont (juce::FontOptions (13.5f));
        g.drawText (s.message, statusArea, juce::Justification::centredLeft, true);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! card.contains (e.getPosition()) && onClose)   // Klick daneben schliesst
            onClose();
    }

private:
    void timerCallback() override { refresh(); }

    void refresh()
    {
        const auto s = updater.get();
        using P = K54Updater::Phase;
        const bool running = updater.busy();

        accountButton.setButtonText (s.phase == P::WaitingForBrowser ? "ABBRECHEN" : (s.loggedIn ? "ABMELDEN" : "ANMELDEN"));
        accountButton.setEnabled (! running || s.phase == P::WaitingForBrowser);

        juce::String act;
        if (s.phase == P::Downloading)                 act = "ABBRECHEN";
        else if (s.updateAvailable && s.loggedIn)      act = "UPDATE LADEN & INSTALLIEREN";
        else if (s.updateAvailable)                    act = "ANMELDEN & UPDATE LADEN";
        else                                           act = "ERNEUT PRUEFEN";
        actionButton.setButtonText (act);
        actionButton.setToggleState (s.updateAvailable && s.phase != P::Downloading, juce::dontSendNotification);
        actionButton.setEnabled (! running || s.phase == P::Downloading);
        repaint();
    }

    K54Updater& updater;
    juce::TextButton actionButton { "ERNEUT PRUEFEN" }, accountButton { "ANMELDEN" }, closeButton { "X" }, siteButton { "KELLER54.DE" };
    juce::Rectangle<int> card, notesArea, progressArea, statusArea;
};

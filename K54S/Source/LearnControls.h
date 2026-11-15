#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

// ============================================================================
//  Regler, Auswahlfelder und Schalter mit Rechtsklick-Menue:
//    MIDI lernen / Zuweisung entfernen / Funktionen der DAW (Automation …)
//  Zugewiesene Regler zeigen klein "CC 74", lernende Regler einen orangen Rahmen.
// ============================================================================
namespace K54Learn
{
    inline juce::String u8 (const char* t) { return juce::String (juce::CharPointer_UTF8 (t)); }

    inline K55Processor* findProcessor (juce::Component& c)
    {
        if (auto* ed = c.findParentComponentOfClass<juce::AudioProcessorEditor>())
            return dynamic_cast<K55Processor*> (ed->getAudioProcessor());
        return nullptr;
    }

    inline juce::String describe (const K54MidiLearn::Assignment& a)
    {
        return "CC " + juce::String (a.cc) + (a.channel > 0 ? " / Kanal " + juce::String (a.channel) : juce::String());
    }

    inline void showMenu (juce::Component& c, const juce::String& paramID)
    {
        auto* proc = findProcessor (c);
        if (proc == nullptr) return;
        auto& learn = proc->midiLearn;
        const int idx = learn.indexOf (paramID);
        auto* param = proc->apvts.getParameter (paramID);
        if (idx < 0 || param == nullptr) return;

        const auto a = learn.getAssignment (idx);
        const bool learningThis = learn.getLearningParam() == idx;

        juce::PopupMenu m;
        m.addSectionHeader (param->getName (40).toUpperCase());
        if (learningThis)
            m.addItem (u8 ("Lernen abbrechen (Regler am Ger\xc3\xa4t bewegen \xe2\x80\xa6)"), [&learn] { learn.cancelLearning(); });
        else
            m.addItem (a.cc >= 0 ? u8 ("MIDI neu lernen \xe2\x80\xa6") : u8 ("MIDI lernen \xe2\x80\xa6"), [&learn, idx] { learn.startLearning (idx); });
        if (a.cc >= 0)
            m.addItem ("Zuweisung entfernen  (" + describe (a) + ")", [&learn, idx] { learn.removeParam (idx); });

        if (learn.numAssignments() > 0)
        {
            m.addSeparator();
            m.addItem (u8 ("Alle MIDI-Zuweisungen l\xc3\xb6schen (") + juce::String (learn.numAssignments()) + ")", [&learn] { learn.clearAll(); });
        }

        // Funktionen der DAW fuer diesen Parameter (z. B. Automation zeigen, Controller zuweisen)
        if (auto* ed = c.findParentComponentOfClass<juce::AudioProcessorEditor>())
            if (auto* host = ed->getHostContext())
                if (auto hostMenu = host->getContextMenuForParameter (param))
                {
                    auto sub = hostMenu->getEquivalentPopupMenu();
                    if (sub.getNumItems() > 0)
                    {
                        m.addSeparator();
                        m.addSubMenu ("DAW", sub);
                    }
                }

        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&c));
    }

    /** Gemeinsamer Teil: merkt sich den Parameter, hoert auf Aenderungen, zeichnet die Markierung */
    struct Tag : private juce::ChangeListener
    {
        juce::String paramID;
        juce::Component* owner = nullptr;
        K54MidiLearn* learn = nullptr;
        bool rightClick = false;

        ~Tag() override { if (learn != nullptr) learn->removeChangeListener (this); }

        void connect()
        {
            if (learn != nullptr || owner == nullptr || paramID.isEmpty()) return;
            if (auto* p = findProcessor (*owner))
            {
                learn = &p->midiLearn;
                learn->addChangeListener (this);
            }
        }
        void changeListenerCallback (juce::ChangeBroadcaster*) override { if (owner != nullptr) owner->repaint(); }

        /** true = Rechtsklick wurde hier behandelt */
        bool handleDown (const juce::MouseEvent& e)
        {
            rightClick = e.mods.isPopupMenu() && paramID.isNotEmpty();
            if (rightClick) showMenu (*owner, paramID);
            return rightClick;
        }

        void paintOver (juce::Graphics& g)
        {
            connect();
            if (learn == nullptr) return;
            const int idx = learn->indexOf (paramID);
            if (idx < 0) return;
            auto r = owner->getLocalBounds().toFloat();
            if (learn->getLearningParam() == idx)
            {
                g.setColour (K54Colours::gold);
                g.drawRoundedRectangle (r.reduced (1.0f), 4.0f, 2.0f);
                g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 8.0f, 0.06f));
                g.drawText ("MIDI?", r.removeFromTop (11.0f), juce::Justification::centredRight);
                return;
            }
            const auto a = learn->getAssignment (idx);
            if (a.cc >= 0)
            {
                g.setColour (K54Colours::gold.withAlpha (0.9f));
                g.setFont (K54Fonts::make (K54Fonts::syncopateBold(), 7.5f, 0.04f));
                g.drawText ("CC" + juce::String (a.cc), r.removeFromTop (10.0f), juce::Justification::centredRight);
            }
        }
    };
}

class LearnSlider : public juce::Slider
{
public:
    LearnSlider() { tag.owner = this; }
    void setParamID (const juce::String& id) { tag.paramID = id; }
    void mouseDown (const juce::MouseEvent& e) override { if (! tag.handleDown (e)) juce::Slider::mouseDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override { if (! tag.rightClick) juce::Slider::mouseDrag (e); }
    void mouseUp   (const juce::MouseEvent& e) override { if (! tag.rightClick) juce::Slider::mouseUp (e); tag.rightClick = false; }
    void paintOverChildren (juce::Graphics& g) override { tag.paintOver (g); }
private:
    K54Learn::Tag tag;
};

class LearnCombo : public juce::ComboBox
{
public:
    LearnCombo() { tag.owner = this; }
    void setParamID (const juce::String& id) { tag.paramID = id; }
    void mouseDown (const juce::MouseEvent& e) override { if (! tag.handleDown (e)) juce::ComboBox::mouseDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override { if (! tag.rightClick) juce::ComboBox::mouseDrag (e); }
    void mouseUp   (const juce::MouseEvent& e) override { if (! tag.rightClick) juce::ComboBox::mouseUp (e); tag.rightClick = false; }
    void paintOverChildren (juce::Graphics& g) override { tag.paintOver (g); }
private:
    K54Learn::Tag tag;
};

class LearnButton : public juce::TextButton
{
public:
    LearnButton() { tag.owner = this; }
    explicit LearnButton (const juce::String& text) : juce::TextButton (text) { tag.owner = this; }
    void setParamID (const juce::String& id) { tag.paramID = id; }
    void mouseDown (const juce::MouseEvent& e) override { if (! tag.handleDown (e)) juce::TextButton::mouseDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override { if (! tag.rightClick) juce::TextButton::mouseDrag (e); }
    void mouseUp   (const juce::MouseEvent& e) override { if (! tag.rightClick) juce::TextButton::mouseUp (e); tag.rightClick = false; }
    void paintOverChildren (juce::Graphics& g) override { tag.paintOver (g); }
private:
    K54Learn::Tag tag;
};

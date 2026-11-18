// K-54 Analyzer · Keller54 · by Frank E.
// Oberfläche der Abhör-Simulation: Knopf im Kopf + Fenster mit Geräten, Frequenzgang und Optionen
#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

namespace k54a
{

void drawDeviceIcon (juce::Graphics&, int icon, juce::Rectangle<float> r, juce::Colour c, float stroke = 1.6f);
juce::Colour deviceGroupColour (const juce::String& group);

//==============================================================================
// Knopf im Kopf: zeigt das gewählte Gerät, Klick öffnet das Fenster, Kreis rechts = an/aus
class DeviceButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit DeviceButton (AnalyzerProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void refresh();
    std::function<void()> onOpen;
    std::function<void (bool)> onToggle;

private:
    juce::Rectangle<float> powerArea() const;
    AnalyzerProcessor& proc;
    int device = 0;
    bool on = true, hover = false, limiting = false;
};

//==============================================================================
class DeviceTile : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    int index = 0;
    bool selected = false, hover = false, simOn = true;
    std::function<void()> onClick;
};

class ResponseView : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    int device = 0, volume = 1;
    float trimDb = 0.0f;
    bool on = true;
};

//==============================================================================
class DevicePanel : public juce::Component
{
public:
    explicit DevicePanel (AnalyzerProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override { if (! box.contains (e.position)) setVisible (false); }
    bool keyPressed (const juce::KeyPress&) override;
    void refresh();                       // Zustand aus den Parametern holen (Timer)
    std::function<void (const char*, float)> setParam;

private:
    struct Segment
    {
        std::array<juce::TextButton, 3> b;
        void set (int v) { for (int i = 0; i < 3; ++i) b[(size_t) i].setToggleState (i == v, juce::dontSendNotification); }
    };
    void setupSegment (Segment&, const char* id, std::initializer_list<const char*> names);
    void selectDevice (int d);

    AnalyzerProcessor& proc;
    std::vector<std::unique_ptr<DeviceTile>> tiles;
    std::vector<std::pair<juce::String, juce::Rectangle<float>>> groupLabels;
    ResponseView response;
    juce::TextButton btnOn, btnMatch, btnClose;
    Segment segVol, segNoise, segSeat;
    juce::Rectangle<float> box, infoArea, optArea;
    juce::Rectangle<int> rowVol, rowNoise, rowSeat, rowMatch, rowOn;
    int device = 0, noiseLevel = 0;
    bool on = true, isCar = false;
    float limiterDb = 0.0f;
    bool clipping = false;
};

} // namespace k54a

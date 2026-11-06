#pragma once
#include "PanelKit.h"

// ---------------------------------------------------------------------------
//  Sample-Browser: Apple Loops / Logic Sound Library, Sample-Platten, eigene Ordner
//  Klick = vorhoeren, Doppelklick oder LADEN = als Sample uebernehmen
// ---------------------------------------------------------------------------
class SampleBrowser : public juce::Component, private juce::ListBoxModel, private juce::Timer
{
public:
    struct Source { juce::String name; juce::Array<juce::File> roots; bool custom = false; };

    explicit SampleBrowser (K55Processor& p) : proc (p), scanner (*this)
    {
        buildSources();
        addAndMakeVisible (sourceBox);
        for (int i = 0; i < sources.size(); ++i) sourceBox.addItem (sources[i].name.toUpperCase(), i + 1);
        sourceBox.onChange = [this] { selectSource (sourceBox.getSelectedItemIndex()); };
        sourceBox.setTooltip ("Wo gesucht wird: Logic-Loops, Sample-Platten, Aufnahmen oder eigene Ordner");

        addAndMakeVisible (search);
        search.setTextToShowWhenEmpty ("SUCHEN (Z. B. KICK 120 PIANO)", K54Colours::creamDim.withAlpha (0.5f));
        search.setFont (K54Fonts::make (K54Fonts::syncopate(), 10.0f, 0.04f));
        search.onTextChange = [this] { applyFilter(); };

        addAndMakeVisible (list);
        list.setModel (this);
        list.setRowHeight (24);
        list.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff0E0E10));
        list.setColour (juce::ListBox::outlineColourId, K54Colours::line);
        list.setOutlineThickness (1);

        for (auto* b : { &playBtn, &loadBtn, &addBtn, &autoBtn })
            addAndMakeVisible (b);
        autoBtn.setClickingTogglesState (true);
        autoBtn.setToggleState (true, juce::dontSendNotification);
        autoBtn.setTooltip ("An: Klick auf eine Datei spielt sie sofort vor");
        playBtn.setTooltip ("Ausgewaehlte Datei vorhoeren / stoppen");
        loadBtn.setTooltip ("Ausgewaehlte Datei als Sample laden (oder Doppelklick)");
        addBtn.setTooltip ("Eigenen Ordner zur Liste hinzufuegen");
        playBtn.onClick = [this] { if (proc.previewPlaying.load()) proc.stopPreview(); else previewSelected(); };
        loadBtn.onClick = [this] { loadSelected(); };
        addBtn.onClick  = [this] { addFolder(); };

        sourceBox.setSelectedItemIndex (0, juce::sendNotificationSync);
        startTimerHz (8);
    }

    ~SampleBrowser() override { scanner.stopThread (3000); proc.stopPreview(); }

    std::function<void (const juce::File&)> onLoad;

    void resized() override
    {
        auto r = getLocalBounds();
        auto top = r.removeFromTop (28);
        addBtn.setBounds (top.removeFromRight (40));
        top.removeFromRight (6);
        sourceBox.setBounds (top);
        r.removeFromTop (6);
        search.setBounds (r.removeFromTop (26));
        r.removeFromTop (6);
        auto bottom = r.removeFromBottom (28);
        r.removeFromBottom (6);
        list.setBounds (r);
        playBtn.setBounds (bottom.removeFromLeft (90)); bottom.removeFromLeft (6);
        autoBtn.setBounds (bottom.removeFromLeft (110)); bottom.removeFromLeft (6);
        loadBtn.setBounds (bottom);
    }

    void paint (juce::Graphics& g) override
    {
        if (status.isNotEmpty())
        {
            g.setColour (K54Colours::creamDim.withAlpha (0.7f));
            g.setFont (K54Fonts::make (K54Fonts::syncopate(), 8.5f, 0.12f));
            g.drawText (status, list.getBounds().reduced (10).removeFromBottom (16), juce::Justification::centredRight);
        }
    }

    // --- vom Scanner (Message-Thread)
    void scanFinished (int sourceIndex, juce::Array<juce::File> found, bool truncated)
    {
        if (sourceIndex != current) return;
        files = std::move (found);
        files.sort (sorter);
        scanning = false;
        this->truncated = truncated;
        applyFilter();
    }

private:
    // ------------------------------------------------------------ Quellen
    static juce::File settingsFile()
    {
        return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Keller54").getChildFile ("K-54 S Browser.xml");
    }

    void buildSources()
    {
        const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
        auto add = [this] (const juce::String& n, juce::Array<juce::File> roots, bool custom = false)
        {
            juce::Array<juce::File> ok;
            for (auto& f : roots) if (f.isDirectory()) ok.add (f);
            if (! ok.isEmpty() || custom) sources.add ({ n, ok, custom });
        };
        // Logic: Apple Loops (auch wenn die Sound-Library auf eine andere Platte verschoben wurde)
        juce::Array<juce::File> loops { juce::File ("/Library/Audio/Apple Loops"), home.getChildFile ("Library/Audio/Apple Loops") };
        juce::Array<juce::File> logicLib { juce::File ("/Library/Application Support/Logic"), home.getChildFile ("Music/Audio Music Apps") };
        juce::Array<juce::File> volSamples;
        for (auto& v : juce::File ("/Volumes").findChildFiles (juce::File::findDirectories, false))
        {
            for (auto* n : { "Logic Sound Library", "Apple Loops", "Sound Library" })
                if (v.getChildFile (n).isDirectory()) logicLib.add (v.getChildFile (n));
            for (auto* n : { "Samples", "Sample", "Loops" })
                if (v.getChildFile (n).isDirectory()) volSamples.add (v.getChildFile (n));
        }
        add ("Logic Apple Loops", loops);
        add ("Logic Sound Library", logicLib);
        add ("Sample-Platten", volSamples);
        add ("K-54 S Aufnahmen", { K55Processor::getSampleFolder() });
        add ("Musik", { juce::File::getSpecialLocation (juce::File::userMusicDirectory) });

        if (auto xml = juce::XmlDocument::parse (settingsFile()))
            for (auto* e : xml->getChildWithTagNameIterator ("folder"))
            {
                const juce::File f (e->getStringAttribute ("path"));
                if (f.isDirectory()) add (f.getFileName(), { f }, true);
            }
        if (sources.isEmpty()) add ("Musik", { home });
    }

    void saveCustom()
    {
        juce::XmlElement xml ("K54SBrowser");
        for (auto& s : sources)
            if (s.custom && ! s.roots.isEmpty())
                xml.createNewChildElement ("folder")->setAttribute ("path", s.roots[0].getFullPathName());
        settingsFile().getParentDirectory().createDirectory();
        xml.writeTo (settingsFile());
    }

    void addFolder()
    {
        chooser = std::make_unique<juce::FileChooser> ("Ordner mit Samples/Loops", juce::File::getSpecialLocation (juce::File::userMusicDirectory));
        juce::Component::SafePointer<SampleBrowser> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories, [safe] (const juce::FileChooser& fc)
        {
            if (safe == nullptr || ! fc.getResult().isDirectory()) return;
            safe->sources.add ({ fc.getResult().getFileName(), { fc.getResult() }, true });
            safe->saveCustom();
            safe->sourceBox.addItem (fc.getResult().getFileName().toUpperCase(), safe->sources.size());
            safe->sourceBox.setSelectedItemIndex (safe->sources.size() - 1, juce::sendNotificationSync);
        });
    }

    void selectSource (int idx)
    {
        if (! juce::isPositiveAndBelow (idx, sources.size())) return;
        current = idx;
        files.clear();
        shown.clear();
        list.updateContent();
        scanning = true;
        status = "SUCHE DATEIEN ...";
        repaint();
        scanner.request (idx, sources[idx].roots);
    }

    void applyFilter()
    {
        shown.clear();
        juce::StringArray words;
        words.addTokens (search.getText().toLowerCase(), " ", "");
        words.removeEmptyStrings();
        for (int i = 0; i < files.size(); ++i)
        {
            const auto n = files.getReference (i).getFileNameWithoutExtension().toLowerCase()
                         + " " + files.getReference (i).getParentDirectory().getFileName().toLowerCase();
            bool ok = true;
            for (auto& w : words) if (! n.contains (w)) { ok = false; break; }
            if (ok) shown.add (i);
        }
        list.updateContent();
        list.repaint();
        status = scanning ? juce::String ("SUCHE DATEIEN ...")
                          : juce::String (shown.size()) + " VON " + juce::String (files.size()) + " DATEIEN" + (truncated ? " (ERSTE 20000)" : "");
        repaint();
    }

    juce::File selectedFile() const
    {
        const int row = list.getSelectedRow();
        return juce::isPositiveAndBelow (row, shown.size()) ? files[shown[row]] : juce::File();
    }
    void previewSelected() { auto f = selectedFile(); if (f.existsAsFile()) proc.previewFile (f); }
    void loadSelected()    { auto f = selectedFile(); if (f.existsAsFile()) { proc.stopPreview(); if (onLoad) onLoad (f); } }

    // ------------------------------------------------------------ Liste
    int getNumRows() override { return shown.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        using namespace K54Colours;
        if (! juce::isPositiveAndBelow (row, shown.size())) return;
        const auto& f = files.getReference (shown[row]);
        if (selected) { g.setColour (goldDim.withAlpha (0.55f)); g.fillRect (0, 0, w, h); }
        else if (row % 2) { g.setColour (juce::Colours::white.withAlpha (0.02f)); g.fillRect (0, 0, w, h); }
        g.setColour (selected ? cream : cream.withAlpha (0.9f));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 9.5f, 0.02f));
        auto r = juce::Rectangle<int> (8, 0, w - 16, h);
        g.drawText (f.getFileNameWithoutExtension().toUpperCase(), r.removeFromLeft ((int) (w * 0.62f)), juce::Justification::centredLeft, true);
        g.setColour (creamDim.withAlpha (0.6f));
        g.setFont (K54Fonts::make (K54Fonts::syncopate(), 7.5f, 0.05f));
        g.drawText (f.getParentDirectory().getFileName().toUpperCase(), r, juce::Justification::centredRight, true);
    }
    void selectedRowsChanged (int) override { if (autoBtn.getToggleState()) previewSelected(); }
    void listBoxItemDoubleClicked (int, const juce::MouseEvent&) override { loadSelected(); }
    void returnKeyPressed (int) override { loadSelected(); }
    juce::var getDragSourceDescription (const juce::SparseSet<int>&) override { return {}; }

    void timerCallback() override
    {
        const bool p = proc.previewPlaying.load();
        if (p != lastPlaying) { lastPlaying = p; playBtn.setButtonText (p ? ParamChoices::u8 ("\xe2\x96\xa0 STOP") : ParamChoices::u8 ("\xe2\x96\xb6 H\xc3\x96REN")); }
    }

    // ------------------------------------------------------------ Hintergrund-Suche
    class Scanner : public juce::Thread
    {
    public:
        explicit Scanner (SampleBrowser& b) : juce::Thread ("K-54 S Browser"), owner (b) {}
        void request (int idx, const juce::Array<juce::File>& roots)
        {
            stopThread (2000);
            index = idx;
            dirs = roots;
            startThread();
        }
        void run() override
        {
            juce::Array<juce::File> found;
            bool cut = false;
            for (auto& d : dirs)
            {
                for (const auto& e : juce::RangedDirectoryIterator (d, true, "*.wav;*.wave;*.aif;*.aiff;*.caf;*.flac;*.mp3;*.m4a;*.ogg",
                                                                     juce::File::findFiles, juce::File::FollowSymlinks::noCycles))
                {
                    if (threadShouldExit()) return;
                    found.add (e.getFile());
                    if (found.size() >= 20000) { cut = true; break; }
                }
                if (cut) break;
            }
            juce::Component::SafePointer<SampleBrowser> safe (&owner);
            const int idx = index;
            juce::MessageManager::callAsync ([safe, idx, found, cut] { if (safe != nullptr) safe->scanFinished (idx, found, cut); });
        }
    private:
        SampleBrowser& owner;
        int index = 0;
        juce::Array<juce::File> dirs;
    };

    struct NameSorter
    {
        static int compareElements (const juce::File& a, const juce::File& b) { return a.getFileName().compareNatural (b.getFileName()); }
    } sorter;

    K55Processor& proc;
    juce::Array<Source> sources;
    juce::Array<juce::File> files;
    juce::Array<int> shown;
    int current = -1;
    bool scanning = false, truncated = false, lastPlaying = true;
    juce::String status;
    juce::ComboBox sourceBox;
    juce::TextEditor search;
    juce::ListBox list;
    juce::TextButton playBtn { ParamChoices::u8 ("\xe2\x96\xb6 H\xc3\x96REN") }, loadBtn { "LADEN" }, addBtn { "+" }, autoBtn { ParamChoices::u8 ("AUTO-H\xc3\x96REN") };
    std::unique_ptr<juce::FileChooser> chooser;
    Scanner scanner;
};

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

// ============================================================================
//  MIDI-Learn fuer den K-54 S
//  - Rechtsklick auf einen Regler > "MIDI lernen", dann am Hardware-Controller
//    einen Regler/Fader/Taster bewegen -> fest zugewiesen (CC + Kanal).
//  - Der Audio-Thread liest die CCs nur aus und merkt sich den Wert.
//    Der Message-Thread gibt ihn ~60x pro Sekunde mit Begin/End-Geste an den
//    Parameter weiter. So sieht jede DAW die Bewegung wie eine Mausbewegung
//    und kann sie als Automation aufnehmen (Touch/Latch/Write).
//  - Zuweisungen werden im Projekt gespeichert und zusaetzlich als Standard
//    fuer neue Instanzen (gleiche Hardware in jedem Projekt).
// ============================================================================
class K54MidiLearn : public juce::ChangeBroadcaster, private juce::Timer
{
public:
    static constexpr int numCC = 128;

    explicit K54MidiLearn (juce::AudioProcessor& p) : proc (p)
    {
        for (auto& m : map) { m.param.store (-1); m.channel.store (0); }
        auto& params = proc.getParameters();
        pending = std::make_unique<std::atomic<float>[]> ((size_t) params.size());
        gestureEnd.assign ((size_t) params.size(), 0);
        for (int i = 0; i < params.size(); ++i) pending[(size_t) i].store (-1.0f);
        loadDefaults();
        startTimerHz (60);
    }

    ~K54MidiLearn() override { stopTimer(); }

    // ------------------------------------------------------------ Audio-Thread
    /** Liest CCs aus dem Puffer. Zugewiesene CCs werden entfernt, damit sie nicht
        zusaetzlich als Mod-Rad, Sustain usw. wirken. */
    void process (juce::MidiBuffer& midi)
    {
        if (! anyMapped.load (std::memory_order_relaxed) && learnParam.load (std::memory_order_relaxed) < 0)
            return;

        bool removeAny = false;
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (! m.isController()) continue;
            const int cc = m.getControllerNumber(), ch = m.getChannel();
            if (! isLearnable (cc)) continue;

            const int target = learnParam.load (std::memory_order_relaxed);
            if (target >= 0)
            {
                learnedCC.store (cc); learnedChannel.store (ch);
                learnParam.store (-1);
                learnHit.store (target);           // Message-Thread traegt ein
                removeAny = true;
                continue;
            }
            const int idx = map[(size_t) cc].param.load (std::memory_order_relaxed);
            const int mch = map[(size_t) cc].channel.load (std::memory_order_relaxed);
            if (idx >= 0 && (mch == 0 || mch == ch))
            {
                pending[(size_t) idx].store ((float) m.getControllerValue() / 127.0f, std::memory_order_relaxed);
                removeAny = true;
            }
        }
        if (! removeAny) return;

        juce::MidiBuffer keep;
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isController() && isLearnable (m.getControllerNumber()))
            {
                const int cc = m.getControllerNumber();
                const int idx = map[(size_t) cc].param.load (std::memory_order_relaxed);
                const int mch = map[(size_t) cc].channel.load (std::memory_order_relaxed);
                if (idx >= 0 && (mch == 0 || mch == m.getChannel())) continue;
                if (learnHit.load (std::memory_order_relaxed) >= 0 && cc == learnedCC.load (std::memory_order_relaxed)) continue;
            }
            keep.addEvent (meta.data, meta.numBytes, meta.samplePosition);
        }
        midi.swapWith (keep);
    }

    // ------------------------------------------------------------ Message-Thread
    /** 0 = Bank-Select, 120-127 = Kanal-Modus (All Notes Off usw.) – nie lernen */
    static bool isLearnable (int cc) { return cc > 0 && cc != 32 && cc < 120; }

    void startLearning (int paramIndex) { learnParam.store (paramIndex); sendChangeMessage(); }
    void cancelLearning()               { learnParam.store (-1); sendChangeMessage(); }
    int  getLearningParam() const       { return learnParam.load(); }

    struct Assignment { int cc = -1, channel = 0; };
    Assignment getAssignment (int paramIndex) const
    {
        for (int cc = 0; cc < numCC; ++cc)
            if (map[(size_t) cc].param.load() == paramIndex)
                return { cc, map[(size_t) cc].channel.load() };
        return {};
    }

    void assign (int cc, int channel, int paramIndex, bool save = true)
    {
        if (! isLearnable (cc)) return;
        for (auto& m : map)                                   // ein Parameter = ein CC
            if (m.param.load() == paramIndex) m.param.store (-1);
        map[(size_t) cc].channel.store (channel);
        map[(size_t) cc].param.store (paramIndex);
        updateAnyMapped();
        if (save) saveDefaults();
        sendChangeMessage();
    }

    void removeParam (int paramIndex)
    {
        for (auto& m : map)
            if (m.param.load() == paramIndex) m.param.store (-1);
        updateAnyMapped();
        saveDefaults();
        sendChangeMessage();
    }

    void clearAll()
    {
        for (auto& m : map) m.param.store (-1);
        learnParam.store (-1);
        updateAnyMapped();
        saveDefaults();
        sendChangeMessage();
    }

    int numAssignments() const
    {
        int n = 0;
        for (auto& m : map) if (m.param.load() >= 0) ++n;
        return n;
    }

    // ------------------------------------------------------------ Speichern
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree t ("MIDILEARN");
        auto& params = proc.getParameters();
        for (int cc = 0; cc < numCC; ++cc)
        {
            const int idx = map[(size_t) cc].param.load();
            if (auto* rp = juce::isPositiveAndBelow (idx, params.size()) ? dynamic_cast<juce::AudioProcessorParameterWithID*> (params[idx]) : nullptr)
            {
                juce::ValueTree e ("CC");
                e.setProperty ("cc", cc, nullptr);
                e.setProperty ("ch", map[(size_t) cc].channel.load(), nullptr);
                e.setProperty ("param", rp->getParameterID(), nullptr);
                t.appendChild (e, nullptr);
            }
        }
        return t;
    }

    void fromValueTree (const juce::ValueTree& t, bool save)
    {
        if (! t.hasType ("MIDILEARN")) return;
        for (auto& m : map) m.param.store (-1);
        for (const auto& e : t)
        {
            const int cc = (int) e.getProperty ("cc", -1);
            const int idx = indexOf (e.getProperty ("param").toString());
            if (isLearnable (cc) && idx >= 0)
            {
                map[(size_t) cc].channel.store (juce::jlimit (0, 16, (int) e.getProperty ("ch", 0)));
                map[(size_t) cc].param.store (idx);
            }
        }
        updateAnyMapped();
        if (save) saveDefaults();
        sendChangeMessage();
    }

    int indexOf (const juce::String& paramID) const
    {
        auto& params = proc.getParameters();
        for (int i = 0; i < params.size(); ++i)
            if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*> (params[i]))
                if (p->getParameterID() == paramID) return i;
        return -1;
    }

    static juce::File getDefaultsFile()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Keller54").getChildFile ("K-54 S").getChildFile ("MIDI-Zuweisungen.xml");
    }

private:
    struct Slot { std::atomic<int> param { -1 }, channel { 0 }; };
    std::array<Slot, numCC> map;
    std::atomic<bool> anyMapped { false };
    std::atomic<int> learnParam { -1 }, learnHit { -1 }, learnedCC { -1 }, learnedChannel { 0 };
    std::unique_ptr<std::atomic<float>[]> pending;
    std::vector<juce::uint32> gestureEnd;       // 0 = keine offene Geste
    juce::AudioProcessor& proc;

    void updateAnyMapped()
    {
        bool any = false;
        for (auto& m : map) any = any || m.param.load() >= 0;
        anyMapped.store (any);
    }

    void timerCallback() override
    {
        const int hit = learnHit.exchange (-1);
        if (hit >= 0)
            assign (learnedCC.load(), learnedChannel.load(), hit);
        flushPending (false);
    }

    void flushPending (bool endAll)
    {
        auto& params = proc.getParameters();
        const auto now = juce::Time::getMillisecondCounter();
        for (int i = 0; i < params.size(); ++i)
        {
            const float v = pending[(size_t) i].exchange (-1.0f, std::memory_order_relaxed);
            auto* p = params[i];
            auto& end = gestureEnd[(size_t) i];
            if (v >= 0.0f)
            {
                if (end == 0) p->beginChangeGesture();
                p->setValueNotifyingHost (v);
                end = now + 400;                               // Geste bleibt offen, solange gedreht wird
            }
            else if (end != 0 && (endAll || (int) (now - end) >= 0))
            {
                p->endChangeGesture();
                end = 0;
            }
        }
    }

    void loadDefaults()
    {
        if (auto xml = juce::XmlDocument::parse (getDefaultsFile()))
            fromValueTree (juce::ValueTree::fromXml (*xml), false);
    }

    void saveDefaults() const
    {
        auto f = getDefaultsFile();
        f.getParentDirectory().createDirectory();
        if (auto xml = toValueTree().createXml()) xml->writeTo (f);
    }

    JUCE_DECLARE_NON_COPYABLE (K54MidiLearn)
};

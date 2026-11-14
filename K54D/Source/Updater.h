#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_cryptography/juce_cryptography.h>

// ============================================================================
//  K-54 D Updater (abgeleitet vom K-54 Updater)
//  - prueft auf keller54.de, ob es eine neue Version gibt (oeffentlich, product=k54d)
//  - Einstellungen bleiben beim Update erhalten: DAW-Projekte speichern den Zustand selbst,
//    eigene Presets/Samples liegen in ~/Music/Keller54, das Konto hier in Application Support;
//    der Installer ersetzt nur die Plugin-Dateien
//  - verbindet das Plugin mit dem keller54.de-Konto (Bestaetigung im Browser,
//    kein Passwort im Plugin) und merkt sich nur einen widerrufbaren Schluessel
//  - laedt den Installer, prueft die Pruefsumme und startet die Installation
//  Alle Netzwerkzugriffe laufen in einem eigenen Thread.
// ============================================================================

class K54DUpdater : private juce::Thread
{
public:
    enum class Phase { Idle, Checking, WaitingForBrowser, Downloading, Installing, Done, Error };

    struct Snapshot
    {
        Phase phase = Phase::Idle;
        juce::String installed, latest, notes, account, message, verifyCode, pageUrl;
        bool updateAvailable = false, loggedIn = false, fileAvailable = false;
        double progress = 0.0;
        juce::Image avatar;
        juce::int64 size = 0;
    };

    K54DUpdater() : juce::Thread ("K54D Updater")
    {
        juce::PropertiesFile::Options o;
        o.applicationName     = "K-54 D";
        o.folderName          = "Keller54";
        o.filenameSuffix      = "settings";
        o.osxLibrarySubFolder = "Application Support";
        props = std::make_unique<juce::PropertiesFile> (o);

        auto env = juce::SystemStats::getEnvironmentVariable ("K54_UPDATE_SERVER", {});
        server = env.isNotEmpty() ? env.trimCharactersAtEnd ("/") : juce::String ("https://keller54.de");

        // gemeinsames Keller54-Konto: ist der K-54 schon verbunden, dessen Schluessel uebernehmen
        if (props->getValue ("token").isEmpty())
        {
            juce::PropertiesFile::Options k;
            k.applicationName = "K-54"; k.folderName = "Keller54"; k.filenameSuffix = "settings"; k.osxLibrarySubFolder = "Application Support";
            juce::PropertiesFile k54 (k);
            if (k54.getValue ("token").isNotEmpty())
            {
                props->setValue ("token", k54.getValue ("token"));
                props->setValue ("account", k54.getValue ("account"));
                props->saveIfNeeded();
            }
        }

        state.installed = installedVersion();
        state.account   = props->getValue ("account");
        state.loggedIn  = props->getValue ("token").isNotEmpty();
        loadCachedAvatar();
    }

    ~K54DUpdater() override
    {
        signalThreadShouldExit();
        stopThread (8000);
    }

    static juce::String installedVersion()
    {
       #ifdef JucePlugin_VersionString
        return JucePlugin_VersionString;
       #else
        return "0.0.0";
       #endif
    }

    Snapshot get() const { const juce::ScopedLock sl (lock); return state; }
    bool busy() const    { return isThreadRunning(); }

    // ---------------------------------------------------------------- Aktionen
    /** Pruefung im Hintergrund; force=false prueft hoechstens alle 6 Stunden. */
    void checkNow (bool force)
    {
        if (! force)
        {
            const auto last = props->getValue ("lastCheck").getLargeIntValue();
            if (juce::Time::currentTimeMillis() - last < 6LL * 3600 * 1000)
            {
                // Ergebnis der letzten Pruefung anzeigen
                const juce::ScopedLock sl (lock);
                state.latest = props->getValue ("latestVersion");
                state.updateAvailable = isNewer (state.latest, state.installed);
                return;
            }
        }
        startJob (Job::Check);
    }

    void login()    { startJob (Job::Login); }
    void download() { startJob (Job::Download); }
    void cancel()   { signalThreadShouldExit(); }

    void logout()
    {
        const auto token = props->getValue ("token");
        props->removeValue ("token");
        props->removeValue ("account");
        props->saveIfNeeded();
        {
            const juce::ScopedLock sl (lock);
            state.loggedIn = false; state.account = {}; state.message = "Abgemeldet.";
        }
        if (token.isNotEmpty())   // Schluessel auch auf dem Server widerrufen (best effort)
            juce::Thread::launch ([srv = server, token]
            {
                request (srv + "/wp-json/k54/v1/logout", "{}", token, nullptr);
            });
    }

    juce::String accountPage() const
    {
        const juce::ScopedLock sl (lock);
        return state.pageUrl.isNotEmpty() ? state.pageUrl : server + "/k54d/";
    }

    static juce::String product() { return "k54d"; }

    static juce::String platformName()
    {
       #if JUCE_WINDOWS
        return "windows";
       #else
        return "mac";
       #endif
    }

    static bool isNewer (const juce::String& a, const juce::String& b)
    {
        auto pa = juce::StringArray::fromTokens (a, ".", {}), pb = juce::StringArray::fromTokens (b, ".", {});
        for (int i = 0; i < juce::jmax (pa.size(), pb.size()); ++i)
        {
            const int x = pa[i].getIntValue(), y = pb[i].getIntValue();
            if (x != y) return x > y;
        }
        return false;
    }

private:
    enum class Job { Check, Login, Download };

    void startJob (Job j)
    {
        if (isThreadRunning()) return;
        job = j;
        startThread();
    }

    void set (Phase p, const juce::String& msg, double prog = -1.0)
    {
        const juce::ScopedLock sl (lock);
        state.phase = p;
        state.message = msg;
        if (prog >= 0.0) state.progress = prog;
    }

    /** HTTP-Anfrage; liefert Text, setzt status. post leer = GET. */
    static juce::String request (const juce::String& url, const juce::String& post, const juce::String& token, int* status)
    {
        juce::URL u (url);
        juce::String headers = "Accept: application/json\r\nUser-Agent: K-54-S/" + installedVersion();
        if (post.isNotEmpty())
        {
            u = u.withPOSTData (post);
            headers << "\r\nContent-Type: application/json";
        }
        if (token.isNotEmpty())
            headers << "\r\nX-K54-Token: " << token;

        int code = 0;
        auto opts = juce::URL::InputStreamOptions (post.isNotEmpty() ? juce::URL::ParameterHandling::inPostData
                                                                     : juce::URL::ParameterHandling::inAddress)
                        .withExtraHeaders (headers)
                        .withConnectionTimeoutMs (12000)
                        .withNumRedirectsToFollow (3)
                        .withStatusCode (&code);
        auto in = u.createInputStream (opts);
        if (status) *status = in != nullptr ? code : 0;
        return in != nullptr ? in->readEntireStreamAsString() : juce::String();
    }

    juce::var getJson (const juce::String& path, const juce::String& post, const juce::String& token, int& status)
    {
        const auto txt = request (server + "/wp-json/k54/v1/" + path, post, token, &status);
        return juce::JSON::parse (txt);
    }

    void run() override
    {
        switch (job)
        {
            case Job::Check:    runCheck (true); break;
            case Job::Login:    runLogin(); break;
            case Job::Download: if (runCheck (false)) runDownload(); break;
        }
    }

    bool runCheck (bool final)
    {
        set (Phase::Checking, "Suche nach Updates ...", 0.0);
        int st = 0;
        const auto v = getJson ("latest?product=" + product() + "&platform=" + platformName(), {}, {}, st);
        if (st != 200 || ! v.isObject())
        {
            set (Phase::Error, st == 0 ? "keller54.de ist nicht erreichbar. Internetverbindung pruefen."
                                       : "Update-Server antwortet nicht richtig (" + juce::String (st) + ").");
            return false;
        }
        {
            const juce::ScopedLock sl (lock);
            state.latest          = v["version"].toString();
            state.notes           = v["notes"].toString();
            state.pageUrl         = v["page"].toString();
            state.fileAvailable   = (bool) v["available"];
            state.size            = (juce::int64) v["size"];
            state.updateAvailable = isNewer (state.latest, state.installed);
            expectedSha           = v["sha256"].toString();
            fileName              = v["filename"].toString();
        }
        props->setValue ("lastCheck", juce::String (juce::Time::currentTimeMillis()));
        props->setValue ("latestVersion", v["version"].toString());
        props->saveIfNeeded();

        fetchAvatar (v["image"].toString());

        if (final)
        {
            const auto s = get();
            set (Phase::Idle, s.latest.isEmpty() ? "Noch keine Version auf keller54.de."
                              : (s.updateAvailable ? "Version " + s.latest + " ist verfuegbar."
                                                   : "Du hast die aktuelle Version."));
        }
        return true;
    }

    void runLogin()
    {
        set (Phase::Checking, "Verbinde mit keller54.de ...", 0.0);
        int st = 0;
        const auto dev = "K-54 D " + installedVersion() + " (" + juce::SystemStats::getOperatingSystemName() + ")";
        auto* body = new juce::DynamicObject();
        body->setProperty ("device", dev);
        const auto start = getJson ("device/start", juce::JSON::toString (juce::var (body), true), {}, st);
        if (st != 200 || ! start.isObject())
        {
            set (Phase::Error, "Anmeldung nicht moeglich (keller54.de nicht erreichbar).");
            return;
        }
        const auto deviceCode = start["device_code"].toString();
        const auto verifyUrl  = start["verify_url"].toString();
        const int interval    = juce::jlimit (2, 10, (int) start["interval"]);
        const int expires     = juce::jlimit (60, 1800, (int) start["expires_in"]);
        {
            const juce::ScopedLock sl (lock);
            state.verifyCode = start["user_code"].toString();
        }
        juce::MessageManager::callAsync ([verifyUrl] { juce::URL (verifyUrl).launchInDefaultBrowser(); });
        set (Phase::WaitingForBrowser, "Bitte im Browser anmelden und die Verbindung bestaetigen ...");

        const auto until = juce::Time::getMillisecondCounter() + (juce::uint32) expires * 1000;
        while (! threadShouldExit() && juce::Time::getMillisecondCounter() < until)
        {
            wait (interval * 1000);
            if (threadShouldExit()) break;
            const auto r = getJson ("device/poll", "{\"device_code\":\"" + deviceCode + "\"}", {}, st);
            const auto status = r["status"].toString();
            if (status == "approved")
            {
                props->setValue ("token", r["token"].toString());
                props->setValue ("account", r["name"].toString());
                props->saveIfNeeded();
                const juce::ScopedLock sl (lock);
                state.loggedIn = true; state.account = r["name"].toString(); state.verifyCode = {};
                state.phase = Phase::Idle; state.message = "Angemeldet als " + state.account + ".";
                return;
            }
            if (status == "expired")
                break;
        }
        {
            const juce::ScopedLock sl (lock);
            state.verifyCode = {};
        }
        set (threadShouldExit() ? Phase::Idle : Phase::Error,
             threadShouldExit() ? "Anmeldung abgebrochen." : "Die Anmeldung ist abgelaufen. Bitte erneut versuchen.");
    }

    void runDownload()
    {
        const auto token = props->getValue ("token");
        const auto s = get();
        if (token.isEmpty())      { set (Phase::Error, "Bitte zuerst anmelden."); return; }
        if (! s.fileAvailable)    { set (Phase::Error, "Auf keller54.de ist noch kein Installer hinterlegt."); return; }
       #if JUCE_WINDOWS
        // Windows braucht einen .msi- oder .exe-Installer; liefert der Server (noch) keinen, die Download-Seite oeffnen
        if (! (fileName.endsWithIgnoreCase (".exe") || fileName.endsWithIgnoreCase (".msi")))
        {
            const auto page = accountPage();
            juce::MessageManager::callAsync ([page] { juce::URL (page).launchInDefaultBrowser(); });
            set (Phase::Done, "Der Windows-Installer wird auf keller54.de angeboten - die Seite wurde geoeffnet.");
            return;
        }
       #endif

        auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Downloads");
        if (! dir.isDirectory()) dir = juce::File::getSpecialLocation (juce::File::tempDirectory);
        auto target = dir.getChildFile (fileName.isNotEmpty() ? juce::File::createLegalFileName (fileName)
                                                              : "K-54-D-" + s.latest + (platformName() == "windows" ? "-Setup.exe" : "-Installer.dmg"));
        auto part = target.withFileExtension ("part");
        part.deleteFile();

        set (Phase::Downloading, "Lade Version " + s.latest + " ...", 0.0);
        int code = 0;
        auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                        .withExtraHeaders ("X-K54-Token: " + token + "\r\nUser-Agent: K-54-S/" + installedVersion())
                        .withConnectionTimeoutMs (15000)
                        .withNumRedirectsToFollow (3)
                        .withStatusCode (&code);
        auto in = juce::URL (server + "/wp-json/k54/v1/download?product=" + product() + "&platform=" + platformName()).createInputStream (opts);
        if (in == nullptr || code != 200)
        {
            if (code == 401)
            {
                props->removeValue ("token"); props->saveIfNeeded();
                const juce::ScopedLock sl (lock);
                state.loggedIn = false;
            }
            set (Phase::Error, code == 401 ? "Anmeldung ist nicht mehr gueltig. Bitte neu anmelden."
                                           : "Download fehlgeschlagen (" + juce::String (code) + ").");
            return;
        }

        {
            juce::FileOutputStream out (part);
            if (! out.openedOk()) { set (Phase::Error, "Kann nicht in den Download-Ordner schreiben."); return; }
            const auto total = in->getTotalLength() > 0 ? in->getTotalLength() : s.size;
            juce::HeapBlock<char> buf (65536);
            juce::int64 done = 0;
            while (! in->isExhausted() && ! threadShouldExit())
            {
                const int n = in->read (buf.get(), 65536);
                if (n <= 0) break;
                out.write (buf.get(), (size_t) n);
                done += n;
                if (total > 0) set (Phase::Downloading, "Lade Version " + s.latest + " ... "
                                    + juce::String ((double) done / 1048576.0, 1) + " MB", (double) done / (double) total);
            }
            out.flush();
        }
        if (threadShouldExit()) { part.deleteFile(); set (Phase::Idle, "Download abgebrochen."); return; }

        // Pruefsumme
        if (expectedSha.isNotEmpty())
        {
            const auto got = juce::SHA256 (part).toHexString();
            if (! got.equalsIgnoreCase (expectedSha))
            {
                part.deleteFile();
                set (Phase::Error, "Die Datei ist beschaedigt angekommen. Bitte erneut versuchen.");
                return;
            }
        }
        target.deleteFile();
        part.moveFileTo (target);

        set (Phase::Installing, "Installer wird geoeffnet ...", 1.0);
        install (target);
    }

    void install (const juce::File& dmg)
    {
       #if JUCE_MAC
        juce::ChildProcess hd;
        if (hd.start (juce::StringArray { "/usr/bin/hdiutil", "attach", "-noverify", "-noautoopen", dmg.getFullPathName() }))
        {
            const auto out = hd.readAllProcessOutput();
            hd.waitForProcessToFinish (30000);
            // letzte Spalte der Ausgabe ist der Mount-Punkt (/Volumes/K-54 x.y.z)
            juce::String mount;
            for (auto& line : juce::StringArray::fromLines (out))
                if (line.contains ("/Volumes/"))
                    mount = line.fromFirstOccurrenceOf ("/Volumes/", true, false).trim();
            if (mount.isNotEmpty())
            {
                auto pkgs = juce::File (mount).findChildFiles (juce::File::findFiles, false, "*.pkg");
                if (! pkgs.isEmpty())
                {
                    pkgs.getFirst().startAsProcess();
                    set (Phase::Done, "Installer gestartet. DAW speichern und schliessen, installieren, neu starten - deine Einstellungen bleiben erhalten.");
                    return;
                }
            }
        }
       #endif
       #if JUCE_WINDOWS
        if (dmg.startAsProcess())   // MSI / Setup.exe starten (fragt selbst nach Admin-Rechten)
        {
            set (Phase::Done, "Setup gestartet. DAW speichern und schliessen, installieren, neu starten - deine Einstellungen bleiben erhalten.");
            return;
        }
       #endif
        dmg.startAsProcess();   // Fallback: Datei im Finder/Explorer oeffnen
        set (Phase::Done, "Installer liegt in Downloads und wurde geoeffnet: " + dmg.getFileName());
    }

    // ---------------------------------------------------------------- Profilbild
    juce::File avatarFile() const { return props->getFile().getSiblingFile ("K-54-D-avatar.img"); }

    void loadCachedAvatar()
    {
        auto f = avatarFile();
        if (f.existsAsFile())
        {
            auto img = juce::ImageFileFormat::loadFrom (f);
            const juce::ScopedLock sl (lock);
            state.avatar = img;
        }
    }

    void fetchAvatar (const juce::String& url)
    {
        if (url.isEmpty() || url == props->getValue ("avatarUrl")) return;
        juce::MemoryBlock mb;
        if (juce::URL (url).readEntireBinaryStream (mb) && mb.getSize() > 100 && mb.getSize() < 5 * 1024 * 1024)
        {
            auto img = juce::ImageFileFormat::loadFrom (mb.getData(), mb.getSize());
            if (img.isValid())
            {
                avatarFile().replaceWithData (mb.getData(), mb.getSize());
                props->setValue ("avatarUrl", url);
                props->saveIfNeeded();
                const juce::ScopedLock sl (lock);
                state.avatar = img;
            }
        }
    }

    std::unique_ptr<juce::PropertiesFile> props;
    juce::String server, expectedSha, fileName;
    Job job = Job::Check;
    juce::CriticalSection lock;
    Snapshot state;
};

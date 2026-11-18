// K-54 Analyzer · Keller54 · by Frank E.
// Abhör-Simulation: So klingt der Mix auf Handy, Laptop, Ohrhörern, Küchenradio, im Auto, im Club …
//
// Jedes Gerät ist ein Modell aus
//   - Frequenzgang (Hoch-/Tiefpass des Lautsprechers + Resonanzen, Gehäuse, Kabinen-Verstärkung)
//   - Stereo-Breite (Handy hochkant = Mono, Laptop = schmal, Kopfhörer = voll)
//   - eingebautem Limiter (Handy, Bluetooth-Box …) und Lautsprecher-Zerre bei Pegel
//   - Raum (Schreibtisch, Küche, Wohnzimmer, Auto-Innenraum, Club) mit frühen Reflexionen und Nachhall
//   - Sitzplatz im Auto (Fahrer / Mitte / Beifahrer: Laufzeit- und Pegelunterschied)
//   - Umgebungsgeräusch (Straße, Dunstabzug, Fahrgeräusch, Publikum, Raum, Lüfter)
// Pegel-Angleich: jedes Gerät wird auf gleiche Lautheit (K-gewichtet, rosa Rauschen) gebracht,
// damit man den Klang vergleicht und nicht die Lautstärke.
// Die Messungen des Analyzers bleiben unverändert – die Simulation wirkt nur auf das, was man hört.
#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace k54a
{

//==============================================================================
struct DevBq
{
    enum Type { LP, HP, Peak, LowShelf, HighShelf };
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;

    static DevBq make (int type, double f, double q, double gainDb, double sr)
    {
        DevBq c;
        f = std::min (f, sr * 0.45);
        const double w = 2.0 * 3.14159265358979323846 * f / sr;
        const double cw = std::cos (w), sw = std::sin (w);
        const double alpha = sw / (2.0 * q);
        const double A = std::pow (10.0, gainDb / 40.0);
        double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;
        switch (type)
        {
            case LP:   b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = (1 - cw) / 2; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            case HP:   b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = (1 + cw) / 2; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            case Peak: b0 = 1 + alpha * A; b1 = -2 * cw; b2 = 1 - alpha * A; a0 = 1 + alpha / A; a1 = -2 * cw; a2 = 1 - alpha / A; break;
            case LowShelf:
            {
                const double s = 2 * std::sqrt (A) * alpha;
                b0 = A * ((A + 1) - (A - 1) * cw + s); b1 = 2 * A * ((A - 1) - (A + 1) * cw); b2 = A * ((A + 1) - (A - 1) * cw - s);
                a0 = (A + 1) + (A - 1) * cw + s; a1 = -2 * ((A - 1) + (A + 1) * cw); a2 = (A + 1) + (A - 1) * cw - s;
                break;
            }
            case HighShelf:
            {
                const double s = 2 * std::sqrt (A) * alpha;
                b0 = A * ((A + 1) + (A - 1) * cw + s); b1 = -2 * A * ((A - 1) + (A + 1) * cw); b2 = A * ((A + 1) + (A - 1) * cw - s);
                a0 = (A + 1) - (A - 1) * cw + s; a1 = 2 * ((A - 1) - (A + 1) * cw); a2 = (A + 1) - (A - 1) * cw - s;
                break;
            }
            default: break;
        }
        c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0; c.a1 = a1 / a0; c.a2 = a2 / a0;
        return c;
    }

    double magDb (double f, double sr) const
    {
        const double w = 2.0 * 3.14159265358979323846 * f / sr;
        const double c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
        const double nr = b0 + b1 * c1 + b2 * c2, ni = -(b1 * s1 + b2 * s2);
        const double dr = 1 + a1 * c1 + a2 * c2, di = -(a1 * s1 + a2 * s2);
        const double m2 = (nr * nr + ni * ni) / std::max (1.0e-30, dr * dr + di * di);
        return 10.0 * std::log10 (std::max (1.0e-30, m2));
    }
};

struct DevBqState
{
    double z1 = 0, z2 = 0;
    inline double process (const DevBq& c, double x) noexcept
    {
        const double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return y;
    }
    void reset() noexcept { z1 = z2 = 0; }
};

//==============================================================================
namespace devsim
{
    enum Icon { icStudio, icMonitor, icCube, icPhone, icPhoneWide, icTablet, icLaptop, icTelephone,
                icEarbud, icTws, icHeadphone, icBtBox, icSmart, icTv, icRadio, icHifi, icCar, icCarSub, icClub };
    enum Room { rmNone, rmDesk, rmKitchen, rmLiving, rmCar, rmClub, rmStudio };
    enum Noise { nzNone, nzStreet, nzKitchen, nzCar, nzCrowd, nzLiving, nzOffice, numNoise };

    struct Band { int type; float f, gainDb, q; };

    struct Model
    {
        const char* name;
        const char* group;
        int icon;
        const char* info;
        const char* tip;
        float hp; int hpOrder;          // Hochpass (Lautsprecher-Grenze), Ordnung 2 oder 4, 0 = keiner
        float lp; int lpOrder;
        std::vector<Band> eq;
        float width;                     // 0 = Mono … 1 = volles Stereo
        float driveDb;                   // Zerre: Vorverstärkung in den Sättiger bei „Normal“ (0 = linear)
        float limitDb;                   // eingebauter Limiter, Schwelle in dBFS (0 = keiner)
        int room;
        int noise;
        bool car;
        bool headphones;
    };

    inline const std::vector<Model>& models()
    {
        using B = DevBq;
        static const std::vector<Model> m {
            { "Original (Studio)", "STUDIO", icStudio,
              u8"Keine Simulation – du hörst deinen Mix unverändert über deine Abhöre.",
              u8"Zum Vergleichen: zwischen Original und Gerät hin- und herschalten.",
              0, 0, 0, 0, {}, 1.0f, 0, 0, rmNone, nzNone, false, false },

            { "Kleine Monitore", "STUDIO", icMonitor,
              u8"Typische 5-Zoll-Nahfeldmonitore auf dem Schreibtisch: wenig Tiefbass, kleiner Raum.",
              u8"Stimmen Balance und Stereobild auch ohne Sub-Bass?",
              60, 2, 21000, 2, { { B::Peak, 78, 2.0f, 1.2f }, { B::Peak, 2500, 1.0f, 1.0f } },
              1.0f, 0, 0, rmStudio, nzOffice, false, false },

            { u8"Mittenwürfel", "STUDIO", icCube,
              u8"Kleiner Breitband-Würfel in Mono – der Studio-Klassiker, um die Mitten zu prüfen.",
              u8"Ist die Stimme verständlich? Hört man Bass und Kick noch über die Obertöne?",
              110, 4, 9500, 2, { { B::Peak, 300, 1.5f, 1.0f }, { B::Peak, 1300, 3.5f, 0.8f }, { B::Peak, 3500, -1.5f, 1.5f } },
              0.0f, 0, 0, rmDesk, nzOffice, false, false },

            { "Smartphone", "HANDY & COMPUTER", icPhone,
              u8"Handy-Lautsprecher, hochkant: kein Bass unter 600 Hz, Mono, eingebauter Limiter.",
              u8"Bleibt der Bass über seine Obertöne hörbar? Ist die Stimme vorne und nicht zu scharf?",
              650, 4, 12000, 2, { { B::Peak, 900, 5.0f, 1.6f }, { B::Peak, 1800, -2.0f, 1.2f }, { B::Peak, 4500, 4.0f, 1.4f },
                                  { B::HighShelf, 9000, -2.0f, 0.7f } },
              0.0f, 5.0f, -9.0f, rmNone, nzStreet, false, false },

            { "Smartphone quer", "HANDY & COMPUTER", icPhoneWide,
              u8"Handy quer gehalten (Video, Spiel): zwei winzige Lautsprecher, sehr schmales Stereo.",
              u8"Fallen Instrumente weg, die nur ganz außen im Panorama liegen?",
              500, 4, 12500, 2, { { B::Peak, 800, 4.5f, 1.5f }, { B::Peak, 1800, -2.0f, 1.2f }, { B::Peak, 4500, 3.5f, 1.4f },
                                  { B::HighShelf, 9000, -1.5f, 0.7f } },
              0.35f, 4.0f, -9.0f, rmNone, nzStreet, false, false },

            { "Tablet", "HANDY & COMPUTER", icTablet,
              u8"Tablet-Lautsprecher: etwas mehr Körper als ein Handy, Bass endet um 300 Hz.",
              u8"Tragen Bass und Kick über die Grundtöne? Ist das Stereo noch erkennbar?",
              280, 4, 15000, 2, { { B::Peak, 420, 4.0f, 1.4f }, { B::Peak, 1100, -1.5f, 1.0f }, { B::Peak, 3200, 2.5f, 1.2f } },
              0.5f, 3.0f, -8.0f, rmNone, nzLiving, false, false },

            { "Laptop", "HANDY & COMPUTER", icLaptop,
              u8"Laptop-Lautsprecher auf dem Tisch: Resonanz um 260 Hz, kaum Bass, schmales Stereo.",
              u8"Klingt der Mix dünn oder dröhnt es in den unteren Mitten?",
              200, 4, 16000, 2, { { B::Peak, 260, 5.0f, 2.0f }, { B::Peak, 650, -3.0f, 1.2f }, { B::Peak, 2600, 3.0f, 1.2f },
                                  { B::HighShelf, 10000, -2.0f, 0.7f } },
              0.6f, 4.0f, -9.0f, rmDesk, nzOffice, false, false },

            { "Telefon", "HANDY & COMPUTER", icTelephone,
              u8"Telefonleitung (300–3400 Hz), z. B. Warteschleife oder Sprachanruf.",
              u8"Bleiben Melodie und Text verständlich?",
              320, 4, 3400, 4, { { B::Peak, 1500, 2.0f, 0.8f } },
              0.0f, 4.0f, -10.0f, rmNone, nzStreet, false, false },

            { u8"Ohrhörer", u8"KOPFHÖRER", icEarbud,
              u8"Einfache Ohrhörer mit Kabel: sitzen locker, daher wenig Bass, spitze Höhen.",
              u8"Werden S-Laute und Becken unangenehm? Bleibt genug Bass übrig?",
              70, 2, 15000, 2, { { B::LowShelf, 180, -7.0f, 0.7f }, { B::Peak, 2700, 5.0f, 1.3f }, { B::Peak, 6500, 4.0f, 2.0f },
                                 { B::Peak, 10000, -3.0f, 1.0f } },
              1.0f, 2.0f, 0, rmNone, nzStreet, false, true },

            { "True-Wireless", u8"KOPFHÖRER", icTws,
              u8"Moderne kabellose In-Ears mit Bass-Anhebung – so hört heute ein Großteil der Leute.",
              u8"Wird der Bass zu dick? Ist die Stimme trotzdem klar?",
              20, 2, 17000, 2, { { B::LowShelf, 90, 6.0f, 0.7f }, { B::Peak, 300, -1.0f, 1.0f }, { B::Peak, 3000, 3.0f, 1.4f },
                                 { B::HighShelf, 9000, -2.5f, 0.7f } },
              1.0f, 0, -3.0f, rmNone, nzStreet, false, true },

            { u8"Kopfhörer bassbetont", u8"KOPFHÖRER", icHeadphone,
              u8"Geschlossener Consumer-Kopfhörer mit V-Klang: viel Bass, betonte Höhen.",
              u8"Matscht der Bass mit der Kick? Zischen die Höhen?",
              15, 2, 20000, 2, { { B::LowShelf, 95, 8.0f, 0.7f }, { B::Peak, 1000, -1.5f, 0.8f }, { B::Peak, 5800, 3.5f, 1.5f },
                                 { B::HighShelf, 12000, 1.0f, 0.7f } },
              1.0f, 0, 0, rmNone, nzNone, false, true },

            { "Bluetooth-Box", "ZUHAUSE", icBtBox,
              u8"Kleine Box mit Passivmembran: dicker Oberbass um 110 Hz, starker Limiter, fast Mono.",
              u8"Pumpt der Limiter mit der Kick? Dröhnt es bei 100 Hz?",
              75, 4, 14000, 2, { { B::Peak, 115, 6.0f, 1.6f }, { B::Peak, 320, -3.0f, 1.2f }, { B::Peak, 2500, 2.0f, 1.0f },
                                 { B::HighShelf, 9000, -2.0f, 0.7f } },
              0.15f, 3.0f, -12.0f, rmLiving, nzLiving, false, false },

            { "Smart-Speaker", "ZUHAUSE", icSmart,
              u8"Runder Smart-Speaker (Mono, 360°): warmer Bass, gedeckte Höhen, viel Raum.",
              u8"Bleiben Details und Stimme im Raum hörbar?",
              60, 4, 12500, 2, { { B::Peak, 90, 5.0f, 1.4f }, { B::Peak, 450, -2.0f, 1.0f }, { B::Peak, 3000, 1.5f, 1.0f },
                                 { B::HighShelf, 7500, -4.0f, 0.7f } },
              0.0f, 2.0f, -10.0f, rmLiving, nzLiving, false, false },

            { "Fernseher", "ZUHAUSE", icTv,
              u8"Flachbild-Fernseher mit nach unten strahlenden Lautsprechern im Wohnzimmer.",
              u8"Ist die Stimme verständlich? Fehlt das Fundament komplett?",
              130, 4, 13000, 2, { { B::Peak, 210, 3.0f, 1.5f }, { B::Peak, 550, -2.5f, 1.0f }, { B::Peak, 2500, 5.0f, 1.1f },
                                  { B::HighShelf, 8000, -3.0f, 0.7f } },
              0.45f, 2.0f, -10.0f, rmLiving, nzLiving, false, false },

            { u8"Küchenradio", "ZUHAUSE", icRadio,
              u8"Kleines Mono-Radio in der gefliesten Küche: harte Reflexionen, wenig Höhen, Zerre bei Pegel.",
              u8"Erkennt man den Song sofort? Setzt sich die Hookline durch?",
              150, 4, 6500, 4, { { B::Peak, 190, 4.0f, 2.0f }, { B::Peak, 1200, 3.0f, 0.9f }, { B::Peak, 3000, -2.0f, 1.5f } },
              0.0f, 7.0f, 0, rmKitchen, nzKitchen, false, false },

            { "HiFi Wohnzimmer", "ZUHAUSE", icHifi,
              u8"Gute Standboxen im Wohnzimmer: Raummoden – bei 50 Hz dröhnt es, bei 115 Hz fehlt Bass.",
              u8"Hängt der Bass an einzelnen Tönen? Ist der Mix im Raum zu hell?",
              35, 2, 20000, 2, { { B::Peak, 52, 5.0f, 4.0f }, { B::Peak, 115, -6.0f, 2.2f }, { B::Peak, 250, 2.0f, 2.0f },
                                 { B::HighShelf, 9000, -2.5f, 0.7f } },
              1.0f, 0, 0, rmLiving, nzLiving, false, false },

            { "Auto Kleinwagen", "AUTO", icCar,
              u8"Werks-Radio im Kleinwagen: Türlautsprecher, Kabinen-Bass, Loch um 160 Hz, Fahrgeräusch.",
              u8"Kommt die Stimme über das Fahrgeräusch? Dröhnt der Bass?",
              45, 2, 11500, 2, { { B::LowShelf, 70, 7.0f, 0.7f }, { B::Peak, 160, -5.0f, 1.4f }, { B::Peak, 480, 2.5f, 1.6f },
                                 { B::Peak, 1100, -3.0f, 1.0f }, { B::Peak, 3300, 3.0f, 1.4f } },
              1.0f, 3.0f, -6.0f, rmCar, nzCar, true, false },

            { "Auto Mittelklasse", "AUTO", icCar,
              u8"Typisches Auto-Soundsystem: starke Kabinen-Verstärkung im Bass, Hochtöner im Armaturenbrett.",
              u8"Wird der Bass zu dick? Sind Snare und Stimme bei Fahrgeräusch noch da?",
              35, 2, 14000, 2, { { B::LowShelf, 75, 9.0f, 0.7f }, { B::Peak, 190, -4.5f, 1.3f }, { B::Peak, 420, 2.0f, 1.5f },
                                 { B::Peak, 1000, -2.5f, 1.0f }, { B::Peak, 3200, 3.5f, 1.3f }, { B::HighShelf, 10000, 1.0f, 0.7f } },
              1.0f, 1.0f, 0, rmCar, nzCar, true, false },

            { "Auto mit Subwoofer", "AUTO", icCarSub,
              u8"Premium-Anlage mit Subwoofer und Loudness: sehr viel Tiefbass, glänzende Höhen.",
              u8"Bleibt der Sub-Bass kontrolliert? Werden die Höhen zu scharf?",
              25, 2, 16000, 2, { { B::LowShelf, 80, 12.0f, 0.7f }, { B::Peak, 45, 3.0f, 1.5f }, { B::Peak, 210, -4.0f, 1.2f },
                                 { B::Peak, 1000, -2.0f, 1.0f }, { B::Peak, 3500, 3.0f, 1.2f }, { B::HighShelf, 9000, 2.5f, 0.7f } },
              1.0f, 0, 0, rmCar, nzCar, true, false },

            { "Club / PA", "LIVE", icClub,
              u8"Club-Anlage mit Subs im großen Raum: Bass-Druck, Nachhall, Limiter, Publikum.",
              u8"Drückt die Kick? Verschwimmen Details im Hall?",
              32, 4, 16000, 2, { { B::LowShelf, 75, 9.0f, 0.7f }, { B::Peak, 52, 3.0f, 1.5f }, { B::Peak, 350, -2.0f, 1.0f },
                                 { B::Peak, 2500, 1.0f, 1.0f }, { B::HighShelf, 10000, -4.0f, 0.7f } },
              0.7f, 2.0f, -4.0f, rmClub, nzCrowd, false, false },
        };
        return m;
    }

    inline int numModels() { return (int) models().size(); }

    // Namen für die Umgebungsgeräusch-Stufen je Geräuschart
    inline const char* noiseName (int noise)
    {
        switch (noise)
        {
            case nzStreet:  return u8"Straße";
            case nzKitchen: return "Dunstabzug";
            case nzCar:     return u8"Fahrgeräusch";
            case nzCrowd:   return "Publikum";
            case nzLiving:  return "Raum";
            case nzOffice:  return u8"Lüfter";
            default:        return u8"Geräusch";
        }
    }
    inline const char* noiseLabel (int noise)   // Großbuchstaben für die Oberfläche
    {
        switch (noise)
        {
            case nzStreet:  return u8"STRASSE";
            case nzKitchen: return "DUNSTABZUG";
            case nzCar:     return u8"FAHRGERÄUSCH";
            case nzCrowd:   return "PUBLIKUM";
            case nzLiving:  return "RAUM";
            case nzOffice:  return u8"LÜFTER";
            default:        return u8"GERÄUSCH";
        }
    }
    inline const char* noiseLevelName (int noise, int level)
    {
        if (level <= 0) return "Aus";
        if (noise == nzCar) return level == 1 ? "Stadt 50" : "Autobahn 130";
        return level == 1 ? "Leise" : "Laut";
    }

    // Pegel des Umgebungsgeräuschs (dBFS RMS) bei Stufe 1 / 2
    inline float noiseLevelDb (int noise, int level)
    {
        static const float t[numNoise][2] = { { -200, -200 }, { -42, -30 }, { -40, -30 }, { -32, -22 },
                                              { -36, -25 }, { -52, -42 }, { -50, -40 } };
        if (level <= 0 || noise <= 0 || noise >= numNoise) return -200.0f;
        return t[noise][level - 1];
    }

    inline int numBiquads (const Model& m)
    {
        return (m.hpOrder >= 4 ? 2 : m.hpOrder > 0 ? 1 : 0) + (m.lpOrder >= 4 ? 2 : m.lpOrder > 0 ? 1 : 0) + (int) m.eq.size();
    }

    // Filterkette eines Geräts (+ Leise-Kontur), für Klang und Anzeige gleich
    inline std::vector<DevBq> makeChain (const Model& m, double sr, int volume)
    {
        std::vector<DevBq> c;
        auto butter = [&] (int type, float f, int order)
        {
            if (order <= 0 || f <= 0) return;
            if (order >= 4) { c.push_back (DevBq::make (type, f, 0.5412, 0, sr)); c.push_back (DevBq::make (type, f, 1.3066, 0, sr)); }
            else            c.push_back (DevBq::make (type, f, 0.7071, 0, sr));
        };
        butter (DevBq::HP, m.hp, m.hpOrder);
        if (m.lp < sr * 0.45) butter (DevBq::LP, m.lp, m.lpOrder);
        for (auto& b : m.eq)
            c.push_back (DevBq::make (b.type, b.f, b.q, b.gainDb, sr));
        if (volume == 0)   // leise hören: Ohr hört Bass und Höhen schwächer (Fletcher-Munson)
        {
            c.push_back (DevBq::make (DevBq::LowShelf, 150, 0.7, -6.0, sr));
            c.push_back (DevBq::make (DevBq::HighShelf, 8000, 0.7, -2.5, sr));
        }
        return c;
    }

    inline float volumeDb (int volume) { return volume <= 0 ? -8.0f : volume == 1 ? 0.0f : 6.0f; }
    inline const char* volumeName (int v) { return v <= 0 ? "Leise" : v == 1 ? "Normal" : "Laut"; }
    inline const char* seatName (int s) { return s <= 0 ? "Fahrer" : s == 1 ? "Mitte" : "Beifahrer"; }
}

//==============================================================================
class DeviceSim
{
public:
    struct Settings
    {
        int device = 0;      // 0 = Original
        bool on = true;
        int volume = 1;      // 0 leise, 1 normal, 2 laut
        int noise = 0;       // 0 aus, 1 leise, 2 laut
        int seat = 0;        // Auto: 0 Fahrer, 1 Mitte, 2 Beifahrer
        bool match = true;   // Pegel angleichen

        bool sameSound (const Settings& o) const noexcept
        {
            return device == o.device && volume == o.volume && noise == o.noise && seat == o.seat && match == o.match;
        }
    };

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0 ? sampleRate : 48000.0;
        early.assign ((size_t) (2 * bufLen()), 0.0f);
        for (auto& d : fdnBuf) d.assign ((size_t) bufLen(), 0.0f);
        seatBuf.assign (2 * 256, 0.0f);
        calibrate();
        configure (wanted);
        active = wanted;
        mix = (wanted.on && wanted.device > 0) ? 1.0f : 0.0f;
        env = 1.0f;
        pending = false;
        prepared = true;
    }

    void setSettings (const Settings& s) noexcept { wanted = s; }

    // Stereo-Verarbeitung an Ort und Stelle. L und R müssen verschiedene Puffer sein.
    void process (float* L, float* R, int n) noexcept
    {
        if (! prepared) return;
        const bool wantOn = wanted.on && wanted.device > 0;

        if (! wanted.sameSound (active))
        {
            if (mix <= 0.0f) { configure (wanted); active = wanted; }
            else if (! pending) pending = true;
        }
        active.on = wanted.on;

        if (! wantOn && mix <= 0.0f && ! pending)
        {
            grDb.store (0.0f);
            clipping.store (false);
            return;
        }

        const float step = (float) (1.0 / (0.015 * sr));
        float maxGr = 0.0f;
        int clips = 0;

        for (int i = 0; i < n; ++i)
        {
            if (pending)
            {
                env -= step;
                if (env <= 0.0f)
                {
                    env = 0.0f;
                    configure (wanted);
                    active = wanted;
                    pending = false;
                }
            }
            else if (env < 1.0f) env = std::min (1.0f, env + step);

            const float mixTarget = (wanted.on && active.device > 0) ? 1.0f : 0.0f;
            if (mix < mixTarget) mix = std::min (mixTarget, mix + step);
            else if (mix > mixTarget) mix = std::max (mixTarget, mix - step);

            const float dl = L[i], dr = R[i];
            if (active.device <= 0 || mix <= 0.0f)
                continue;

            float wl, wr;
            processSample (dl, dr, wl, wr, maxGr, clips);
            const float g = mix * env;
            L[i] = dl * (1.0f - mix) + wl * g;
            R[i] = dr * (1.0f - mix) + wr * g;
        }

        grDb.store (maxGr);
        clipping.store (clips > n / 200);
    }

    bool isActive() const noexcept { return wanted.on && wanted.device > 0; }
    float getLimiterDb() const noexcept { return grDb.load(); }
    bool isClipping() const noexcept { return clipping.load(); }
    float getTrimDb (int device) const noexcept
    {
        return device >= 0 && device < (int) trims.size() ? trims[(size_t) device] : 0.0f;
    }

    // Frequenzgang für die Anzeige (Filter + Leise-Kontur + Angleich), in dB
    static float responseDb (int device, double f, int volume, float trimDb)
    {
        const auto& ms = devsim::models();
        if (device <= 0 || device >= (int) ms.size()) return 0.0f;
        const double rate = 48000.0;
        double db = trimDb;
        for (auto& b : devsim::makeChain (ms[(size_t) device], rate, volume))
            db += b.magDb (f, rate);
        return (float) db;
    }

private:
    int bufLen() const { int l = 1; while (l < (int) (0.12 * sr) + 8) l <<= 1; return l; }

    struct Tap { float ms, gl, gr; };
    struct RoomDef { std::vector<Tap> taps; float rt, scale, wetDb, dampHz, preMs; };

    static const RoomDef& roomDef (int room)
    {
        static const RoomDef none { {}, 0, 0, -200, 8000, 0 };
        static const RoomDef rooms[] = {
            none,
            { { { 0.6f, -9, -9 }, { 1.4f, -14, -16 }, { 3.1f, -18, -18 } }, 0, 0, -200, 9000, 0 },                       // Schreibtisch
            { { { 2.3f, -8, -10 }, { 4.1f, -10, -8 }, { 6.7f, -12, -13 }, { 9.8f, -14, -12 }, { 14.2f, -16, -17 },
                { 19.5f, -19, -18 } }, 0.42f, 0.45f, -13, 6500, 6 },                                                        // Küche (hart)
            { { { 3.5f, -11, -13 }, { 6.2f, -13, -11 }, { 9.9f, -15, -15 }, { 14.8f, -17, -17 } }, 0.5f, 0.8f, -17, 4500, 9 }, // Wohnzimmer
            { { { 0.9f, -6, -9 }, { 1.7f, -9, -7 }, { 2.6f, -10, -11 }, { 3.4f, -12, -10 }, { 4.6f, -14, -15 },
                { 6.1f, -17, -16 } }, 0.12f, 0.25f, -16, 3500, 2 },                                                         // Auto
            { { { 11.0f, -10, -12 }, { 17.0f, -12, -10 }, { 26.0f, -14, -15 }, { 38.0f, -16, -15 } }, 1.4f, 1.7f, -8, 3000, 15 }, // Club
            { { { 1.9f, -15, -17 }, { 3.8f, -17, -15 }, { 5.5f, -20, -20 } }, 0.25f, 0.4f, -22, 8000, 3 },                 // kleiner Regieraum
        };
        return room >= 0 && room < 7 ? rooms[room] : none;
    }

    static float db2g (float db) { return db <= -150.0f ? 0.0f : std::pow (10.0f, db / 20.0f); }

    void configure (const Settings& s)
    {
        const auto& ms = devsim::models();
        dev = juce_clamp (s.device, 0, (int) ms.size() - 1);
        const auto& m = ms[(size_t) dev];

        chain = devsim::makeChain (m, sr, s.volume);
        for (auto& st : stL) st.reset();
        for (auto& st : stR) st.reset();

        width = m.headphones ? 1.0f : m.width;
        pre = db2g (devsim::volumeDb (s.volume));
        post = 1.0f / pre;
        useLimiter = m.limitDb < 0.0f;
        limThr = db2g (m.limitDb);
        limG = 1.0f;
        limAtk = (float) (1.0 - std::exp (-1.0 / (0.0008 * sr)));
        limRel = (float) (1.0 - std::exp (-1.0 / (0.12 * sr)));
        drive = m.driveDb > 0.0f ? db2g (m.driveDb) : 0.0f;

        // Sitzplatz im Auto: näherer Lautsprecher lauter und früher
        seatGL = seatGR = 1.0f;
        seatDL = seatDR = 0;
        if (m.car && s.seat != 1)
        {
            const int d = (int) std::lround (0.0012 * sr);
            const float nearG = db2g (2.0f), farG = db2g (-1.5f);
            if (s.seat == 0) { seatGL = nearG; seatGR = farG; seatDR = std::min (d, 250); }
            else             { seatGR = nearG; seatGL = farG; seatDL = std::min (d, 250); }
        }
        std::fill (seatBuf.begin(), seatBuf.end(), 0.0f);
        seatPos = 0;

        // Raum
        const auto& rd = roomDef (m.room);
        taps.clear();
        for (auto& t : rd.taps)
            taps.push_back ({ (int) std::lround (t.ms * 0.001 * sr), db2g (t.gl), db2g (t.gr) });
        tapLp = (float) (1.0 - std::exp (-2.0 * 3.14159265 * rd.dampHz / sr));
        tapZL = tapZR = 0.0f;
        std::fill (early.begin(), early.end(), 0.0f);
        ePos = 0;

        useFdn = rd.rt > 0.0f;
        fdnWet = db2g (rd.wetDb);
        fdnPre = (int) std::lround (rd.preMs * 0.001 * sr);
        static const float baseMs[4] = { 23.1f, 29.3f, 35.9f, 41.7f };
        for (size_t k = 0; k < 4; ++k)
        {
            fdnLen[k] = juce_clamp ((int) std::lround (baseMs[k] * rd.scale * 0.001 * sr), 8, bufLen() - 1);
            fdnG[k] = rd.rt > 0.0f ? (float) std::pow (10.0, -3.0 * fdnLen[k] / (rd.rt * sr)) : 0.0f;
            std::fill (fdnBuf[k].begin(), fdnBuf[k].end(), 0.0f);
            fdnZ[k] = 0.0f;
        }
        fdnPos = 0;
        fdnLp = (float) (1.0 - std::exp (-2.0 * 3.14159265 * rd.dampHz / sr));

        trim = s.match ? db2g (dev < (int) trims.size() ? trims[(size_t) dev] : 0.0f) : 1.0f;

        // Umgebungsgeräusch
        noiseType = m.noise;
        const float lvl = devsim::noiseLevelDb (m.noise, s.noise);
        const float volShift = s.volume == 0 ? 8.0f : s.volume == 2 ? -6.0f : 0.0f;   // leise hören = Geräusch relativ lauter
        noiseGain = lvl > -150.0f ? db2g (lvl + volShift) * noiseNorm[(size_t) juce_clamp (noiseType, 0, devsim::numNoise - 1)] : 0.0f;
        noiseChain = makeNoiseChain (noiseType, sr);
        for (auto& st : nzL) st.reset();
        for (auto& st : nzR) st.reset();
    }

    static int juce_clamp (int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

    static std::vector<DevBq> makeNoiseChain (int type, double rate)
    {
        using B = DevBq;
        std::vector<DevBq> c;
        auto add = [&] (int t, double f, double q, double g) { c.push_back (B::make (t, f, q, g, rate)); };
        switch (type)
        {
            case devsim::nzStreet:  add (B::HP, 100, 0.7, 0); add (B::LP, 4000, 0.7, 0); add (B::Peak, 400, 1.0, 3); break;
            case devsim::nzKitchen: add (B::HP, 120, 0.7, 0); add (B::LP, 1800, 0.7, 0); add (B::Peak, 300, 1.5, 4); break;
            case devsim::nzCar:     add (B::HP, 25, 0.7, 0); add (B::LP, 260, 0.6, 0); add (B::LP, 900, 0.7, 0); add (B::Peak, 70, 1.2, 6); break;
            case devsim::nzCrowd:   add (B::HP, 280, 0.7, 0); add (B::LP, 3200, 0.7, 0); add (B::Peak, 800, 0.8, 4); break;
            case devsim::nzLiving:  add (B::HP, 60, 0.7, 0); add (B::LP, 1000, 0.7, 0); break;
            case devsim::nzOffice:  add (B::HP, 150, 0.7, 0); add (B::LP, 5000, 0.7, 0); add (B::Peak, 1200, 2.0, 3); break;
            default: break;
        }
        return c;
    }

    inline float white() noexcept
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float) ((double) rng / 4294967295.0 * 2.0 - 1.0);
    }

    struct Pink
    {
        float b0 = 0, b1 = 0, b2 = 0;
        inline float next (float w) noexcept
        {
            b0 = 0.99765f * b0 + w * 0.0990460f;
            b1 = 0.96300f * b1 + w * 0.2965164f;
            b2 = 0.57000f * b2 + w * 1.0526913f;
            return (b0 + b1 + b2 + w * 0.1848f) * 0.25f;
        }
    };

    inline void noiseSample (float& nl, float& nr) noexcept
    {
        double l = pinkL.next (white()), r = pinkR.next (white());
        for (size_t k = 0; k < noiseChain.size(); ++k)
        {
            l = nzL[k].process (noiseChain[k], l);
            r = nzR[k].process (noiseChain[k], r);
        }
        float g = noiseGain;
        if (noiseType == devsim::nzCrowd)   // Publikum: lebendige Lautstärke
        {
            if (--crowdCount <= 0)
            {
                crowdCount = (int) (0.18 * sr);
                crowdTarget = 0.45f + 0.9f * (0.5f + 0.5f * white());
            }
            crowdEnv += (crowdTarget - crowdEnv) * (float) (1.0 / (0.08 * sr));
            g *= crowdEnv;
        }
        nl = (float) l * g;
        nr = (float) r * g;
    }

    inline void processSample (float inL, float inR, float& outL, float& outR, float& maxGr, int& clips) noexcept
    {
        double l = inL, r = inR;
        if (width < 1.0f)
        {
            const double m = 0.5 * (l + r), s = 0.5 * (l - r) * width;
            l = m + s; r = m - s;
        }
        l *= pre; r *= pre;
        for (size_t k = 0; k < chain.size(); ++k)
        {
            l = stL[k].process (chain[k], l);
            r = stR[k].process (chain[k], r);
        }

        if (useLimiter && ! calibrating)
        {
            const float pk = (float) std::max (std::abs (l), std::abs (r));
            const float tgt = pk > limThr ? limThr / pk : 1.0f;
            limG += (tgt - limG) * (tgt < limG ? limAtk : limRel);
            l *= limG; r *= limG;
            maxGr = std::max (maxGr, -20.0f * std::log10 (std::max (1.0e-6f, limG)));
        }
        if (drive > 0.0f && ! calibrating)
        {
            const double xl = l * drive, xr = r * drive;
            if (std::abs (xl) > 0.9 || std::abs (xr) > 0.9) ++clips;
            l = std::tanh (xl) / drive;
            r = std::tanh (xr) / drive;
        }
        l *= post; r *= post;

        // Sitzplatz (Auto)
        if (seatDL > 0 || seatDR > 0)
        {
            seatBuf[(size_t) seatPos] = (float) l;
            seatBuf[(size_t) (256 + seatPos)] = (float) r;
            l = seatBuf[(size_t) ((seatPos - seatDL) & 255)];
            r = seatBuf[(size_t) (256 + ((seatPos - seatDR) & 255))];
            seatPos = (seatPos + 1) & 255;
        }
        l *= seatGL; r *= seatGR;

        // Raum: frühe Reflexionen + Nachhall
        if (! taps.empty() || useFdn)
        {
            const int len = bufLen(), mask = len - 1;
            early[(size_t) ePos] = (float) l;
            early[(size_t) (len + ePos)] = (float) r;
            float el = 0.0f, er = 0.0f;
            for (auto& t : taps)
            {
                const int p = (ePos - t.d) & mask;
                // gekreuzt: Reflexionen kommen auch von der anderen Seite
                el += t.gl * (0.7f * early[(size_t) p] + 0.3f * early[(size_t) (len + p)]);
                er += t.gr * (0.7f * early[(size_t) (len + p)] + 0.3f * early[(size_t) p]);
            }
            tapZL += (el - tapZL) * tapLp;
            tapZR += (er - tapZR) * tapLp;

            float rl = 0.0f, rr = 0.0f;
            if (useFdn)
            {
                const int pp = (ePos - fdnPre) & mask;
                const float in = 0.5f * (early[(size_t) pp] + early[(size_t) (len + pp)]);
                float o[4];
                for (size_t k = 0; k < 4; ++k)
                {
                    const int rp = (fdnPos - fdnLen[k]) & mask;
                    fdnZ[k] += (fdnBuf[(size_t) k][(size_t) rp] - fdnZ[k]) * fdnLp;
                    o[k] = fdnZ[k];
                }
                const float sum = 0.5f * (o[0] + o[1] + o[2] + o[3]);   // Householder 4×4
                for (size_t k = 0; k < 4; ++k)
                    fdnBuf[(size_t) k][(size_t) fdnPos] = fdnG[k] * (o[k] - sum) + in * (k % 2 == 0 ? 0.5f : -0.5f);
                fdnPos = (fdnPos + 1) & mask;
                rl = (o[0] + o[2]) * fdnWet;
                rr = (o[1] - o[3]) * fdnWet;
            }
            ePos = (ePos + 1) & mask;
            l += tapZL + rl;
            r += tapZR + rr;
        }

        l *= trim; r *= trim;

        if (noiseGain > 0.0f && ! calibrating)
        {
            float nl, nr;
            noiseSample (nl, nr);
            l += nl; r += nr;
        }

        outL = std::isfinite (l) ? (float) l : 0.0f;
        outR = std::isfinite (r) ? (float) r : 0.0f;
    }

    // Pegel-Angleich je Gerät (K-gewichtete Leistung, rosa Rauschen, linear) + Geräusch-Normierung
    void calibrate()
    {
        const auto& ms = devsim::models();
        trims.assign (ms.size(), 0.0f);
        const DevBq kHp = DevBq::make (DevBq::HP, 38.0, 0.5, 0, sr);
        const DevBq kHs = DevBq::make (DevBq::HighShelf, 1681.0, 0.7071, 4.0, sr);
        const int warm = (int) (0.15 * sr), len = (int) (0.6 * sr);

        calibrating = true;
        for (int d = 1; d < (int) ms.size(); ++d)
        {
            Settings s; s.device = d; s.match = false; s.noise = 0; s.seat = 1; s.volume = 1;
            configure (s);
            rng = 0x12345678u;
            Pink p1, p2;
            DevBqState ki[4], ko[4];
            double ein = 0, eout = 0;
            float gr = 0; int cl = 0;
            for (int i = 0; i < warm + len; ++i)
            {
                const float a = p1.next (white()), b = p2.next (white());
                const float xl = 0.3f * a, xr = 0.3f * (0.6f * a + 0.8f * b);
                float yl, yr;
                processSample (xl, xr, yl, yr, gr, cl);
                if (i < warm) continue;
                const double il = ki[1].process (kHs, ki[0].process (kHp, xl)), ir = ki[3].process (kHs, ki[2].process (kHp, xr));
                const double ol = ko[1].process (kHs, ko[0].process (kHp, yl)), orr = ko[3].process (kHs, ko[2].process (kHp, yr));
                ein += il * il + ir * ir;
                eout += ol * ol + orr * orr;
            }
            trims[(size_t) d] = eout > 1.0e-20 ? (float) (10.0 * std::log10 (ein / eout)) : 0.0f;
            trims[(size_t) d] = std::max (-24.0f, std::min (24.0f, trims[(size_t) d]));
        }
        calibrating = false;

        noiseNorm.fill (1.0f);
        for (int t = 1; t < devsim::numNoise; ++t)
        {
            auto c = makeNoiseChain (t, sr);
            Pink p; DevBqState st[6];
            double e = 0;
            rng = 0x9e3779b9u;
            for (int i = 0; i < warm + len; ++i)
            {
                double v = p.next (white());
                for (size_t k = 0; k < c.size(); ++k) v = st[k].process (c[k], v);
                if (i >= warm) e += v * v;
            }
            const double rms = std::sqrt (e / len);
            noiseNorm[(size_t) t] = rms > 1.0e-9 ? (float) (1.0 / rms) : 1.0f;
        }
        rng = 0x2545f491u;
    }

    struct TapI { int d; float gl, gr; };

    double sr = 48000.0;
    bool prepared = false, calibrating = false, pending = false;
    Settings wanted, active;
    float mix = 0.0f, env = 1.0f;

    int dev = 0;
    std::vector<DevBq> chain;
    std::array<DevBqState, 16> stL {}, stR {};
    float width = 1.0f, pre = 1.0f, post = 1.0f, trim = 1.0f, drive = 0.0f;
    bool useLimiter = false;
    float limThr = 1.0f, limG = 1.0f, limAtk = 0.1f, limRel = 0.001f;

    float seatGL = 1, seatGR = 1;
    int seatDL = 0, seatDR = 0, seatPos = 0;
    std::vector<float> seatBuf;

    std::vector<TapI> taps;
    std::vector<float> early;
    int ePos = 0;
    float tapLp = 1.0f, tapZL = 0, tapZR = 0;

    bool useFdn = false;
    std::array<std::vector<float>, 4> fdnBuf;
    std::array<int, 4> fdnLen {};
    std::array<float, 4> fdnG {}, fdnZ {};
    int fdnPos = 0, fdnPre = 0;
    float fdnWet = 0, fdnLp = 1.0f;

    int noiseType = 0;
    float noiseGain = 0.0f;
    std::vector<DevBq> noiseChain;
    std::array<DevBqState, 6> nzL {}, nzR {};
    Pink pinkL, pinkR;
    uint32_t rng = 0x2545f491u;
    float crowdEnv = 1.0f, crowdTarget = 1.0f;
    int crowdCount = 0;

    std::vector<float> trims;
    std::array<float, devsim::numNoise> noiseNorm {};

    std::atomic<float> grDb { 0.0f };
    std::atomic<bool> clipping { false };
};

} // namespace k54a

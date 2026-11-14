#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>

// ============================================================================
//  K-54 D · Drum-Engine
//  12 Spuren, jede mit mehreren synthetischen Modellen (808, 909, Linn-Stil,
//  akustisch, Jazz/Besen, Trap, Lo-Fi ...) oder einem eigenen Sample.
//  Alles wird aus Schaltungs-aehnlichen Bausteinen erzeugt:
//  Koerper (Oszillator mit Pitch-Huellkurve), Rauschen (Filter), Klick,
//  Metall (6 Rechtecke wie in der 808), Clap-Stoesse, Glocke, Saettigung.
// ============================================================================

namespace DrumInfo
{
    inline constexpr int numLanes = 12;
    inline constexpr int maxSteps = 32;
    inline constexpr int numPatterns = 4;

    inline const char* names[numLanes]      { "Kick", "Snare", "Clap", "Rim", "Closed Hat", "Open Hat",
                                              "Tom Low", "Tom Mid", "Tom High", "Crash", "Ride", "Perc" };
    inline const char* shortNames[numLanes] { "KICK", "SNARE", "CLAP", "RIM", "CL HAT", "OP HAT",
                                              "TOM L", "TOM M", "TOM H", "CRASH", "RIDE", "PERC" };
    // Buchstaben fuer die Preset-Schreibweise
    inline constexpr char codes[numLanes]   { 'K', 'S', 'C', 'R', 'H', 'O', 'L', 'M', 'T', 'Y', 'D', 'P' };

    // General MIDI (Kanal egal): Note -> Spur
    inline int laneForNote (int note)
    {
        switch (note)
        {
            case 35: case 36:          return 0;
            case 38: case 40:          return 1;
            case 39:                   return 2;
            case 37:                   return 3;
            case 42: case 44:          return 4;
            case 46:                   return 5;
            case 41: case 43:          return 6;
            case 45: case 47:          return 7;
            case 48: case 50:          return 8;
            case 49: case 52: case 55: case 57: return 9;
            case 51: case 53: case 59: return 10;
            case 54: case 56: case 58: case 60: case 61: case 62: case 63: case 64:
            case 69: case 70: case 75: case 76: case 77: case 81: return 11;
            default:                   return -1;
        }
    }
    inline constexpr int laneNote[numLanes] { 36, 38, 39, 37, 42, 46, 41, 45, 48, 49, 51, 56 };

    inline juce::StringArray models (int lane)
    {
        switch (lane)
        {
            case 0:  return { "808 Boom", "808 Short", "909", "Linn", "Disco", "Acoustic", "Jazz", "Trap Sub", "Techno", "Lo-Fi", "Sample" };
            case 1:  return { "808", "909", "Linn", "Acoustic", "Brush", "Disco", "Trap", "Lo-Fi", "Gated 80s", "Sample" };
            case 2:  return { "808", "909", "Big", "Snap", "Sample" };
            case 3:  return { "808 Rim", "909 Rim", "Sidestick", "Clave", "Sample" };
            case 4:  return { "808", "909", "Linn", "Acoustic", "Trap", "Sample" };
            case 5:  return { "808", "909", "Linn", "Acoustic", "Sample" };
            case 6: case 7: case 8:
                     return { "808", "909", "Electronic", "Acoustic", "Sample" };
            case 9:  return { "909", "Acoustic", "China", "Sample" };
            case 10: return { "909", "Jazz", "Acoustic", "Bell", "Sample" };
            case 11: return { "Cowbell", "Shaker", "Tambourine", "Conga", "Bongo", "Woodblock", "Triangle", "Sample" };
            default: return { "Sample" };
        }
    }
    inline int sampleModel (int lane) { return models (lane).size() - 1; }
}

/** Einstellungen einer Spur (aus den Parametern, pro Block gelesen) */
struct LaneParams
{
    int   model = 0;
    float level = 0.8f, tune = 0.0f, decay = 0.5f, tone = 0.0f, snap = 0.5f, drive = 0.0f, pan = 0.0f, room = 0.0f;
    bool  mute = false;
};

/** Rezept eines Modells: welche Bausteine wie klingen */
struct Recipe
{
    // Koerper
    float body = 0.0f, f0 = 50.0f, pAmt = 0.0f, pT = 0.03f, bDec = 0.3f, bHold = 0.0f, glide = 0.0f;
    int   wave = 0;                       // 0 Sinus, 1 Dreieck, 2 Sinus + Oberton
    float f2ratio = 0.0f, b2 = 0.0f;      // zweiter Teilton
    // Rauschen
    float noise = 0.0f; int nType = 1;    // 0 LP, 1 BP, 2 HP
    float nF = 3000.0f, nQ = 0.7f, nDec = 0.15f, nAtk = 0.0f, nHP = 0.0f, nHold = 0.0f, nLP = 0.0f;
    // Klick
    float click = 0.0f, cDec = 0.002f, cF = 2000.0f;
    // Metall (6 Rechtecke)
    float metal = 0.0f; int mSet = 0;
    float mMul = 1.0f, mBP = 8000.0f, mHP = 7000.0f, mDec = 0.1f, mAtk = 0.0f;
    // Clap-Stoesse
    int   bursts = 0; float bSpace = 0.011f, bLen = 0.0045f;
    // Glocke (zwei Toene)
    float bell = 0.0f, bellF1 = 0.0f, bellF2 = 0.0f, bellDec = 0.3f; bool bellSquare = false;
    // Charakter
    float drive = 0.0f, gain = 1.0f, crush = 0.0f;
    bool  mono = true;
};

inline Recipe makeRecipe (int lane, int model)
{
    Recipe r;
    switch (lane)
    {
        case 0:   // ------------------------------------------------ KICK
            r.body = 1.0f; r.click = 0.25f;
            switch (model)
            {
                case 0: r.f0 = 46; r.pAmt = 2.2f; r.pT = 0.035f; r.bDec = 0.75f; r.bHold = 0.04f; r.glide = 0.08f; r.click = 0.12f; r.drive = 0.7f; r.gain = 1.05f; break; // 808 Boom
                case 1: r.f0 = 52; r.pAmt = 2.5f; r.pT = 0.03f;  r.bDec = 0.22f; r.click = 0.18f; r.drive = 0.4f; break;                                          // 808 Short
                case 2: r.f0 = 50; r.pAmt = 6.0f; r.pT = 0.018f; r.bDec = 0.32f; r.wave = 1; r.click = 0.55f; r.cF = 3500; r.noise = 0.18f; r.nType = 2; r.nF = 4000; r.nDec = 0.012f; r.drive = 1.4f; break; // 909
                case 3: r.f0 = 60; r.pAmt = 3.0f; r.pT = 0.012f; r.bDec = 0.16f; r.click = 0.5f; r.cF = 1500; r.noise = 0.12f; r.nType = 0; r.nF = 1200; r.nDec = 0.02f; r.drive = 0.6f; break;      // Linn
                case 4: r.f0 = 55; r.pAmt = 4.0f; r.pT = 0.02f;  r.bDec = 0.28f; r.wave = 2; r.click = 0.45f; r.cF = 2500; r.drive = 1.1f; break;                    // Disco
                case 5: r.f0 = 58; r.pAmt = 1.6f; r.pT = 0.015f; r.bDec = 0.24f; r.wave = 2; r.click = 0.6f; r.cF = 900; r.noise = 0.35f; r.nType = 0; r.nF = 700; r.nQ = 0.5f; r.nDec = 0.06f; r.drive = 0.3f; break; // Acoustic
                case 6: r.f0 = 62; r.pAmt = 1.0f; r.pT = 0.02f;  r.bDec = 0.20f; r.click = 0.3f; r.cF = 800; r.noise = 0.25f; r.nType = 0; r.nF = 500; r.nDec = 0.08f; r.drive = 0.1f; r.gain = 0.8f; break; // Jazz
                case 7: r.f0 = 40; r.pAmt = 1.6f; r.pT = 0.05f;  r.bDec = 1.1f;  r.bHold = 0.1f; r.glide = 0.12f; r.click = 0.2f; r.drive = 1.6f; r.gain = 1.0f; break;           // Trap Sub
                case 8: r.f0 = 48; r.pAmt = 7.0f; r.pT = 0.014f; r.bDec = 0.38f; r.wave = 1; r.click = 0.7f; r.cF = 4000; r.drive = 2.4f; r.gain = 0.95f; break;     // Techno
                case 9: r.f0 = 54; r.pAmt = 2.5f; r.pT = 0.025f; r.bDec = 0.26f; r.click = 0.35f; r.cF = 1200; r.drive = 0.9f; r.crush = 0.6f; break;                // Lo-Fi
                default: break;
            }
            break;

        case 1:   // ------------------------------------------------ SNARE
            r.body = 0.7f; r.f0 = 185; r.f2ratio = 1.65f; r.b2 = 0.55f; r.pAmt = 0.3f; r.pT = 0.012f; r.bDec = 0.08f;
            r.noise = 0.9f; r.nType = 1; r.nF = 5000; r.nQ = 0.6f; r.nDec = 0.16f; r.nHP = 1200; r.nLP = 8500;
            switch (model)
            {
                case 0: r.f0 = 175; r.b2 = 0.6f; r.bDec = 0.09f; r.noise = 0.7f; r.nF = 6500; r.nDec = 0.13f; r.click = 0.15f; r.nLP = 7000; break;               // 808
                case 1: r.f0 = 200; r.pAmt = 0.8f; r.bDec = 0.07f; r.noise = 1.1f; r.nType = 2; r.nF = 3500; r.nDec = 0.2f; r.click = 0.35f; r.cF = 3000; r.drive = 0.6f; break; // 909
                case 2: r.f0 = 190; r.bDec = 0.1f; r.noise = 1.0f; r.nF = 4200; r.nQ = 0.8f; r.nDec = 0.22f; r.click = 0.4f; r.drive = 0.5f; break;                // Linn
                case 3: r.f0 = 200; r.f2ratio = 1.5f; r.b2 = 0.4f; r.bDec = 0.11f; r.noise = 1.0f; r.nF = 4500; r.nQ = 0.5f; r.nDec = 0.24f; r.click = 0.6f; r.cF = 1800; r.drive = 0.3f; r.nLP = 10000; break; // Acoustic
                case 4: r.body = 0.08f; r.noise = 0.85f; r.nType = 0; r.nF = 7000; r.nQ = 0.5f; r.nAtk = 0.03f; r.nDec = 0.20f; r.nHP = 600; r.nLP = 0.0f; r.click = 0.0f; r.gain = 1.5f; r.mono = false; break; // Brush
                case 5: r.f0 = 210; r.bDec = 0.08f; r.noise = 1.0f; r.nF = 5500; r.nDec = 0.15f; r.click = 0.4f; r.drive = 0.4f; r.gain = 0.95f; break;            // Disco
                case 6: r.f0 = 230; r.pAmt = 0.6f; r.bDec = 0.06f; r.noise = 1.1f; r.nType = 2; r.nF = 2800; r.nDec = 0.14f; r.click = 0.5f; r.cF = 4000; r.drive = 1.2f; r.nLP = 10500; break; // Trap
                case 7: r.f0 = 180; r.bDec = 0.09f; r.noise = 0.8f; r.nF = 3800; r.nDec = 0.15f; r.click = 0.3f; r.drive = 0.8f; r.crush = 0.7f; r.nLP = 5500; break; // Lo-Fi
                case 8: r.f0 = 190; r.bDec = 0.12f; r.noise = 1.2f; r.nF = 4000; r.nDec = 0.10f; r.nHold = 0.16f; r.click = 0.5f; r.drive = 0.9f; r.gain = 1.0f; break; // Gated 80s
                default: break;
            }
            break;

        case 2:   // ------------------------------------------------ CLAP
            r.noise = 1.0f; r.nType = 1; r.nF = 1150; r.nQ = 1.6f; r.bursts = 3; r.bSpace = 0.011f; r.bLen = 0.0045f; r.nDec = 0.14f; r.nHP = 600; r.nLP = 9000;
            switch (model)
            {
                case 0: r.nF = 1100; r.nDec = 0.16f; r.gain = 1.3f; break;                               // 808
                case 1: r.nF = 1400; r.nQ = 1.2f; r.bSpace = 0.009f; r.nDec = 0.11f; r.drive = 0.4f; r.gain = 1.2f; break; // 909
                case 2: r.nF = 1300; r.nQ = 0.9f; r.bursts = 4; r.bSpace = 0.013f; r.nDec = 0.28f; r.drive = 0.6f; r.gain = 1.1f; break; // Big
                case 3: r.nF = 2600; r.nQ = 2.2f; r.bursts = 1; r.nDec = 0.035f; r.body = 0.3f; r.f0 = 1800; r.bDec = 0.01f; r.gain = 1.1f; break; // Snap
                default: break;
            }
            break;

        case 3:   // ------------------------------------------------ RIM
            r.body = 1.0f; r.wave = 0; r.bDec = 0.025f; r.click = 0.5f; r.cF = 3000; r.mono = false;
            switch (model)
            {
                case 0: r.f0 = 1650; r.f2ratio = 0.33f; r.b2 = 0.6f; r.bDec = 0.02f; r.noise = 0.2f; r.nType = 2; r.nF = 5000; r.nDec = 0.006f; r.drive = 0.8f; break; // 808 Rim
                case 1: r.f0 = 1900; r.f2ratio = 0.55f; r.b2 = 0.5f; r.bDec = 0.018f; r.noise = 0.3f; r.nType = 2; r.nF = 4000; r.nDec = 0.01f; r.drive = 0.6f; break;
                case 2: r.f0 = 420; r.f2ratio = 2.3f; r.b2 = 0.5f; r.bDec = 0.04f; r.noise = 0.5f; r.nType = 1; r.nF = 2200; r.nQ = 1.0f; r.nDec = 0.03f; r.click = 0.8f; r.gain = 0.7f; break; // Sidestick
                case 3: r.f0 = 2450; r.bDec = 0.05f; r.click = 0.2f; r.gain = 0.8f; break;                                          // Clave
                default: break;
            }
            break;

        case 4: case 5:   // ----------------------------------------- HATS
        {
            const bool open = lane == 5;
            r.metal = 1.0f; r.mSet = 0; r.mBP = 9000; r.mHP = 7000; r.noise = 0.25f; r.nType = 2; r.nF = 8000;
            r.mDec = open ? 0.35f : 0.045f; r.nDec = r.mDec;
            r.gain = 1.0f;
            switch (model)
            {
                case 0: break;                                                                                                   // 808
                case 1: r.metal = 0.6f; r.noise = 0.6f; r.mBP = 10000; r.mHP = 8000; r.nF = 9000; r.mMul = 1.2f; r.mDec = open ? 0.42f : 0.05f; r.nDec = r.mDec; break; // 909
                case 2: r.metal = 0.5f; r.noise = 0.7f; r.mSet = 1; r.mBP = 8000; r.mHP = 6500; r.mDec = open ? 0.3f : 0.04f; r.nDec = r.mDec; r.crush = 0.25f; break;       // Linn
                case 3: r.metal = 0.45f; r.mSet = 1; r.noise = 0.8f; r.nType = 1; r.nF = 9000; r.nQ = 0.4f; r.mBP = 7000; r.mHP = 5500;
                        r.mDec = open ? 0.55f : 0.06f; r.nDec = r.mDec; r.click = 0.15f; r.cF = 6000; r.mAtk = 0.002f; break;     // Acoustic
                case 4: r.metal = 0.9f; r.noise = 0.3f; r.mBP = 11000; r.mHP = 8500; r.mDec = 0.03f; r.nDec = 0.03f; r.gain = 1.0f; break;     // Trap (kurz, hell)
                default: break;
            }
            break;
        }

        case 6: case 7: case 8:   // ---------------------------------- TOMS
        {
            const float pitchMul = lane == 6 ? 1.0f : (lane == 7 ? 1.35f : 1.8f);
            r.body = 1.0f; r.click = 0.2f; r.mono = false; r.gain = 0.68f;
            switch (model)
            {
                case 0: r.f0 = 85 * pitchMul;  r.pAmt = 0.5f; r.pT = 0.05f; r.bDec = 0.45f; r.noise = 0.1f; r.nType = 0; r.nF = 1500; r.nDec = 0.02f; r.drive = 0.5f; break; // 808
                case 1: r.f0 = 95 * pitchMul;  r.pAmt = 0.8f; r.pT = 0.035f; r.bDec = 0.32f; r.wave = 2; r.noise = 0.25f; r.nType = 1; r.nF = 1800; r.nDec = 0.04f; r.drive = 0.7f; break; // 909
                case 2: r.f0 = 110 * pitchMul; r.pAmt = 1.6f; r.pT = 0.12f; r.bDec = 0.35f; r.wave = 1; r.noise = 0.15f; r.nType = 0; r.nF = 3000; r.nDec = 0.05f; r.drive = 0.6f; break; // Electronic
                case 3: r.f0 = 92 * pitchMul;  r.pAmt = 0.25f; r.pT = 0.03f; r.bDec = 0.38f; r.f2ratio = 1.52f; r.b2 = 0.35f; r.noise = 0.35f; r.nType = 1; r.nF = 900; r.nQ = 0.6f; r.nDec = 0.06f; r.click = 0.5f; r.cF = 1500; r.gain = 0.6f; break; // Acoustic
                default: break;
            }
            break;
        }

        case 9:   // ------------------------------------------------ CRASH
            r.metal = 0.8f; r.mSet = 2; r.noise = 0.6f; r.nType = 2; r.nF = 5000; r.mBP = 7000; r.mHP = 4500;
            r.mDec = 1.4f; r.nDec = 1.2f; r.mAtk = 0.003f; r.gain = 0.75f; r.mono = false;
            switch (model)
            {
                case 0: r.mMul = 1.1f; break;                                                          // 909
                case 1: r.mSet = 3; r.metal = 0.7f; r.noise = 0.7f; r.nType = 1; r.nF = 6000; r.nQ = 0.3f; r.mDec = 1.9f; r.nDec = 1.7f; r.click = 0.2f; r.cF = 3000; r.gain = 1.2f; break; // Acoustic
                case 2: r.mSet = 3; r.metal = 1.0f; r.mBP = 4500; r.mHP = 3000; r.noise = 0.5f; r.nF = 3500; r.mDec = 1.1f; r.nDec = 0.9f; r.drive = 0.8f; break;   // China
                default: break;
            }
            break;

        case 10:  // ------------------------------------------------ RIDE
            r.metal = 0.7f; r.mSet = 3; r.mBP = 6000; r.mHP = 4000; r.mDec = 1.2f; r.mAtk = 0.001f;
            r.noise = 0.2f; r.nType = 2; r.nF = 7000; r.nDec = 0.6f; r.bell = 0.25f; r.bellF1 = 760; r.bellF2 = 1130; r.bellDec = 0.7f;
            r.click = 0.15f; r.cF = 5000; r.gain = 0.85f; r.mono = false;
            switch (model)
            {
                case 0: r.mSet = 2; r.metal = 0.8f; r.bell = 0.15f; r.mDec = 1.0f; break;                               // 909
                case 1: r.metal = 0.6f; r.noise = 0.35f; r.nType = 1; r.nF = 8000; r.nQ = 0.4f; r.mDec = 1.8f; r.nDec = 1.0f; r.bell = 0.12f; r.click = 0.3f; r.gain = 0.9f; break; // Jazz (Wash)
                case 2: break;                                                                                             // Acoustic
                case 3: r.metal = 0.25f; r.bell = 0.9f; r.bellF1 = 820; r.bellF2 = 1960; r.bellDec = 1.0f; r.mDec = 0.6f; r.gain = 0.7f; break; // Bell
                default: break;
            }
            break;

        case 11:  // ------------------------------------------------ PERC
            r.mono = false;
            switch (model)
            {
                case 0: r.bell = 1.0f; r.bellSquare = true; r.bellF1 = 540; r.bellF2 = 800; r.bellDec = 0.16f; r.mBP = 1400; r.gain = 0.9f; r.click = 0.05f; break; // Cowbell
                case 1: r.noise = 1.0f; r.nType = 1; r.nF = 7500; r.nQ = 0.8f; r.nAtk = 0.012f; r.nDec = 0.05f; r.nHP = 3000; r.gain = 0.8f; break;                 // Shaker
                case 2: r.noise = 0.7f; r.nType = 1; r.nF = 9000; r.nQ = 1.2f; r.nDec = 0.12f; r.metal = 0.6f; r.mSet = 1; r.mMul = 1.6f; r.mBP = 9500; r.mHP = 7000; r.mDec = 0.14f; r.bursts = 2; r.bSpace = 0.008f; r.bLen = 0.006f; r.gain = 0.8f; break; // Tambourine
                case 3: r.body = 1.0f; r.f0 = 210; r.pAmt = 0.5f; r.pT = 0.008f; r.bDec = 0.2f; r.click = 0.4f; r.cF = 2500; r.noise = 0.15f; r.nType = 1; r.nF = 1500; r.nDec = 0.015f; break; // Conga
                case 4: r.body = 1.0f; r.f0 = 380; r.pAmt = 0.4f; r.pT = 0.006f; r.bDec = 0.12f; r.click = 0.5f; r.cF = 3000; r.noise = 0.15f; r.nType = 1; r.nF = 2500; r.nDec = 0.012f; r.gain = 0.75f; break; // Bongo
                case 5: r.body = 1.0f; r.f0 = 900; r.f2ratio = 2.7f; r.b2 = 0.3f; r.bDec = 0.06f; r.click = 0.5f; r.cF = 2500; r.gain = 0.8f; break;               // Woodblock
                case 6: r.bell = 1.0f; r.bellF1 = 4500; r.bellF2 = 6750; r.bellDec = 0.9f; r.gain = 0.6f; r.click = 0.05f; r.cF = 6000; break;                      // Triangle
                default: break;
            }
            break;

        default: break;
    }
    return r;
}

// ---------------------------------------------------------------------------
//  Zustandsvariablen-Filter (TPT) – LP/BP/HP
class DSVF
{
public:
    void set (double sr, float fc, float q)
    {
        fc = juce::jlimit (20.0f, (float) (sr * 0.45), fc);
        g = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
        k = 1.0f / juce::jmax (0.1f, q);
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0.0f; }
    void process (float x, float& lp, float& bp, float& hp)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
    float lp (float x) { float l, b, h; process (x, l, b, h); return l; }
    float bp (float x) { float l, b, h; process (x, l, b, h); return b; }
    float hp (float x) { float l, b, h; process (x, l, b, h); return h; }
private:
    float g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;
};

/** geladenes Sample einer Spur (wird zwischen Threads per shared_ptr getauscht) */
struct DrumSample
{
    juce::AudioBuffer<float> audio;   // Stereo
    double sampleRate = 44100.0;
    juce::String name;
};

// ---------------------------------------------------------------------------
class DrumVoice
{
public:
    void prepare (double sampleRate, int seed) { sr = sampleRate; rng.setSeed (seed); active = false; }

    void trigger (int laneIndex, float velocity, const LaneParams& p, std::shared_ptr<const DrumSample> smp)
    {
        lane = laneIndex;
        vel = juce::jlimit (0.02f, 1.0f, velocity);
        prm = p;
        sample = std::move (smp);
        isSample = p.model >= DrumInfo::sampleModel (lane);
        t = 0.0; age = 0; active = true; fade = 1.0f; fadeStep = 0.0f;
        phase1 = phase2 = phB1 = phB2 = 0.0; samplePos = 0.0;
        for (auto& ph : mPhase) ph = rng.nextDouble();
        nf.reset(); nf2.reset(); nf3.reset(); cf.reset(); mf1.reset(); mf2.reset(); tf.reset();
        crushHold = 0.0f; crushCount = 0;

        const float tuneMul = std::exp2 (p.tune / 12.0f);
        const float decMul  = std::exp2 ((p.decay - 0.5f) * 4.0f);         // 0.25x .. 4x
        const float toneMul = std::exp2 (p.tone * 1.3f);                    // dunkler .. heller
        const float snapMul = p.snap * 2.0f;                                // 0 .. 2
        const float velBright = 0.8f + 0.3f * vel;

        if (isSample)
        {
            rate = (sample != nullptr ? sample->sampleRate / sr : 1.0) * tuneMul;
            sampleTau = p.decay >= 0.5f ? 0.0f : 0.01f * std::exp2 (p.decay * 12.0f);
            if (p.tone < -0.02f) tf.set (sr, 20000.0f * std::exp2 (p.tone * 7.0f), 0.7f);
            else if (p.tone > 0.02f) tf.set (sr, 20.0f * std::exp2 (p.tone * 8.0f), 0.7f);
            snapBoost = (snapMul - 1.0f);
            drv = p.drive * 3.0f;
            outGain = 1.0f;
            maxLen = sample != nullptr ? (double) sample->audio.getNumSamples() / juce::jmax (0.01, rate) / sr + 0.01 : 0.0;
            if (sampleTau > 0.0f) maxLen = juce::jmin (maxLen, (double) sampleTau * 8.0 + 0.01);
            return;
        }

        rc = makeRecipe (lane, p.model);
        rc.f0 *= tuneMul;
        rc.bellF1 *= tuneMul; rc.bellF2 *= tuneMul;
        rc.mMul *= tuneMul;
        rc.bDec *= decMul; rc.bHold *= decMul; rc.nDec *= decMul; rc.mDec *= decMul; rc.bellDec *= decMul; rc.nHold *= decMul;
        rc.pAmt *= (1.0f + 0.4f * p.tone);
        rc.click *= snapMul;
        if (lane == 1 || lane == 2) rc.noise *= 0.55f + 0.45f * snapMul;
        if (lane == 1) { rc.noise *= std::sqrt (toneMul); rc.body *= 1.0f / std::sqrt (toneMul); }

        const float nfc = rc.nF * toneMul * velBright * (lane >= 4 ? 1.0f : std::sqrt (tuneMul));
        nf.set (sr, nfc, rc.nQ);
        if (rc.nHP > 0.0f) nf2.set (sr, rc.nHP * std::sqrt (toneMul), 0.7f);
        if (rc.nLP > 0.0f) nf3.set (sr, rc.nLP * toneMul * velBright, 0.6f);
        cf.set (sr, rc.cF * toneMul, 0.7f);
        mf1.set (sr, rc.mBP * toneMul * velBright, 0.9f);
        mf2.set (sr, rc.mHP * toneMul, 0.7f);
        drv = rc.drive + p.drive * 3.0f;
        outGain = rc.gain;

        const float longest = juce::jmax (rc.body > 0 ? rc.bDec + rc.bHold : 0.0f, rc.noise > 0 ? rc.nDec + rc.nHold + rc.nAtk : 0.0f,
                                          rc.metal > 0 ? rc.mDec : 0.0f, rc.bell > 0 ? rc.bellDec : 0.0f);
        maxLen = longest * 7.0 + 0.06 + (rc.bursts > 0 ? rc.bSpace * rc.bursts : 0.0f);
    }

    /** sanft ausblenden (neuer Schlag auf derselben Spur / Hi-Hat abwuergen) */
    void fadeOut (float seconds)
    {
        if (! active) return;
        fadeStep = 1.0f / juce::jmax (1.0f, (float) (seconds * sr));
    }

    bool isActive() const  { return active; }
    int  getLane() const   { return lane; }
    int  getAge() const    { return age; }

    /** addiert in Main (l/r) und Raum-Send (rl/rr) */
    void render (float* l, float* r, float* rl, float* rr, int start, int end)
    {
        if (! active) return;
        const float panL = std::cos ((prm.pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        const float panR = std::sin ((prm.pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        const float velGain = 0.1f + 0.9f * std::pow (vel, 1.4f);
        const float gain = prm.level * velGain * outGain;
        const float send = prm.room;
        const double dt = 1.0 / sr;

        for (int s = start; s < end; ++s)
        {
            float yl, yr;
            if (isSample) nextSample (yl, yr);
            else { yl = yr = nextSynth ((float) t); }
            t += dt;

            if (fadeStep > 0.0f)
            {
                fade -= fadeStep;
                if (fade <= 0.0f) { active = false; break; }
            }
            const float g = gain * fade;
            yl *= g; yr *= g;
            l[s] += yl * panL;  r[s] += yr * panR;
            if (send > 0.0f) { rl[s] += yl * panL * send; rr[s] += yr * panR * send; }
            if (t > maxLen) { active = false; break; }
        }
        ++age;
    }

private:
    float noise() { return rng.nextFloat() * 2.0f - 1.0f; }
    static float sq (double ph) { return ph - std::floor (ph) < 0.5 ? 1.0f : -1.0f; }

    static float shape (float y, float drive)
    {
        if (drive <= 0.001f) return y;
        const float k = 1.0f + drive;
        return std::tanh (y * k) / std::tanh (k);
    }

    float nextSynth (float tt)
    {
        const float twoPi = juce::MathConstants<float>::twoPi;
        float y = 0.0f;

        // ----- Koerper
        if (rc.body > 0.0f)
        {
            float f = rc.f0 * (1.0f + rc.pAmt * std::exp (-tt / rc.pT));
            if (rc.glide > 0.0f) f *= 1.0f - 0.12f * (1.0f - std::exp (-tt / juce::jmax (0.01f, rc.glide * 4.0f)));
            phase1 += f / sr;
            phase1 -= std::floor (phase1);
            const float ph = (float) phase1;
            float o;
            switch (rc.wave)
            {
                case 1:  o = 0.75f * std::sin (twoPi * ph) + 0.25f * (4.0f * std::abs (ph - 0.5f) - 1.0f); break;
                case 2:  o = std::sin (twoPi * ph) + 0.22f * std::sin (2.0f * twoPi * ph + 0.3f); break;
                default: o = std::sin (twoPi * ph); break;
            }
            const float env = tt < rc.bHold ? 1.0f : std::exp (-(tt - rc.bHold) / rc.bDec);
            const float atk = juce::jmin (1.0f, tt * 2500.0f);
            float b = o * env * atk;
            if (rc.b2 > 0.0f)
            {
                phase2 += f * rc.f2ratio / sr;
                phase2 -= std::floor (phase2);
                b += rc.b2 * std::sin (twoPi * (float) phase2) * std::exp (-tt / (rc.bDec * 0.7f)) * atk;
            }
            y += b * rc.body;
        }

        // ----- Rauschen
        if (rc.noise > 0.0f)
        {
            float env;
            if (rc.bursts > 0)
            {
                env = 0.0f;
                for (int i = 0; i < rc.bursts; ++i)
                {
                    const float ti = tt - rc.bSpace * (float) i;
                    if (ti >= 0.0f) env = juce::jmax (env, std::exp (-ti / rc.bLen));
                }
                const float tail = tt - rc.bSpace * (float) rc.bursts;
                if (tail >= -0.002f) env = juce::jmax (env, 0.75f * std::exp (-juce::jmax (0.0f, tail) / rc.nDec));
            }
            else
            {
                const float a = rc.nAtk > 0.0f ? juce::jmin (1.0f, tt / rc.nAtk) : 1.0f;
                const float tt2 = tt - rc.nAtk;
                env = tt2 < rc.nHold ? a : a * std::exp (-(tt2 - rc.nHold) / rc.nDec);
                if (rc.nHold > 0.0f && tt2 > rc.nHold) env *= std::exp (-(tt2 - rc.nHold) / 0.012f);   // Gate zu
            }
            float lp, bp, hp;
            nf.process (noise(), lp, bp, hp);
            float n = rc.nType == 0 ? lp : (rc.nType == 1 ? bp * 1.6f : hp);
            if (rc.nHP > 0.0f) n = nf2.hp (n);
            if (rc.nLP > 0.0f) n = nf3.lp (n) * 1.25f;
            y += n * env * rc.noise;
        }

        // ----- Klick
        if (rc.click > 0.0f && tt < 0.03f)
        {
            float c = cf.hp (noise()) * std::exp (-tt / rc.cDec);
            if (tt < 0.0004f) c += 0.8f;
            y += c * rc.click;
        }

        // ----- Metall
        if (rc.metal > 0.0f)
        {
            static constexpr float sets[4][6] {
                { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f },        // 808
                { 263.0f, 400.0f, 421.0f, 474.0f, 587.0f, 845.0f },        // hellere Becken
                { 297.0f, 371.0f, 533.0f, 611.0f, 689.0f, 893.0f },        // Crash
                { 318.0f, 457.0f, 501.0f, 627.0f, 719.0f, 977.0f } };      // Ride / akustisch
            float m = 0.0f;
            const auto& set = sets[juce::jlimit (0, 3, rc.mSet)];
            for (int i = 0; i < 6; ++i)
            {
                mPhase[(size_t) i] += set[i] * 1.7f * rc.mMul / sr;
                m += sq (mPhase[(size_t) i]);
            }
            m = m * 0.16f + noise() * 0.15f;
            const float bp = mf1.bp (m);
            const float hp = mf2.hp (bp + m * 0.3f);
            const float a = rc.mAtk > 0.0f ? juce::jmin (1.0f, tt / rc.mAtk) : 1.0f;
            const float env = a * (0.35f * std::exp (-tt / juce::jmin (0.02f, rc.mDec)) + 0.65f * std::exp (-tt / rc.mDec));
            y += hp * env * rc.metal * 1.6f;
        }

        // ----- Glocke
        if (rc.bell > 0.0f)
        {
            phB1 += rc.bellF1 / sr; phB2 += rc.bellF2 / sr;
            phB1 -= std::floor (phB1); phB2 -= std::floor (phB2);
            float b = rc.bellSquare ? (sq (phB1) + sq (phB2)) * 0.5f
                                    : (std::sin (twoPi * (float) phB1) + 0.7f * std::sin (twoPi * (float) phB2)) * 0.6f;
            if (rc.bellSquare) b = mf1.bp (b) * 2.4f + b * 0.05f;
            const float env = 0.55f * std::exp (-tt / 0.012f) + 0.45f * std::exp (-tt / rc.bellDec);
            y += b * env * rc.bell;
        }

        y = shape (y, drv);

        if (rc.crush > 0.0f)   // Lo-Fi: weniger Bits + Samplerate
        {
            const int hold = 1 + (int) (rc.crush * 3.0f);
            if (crushCount++ % hold == 0) crushHold = y;
            const float steps = 64.0f + (1.0f - rc.crush) * 2000.0f;
            y = std::round (crushHold * steps) / steps;
        }
        return y;
    }

    void nextSample (float& yl, float& yr)
    {
        yl = yr = 0.0f;
        if (sample == nullptr) return;
        const auto& a = sample->audio;
        const int n = a.getNumSamples();
        const int i0 = (int) samplePos;
        if (i0 >= n - 1) { active = false; return; }
        const float fr = (float) (samplePos - i0);
        const float* L = a.getReadPointer (0);
        const float* R = a.getReadPointer (a.getNumChannels() > 1 ? 1 : 0);
        yl = L[i0] + fr * (L[i0 + 1] - L[i0]);
        yr = R[i0] + fr * (R[i0 + 1] - R[i0]);
        samplePos += rate;

        const float tt = (float) t;
        float env = sampleTau > 0.0f ? std::exp (-tt / sampleTau) : 1.0f;
        if (snapBoost != 0.0f && tt < 0.012f) env *= 1.0f + snapBoost * (1.0f - tt / 0.012f);
        yl *= env; yr *= env;
        if (std::abs (prm.tone) > 0.02f)
        {
            const float m = (yl + yr) * 0.5f, d = (yl - yr) * 0.5f;
            const float fm = prm.tone < 0.0f ? tf.lp (m) : tf.hp (m);
            yl = fm + d * 0.7f; yr = fm - d * 0.7f;
        }
        yl = shape (yl, drv); yr = shape (yr, drv);
    }

    double sr = 48000.0, t = 0.0, maxLen = 1.0;
    double phase1 = 0.0, phase2 = 0.0, phB1 = 0.0, phB2 = 0.0, samplePos = 0.0, rate = 1.0;
    std::array<double, 6> mPhase {};
    int lane = 0, age = 0, crushCount = 0;
    float vel = 1.0f, fade = 1.0f, fadeStep = 0.0f, drv = 0.0f, outGain = 1.0f, crushHold = 0.0f;
    float sampleTau = 0.0f, snapBoost = 0.0f;
    bool active = false, isSample = false;
    Recipe rc;
    LaneParams prm;
    std::shared_ptr<const DrumSample> sample;
    DSVF nf, nf2, nf3, cf, mf1, mf2, tf;
    juce::Random rng;
};

// ---------------------------------------------------------------------------
//  Alle Stimmen: 3 pro Spur (Becken klingen weiter, Kick/Snare schneiden sich ab)
class DrumEngine
{
public:
    static constexpr int voicesPerLane = 3;

    void prepare (double sampleRate)
    {
        int seed = 5400;
        for (auto& lane : voices)
            for (auto& v : lane) v.prepare (sampleRate, seed += 37);
    }

    void trigger (int lane, float velocity, const LaneParams& p, std::shared_ptr<const DrumSample> smp)
    {
        if (lane < 0 || lane >= DrumInfo::numLanes || p.mute) return;
        auto& vs = voices[(size_t) lane];
        const bool isSmp = p.model >= DrumInfo::sampleModel (lane);
        const bool mono = isSmp ? (lane <= 5) : makeRecipe (lane, p.model).mono;

        if (lane == 4)   // Closed Hat wuergt Open Hat ab
            for (auto& v : voices[5]) v.fadeOut (0.008f);

        DrumVoice* target = nullptr;
        if (mono)
            for (auto& v : vs) v.fadeOut (0.004f);
        for (auto& v : vs) if (! v.isActive()) { target = &v; break; }
        if (target == nullptr)
        {
            // aeltesten Platz ueberschreiben
            target = &vs[0];
            for (auto& v : vs) if (v.getAge() > target->getAge()) target = &v;
        }
        target->trigger (lane, velocity, p, std::move (smp));
    }

    void render (float* l, float* r, float* rl, float* rr, int start, int end)
    {
        for (auto& lane : voices)
            for (auto& v : lane) v.render (l, r, rl, rr, start, end);
    }

    /** fuer die Spur-LEDs: klingt die Spur gerade? */
    bool laneActive (int lane) const
    {
        for (auto& v : voices[(size_t) lane]) if (v.isActive()) return true;
        return false;
    }

    void allOff()
    {
        for (auto& lane : voices) for (auto& v : lane) v.fadeOut (0.005f);
    }

private:
    std::array<std::array<DrumVoice, voicesPerLane>, DrumInfo::numLanes> voices;
};

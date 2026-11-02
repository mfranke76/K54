#include "Presets.h"

// Kurzschreibweise der Parameter-IDs
#define CUT  "cutoff"
#define RES  "resonance"
#define ENV  "envAmount"
#define DRV  "drive"
#define AA   "ampAttack"
#define AD   "ampDecay"
#define AS   "ampSustain"
#define AR   "ampRelease"
#define FA   "fltAttack"
#define FD   "fltDecay"
#define FS   "fltSustain"
#define FR   "fltRelease"
#define GLD  "glide"
#define KTR  "keyTrack"
#define VF   "velFilter"
#define VA   "velAmp"
#define VOL  "master"
#define W1   "osc1Wave"
#define W2   "osc2Wave"
#define W3   "osc3Wave"
#define O1   "osc1Oct"
#define O2   "osc2Oct"
#define O3   "osc3Oct"
#define T2   "osc2Tune"
#define T3   "osc3Tune"
#define L1   "osc1Level"
#define L2   "osc2Level"
#define L3   "osc3Level"
#define NOI  "noiseLevel"
#define PW   "pulseWidth"
#define LR   "lfoRate"
#define LP   "lfoPitch"
#define LF   "lfoFilter"
#define LW   "lfoWave"
#define MODE "voiceMode"
#define DRF  "drift"

enum { TRI = 0, SAW = 1, SQR = 2, PLS = 3 };
enum { R32 = 0, R16 = 1, R8 = 2, R4 = 3, R2 = 4 };
enum { MONO = 0, LEGATO = 1, POLY = 2 };

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets
    {
        // ------------------------------------------------------------- BASS
        { "Keller Classic Bass", "Bass", {
            { W1, SAW }, { O1, R16 }, { W2, SAW }, { O2, R16 }, { T2, 0.06f }, { W3, SQR }, { O3, R32 },
            { L1, 0.85f }, { L2, 0.7f }, { L3, 0.5f },
            { CUT, 380 }, { RES, 0.3f }, { ENV, 0.55f }, { DRV, 0.35f }, { KTR, 0.4f },
            { FA, 0.002f }, { FD, 0.32f }, { FS, 0.1f }, { FR, 0.15f },
            { AA, 0.002f }, { AD, 0.6f }, { AS, 0.85f }, { AR, 0.12f }, { MODE, MONO } } },

        { "Basement Sub", "Bass", {
            { W1, TRI }, { O1, R16 }, { W2, SQR }, { O2, R32 }, { L1, 1.0f }, { L2, 0.45f }, { L3, 0.0f },
            { CUT, 220 }, { RES, 0.1f }, { ENV, 0.25f }, { DRV, 0.4f }, { FD, 0.5f }, { FS, 0.3f },
            { AA, 0.003f }, { AS, 1.0f }, { AR, 0.1f }, { MODE, LEGATO } } },

        { "Pluck Bass", "Bass", {
            { W1, SAW }, { O1, R16 }, { W2, PLS }, { O2, R16 }, { PW, 0.3f }, { L1, 0.8f }, { L2, 0.7f },
            { CUT, 250 }, { RES, 0.45f }, { ENV, 0.7f }, { FA, 0.001f }, { FD, 0.18f }, { FS, 0.0f }, { FR, 0.1f },
            { AA, 0.001f }, { AD, 0.5f }, { AS, 0.0f }, { AR, 0.1f }, { VF, 0.5f }, { MODE, MONO } } },

        { "Acid Lane", "Bass", {
            { W1, SAW }, { O1, R16 }, { L1, 1.0f }, { L2, 0.0f },
            { CUT, 300 }, { RES, 0.78f }, { ENV, 0.65f }, { DRV, 0.55f }, { FD, 0.22f }, { FS, 0.0f },
            { AS, 0.9f }, { AR, 0.08f }, { GLD, 0.08f }, { VF, 0.6f }, { MODE, LEGATO } } },

        { "Funk Bass 73", "Bass", {
            { W1, SQR }, { O1, R16 }, { W2, SAW }, { O2, R16 }, { T2, -0.05f }, { L1, 0.8f }, { L2, 0.6f },
            { CUT, 500 }, { RES, 0.4f }, { ENV, 0.5f }, { DRV, 0.3f }, { FD, 0.15f }, { FS, 0.15f },
            { VF, 0.6f }, { VA, 0.5f }, { AD, 0.3f }, { AS, 0.6f }, { AR, 0.08f }, { MODE, MONO } } },

        { "Fat Three Bass", "Bass", {
            { W1, SAW }, { O1, R16 }, { W2, SAW }, { O2, R16 }, { T2, 0.08f }, { W3, SAW }, { O3, R16 }, { T3, -0.11f },
            { L1, 0.75f }, { L2, 0.7f }, { L3, 0.7f },
            { CUT, 600 }, { RES, 0.2f }, { ENV, 0.4f }, { DRV, 0.5f }, { FD, 0.45f }, { FS, 0.25f },
            { AS, 0.9f }, { AR, 0.12f }, { MODE, MONO } } },

        { "Rubber Bass", "Bass", {
            { W1, TRI }, { O1, R16 }, { W2, SQR }, { O2, R32 }, { L1, 0.9f }, { L2, 0.5f },
            { CUT, 300 }, { RES, 0.55f }, { ENV, 0.6f }, { FD, 0.4f }, { FS, 0.1f },
            { GLD, 0.05f }, { AS, 0.9f }, { AR, 0.1f }, { MODE, LEGATO } } },

        { "Deep Reese", "Bass", {
            { W1, SAW }, { O1, R16 }, { W2, SAW }, { O2, R16 }, { T2, 0.18f }, { W3, SAW }, { O3, R32 },
            { L1, 0.8f }, { L2, 0.8f }, { L3, 0.6f },
            { CUT, 450 }, { RES, 0.15f }, { ENV, 0.1f }, { LF, 0.1f }, { LR, 0.3f }, { DRV, 0.45f },
            { AS, 1.0f }, { AR, 0.2f }, { MODE, LEGATO } } },

        // ------------------------------------------------------------- LEAD
        { "Gold Lead", "Lead", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.08f }, { W3, SQR }, { O3, R4 },
            { L1, 0.85f }, { L2, 0.75f }, { L3, 0.3f },
            { CUT, 1800 }, { RES, 0.35f }, { ENV, 0.4f }, { DRV, 0.3f }, { FD, 0.5f }, { FS, 0.4f },
            { GLD, 0.06f }, { LR, 5.5f }, { AA, 0.01f }, { AS, 0.9f }, { AR, 0.3f }, { MODE, LEGATO } } },

        { "Funk Lead 74", "Lead", {
            { W1, SQR }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.05f }, { L1, 0.8f }, { L2, 0.8f },
            { CUT, 1500 }, { RES, 0.5f }, { ENV, 0.55f }, { FD, 0.3f }, { FS, 0.3f }, { VF, 0.5f },
            { GLD, 0.04f }, { AS, 0.85f }, { AR, 0.2f }, { MODE, LEGATO } } },

        { "Solo Fifth", "Lead", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 7.0f }, { W3, SAW }, { O3, R4 }, { T3, 0.04f },
            { L1, 0.85f }, { L2, 0.6f }, { L3, 0.3f },
            { CUT, 2200 }, { RES, 0.3f }, { ENV, 0.35f }, { FD, 0.6f }, { FS, 0.5f },
            { GLD, 0.05f }, { AS, 0.9f }, { AR, 0.25f }, { MODE, LEGATO } } },

        { "Whistle Lead", "Lead", {
            { W1, TRI }, { O1, R8 }, { W2, TRI }, { O2, R4 }, { L1, 0.9f }, { L2, 0.4f },
            { CUT, 3000 }, { RES, 0.2f }, { ENV, 0.1f }, { LP, 0.18f }, { LR, 5.8f },
            { GLD, 0.1f }, { AA, 0.03f }, { AS, 1.0f }, { AR, 0.35f }, { MODE, LEGATO } } },

        { "Screamer", "Lead", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, -0.09f }, { L1, 0.9f }, { L2, 0.8f },
            { CUT, 900 }, { RES, 0.85f }, { ENV, 0.6f }, { DRV, 0.8f }, { FD, 0.8f }, { FS, 0.5f },
            { GLD, 0.05f }, { AS, 1.0f }, { AR, 0.2f }, { MODE, LEGATO } } },

        { "Portamento Lead", "Lead", {
            { W1, SQR }, { O1, R8 }, { W2, PLS }, { O2, R8 }, { PW, 0.15f }, { L1, 0.75f }, { L2, 0.75f },
            { CUT, 1400 }, { RES, 0.4f }, { ENV, 0.35f }, { FD, 0.5f }, { FS, 0.45f },
            { GLD, 0.25f }, { AS, 1.0f }, { AR, 0.3f }, { MODE, LEGATO } } },

        { "Hollow Square", "Lead", {
            { W1, SQR }, { O1, R8 }, { W2, SQR }, { O2, R4 }, { L1, 0.9f }, { L2, 0.3f },
            { CUT, 2500 }, { RES, 0.15f }, { ENV, 0.2f }, { AS, 0.95f }, { AR, 0.2f }, { MODE, MONO } } },

        { "Nasal Bite", "Lead", {
            { W1, SAW }, { O1, R8 }, { W2, PLS }, { O2, R4 }, { PW, 0.08f }, { L1, 0.8f }, { L2, 0.6f },
            { CUT, 700 }, { RES, 0.6f }, { ENV, 0.7f }, { FD, 0.25f }, { FS, 0.35f }, { DRV, 0.4f },
            { AS, 0.9f }, { AR, 0.15f }, { MODE, MONO } } },

        // ------------------------------------------------------------- BRASS
        { "Brass Stab", "Brass", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.1f }, { W3, SAW }, { O3, R16 },
            { L1, 0.8f }, { L2, 0.7f }, { L3, 0.4f },
            { CUT, 500 }, { RES, 0.2f }, { ENV, 0.6f }, { FA, 0.06f }, { FD, 0.4f }, { FS, 0.45f }, { FR, 0.2f },
            { AA, 0.02f }, { AD, 0.5f }, { AS, 0.7f }, { AR, 0.2f }, { VF, 0.5f }, { MODE, POLY } } },

        { "Soul Brass", "Brass", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, -0.08f }, { L1, 0.8f }, { L2, 0.8f },
            { CUT, 700 }, { RES, 0.15f }, { ENV, 0.5f }, { FA, 0.12f }, { FD, 0.6f }, { FS, 0.55f }, { FR, 0.3f },
            { AA, 0.06f }, { AS, 0.85f }, { AR, 0.3f }, { VF, 0.4f }, { LP, 0.08f }, { LR, 5.0f }, { MODE, POLY } } },

        { "Synth Horns", "Brass", {
            { W1, SAW }, { O1, R8 }, { W2, PLS }, { O2, R8 }, { PW, 0.35f }, { T2, 0.07f }, { W3, SAW }, { O3, R4 },
            { L1, 0.8f }, { L2, 0.6f }, { L3, 0.25f },
            { CUT, 650 }, { RES, 0.25f }, { ENV, 0.55f }, { FA, 0.08f }, { FD, 0.35f }, { FS, 0.4f },
            { AA, 0.03f }, { AS, 0.8f }, { AR, 0.25f }, { VF, 0.5f }, { MODE, POLY } } },

        // ------------------------------------------------------------- PAD
        { "Keller Pad", "Pad", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.12f }, { L1, 0.7f }, { L2, 0.7f },
            { CUT, 900 }, { RES, 0.15f }, { ENV, 0.2f }, { FA, 1.2f }, { FD, 2.0f }, { FS, 0.6f }, { FR, 1.5f },
            { AA, 0.8f }, { AD, 1.0f }, { AS, 0.9f }, { AR, 2.0f }, { LF, 0.15f }, { LR, 0.25f }, { DRF, 0.6f },
            { VA, 0.15f }, { MODE, POLY } } },

        { "Warm Strings", "Pad", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.12f }, { W3, SAW }, { O3, R4 }, { T3, -0.1f },
            { L1, 0.7f }, { L2, 0.65f }, { L3, 0.3f },
            { CUT, 2200 }, { RES, 0.1f }, { ENV, 0.15f }, { FA, 0.5f }, { FD, 1.0f }, { FS, 0.7f },
            { AA, 0.45f }, { AS, 0.9f }, { AR, 1.2f }, { LP, 0.08f }, { LR, 5.0f }, { DRF, 0.5f }, { MODE, POLY } } },

        { "Night Pad", "Pad", {
            { W1, TRI }, { O1, R8 }, { W2, SQR }, { O2, R8 }, { T2, 0.1f }, { W3, TRI }, { O3, R4 },
            { L1, 0.8f }, { L2, 0.5f }, { L3, 0.3f },
            { CUT, 600 }, { RES, 0.25f }, { ENV, 0.15f }, { LF, 0.3f }, { LR, 0.12f },
            { AA, 1.5f }, { AS, 1.0f }, { AR, 3.0f }, { DRF, 0.7f }, { MODE, POLY } } },

        { "Glass Sweep", "Pad", {
            { W1, SAW }, { O1, R8 }, { W2, PLS }, { O2, R8 }, { PW, 0.2f }, { T2, 0.09f }, { L1, 0.7f }, { L2, 0.7f },
            { CUT, 400 }, { RES, 0.6f }, { ENV, 0.55f }, { FA, 3.0f }, { FD, 4.0f }, { FS, 0.2f }, { FR, 2.0f },
            { AA, 1.0f }, { AS, 0.9f }, { AR, 2.5f }, { MODE, POLY } } },

        // ------------------------------------------------------------- KEYS
        { "Funk Clav Pulse", "Keys", {
            { W1, PLS }, { O1, R8 }, { PW, 0.12f }, { W2, PLS }, { O2, R4 }, { L1, 0.9f }, { L2, 0.3f },
            { CUT, 1500 }, { RES, 0.3f }, { ENV, 0.5f }, { FA, 0.001f }, { FD, 0.12f }, { FS, 0.0f },
            { AA, 0.001f }, { AD, 0.35f }, { AS, 0.0f }, { AR, 0.1f }, { VF, 0.7f }, { VA, 0.6f }, { MODE, POLY } } },

        { "Poly Keys", "Keys", {
            { W1, SQR }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.06f }, { L1, 0.7f }, { L2, 0.6f },
            { CUT, 1100 }, { RES, 0.2f }, { ENV, 0.4f }, { FD, 0.6f }, { FS, 0.3f },
            { AD, 1.2f }, { AS, 0.5f }, { AR, 0.4f }, { VF, 0.5f }, { VA, 0.5f }, { MODE, POLY } } },

        { "Organ Tone", "Keys", {
            { W1, SQR }, { O1, R8 }, { W2, SQR }, { O2, R4 }, { W3, SQR }, { O3, R2 },
            { L1, 0.7f }, { L2, 0.5f }, { L3, 0.25f },
            { CUT, 5000 }, { RES, 0.05f }, { ENV, 0.0f }, { AA, 0.005f }, { AS, 1.0f }, { AR, 0.05f },
            { LP, 0.06f }, { LR, 6.5f }, { VA, 0.0f }, { MODE, POLY } } },

        { "Bell Pluck", "Keys", {
            { W1, TRI }, { O1, R8 }, { W2, TRI }, { O2, R2 }, { T2, 0.03f }, { L1, 0.8f }, { L2, 0.5f },
            { CUT, 3000 }, { RES, 0.4f }, { ENV, 0.3f }, { FD, 0.5f }, { FS, 0.1f },
            { AA, 0.001f }, { AD, 1.5f }, { AS, 0.0f }, { AR, 1.2f }, { VA, 0.5f }, { MODE, POLY } } },

        { "Chord Stab", "Keys", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.1f }, { L1, 0.8f }, { L2, 0.7f },
            { CUT, 600 }, { RES, 0.35f }, { ENV, 0.65f }, { FD, 0.2f }, { FS, 0.05f },
            { AD, 0.3f }, { AS, 0.0f }, { AR, 0.15f }, { VF, 0.4f }, { MODE, POLY } } },

        // ------------------------------------------------------------- FX
        { "Laser Zap", "FX", {
            { W1, SAW }, { O1, R4 }, { L1, 1.0f }, { L2, 0.0f },
            { CUT, 300 }, { RES, 0.9f }, { ENV, 1.0f }, { FA, 0.001f }, { FD, 0.12f }, { FS, 0.0f },
            { AD, 0.25f }, { AS, 0.0f }, { AR, 0.1f }, { MODE, MONO } } },

        { "Wind Tunnel", "FX", {
            { L1, 0.0f }, { L2, 0.0f }, { L3, 0.0f }, { NOI, 1.0f },
            { CUT, 800 }, { RES, 0.7f }, { ENV, 0.2f }, { KTR, 1.0f }, { LF, 0.5f }, { LR, 0.15f },
            { AA, 1.0f }, { AS, 1.0f }, { AR, 2.0f }, { MODE, POLY } } },

        { "Sample & Hold", "FX", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 7.0f }, { L1, 0.8f }, { L2, 0.6f },
            { CUT, 900 }, { RES, 0.7f }, { ENV, 0.1f }, { LW, 3 }, { LR, 7.0f }, { LF, 0.55f },
            { AS, 1.0f }, { AR, 0.3f }, { MODE, LEGATO } } },

        { "Siren", "FX", {
            { W1, TRI }, { O1, R8 }, { W2, SQR }, { O2, R8 }, { L1, 0.8f }, { L2, 0.3f },
            { CUT, 2500 }, { RES, 0.3f }, { ENV, 0.0f }, { LP, 0.5f }, { LR, 0.6f }, { LW, 0 },
            { AS, 1.0f }, { AR, 0.4f }, { MODE, MONO } } },

        { "Drone Machine", "FX", {
            { W1, SAW }, { O1, R32 }, { W2, SAW }, { O2, R16 }, { T2, 0.1f }, { W3, SAW }, { O3, R16 }, { T3, 7.02f },
            { L1, 0.7f }, { L2, 0.6f }, { L3, 0.4f },
            { CUT, 500 }, { RES, 0.5f }, { ENV, 0.0f }, { LF, 0.25f }, { LR, 0.07f }, { DRV, 0.5f },
            { AA, 2.0f }, { AS, 1.0f }, { AR, 4.0f }, { DRF, 1.0f }, { MODE, POLY } } },

        // ------------------------------------------------------------- INIT
        { "Init", "Init", {} },
    };
    return presets;
}

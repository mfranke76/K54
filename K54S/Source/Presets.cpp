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
#define T1   "osc1Tune"
#define T2   "osc2Tune"
#define T3   "osc3Tune"
#define TB1  "osc1Table"
#define TB2  "osc2Table"
#define TB3  "osc3Table"
#define P1   "osc1Pos"
#define P2   "osc2Pos"
#define P3   "osc3Pos"
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
#define UNI  "uniVoices"
#define UDT  "uniDetune"
#define USP  "uniSpread"
#define SUB  "subLevel"
#define FTY  "fltType"
#define HEAT "heat"
#define L2R  "lfo2Rate"
#define L2W  "lfo2Wave"
#define CHM  "chorusMix"
#define CHR  "chorusRate"
#define CHD  "chorusDepth"
#define DLM  "delayMix"
#define DLT  "delayTime"
#define DLF  "delayFb"
#define RVS  "revSend"
#define RVZ  "revSize"
#define M1S  "mod1Src"
#define M1D  "mod1Dst"
#define M1A  "mod1Amt"
#define M2S  "mod2Src"
#define M2D  "mod2Dst"
#define M2A  "mod2Amt"
#define M3S  "mod3Src"
#define M3D  "mod3Dst"
#define M3A  "mod3Amt"
#define SKT  "smpKeyTrack"
#define SOS  "smpOneShot"
#define SLP  "smpLoop"
#define SLS  "smpLoopStart"
#define RVD  "revDamp"
#define RVL  "revLevel"
#define MEA  "modAttack"
#define MED  "modDecay"
#define MES  "modSustain"
#define MER  "modRelease"
#define MWV  "mwVibrato"
#define MWC  "mwCutoff"
#define ATV  "atVibrato"
// KEYS-Ebene + Talkbox (1.2.0)
#define SON  "synthOn"
#define KON  "keysOn"
#define KM   "keysModel"
#define KV   "keysVariant"
#define KL   "keysLevel"
#define KO   "keysOct"
#define KT   "keysTone"
#define KD   "keysDecay"
#define KA   "keysA"
#define KB   "keysB"
#define KVEL "keysVel"
#define KDRV "keysDrive"
#define KFX  "keysFx"
#define KFA  "keysFxAmt"
#define KFR  "keysFxRate"
#define KW   "keysWheel"
#define TKM  "talkMix"
#define TKS  "talkSrc"
#define TKF  "talkShift"
#define TKR  "talkRate"

enum { CLAV = 0, TINE, TWHEEL, PIPE, FMP, FMBELL, FMBASS, HARP };
enum { FX_OFF = 0, FX_PHASER, FX_TREM, FX_PAN, FX_CHORUS, FX_WAHENV, FX_WAHMW, FX_LESLIE };
enum { KW_OFF = 0, KW_LESLIE, KW_SWELL, KW_VIB, KW_REG, KW_TONE };
enum { KO_M2 = 0, KO_M1, KO_0, KO_P1, KO_P2 };
enum { TK_WHEEL = 0, TK_ENV, TK_AT, TK_LFO };
enum { TRI = 0, SAW = 1, SQR = 2, PLS = 3, WT = 4, SMP = 5, PNO = 6 };
enum { T_BASIC = 0, T_PWM, T_HARM, T_VOCAL, T_FM, T_SYNC, T_ORGAN, T_BELL, T_GROWL, T_PRINC, T_FLUTE, T_REED, T_EP, T_USER };
enum { R32 = 0, R16 = 1, R8 = 2, R4 = 3, R2 = 4 };
enum { MONO = 0, LEGATO = 1, POLY = 2 };
enum { UNI1 = 0, UNI2 = 1, UNI3 = 2, UNI4 = 3, UNI5 = 4, UNI7 = 5 };
enum { LP24 = 0, LP12 = 1, BPF = 2, HPF = 3 };
enum { D_1_16 = 2, D_1_8 = 5, D_1_8D = 6, D_1_4 = 8 };
enum { S_LFO1 = 1, S_LFO2, S_FENV, S_AENV, S_MENV, S_VEL, S_WHEEL, S_BEND, S_AT, S_KEY, S_RND, S_BREATH, S_EXPR };
enum { D_PITCH = 1, D_O1, D_O2, D_O3, D_CUT, D_RES, D_DRV, D_PW, D_VOL, D_PAN, D_DET, D_NOISE, D_SUB,
       D_LV1, D_LV2, D_LV3, D_POS1, D_POS2, D_POS3, D_POSALL };

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets
    {
        { "Init", "Init", { { MODE, POLY } } },

        // ============================================================ PIANO (Konzertfluegel-Samples)
#define PIANO_BASE { W1, PNO }, { O1, R8 }, { L1, 1.0f }, { L2, 0.0f }, { RES, 0.0f }, { ENV, 0.0f }, { DRV, 0.0f }, { DRF, 0.0f }, { KTR, 0.5f }, \
                   { AA, 0.001f }, { AD, 10.0f }, { AS, 1.0f }, { AR, 0.45f }, { FTY, LP12 }, { MWV, 0.15f }, { MODE, POLY }
        { "Konzertfluegel", "Piano", { PIANO_BASE,
            { CUT, 2800 }, { M1S, S_VEL }, { M1D, D_CUT }, { M1A, 0.55f }, { VA, 0.75f }, { RVS, 0.3f }, { RVZ, 0.8f }, { VOL, 3.0f } } },

        { "Klassik Pianissimo", "Piano", { PIANO_BASE,
            { CUT, 1500 }, { M1S, S_VEL }, { M1D, D_CUT }, { M1A, 0.6f }, { VA, 0.85f }, { RVS, 0.38f }, { RVZ, 0.9f }, { RVD, 0.55f }, { AR, 0.6f }, { VOL, 5.0f } } },

        { "Pop Piano Bright", "Piano", { PIANO_BASE,
            { CUT, 20000 }, { DRV, 0.12f }, { HEAT, 0.12f }, { VA, 0.5f }, { CHM, 0.1f }, { RVS, 0.16f }, { RVZ, 0.55f }, { VOL, 2.0f } } },

        { "Ballade Piano", "Piano", { PIANO_BASE,
            { CUT, 4200 }, { M1S, S_VEL }, { M1D, D_CUT }, { M1A, 0.35f }, { VA, 0.7f }, { CHM, 0.15f }, { DLM, 0.1f }, { DLT, D_1_4 },
            { RVS, 0.42f }, { RVZ, 0.92f }, { AR, 0.8f }, { VOL, 3.0f } } },

        { "Grand Oktaven", "Piano", { PIANO_BASE,
            { W2, PNO }, { O2, R4 }, { L2, 0.4f }, { CUT, 5000 }, { M1S, S_VEL }, { M1D, D_CUT }, { M1A, 0.4f }, { VA, 0.7f }, { RVS, 0.3f }, { VOL, 1.0f } } },

        { "Honky Tonk", "Piano", { PIANO_BASE,
            { CUT, 6000 }, { UNI, UNI2 }, { UDT, 0.14f }, { USP, 0.35f }, { VA, 0.6f }, { RVS, 0.15f }, { VOL, 2.0f } } },

        { "Lo-Fi Felt Piano", "Piano", { PIANO_BASE,
            { CUT, 850 }, { RES, 0.1f }, { M1S, S_VEL }, { M1D, D_CUT }, { M1A, 0.3f }, { DRF, 0.5f }, { LP, 0.035f }, { LR, 0.6f },
            { NOI, 0.02f }, { VA, 0.6f }, { AR, 0.3f }, { RVS, 0.3f }, { HEAT, 0.2f }, { VOL, 6.0f } } },

        { "Piano + Pad", "Piano", { PIANO_BASE,
            { W2, SAW }, { O2, R8 }, { T2, 0.08f }, { L2, 0.0f }, { CUT, 3500 }, { M1S, S_VEL }, { M1D, D_CUT }, { M1A, 0.4f },
            { MEA, 1.4f }, { MED, 1.0f }, { MES, 1.0f }, { MER, 1.5f }, { M2S, S_MENV }, { M2D, D_LV2 }, { M2A, 0.32f },
            { CHM, 0.45f }, { RVS, 0.4f }, { AR, 1.2f }, { VOL, 0.0f } } },

        { "Piano + Strings", "Piano", { PIANO_BASE,
            { W2, SAW }, { O2, R8 }, { T2, 0.12f }, { W3, SAW }, { O3, R4 }, { T3, -0.1f }, { L2, 0.0f }, { L3, 0.0f },
            { CUT, 4000 }, { M1S, S_VEL }, { M1D, D_CUT }, { M1A, 0.35f },
            { MEA, 0.9f }, { MED, 1.0f }, { MES, 1.0f }, { MER, 1.4f }, { M2S, S_MENV }, { M2D, D_LV2 }, { M2A, 0.3f }, { M3S, S_MENV }, { M3D, D_LV3 }, { M3A, 0.15f },
            { CHM, 0.5f }, { RVS, 0.42f }, { AR, 1.2f }, { VOL, -1.5f } } },
#undef PIANO_BASE

        // ============================================================ E-PIANO & KEYS
        { "Tine E-Piano", "E-Piano & Keys", {
            { W1, WT }, { TB1, T_EP }, { P1, 1.0f }, { O1, R8 }, { L1, 0.9f }, { L2, 0.0f }, { DRF, 0.1f },
            { M1S, S_AENV }, { M1D, D_POS1 }, { M1A, -0.85f }, { M2S, S_VEL }, { M2D, D_POS1 }, { M2A, -0.25f },
            { CUT, 9000 }, { ENV, 0.0f }, { AA, 0.001f }, { AD, 2.6f }, { AS, 0.0f }, { AR, 0.35f }, { VA, 0.65f },
            { CHM, 0.3f }, { RVS, 0.2f }, { MWV, 0.15f }, { MODE, POLY } } },

        { "Suitcase Tremolo", "E-Piano & Keys", {
            { W1, WT }, { TB1, T_EP }, { P1, 1.0f }, { O1, R8 }, { L1, 0.9f }, { L2, 0.0f },
            { M1S, S_AENV }, { M1D, D_POS1 }, { M1A, -0.8f }, { M2S, S_LFO2 }, { M2D, D_PAN }, { M2A, 0.55f }, { L2R, 4.8f },
            { CUT, 7000 }, { ENV, 0.0f }, { AA, 0.001f }, { AD, 3.0f }, { AS, 0.0f }, { AR, 0.4f }, { VA, 0.6f },
            { HEAT, 0.15f }, { RVS, 0.22f }, { MWV, 0.0f }, { MODE, POLY } } },

        { "Wurly Pop", "E-Piano & Keys", {
            { W1, WT }, { TB1, T_EP }, { P1, 0.55f }, { O1, R8 }, { W2, WT }, { TB2, T_REED }, { P2, 0.15f }, { O2, R8 }, { L1, 0.8f }, { L2, 0.28f },
            { CUT, 2600 }, { RES, 0.1f }, { ENV, 0.35f }, { FD, 0.45f }, { FS, 0.2f }, { VF, 0.7f },
            { AA, 0.001f }, { AD, 1.6f }, { AS, 0.15f }, { AR, 0.25f }, { VA, 0.6f }, { DRV, 0.25f }, { CHM, 0.2f }, { MODE, POLY } } },

        { "Clavinet Funk", "E-Piano & Keys", {
            { W1, PLS }, { O1, R8 }, { PW, 0.12f }, { W2, PLS }, { O2, R4 }, { L1, 0.9f }, { L2, 0.3f }, { FTY, BPF },
            { CUT, 1700 }, { RES, 0.3f }, { ENV, 0.5f }, { FA, 0.001f }, { FD, 0.14f }, { FS, 0.1f },
            { AA, 0.001f }, { AD, 0.4f }, { AS, 0.0f }, { AR, 0.1f }, { VF, 0.7f }, { VA, 0.6f }, { DRF, 0.1f }, { MODE, POLY } } },

        { "FM Glass Keys", "E-Piano & Keys", {
            { W1, WT }, { TB1, T_FM }, { P1, 0.8f }, { O1, R8 }, { W2, WT }, { TB2, T_BELL }, { P2, 0.2f }, { O2, R4 }, { L1, 0.8f }, { L2, 0.25f },
            { M1S, S_AENV }, { M1D, D_POS1 }, { M1A, -0.6f }, { CUT, 12000 }, { ENV, 0.0f },
            { AA, 0.001f }, { AD, 1.8f }, { AS, 0.0f }, { AR, 0.6f }, { VA, 0.6f }, { CHM, 0.3f }, { DLM, 0.15f }, { RVS, 0.3f }, { MODE, POLY } } },

        // ============================================================ ORGEL (Kirche, Drawbar, Harmonium)
#define CHURCH { CUT, 12000 }, { RES, 0.0f }, { ENV, 0.0f }, { DRV, 0.0f }, { DRF, 0.05f }, { VA, 0.0f }, { KTR, 0.0f }, \
               { AA, 0.06f }, { AS, 1.0f }, { AR, 0.35f }, { MEA, 0.001f }, { MED, 0.07f }, { MES, 0.0f }, \
               { RVS, 0.55f }, { RVZ, 0.96f }, { RVD, 0.3f }, { RVL, 1.1f }, { MWV, 0.0f }, { MODE, POLY }
        { "Kirchenorgel Principal 8'", "Orgel", { CHURCH,
            { W1, WT }, { TB1, T_PRINC }, { P1, 0.0f }, { O1, R8 }, { W2, WT }, { TB2, T_PRINC }, { P2, 0.0f }, { O2, R4 },
            { L1, 0.8f }, { L2, 0.3f }, { M1S, S_MENV }, { M1D, D_NOISE }, { M1A, 0.22f }, { VOL, -8.0f } } },

        { "Kirchenorgel Plenum", "Orgel", { CHURCH,
            { W1, WT }, { TB1, T_PRINC }, { P1, 0.85f }, { O1, R8 }, { W2, WT }, { TB2, T_PRINC }, { P2, 0.2f }, { O2, R16 },
            { W3, WT }, { TB3, T_PRINC }, { P3, 0.6f }, { O3, R4 }, { L1, 0.75f }, { L2, 0.4f }, { L3, 0.3f },
            { M1S, S_MENV }, { M1D, D_NOISE }, { M1A, 0.15f }, { VOL, -9.0f } } },

        { "Kirchenorgel Tutti", "Orgel", { CHURCH,
            { W1, WT }, { TB1, T_PRINC }, { P1, 1.0f }, { O1, R8 }, { W2, WT }, { TB2, T_PRINC }, { P2, 0.3f }, { O2, R16 },
            { W3, WT }, { TB3, T_REED }, { P3, 0.9f }, { O3, R8 }, { L1, 0.7f }, { L2, 0.45f }, { L3, 0.45f },
            { M1S, S_MENV }, { M1D, D_NOISE }, { M1A, 0.1f }, { VOL, -10.0f } } },

        { "Floete 8' + 4'", "Orgel", { CHURCH,
            { W1, WT }, { TB1, T_FLUTE }, { P1, 0.0f }, { O1, R8 }, { W2, WT }, { TB2, T_FLUTE }, { P2, 1.0f }, { O2, R4 },
            { L1, 0.85f }, { L2, 0.35f }, { M1S, S_MENV }, { M1D, D_NOISE }, { M1A, 0.35f }, { LP, 0.02f }, { LR, 5.6f }, { VOL, -10.0f } } },

        { "Kathedrale Pedal 16'+32'", "Orgel", { CHURCH,
            { W1, WT }, { TB1, T_PRINC }, { P1, 0.3f }, { O1, R16 }, { W2, WT }, { TB2, T_FLUTE }, { P2, 0.0f }, { O2, R32 },
            { L1, 0.8f }, { L2, 0.6f }, { M1S, S_MENV }, { M1D, D_NOISE }, { M1A, 0.12f }, { AA, 0.12f }, { VOL, -10.0f } } },

        { "Harmonium", "Orgel", {
            { W1, WT }, { TB1, T_REED }, { P1, 0.0f }, { O1, R8 }, { W2, WT }, { TB2, T_REED }, { P2, 0.3f }, { O2, R4 }, { L1, 0.8f }, { L2, 0.3f },
            { CUT, 5000 }, { ENV, 0.0f }, { VA, 0.2f }, { AA, 0.12f }, { AS, 1.0f }, { AR, 0.2f },
            { M1S, S_LFO2 }, { M1D, D_VOL }, { M1A, 0.06f }, { L2R, 0.7f }, { RVS, 0.25f }, { DRF, 0.2f }, { MODE, POLY } } },

        { "Jazz Orgel (Drawbar)", "Orgel", {
            { W1, WT }, { TB1, T_ORGAN }, { P1, 0.75f }, { O1, R8 }, { L1, 0.9f }, { L2, 0.0f }, { CUT, 14000 }, { ENV, 0.0f }, { VA, 0.0f },
            { AA, 0.003f }, { AS, 1.0f }, { AR, 0.05f }, { DRV, 0.2f }, { HEAT, 0.12f },
            { CHM, 0.6f }, { CHR, 0.8f }, { CHD, 0.45f }, { M1S, S_LFO2 }, { M1D, D_PAN }, { M1A, 0.25f }, { L2R, 6.0f }, { RVS, 0.18f }, { MWV, 0.0f }, { MODE, POLY } } },

        { "Gospel Orgel", "Orgel", {
            { W1, WT }, { TB1, T_ORGAN }, { P1, 1.0f }, { O1, R8 }, { W2, WT }, { TB2, T_ORGAN }, { P2, 0.4f }, { O2, R16 }, { L1, 0.8f }, { L2, 0.35f },
            { CUT, 12000 }, { ENV, 0.0f }, { VA, 0.0f }, { AA, 0.003f }, { AS, 1.0f }, { AR, 0.06f }, { DRV, 0.35f }, { HEAT, 0.3f },
            { CHM, 0.65f }, { CHR, 0.95f }, { CHD, 0.5f }, { RVS, 0.22f }, { MWV, 0.0f }, { VOL, -8.0f }, { MODE, POLY } } },
#undef CHURCH

        // ============================================================ SOUL KEYS 70s (KEYS-Ebene: Clav, Tines, Harp + Talkbox, Synth-Wand)
#define KEYS_ONLY { SON, 0.0f }, { KON, 1.0f }, { MODE, POLY }
        { "Funk Clav 72", "Soul Keys 70s", { KEYS_ONLY,
            { KM, CLAV }, { KV, 1 }, { KT, 0.55f }, { KD, 0.45f }, { KA, 0.35f }, { KB, 0.7f }, { KVEL, 0.75f }, { KDRV, 0.12f },
            { KL, -3.0f }, { RVS, 0.06f } } },

        { "Clav Wah Groove", "Soul Keys 70s", { KEYS_ONLY,
            { KM, CLAV }, { KV, 0 }, { KT, 0.5f }, { KD, 0.4f }, { KA, 0.45f }, { KB, 0.6f }, { KVEL, 0.7f }, { KDRV, 0.2f },
            { KFX, FX_WAHENV }, { KFA, 0.6f }, { KFR, 0.45f }, { KL, -3.0f }, { RVS, 0.06f } } },

        { "Clav Phase Funk", "Soul Keys 70s", { KEYS_ONLY,
            { KM, CLAV }, { KV, 4 }, { KT, 0.5f }, { KA, 0.3f }, { KB, 0.55f }, { KVEL, 0.7f }, { KDRV, 0.1f },
            { KFX, FX_PHASER }, { KFA, 0.7f }, { KFR, 0.35f }, { KL, -3.0f }, { RVS, 0.08f } } },

        { "Clav Mute Pluck", "Soul Keys 70s", { KEYS_ONLY,
            { KM, CLAV }, { KV, 2 }, { KT, 0.45f }, { KA, 0.85f }, { KB, 0.4f }, { KVEL, 0.8f }, { KL, -2.0f }, { RVS, 0.05f } } },

        { "Clav Wah Pedal (Mod-Rad)", "Soul Keys 70s", { KEYS_ONLY,
            { KM, CLAV }, { KV, 1 }, { KA, 0.4f }, { KB, 0.65f }, { KDRV, 0.25f }, { KFX, FX_WAHMW }, { KFA, 0.85f }, { MWV, 0.0f },
            { KL, -3.0f }, { RVS, 0.06f } } },

        { "Soul Tines", "Soul Keys 70s", { KEYS_ONLY,
            { KM, TINE }, { KV, 0 }, { KT, 0.5f }, { KD, 0.55f }, { KA, 0.45f }, { KB, 0.5f }, { KVEL, 0.7f },
            { KFX, FX_PHASER }, { KFA, 0.35f }, { KFR, 0.25f }, { KL, -2.0f }, { RVS, 0.15f } } },

        { "Suitcase Soul", "Soul Keys 70s", { KEYS_ONLY,
            { KM, TINE }, { KV, 1 }, { KT, 0.45f }, { KA, 0.55f }, { KB, 0.45f }, { KVEL, 0.7f },
            { KFX, FX_PAN }, { KFA, 0.7f }, { KFR, 0.45f }, { KL, -2.0f }, { RVS, 0.15f } } },

        { "Sunshine Tines", "Soul Keys 70s", { KEYS_ONLY,
            { KM, TINE }, { KV, 2 }, { KT, 0.6f }, { KD, 0.6f }, { KA, 0.35f }, { KB, 0.65f }, { KVEL, 0.65f },
            { KFX, FX_CHORUS }, { KFA, 0.45f }, { KFR, 0.35f }, { KL, -2.0f }, { RVS, 0.22f } } },

        { "Mellow Ballad Tines", "Soul Keys 70s", { KEYS_ONLY,
            { KM, TINE }, { KV, 3 }, { KT, 0.35f }, { KD, 0.7f }, { KA, 0.2f }, { KB, 0.3f }, { KVEL, 0.6f },
            { KFX, FX_TREM }, { KFA, 0.25f }, { KFR, 0.35f }, { KL, -1.0f }, { RVS, 0.25f } } },

        { "Reed Piano Pop", "Soul Keys 70s", { KEYS_ONLY,
            { KM, TINE }, { KV, 4 }, { KT, 0.5f }, { KA, 0.45f }, { KVEL, 0.7f },
            { KFX, FX_TREM }, { KFA, 0.35f }, { KFR, 0.5f }, { KL, -3.0f }, { RVS, 0.12f } } },

        { "Tines + Pad", "Soul Keys 70s", {
            { KON, 1.0f }, { KM, TINE }, { KV, 0 }, { KA, 0.4f }, { KB, 0.5f }, { KFX, FX_PHASER }, { KFA, 0.3f }, { KFR, 0.2f }, { KL, -3.0f },
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.1f }, { L1, 0.5f }, { L2, 0.5f }, { FTY, LP12 }, { CUT, 1800 }, { ENV, 0.05f },
            { AA, 1.2f }, { AS, 1.0f }, { AR, 1.6f }, { CHM, 0.5f }, { VOL, -16.0f }, { RVS, 0.25f }, { MODE, POLY } } },

        { "Blues Harp", "Soul Keys 70s", { KEYS_ONLY,
            { KM, HARP }, { KV, 0 }, { KT, 0.5f }, { KA, 0.35f }, { KB, 0.45f }, { KVEL, 0.5f }, { KW, KW_VIB }, { MWV, 0.0f },
            { KL, -4.0f }, { RVS, 0.15f } } },

        { "Harp Through Mic", "Soul Keys 70s", { KEYS_ONLY,
            { KM, HARP }, { KV, 1 }, { KT, 0.55f }, { KA, 0.55f }, { KB, 0.55f }, { KVEL, 0.5f }, { KDRV, 0.15f }, { KW, KW_VIB }, { MWV, 0.0f },
            { KL, -4.0f }, { RVS, 0.12f } } },

        { "Chromatic Harp Ballad", "Soul Keys 70s", { KEYS_ONLY,
            { KM, HARP }, { KV, 2 }, { KT, 0.6f }, { KA, 0.2f }, { KB, 0.25f }, { KVEL, 0.4f }, { KW, KW_VIB }, { MWV, 0.0f },
            { KL, -4.0f }, { RVS, 0.25f } } },

        { "Talk Box Lead", "Soul Keys 70s", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.1f }, { L1, 0.9f }, { L2, 0.6f }, { CUT, 7000 }, { RES, 0.1f }, { ENV, 0.1f },
            { AA, 0.01f }, { AS, 1.0f }, { AR, 0.2f }, { GLD, 0.04f }, { DRV, 0.3f }, { MWV, 0.0f }, { ATV, 0.3f },
            { TKM, 0.9f }, { TKS, TK_WHEEL }, { DLM, 0.12f }, { DLT, D_1_8D }, { RVS, 0.12f }, { MODE, LEGATO } } },

        { "Talk Box Funk", "Soul Keys 70s", {
            { W1, SQR }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.06f }, { L1, 0.8f }, { L2, 0.7f }, { CUT, 5000 }, { ENV, 0.2f }, { DRV, 0.45f },
            { AA, 0.003f }, { AD, 0.4f }, { AS, 0.7f }, { AR, 0.12f }, { MWV, 0.0f },
            { TKM, 0.9f }, { TKS, TK_ENV }, { TKF, -0.2f }, { RVS, 0.08f }, { MODE, MONO } } },

        { "Talk Box Chords", "Soul Keys 70s", {
            { W1, SAW }, { O1, R8 }, { W2, PLS }, { O2, R8 }, { PW, 0.3f }, { L1, 0.75f }, { L2, 0.55f }, { CUT, 6000 }, { ENV, 0.0f },
            { AA, 0.02f }, { AS, 1.0f }, { AR, 0.3f }, { MWV, 0.0f }, { TKM, 0.85f }, { TKS, TK_LFO }, { TKR, 0.8f }, { CHM, 0.3f }, { RVS, 0.2f }, { MODE, POLY } } },

        { "Synth Wall Bass", "Soul Keys 70s", {
            { W1, SAW }, { O1, R16 }, { W2, SQR }, { O2, R16 }, { T2, 0.05f }, { L1, 0.9f }, { L2, 0.6f }, { SUB, 0.3f },
            { CUT, 420 }, { RES, 0.35f }, { ENV, 0.5f }, { DRV, 0.35f }, { FD, 0.3f }, { FS, 0.25f },
            { AA, 0.002f }, { AS, 0.9f }, { AR, 0.1f }, { GLD, 0.02f }, { VF, 0.4f }, { MODE, MONO } } },

        { "Synth Wall Lead", "Soul Keys 70s", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.08f }, { W3, SQR }, { O3, R4 }, { L1, 0.85f }, { L2, 0.6f }, { L3, 0.2f },
            { CUT, 2600 }, { RES, 0.3f }, { ENV, 0.35f }, { FA, 0.02f }, { FD, 0.5f }, { FS, 0.5f },
            { AS, 1.0f }, { AR, 0.25f }, { GLD, 0.08f }, { MWV, 0.3f }, { ATV, 0.35f }, { DLM, 0.2f }, { DLT, D_1_8D }, { RVS, 0.15f }, { MODE, LEGATO } } },

        { "Synth Wall Brass", "Soul Keys 70s", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.1f }, { L1, 0.8f }, { L2, 0.7f }, { UNI, UNI2 }, { UDT, 0.15f },
            { CUT, 900 }, { RES, 0.15f }, { ENV, 0.55f }, { FA, 0.06f }, { FD, 0.5f }, { FS, 0.5f },
            { AA, 0.03f }, { AS, 0.9f }, { AR, 0.3f }, { CHM, 0.2f }, { RVS, 0.2f }, { VOL, -8.0f }, { MODE, POLY } } },

        // ============================================================ TONEWHEEL & LESLIE
#define TW_BASE KEYS_ONLY, { KM, TWHEEL }, { KFX, FX_LESLIE }, { KW, KW_LESLIE }, { MWV, 0.0f }, { KL, -1.0f }
        { "Jazz Drawbars", "Tonewheel & Leslie", { TW_BASE,
            { KV, 0 }, { KA, 0.55f }, { KB, 0.35f }, { KD, 0.35f }, { KT, 0.55f }, { KDRV, 0.1f }, { KFA, 0.7f }, { RVS, 0.12f } } },
        { "Gospel Leslie", "Tonewheel & Leslie", { TW_BASE,
            { KV, 1 }, { KA, 0.0f }, { KB, 0.4f }, { KT, 0.65f }, { KDRV, 0.3f }, { KFA, 0.8f }, { RVS, 0.15f } } },
        { "Rock Organ Drive", "Tonewheel & Leslie", { TW_BASE,
            { KV, 7 }, { KA, 0.0f }, { KB, 0.5f }, { KT, 0.7f }, { KDRV, 0.6f }, { KFA, 0.85f }, { RVS, 0.1f }, { KL, -6.0f } } },
        { "Ballad Drawbars", "Tonewheel & Leslie", { TW_BASE,
            { KV, 4 }, { KA, 0.0f }, { KB, 0.15f }, { KT, 0.45f }, { KFA, 0.5f }, { RVS, 0.22f } } },
        { "Blues Organ", "Tonewheel & Leslie", { TW_BASE,
            { KV, 8 }, { KA, 0.4f }, { KB, 0.3f }, { KT, 0.6f }, { KDRV, 0.35f }, { KFA, 0.75f }, { RVS, 0.12f } } },
        { "Full Organ 888", "Tonewheel & Leslie", { TW_BASE,
            { KV, 2 }, { KA, 0.0f }, { KB, 0.3f }, { KT, 0.55f }, { KDRV, 0.25f }, { KFA, 0.75f }, { RVS, 0.15f }, { KL, -7.0f } } },
        { "Soft Organ 8'", "Tonewheel & Leslie", { TW_BASE,
            { KV, 5 }, { KA, 0.0f }, { KB, 0.1f }, { KT, 0.4f }, { KFA, 0.45f }, { RVS, 0.25f }, { KL, -2.0f } } },
        { "Groove Organ Perc", "Tonewheel & Leslie", { TW_BASE,
            { KV, 6 }, { KA, 0.7f }, { KD, 0.25f }, { KB, 0.45f }, { KT, 0.6f }, { KDRV, 0.2f }, { KFA, 0.7f }, { RVS, 0.1f } } },
        { "Theater Drawbars", "Tonewheel & Leslie", { TW_BASE,
            { KV, 9 }, { KA, 0.0f }, { KB, 0.1f }, { KT, 0.5f }, { KFA, 0.6f }, { KFR, 0.7f }, { KW, KW_OFF }, { RVS, 0.3f } } },
#undef TW_BASE

        // ============================================================ KIRCHENORGELN DER WELT (Pfeifenorgel, 15 Orgelwelten)
#define PIPE_BASE KEYS_ONLY, { KM, PIPE }, { KT, 0.5f }, { KD, 0.5f }, { KB, 0.4f }, { MWV, 0.0f }, \
                  { RVS, 0.55f }, { RVZ, 0.97f }, { RVD, 0.3f }, { RVL, 1.1f }, { KL, -3.0f }
        { "Notre-Dame Paris Plenum", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 0 }, { KA, 0.85f } } },
        { "Kathedrale Crescendo (Mod-Rad)", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 0 }, { KA, 0.3f }, { KW, KW_REG } } },
        { "Notre-Dame Grand Choeur Chamades", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 0 }, { KA, 1.0f }, { KL, -2.0f }, { RVS, 0.6f } } },
        { "Notre-Dame Flute harmonique", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 0 }, { KA, 0.0f }, { KT, 0.45f } } },
        { "Freiberg Plenum", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 1 }, { KA, 0.62f }, { KB, 0.5f } } },
        { "Freiberg Tutti mit Zungen", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 1 }, { KA, 1.0f }, { KB, 0.5f }, { KL, -3.5f } } },
        { "Freiberg Silbermann", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 1 }, { KA, 0.75f }, { KB, 0.5f } } },
        { "Leipzig Bach-Choral", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 2 }, { KA, 0.45f } } },
        { "Passau Dom Tutti", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 3 }, { KA, 1.0f }, { KL, -5.0f } } },
        { "Koeln Dom Grand Choeur", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 4 }, { KA, 0.85f }, { KL, -4.0f } } },
        { "Ulm Muenster Romantik", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 5 }, { KA, 0.35f } } },
        { "Rom St. Peter Ripieno", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 6 }, { KA, 0.8f } } },
        { "London Westminster Diapason", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 7 }, { KA, 0.35f } } },
        { "Haarlem St. Bavo Brillant", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 8 }, { KA, 0.75f }, { KB, 0.5f } } },
        { "Wien Stephansdom", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 9 }, { KA, 0.7f } } },
        { "Hamburg Michel Festlich", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 10 }, { KA, 0.9f }, { KL, -4.0f } } },
        { "Riga Dom Abendlied", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 11 }, { KA, 0.3f }, { KT, 0.4f } } },
        { "Wanamaker Streicher", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 12 }, { KA, 0.2f } } },
        { "Theater-Orgel Tibia", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 13 }, { KA, 0.4f }, { RVS, 0.3f }, { RVZ, 0.75f } } },
        { "Dorfkirche Sonntag", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 14 }, { KA, 0.45f }, { KB, 0.6f }, { RVS, 0.4f }, { RVZ, 0.85f } } },
        { "Floeten-Choral leise", "Kirchenorgeln der Welt", { PIPE_BASE, { KV, 5 }, { KA, 0.0f }, { KT, 0.35f } } },
#undef PIPE_BASE

        // ============================================================ FM KLASSIKER (80er Digital-Synth)
#define FM_BASE KEYS_ONLY, { KL, -3.0f }
        { "FM Ballade Piano", "FM Klassiker", { FM_BASE, { KM, FMP }, { KV, 0 }, { KA, 0.45f }, { KB, 0.5f }, { KFX, FX_CHORUS }, { KFA, 0.35f }, { RVS, 0.2f } } },
        { "FM Bright Tine", "FM Klassiker", { FM_BASE, { KM, FMP }, { KV, 1 }, { KA, 0.5f }, { KB, 0.55f }, { KFX, FX_CHORUS }, { KFA, 0.5f }, { RVS, 0.18f } } },
        { "FM Soft Keys", "FM Klassiker", { FM_BASE, { KM, FMP }, { KV, 2 }, { KA, 0.4f }, { KB, 0.4f }, { KFX, FX_TREM }, { KFA, 0.2f }, { RVS, 0.25f } } },
        { "FM Hard Funk Keys", "FM Klassiker", { FM_BASE, { KM, FMP }, { KV, 3 }, { KA, 0.55f }, { KB, 0.6f }, { KD, 0.35f }, { RVS, 0.08f } } },
        { "FM Tubular Bells", "FM Klassiker", { FM_BASE, { KM, FMBELL }, { KV, 0 }, { KA, 0.5f }, { KB, 0.5f }, { KD, 0.6f }, { RVS, 0.35f }, { RVZ, 0.9f } } },
        { "FM Glass Bell", "FM Klassiker", { FM_BASE, { KM, FMBELL }, { KV, 1 }, { KA, 0.45f }, { KB, 0.4f }, { KFX, FX_CHORUS }, { KFA, 0.4f }, { RVS, 0.3f } } },
        { "FM Celesta", "FM Klassiker", { FM_BASE, { KM, FMBELL }, { KV, 2 }, { KA, 0.45f }, { KB, 0.4f }, { RVS, 0.25f } } },
        { "FM Marimba", "FM Klassiker", { FM_BASE, { KM, FMBELL }, { KV, 3 }, { KA, 0.5f }, { KB, 0.3f }, { RVS, 0.15f } } },
        { "FM Vibes", "FM Klassiker", { FM_BASE, { KM, FMBELL }, { KV, 4 }, { KA, 0.4f }, { KB, 0.3f }, { KFX, FX_TREM }, { KFA, 0.4f }, { KFR, 0.4f }, { RVS, 0.22f } } },
        { "FM Solid Bass", "FM Klassiker", { FM_BASE, { KM, FMBASS }, { KV, 0 }, { KA, 0.45f }, { KB, 0.4f }, { KO, KO_M1 } } },
        { "FM Punch Bass", "FM Klassiker", { FM_BASE, { KM, FMBASS }, { KV, 1 }, { KA, 0.5f }, { KB, 0.5f }, { KO, KO_M1 } } },
        { "FM Pluck Bass", "FM Klassiker", { FM_BASE, { KM, FMBASS }, { KV, 2 }, { KA, 0.5f }, { KB, 0.4f }, { KO, KO_M1 } } },
        { "FM Growl Bass", "FM Klassiker", { FM_BASE, { KM, FMBASS }, { KV, 3 }, { KA, 0.55f }, { KB, 0.5f }, { KO, KO_M1 }, { KDRV, 0.2f } } },
#undef FM_BASE
#undef KEYS_ONLY

        // ============================================================ POLY LEGENDEN (im Stil der grossen 70er/80er Polysynths)
        { "Poly 5 Brass", "Poly Legenden", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.08f }, { L1, 0.8f }, { L2, 0.7f },
            { CUT, 700 }, { RES, 0.2f }, { ENV, 0.6f }, { FA, 0.05f }, { FD, 0.7f }, { FS, 0.45f },
            { AA, 0.02f }, { AD, 0.5f }, { AS, 0.85f }, { AR, 0.35f }, { DRF, 0.45f }, { RVS, 0.2f }, { VOL, -8.0f }, { MODE, POLY } } },
        { "Poly 5 Warm Pad", "Poly Legenden", {
            { W1, PLS }, { O1, R8 }, { PW, 0.35f }, { W2, SAW }, { O2, R8 }, { T2, 0.07f }, { L1, 0.7f }, { L2, 0.6f },
            { L2R, 0.3f }, { M1S, S_LFO2 }, { M1D, D_PW }, { M1A, 0.3f }, { CUT, 1500 }, { RES, 0.15f }, { ENV, 0.2f },
            { FA, 0.8f }, { FD, 1.5f }, { FS, 0.6f }, { AA, 0.7f }, { AS, 1.0f }, { AR, 1.6f }, { DRF, 0.5f }, { CHM, 0.2f }, { RVS, 0.3f }, { VOL, -8.0f }, { MODE, POLY } } },
        { "Poly 5 Sync Keys", "Poly Legenden", {
            { W1, WT }, { TB1, T_SYNC }, { P1, 0.2f }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { L1, 0.8f }, { L2, 0.3f },
            { M1S, S_FENV }, { M1D, D_POS1 }, { M1A, 0.5f }, { CUT, 2500 }, { ENV, 0.35f }, { FD, 0.4f }, { FS, 0.2f },
            { AA, 0.002f }, { AD, 0.8f }, { AS, 0.5f }, { AR, 0.3f }, { RVS, 0.15f }, { MODE, POLY } } },
        { "Eight Voice Brass", "Poly Legenden", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.12f }, { L1, 0.8f }, { L2, 0.75f }, { UNI, UNI2 }, { UDT, 0.2f },
            { FTY, LP12 }, { CUT, 1100 }, { RES, 0.1f }, { ENV, 0.5f }, { FA, 0.02f }, { FD, 0.4f }, { FS, 0.6f },
            { AA, 0.01f }, { AS, 0.9f }, { AR, 0.3f }, { DRV, 0.2f }, { CHM, 0.15f }, { RVS, 0.15f }, { VOL, -9.0f }, { MODE, POLY } } },
        { "Eight Voice Strings", "Poly Legenden", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.1f }, { L1, 0.75f }, { L2, 0.7f }, { UNI, UNI3 }, { UDT, 0.22f }, { USP, 0.9f },
            { FTY, LP12 }, { CUT, 2400 }, { ENV, 0.15f }, { FA, 0.6f }, { FD, 1.0f }, { FS, 0.7f },
            { AA, 0.5f }, { AS, 1.0f }, { AR, 1.4f }, { CHM, 0.45f }, { RVS, 0.3f }, { VOL, -10.0f }, { MODE, POLY } } },
        { "Eighties Rock Brass", "Poly Legenden", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.14f }, { W3, SAW }, { O3, R4 }, { L1, 0.8f }, { L2, 0.75f }, { L3, 0.25f },
            { UNI, UNI2 }, { UDT, 0.18f }, { FTY, LP12 }, { CUT, 1400 }, { RES, 0.12f }, { ENV, 0.55f }, { FA, 0.005f }, { FD, 0.35f }, { FS, 0.55f },
            { AA, 0.004f }, { AD, 0.4f }, { AS, 0.85f }, { AR, 0.25f }, { DRV, 0.3f }, { CHM, 0.2f }, { RVS, 0.15f }, { VOL, -10.0f }, { MODE, POLY } } },
        { "Super Poly Strings", "Poly Legenden", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.09f }, { L1, 0.75f }, { L2, 0.7f }, { UNI, UNI2 }, { UDT, 0.15f },
            { FTY, LP12 }, { CUT, 3000 }, { ENV, 0.1f }, { AA, 0.4f }, { AS, 1.0f }, { AR, 1.2f },
            { CHM, 0.55f }, { CHR, 0.3f }, { CHD, 0.6f }, { RVS, 0.3f }, { VOL, -9.0f }, { MODE, POLY } } },
        { "Super Poly Brass", "Poly Legenden", {
            { W1, SAW }, { O1, R8 }, { W2, SQR }, { O2, R8 }, { T2, 0.06f }, { L1, 0.8f }, { L2, 0.4f }, { UNI, UNI2 }, { UDT, 0.12f },
            { CUT, 900 }, { RES, 0.2f }, { ENV, 0.6f }, { FA, 0.08f }, { FD, 0.6f }, { FS, 0.5f },
            { AA, 0.05f }, { AS, 0.9f }, { AR, 0.35f }, { CHM, 0.3f }, { RVS, 0.2f }, { VOL, -8.0f }, { MODE, POLY } } },
        { "Ensemble Chorus Pad", "Poly Legenden", {
            { W1, PLS }, { O1, R8 }, { PW, 0.3f }, { L1, 0.85f }, { L2, 0.0f }, { SUB, 0.35f },
            { L2R, 0.6f }, { M1S, S_LFO2 }, { M1D, D_PW }, { M1A, 0.35f }, { CUT, 2200 }, { RES, 0.1f }, { ENV, 0.1f },
            { AA, 0.3f }, { AS, 1.0f }, { AR, 1.0f }, { CHM, 0.9f }, { CHR, 0.45f }, { CHD, 0.55f }, { RVS, 0.25f }, { MODE, POLY } } },
        { "Chorus Poly Stab", "Poly Legenden", {
            { W1, SQR }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { L1, 0.7f }, { L2, 0.5f }, { SUB, 0.4f },
            { CUT, 1300 }, { RES, 0.25f }, { ENV, 0.55f }, { FD, 0.25f }, { FS, 0.1f },
            { AA, 0.002f }, { AD, 0.35f }, { AS, 0.2f }, { AR, 0.2f }, { CHM, 0.8f }, { CHR, 0.6f }, { RVS, 0.12f }, { MODE, POLY } } },
        { "Chorus Bass", "Poly Legenden", {
            { W1, PLS }, { O1, R16 }, { PW, 0.4f }, { L1, 0.85f }, { L2, 0.0f }, { SUB, 0.6f },
            { CUT, 600 }, { RES, 0.2f }, { ENV, 0.45f }, { FD, 0.25f }, { FS, 0.2f }, { AS, 0.9f }, { AR, 0.1f },
            { CHM, 0.6f }, { CHR, 0.35f }, { MODE, MONO } } },

        // ============================================================ VINTAGE MONO (Ladder-Klassiker)
        
        { "Keller Classic Bass", "Vintage Mono", {
            { W1, SAW }, { O1, R16 }, { W2, SAW }, { O2, R16 }, { T2, 0.06f }, { W3, SQR }, { O3, R32 },
            { L1, 0.85f }, { L2, 0.7f }, { L3, 0.5f },
            { CUT, 380 }, { RES, 0.3f }, { ENV, 0.55f }, { DRV, 0.35f }, { KTR, 0.4f },
            { FA, 0.002f }, { FD, 0.32f }, { FS, 0.1f }, { FR, 0.15f },
            { AA, 0.002f }, { AD, 0.6f }, { AS, 0.85f }, { AR, 0.12f }, { MODE, MONO } } },

        { "Basement Sub", "Vintage Mono", {
            { W1, TRI }, { O1, R16 }, { W2, SQR }, { O2, R32 }, { L1, 1.0f }, { L2, 0.45f }, { L3, 0.0f },
            { CUT, 220 }, { RES, 0.1f }, { ENV, 0.25f }, { DRV, 0.4f }, { FD, 0.5f }, { FS, 0.3f },
            { AA, 0.003f }, { AS, 1.0f }, { AR, 0.1f }, { MODE, LEGATO } } },

        { "Pluck Bass", "Vintage Mono", {
            { W1, SAW }, { O1, R16 }, { W2, PLS }, { O2, R16 }, { PW, 0.3f }, { L1, 0.8f }, { L2, 0.7f },
            { CUT, 250 }, { RES, 0.45f }, { ENV, 0.7f }, { FA, 0.001f }, { FD, 0.18f }, { FS, 0.0f }, { FR, 0.1f },
            { AA, 0.001f }, { AD, 0.5f }, { AS, 0.0f }, { AR, 0.1f }, { VF, 0.5f }, { MODE, MONO } } },

        { "Acid Lane", "Vintage Mono", {
            { W1, SAW }, { O1, R16 }, { L1, 1.0f }, { L2, 0.0f },
            { CUT, 300 }, { RES, 0.78f }, { ENV, 0.65f }, { DRV, 0.55f }, { FD, 0.22f }, { FS, 0.0f },
            { AS, 0.9f }, { AR, 0.08f }, { GLD, 0.08f }, { VF, 0.6f }, { MODE, LEGATO } } },

        { "Funk Bass 73", "Vintage Mono", {
            { W1, SQR }, { O1, R16 }, { W2, SAW }, { O2, R16 }, { T2, -0.05f }, { L1, 0.8f }, { L2, 0.6f },
            { CUT, 500 }, { RES, 0.4f }, { ENV, 0.5f }, { DRV, 0.3f }, { FD, 0.15f }, { FS, 0.15f },
            { VF, 0.6f }, { VA, 0.5f }, { AD, 0.3f }, { AS, 0.6f }, { AR, 0.08f }, { MODE, MONO } } },

        { "Fat Three Bass", "Vintage Mono", {
            { W1, SAW }, { O1, R16 }, { W2, SAW }, { O2, R16 }, { T2, 0.08f }, { W3, SAW }, { O3, R16 }, { T3, -0.11f },
            { L1, 0.75f }, { L2, 0.7f }, { L3, 0.7f },
            { CUT, 600 }, { RES, 0.2f }, { ENV, 0.4f }, { DRV, 0.5f }, { FD, 0.45f }, { FS, 0.25f },
            { AS, 0.9f }, { AR, 0.12f }, { MODE, MONO } } },

        { "Rubber Bass", "Vintage Mono", {
            { W1, TRI }, { O1, R16 }, { W2, SQR }, { O2, R32 }, { L1, 0.9f }, { L2, 0.5f },
            { CUT, 300 }, { RES, 0.55f }, { ENV, 0.6f }, { FD, 0.4f }, { FS, 0.1f },
            { GLD, 0.05f }, { AS, 0.9f }, { AR, 0.1f }, { MODE, LEGATO } } },

        { "Deep Reese", "Vintage Mono", {
            { W1, SAW }, { O1, R16 }, { W2, SAW }, { O2, R16 }, { T2, 0.18f }, { W3, SAW }, { O3, R32 },
            { L1, 0.8f }, { L2, 0.8f }, { L3, 0.6f },
            { CUT, 450 }, { RES, 0.15f }, { ENV, 0.1f }, { LF, 0.1f }, { LR, 0.3f }, { DRV, 0.45f },
            { AS, 1.0f }, { AR, 0.2f }, { MODE, LEGATO } } },

        
        { "Gold Lead", "Vintage Mono", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.08f }, { W3, SQR }, { O3, R4 },
            { L1, 0.85f }, { L2, 0.75f }, { L3, 0.3f },
            { CUT, 1800 }, { RES, 0.35f }, { ENV, 0.4f }, { DRV, 0.3f }, { FD, 0.5f }, { FS, 0.4f },
            { GLD, 0.06f }, { LR, 5.5f }, { AA, 0.01f }, { AS, 0.9f }, { AR, 0.3f }, { MODE, LEGATO } } },

        { "Funk Lead 74", "Vintage Mono", {
            { W1, SQR }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.05f }, { L1, 0.8f }, { L2, 0.8f },
            { CUT, 1500 }, { RES, 0.5f }, { ENV, 0.55f }, { FD, 0.3f }, { FS, 0.3f }, { VF, 0.5f },
            { GLD, 0.04f }, { AS, 0.85f }, { AR, 0.2f }, { MODE, LEGATO } } },

        { "Solo Fifth", "Vintage Mono", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 7.0f }, { W3, SAW }, { O3, R4 }, { T3, 0.04f },
            { L1, 0.85f }, { L2, 0.6f }, { L3, 0.3f },
            { CUT, 2200 }, { RES, 0.3f }, { ENV, 0.35f }, { FD, 0.6f }, { FS, 0.5f },
            { GLD, 0.05f }, { AS, 0.9f }, { AR, 0.25f }, { MODE, LEGATO } } },

        { "Whistle Lead", "Vintage Mono", {
            { W1, TRI }, { O1, R8 }, { W2, TRI }, { O2, R4 }, { L1, 0.9f }, { L2, 0.4f },
            { CUT, 3000 }, { RES, 0.2f }, { ENV, 0.1f }, { LP, 0.18f }, { LR, 5.8f },
            { GLD, 0.1f }, { AA, 0.03f }, { AS, 1.0f }, { AR, 0.35f }, { MODE, LEGATO } } },

        { "Screamer", "Vintage Mono", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, -0.09f }, { L1, 0.9f }, { L2, 0.8f },
            { CUT, 900 }, { RES, 0.85f }, { ENV, 0.6f }, { DRV, 0.8f }, { FD, 0.8f }, { FS, 0.5f },
            { GLD, 0.05f }, { AS, 1.0f }, { AR, 0.2f }, { MODE, LEGATO } } },

        { "Portamento Lead", "Vintage Mono", {
            { W1, SQR }, { O1, R8 }, { W2, PLS }, { O2, R8 }, { PW, 0.15f }, { L1, 0.75f }, { L2, 0.75f },
            { CUT, 1400 }, { RES, 0.4f }, { ENV, 0.35f }, { FD, 0.5f }, { FS, 0.45f },
            { GLD, 0.25f }, { AS, 1.0f }, { AR, 0.3f }, { MODE, LEGATO } } },

        { "Hollow Square", "Vintage Mono", {
            { W1, SQR }, { O1, R8 }, { W2, SQR }, { O2, R4 }, { L1, 0.9f }, { L2, 0.3f },
            { CUT, 2500 }, { RES, 0.15f }, { ENV, 0.2f }, { AS, 0.95f }, { AR, 0.2f }, { MODE, MONO } } },

        { "Nasal Bite", "Vintage Mono", {
            { W1, SAW }, { O1, R8 }, { W2, PLS }, { O2, R4 }, { PW, 0.08f }, { L1, 0.8f }, { L2, 0.6f },
            { CUT, 700 }, { RES, 0.6f }, { ENV, 0.7f }, { FD, 0.25f }, { FS, 0.35f }, { DRV, 0.4f },
            { AS, 0.9f }, { AR, 0.15f }, { MODE, MONO } } },


        // ============================================================ ANALOG POLY (warm, driftend, Chorus)
        { "Brass Stab", "Analog Poly", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.1f }, { W3, SAW }, { O3, R16 },
            { L1, 0.8f }, { L2, 0.7f }, { L3, 0.4f },
            { CUT, 500 }, { RES, 0.2f }, { ENV, 0.6f }, { FA, 0.06f }, { FD, 0.4f }, { FS, 0.45f }, { FR, 0.2f },
            { AA, 0.02f }, { AD, 0.5f }, { AS, 0.7f }, { AR, 0.2f }, { VF, 0.5f }, { MODE, POLY } } },

        { "Soul Brass", "Analog Poly", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, -0.08f }, { L1, 0.8f }, { L2, 0.8f },
            { CUT, 700 }, { RES, 0.15f }, { ENV, 0.5f }, { FA, 0.12f }, { FD, 0.6f }, { FS, 0.55f }, { FR, 0.3f },
            { AA, 0.06f }, { AS, 0.85f }, { AR, 0.3f }, { VF, 0.4f }, { LP, 0.08f }, { LR, 5.0f }, { MODE, POLY } } },

        { "Synth Horns", "Analog Poly", {
            { W1, SAW }, { O1, R8 }, { W2, PLS }, { O2, R8 }, { PW, 0.35f }, { T2, 0.07f }, { W3, SAW }, { O3, R4 },
            { L1, 0.8f }, { L2, 0.6f }, { L3, 0.25f },
            { CUT, 650 }, { RES, 0.25f }, { ENV, 0.55f }, { FA, 0.08f }, { FD, 0.35f }, { FS, 0.4f },
            { AA, 0.03f }, { AS, 0.8f }, { AR, 0.25f }, { VF, 0.5f }, { MODE, POLY } } },


        { "Velvet Strings", "Analog Poly", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.14f }, { W3, SAW }, { O3, R4 }, { T3, -0.09f },
            { L1, 0.7f }, { L2, 0.7f }, { L3, 0.25f }, { FTY, LP12 },
            { CUT, 2600 }, { RES, 0.1f }, { ENV, 0.15f }, { FA, 0.6f }, { FD, 1.2f }, { FS, 0.7f },
            { AA, 0.45f }, { AS, 0.9f }, { AR, 1.4f }, { DRF, 0.6f }, { CHM, 0.5f }, { RVS, 0.25f }, { VA, 0.2f }, { VOL, -9.0f }, { MODE, POLY } } },

        { "Silk PWM Pad", "Analog Poly", {
            { W1, PLS }, { O1, R8 }, { W2, PLS }, { O2, R8 }, { T2, 0.1f }, { PW, 0.3f }, { L1, 0.75f }, { L2, 0.65f }, { FTY, LP12 },
            { CUT, 1800 }, { RES, 0.15f }, { ENV, 0.2f }, { FA, 0.8f }, { FD, 2.0f }, { FS, 0.6f },
            { AA, 0.7f }, { AS, 0.95f }, { AR, 1.8f }, { L2R, 0.35f }, { M1S, S_LFO2 }, { M1D, D_PW }, { M1A, 0.45f },
            { CHM, 0.7f }, { CHR, 0.3f }, { CHD, 0.6f }, { RVS, 0.3f }, { DRF, 0.5f }, { VOL, -9.0f }, { MODE, POLY } } },

        { "Neon Rain Brass", "Analog Poly", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.16f }, { L1, 0.8f }, { L2, 0.75f },
            { CUT, 700 }, { RES, 0.2f }, { ENV, 0.5f }, { FA, 0.45f }, { FD, 1.5f }, { FS, 0.45f }, { FR, 1.5f },
            { AA, 0.25f }, { AS, 0.9f }, { AR, 2.2f }, { M1S, S_WHEEL }, { M1D, D_CUT }, { M1A, 0.5f },
            { CHM, 0.4f }, { DLM, 0.22f }, { DLT, D_1_4 }, { RVS, 0.45f }, { RVZ, 0.85f }, { DRF, 0.6f }, { VOL, -9.0f }, { MODE, POLY } } },

        { "Dream Keys", "Analog Poly", {
            { W1, SQR }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.08f }, { L1, 0.7f }, { L2, 0.55f }, { FTY, LP12 },
            { CUT, 1400 }, { RES, 0.15f }, { ENV, 0.35f }, { FD, 1.2f }, { FS, 0.25f },
            { AD, 1.6f }, { AS, 0.35f }, { AR, 0.6f }, { VF, 0.5f }, { VA, 0.5f },
            { CHM, 0.45f }, { DLM, 0.18f }, { DLT, D_1_8D }, { RVS, 0.2f }, { MODE, POLY } } },

        { "Analog Pluck", "Analog Poly", {
            { W1, SAW }, { O1, R8 }, { W2, SQR }, { O2, R4 }, { L1, 0.8f }, { L2, 0.4f },
            { CUT, 500 }, { RES, 0.3f }, { ENV, 0.7f }, { FA, 0.001f }, { FD, 0.25f }, { FS, 0.0f }, { FR, 0.2f },
            { AA, 0.001f }, { AD, 0.6f }, { AS, 0.0f }, { AR, 0.3f }, { VF, 0.5f },
            { DLM, 0.28f }, { DLT, D_1_8D }, { DLF, 0.4f }, { RVS, 0.2f }, { MODE, POLY } } },

        { "Funk Poly Stab", "Analog Poly", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.07f }, { L1, 0.8f }, { L2, 0.7f },
            { CUT, 900 }, { RES, 0.35f }, { ENV, 0.5f }, { FD, 0.18f }, { FS, 0.1f },
            { AA, 0.002f }, { AD, 0.35f }, { AS, 0.0f }, { AR, 0.15f }, { VF, 0.6f }, { CHM, 0.25f }, { MODE, POLY } } },

        { "Drift Poly", "Analog Poly", {
            { W1, SAW }, { O1, R8 }, { W2, TRI }, { O2, R4 }, { L1, 0.8f }, { L2, 0.4f }, { UNI, UNI2 }, { UDT, 0.2f },
            { CUT, 1600 }, { RES, 0.2f }, { ENV, 0.3f }, { FD, 0.8f }, { FS, 0.4f },
            { AA, 0.02f }, { AS, 0.8f }, { AR, 0.7f }, { DRF, 0.95f }, { CHM, 0.35f }, { MODE, POLY } } },

        { "Fat Unison Poly", "Analog Poly", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R16 }, { L1, 0.8f }, { L2, 0.5f }, { UNI, UNI3 }, { UDT, 0.28f }, { USP, 0.8f },
            { CUT, 1100 }, { RES, 0.15f }, { ENV, 0.4f }, { FD, 0.6f }, { FS, 0.4f },
            { AS, 0.85f }, { AR, 0.4f }, { CHM, 0.2f }, { HEAT, 0.2f }, { MODE, POLY } } },

        { "Slow Sweep Pad", "Analog Poly", {
            { W1, SAW }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.11f }, { L1, 0.7f }, { L2, 0.7f },
            { CUT, 450 }, { RES, 0.45f }, { ENV, 0.1f }, { L2R, 0.06f }, { M1S, S_LFO2 }, { M1D, D_CUT }, { M1A, 0.45f },
            { AA, 1.2f }, { AS, 1.0f }, { AR, 2.5f }, { CHM, 0.5f }, { RVS, 0.4f }, { DRF, 0.6f }, { MODE, POLY } } },

        { "Soft Analog Bells", "Analog Poly", {
            { W1, TRI }, { O1, R8 }, { W2, SQR }, { O2, R2 }, { T2, 0.02f }, { L1, 0.8f }, { L2, 0.3f },
            { CUT, 3000 }, { RES, 0.1f }, { ENV, 0.3f }, { FD, 0.9f }, { FS, 0.1f },
            { AA, 0.002f }, { AD, 2.0f }, { AS, 0.0f }, { AR, 1.2f }, { CHM, 0.4f }, { DLM, 0.2f }, { RVS, 0.35f }, { MODE, POLY } } },

        { "Warm Choir Pad", "Analog Poly", {
            { W1, TRI }, { O1, R8 }, { W2, SAW }, { O2, R4 }, { T2, 0.05f }, { L1, 0.85f }, { L2, 0.25f }, { FTY, BPF },
            { CUT, 1100 }, { RES, 0.35f }, { ENV, 0.0f }, { LF, 0.12f }, { LR, 0.2f }, { LP, 0.05f },
            { AA, 0.9f }, { AS, 1.0f }, { AR, 2.0f }, { CHM, 0.6f }, { RVS, 0.45f }, { VOL, -9.0f }, { MODE, POLY } } },

        { "Glass Poly", "Analog Poly", {
            { W1, PLS }, { O1, R8 }, { PW, 0.1f }, { W2, TRI }, { O2, R4 }, { L1, 0.7f }, { L2, 0.5f }, { FTY, HPF },
            { CUT, 300 }, { RES, 0.3f }, { ENV, 0.2f }, { AA, 0.01f }, { AD, 1.0f }, { AS, 0.4f }, { AR, 0.8f },
            { CHM, 0.5f }, { DLM, 0.25f }, { DLT, D_1_8D }, { RVS, 0.35f }, { MODE, POLY } } },

        // ============================================================ WAVETABLE (moderne Klaenge)
        { "Super Saw Wave", "Wavetable", {
            { W1, WT }, { TB1, T_BASIC }, { P1, 0.667f }, { O1, R8 }, { L1, 0.9f }, { L2, 0.0f },
            { UNI, UNI7 }, { UDT, 0.38f }, { USP, 0.95f }, { FTY, LP12 }, { CUT, 7000 }, { RES, 0.1f }, { ENV, 0.1f },
            { AA, 0.005f }, { AS, 0.9f }, { AR, 0.5f }, { RVS, 0.25f }, { VOL, -9.0f }, { MODE, POLY } } },

        { "Growl Bass", "Wavetable", {
            { W1, WT }, { TB1, T_GROWL }, { P1, 0.35f }, { O1, R16 }, { L1, 0.9f }, { L2, 0.0f }, { SUB, 0.55f },
            { LR, 2.0f }, { M1S, S_LFO1 }, { M1D, D_POS1 }, { M1A, 0.45f },
            { CUT, 2200 }, { RES, 0.25f }, { ENV, 0.2f }, { DRV, 0.5f }, { HEAT, 0.35f },
            { AS, 1.0f }, { AR, 0.1f }, { GLD, 0.03f }, { MODE, MONO } } },

        { "Wobble Bass", "Wavetable", {
            { W1, WT }, { TB1, T_GROWL }, { P1, 0.2f }, { O1, R16 }, { L1, 0.9f }, { L2, 0.0f }, { SUB, 0.6f },
            { LR, 4.0f }, { LW, 0 }, { M1S, S_LFO1 }, { M1D, D_POS1 }, { M1A, 0.6f }, { M2S, S_LFO1 }, { M2D, D_CUT }, { M2A, 0.45f },
            { CUT, 900 }, { RES, 0.4f }, { ENV, 0.0f }, { DRV, 0.55f }, { HEAT, 0.4f },
            { AS, 1.0f }, { AR, 0.08f }, { MODE, MONO } } },

        { "Neuro Wheel Bass", "Wavetable", {
            { W1, WT }, { TB1, T_GROWL }, { P1, 0.0f }, { O1, R16 }, { W2, WT }, { TB2, T_SYNC }, { O2, R16 }, { L1, 0.8f }, { L2, 0.45f },
            { M1S, S_WHEEL }, { M1D, D_POSALL }, { M1A, 1.0f }, { MWV, 0.0f }, { SUB, 0.5f },
            { CUT, 3000 }, { RES, 0.2f }, { DRV, 0.6f }, { HEAT, 0.45f }, { AS, 1.0f }, { AR, 0.1f }, { MODE, MONO } } },

        { "Reese Wave", "Wavetable", {
            { W1, WT }, { TB1, T_BASIC }, { P1, 0.66f }, { O1, R16 }, { W2, WT }, { TB2, T_BASIC }, { P2, 0.7f }, { O2, R16 }, { T2, 0.15f },
            { L1, 0.8f }, { L2, 0.8f }, { UNI, UNI2 }, { UDT, 0.2f }, { L2R, 0.15f }, { M1S, S_LFO2 }, { M1D, D_CUT }, { M1A, 0.25f },
            { CUT, 600 }, { RES, 0.15f }, { ENV, 0.1f }, { DRV, 0.45f }, { AS, 1.0f }, { AR, 0.15f }, { MODE, LEGATO } } },

        { "Vowel Talker", "Wavetable", {
            { W1, WT }, { TB1, T_VOCAL }, { P1, 0.3f }, { O1, R8 }, { L1, 0.9f }, { L2, 0.0f }, { UNI, UNI2 }, { UDT, 0.12f },
            { L2R, 0.45f }, { M1S, S_LFO2 }, { M1D, D_POS1 }, { M1A, 0.5f }, { FTY, LP12 }, { CUT, 5000 }, { ENV, 0.0f },
            { GLD, 0.06f }, { AS, 1.0f }, { AR, 0.3f }, { LP, 0.06f }, { DLM, 0.2f }, { MODE, LEGATO } } },

        { "FM Pluck", "Wavetable", {
            { W1, WT }, { TB1, T_FM }, { P1, 0.2f }, { O1, R8 }, { L1, 0.9f }, { L2, 0.0f },
            { M1S, S_FENV }, { M1D, D_POS1 }, { M1A, 0.7f }, { FA, 0.001f }, { FD, 0.3f }, { FS, 0.0f },
            { CUT, 6000 }, { ENV, 0.2f }, { AA, 0.001f }, { AD, 0.8f }, { AS, 0.0f }, { AR, 0.4f },
            { DLM, 0.3f }, { DLT, D_1_8D }, { RVS, 0.25f }, { MODE, POLY } } },

        { "Sync Lead", "Wavetable", {
            { W1, WT }, { TB1, T_SYNC }, { P1, 0.1f }, { O1, R8 }, { W2, SAW }, { O2, R8 }, { T2, 0.08f }, { L1, 0.85f }, { L2, 0.35f },
            { M1S, S_FENV }, { M1D, D_POS1 }, { M1A, 0.7f }, { M2S, S_WHEEL }, { M2D, D_POS1 }, { M2A, 0.4f },
            { FA, 0.001f }, { FD, 0.6f }, { FS, 0.25f }, { CUT, 4000 }, { RES, 0.2f }, { ENV, 0.15f },
            { GLD, 0.05f }, { AS, 0.95f }, { AR, 0.25f }, { DLM, 0.18f }, { MODE, LEGATO } } },

        { "Harmonic Pad", "Wavetable", {
            { W1, WT }, { TB1, T_HARM }, { P1, 0.15f }, { O1, R8 }, { W2, WT }, { TB2, T_HARM }, { P2, 0.4f }, { O2, R4 }, { T2, 0.06f },
            { L1, 0.75f }, { L2, 0.35f }, { L2R, 0.07f }, { M1S, S_LFO2 }, { M1D, D_POSALL }, { M1A, 0.5f },
            { UNI, UNI3 }, { UDT, 0.18f }, { FTY, LP12 }, { CUT, 5000 }, { ENV, 0.0f },
            { AA, 1.4f }, { AS, 1.0f }, { AR, 2.8f }, { CHM, 0.35f }, { RVS, 0.5f }, { RVZ, 0.9f }, { MODE, POLY } } },

        { "Air Vox Pad", "Wavetable", {
            { W1, WT }, { TB1, T_VOCAL }, { P1, 0.6f }, { O1, R8 }, { L1, 0.8f }, { L2, 0.0f }, { NOI, 0.08f },
            { UNI, UNI4 }, { UDT, 0.2f }, { USP, 0.9f }, { L2R, 0.05f }, { M1S, S_LFO2 }, { M1D, D_POS1 }, { M1A, 0.4f },
            { FTY, LP12 }, { CUT, 3500 }, { ENV, 0.0f }, { AA, 1.0f }, { AS, 1.0f }, { AR, 2.5f }, { RVS, 0.55f }, { VOL, -9.0f }, { MODE, POLY } } },

        { "Organ Wave", "Wavetable", {
            { W1, WT }, { TB1, T_ORGAN }, { P1, 0.75f }, { O1, R8 }, { L1, 0.9f }, { L2, 0.0f }, { CUT, 12000 }, { ENV, 0.0f },
            { AA, 0.004f }, { AS, 1.0f }, { AR, 0.08f }, { CHM, 0.6f }, { CHR, 0.75f }, { CHD, 0.4f }, { RVS, 0.2f }, { MODE, POLY } } },

        { "Bell Keys", "Wavetable", {
            { W1, WT }, { TB1, T_BELL }, { P1, 0.35f }, { O1, R8 }, { W2, WT }, { TB2, T_BASIC }, { P2, 0.0f }, { O2, R8 },
            { L1, 0.75f }, { L2, 0.4f }, { M1S, S_AENV }, { M1D, D_POS1 }, { M1A, 0.4f }, { CUT, 9000 }, { ENV, 0.0f },
            { AA, 0.001f }, { AD, 2.2f }, { AS, 0.0f }, { AR, 1.2f }, { VA, 0.6f }, { RVS, 0.35f }, { DLM, 0.15f }, { MODE, POLY } } },

        { "PWM Wave Strings", "Wavetable", {
            { W1, WT }, { TB1, T_PWM }, { P1, 0.3f }, { O1, R8 }, { W2, WT }, { TB2, T_PWM }, { P2, 0.6f }, { O2, R8 }, { T2, 0.1f },
            { L1, 0.75f }, { L2, 0.65f }, { L2R, 0.25f }, { M1S, S_LFO2 }, { M1D, D_POSALL }, { M1A, 0.35f },
            { FTY, LP12 }, { CUT, 3000 }, { ENV, 0.1f }, { AA, 0.5f }, { AS, 0.9f }, { AR, 1.5f }, { CHM, 0.5f }, { RVS, 0.3f }, { MODE, POLY } } },

        { "Digital Arp", "Wavetable", {
            { W1, WT }, { TB1, T_FM }, { P1, 0.45f }, { O1, R8 }, { W2, WT }, { TB2, T_SYNC }, { P2, 0.5f }, { O2, R4 },
            { L1, 0.8f }, { L2, 0.3f }, { M1S, S_VEL }, { M1D, D_POS1 }, { M1A, 0.4f }, { CUT, 8000 }, { ENV, 0.0f },
            { AA, 0.001f }, { AD, 0.25f }, { AS, 0.0f }, { AR, 0.2f }, { DLM, 0.3f }, { DLT, D_1_16 }, { DLF, 0.45f }, { MODE, POLY } } },

        { "Hyper Pluck", "Wavetable", {
            { W1, WT }, { TB1, T_BASIC }, { P1, 0.667f }, { O1, R8 }, { L1, 0.9f }, { L2, 0.0f }, { UNI, UNI5 }, { UDT, 0.3f }, { USP, 0.9f },
            { CUT, 800 }, { RES, 0.2f }, { ENV, 0.65f }, { FD, 0.22f }, { FS, 0.0f }, { FTY, LP12 },
            { AA, 0.001f }, { AD, 0.5f }, { AS, 0.0f }, { AR, 0.3f }, { DLM, 0.25f }, { DLT, D_1_8D }, { RVS, 0.3f }, { MODE, POLY } } },

        { "Morph Lead", "Wavetable", {
            { W1, WT }, { TB1, T_BASIC }, { P1, 0.0f }, { O1, R8 }, { W2, WT }, { TB2, T_BASIC }, { P2, 0.3f }, { O2, R8 }, { T2, 0.07f },
            { L1, 0.85f }, { L2, 0.6f }, { M1S, S_WHEEL }, { M1D, D_POSALL }, { M1A, 1.0f }, { MWV, 0.0f }, { M2S, S_AT }, { M2D, D_CUT }, { M2A, 0.4f },
            { CUT, 3500 }, { RES, 0.25f }, { ENV, 0.2f }, { GLD, 0.05f }, { AS, 1.0f }, { AR, 0.3f }, { LP, 0.05f }, { MODE, LEGATO } } },

        // ============================================================ SAMPLE-VORLAGEN (eigenes Sample laden)
        { "Sample chromatisch", "Sample", {
            { W1, SMP }, { L1, 1.0f }, { L2, 0.0f }, { CUT, 20000 }, { RES, 0.0f }, { ENV, 0.0f }, { DRV, 0.0f }, { KTR, 0.0f },
            { AA, 0.002f }, { AS, 1.0f }, { AR, 0.35f }, { VOL, -3.0f }, { MODE, POLY } } },

        { "Sample Loop-Player", "Sample", {
            { W1, SMP }, { L1, 1.0f }, { L2, 0.0f }, { SKT, 0.0f }, { SLP, 1.0f }, { CUT, 20000 }, { RES, 0.0f }, { ENV, 0.0f }, { DRV, 0.0f }, { KTR, 0.0f },
            { AA, 0.005f }, { AS, 1.0f }, { AR, 0.1f }, { VOL, -3.0f }, { MODE, POLY } } },

        { "Sample One-Shot", "Sample", {
            { W1, SMP }, { L1, 1.0f }, { L2, 0.0f }, { SKT, 0.0f }, { SOS, 1.0f }, { CUT, 20000 }, { RES, 0.0f }, { ENV, 0.0f }, { DRV, 0.0f }, { KTR, 0.0f },
            { AA, 0.001f }, { AS, 1.0f }, { AR, 0.05f }, { VOL, -3.0f }, { MODE, POLY } } },

        { "Sample durch Ladder", "Sample", {
            { W1, SMP }, { L1, 0.9f }, { W2, SAW }, { O2, R16 }, { L2, 0.35f }, { CUT, 1200 }, { RES, 0.35f }, { ENV, 0.45f },
            { FD, 0.6f }, { FS, 0.3f }, { AA, 0.003f }, { AS, 0.9f }, { AR, 0.4f }, { MODE, POLY } } },
    };
    return presets;
}

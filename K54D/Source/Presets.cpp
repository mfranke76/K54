#include "Presets.h"
#include "Params.h"

// Schreibweise der Patterns:  X=127  x=100  o=72  -=42 (Ghost)  _=22  .=aus  1..9=Stufen
//                             r=Wirbel 2x  R=Wirbel 3x  q=Wirbel 4x
// Spuren:  K Kick  S Snare  C Clap  R Rim  H Closed Hat  O Open Hat  L/M/T Toms  Y Crash  D Ride  P Perc

static const FactoryPreset factoryList[] =
{
    // ---------------------------------------------------------------- Classic Machines
    { "Classic Machines", "808 Classic",
      "K=808_Boom S=808 C=808 R=808_Rim H=808 O=808 L=808 M=808 T=808 Y=909 D=909 P=Cowbell Kd=.55",
      "K:x.....x.x....... S:....x.......x... H:x.o.x.o.x.o.x.o. O:..............o. P:..........o.....",
      16, 1, 0.0f, 0.35f, 0.25f, 0.4f },
    { "Classic Machines", "808 Boom Bass",
      "K=808_Boom S=808 C=808 R=808_Rim H=808 O=808 L=808 M=808 T=808 Y=909 D=909 P=Cowbell Kd=.8 Kv=.3",
      "K:x......x..x..... S:....x.......x... C:............x... H:x.x.x.x.x.x.x.x. O:......o.......o.",
      16, 1, 0.1f, 0.45f, 0.3f, 0.35f },
    { "Classic Machines", "808 Electro Funk",
      "K=808_Short S=808 C=808 R=808_Rim H=808 O=808 L=808 M=808 T=808 Y=909 D=909 P=Cowbell Sl=.6 Cl=.62",
      "K:x.....x...x..x.. S:....x.......x... C:....x.......x..x H:xoxoxoxoxoxoxoxo P:......x.......x. L:..............o.",
      16, 1, 0.08f, 0.4f, 0.3f, 0.3f },
    { "Classic Machines", "909 Classic",
      "K=909 S=909 C=909 R=909_Rim H=909 O=909 L=909 M=909 T=909 Y=909 D=909 P=Shaker Sl=.6 Cl=.62",
      "K:x...x...x...x... S:....x.......x... C:....x.......x... H:xo.oxo.oxo.oxo.o O:..x...x...x...x.",
      16, 1, 0.0f, 0.4f, 0.35f, 0.3f },
    { "Classic Machines", "909 Shuffle",
      "K=909 S=909 C=909 R=909_Rim H=909 O=909 L=909 M=909 T=909 Y=909 D=909 P=Shaker",
      "K:x...x...x...x... C:....x.......x... R:...x.....x..x... H:xoxoxoxoxoxoxoxo O:..x...x...x...x.",
      16, 1, 0.45f, 0.4f, 0.35f, 0.25f },
    { "Classic Machines", "Rhythm Box 78",
      "K=808_Short S=808 R=Clave H=808 O=808 P=Shaker L=Electronic M=Electronic T=Electronic Kl=.7 Hl=.55",
      "K:x.......x.x..... R:...x..x....x.... H:x.x.x.x.x.x.x.x. P:x-o-x-o-x-o-x-o-",
      16, 1, 0.0f, 0.25f, 0.2f, 0.35f },
    { "Classic Machines", "Linn Style 80s",
      "K=Linn S=Gated_80s C=909 R=Sidestick H=Linn O=Linn L=Electronic M=Electronic T=Electronic Y=Acoustic D=Acoustic P=Tambourine Sr=.35",
      "K:x.....x.x.x..... S:....x.......x... H:x.x.x.x.x.x.x.x. P:....x.......x...",
      16, 1, 0.0f, 0.4f, 0.4f, 0.6f },

    // ---------------------------------------------------------------- Hip-Hop & Trap
    { "Hip-Hop & Trap", "Boom Bap",
      "K=Linn S=Acoustic C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Shaker Kv=.45 Sv=.35 Sr=.15",
      "K:x......x..x..... S:....x.......x... H:x.o.x.o.x.o.x.o. O:..............o.",
      16, 1, 0.35f, 0.5f, 0.45f, 0.25f },
    { "Hip-Hop & Trap", "Trap 140",
      "K=Trap_Sub S=Trap C=Big R=808_Rim H=Trap O=808 L=808 M=808 T=808 Y=909 D=909 P=Cowbell Kd=.6",
      "K:x.........x..x.. S:........x....... C:........x....... H:x.x.x.x.x.xrx.q. O:..............o.",
      16, 1, 0.0f, 0.5f, 0.35f, 0.2f },
    { "Hip-Hop & Trap", "Trap Rolls",
      "K=Trap_Sub S=Trap C=Big R=808_Rim H=Trap O=808 L=808 M=808 T=808 Y=909 D=909 P=Cowbell Kd=.7 Kt=-2",
      "K:x.....x...x......x.....x..x..... S:........x...............x....... C:........x...............x....... H:x.x.xrx.x.x.x.qqx.x.xrx.x.RRx.x.",
      32, 1, 0.0f, 0.5f, 0.35f, 0.2f },
    { "Hip-Hop & Trap", "Lo-Fi Hip-Hop",
      "K=Lo-Fi S=Lo-Fi C=808 R=Sidestick H=Linn O=Linn L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Shaker Hl=.5 So=-.3",
      "K:x.....x...x...-. S:....x.......x... H:x.o.x.o.x.o.x.o. R:.......-......-.",
      16, 1, 0.5f, 0.5f, 0.5f, 0.3f },
    { "Hip-Hop & Trap", "West Coast",
      "K=808_Short S=Linn C=909 R=808_Rim H=808 O=808 L=808 M=808 T=808 Y=909 D=909 P=Cowbell Sl=.6 Cl=.62",
      "K:x..x...xx.x..... S:....x.......x... C:....x.......x... H:x.x.x.x.x.x.x.x.",
      16, 1, 0.2f, 0.4f, 0.35f, 0.2f },
    { "Hip-Hop & Trap", "Drill",
      "K=Trap_Sub S=Trap C=Snap R=808_Rim H=Trap O=808 L=808 M=808 T=808 Y=909 D=909 P=Cowbell Kd=.65",
      "K:x.......x..x.... S:....x.......x... C:....x.......x... H:x..x..x.x..x..xr P:...........o....",
      16, 1, 0.0f, 0.5f, 0.35f, 0.2f },
    { "Hip-Hop & Trap", "R&B Slow Jam",
      "K=808_Short S=808 C=Snap R=808_Rim H=808 O=808 L=808 M=808 T=808 Y=909 D=909 P=Shaker Cr=.3",
      "K:x.....x...x..x.. C:....x.......x... R:..........-..... H:x.x.x.x.x.x.x.x. P:x-o-x-o-x-o-x-o-",
      16, 1, 0.3f, 0.35f, 0.3f, 0.4f },

    // ---------------------------------------------------------------- House
    { "House", "Classic House",
      "K=909 S=909 C=909 R=909_Rim H=909 O=909 L=909 M=909 T=909 Y=909 D=909 P=Shaker",
      "K:x...x...x...x... C:....x.......x... H:x..xx..xx..xx..x O:..x...x...x...x.",
      16, 1, 0.1f, 0.4f, 0.4f, 0.25f },
    { "House", "Deep House",
      "K=909 S=909 C=909 R=909_Rim H=909 O=909 L=909 M=909 T=909 Y=909 D=909 P=Shaker Kd=.4 Ko=-.3 Cr=.3 Hl=.5",
      "K:x...x...x...x... C:....x.......x... R:...-......-..-.. H:.x-x.x-x.x-x.x-x O:..x...x...x...x.",
      16, 1, 0.25f, 0.35f, 0.35f, 0.4f },
    { "House", "Tech House",
      "K=Techno S=909 C=909 R=909_Rim H=909 O=909 L=909 M=909 T=909 Y=909 D=909 P=Shaker Kd=.35",
      "K:x...x...x...x... C:....x.......x... R:...x.....x...x.. H:xoxoxoxoxoxoxoxo O:..x...x...x...x. P:x-o-x-o-x-o-x-o-",
      16, 1, 0.15f, 0.5f, 0.45f, 0.2f },
    { "House", "Garage 2-Step",
      "K=909 S=909 C=909 R=909_Rim H=909 O=909 L=909 M=909 T=909 Y=909 D=909 P=Shaker",
      "K:x......x..x..... S:....x.......x... R:.......-......-. H:x.x.x.x.x.x.x.x. O:..............o.",
      16, 1, 0.5f, 0.4f, 0.35f, 0.25f },
    { "House", "Piano House",
      "K=909 S=909 C=Big R=909_Rim H=909 O=909 L=909 M=909 T=909 Y=909 D=909 P=Tambourine",
      "K:x...x...x...x... C:....x.......x... O:..x...x...x...x. H:x.x.x.x.x.x.x.x. P:....x.......x...",
      16, 1, 0.1f, 0.45f, 0.45f, 0.35f },

    // ---------------------------------------------------------------- Techno & Electronic
    { "Techno & Electronic", "Techno",
      "K=Techno S=909 C=909 R=909_Rim H=909 O=909 L=909 M=909 T=909 Y=909 D=909 P=Shaker Kd=.6 Kv=.3",
      "K:x...x...x...x... C:....x.......x... H:xoxoxoxoxoxoxoxo O:..x...x...x...x. D:o.o.o.o.o.o.o.o.",
      16, 1, 0.0f, 0.6f, 0.5f, 0.3f },
    { "Techno & Electronic", "Electro",
      "K=808_Boom S=808 C=808 R=808_Rim H=808 O=808 L=808 M=808 T=808 Y=909 D=909 P=Cowbell Kd=.45 Sl=.6 Cl=.62",
      "K:x.....x..x...... S:....x.......x... C:....x.......x... H:x.x.x.x.x.x.x.x. P:x.....x.....x...",
      16, 1, 0.0f, 0.4f, 0.35f, 0.25f },
    { "Techno & Electronic", "Drum & Bass",
      "K=909 S=909 C=909 R=909_Rim H=909 O=909 L=909 M=909 T=909 Y=909 D=909 P=Shaker Kd=.3 So=.4",
      "K:x.........x..... S:....x..-.-..x... H:x.x.x.x.x.x.x.x. D:..o...o...o...o.",
      16, 1, 0.0f, 0.5f, 0.45f, 0.2f },
    { "Techno & Electronic", "Breakbeat",
      "K=Acoustic S=Acoustic C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Shaker Kv=.4 Sv=.4",
      "K:x.........xx.... S:....x..-.x.-x..- H:x.x.x.x.x.x.x.x. O:..............o.",
      16, 1, 0.12f, 0.55f, 0.5f, 0.25f },
    { "Techno & Electronic", "Synthwave",
      "K=Linn S=Gated_80s C=909 R=Sidestick H=Linn O=Linn L=Electronic M=Electronic T=Electronic Y=Acoustic D=Acoustic P=Tambourine Sr=.4",
      "K:x...x...x...x... S:....x.......x... H:x.x.x.x.x.x.x.x. P:..x...x...x...x.",
      16, 1, 0.0f, 0.4f, 0.4f, 0.65f },
    { "Techno & Electronic", "EDM Big Room",
      "K=Techno S=909 C=Big R=909_Rim H=909 O=909 L=909 M=909 T=909 Y=909 D=909 P=Shaker Kv=.5 Kd=.45 Sl=.6 Cl=.62",
      "K:x...x...x...x... S:....x.......x... C:....x.......x... O:..x...x...x...x. H:x.x.x.x.x.x.x.x.",
      16, 1, 0.0f, 0.6f, 0.6f, 0.35f },

    // ---------------------------------------------------------------- Disco & Funk
    { "Disco & Funk", "Disco 70s",
      "K=Disco S=Disco C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Tambourine",
      "K:x...x...x...x... S:....x.......x... H:x.x.x.x.x.x.x.x. O:..x...x...x...x. P:....x.......x...",
      16, 1, 0.0f, 0.35f, 0.35f, 0.3f },
    { "Disco & Funk", "Nu Disco",
      "K=909 S=Disco C=909 R=909_Rim H=909 O=909 L=Electronic M=Electronic T=Electronic Y=909 D=909 P=Shaker Sl=.6 Cl=.62",
      "K:x...x...x...x... S:....x.......x... C:....x.......x... H:xoxoxoxoxoxoxoxo O:..x...x...x...x. P:x-o-x-o-x-o-x-o-",
      16, 1, 0.1f, 0.4f, 0.4f, 0.3f },
    { "Disco & Funk", "Funk",
      "K=Acoustic S=Acoustic C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Cowbell",
      "K:x..x......x.x... S:....x..-.-..x..- H:xoxoxoxoxoxoxoxo O:..............o.",
      16, 1, 0.15f, 0.4f, 0.4f, 0.2f },
    { "Disco & Funk", "Disco Fox Funk",
      "K=Disco S=Disco C=Big R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Cowbell",
      "K:x...x...x...x... S:....x.......x... H:x.x.x.x.x.x.x.x. O:..x...x...x...x. P:..o...o...o...o.",
      16, 1, 0.05f, 0.4f, 0.4f, 0.3f },

    // ---------------------------------------------------------------- Schlager & Party
    { "Schlager & Party", "Discofox",
      "K=Disco S=Linn C=909 R=Sidestick H=909 O=909 L=Electronic M=Electronic T=Electronic Y=909 D=909 P=Tambourine Kd=.4 Kl=.8 Sl=.6 Cl=.62",
      "K:x...x...x...x... S:....x.......x... C:....x.......x... H:x.x.x.x.x.x.x.x. O:..x...x...x...x. P:..x...x...x...x.",
      16, 1, 0.0f, 0.4f, 0.45f, 0.3f },
    { "Schlager & Party", "Schlager Pop",
      "K=Linn S=Linn C=Big R=Sidestick H=Linn O=Linn L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Shaker Sr=.25",
      "K:x...x...x...x... S:....x.......x... H:x.x.x.x.x.x.x.x. P:x-o-x-o-x-o-x-o-",
      16, 1, 0.0f, 0.35f, 0.4f, 0.4f },
    { "Schlager & Party", "Party Schlager",
      "K=909 S=909 C=Big R=909_Rim H=909 O=909 L=909 M=909 T=909 Y=909 D=909 P=Tambourine Kv=.4 Sl=.6 Cl=.62",
      "K:x...x...x...x... C:....x.......x... S:....x.......x... H:xoxoxoxoxoxoxoxo O:..x...x...x...x.",
      16, 1, 0.0f, 0.55f, 0.55f, 0.3f },
    { "Schlager & Party", "Volksmusik Polka",
      "K=Acoustic S=Acoustic C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Tambourine Kd=.35",
      "K:x.x.x.x. S:.x.x.x.x P:.o.o.o.o",
      8, 0, 0.0f, 0.25f, 0.3f, 0.3f },
    { "Schlager & Party", "Schlager Ballade",
      "K=Acoustic S=Acoustic C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Shaker Rr=.35",
      "K:x......x.x...... R:....x.......x... H:x.o.x.o.x.o.x.o.",
      16, 1, 0.0f, 0.25f, 0.3f, 0.5f },
    { "Schlager & Party", "Stimmungs-Walzer",
      "K=Acoustic S=Acoustic C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Tambourine",
      "K:x.....x..... S:..x.x...x.x. H:..o.o...o.o.",
      12, 0, 0.0f, 0.25f, 0.3f, 0.35f },

    // ---------------------------------------------------------------- Jazz & Swing
    { "Jazz & Swing", "Jazz Swing Ride",
      "K=Jazz S=Brush C=Snap R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Jazz P=Shaker Kl=.5 Hl=.55 Dr=.25 Sr=.25",
      "D:x..o.ox..o.o H:...o.....o.. K:_.._.._.._.. S:..-.....-..-",
      12, 3, 0.0f, 0.15f, 0.2f, 0.35f },
    { "Jazz & Swing", "Jazz Brushes",
      "K=Jazz S=Brush C=Snap R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Jazz P=Shaker Sl=.85 Kl=.5 Sr=.3",
      "S:o..x.oo..x.o H:...o.....o.. K:_.....-.....",
      12, 3, 0.0f, 0.1f, 0.2f, 0.4f },
    { "Jazz & Swing", "Jazz Waltz",
      "K=Jazz S=Brush C=Snap R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Jazz P=Shaker Kl=.5 Dr=.25",
      "D:x..o.ox.. H:...o..o.. K:_........ S:......-..",
      9, 3, 0.0f, 0.15f, 0.2f, 0.35f },
    { "Jazz & Swing", "Shuffle Blues",
      "K=Acoustic S=Acoustic C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Shaker",
      "K:x.....x..... S:...x.....x.. H:x.ox.ox.ox.o",
      12, 3, 0.0f, 0.3f, 0.3f, 0.3f },
    { "Jazz & Swing", "Bebop Fast",
      "K=Jazz S=Brush C=Snap R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Jazz P=Shaker Kl=.45 Dd=.4",
      "D:x..x.xx..x.x H:...o.....o.. S:.....-....-. K:........_...",
      12, 3, 0.0f, 0.12f, 0.2f, 0.3f },

    // ---------------------------------------------------------------- Latin & World
    { "Latin & World", "Bossa Nova",
      "K=Jazz S=Brush C=Snap R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Jazz P=Shaker Kl=.6",
      "R:x..x..x...x..x.. K:x..ox..ox..ox..o H:xoxoxoxoxoxoxoxo",
      16, 0, 0.0f, 0.15f, 0.2f, 0.3f },
    { "Latin & World", "Samba",
      "K=Acoustic S=Acoustic C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Tambourine",
      "K:x..xx..xx..xx..x P:xoxoxoxoxoxoxoxo R:..x..x....x..x.. L:........x.......",
      16, 1, 0.1f, 0.3f, 0.3f, 0.25f },
    { "Latin & World", "Reggaeton",
      "K=808_Short S=909 C=Snap R=808_Rim H=808 O=808 L=808 M=808 T=808 Y=909 D=909 P=Cowbell",
      "K:x...x...x...x... S:...x..x....x..x. H:x.x.x.x.x.x.x.x.",
      16, 1, 0.0f, 0.45f, 0.4f, 0.2f },
    { "Latin & World", "Afrobeat",
      "K=Acoustic S=Acoustic C=909 R=Clave H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Conga",
      "K:x......x..x..... S:....x..-....x... P:..x.x..x..x.x..x H:xoxoxoxoxoxoxoxo R:x..x..x...x.x...",
      16, 1, 0.2f, 0.35f, 0.3f, 0.25f },
    { "Latin & World", "Dancehall",
      "K=808_Short S=Snap C=Snap R=808_Rim H=808 O=808 L=808 M=808 T=808 Y=909 D=909 P=Bongo",
      "K:x..x....x..x.... S:...x..x....x..x. H:x.x.x.x.x.x.x.x. P:......o.......o.",
      16, 1, 0.1f, 0.45f, 0.4f, 0.2f },
    { "Latin & World", "Cha Cha",
      "K=Acoustic S=Acoustic C=909 R=Woodblock H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Cowbell",
      "K:x.......x....... P:x.o.x.o.x.o.x.o. L:............x.x. R:....x.......x...",
      16, 1, 0.0f, 0.25f, 0.25f, 0.3f },

    // ---------------------------------------------------------------- Pop & Rock
    { "Pop & Rock", "Pop",
      "K=909 S=Linn C=Big R=Sidestick H=909 O=909 L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Shaker Kd=.35 Sl=.6 Cl=.62",
      "K:x.....x.x...x... S:....x.......x... C:....x.......x... H:x.x.x.x.x.x.x.x. P:x-o-x-o-x-o-x-o-",
      16, 1, 0.0f, 0.4f, 0.45f, 0.3f },
    { "Pop & Rock", "Rock",
      "K=Acoustic S=Acoustic C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Tambourine Kv=.3 Sv=.3 Sr=.3",
      "K:x.....x.x....... S:....x.......x... H:x.x.x.x.x.x.x.x.",
      16, 1, 0.0f, 0.5f, 0.45f, 0.35f },
    { "Pop & Rock", "Ballade",
      "K=Acoustic S=Acoustic C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Shaker Sd=.6 Sr=.45",
      "K:x.........x..... S:........x....... H:x.x.x.x.x.x.x.x.",
      16, 1, 0.0f, 0.3f, 0.35f, 0.5f },
    { "Pop & Rock", "80s Pop",
      "K=Linn S=Gated_80s C=909 R=Sidestick H=Linn O=Linn L=Electronic M=Electronic T=Electronic Y=Acoustic D=Acoustic P=Tambourine Sr=.45",
      "K:x.....x.x...x... S:....x.......x... H:x.x.x.x.x.x.x.x. P:....x.......x...",
      16, 1, 0.0f, 0.4f, 0.45f, 0.6f },
    { "Pop & Rock", "Indie Pop",
      "K=Acoustic S=Acoustic C=909 R=Sidestick H=Acoustic O=Acoustic L=Acoustic M=Acoustic T=Acoustic Y=Acoustic D=Acoustic P=Tambourine Ho=-.3",
      "K:x...x...x...x... S:....x.......x... H:xoxoxoxoxoxoxoxo P:..x...x...x...x.",
      16, 1, 0.05f, 0.35f, 0.4f, 0.3f },
};

static const FactoryPreset initPreset { "", "Init", "", "", 16, 1, 0.0f, 0.35f, 0.3f, 0.4f };

int Presets::numFactory() { return (int) (sizeof (factoryList) / sizeof (factoryList[0])); }

const FactoryPreset& Presets::factory (int index)
{
    return juce::isPositiveAndBelow (index, numFactory()) ? factoryList[index] : initPreset;
}

juce::StringArray Presets::categories()
{
    juce::StringArray c;
    for (int i = 0; i < numFactory(); ++i) c.addIfNotAlreadyThere (factoryList[i].category);
    return c;
}

int Presets::velForChar (juce::juce_wchar c, int& ratchet)
{
    ratchet = 1;
    switch (c)
    {
        case 'X': return 127;
        case 'x': return 100;
        case 'o': return 72;
        case '-': return 42;
        case '_': return 22;
        case 'r': ratchet = 2; return 95;
        case 'R': ratchet = 3; return 95;
        case 'q': ratchet = 4; return 85;
        default: break;
    }
    if (c >= '1' && c <= '9') return (int) (c - '0') * 14;
    return 0;
}

static int laneForCode (juce::juce_wchar c)
{
    for (int i = 0; i < DrumInfo::numLanes; ++i) if (DrumInfo::codes[i] == c) return i;
    return -1;
}

void Presets::parsePattern (const juce::String& text, PatternStore& patterns, int p, int steps)
{
    patterns.clear (p);
    for (auto tok : juce::StringArray::fromTokens (text, " ", ""))
    {
        if (tok.length() < 3 || tok[1] != ':') continue;
        const int lane = laneForCode (tok[0]);
        if (lane < 0) continue;
        const auto steps_ = tok.substring (2);
        for (int s = 0; s < juce::jmin (steps, steps_.length(), DrumInfo::maxSteps); ++s)
        {
            int rat = 1;
            const int v = velForChar (steps_[s], rat);
            patterns.setVel (p, lane, s, v);
            patterns.setRatchet (p, lane, s, rat);
        }
    }
}

void Presets::makeVariations (PatternStore& pt, int steps)
{
    // B = Fill: Ende mit Toms und Snare-Wirbel
    pt.copy (0, 1);
    const int q = juce::jmax (2, steps / 4);
    const int from = steps - q;
    for (int s = from; s < steps; ++s)
        for (int l : { 0, 1, 2, 3, 6, 7, 8 }) pt.setVel (1, l, s, 0);
    for (int i = 0; i < q; ++i)
    {
        const int s = from + i;
        const float pos = (float) i / (float) juce::jmax (1, q - 1);
        const int v = 70 + juce::roundToInt (pos * 57.0f);
        if (i < q / 2)
            pt.setVel (1, i % 2 == 0 ? 8 : 7, s, v + 10);
        else
            pt.setVel (1, (i % 2 == 0) ? 6 : 1, s, v);
        if (i == q - 1) { pt.setVel (1, 1, s, 120); pt.setRatchet (1, 1, s, 2); }
    }
    pt.setVel (1, 0, from, juce::jmax (pt.vel (0, 0, from), 90));

    // C = Break: ohne Kick, Snare, Clap und Toms – Becken/Perc bleiben, leiser
    pt.copy (0, 2);
    for (int s = 0; s < steps; ++s)
    {
        for (int l : { 0, 1, 2, 6, 7, 8, 9 }) pt.setVel (2, l, s, 0);
        for (int l : { 3, 4, 5, 10, 11 }) if (pt.vel (2, l, s) > 0) pt.setVel (2, l, s, juce::roundToInt (pt.vel (2, l, s) * 0.8f));
    }
    pt.setVel (2, 0, 0, 70);   // weicher erster Schlag

    // D = Crash: wie A, mit Becken auf der Eins
    pt.copy (0, 3);
    pt.setVel (3, 9, 0, 115);
    pt.setVel (3, 0, 0, juce::jmax (pt.vel (0, 0, 0), 110));
}

void Presets::applyFactory (int index, juce::AudioProcessorValueTreeState& apvts, PatternStore& patterns)
{
    const auto& fp = factory (index);

    auto set = [&apvts] (const juce::String& id, float value)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
    };

    // alle Spuren auf Grundwerte
    for (int l = 0; l < DrumInfo::numLanes; ++l)
        for (auto* what : { "model", "level", "tune", "decay", "tone", "snap", "drive", "pan", "room", "mute", "solo" })
            if (auto* prm = apvts.getParameter (PID::lane (l, what)))
                prm->setValueNotifyingHost (prm->getDefaultValue());

    // Hi-Hats und Perc leicht verteilt (Stereo-Bild wie am Schlagzeug)
    set (PID::lane (4, "pan"), 0.15f);  set (PID::lane (5, "pan"), 0.15f);
    set (PID::lane (6, "pan"), 0.30f);  set (PID::lane (7, "pan"), 0.0f);  set (PID::lane (8, "pan"), -0.30f);
    set (PID::lane (9, "pan"), -0.25f); set (PID::lane (10, "pan"), 0.25f); set (PID::lane (11, "pan"), -0.15f);

    for (auto tok : juce::StringArray::fromTokens (juce::String (fp.kit), " ", ""))
    {
        const int eq = tok.indexOfChar ('=');
        if (eq < 1) continue;
        const int lane = laneForCode (tok[0]);
        if (lane < 0) continue;
        const auto key = tok.substring (1, eq);
        const auto val = tok.substring (eq + 1);
        if (key.isEmpty())
        {
            const auto names = DrumInfo::models (lane);
            const int m = names.indexOf (val.replaceCharacter ('_', ' '), true);
            if (m >= 0) set (PID::lane (lane, "model"), (float) m);
            continue;
        }
        static const std::pair<const char*, const char*> keys[] {
            { "l", "level" }, { "t", "tune" }, { "d", "decay" }, { "o", "tone" }, { "s", "snap" },
            { "v", "drive" }, { "p", "pan" },  { "r", "room" } };
        for (auto& k : keys)
            if (key == k.first) set (PID::lane (lane, k.second), val.getFloatValue());
    }

    const int steps = juce::jlimit (8, DrumInfo::maxSteps, fp.steps);
    set (PID::steps, (float) (steps - 8));
    set (PID::res, (float) fp.res);
    set (PID::swing, fp.swing);
    set (PID::humanize, 0.0f);
    set (PID::fat, fp.fat);
    set (PID::glue, fp.glue);
    set (PID::roomSize, fp.room);
    set (PID::pattern, 0.0f);

    parsePattern (fp.pattern, patterns, 0, steps);
    makeVariations (patterns, steps);
}

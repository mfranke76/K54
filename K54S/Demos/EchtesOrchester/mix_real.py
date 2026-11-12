import soundfile as sf, numpy as np, sys
parts = {  # Datei: Ziel-Pegel relativ (dB)
 '/home/claude/orch/track1.wav': -2,   # Dudelsack-Orgel (K-54 S)
 'tp.wav': -3, 'hn.wav': -7, 'tb.wav': -9,
 'vn.wav': -8, 'vc.wav': -10, 'cb.wav': -10,
 '/home/claude/orch/track6.wav': -12,
}
L = max(sf.info(f).frames for f in parts); mix = np.zeros((L, 2))
for f, db in parts.items():
    x, _ = sf.read(f, always_2d=True); r = np.sqrt((x**2).mean()) + 1e-9
    mix[:len(x)] += x / r * 0.05 * 10**(db/20)
mix *= 0.9 / np.abs(mix).max()
sf.write('Amazing_Grace_Echtes_Orchester.wav', mix, 48000, subtype='PCM_24')

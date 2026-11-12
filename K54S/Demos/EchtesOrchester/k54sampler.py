#!/usr/bin/env python3
"""Keller54 Echt-Sampler: spielt MIDI-Spuren mit echten Instrumentenaufnahmen (VSCO 2 CE, CC0).
   Aufruf:  k54sampler.py <midi> <spur> <instrument> <ziel.wav> [--section N] [--gain dB] [--vel-curve x]
   Instrumente: trumpet horn trombone tuba violins violas celli contrabass
"""
import sys, os, re, json, argparse, numpy as np, soundfile as sf, mido
from scipy.signal import resample_poly, fftconvolve
VSCO = os.environ.get('VSCO', '/home/claude/sgossner/vsco-2-ce')
SR = 48000
INSTR = {
 'trumpet':   'Brass/Trumpet/sus',        'horn':  'Brass/F Horn/sus',
 'trombone':  'Brass/Tenor Trombone/sus', 'tuba':  'Brass/Tuba/sus',
 'violins':   'Strings/Violin Section/susVib', 'violas': 'Strings/Viola Section/susvib',
 'celli':     'Strings/Cello Section/susvib',  'contrabass': 'Strings/Solo Contrabass/SusVib',
}
def detect_pitch(x, sr):
    seg = x[int(0.35*sr):int(0.35*sr)+16384]; seg = seg - seg.mean()
    if len(seg) < 4096 or np.abs(seg).max() < 1e-4: return None
    sp = np.abs(np.fft.rfft(seg*np.hanning(len(seg)), 4*len(seg)))
    # harmonische Produkt-Spektrum-Schaetzung (robust gegen Oktavfehler)
    hps = sp.copy()
    for h in (2, 3, 4):
        d = sp[::h]; hps[:len(d)] *= d
    f = np.fft.rfftfreq(4*len(seg), 1/sr)
    lo, hi = np.searchsorted(f, 25), np.searchsorted(f, 2200)
    return f[lo + np.argmax(hps[lo:hi])]
def load_instrument(name):
    d = os.path.join(VSCO, INSTR[name]); cache = os.path.join(d, '.k54map.json')
    if os.path.exists(cache): return json.load(open(cache))
    out = []
    for fn in sorted(os.listdir(d)):
        if not fn.lower().endswith('.wav'): continue
        x, sr = sf.read(os.path.join(d, fn), always_2d=True)
        hz = detect_pitch(x.mean(1), sr)
        if not hz: continue
        midi = 69 + 12*np.log2(hz/440.0)
        m = re.search(r'_v(\d+)', fn); layer = int(m.group(1)) if m else 1
        rms = float(np.sqrt((x[int(.2*sr):int(1.2*sr)]**2).mean()))
        out.append({'file': fn, 'midi': round(float(midi), 2), 'layer': layer, 'rms': rms})
    json.dump(out, open(cache, 'w')); return out
_cache = {}
def sample(name, fn):
    k = (name, fn)
    if k not in _cache:
        x, sr = sf.read(os.path.join(VSCO, INSTR[name], fn), always_2d=True)
        if x.shape[1] == 1: x = np.repeat(x, 2, 1)
        if sr != SR: x = resample_poly(x, SR, sr, axis=0)
        _cache[k] = x.astype(np.float32)
    return _cache[k]
def pick(table, note, vel):
    layers = sorted({e['layer'] for e in table})
    want = layers[min(len(layers)-1, int(vel/128*len(layers)))]
    cands = [e for e in table if e['layer'] == want] or table
    return min(cands, key=lambda e: abs(e['midi'] - note) + (0.3 if e['midi'] > note else 0))
def render_note(name, table, note, vel, dur, rng, detune_c=0.0):
    e = pick(table, note, vel)
    x = sample(name, e['file'])
    shift = note - e['midi'] + detune_c/100.0
    ratio = 2 ** (shift/12)
    # Tonhoehe per Resampling (fuer kleine Abstaende klanglich unauffaellig)
    up, down = int(round(1000/ratio)), 1000
    y = resample_poly(x, up, down, axis=0) if abs(shift) > 0.01 else x
    need = int((dur + 0.6) * SR)
    if len(y) < need:   # lange Note: mittleren Teil mit Ueberblendung schleifen
        a, b = int(0.6*len(y)), int(0.9*len(y)); seg = y[a:b]; xf = int(0.08*SR)
        parts = [y[:b].copy()]
        while sum(len(p) for p in parts) < need:
            s = seg.copy(); s[:xf] *= np.linspace(0, 1, xf)[:, None]
            parts[-1][-xf:] *= np.linspace(1, 0, xf)[:, None]
            parts[-1][-xf:] += s[:xf]; parts.append(s[xf:])
        y = np.concatenate(parts)
    y = y[:need].copy()
    rel = int(min(0.35, 0.12 + dur*0.05) * SR)          # natuerliches Ausklingen
    off = int(dur*SR); env = np.ones(len(y)); env[off:off+rel] = np.linspace(1, 0, rel)[:len(env[off:off+rel])]; env[off+rel:] = 0
    g = (vel/127) ** 1.3 / (e['rms'] + 1e-4) * 0.08      # Lautstaerke angleichen, Dynamik behalten
    return y * env[:, None] * g
def hall(x, rt=2.4, pre=0.025, mix=0.28, seed=1):
    rng = np.random.default_rng(seed); n = int(rt*SR); t = np.arange(n)/SR
    ir = rng.standard_normal((n, 2)) * np.exp(-6.9*t/rt)[:, None]
    ir[:int(0.003*SR)] = 0; ir = np.vstack([np.zeros((int(pre*SR), 2)), ir])
    # dunkler werden lassen (Luftdaempfung)
    k = np.exp(-np.arange(64)/12); k /= k.sum(); ir = np.stack([np.convolve(ir[:, c], k, 'same') for c in (0, 1)], 1)
    ir /= np.sqrt((ir**2).sum()/2)
    wet = np.stack([fftconvolve(x[:, c], ir[:, c])[:len(x)] for c in (0, 1)], 1)
    return x*(1-mix*0.5) + wet*mix
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('midi'); ap.add_argument('track', type=int); ap.add_argument('instrument'); ap.add_argument('out')
    ap.add_argument('--section', type=int, default=2, help='Anzahl Spieler (Ensemble)')
    ap.add_argument('--transpose', type=int, default=0); ap.add_argument('--pan', type=float, default=0.0)
    ap.add_argument('--hall', type=float, default=0.28); ap.add_argument('--seed', type=int, default=54)
    a = ap.parse_args()
    table = load_instrument(a.instrument); rng = np.random.default_rng(a.seed)
    mid = mido.MidiFile(a.midi); tempo = 500000; t = 0.0; ons = {}; notes = []
    tempos = []
    for m in mido.merge_tracks([mid.tracks[0]]):
        pass
    tr = mido.merge_tracks([mid.tracks[0], mid.tracks[a.track]])
    for m in tr:
        t += mido.tick2second(m.time, mid.ticks_per_beat, tempo)
        if m.type == 'set_tempo': tempo = m.tempo
        elif m.type == 'note_on' and m.velocity > 0: ons[m.note] = (t, m.velocity)
        elif m.type in ('note_off', 'note_on') and m.note in ons:
            s, v = ons.pop(m.note); notes.append((s, t - s, m.note + a.transpose, v))
    L = int((t + 4) * SR); out = np.zeros((L, 2), np.float32)
    for (s, d, n, v) in notes:
        for p in range(a.section):            # Ensemble: leicht versetzt, leicht verstimmt
            dt = rng.normal(0, 0.012) + (0.008*p if p else 0)
            dc = rng.normal(0, 4.0) + (6.0*(p % 2*2-1) if p else 0)
            vv = int(np.clip(v + rng.normal(0, 5), 20, 127))
            y = render_note(a.instrument, table, n, vv, d, rng, dc) / np.sqrt(a.section)
            i = max(0, int((s + dt) * SR)); k = min(len(y), L - i)
            pan = np.clip(a.pan + (p - (a.section-1)/2) * 0.25, -1, 1)
            out[i:i+k, 0] += y[:k, 0] * (1 - pan) ** 0.5; out[i:i+k, 1] += y[:k, 1] * (1 + pan) ** 0.5
    out = hall(out, mix=a.hall)
    pk = np.abs(out).max(); print(f'{a.instrument}: {len(notes)} Noten, Spitze {pk:.2f}')
    sf.write(a.out, out, SR, subtype='PCM_24')
if __name__ == '__main__': main()

import mido
TPB=480; BPM=128
# Melodie in A-Dur (New Britain +2), 3/4 -> 4/4: erster Ton jedes Taktes eine Zaehlzeit laenger
A4,B4,Cs5,E5,Fs4,E4,A3=69,71,73,76,66,64,57
bars3=[[(A4,2),(Cs5,.5),(A4,.5)],[(Cs5,2),(B4,1)],[(A4,2),(Fs4,1)],[(E4,2),(E4,1)],
       [(A4,2),(Cs5,.5),(A4,.5)],[(Cs5,2),(B4,1)],[(E5,3)],
       [(E5,2),(Cs5,.5),(E5,.5)],[(Cs5,2),(A4,1)],[(E4,2),(Fs4,.5),(A4,.5)],[(A4,2),(Fs4,1)],[(E4,2),(E4,1)],
       [(A4,2),(Cs5,.5),(A4,.5)],[(Cs5,2),(B4,1)],[(A4,3)]]
mel4=[[(n,d+1 if i==0 else d) for i,(n,d) in enumerate(b)] for b in bars3]
chords=['A','A','D','A','Fm','E','E','E','A','D','D','A','Fm','E','A']
PAD={'A':[57,61,64,69],'D':[57,62,66,69],'E':[56,59,64,68],'Fm':[57,61,66,69]}
ROOT={'A':45,'D':38,'E':40,'Fm':42}
TR=['Kick','Clap','HiHat zu','HiHat offen','Snare-Wirbel','Crash',  # Drums (GM, Kanal 10)
    'Bass Offbeat','Pad','Pluck Arp','Lead Breakdown (Dudelsack-Orgel)','Lead Drop (Super Saw)']
ev={k:[] for k in TR}
def tk(b): return int(round(b*TPB))
def note(tr,start,dur,n,vel):
    ev[tr].append((tk(start),1,n,vel)); ev[tr].append((tk(start+dur)-6,0,n,0))
bar=lambda b: b*4
def chord_at(b, start):  # Akkord fuer Takt b, wenn die Melodie in Takt 'start' beginnt
    i=b-start
    return chords[i] if 0<=i<len(chords) else 'A'
BD, DROP = 9, 28            # Melodietakte Breakdown 9..23, Drop 28..42
def section_chord(b):
    if BD<=b<BD+15: return chord_at(b,BD)
    if DROP<=b<DROP+15: return chord_at(b,DROP)
    return 'A' if b<BD or b>=DROP+15 else ('E' if b>=26 else 'A')
for b in range(48):
    c=section_chord(b); t=bar(b)
    kick = b<8 or (26<=b<44) or b>=44
    if b<8 and b<2: kick = True
    if kick:
        for q in range(4): note('Kick',t+q,.25,36,120)
    if 28<=b<44 or 4<=b<8:
        note('Clap',t+1,.25,39,100); note('Clap',t+3,.25,39,100)
    if 4<=b<8 or 28<=b<46:
        for q in range(4):
            note('HiHat offen',t+q+.5,.25,46,90)
            note('HiHat zu',t+q,.1,42,60); note('HiHat zu',t+q+.25,.1,42,45); note('HiHat zu',t+q+.75,.1,42,45)
    # Bass: Offbeat-Achtel (klassischer Trance-Bass)
    if 4<=b<8 or 26<=b<46:
        for q in range(4): note('Bass Offbeat',t+q+.5,.45,ROOT[c],100)
    # Pad: ganze Takte, im Breakdown lauter
    note('Pad',t,4,PAD[c][0],0) if False else None
    for n in PAD[c]: note('Pad',t,3.95,n,90 if BD-1<=b<DROP else 70)
    # Pluck-Arpeggio in Sechzehnteln (Intro ab Takt 4, Build, Drop)
    if 4<=b<8 or 24<=b<44:
        pat=[0,1,2,3,2,1,2,3]
        tones=[n+12 for n in PAD[c]]
        for s in range(16): note('Pluck Arp',t+s*.25,.22,tones[pat[s%8]],85 if s%4==0 else 70)
# Snare-Wirbel im Build (Takte 24-27) und Crash auf dem Drop
for b in range(24,28):
    step={24:1,25:.5,26:.25,27:.125}[b]
    k=0.0
    while k<4-1e-6:
        note('Snare-Wirbel',bar(b)+k,step*.8,38,int(60+40*((b-24)*4+k)/16)); k+=step
note('Crash',bar(DROP),2,49,120); note('Crash',bar(44),2,49,110); note('Crash',bar(8),2,49,90)
# Melodie
def melody(tr,start_bar,octave,vel):
    note(tr,bar(start_bar)-1,0.95,E4+octave,vel)            # Auftakt
    for i,b in enumerate(mel4):
        t=bar(start_bar+i)
        for n,d in b: note(tr,t,d*0.97,n+octave,vel); t+=d
melody('Lead Breakdown (Dudelsack-Orgel)',BD,0,92)
melody('Lead Drop (Super Saw)',DROP,12,110)
mid=mido.MidiFile(ticks_per_beat=TPB)
meta=mido.MidiTrack(); mid.tracks.append(meta)
meta.append(mido.MetaMessage('track_name',name='Amazing Grace - Keller54 Trance 128',time=0))
meta.append(mido.MetaMessage('set_tempo',tempo=mido.bpm2tempo(BPM),time=0))
meta.append(mido.MetaMessage('time_signature',numerator=4,denominator=4,time=0))
meta.append(mido.MetaMessage('key_signature',key='A',time=0))
for name in TR:
    ch=9 if TR.index(name)<6 else TR.index(name)-6
    t=mido.MidiTrack(); mid.tracks.append(t); t.append(mido.MetaMessage('track_name',name=name,time=0))
    last=0
    for tick,on,n,vel in sorted(ev[name],key=lambda e:(e[0],e[1])):
        t.append(mido.Message('note_on' if on else 'note_off',channel=ch,note=n,velocity=vel,time=tick-last)); last=tick
mid.save('/home/claude/trance/Amazing_Grace_Keller54_Trance_128.mid')
print('Laenge', round(mid.length,1),'s, Spuren', len(TR))

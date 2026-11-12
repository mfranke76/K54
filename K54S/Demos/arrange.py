import mido
TPB=480; BPM=66; T=2  # transponiert G -> A
# Melodie (New Britain, G-Dur), (note, beats)
G4,A4,B4,C5,D5,E4,D4,G3=67,69,71,72,74,64,62,55
mel=[(D4,1),
 (G4,2),(B4,.5),(G4,.5), (B4,2),(A4,1), (G4,2),(E4,1), (D4,2),(D4,1),
 (G4,2),(B4,.5),(G4,.5), (B4,2),(A4,1), (D5,3),
 (D5,2),(B4,.5),(D5,.5), (B4,2),(G4,1), (D4,2),(E4,.5),(G4,.5), (G4,2),(E4,1), (D4,2),(D4,1),
 (G4,2),(B4,.5),(G4,.5), (B4,2),(A4,1), (G4,3)]
# Akkorde je Takt (15 Takte nach dem Auftakt)
chords=['G','G','C','G','G','D','D','D','G','C','C','G','G','D','G']
CH={'G':[55,59,62,67],'C':[55,60,64,67],'D':[54,57,62,66]}
ROOT={'G':43,'C':48,'D':38}
verse_beats=1+15*3
def tick(b): return int(round(b*TPB))
tracks={k:[] for k in ['Melodie Dudelsack-Orgel','Melodie Spanische Trompeten','Streicher','Blech','Bass','Glocken']}
def note(tr,start,dur,n,vel):
    tracks[tr].append((tick(start),'on',n+T,vel)); tracks[tr].append((tick(start+dur)-8,'off',n+T,0))
for v in range(2):
    off=v*verse_beats
    # Melodie
    b=off
    tr='Melodie Dudelsack-Orgel' if v==0 else 'Melodie Spanische Trompeten'
    for n,d in mel: note(tr,b,d*0.98,n,96 if v else 90); b+=d
    if v==1:  # letzter Ton laenger
        pass
    for i,c in enumerate(chords):
        bar=off+1+i*3
        last = (v==1 and i==14)
        L = 6 if last else 3
        # Streicher: ganze Takte, Strophe 1 leise, Strophe 2 voller (Oktave dazu)
        for n in CH[c]: note('Streicher',bar,L,n,58 if v==0 else 84)
        if v==1:
            for n in CH[c][:2]: note('Streicher',bar,L,n+12,70)
        # Bass
        note('Bass',bar,L if last else 2.9,ROOT[c],80 if v else 64)
        if v==1:
            # Blech: Akkord auf 1, kurzer Akzent auf 3 in jedem zweiten Takt
            for n in CH[c][1:]: note('Blech',bar,2.0 if not last else L,n,88)
            if i%2==1 and not last:
                for n in CH[c][1:]: note('Blech',bar+2,0.9,n,72)
            if i in (0,4,8,12,14):
                note('Glocken',bar,3,CH[c][-1]+12,92)
mid=mido.MidiFile(ticks_per_beat=TPB)
meta=mido.MidiTrack(); mid.tracks.append(meta)
meta.append(mido.MetaMessage('track_name',name='Amazing Grace - Keller54 Orchester',time=0))
meta.append(mido.MetaMessage('set_tempo',tempo=mido.bpm2tempo(BPM),time=0))
meta.append(mido.MetaMessage('time_signature',numerator=3,denominator=4,time=0))
meta.append(mido.MetaMessage('key_signature',key='A',time=0))
for ch,(name,evs) in enumerate(tracks.items()):
    t=mido.MidiTrack(); mid.tracks.append(t)
    t.append(mido.MetaMessage('track_name',name=name,time=0))
    evs.sort(key=lambda e:(e[0],0 if e[1]=='off' else 1))
    last=0
    for tk,typ,n,vel in evs:
        t.append(mido.Message('note_on' if typ=='on' else 'note_off',channel=ch,note=n,velocity=vel,time=tk-last)); last=tk
mid.save('/home/claude/orch/Amazing_Grace_Keller54_Orchester.mid')
print('Laenge s:', round(mid.length,1), 'Spuren:', len(mid.tracks)-1)

import wave, numpy as np, mido
SR=48000
def rd(f):
  w=wave.open(f);d=w.readframes(w.getnframes());a=np.frombuffer(d,dtype=np.uint8).reshape(-1,3)
  v=(a[:,0].astype(np.int32)|(a[:,1].astype(np.int32)<<8)|(a[:,2].astype(np.int32)<<16));v=np.where(v>=1<<23,v-(1<<24),v)/float(1<<23)
  return v.reshape(-1,2)
mid=mido.MidiFile('Amazing_Grace_Keller54_Trance_128.mid')
def hits(idx):
  t=0; out=[]; tempo=mido.bpm2tempo(128)
  for m in mid.tracks[idx]:
    t+=mido.tick2second(m.time,mid.ticks_per_beat,tempo)
    if m.type=='note_on' and m.velocity>0: out.append((t,m.velocity/127))
  return out
N=int(94*SR); rng=np.random.default_rng(54)
def lp(x,a):  # einfacher Tiefpass
  y=np.empty_like(x); s=0.0
  for i,v in enumerate(x): s+=a*(v-s); y[i]=s
  return y
# Samples bauen
t=np.arange(int(.45*SR))/SR
f=48+110*np.exp(-t*28)                      # 909-artige Kick: Pitch faellt von ~160 auf 48 Hz
kick=np.sin(2*np.pi*np.cumsum(f)/SR)*np.exp(-t*7.5)
kick+=0.35*np.exp(-t*900)*rng.standard_normal(len(t))  # Klick
kick=np.tanh(kick*1.8)*0.95
tc=np.arange(int(.3*SR))/SR; nz=rng.standard_normal(len(tc))
env=np.exp(-tc*28)+0.6*(np.exp(-np.clip(tc-0.012,0,None)*28)*(tc>0.012))+0.5*(np.exp(-np.clip(tc-0.024,0,None)*22)*(tc>0.024))
clap=(nz-lp(nz,0.25))*env*0.55
th=np.arange(int(.06*SR))/SR; hz=rng.standard_normal(len(th)); hihat=(hz-lp(hz,0.6))*np.exp(-th*70)*0.35
to=np.arange(int(.28*SR))/SR; ho=rng.standard_normal(len(to)); hopen=(ho-lp(ho,0.55))*np.exp(-to*12)*0.32
ts=np.arange(int(.18*SR))/SR; sn=rng.standard_normal(len(ts)); snare=((sn-lp(sn,0.15))*0.6+0.4*np.sin(2*np.pi*190*ts))*np.exp(-ts*22)*0.5
tcr=np.arange(int(2.5*SR))/SR; cr=rng.standard_normal(len(tcr)); crash=(cr-lp(cr,0.5))*np.exp(-tcr*1.6)*0.28
drums=np.zeros((N,2)); kickbus=np.zeros(N)
def place(sample,events,pan=0.0,bus=None):
  for tt,v in events:
    i=int(tt*SR); n=min(len(sample),N-i)
    if n<=0: continue
    drums[i:i+n,0]+=sample[:n]*v*(1-pan); drums[i:i+n,1]+=sample[:n]*v*(1+pan)
    if bus is not None: bus[i:i+n]+=np.abs(sample[:n])*v
place(kick,hits(1),0,kickbus); place(clap,hits(2)); place(hihat,hits(3),0.25); place(hopen,hits(4),-0.2)
place(snare,hits(5)); place(crash,hits(6))
# Riser (Rauschen, das im Build ansteigt): Takte 24-28
b=60/128*4; s0,s1=int(24*b*SR),int(28*b*SR)
rz=rng.standard_normal(s1-s0); ramp=np.linspace(0,1,s1-s0)**2
riser=np.zeros(s1-s0); a=0.02
# Hochpass mit steigender Grenzfrequenz
lo=0.0
for i in range(len(rz)):
  aa=0.25-0.22*ramp[i]; lo+=aa*(rz[i]-lo); riser[i]=(rz[i]-lo)*ramp[i]*0.25
drums[s0:s1,0]+=riser; drums[s0:s1,1]+=riser
# Sidechain: Huellkurve aus der Kick, Synths pumpen (typisch Trance)
env=np.zeros(N); e=0.0
att=1-np.exp(-1/(0.002*SR)); rel=1-np.exp(-1/(0.16*SR))
for i in range(N):
  x=kickbus[i]; e+= (att if x>e else rel)*(x-e); env[i]=e
duck=1-0.62*np.clip(env/ (env.max()+1e-9)*1.6,0,1)
tr={}
for k in (7,8,9,10,11):
  x=rd(f't{k}.wav'); x=x[:N] if len(x)>=N else np.vstack([x,np.zeros((N-len(x),2))]); tr[k]=x
g={7:1.25,8:0.55,9:0.55,10:1.9,11:0.85}
mix=drums*1.0
for k,x in tr.items():
  d=duck if k in (7,8,9,11) else (1-0.25*(1-duck))
  mix+=x*g[k]*d[:,None]
# Breakdown etwas lauter (Takte 8-23), weich ueberblendet
sec=np.ones(N); a_,b_=int(8*b*SR),int(24*b*SR); x=int(0.5*SR)
sec[a_:b_]=1.6; sec[a_:a_+x]=np.linspace(1,1.6,x); sec[b_-x:b_]=np.linspace(1.6,1,x)
mix*=sec[:,None]
# Bus-Kompression light + Limiter
pk=np.abs(mix).max(); mix=mix/pk
mix=np.tanh(mix*1.6)/np.tanh(1.6)*0.93
L=int(91.5*SR); mix=mix[:L]; fo=int(3*SR); mix[-fo:]*=np.linspace(1,0,fo)[:,None]
y=(mix*32767).astype(np.int16)
w=wave.open('Amazing_Grace_Keller54_Trance_128.wav','wb'); w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR); w.writeframes(y.tobytes()); w.close()
# Pegelverlauf zur Kontrolle (RMS je 8 Takte)
for s in range(0,48,4):
  a_,b_=int(s*b*SR),int(min(48,s+4)*b*SR); seg=mix[a_:b_]
  print(f'Takt {s:2d}-{s+3:2d}: {20*np.log10(np.sqrt((seg**2).mean())+1e-9):6.1f} dB RMS')

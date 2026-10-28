// Original Paul Mudra score: VGM logs from VGMPF, kept alongside the source data.
const fs=require('fs'),path=require('path'),zlib=require('zlib'),crypto=require('crypto');
const root=path.resolve(__dirname,'..'),dir=path.join(root,'assets/audio');
function parse(file){
 const gz=fs.readFileSync(path.join(dir,file)),b=gz[0]===31?zlib.gunzipSync(gz):gz;
 if(b.toString('ascii',0,4)!=='Vgm ')throw Error('Invalid VGM');
 const version=b.readUInt32LE(8),data=version>=0x150&&b.readUInt32LE(0x34)?0x34+b.readUInt32LE(0x34):0x40;
 const regs=new Uint8Array(256),frames=new Map();let i=data,time=0,loopTime=null,loopAt=b.readUInt32LE(0x1c)?0x1c+b.readUInt32LE(0x1c):0;
 const getFrame=()=>{const t=Math.round(time/882);if(!frames.has(t))frames.set(t,{raw:[],notes:new Map()});return frames.get(t);};
 const carrier=[3,4,5,11,12,13,19,20,21];
 const instruments=[1,8,5,4,12,3,1,14,4];
 function note(ch){const f=regs[0xa0+ch]|((regs[0xb0+ch]&3)<<8),block=(regs[0xb0+ch]>>2)&7,key=!!(regs[0xb0+ch]&32);
  const hz=f*49716*Math.pow(2,block)/1048576,period=hz?Math.max(1,Math.min(4095,Math.round(1789772.5/(16*hz)))):0;
  const fm=Math.round(f/2)|(block<<9),volume=key?Math.max(1,15-Math.round((regs[0x40+carrier[ch]]&63)/4)):0;
  getFrame().notes.set(ch,[ch,period&255,period>>8,fm&255,fm>>8,(instruments[ch]<<4)|volume]);
 }
 while(i<b.length){if(i===loopAt)loopTime=Math.round(time/882);const c=b[i++];
  if(c===0x5a){const r=b[i++],v=b[i++];regs[r]=v;getFrame().raw.push(r,v);if(r>=0xa0&&r<=0xa8)note(r-0xa0);else if(r>=0xb0&&r<=0xb8)note(r-0xb0);else{const ch=carrier.indexOf(r-0x40);if(ch>=0)note(ch);}}
  else if(c===0x61){time+=b.readUInt16LE(i);i+=2;}else if(c===0x62)time+=735;else if(c===0x63)time+=882;else if(c>=0x70&&c<=0x7f)time+=(c&15)+1;else if(c===0x66)break;
  else if(c===0x67){if(b[i++]!==0x66)throw Error('VGM block');i++;const n=b.readUInt32LE(i);i+=4+n;}
  else throw Error(`Unsupported VGM command ${c.toString(16)} at ${i-1}`);
 }
 function encode(kind){const out=[];let prev=0;for(const [t,f]of frames){const values=kind==='raw'?f.raw:[...f.notes.values()].flat();const n=values.length/(kind==='raw'?2:6);if(!n)continue;if(n>254)throw Error('Oversized music frame');const delay=t-prev;if(delay>65534)throw Error('Wait overflow');out.push(delay&255,delay>>8,n,...values);prev=t;}
  const tail=Math.max(0,Math.round(time/882)-prev);out.push(tail&255,tail>>8,255);return Buffer.from(out);
 }
 return {file,sha256:crypto.createHash('sha256').update(gz).digest('hex'),seconds:time/44100,loopTime,raw:encode('raw'),notes:encode('notes')};
}
const tracks=['04 Character Generation.vgz','05 Ending Theme.vgz','01 Westwood Associates Logo.vgz','02 Introduction Part 1.vgz','03 Introduction Part 2.vgz'].map(parse);
// Stable separate bank region, far above the graphics payload.
let offset=0x1e0000;const parts=[],meta=[];
for(const t of tracks){const slots=[];for(const kind of ['notes','raw']){const b=t[kind];slots.push({bank:offset>>13,offset:offset&8191,bytes:b.length});parts.push(b);offset+=b.length;}meta.push({title:t.file,seconds:t.seconds,loopTime:t.loopTime,sha256:t.sha256,streams:slots});}
if(offset>0x200000)throw Error('Audio exceeds 2MB ROM');
fs.writeFileSync(path.join(root,'assets/generated/music.bin'),Buffer.concat(parts));
fs.writeFileSync(path.join(root,'assets/generated/music.json'),JSON.stringify(meta,null,2));
fs.writeFileSync(path.join(root,'src/music.h'),'/* Generated: original Paul Mudra DOS score. */\nstatic const unsigned char music_banks[10]={'+meta.flatMap(m=>m.streams.map(s=>s.bank)).join(',')+'};\nstatic const unsigned int music_offsets[10]={'+meta.flatMap(m=>m.streams.map(s=>s.offset)).join(',')+'};\n');
console.log(meta);



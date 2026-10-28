// SPDX-License-Identifier: GPL-3.0-or-later
// VMP/VCN layout and composition adapted from ScummVM scene_rpg.cpp and scene_eob.cpp.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const {decode,png}=require('./bmp-codec.cjs');
const root=path.resolve(__dirname,'..'),source=path.join(root,'assets/source'),out=path.join(root,'assets/generated');fs.mkdirSync(out,{recursive:true});
const read=n=>fs.readFileSync(path.join(source,n));
const vcn=decode(read('BRICK.VCN')).pixels,count=vcn.readUInt16LE(0),col=vcn.subarray(2,34),tiles=vcn.subarray(34);
if(tiles.length!==count*32)throw Error('VCN length');
const m=read('BRICK.VMP'),vmp=Array.from({length:m.readUInt16LE(0)},(_,i)=>m.readUInt16LE(2+i*2));
const rgbpal=read('BRICK.PAL'),globalpal=read('EOBPAL.COL');
const rgb=(p,n)=>[0,1,2].map(c=>Math.round((p[n*3+c]&63)*255/63));
const wallPalette=Array.from({length:256},(_,n)=>rgb(rgbpal,n));
const basePalette=Array.from({length:256},(_,n)=>rgb(globalpal,n));
const maze=JSON.parse(fs.readFileSync(path.join(root,'assets/map.json'))).rows,dx=[0,1,0,-1],dy=[-1,0,1,0];
const solid=(x,y)=>x<0||x>=12||y<0||y>=12||maze[y][x]==='#';
const locations=[[-3,3],[-2,3],[-1,3],[0,3],[1,3],[2,3],[3,3],[-2,2],[-1,2],[0,2],[1,2],[2,2],[-1,1],[0,1],[1,1],[-1,0],[0,0],[1,0]];
// x, y, visible cell, wall side (right=1/left=-1/front=0), VMP offset, width, height, flip
const operations=[[-2,3,0,1,102,3,5,0],[21,3,6,-1,102,3,5,1],
 [2,3,1,1,102,3,5,0],[17,3,5,-1,102,3,5,1],[8,3,2,1,97,1,5,0],[13,3,4,-1,97,1,5,1],
 [-4,3,1,0,129,6,5,0],[20,3,5,0,129,6,5,0],[2,3,2,0,129,6,5,0],[14,3,4,0,129,6,5,0],[8,3,3,0,129,6,5,0],
 [0,3,7,1,117,2,6,0],[20,3,11,-1,117,2,6,1],[6,2,8,1,81,2,8,0],[14,2,10,-1,81,2,8,1],
 [-4,2,8,0,159,10,8,0],[16,2,10,0,159,10,8,0],[6,2,9,0,159,10,8,0],
 [3,1,12,1,45,3,12,0],[16,1,14,-1,45,3,12,1],[-13,1,12,0,239,16,12,0],[19,1,14,0,239,16,12,0],[3,1,13,0,239,16,12,0],
 [0,0,15,1,0,3,15,0],[19,0,17,-1,0,3,15,1]];
function scene(px,py,dir){
 const front=new Uint16Array(330),floor=new Uint16Array(330);const parity=(px+py+dir)&1;
 for(let y=0;y<15;y++)for(let x=0;x<22;x++){let v=vmp[y*22+(parity?21-x:x)];if(parity)v^=0x4000;floor[y*22+x]=v;}
 for(const [x,y,id,side,offset,w,h,flip] of operations){
  const [s,f]=locations[id],wx=px+dx[dir]*f+dx[(dir+1)&3]*s,wy=py+dy[dir]*f+dy[(dir+1)&3]*s;
  const wallDir=side===0?dir:(dir+(side===1?1:3))&3;
  if(!solid(wx+dx[wallDir],wy+dy[wallDir]))continue;
  for(let yy=0;yy<h;yy++)for(let xx=0;xx<w;xx++){
   const dest=x+xx;if(dest<0||dest>=22)continue;
   let v=vmp[330+offset+yy*w+(flip?w-1-xx:xx)];if(!v)continue;if(flip)v^=0x4000;front[(y+yy)*22+dest]=v;
  }
 }
 const pixels=Buffer.alloc(176*120*3);
 function draw(word,bx,by,transparent){const id=word&0x3fff,flip=word&0x4000;if(id>=count)throw Error(`tile ${id}`);
  for(let y=0;y<8;y++)for(let x=0;x<8;x++){const sx=flip?7-x:x,b=tiles[id*32+y*4+(sx>>1)],n=(sx&1)?b&15:b>>4;if(transparent&&!n)continue;const c=wallPalette[col[n]],at=((by*8+y)*176+bx*8+x)*3;for(let k=0;k<3;k++)pixels[at+k]=c[k];}}
 for(let i=0;i<330;i++){const x=i%22,y=Math.floor(i/22),v=front[i];if(!v||v&0x8000){draw(floor[i],x,y,false);if(v)draw(v&0x7fff,x,y,true);}else draw(v,x,y,false);}
 return pixels;
}
function bitmap(n){const p=decode(read(n)).pixels,b=Buffer.alloc(320*200*3);for(let i=0;i<p.length;i++){const c=basePalette[p[i]];b.set(c,i*3);}return b;}
function blit(src,sw,dst,dw,sx,sy,w,h,x,y,ww=w,hh=h){for(let yy=0;yy<hh;yy++)for(let xx=0;xx<ww;xx++){const si=((sy+Math.floor(yy*h/hh))*sw+sx+Math.floor(xx*w/ww))*3,di=((y+yy)*dw+x+xx)*3;src.copy(dst,di,si,si+3);}}
const ui=Buffer.alloc(256*212*3),play=bitmap('PLAYFLD.CPS'),faces=bitmap('CHARGENA.CPS'),kobold=bitmap('KOBOLD.CPS'),doors=bitmap('DOOR.CPS');
blit(play,320,ui,256,0,120,176,80,0,120);
for(let j=0;j<4;j++){blit(play,320,ui,256,180,0,68,48,178,j*44,76,44);blit(faces,320,ui,256,j*32,0,32,32,181,j*44+4,30,30);}
const atlas=Buffer.alloc(256*160*3);
blit(kobold,320,atlas,256,4,44,44,60,0,0,44,60);
blit(kobold,320,atlas,256,4,44,44,60,44,0,22,30);
blit(kobold,320,atlas,256,4,44,44,60,66,0,12,16);
blit(doors,320,atlas,256,0,0,80,72,80,0,64,96);
blit(doors,320,atlas,256,0,0,80,72,144,0,40,64);
blit(doors,320,atlas,256,0,0,80,72,184,0,24,40);
const items=bitmap('ITEMICN.CPS');
blit(items,320,atlas,256,128,32,16,16,208,0);
blit(items,320,atlas,256,128,32,16,16,224,0,8,8);
const font=read('FONT6.FNT');
for(let color=0;color<3;color++)for(let code=32;code<96;code++){
 const at=font.readUInt16LE(2+code*2),left=((code-32)&31)*8,top=96+((code-32)>>5)*8+color*16;
 const rgb=color===0?[255,219,73]:color===1?[255,255,255]:[182,182,219];
 for(let y=0;y<6;y++)for(let x=0;x<6;x++)if(font[at+y]&(128>>x))atlas.set(rgb,((top+y)*256+left+x)*3);
}
const rawViews=[],viewKeys=[],unique=new Map();
for(let y=0;y<12;y++)for(let x=0;x<12;x++)for(let d=0;d<4;d++){
 if(solid(x,y)){viewKeys.push(-1);continue;}const image=scene(x,y,d),hash=crypto.createHash('sha256').update(image).digest('hex');
 if(!unique.has(hash)){unique.set(hash,rawViews.length);rawViews.push(image);}viewKeys.push(unique.get(hash));
}
// Build one shared palette in the MSX2 RGB333 gamut. Black, gold and white are reserved.
const histogram=new Map();function add(image,weight){for(let i=0;i<image.length;i+=3){const q=[0,1,2].map(k=>Math.round(image[i+k]*7/255)),id=q[0]*64+q[1]*8+q[2];if(!id)continue;histogram.set(id,(histogram.get(id)||0)+weight);}}
add(ui,10);add(atlas,10);for(const v of rawViews)add(v,1);
const points=[...histogram].map(([id,n])=>({v:[id>>6,(id>>3)&7,id&7],n}));
const dist=(a,b)=>a.reduce((s,v,k)=>s+(v-b[k])**2,0);
let centers=[[0,0,0],[7,6,2],[7,7,7]];
while(centers.length<16){let best=points.reduce((a,p)=>{const score=Math.min(...centers.map(c=>dist(c,p.v)))*Math.sqrt(p.n);return score>a.score?{score,v:p.v}:a;},{score:-1});centers.push([...best.v]);}
for(let iter=0;iter<18;iter++){const sum=centers.map(()=>[0,0,0,0]);for(const p of points){let best=0;for(let c=1;c<16;c++)if(dist(p.v,centers[c])<dist(p.v,centers[best]))best=c;for(let k=0;k<3;k++)sum[best][k]+=p.v[k]*p.n;sum[best][3]+=p.n;}for(let c=3;c<16;c++)if(sum[c][3])centers[c]=sum[c].slice(0,3).map(v=>Math.round(v/sum[c][3]));}
centers=[centers[0],...centers.slice(3),centers[1],centers[2]];
const lookup=new Uint8Array(512);for(let id=0;id<512;id++){const v=[id>>6,(id>>3)&7,id&7];let best=0;for(let c=1;c<16;c++)if(dist(v,centers[c])<dist(v,centers[best]))best=c;lookup[id]=best;}
function quant(image,w,h,padded=256){const b=Buffer.alloc(padded*h/2);for(let y=0;y<h;y++)for(let x=0;x<w;x++){const at=(y*w+x)*3,q=[0,1,2].map(k=>Math.round(image[at+k]*7/255)),n=lookup[q[0]*64+q[1]*8+q[2]],i=y*padded/2+(x>>1);b[i]|=n<<((x&1)?0:4);}return b;}
function rle(data){const a=[];let i=0;while(i<data.length){let n=1;while(n<128&&data[i+n]===data[i]&&i+n<data.length)n++;if(n>=3){a.push(128+n-1,data[i]);i+=n;}else{const start=i;i+=n;while(i<data.length&&i-start<128){let r=1;while(r<3&&data[i+r]===data[i]&&i+r<data.length)r++;if(r>=3)break;i++;}const len=i-start;a.push(len-1,...data.subarray(start,i));}}return Buffer.from(a);}
const all=[quant(ui,256,212),quant(atlas,256,160),...rawViews.map(v=>quant(v,176,120))];
all.push(...require('./generate-front.cjs')({root,read,decode,blit,quant,png,base:all.length}));
const compressed=all.map(rle);
// Verify round-trip for every stream before building the cartridge.
for(let n=0;n<all.length;n++){const a=[];for(let i=0;i<compressed[n].length;){const c=compressed[n][i++],len=(c&127)+1;if(c&128){const v=compressed[n][i++];for(let j=0;j<len;j++)a.push(v);}else for(let j=0;j<len;j++)a.push(compressed[n][i++]);}if(!Buffer.from(a).equals(all[n]))throw Error(`RLE roundtrip ${n}`);}
let offset=24576;const meta=compressed.map((b,i)=>{const a={bank:offset>>13,offset:offset&8191,size:all[i].length};offset+=b.length;return a;});
const binary=Buffer.concat(compressed);fs.writeFileSync(path.join(out,'assets.bin'),binary);
let header='/* Generated by generate-assets.cjs. Do not edit. */\n';
header+='typedef struct { unsigned char bank; unsigned int offset,size; } Asset;\n';
header+=`static const Asset assets[${meta.length}]={${meta.map(a=>`{${a.bank},${a.offset},${a.size}}`).join(',')}};\n`;
header+=`static const unsigned int views[576]={${viewKeys.map(k=>k<0?2:k+2).join(',')}};\n`;
header+=`static const Palette palette={{{${centers.map((c,i)=>[i,...c].join(',')).join('},{')}}}};\n`;
header+=`static const char * const maze[12]={${maze.map(s=>JSON.stringify(s)).join(',')}};\n`;
fs.writeFileSync(path.join(root,'src/assets.h'),header);
function preview(packed,w,h,file){const image=Buffer.alloc(w*h*3);for(let y=0;y<h;y++)for(let x=0;x<w;x++){const n=(packed[y*128+(x>>1)]>>((x&1)?0:4))&15;for(let c=0;c<3;c++)image[(y*w+x)*3+c]=Math.round(centers[n][c]*255/7);}png(path.join(out,file),w,h,image);}
preview(all[0],256,212,'ui-msx2.png');preview(all[1],256,160,'atlas-msx2.png');preview(all[viewsIndex(1,10,0)],176,120,'dungeon-msx2.png');
function viewsIndex(x,y,d){return viewKeys[(y*12+x)*4+d]+2;}
const report={tiles:count,uniqueViews:rawViews.length,compressedBytes:binary.length,romBytes:2097152,paletteRGB333:centers,sourceHashes:Object.fromEntries(fs.readdirSync(source).map(n=>[n,crypto.createHash('sha256').update(read(n)).digest('hex')]))};
fs.writeFileSync(path.join(out,'manifest.json'),JSON.stringify(report,null,2));console.log(report);






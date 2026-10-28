// SPDX-License-Identifier: GPL-3.0-or-later
module.exports=({root,read,decode,blit,quant,png,base})=>{
 const fs=require('fs'),path=require('path');
 function rgbImage(name,palette){const d=decode(read(name)),p=d.palette.length===768?d.palette:read(palette||'EOBPAL.COL'),b=Buffer.alloc(64000*3);for(let i=0;i<64000;i++)for(let c=0;c<3;c++)b[i*3+c]=Math.round((p[d.pixels[i]*3+c]&63)*255/63);return b;}
 const dist=(a,b)=>a.reduce((s,v,k)=>s+(v-b[k])**2,0);
 function palette(images){const map=new Map();for(const img of images)for(let i=0;i<img.length;i+=3){const q=[0,1,2].map(c=>Math.round(img[i+c]*7/255)),id=q[0]*64+q[1]*8+q[2];if(id)map.set(id,(map.get(id)||0)+1);}
  const points=[...map].map(([id,n])=>({v:[id>>6,(id>>3)&7,id&7],n}));let cs=[[0,0,0],[7,6,2],[7,7,7]];
  while(cs.length<16){const best=points.reduce((a,p)=>{const score=Math.min(...cs.map(c=>dist(c,p.v)))*Math.sqrt(p.n);return score>a.score?{score,v:p.v}:a;},{score:-1});cs.push([...best.v]);}
  for(let it=0;it<15;it++){const sum=cs.map(()=>[0,0,0,0]);for(const p of points){let j=0;for(let c=1;c<16;c++)if(dist(p.v,cs[c])<dist(p.v,cs[j]))j=c;for(let k=0;k<3;k++)sum[j][k]+=p.v[k]*p.n;sum[j][3]+=p.n;}for(let c=3;c<16;c++)if(sum[c][3])cs[c]=sum[c].slice(0,3).map(v=>Math.round(v/sum[c][3]));}
  return [cs[0],...cs.slice(3),cs[1],cs[2]];
 }
 function pack(img,pal,h=212){const table=new Uint8Array(512);for(let id=0;id<512;id++){const v=[id>>6,(id>>3)&7,id&7];let j=0;for(let c=1;c<16;c++)if(dist(v,pal[c])<dist(v,pal[j]))j=c;table[id]=j;}const b=Buffer.alloc(128*h);for(let i=0;i<256*h;i++){const at=i*3,q=[0,1,2].map(c=>Math.round(img[at+c]*7/255)),n=table[q[0]*64+q[1]*8+q[2]];b[i>>1]|=n<<((i&1)?0:4);}return b;}
 const pictures=[],palettes=[];
 const specs=[['WESTWOOD.CMP','WESTWOOD.COL',0,0,320,200],['TITLE-V.CMP','TITLE-V.COL',0,0,320,200],['WTRDP2.CMP','WTRDP2.COL',0,0,160,136],['KING.CMP','WTRDP2.COL',48,0,224,184],['TUNNEL.CMP','ZOOMTUNL.COL',0,0,160,120],['ZOOMTUNL.CMP','ZOOMTUNL.COL',0,0,160,120]];
 for(const [file,pal,x,y,w,h]of specs){const image=Buffer.alloc(256*212*3);blit(rgbImage(file,pal),320,image,256,x,y,w,h,0,0,256,160);const p=palette([image]);pictures.push(pack(image,p));palettes.push(p);}
 const creator=Buffer.alloc(256*212*3);blit(rgbImage('CHARGEN.CPS'),320,creator,256,0,0,320,200,0,0,256,200);
 const portraits=Buffer.alloc(256*192*3),faces=rgbImage('CHARGENA.CPS');
 for(let i=0;i<44;i++)blit(faces,320,portraits,256,(i%10)*32,Math.floor(i/10)*32,32,32,(i&7)*32,(i>>3)*32);
 const cp=palette([creator,portraits,portraits]);palettes.push(cp);pictures.push(pack(creator,cp),pack(portraits,cp,192),quant(portraits,256,192));
 let h='/* Generated frontend resources. */\n';
 for(const [i,name]of ['LOGO','TITLE','CITY','KING','TUNNEL','BLOCKED','CREATOR','FACES','GAME_FACES'].entries())h+=`#define FRONT_${name} ${base+i}\n`;
 h+='static const Palette front_palettes[7]={'+palettes.map(p=>'{{'+p.map((c,i)=>'{'+[i,...c].join(',')+'}').join(',')+'}}').join(',')+'};\n';
 fs.writeFileSync(path.join(root,'src/front_assets.h'),h);
 for(let i=0;i<7;i++){const image=Buffer.alloc(256*212*3);for(let n=0;n<256*212;n++){const v=(pictures[i][n>>1]>>((n&1)?0:4))&15;for(let c=0;c<3;c++)image[n*3+c]=Math.round(palettes[i][v][c]*255/7);}png(path.join(root,'assets/generated',`front-${i}.png`),256,212,image);}
 return pictures;
};

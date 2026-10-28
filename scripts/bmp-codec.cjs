// SPDX-License-Identifier: GPL-3.0-or-later
// Format-80 decoder follows the documented implementation in ScummVM screen.cpp.
// Reference: https://github.com/scummvm/scummvm/blob/master/engines/kyra/graphics/screen.cpp
const fs=require('fs'),path=require('path'),zlib=require('zlib');
function decode(b){
 const type=b[2],size=b.readUInt32LE(4),palSize=b.readUInt16LE(8);let i=10+palSize,o=0;
 if(!size||size>131072)throw Error(`Unexpected bitmap size ${size}`);
 const dst=Buffer.alloc(size);
 function byte(){if(i>=b.length)throw Error('Truncated compressed stream');return b[i++];}
 function word(){const a=byte();return a|(byte()<<8);}
 function copy(src,len){if(src<0||src>=o||o+len>size)throw Error('Invalid backreference');for(let n=0;n<len;n++)dst[o++]=dst[src++];}
 function fill(n,v){if(o+n>size)throw Error('Invalid fill');dst.fill(v,o,o+n);o+=n;}
 if(type===0){if(b.length-i<size)throw Error('Truncated raw image');b.copy(dst,0,i,i+size);o=size;}
 else if(type===1){
  let nib=0;const patterns=[];
  function code(){if(i+1>=b.length)throw Error('Truncated 12-bit code');let c=b.readUInt16BE(i++);if(++nib&1)c>>=4;else{i++;c&=4095;}return c;}
  let last=code()&255,prev=0,count=1,countPrev=1;dst[o++]=last;
  while(o<size){let c=code(),cmd=c>>8;
   if(cmd){c=((cmd-1)<<8)|(c&255);const start=o;
    if(c<patterns.length){const [at,n]=patterns[c];countPrev=n;last=dst[at];copy(at,n);}
    else{count=countPrev;copy(prev,countPrev);if(o>=size)throw Error('LZW overflow');dst[o++]=last;countPrev++;}
    if(patterns.length<3840)patterns.push([prev,++count]);prev=start;count=countPrev;
   }else{dst[o++]=last=c&255;if(patterns.length<3840)patterns.push([prev,++count]);prev=o-1;count=countPrev=1;}
  }
 }
 else if(type===4){while(o<size){const c=byte();if(c===128)break;if(!(c&128)){const n=(c>>4)+3,off=((c&15)<<8)|byte();copy(o-off,n);}else if(!(c&64)){const n=c&63;for(let k=0;k<n;k++){if(o>=size)throw Error('Overflow');dst[o++]=byte();}}else if(c===254){const n=word();fill(n,byte());}else{const n=c===255?word():(c&63)+3;copy(word(),n);}}}
 else if(type===3){while(o<size){let c=byte();if(c===0){const n=(byte()<<8)|byte();fill(n,byte());}else if(c&128)fill(256-c,byte());else for(let k=0;k<c;k++)dst[o++]=byte();}}
 else throw Error(`Unsupported compression ${type}`);
 if(o!==size)throw Error(`Decoded ${o}/${size}`);
 return {pixels:dst,palette:b.subarray(10,10+palSize),type};
}
function crc(b){let c=0xffffffff;for(const v of b){c^=v;for(let j=0;j<8;j++)c=(c>>>1)^((c&1)?0xedb88320:0);}return (c^0xffffffff)>>>0;}
function chunk(tag,data){const t=Buffer.from(tag),n=Buffer.alloc(4),c=Buffer.alloc(4);n.writeUInt32BE(data.length);c.writeUInt32BE(crc(Buffer.concat([t,data])));return Buffer.concat([n,t,data,c]);}
function png(file,w,h,rgb){const ih=Buffer.alloc(13);ih.writeUInt32BE(w);ih.writeUInt32BE(h,4);ih[8]=8;ih[9]=2;const raw=Buffer.alloc((w*3+1)*h);for(let y=0;y<h;y++)rgb.copy(raw,y*(w*3+1)+1,y*w*3,(y+1)*w*3);fs.writeFileSync(file,Buffer.concat([Buffer.from('89504e470d0a1a0a','hex'),chunk('IHDR',ih),chunk('IDAT',zlib.deflateSync(raw)),chunk('IEND',Buffer.alloc(0))]));}
module.exports={decode,png};

#include "msx_fusion.h"
#include "audio.h"
#include "music.h"
typedef unsigned char u8;
typedef unsigned int u16;
void AudioInstallHook(void);void AudioRemoveHook(void);
u8 AudioReadSlot(u8 slot,u16 address);void AudioWriteSlot(u8 slot,u16 address,u8 value);
void AudioPSG(u8 reg,u8 value);void AudioFM(u8 reg,u8 value);void AudioOPL(u8 reg,u8 value);
extern volatile u8 stream_bank;
volatile u8 music_backend,music_found,music_enabled,music_loops;
volatile u16 music_clock;
static u8 scc_slot,psg_io,refresh_rate,accumulator,track,score_bank,frame_count;
static const u8 *score;
static u16 wait_ticks,periods[9];
static u8 volumes[9],presets[9],sorted[9];
static volatile u8 request;
static u8 effect,age;
static u8 user_muted;
static const signed char wave[32]={0,18,35,50,64,76,85,91,94,91,85,76,64,50,35,18,0,-18,-35,-50,-64,-76,-85,-91,-94,-91,-85,-76,-64,-50,-35,-18};
static void scc(u16 address,u8 v){AudioWriteSlot(scc_slot,address,v);}
static u8 fetch(void){u8 v=*score++;if(score==(const u8 *)0xc000){score=(const u8 *)0xa000;*(volatile u8 *)0x7800=++score_bank;}return v;}
static void next_frame(void){wait_ticks=fetch();wait_ticks|=(u16)fetch()<<8;frame_count=fetch();}
static void silence(void){u8 i;
 for(i=0;i<3;i++)AudioPSG(8+i,0);
 if(music_found&1)for(i=0;i<5;i++)scc(0x988a+i,0);
 if(music_found&2)for(i=0;i<9;i++)AudioFM(0x20+i,0);
 if(music_found&4)for(i=0;i<9;i++)AudioOPL(0xb0+i,0);
}
static void start(void){u8 id,i;
 silence();id=(track<<1)+(music_backend==3);
 score_bank=music_banks[id];score=(const u8 *)(0xa000+music_offsets[id]);
 *(volatile u8 *)0x7800=score_bank;
 next_frame();for(i=0;i<9;i++){periods[i]=0;volumes[i]=0;}
 *(volatile u8 *)0x7800=stream_bank;
}
static u8 probe_scc(u8 slot){u8 old=AudioReadSlot(slot,0x9800);
 AudioWriteSlot(slot,0x9800,old^0x55);
 if(AudioReadSlot(slot,0x9800)==(u8)(old^0x55)){AudioWriteSlot(slot,0x9800,old);return 0;}
 AudioWriteSlot(slot,0x9000,0x3f);AudioWriteSlot(slot,0x9800,0x55);if(AudioReadSlot(slot,0x9800)!=0x55)return 0;
 AudioWriteSlot(slot,0x9800,0xaa);if(AudioReadSlot(slot,0x9800)!=0xaa)return 0;
 AudioWriteSlot(slot,0x9000,0);AudioWriteSlot(slot,0x9800,0x77);if(AudioReadSlot(slot,0x9800)==0x77)return 0;
 AudioWriteSlot(slot,0x9000,0x3f);return 1;
}
void AudioInit(void){u8 p,s,n,slot,i,j,internal=0,fm_slot=255;u16 delay;
 __asm di __endasm;
 music_found=0;music_backend=0;music_enabled=1;music_clock=0;music_loops=0;accumulator=0;track=0;request=0;effect=0;user_muted=0;
 refresh_rate=(Peek(0x002b)&0x80)?50:60;
 OutPort(0xa0,7);psg_io=InPort(0xa2)&0xc0;AudioPSG(7,psg_io|0x38);
 for(p=0;p<4;p++){
  n=(Peek(0xfcc1+p)&0x80)?4:1;
  for(s=0;s<n;s++){
   slot=p;if(n==4)slot|=0x80|(s<<2);
   if(AudioReadSlot(slot,0x401c)=='O' && AudioReadSlot(slot,0x401d)=='P' && AudioReadSlot(slot,0x401e)=='L' && AudioReadSlot(slot,0x401f)=='L'){
    if(AudioReadSlot(slot,0x4018)=='A'){internal=1;fm_slot=slot;}else if(!internal)fm_slot=slot;
   }
   if(p && !(music_found&1) && probe_scc(slot)){music_found|=1;scc_slot=slot;}
  }
 }
 if(fm_slot!=255){music_found|=2;if(!internal)AudioWriteSlot(fm_slot,0x7ff6,AudioReadSlot(fm_slot,0x7ff6)|1);}
 /* AdLib timer signature at the MoonSound FM ports; absent ports read 0xff. */
 if(!(InPort(0xc4)&0xe0)){
  AudioOPL(4,0x60);AudioOPL(4,0x80);AudioOPL(2,255);AudioOPL(4,0x21);
  for(delay=0;delay<1000;delay++){__asm nop __endasm;}
  if((InPort(0xc4)&0xc0)==0xc0)music_found|=4;
  AudioOPL(4,0x60);AudioOPL(4,0x80);
 }
 if(music_found&1){for(i=0;i<4;i++)for(j=0;j<32;j++)scc(0x9800+((u16)i<<5)+j,(u8)wave[j]);scc(0x988f,31);music_backend=1;}
 if(music_found&2){AudioFM(0x0e,0);music_backend=2;}
 if(music_found&4){AudioOPL(1,0x20);music_backend=3;}
 start();AudioInstallHook();__asm ei __endasm;
}
void AudioStop(void){__asm di __endasm;silence();AudioRemoveHook();__asm ei __endasm;}
void AudioEffect(u8 kind){request=kind;}
void AudioToggle(void){__asm di __endasm;user_muted^=1;music_enabled=!user_muted;start();__asm ei __endasm;}
void AudioCycle(void){u8 b;__asm di __endasm;b=music_backend;do{b=(b+1)&3;}while(b && !(music_found&(1<<(b-1))));music_backend=b;start();__asm ei __endasm;}
void AudioSelect(u8 index){if(index>4)return;__asm di __endasm;track=index;music_enabled=!user_muted;start();__asm ei __endasm;}
void AudioEnding(void){AudioSelect(1);}
void AudioRestart(void){effect=0;request=0;AudioSelect(0);}
void AudioTick(void){u8 i,j,c,n,v,reg,lo,hi,packed,total,idx,mixer;u16 p;
 accumulator+=50;if(accumulator<refresh_rate)return;accumulator-=refresh_rate;music_clock++;
 *(volatile u8 *)0x7800=score_bank;
 if(wait_ticks)wait_ticks--;
 if(!wait_ticks){
  if(frame_count==255){if(!track){music_loops++;start();*(volatile u8 *)0x7800=score_bank;}else{silence();music_enabled=0;frame_count=0;wait_ticks=65535;}}
  n=frame_count;
  for(i=0;i<n;i++){
   if(music_backend==3){reg=fetch();v=fetch();if(music_enabled)AudioOPL(reg,v);}
   else {c=fetch();lo=fetch();hi=fetch();periods[c]=lo|((u16)hi<<8);lo=fetch();hi=fetch();packed=fetch();volumes[c]=packed&15;presets[c]=packed>>4;
    if(music_backend==2){AudioFM(0x10+c,lo);AudioFM(0x30+c,(packed&0xf0)|(15-(music_enabled?volumes[c]:0)));AudioFM(0x20+c,hi|(volumes[c]&&music_enabled?16:0));}
   }
  }
  if(music_enabled||!track)next_frame();
 }
 *(volatile u8 *)0x7800=stream_bank;
 total=0;
 for(i=0;i<9;i++)if(volumes[i]&&periods[i]){j=total;while(j && periods[sorted[j-1]]>periods[i]){sorted[j]=sorted[j-1];j--;}sorted[j]=i;total++;}
 if(music_backend<2){n=music_backend?5:3;for(i=0;i<n;i++){
  v=0;p=1;
  if(i<total && music_enabled){idx=i==0?0:i==1?total-1:(u8)(i-1);if(idx<total){c=sorted[idx];v=volumes[c];if(v>12)v=12;if(i>1&&v>2)v-=2;p=periods[c];}}
  if(music_backend){if(p)p--;scc(0x9880+((u16)i<<1),(u8)p);scc(0x9881+((u16)i<<1),p>>8);scc(0x988a+i,v);}
  else {AudioPSG(i<<1,(u8)p);AudioPSG((i<<1)+1,p>>8);AudioPSG(8+i,v);}
 }}
 mixer=music_backend?0x3f:0x38;
 if(request){effect=request;request=0;age=0;}
 if(effect){v=0;p=100;
  if(effect==FX_MOVE||effect==FX_BLOCK||effect==FX_HIT||effect==FX_DOOR||effect==FX_KILL||effect==FX_DEATH){
   n=effect==FX_MOVE?4:effect==FX_BLOCK?5:effect==FX_HIT?9:effect==FX_DOOR?16:24;
   if(age<n){v=effect==FX_MOVE?7-(age>>1):13-(age>>1);AudioPSG(6,3+(age&15));mixer=(mixer|4)&~32;}
  }else if(age<18){v=12-(age>>1);p=age<6?214:age<12?170:143;mixer&=~4;}
  if(v){AudioPSG(4,(u8)p);AudioPSG(5,p>>8);AudioPSG(10,v);age++;}
  else {effect=0;if(music_backend)AudioPSG(10,0);}
 }
 AudioPSG(7,psg_io|mixer);
}



/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "msx_fusion.h"
#include "vdp_graph2.h"
#include "assets.h"
#include "audio.h"
#include "front.h"
static const signed char dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
/* Stable first six bytes for emulator fixtures. */
static unsigned char px,py,dir,showmap,door,has_key;
static unsigned char enemy_hp,party_hp,potions,won,ready;
static char *message;
static unsigned char combat_target;
unsigned char matrix(unsigned char row);
void stream(unsigned char bank,unsigned int offset,unsigned char segment,unsigned int size);
unsigned char UiPoll(void){
 unsigned char k=0,r,i;
 r=matrix(0);for(i=0;i<8;i++)if(!(r&(1<<i)))k='0'+i;
 r=matrix(2);if(!(r&64))k='A';else if(!(r&128))k='B';
 r=matrix(3);for(i=0;i<8;i++)if(!(r&(1<<i)))k='C'+i;
 r=matrix(4);for(i=0;i<8;i++)if(!(r&(1<<i)))k='K'+i;
 r=matrix(5);for(i=0;i<8;i++)if(!(r&(1<<i)))k='S'+i;
 r=matrix(8);if(!(r&32))k='W';else if(!(r&64))k='S';else if(!(r&16))k='Q';else if(!(r&128))k='E';else if(!(r&1))k=' ';
 r=matrix(7);if(!(r&4))k=27;else if(!(r&128))k=13;else if(!(r&32))k=8;
 if(!(matrix(6)&64))k=0x82;
 return k;
}
void UiRelease(void){while(matrix(0)!=255||matrix(2)!=255||matrix(3)!=255||matrix(4)!=255||matrix(5)!=255||matrix(6)!=255||matrix(7)!=255||matrix(8)!=255){}}
unsigned char UiRead(void){unsigned char k;do{k=UiPoll();}while(!k);UiRelease();return k;}
static unsigned char readkey(void){return UiRead();}
void VideoWait(void);
static void copy(int sx,int sy,int x,int y,int w,int h){VideoWait();HMMM(sx,sy,x,y,w,h);}
static void blend(int sx,int sy,int x,int y,int w,int h,unsigned char op){VideoWait();LMMM(sx,sy,x,y,w,h,op);}
static void box(int x,int y,int xx,int yy,char c){if(xx<x+2)xx=x+2;if(yy<=y)yy=y+1;VideoWait();BoxFill(x,y,xx,yy,c,0);}
static void text(int x,int y,char *s,char color){
 unsigned char c;int source_y=color==14?608:color==15?624:640;
 while(*s){c=*s++-32;if(c<64)copy((c&31)<<3,source_y+((c>>5)<<3),x,y,6,6);x+=6;}
}
static void load(unsigned int id,unsigned char segment){const Asset *a=&assets[id];stream(a->bank,a->offset,segment,a->size);}
void UiLoad(unsigned int id,unsigned char segment){load(id,segment);}
void UiText(int x,int y,char *s,char c){text(x,y,s,c);}
void UiBox(int x,int y,int xx,int yy,char c){box(x,y,xx,yy,c);}
void UiCopy(int sx,int sy,int x,int y,int w,int h){copy(sx,sy,x,y,w,h);}
static unsigned char cell(int x,int y){
 if(x<0 || y<0 || x>11 || y>11)return 1;
 if(x==6 && y==1 && !door)return 2;
 return maze[y][x]=='#';
}
static unsigned char sample(int depth,int side){return cell(px+dx[dir]*depth+dx[(dir+1)&3]*side,py+dy[dir]*depth+dy[(dir+1)&3]*side);}
static unsigned char enemy_distance(void){
 int i;if(!enemy_hp)return 0;
 for(i=1;i<=3;++i){if(sample(i,0))return 0;if(px+dx[dir]*i==1 && py+dy[dir]*i==8)return i;}
 return 0;
}
static void minimap(void){
 unsigned char x,y;box(0,0,175,119,0);
 for(y=0;y<12;++y)for(x=0;x<12;++x)box(27+x*10,3+y*9,34+x*10,9+y*9,cell(x,y)?11:0);
 box(27+px*10,3+py*9,34+px*10,9+py*9,14);
 if(enemy_hp)box(38,76,44,81,7);
}
static void render(void){
 unsigned char i,d;unsigned int view;ready=0;
 view=views[(((unsigned int)py*12+px)<<2)+dir];load(view,2);
 for(i=1;i<=3;++i){d=sample(i,0);if(d==2){
  if(i==1)blend(80,512,56,264,64,96,8);
  else if(i==2)blend(144,512,68,276,40,64,8);
  else blend(184,512,76,280,24,40,8);break;
 }if(d)break;}
 if(!has_key){
  for(i=1;i<=2;++i){if(sample(i,0))break;
   if(px+dx[dir]*(int)i==1 && py+dy[dir]*(int)i==7){
    if(i==1)blend(208,512,80,356,16,16,8);else blend(224,512,84,334,8,8,8);
   }
  }
 }
 d=enemy_distance();
 if(d==1)blend(0,512,66,308,44,60,8);
 else if(d==2)blend(44,512,77,310,22,30,8);
 else if(d==3)blend(66,512,82,306,12,16,8);
 copy(0,256,0,0,176,120);if(showmap)minimap();
 box(0,168,175,211,0);text(3,170,message,14);
 text(3,184,"SPACE HIT H HEAL M MAP",15);text(3,198,"P GRUPPO  F2 MENU R RESET",11);
 for(i=0;i<4;++i){
  char hp[6];int width;
  box(214,i*44+4,255,i*44+32,0);
  text(215,i*44+5,heroes[i].klass==0?"GUE":heroes[i].klass==1?"RAN":heroes[i].klass==2?"PAL":heroes[i].klass==3?"MAG":heroes[i].klass==4?"CHI":"LAD",15);
  hp[0]='0'+heroes[i].hp/10;hp[1]='0'+heroes[i].hp%10;hp[2]='/';hp[3]='0'+heroes[i].maxhp/10;hp[4]='0'+heroes[i].maxhp%10;hp[5]=0;
  text(215,i*44+15,hp,15);width=(int)heroes[i].hp*36/heroes[i].maxhp;
  if(width)box(215,i*44+26,214+width,i*44+29,14);
  box(179,i*44+35,255,i*44+42,0);text(181,i*44+36,heroes[i].name,15);
 }
 box(179,179,255,211,0);text(181,181,dir==0?"NORTH":dir==1?"EAST":dir==2?"SOUTH":"WEST",14);
 text(181,195,has_key?"KEY: YES":"KEY: NO",15);
 text(181,205,music_enabled?(music_backend==0?"PSG":music_backend==1?"SCC":music_backend==2?"FM-PAC":"OPL FM"):"MUTED",11);ready=1;
}
static void move(int forward,int strafe){
 int nx=px+dx[dir]*forward+dx[(dir+1)&3]*strafe,ny=py+dy[dir]*forward+dy[(dir+1)&3]*strafe;
 if(cell(nx,ny)){AudioEffect(FX_BLOCK);message="THE WAY IS BLOCKED";return;}
 if(enemy_hp && nx==1 && ny==8){AudioEffect(FX_BLOCK);message="A KOBOLD BLOCKS YOU";return;}
 px=nx;py=ny;AudioEffect(FX_MOVE);message="FOOTSTEPS IN THE DARK";
 if(px==1 && py==7 && !has_key){has_key=1;AudioEffect(FX_KEY);message="BRASS KEY COLLECTED";}
 if(px==10 && py==1){won=1;AudioEnding();AudioEffect(FX_WIN);message="EXIT REACHED - YOU WIN";}
}
static void reset(void){
 AudioRestart();
 HeroesRestore();combat_target=0;
 px=1;py=10;dir=0;showmap=0;door=0;has_key=0;enemy_hp=12;party_hp=party_max;potions=2;won=0;
 SetSC5Palette((Palette *)&palette);message="EXPLORE THE SEWERS";load(0,0);load(1,4);FrontGameFaces();render();
}
static void hurt(void){unsigned char i,damage;
 i=combat_target;combat_target^=1;
 if(!heroes[i].hp)for(i=0;i<4;i++)if(heroes[i].hp)break;
 if(i==4)return;
 damage=heroes[i].hp>6?6:heroes[i].hp;heroes[i].hp-=damage;party_hp-=damage;
}
static unsigned char attack_power(void){unsigned char i;
 for(i=0;i<4;i++)if(heroes[i].hp)return 2+(heroes[i].stats[0]>=12)+(heroes[i].stats[0]>=16);
 return 0;
}
void main(void){
 unsigned char key;
 if((*(volatile unsigned char *)0x002d)==0){Screen(0);Print("MSX2 REQUIRED");return;}
 Screen(5);SetSC5Palette((Palette *)&palette);SetColors(15,0,0);KeySound(0);AudioInit();load(1,4);
 if(!FrontRun()){AudioStop();return;}reset();
 while(1){
  key=readkey();if(key==27)break;
  if(key==0x82){if(!FrontRun())break;reset();continue;}
  if(key=='P'){FrontInspect();SetSC5Palette((Palette *)&palette);load(0,0);FrontGameFaces();render();continue;}
  if(key=='U'){AudioToggle();render();continue;}if(key=='B'){AudioCycle();render();continue;}if(key=='R'){reset();continue;}
  if(!party_hp || won){message=won?"VICTORY! R TO RESTART":"PARTY LOST - PRESS R";render();continue;}
  if(key=='W')move(1,0);else if(key=='S')move(-1,0);else if(key=='A')move(0,-1);else if(key=='D')move(0,1);
  else if(key=='Q')dir=(dir+3)&3;else if(key=='E')dir=(dir+1)&3;else if(key=='M')showmap=!showmap;
  else if(key=='F'){
   if(sample(1,0)==2){if(has_key){door=1;AudioEffect(FX_DOOR);message="THE LOCK CLICKS OPEN";}else message="A BRASS KEY IS NEEDED";}
   else message="NOTHING TO OPEN";
  }else if(key==' '){
   if(enemy_distance()==1){unsigned char power=attack_power();enemy_hp=enemy_hp>power?enemy_hp-power:0;AudioEffect(enemy_hp?FX_HIT:FX_KILL);if(enemy_hp){hurt();if(!party_hp)AudioEffect(FX_DEATH);message=party_hp?"HIT! KOBOLD STRIKES":"PARTY LOST - PRESS R";}else message="THE KOBOLD FALLS";}
   else message="NO ENEMY IN REACH";
  }else if(key=='H'){
   if(potions && party_hp<party_max){--potions;HeroesRestore();party_hp=party_max;AudioEffect(FX_HEAL);message="HEALING POTION USED";}else message="NO HEALING NEEDED";
  }else continue;
  render();
 }
 AudioStop();Screen(0);Print("BEHOLDER MSX2 DEMO");
}









/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "msx_fusion.h"
#include "vdp_graph2.h"
#include "audio.h"
#include "front.h"
#include "front_assets.h"
#include "front_rules.h"
typedef unsigned char u8;
typedef unsigned int u16;
Hero heroes[4];
u8 party_max;
volatile u8 front_phase,intro_scene;
volatile u8 front_ready;
static u8 focus,stat_index,confirmed;
static u16 random_state;
static const char * const race_names[6]={"UMANO","ELFO","MEZZELFO","NANO","HALFLING","GNOMO"};
static const char * const class_names[6]={"GUERRIERO","RANGER","PALADINO","MAGO","CHIERICO","LADRO"};
static const char * const align_names[9]={"LEGALE BUONO","NEUTR. BUONO","CAOTICO BUONO","LEGALE NEUTRO","NEUTRALE","CAOTICO NEUTRO","LEGALE MALV.","NEUTR. MALV.","CAOTICO MALV."};
static const char * const stat_names[6]={"FOR","DES","COS","INT","SAG","CAR"};
static const char * const defaults[4]={"DORN","LYRA","VAEL","MIRA"};
static const u8 default_class[4]={0,1,4,3},default_face[4]={0,3,6,1};
static const u8 default_stats[4][6]={{17,13,16,9,10,10},{16,16,15,10,14,11},{13,11,15,10,17,12},{9,14,12,17,12,10}};
static const u8 class_hp[6]={10,10,10,4,8,6},class_ac[6]={6,7,6,10,8,8};
static const u8 allowed_classes[6]={63,41,59,49,33,57};
static void strcopy(char *d,const char *s){while((*d++=*s++)!=0){}}
static void number(int x,int y,u8 n,u8 c){char s[4];s[0]='0'+n/10;s[1]='0'+n%10;s[2]=0;UiText(x,y,s,c);}
static void frame(int x,int y,int w,int h,u8 c){UiBox(x,y,x+w,y,c);UiBox(x,y+h,x+w,y+h,c);UiBox(x,y,x,y+h,c);UiBox(x+w,y,x+w,y+h,c);}
static void bounds(Hero *h){u8 i,lo;
 for(i=0;i<6;i++){lo=race_min[h->race][i];if(class_min[h->klass][i]>lo)lo=class_min[h->klass][i];if(h->stats[i]<lo)h->stats[i]=lo;if(h->stats[i]>race_max[h->race][i])h->stats[i]=race_max[h->race][i];}
 if(h->klass==2)h->alignment=0;else if(h->klass==1 && h->alignment>2)h->alignment=1;
}
static void derive(Hero *h){int hp=class_hp[h->klass],bonus=0;
 if(h->stats[2]>=15)bonus=h->stats[2]-14;else if(h->stats[2]<=6)bonus=-1;
 if(h->klass>=3 && bonus>2)bonus=2;
 hp+=bonus;h->maxhp=hp>0?hp:1;
 h->ac=class_ac[h->klass];if(h->stats[1]>14){bonus=h->stats[1]-14;h->ac=h->ac>bonus?h->ac-bonus:0;}
}
void HeroesRestore(void){u8 i;party_max=0;for(i=0;i<4;i++){derive(&heroes[i]);heroes[i].hp=heroes[i].maxhp;party_max+=heroes[i].maxhp;}}
void HeroesDefaults(void){u8 i,j;
 for(i=0;i<4;i++){strcopy(heroes[i].name,defaults[i]);for(j=0;j<6;j++)heroes[i].stats[j]=default_stats[i][j];heroes[i].klass=default_class[i];heroes[i].portrait=default_face[i];heroes[i].race=0;heroes[i].sex=i&1;heroes[i].alignment=1;}
 HeroesRestore();confirmed=0;focus=0;stat_index=0;
}
static void face(u8 index,int x,int y){u8 n=heroes[index].portrait;UiCopy((n&7)<<5,768+((n>>3)<<5),x,y,32,32);}
void FrontGameFaces(void){u8 i;UiLoad(FRONT_GAME_FACES,6);for(i=0;i<4;i++)face(i,181,i*44+4);}
static void picture(u8 index,u8 fade){u8 i,j;u16 start;Palette pal;
 front_ready=0;
 for(i=0;i<16;i++){pal.rgb[i].colour=i;pal.rgb[i].R=pal.rgb[i].G=pal.rgb[i].B=0;}
 SetSC5Palette(&pal);UiLoad(FRONT_LOGO+index,0);
 if(!fade){SetSC5Palette((Palette *)&front_palettes[index]);return;}
 for(j=1;j<=4;j++){
  for(i=0;i<16;i++){pal.rgb[i].R=(front_palettes[index].rgb[i].R*j)>>2;pal.rgb[i].G=(front_palettes[index].rgb[i].G*j)>>2;pal.rgb[i].B=(front_palettes[index].rgb[i].B*j)>>2;}
  SetSC5Palette(&pal);start=music_clock;while((u16)(music_clock-start)<2){}
 }
}
static void intro(void){
 static const u16 delays[6]={350,400,650,650,550,450};
 static const char * const lines[6]={"WESTWOOD ASSOCIATES","EYE OF THE BEHOLDER","IL MALE MINACCIA WATERDEEP.","I LORD CHIEDONO IL VOSTRO AIUTO.","IL GRUPPO ENTRA NELLE FOGNE.","LA VIA DEL RITORNO E' BLOCCATA."};
 u8 i,k;u16 start;
 front_phase=0;AudioSelect(2);
 for(i=0;i<6;i++){
  intro_scene=i;if(i==2)AudioSelect(3);if(i==5)AudioSelect(4);
  picture(i,1);UiText(8,171,(char *)lines[i],14);UiText(8,186,"MUSICA ORIGINALE: PAUL MUDRA",15);UiText(8,201,"INVIO AVANTI  SPAZIO SALTA INTRO",15);
  front_ready=1;start=music_clock;while((u16)(music_clock-start)<delays[i]){
   k=UiPoll();if(k){UiRelease();if(k==' '||k==27)return;if(k==13)break;if(k=='U')AudioToggle();}
  }
 }
}
static void creator_draw(void){u8 i;Hero *h=&heroes[focus];
 picture(6,0);
 for(i=0;i<4;i++){
  UiBox(4,42+i*36,105,76+i*36,0);frame(4,42+i*36,101,34,i==focus?14:15);face(i,6,43+i*36);
  UiText(41,46+i*36,heroes[i].name,15);UiText(41,57+i*36,(char *)class_names[heroes[i].klass],14);
  UiText(41,68+i*36,(confirmed&(1<<i))?"PRONTO":"IN ATTESA",15);
 }
 UiBox(111,42,251,186,0);
 UiText(116,45,h->name,14);UiText(116,58,(char *)class_names[h->klass],15);
 UiText(116,70,(char *)race_names[h->race],15);UiText(116,82,(char *)align_names[h->alignment],15);
 UiText(116,94,h->sex?"SESSO: F":"SESSO: M",15);
 for(i=0;i<6;i++){UiText(118,107+i*9,i==stat_index?">":" ",14);UiText(130,107+i*9,(char *)stat_names[i],15);number(177,107+i*9,h->stats[i],i==stat_index?14:15);}
 derive(h);UiText(116,166,"PF",14);number(136,166,h->maxhp,15);UiText(172,166,"CA",14);number(194,166,h->ac,15);
 UiText(116,178,"1-4 CAMBIA PERSONAGGIO",15);
 UiBox(0,187,255,211,0);
 UiText(2,188,"Q/E VOLTO C CLASSE R RAZZA F SESSO",15);
 UiText(2,196,"W/S STAT A/D +/- T DADI N NOME G ALLIN.",15);
 UiText(2,204,"INVIO CONFERMA   ESC MENU",14);
 front_ready=1;
}
static void name_edit(void){Hero *h=&heroes[focus];u8 n=0,k;while(h->name[n])n++;
 UiBox(111,42,251,54,0);UiText(116,45,"NOME: INVIO PER FINIRE",14);
 while(1){UiBox(116,58,245,66,0);UiText(116,58,h->name,15);k=UiRead();if(k==13||k==27)break;if(k==8&&n)h->name[--n]=0;else if(k>='A'&&k<='Z'&&n<8){h->name[n++]=k;h->name[n]=0;}}
 if(!n)strcopy(h->name,"HERO");
}
static void roll(Hero *h){u8 i,j;random_state^=music_clock;
 for(i=0;i<6;i++){h->stats[i]=0;for(j=0;j<3;j++){random_state=random_state*25173u+13849u;h->stats[i]+=1+(random_state>>8)%6;}}bounds(h);
}
static void review(u8 inspect){u8 i,j;Hero *h;
 picture(6,0);UiBox(2,38,253,186,0);
 for(i=0;i<4;i++){
  int x=5+(i&1)*125,y=42+(i>>1)*71;h=&heroes[i];frame(x,y,120,67,14);face(i,x+3,y+3);
  UiText(x+40,y+4,h->name,15);UiText(x+40,y+15,(char *)class_names[h->klass],14);UiText(x+40,y+26,(char *)race_names[h->race],15);
  UiText(x+40,y+37,"PF",14);number(x+58,y+37,inspect?h->hp:h->maxhp,15);UiText(x+79,y+37,"CA",14);number(x+97,y+37,h->ac,15);
  for(j=0;j<6;j++)number(x+3+j*19,y+53,h->stats[j],15);
 }
 UiText(5,188,"FOR DES COS INT SAG CAR",14);
 UiText(5,199,inspect?"ESC TORNA AL DUNGEON":"INVIO INIZIA  1-4 MODIFICA  ESC MENU",15);
 front_ready=1;
}
static u8 edit_group(void){u8 k,i;Hero *h;
 front_phase=2;UiLoad(FRONT_FACES,6);
 while(1){creator_draw();k=UiRead();h=&heroes[focus];
  if(k==27)return 0;
  if(k>='1'&&k<='4')focus=k-'1';
  else if(k=='W')stat_index=(stat_index+5)%6;
  else if(k=='S')stat_index=(stat_index+1)%6;
  else if(k=='Q')h->portrait=h->portrait?h->portrait-1:43;
  else if(k=='E')h->portrait=(h->portrait+1)%44;
  else if(k=='A'){if(h->stats[stat_index]>2)--h->stats[stat_index];bounds(h);}
  else if(k=='D'){++h->stats[stat_index];bounds(h);}
  else if(k=='C'){do{h->klass=(h->klass+1)%6;}while(!(allowed_classes[h->race]&(1<<h->klass)));bounds(h);}
  else if(k=='R'){h->race=(h->race+1)%6;if(!(allowed_classes[h->race]&(1<<h->klass)))h->klass=0;bounds(h);}
  else if(k=='F')h->sex^=1;
  else if(k=='G'){h->alignment=(h->alignment+1)%9;bounds(h);}
  else if(k=='T')roll(h);
  else if(k=='N')name_edit();
  else if(k==13||k==' '){confirmed|=1<<focus;derive(h);if(confirmed==15)return 1;for(i=1;i<=4;i++)if(!(confirmed&(1<<((focus+i)&3)))){focus=(focus+i)&3;break;}}
 }
}
unsigned char FrontRun(void){u8 choice=0,k,go;
 HeroesDefaults();random_state=music_clock+123;intro();AudioSelect(0);
 while(1){front_phase=1;picture(1,0);
  UiText(23,168,choice==0?"> CREA NUOVO GRUPPO":"  CREA NUOVO GRUPPO",14);
  UiText(23,180,choice==1?"> GRUPPO PREDEFINITO":"  GRUPPO PREDEFINITO",14);
  UiText(23,192,choice==2?"> RIVEDI INTRO":"  RIVEDI INTRO",14);
  UiText(23,205,"W/S SCEGLI  INVIO CONFERMA",15);
  front_ready=1;
  k=UiRead();if(k==27)return 0;if(k=='W'){choice=(choice+2)%3;continue;}if(k=='S'){choice=(choice+1)%3;continue;}
  if(k!=13&&k!=' ')continue;
  if(choice==2){intro();AudioSelect(0);continue;}
  HeroesDefaults();if(choice==0){if(!edit_group())continue;}else confirmed=15;
  while(1){front_phase=3;HeroesRestore();UiLoad(FRONT_FACES,6);review(0);k=UiRead();
   if(k==27)break;
   if(k>='1'&&k<='4'){focus=k-'1';go=edit_group();if(!go)break;}
   else if(k==13||k==' '){front_phase=4;return 1;}
  }
 }
}
void FrontInspect(void){u8 k;front_phase=5;UiLoad(FRONT_FACES,6);review(1);do{k=UiRead();}while(k!=27&&k!=13&&k!=' ');front_phase=4;}


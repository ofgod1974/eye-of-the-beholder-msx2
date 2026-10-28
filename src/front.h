#ifndef BEHOLDER_FRONT_H
#define BEHOLDER_FRONT_H
typedef struct {
 char name[9];
 unsigned char stats[6]; /* STR, DEX, CON, INT, WIS, CHA */
 unsigned char race,klass,sex,alignment,portrait,hp,maxhp,ac;
} Hero;
extern Hero heroes[4];
extern unsigned char party_max;
extern volatile unsigned char front_phase,intro_scene;
extern volatile unsigned char front_ready;
void HeroesDefaults(void);
void HeroesRestore(void);
unsigned char FrontRun(void);
void FrontGameFaces(void);
void FrontInspect(void);
void UiLoad(unsigned int id,unsigned char segment);
void UiText(int x,int y,char *s,char color);
void UiBox(int x,int y,int xx,int yy,char color);
void UiCopy(int sx,int sy,int x,int y,int w,int h);
unsigned char UiRead(void);
unsigned char UiPoll(void);
void UiRelease(void);
#endif

/* Stack ABI arithmetic for Fusion-C. The installed SDCC runtime uses ABI 1. */
int _mulint(int aa,int bb){
 unsigned int a=aa,b=bb;
 unsigned int r=0;
 while(b){if(b&1)r+=a;a<<=1;b>>=1;}
 return r;
}
static unsigned int divide(unsigned int a,unsigned int b,unsigned char remainder){
 unsigned int q=0,r=0;unsigned char i,carry;
 if(!b)return 0;
 for(i=0;i<16;++i){
  carry=(r&0x8000)!=0;r=(r<<1)|(a>>15);a<<=1;q<<=1;
  if(carry || r>=b){r-=b;q|=1;}
 }
 return remainder?r:q;
}
unsigned int _divuint(unsigned int a,unsigned int b){return divide(a,b,0);}
unsigned int _moduint(unsigned int a,unsigned int b){return divide(a,b,1);}
unsigned int _divuchar(unsigned char a,unsigned char b){return divide(a,b,0);}
unsigned int _moduchar(unsigned char a,unsigned char b){return divide(a,b,1);}
int _divsint(int a,int b){
 unsigned int ua=a,ub=b,r;
 if(a<0)ua=0-ua;if(b<0)ub=0-ub;
 r=divide(ua,ub,0);return (a<0)!=(b<0)?0-r:r;
}
int _modsint(int a,int b){
 unsigned int ua=a,ub=b,r;
 if(a<0)ua=0-ua;if(b<0)ub=0-ub;
 r=divide(ua,ub,1);return a<0?0-r:r;
}



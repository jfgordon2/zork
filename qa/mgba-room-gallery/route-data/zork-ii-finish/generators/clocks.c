#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "zmachine.h"
#include "save.h"
static uint8_t story[100000],sram[SRAM_SIZE],visited[VISITED_BYTES];
static void tx(const char*s,size_t n){fwrite(s,1,n,stdout);}
static void die(const char*s,unsigned p,unsigned o){fprintf(stderr,"%s %u %u",s,p,o);exit(2);}
static uint32_t entropy(void){return 4372;}
static ZPlatform io={tx,die,entropy,0,0};
int main(int argc,char**argv){FILE*f=fopen("stories/zork2.z3","rb");assert(f);size_t n=fread(story,1,sizeof(story),f);fclose(f);z_start(story,n,&io);assert(z_run(1000000)==Z_PROMPT);f=fopen(argv[1],"rb");assert(f);assert(fread(sram,1,sizeof(sram),f)==sizeof(sram));fclose(f);assert(save_read(sram,1,visited,0));for(int i=2;i<argc;i++){assert(z_submit(argv[i]));z_run(1000000);}const uint8_t*m=z_dynamic();unsigned len=z_dynamic_size();printf("\nsize=%u globalbase=%u\n",len,m[12]*256+m[13]);for(unsigned a=0;a+2<len;a++)if(m[a]*256+m[a+1]==21855){printf("\nFUSE record around %x:",a);for(unsigned j=a-8;j<a+8;j+=2)printf(" %x:%u",j,m[j]*256+m[j+1]);}for(unsigned a=9315;a<9450;a+=2)printf("\nW %u %u",a,m[a]*256+m[a+1]);puts("");}
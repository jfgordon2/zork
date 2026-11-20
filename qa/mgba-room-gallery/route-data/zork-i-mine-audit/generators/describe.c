#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "interaction.h"
#include "save.h"
static uint8_t story[100000],sram[SRAM_SIZE],seen[VISITED_BYTES];
static void text(const char*s,size_t n){(void)s;(void)n;}
static void fatal(const char*s,unsigned pc,unsigned op){fprintf(stderr,"%s %u %u\n",s,pc,op);exit(2);}
static uint32_t entropy(void){return 7;}
static const ZPlatform io={text,fatal,entropy,0,0};
int main(void){
 FILE*f=fopen("stories/zork1.z3","rb");size_t n=fread(story,1,sizeof(story),f);fclose(f);z_start(story,n,&io);assert(z_run(1000000)==Z_PROMPT);
 f=fopen("build/zork-i-mine-audit/route-46-lit.sav","rb");assert(fread(sram,1,sizeof(sram),f)==sizeof(sram));fclose(f);assert(save_read(sram,0,seen,0));
 assert(z_submit("take bracelet"));assert(z_run(1000000)==Z_PROMPT);
 Action a[MAX_ACTIONS];unsigned count=story_actions(0,214,true,PRIMARY,a);
 for(unsigned i=0;i<count;i++)printf("%u %s\n",i,a[i].name);
}

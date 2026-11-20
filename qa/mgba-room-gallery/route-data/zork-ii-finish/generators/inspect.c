#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "zmachine.h"
#include "save.h"
#include "visual_state.h"
static uint8_t story[100000],sram[SRAM_SIZE],visited[VISITED_BYTES];
static void tx(const char*s,size_t n){fwrite(s,1,n,stdout);}
static void die(const char*s,unsigned p,unsigned o){fprintf(stderr,"%s %u %u",s,p,o);exit(2);}
static uint32_t entropy(void){return 4372;}
static ZPlatform io={tx,die,entropy,0,0};
int main(int argc,char**argv){FILE*f=fopen("stories/zork2.z3","rb");assert(f);size_t n=fread(story,1,sizeof(story),f);fclose(f);z_start(story,n,&io);assert(z_run(1000000)==Z_PROMPT);f=fopen(argv[1],"rb");assert(f);assert(fread(sram,1,sizeof(sram),f)==sizeof(sram));fclose(f);assert(save_read(sram,1,visited,0));for(int i=2;i<argc;i++){assert(z_submit(argv[i]));z_run(1000000);}for(unsigned id=1;id<252;id++){char name[Z_NAME_MAX];z_name(id,name,sizeof(name));printf("\nOBJ %u parent=%u attrs=",id,z_parent(id));for(unsigned a=0;a<32;a++)if(z_attr(id,a))printf("%u,",a);printf(" %s",name);}for(unsigned room=1;room<252;room++)for(unsigned prop=20;prop<32;prop++){const uint8_t*v;size_t len=z_property(room,prop,&v);if(len){printf("\nPROP %u %u=",room,prop);for(size_t i=0;i<len;i++)printf("%02x",v[i]);}}for(unsigned g=0;g<240;g++)printf("\nGLOBAL %u=%u",g,z_global(g));puts("");}

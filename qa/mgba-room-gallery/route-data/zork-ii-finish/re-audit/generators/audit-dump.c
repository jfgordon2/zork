#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define main gba_main
#include "trilogy-host.inc"
#undef main
#include "video-host.inc"
#include "art-dictionary.inc"


static uint8_t story[96 * 1024], sram[SRAM_SIZE];
static bool seen[256][2];
static unsigned current_command;
static unsigned seed;
static void report_fatal(const char *msg, unsigned pc, unsigned op) {
    fprintf(stderr,"Fatal: %s (pc %u op %u)\n",msg,pc,op);
    exit(3);
}
static uint32_t host_entropy(void) { return seed; }
static const ZPlatform host = {story_text,report_fatal,host_entropy,NULL,NULL};

static void pnm(const char *name) {
    char path[192];
    snprintf(path,sizeof(path),"build/zork-ii-finish/re-audit/%s.ppm",name);
    FILE *fp=fopen(path,"wb");
    assert(fp);
    fprintf(fp,"P6\n240 160\n255\n");
    for (unsigned p=0;p<SCREEN_W*SCREEN_H;p++) {
        Color c=framebuffer[p];
        fputc((c&31)*255/31,fp);
        fputc(((c>>5)&31)*255/31,fp);
        fputc(((c>>10)&31)*255/31,fp);
    }
    fclose(fp);
}

static void snapshot(const char *name, unsigned focused) {
    if (focused) {
        for (focus=0;focus<nearby.count;focus++) if (nearby.id[focus]==focused) break;
    }
    const Color *background=story_scene(chapter,room_name);
    const char *asset="unidentified";
    for (unsigned i=0;i<sizeof(palette)/sizeof(*palette);i++)
        if (palette[i].art==background) asset=palette[i].name;
    printf("\nSHOT %s room=%u scene=%s title=%s dark=%d pages=%u room targets=",
           name,room_object,asset,room_name,dark,page_count);
    for (unsigned i=0;i<nearby.count;i++) {
        char label[Z_NAME_MAX];
        story_name(chapter,nearby.id[i],label);
        printf(" %u:%s;",nearby.id[i],label);
    }
    puts("");
    render();
    pnm(name);
    char plain[160];
    snprintf(plain,sizeof(plain),"%s-clear",name);
    draw_explore(true);
    pnm(plain);
    if (!dark) {
        scene_rows(ART_Y,background+10*SCREEN_W,ART_HEIGHT);
        draw_room_details(chapter,room_object,background,&nearby,0);
        snprintf(plain,sizeof(plain),"%s-uncovered",name);
        pnm(plain);
    }
}

static void restore_state(const char *path) {
    FILE *file=fopen(path,"rb");
    assert(file && fread(sram,1,sizeof(sram),file)==sizeof(sram));
    fclose(file);
    assert(save_read(sram,chapter,visited,&trail,&puzzle_map));
    text_reset();
    set_response("Saved game restored.");
    text_pages();
    refresh_scope();
    screen=EXPLORE;
    busy=false;
}

static void save_state(const char *path) {
    assert(save_write(sram,chapter,visited,&trail,&puzzle_map));
    FILE *file=fopen(path,"wb");
    assert(file && fwrite(sram,1,sizeof(sram),file)==sizeof(sram));
    fclose(file);
}

#include "access.inc"
#include "persistence.inc"
#include "../finish.inc"

static void room_snapshot(void) {
    if (seen[room_object][dark]) return;
    seen[room_object][dark]=true;
    char name[40];
    snprintf(name,sizeof(name),"route-%u-%s",room_object,dark?"dark":"lit");
    snapshot(name,0);
    if (!dark && screen == EXPLORE) audit_access();
    char path[128];
    snprintf(path,sizeof(path),"build/zork-ii-finish/re-audit/%s.sav",name);
    save_state(path);
}

int main(int argc, char **argv) {
    assert(argc==3); setvbuf(stdout,NULL,_IONBF,0);
    seed=atoi(argv[1]);
    chapter=1;
    FILE *file=fopen("stories/zork2.z3","rb");
    assert(file);
    size_t length=fread(story,1,sizeof(story),file);
    fclose(file);
    z_start(story,length,&host);
    assert(z_run(1000000)==Z_PROMPT);
    text_pages();
    refresh_scope();
    screen=EXPLORE;
    FILE *script=fopen(argv[2],"r");
    assert(script);
    char call[180];
    while(fgets(call,sizeof(call),script)) {
        call[strcspn(call,"\r\n")]=0;
        if (!*call || *call=='#') continue;
        if (*call=='@') {restore_state(call+1); printf("ROOM=%u GLOBALS",z_global(0)); for(unsigned gi=0;gi<240;gi++){unsigned gv=z_global(gi);if(gv)printf(" %u=%u",gi,gv);} puts(""); continue;}
        if (*call=='!') {save_state(call+1);continue;}
        if (*call=='|') {
            unsigned want=0;
            char name[96];
            if (sscanf(call+1,"%95s %u",name,&want)>=1) snapshot(name,want);
            continue;
        }
        if (*call=='^') {finish_case(call[1]);continue;}
        if (*call=='&') {special_audit(call[1]);continue;}
        if (*call=='%') {check_target_picker();continue;}
        if (*call=='$') { room_snapshot();continue; }
        if (!strncmp(call,">>>> ",5)) {
            char *end=strstr(call+5," [room ");
            assert(end);
            *end=0;
            memmove(call,call+5,strlen(call+5)+1);
        }
        current_command++;
        printf("\n>>>> %s [room #%u score %u turn %u]\n",call,room_object,z_global(1),z_global(2));
        if (screen==FINISHED) break;
        send_command(call);
        while (busy) complete_turn();
        puts(response);
    }
    fclose(script);
    printf("PICKERS %u reachable argument pickers\n", checked_pickers);
    printf("ACCESS %u room states, %u focus targets, %u bag entries, %u traversed menu rows, %u matching room crops\n", audited_views, access_targets, carried_targets, menu_rows, art_checks);
    printf("AUDIT RESULT screen=%u room=%u score=%u moves=%u\n",screen,room_object,z_global(1),z_global(2));
}

void _start(void) { exit(3); }

void visual_qa_ui_focus(void) {}

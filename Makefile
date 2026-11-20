CROSS ?= arm-none-eabi-
PYTHON ?= python3
HOST_CC ?= cc
CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy

SCENES = west north south rear kitchen living living_moved cellar
EXTRA_SCENES = barrow stairway cavern water hall forest troll_room riddle_room riddle_room_open mirror_room_v2 \
               canyon dam egyptian carousel tea lake_shore jewel_room \
               treasure_room atlantis great_cavern workroom cliff_ledge_upper_shelf flathead_ocean \
               cyclops_room aragain_falls end_of_rainbow pentagram_room menhir_room flathead_ship \
               dragon_room robot_machine land_of_shadow dungeon_entrance aquarium aquarium_broken \
               bank_vault bank_small_room bank_depository cerberus_room crypt_anteroom crypt_anteroom_open \
               hades hades_inside engravings cliff_top cliff_base junction technology_museum \
               museum_entrance museum_entrance_open
ART = $(SCENES:%=build/%.rgb555)
EXTRA_SCENES += $(patsubst assets/%.png,%,$(wildcard assets/z1_*.png assets/z2_*.png))
Z3_SCENES = z3_aqueduct z3_aqueduct_view z3_barren_area z3_beam_room z3_button_room \
            z3_creepy_crawl z3_crystal_grotto z3_damp_passage z3_dead_end z3_east_corridor \
            z3_endless_stair z3_foggy_room z3_great_door z3_guardian_hallway_v3 \
            z3_hairpin_loop z3_high_arch z3_jewel_unfinished z3_key_room z3_ladder_bottom \
            z3_ladder_top z3_lake_surface z3_marble_hall z3_marble_hall_prison_open \
            z3_mine_dead_end z3_mirror_hall_closed z3_mirror_hall_open z3_narrow_corridor z3_narrow_mirror \
            z3_narrow_mirror_guardians z3_narrow_mirror_guardians_north z3_parapet \
            z3_prison_cell_bronze z3_prison_cell_closed z3_prison_cell_open z3_puzzle_room \
            z3_royal_hall z3_royal_puzzle_entrance z3_scenic_vista z3_shadow_cliff z3_land_of_shadow_69 \
            z3_side_room z3_south_corridor z3_south_shore z3_technology_unfinished \
            z3_tight_squeeze z3_timber_room z3_treasury z3_underwater z3_water_slide z3_west_shore
EXTRA_SCENES += $(Z3_SCENES)
MIRROR_SCENES = z3_narrow_mirror_west z3_narrow_mirror_guardians_west z3_narrow_mirror_guardians_north_west z3_west_corridor
EXTRA_SCENES += $(MIRROR_SCENES)
EXTRA_ART = $(EXTRA_SCENES:%=build/%.rgb555)
ENCOUNTER_ART = assets/encounter-options/imagegen-sprites
ITEM_ART = assets/items
DEMO = build/start.o build/adventure.o build/main.o build/video.o build/scenes.o
TRILOGY = build/start.o build/trilogy.o build/chapter.o build/interaction.o build/save.o build/zmachine.o \
          build/video.o build/room_visuals.o build/puzzle_map.o build/room_map.o build/trail.o \
          build/trilogy-scenes.o build/stories.o build/visual_state.o build/transcript_codec.o
CFLAGS = -mcpu=arm7tdmi -marm -O2 -std=c11 -Wall -Wextra -Werror \
         -ffreestanding -fno-builtin -ffunction-sections -fdata-sections -MMD -MP

.DELETE_ON_ERROR:
.PHONY: all test test-transcripts
all: zork-trilogy.gba zork-white-house.gba

test: build/story-test build/transcript-replay-test
	$<
	build/transcript-replay-test

test-transcripts: build/transcript-replay-test
	$<

TEST_SOURCES = tests/story_test.c src/zmachine.c src/save.c src/chapter.c src/interaction.c src/puzzle_map.c src/trail.c src/room_map.c src/video.c src/visual_state.c src/transcript_codec.c
TEST_SOURCES += tests/scene_stubs.c tests/z3_scene_stubs.c
TEST_HEADERS = src/zmachine.h src/save.h src/chapter.h src/interaction.h src/puzzle_map.h src/video.h src/trail.h src/room_map.h src/visual_state.h src/transcript_codec.h
TEST_HEADERS += src/zork1_scenes.h src/zork1_scenes.def src/zork2_scenes.h src/zork2_scenes.def

build/story-test: $(TEST_SOURCES) $(TEST_HEADERS) vendor/mojozork/mojozork.c | build
	$(HOST_CC) -O2 -std=c11 -Wall -Wextra -Werror -fno-builtin -Isrc $(TEST_SOURCES) -o $@

build/transcript-replay-test: tests/transcript_replay_test.c tests/fixtures/zork1-winning-route.tsv \
	                             tests/fixtures/zork2-winning-route.tsv tests/fixtures/zork3-winning-route.tsv \
	                             src/zmachine.c src/zmachine.h src/chapter.c src/chapter.h src/interaction.c src/interaction.h \
	                             src/room_visuals.c src/room_visuals.h src/visual_state.c src/visual_state.h \
	                             src/video.c src/video.h src/transcript_codec.c src/transcript_codec.h \
	                             tests/scene_stubs.c tests/z3_scene_stubs.c build/encounter_art.h build/item_art.h \
	                             stories/zork1.z3 stories/zork2.z3 stories/zork3.z3 | build
	$(HOST_CC) -O2 -std=c11 -Wall -Wextra -Werror -fno-builtin -DVISUAL_QA -Isrc -Ibuild \
		tests/transcript_replay_test.c src/zmachine.c src/chapter.c src/interaction.c src/room_visuals.c src/visual_state.c \
		src/video.c src/transcript_codec.c tests/scene_stubs.c tests/z3_scene_stubs.c -o $@

zork-trilogy.gba: build/trilogy.elf tools/rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/rom.py $@ trilogy

build/trilogy.elf: $(TRILOGY) src/gba.ld
	$(CC) -mcpu=arm7tdmi -marm -nostdlib -T src/gba.ld -Wl,--gc-sections $(TRILOGY) -o $@

zork-white-house.gba: build/zork.elf tools/rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/rom.py $@

build/zork.elf: $(DEMO) src/gba.ld
	$(CC) -mcpu=arm7tdmi -marm -nostdlib -T src/gba.ld -Wl,--gc-sections $(DEMO) -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: src/%.S | build
	$(CC) -mcpu=arm7tdmi -marm -c $< -o $@

build/room_visuals.o: CFLAGS += -Ibuild
build/room_visuals.o: build/encounter_art.h build/item_art.h

build/encounter_art.h: $(ENCOUNTER_ART)/frames.json $(wildcard $(ENCOUNTER_ART)/*.png) tools/encounters.py | build
	$(PYTHON) tools/encounters.py $< $@

build/item_art.h: $(ITEM_ART)/frames.json $(wildcard $(ITEM_ART)/*.png) tools/encounters.py | build
	$(PYTHON) tools/encounters.py $< $@

build/scenes.o: $(ART)
build/trilogy-scenes.o: src/scenes.S Makefile src/zork1_scenes.def src/zork2_scenes.def $(ART) $(EXTRA_ART) | build
	$(CC) -mcpu=arm7tdmi -marm -DTRILOGY -c $< -o $@

build/stories.o: stories/zork1.z3 stories/zork2.z3 stories/zork3.z3

build/%.rgb555: assets/%.png tools/scene.py | build
	$(PYTHON) tools/scene.py $< $@

build/z3_narrow_mirror_west.rgb555: assets/z3_narrow_mirror.png tools/scene.py | build
	$(PYTHON) tools/scene.py --flip-horizontal $< $@

build/z3_narrow_mirror_guardians_west.rgb555: assets/z3_narrow_mirror_guardians.png tools/scene.py | build
	$(PYTHON) tools/scene.py --flip-horizontal $< $@

build/z3_narrow_mirror_guardians_north_west.rgb555: assets/z3_narrow_mirror_guardians_north.png tools/scene.py | build
	$(PYTHON) tools/scene.py --flip-horizontal $< $@

build/z3_west_corridor.rgb555: assets/z3_east_corridor.png tools/scene.py | build
	$(PYTHON) tools/scene.py --flip-horizontal $< $@

build:
	mkdir -p $@

-include $(DEMO:.o=.d) $(TRILOGY:.o=.d)

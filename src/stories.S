.section .rodata.stories, "a", %progbits
/* Standard GBA backup identifier used by emulator autodetection. */
.asciz "SRAM_V113"
.macro STORY name
    .balign 4
    .global story_\name, story_\name\()_end
story_\name:
    .incbin "stories/\name\().z3"
story_\name\()_end:
.endm

STORY zork1
STORY zork2
STORY zork3

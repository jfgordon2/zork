OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY {
    ROM (rx) : ORIGIN = 0x08000000, LENGTH = 32M
    EWRAM (rw) : ORIGIN = 0x02000000, LENGTH = 256K
}

SECTIONS {
    .text : {
        KEEP(*(.header))
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > ROM

    .data : {
        *(.data .data.*)
    } > EWRAM AT > ROM
    ASSERT(SIZEOF(.data) == 0, "startup does not copy initialized RAM")

    .bss (NOLOAD) : {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > EWRAM

    /DISCARD/ : { *(.ARM.exidx* .ARM.extab*) }
}

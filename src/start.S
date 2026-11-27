.syntax unified
.arm
.section .header, "ax", %progbits
.global _start
_start:
    b init
    .space 188

.section .text.init, "ax", %progbits
init:
    ldr sp, =0x03007F00
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
1:
    cmp r0, r1
    strlo r2, [r0], #4
    blo 1b
    ldr r0, =__iwram_start
    ldr r1, =__iwram_end
    mov r2, #0
2:
    cmp r0, r1
    strlo r2, [r0], #4
    blo 2b
    bl main
3:
    b 3b

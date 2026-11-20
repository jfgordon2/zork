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
    bl main
2:
    b 2b

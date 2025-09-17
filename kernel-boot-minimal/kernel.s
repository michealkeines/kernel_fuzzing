    .section .text,"ax"
    .globl _start

    .equ UART0, 0x09000000           // PL011 on QEMU virt

_start:
    // x0 = UART base = 0x09000000
    movz    x0, #(UART0 & 0xFFFF)
    movk    x0, #((UART0 >> 16) & 0xFFFF), lsl #16

    adr     x1, msg                  // address of string (nearby)
1:  ldrb    w3, [x1], #1
    cbz     w3, 2f

// wait while TX FIFO full (FR bit5 == 1)
3:  ldr     w2, [x0, #0x18]          // FR
    tbz     w2, #5, 4f               // if bit5==0 -> ready
    b       3b
4:  str     w3, [x0, #0x00]          // DR
    b       1b

2:  b       2b

    .section .rodata,"a"
msg: .asciz "KERNEL OK\r\n"

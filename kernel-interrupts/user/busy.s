.section .text, "ax"
.globl user_loop

user_loop:
// we excepting the kernel to interrupt and switch back here after the interrupt handling
1:
    mov x0, #1 // just move something in a register
    mov x0, #2 // just move something in a register
    b 1b // keep repeating this move


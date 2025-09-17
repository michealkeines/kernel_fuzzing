.section .text, "ax"
.globl _start
.extern kmain
.extern __bss_start
.extern __bss_end
.extern vectors

_start:
    // Mask all D, A, I, F, so we dont have any interrupts at this stage
    msr DAIFSet, #0xf

    // init SP for C runtime
    ldr x0, =_stack_top
    mov sp, x0

    // Zero .bss
    ldr  x1, =__bss_start
    ldr  x2, =__bss_end
1:  cmp  x1, x2
    b.eq 2f
    str  xzr, [x1], #8
    b    1b

    // set vector base address for EL1
2:  ldr     x0, =vectors
    msr     VBAR_EL1, x0
    isb

    bl      kmain
    b       .
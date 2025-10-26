.section .text, "ax"
.global _start
.extern kmain
.extern __bss_start
.extern __bss_end
.extern vectors
.extern _stack_top
.extern mmu_init

_start:
    // disable interrupts
    msr daifset, #0xf

    // select proper EL1 stack pointer
    msr spsel, #1

    // update the stack to proper address
    ldr x0, =_stack_top
    mov sp, x0

    // Zero .bss
    ldr x1, =__bss_start
    ldr x2, =__bss_end
1:  cmp x1, x2
    b.eq 2f
    str xzr, [x1], #8
    b    1b

    // Vector Base Address, EL1
2:  ldr x0, =vectors
    msr VBAR_EL1, x0

    bl mmu_init

    bl  kmain
    b   .



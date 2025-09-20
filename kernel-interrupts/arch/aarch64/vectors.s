.macro VEC handler
    b \handler
    .space 128-4
.endm

.section .text, "ax"
.align   11
.globl   vectors

.extern  el1_sync
.extern  el1_irq
.extern  el0_sync

// we need to have all the 16 exceptions handled
// Exceptions are handler using the Base vector + vector index
// 4 slots defines, where the interrupt is comming from

vectors:
    // EL1t
    VEC el1_sync
    VEC el1_irq
    VEC .
    VEC .

    // EL1h
    VEC el1_sync
    VEC el1_irq
    VEC .
    VEC .

    // EL0 32-bit
    VEC el0_sync
    VEC el1_irq
    VEC .
    VEC .

    // EL0 64-bit
    VEC el0_sync
    VEC el1_irq
    VEC .
    VEC .


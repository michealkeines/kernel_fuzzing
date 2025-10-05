.macro VEC handler
    b \handler
    .space 128-4
.endm

.macro SAVE_CONTEXT
    stp x0,  x1,  [sp, #-16]!
    stp x2,  x3,  [sp, #-16]!
    stp x4,  x5,  [sp, #-16]!
    stp x6,  x7,  [sp, #-16]!
    stp x8,  x9,  [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x18, x19, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    stp x28, x29, [sp, #-16]!
    stp x30, xzr, [sp, #-16]!
    mrs x0, ELR_EL1
    mrs x1, SPSR_EL1
    stp x0, x1,   [sp, #-16]!
    mrs x2, SP_EL0
    stp x2, xzr, [sp, #-16]!
.endm
     
.macro RESTORE_CONTEXT
    ldp x2, xzr, [sp], #16
    msr SP_EL0, x2
    ldp x0, x1, [sp], #16
    msr ELR_EL1, x0
    msr SPSR_EL1, x1
    ldp x30, xzr, [sp], #16
    ldp x28, x29, [sp], #16
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x18, x19, [sp], #16
    ldp x16, x17, [sp], #16
    ldp x14, x15, [sp], #16
    ldp x12, x13, [sp], #16
    ldp x10, x11, [sp], #16
    ldp x8,  x9,  [sp], #16
    ldp x6,  x7,  [sp], #16
    ldp x4,  x5,  [sp], #16
    ldp x2,  x3,  [sp], #16
    ldp x0,  x1,  [sp], #16 
.endm

.section .text, "ax"
.align 11
.globl vectors

.extern irq_dispatcher


vectors:
    // EL1t — took exception from EL1 using SP_EL0
    VEC el1_sync_entry   // synchronous exception
    VEC el1_irq_entry    // IRQ interrupt
    VEC default_entry    // FIQ (unused)
    VEC default_entry    // SError (unused)

    // EL1h — took exception from EL1 using SP_EL1
    VEC el1_sync_entry
    VEC el1_irq_entry
    VEC default_entry
    VEC default_entry

    // EL0 32-bit
    VEC el0_sync_entry
    VEC el1_irq_entry
    VEC default_entry
    VEC default_entry

    // EL0 64-bit
    VEC el0_sync_entry
    VEC el1_irq_entry
    VEC default_entry
    VEC default_entry


el1_sync_entry: 
    SAVE_CONTEXT
    bl irq_dispatcher
    RESTORE_CONTEXT
    eret

el1_irq_entry:
    SAVE_CONTEXT
    bl irq_dispatcher
    RESTORE_CONTEXT
    eret


el0_sync_entry:
    SAVE_CONTEXT
    bl irq_dispatcher
    RESTORE_CONTEXT
    eret

default_entry: b .
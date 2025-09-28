    .set CTX_X,        0
    .set CTX_SP_EL0,   (31*8)
    .set CTX_ELR,      (32*8)
    .set CTX_SPSR,     (33*8)
    .set CTX_SIZE,     (34*8)

    .macro SAVE_CONTEXT
        sub     sp, sp, #CTX_SIZE

        // save x0..x29 as pairs, then x30
        stp     x0,  x1,  [sp, #(0*16)]
        stp     x2,  x3,  [sp, #(1*16)]
        stp     x4,  x5,  [sp, #(2*16)]
        stp     x6,  x7,  [sp, #(3*16)]
        stp     x8,  x9,  [sp, #(4*16)]
        stp     x10, x11, [sp, #(5*16)]
        stp     x12, x13, [sp, #(6*16)]
        stp     x14, x15, [sp, #(7*16)]
        stp     x16, x17, [sp, #(8*16)]
        stp     x18, x19, [sp, #(9*16)]
        stp     x20, x21, [sp, #(10*16)]
        stp     x22, x23, [sp, #(11*16)]
        stp     x24, x25, [sp, #(12*16)]
        stp     x26, x27, [sp, #(13*16)]
        stp     x28, x29, [sp, #(14*16)]
        str     x30,       [sp, #(15*16)]

        // we are writing the values based on offsets from the Context struct
        mrs     x0, SP_EL0
        str     x0, [sp, #CTX_SP_EL0]
        mrs     x0, ELR_EL1
        str     x0, [sp, #CTX_ELR]
        mrs     x0, SPSR_EL1
        str     x0, [sp, #CTX_SPSR]
    .endm

    .macro RESTORE_AND_ERET
        // we are reading the values based on offsets from the Context struct
        ldr     x1, [sp, #CTX_SP_EL0]
        msr     SP_EL0, x1
        ldr     x1, [sp, #CTX_ELR]
        msr     ELR_EL1, x1
        ldr     x1, [sp, #CTX_SPSR]
        msr     SPSR_EL1, x1

        ldr     x30,       [sp, #(15*16)]
        ldp     x28, x29,  [sp, #(14*16)]
        ldp     x26, x27,  [sp, #(13*16)]
        ldp     x24, x25,  [sp, #(12*16)]
        ldp     x22, x23,  [sp, #(11*16)]
        ldp     x20, x21,  [sp, #(10*16)]
        ldp     x18, x19,  [sp, #(9*16)]
        ldp     x16, x17,  [sp, #(8*16)]
        ldp     x14, x15,  [sp, #(7*16)]
        ldp     x12, x13,  [sp, #(6*16)]
        ldp     x10, x11,  [sp, #(5*16)]
        ldp     x8,  x9,   [sp, #(4*16)]
        ldp     x6,  x7,   [sp, #(3*16)]
        ldp     x4,  x5,   [sp, #(2*16)]
        ldp     x2,  x3,   [sp, #(1*16)]
        ldp     x0,  x1,   [sp, #(0*16)]

        add     sp, sp, #CTX_SIZE
        eret
    .endm

    .macro VEC handler
        b \handler
        .space 128-4
    .endm

    .section .text, "ax"
    .align  11
    .globl  vectors

vectors:
    // EL1t
    VEC el1_sync_entry
    VEC el1_irq_entry
    VEC default_entry
    VEC default_entry

    // EL1h
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

    .globl el1_irq_entry
el1_irq_entry:
    SAVE_CONTEXT
    mov     x0, sp
    bl      schedule_on_tick
    mov     sp, x0
    RESTORE_AND_ERET

    .globl el0_sync_entry
el0_sync_entry:
    SAVE_CONTEXT
    mov     x0, sp
    bl      el0_sync_handler
    mov     sp, x0
    RESTORE_AND_ERET

    .globl el1_sync_entry
el1_sync_entry:
    SAVE_CONTEXT
    mov     x0, sp
    bl      el1_sync_handler
    mov     sp, x0
    RESTORE_AND_ERET

default_entry:
    b default_entry

    .globl first_user_enter
first_user_enter:
    // push the first task into execution
    mov     sp, x0
    RESTORE_AND_ERET

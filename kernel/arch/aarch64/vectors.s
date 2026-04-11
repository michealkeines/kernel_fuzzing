.macro VEC handler
    b \handler
    .space 128-4
.endm
    .set CTX_X,        0
    .set CTX_TTBR0_EL1, (34*8)
    .set CTX_SPSR,     (33*8)
    .set CTX_ELR,      (32*8)
    .set CTX_SP_EL0,   (31*8)
    .set CTX_SIZE,     (34*8) // this is the total number of 8 bytes values store in stack

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
        mrs     x0, TTBR0_EL1
        str     x0, [sp, #CTX_TTBR0_EL1]
    .endm

    .macro RESTORE_AND_ERET
        // we are reading the values based on offsets from the Context struct
        ldr     x1, [sp, #CTX_TTBR0_EL1]
        msr     TTBR0_EL1, x1
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

.section .text, "ax"
.align 11
.globl vectors
.globl first_user_enter

.extern irq_dispatcher
.extern schedule_next
.extern syscall_dispatcher


vectors:
    // EL1t — took exception from EL1 using SP_EL0
    VEC el1_sync_entry   // synchronous exception
    VEC el1_irq_entry    // IRQ interrupt, we need this to be done
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
    VEC el0_sync_entry // we need this to be done
    VEC el1_irq_entry
    VEC default_entry
    VEC default_entry


el1_sync_entry:
    // https://developer.arm.com/documentation/ddi0601/2025-12/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-
    // [31:26] bit is the one that hold the exception information
    // so we shift 26 times, to move the 26th bit to the 0th bit
    // 0b010101 or 0x15 is the SVC exception, that is the syscall
    mrs x10, ESR_EL1
    lsr x11, x10, #26
    cmp x11, #0x15
    b.ne not_syscall

    bl syscall_dispatcher
    eret

el1_irq_entry:
    SAVE_CONTEXT
    mov x0, sp
    bl irq_dispatcher
    mov sp, x0
    RESTORE_AND_ERET



// syscall number in w8
// retval1 x0
// retval2 x1
el0_sync_entry:
    mrs x10, ESR_EL1
    lsr x11, x10, #26
    cmp x11, #0x15
    b.ne not_syscall
    bic x8, x8, #0x000000000000ffff // take the lower 15:0 bit into x0
    mov x7, x8 // syscall number
    bl syscall_dispatcher
    eret

not_syscall:
    eret

default_entry: eret
// irq_dispatcher: eret
syscall_dispatcher: eret 
first_user_enter:
    mov sp, x0;
    RESTORE_AND_ERET
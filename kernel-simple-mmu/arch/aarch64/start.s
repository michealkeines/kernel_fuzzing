.section .text, "ax"
.global _start
.extern kmain
.extern __bss_start
.extern __bss_end
.extern vectors
.extern _stack_top
.extern mmu_init
.extern get_total_ram

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
    bl get_total_ram

    mov x13, x0 // we are writing to x13, so make sure, in future if we chanages, x13 is not written

    bl mmu_init

    ldr x0, =0xFFFF000080000000
    ldr x1, =0x0000000080000000
    sub x2, x0, x1

    adrp x3, virt_entry
    add  x3, x3, :lo12:virt_entry

    add x3, x3, x2

    br x3

virt_entry:
    ldp x0, x1, [sp], #16
    bl  kmain
    b   .




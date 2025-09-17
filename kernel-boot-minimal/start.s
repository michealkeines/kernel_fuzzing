    .section    .text.boot, "ax"
    .globl  _start

    .equ    KERNEL_LOAD_ADDR, 0x40080000

    .extern _binary_kernel_bin_start
    .extern _binary_kernel_bin_end

    .extern __bss_start
    .extern __bss_end
    .extern _stack_top 

_start:
    /* Enable Interrupts */
    msr     daifset, #0xf
    
    /* Set Stack Pointer */
    ldr     x0, =_stack_top
    mov     sp, x0

    /* Zero bss */
    ldr     x1, =__bss_start
    ldr     x2, =__bss_end
    sub     x2, x2, x1
    cbz     x2, 2f
1:    
    str     xzr, [x1], #8
    subs    x2, x2, #8
    b.gt    1b

2:
    /* Copy Kernel and Jump */

    ldr     x3, =_binary_kernel_bin_start
    ldr     x4, =_binary_kernel_bin_end
    sub     x5, x4, x3
    ldr     x6, =KERNEL_LOAD_ADDR

    cbz     x5, 3f

mem_copy:
    ldr     x7, [x3], #8
    str     x7, [x6], #8
    subs    x5, x5, #8
    b.gt    mem_copy

3:
    mov     x0, xzr
    mov     x1, xzr
    mov     x2, xzr
    ldr     x3, =KERNEL_LOAD_ADDR
    br      x3

loop:
    wfe
    b       loop
    

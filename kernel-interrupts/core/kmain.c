
#include <stdint.h>

extern void uart_puts(const char *);
extern void gic_init(void);
extern void timer_init_1khz(void);
extern void user_loop(void);
extern void uart_init(void);

/*
    ELR_EL1 = location of the function to run after jumping into EL0

    SPSR_EL1 = what are masks and privilages

    SP_EL0 = stack location

    ERET = jump into a ELR_EL1 location and execute

    PC ← ELR_EL1
    PSTATE ← SPSR_EL1
    Switches to the new exception level
    Switches the stack pointer to SP_EL0
*/
static void enter_el0(void (*fn)(void))
{
    // pick a location for EL0 stack starting, and store it ot sp of EL0
    register uint64_t sp0 asm("x0") = (uint64_t) 0x40020000;
    asm volatile("msr SP_EL0, %0"::"r"(sp0));

    // SPSR_EL1, set it with all zeroed, set the function and finally call eret
    uint64_t spsr = 0;
    asm volatile("msr SPSR_EL1, %0"::"r"(spsr));
    asm volatile("msr ELR_EL1, %0"::"r"(fn));
    uart_puts("EL1: all set now -> (EL0)\n");

    asm volatile("eret");
}

void kmain(void)
{
    uart_init(); // init uart
    uart_puts("\r\nEL1: Kernel Starting\n");

    // start gic
    gic_init();

    // start timer
    timer_init_1khz();

    uart_puts("EL1: enable irq\n");

    asm volatile("msr DAIFClr, #2");

    uart_puts("EL1: jump into user function (EL0)\n");

    enter_el0(user_loop);

    while (1) { asm volatile("wfi"); }
}
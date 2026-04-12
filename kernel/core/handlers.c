#include "scheduler/context.h"
#include "scheduler/sched.h"
#include "stdarg.h"
#include "mmu.h"

extern void uart_printf(const char* fmt, ...);
extern void set_cntv(void); 
extern unsigned int gic_ack(void);
extern void gic_eoi(unsigned int irq);
/*

    we need function that can check the current irq and call its respective handler
*/

void timer_irq_handler()
{
    set_cntv();
}

uint64_t syscall_dispatcher(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7, uint64_t syscall_number)
{
    uart_printf("current syscall num: %l\n", syscall_number);
    uint64_t result = 0;
    switch (syscall_number)
    {
    case 5:
        result = kmalloc(arg1); // TODO: KERNEL to userspace mapping here
        break;
    
    default:
        break;
    }

    return result;
}

uint64_t check_current_el(void)
{
    uint64_t spsr;
    uint64_t current_el;

    __asm__ volatile (
        "mrs %0, SPSR_EL1\n"
        "ubfx %1, %0, #2, #2\n"
        : "=r"(spsr), "=r"(current_el) : : "memory" 
    );

    return current_el == 0x1;
}

void test_test() 
{
    volatile int a = 1;
}

Context *irq_dispatcher(Context *context)
{
            // uart_printf("Timer IRQ handler fired!\n");
    unsigned int iar = gic_ack(); // we get the current interrupt
    unsigned int id = iar & 0x3FF;

    switch (id)
    {
        case 27:
            timer_irq_handler();

            uint64_t el = check_current_el();
            uart_printf("Timer IRQ handler fired for return address %l , EL: %l\n", context->elr_el1, el);
            /*
            currently we are assuming if something comes from EL1, we dont have run schedule on tick

            case:
            - timer iterrupts when we have yet jumped into process
            - incase the userprocess is inside EL1 for a syscall, then we should not interrupt it
            */
            if (!el)
            {
                context = schedule_on_tick(context);
            }
            // enable_cntv();
            break;
        // TODO: Add future IRQ handlers here
        default:
            break;
    }
    // uart_printf(" return address %l\n", context->elr_el1);
    gic_eoi(iar); // we say that we are done now
            // uart_printf("Out of IRQ handler\n");
    return context;
}
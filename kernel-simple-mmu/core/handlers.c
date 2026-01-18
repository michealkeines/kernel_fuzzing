#include "scheduler/context.h"
#include "scheduler/sched.h"
#include "stdarg.h"
#include "mmu.h"

extern void uart_printf(const char* fmt, ...);
extern uint64_t kmalloc(AllocateMem *mem, uint64_t size);
extern AllocateMem GlobalBitMapArray;
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

uint64_t syscall_dispatcher(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    uart_printf("current syscall num: %l\n", syscall_num);
    uint64_t result = 0;
    switch (syscall_num)
    {
    case 5:
        result = kmalloc(&GlobalBitMapArray, arg1);
        break;
    
    default:
        break;
    }

    return result;
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
            uart_printf("Timer IRQ handler fired for return address %l\n", context->elr_el1);
            context = schedule_on_tick(context);
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
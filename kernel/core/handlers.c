#include "scheduler/context.h"
#include "scheduler/sched.h"
#include "stdarg.h"
#include "mmu.h"
#include "../drivers/block/block_driver.h"
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
    case 64:
        uart_printf("Syscall write is caled\n");
        // because the overall size is increaing everytim i add some ccode, i have to maek the vector table address not hard coded anymore this is cause the touble , fix this first
        // here as soon as we call the kmalloc and one the l3 table locaiton is getting page fault
        // we fixed the label of vector table
        // now as sonn as i enter into el1 from el0, the address of tables are not accessible, eg: 0x80070000, which is the L2 table, i breakout at the el0_sync_entry handler, so this is right after we jump into el1 from el0, nothing else has been touched yet,
        // issue is that table is used based on the address starting bit and 0x80000 work in kmain because i have filed tb0 with the entries and when i go into el0 i overwrite them and when i go back to el1, we need to write them back to fix this case fully by mapping all kernel address with upper bits set
        uint64_t read = block_write(1000, (char *)arg2, arg3);
        // uint64_t read = 0;
        uart_printf("Syscall write is done %l\n", read);
        char data[512];
        read = block_read(1000, data, 512);
        for (int i = 0; i < 512; i++) {
    
        uart_printf("%d", data[i]);
        }
        result = read;
        uart_printf("returing from sys_write\n");
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
            // uart_printf("Timer IRQ handler fired for return address %l , EL: %l\n", context->elr_el1, el);
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

#include <stdint.h>

extern uint32_t gic_iar(void);
extern void gic_eoir(uint32_t);
extern void uart_printf(const char *fmt, ...);
#include "context.h"
extern void timer_handler(void);
extern Context *schedule_next(Context *);
extern void write_ticks(void);


Context *irq_dispatcher(Context *ctx)
{
    // uart_printf("Current irq in disptacher");
    uint32_t id = gic_iar();
    // uart_printf("Current irq ID: %d\n", id);
    switch (id)
    {
        case 27:
            ctx = schedule_next(ctx);
            timer_handler();
            write_ticks();
        break;
        default:
        break;
    }

    gic_eoir(id);
    return ctx;
}

long sys_write(int fd, const char *buf, unsigned long count)
{
    if (fd != 1)
        return -1; // only stdout for now

    uart_printf("SYS_CALL_WRITE: %s\n", buf);

    return count;
}


__attribute__((naked, noinline))
long syscall_dispatcher(void)
{
    asm volatile(
        // Save LR (x30) to stack
        "stp x29, x30, [sp, #-16]!\n"

        // Shift args to fit syscall number
        "mov x3, x2\n"
        "mov x2, x1\n"
        "mov x1, x0\n"
        "mov x0, x8\n"

        // Call body
        "bl syscall_dispatch_body\n"

        // Restore LR and FP
        "ldp x29, x30, [sp], #16\n"

        // Return (uses restored x30)
        "ret\n"
    );
}




long syscall_dispatch_body(uint64_t sys_num,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2)
{
    // uart_printf("sys num %d\n", (int)sys_num);

    switch (sys_num)
    {
        case 1:
        // uart_printf("Write dispatch\n");
        return sys_write((int)arg0, (const char *)arg1, arg2);

        default:
        uart_printf("Unknown syscall: %d\n", (int)sys_num);
        return -1;
    }
}




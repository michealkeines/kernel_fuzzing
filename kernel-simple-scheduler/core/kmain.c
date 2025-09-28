#include <stdint.h>
#include <stddef.h>
#include "context.h"
#include "sched.h"

extern void uart_puts(const char *);
extern void uart_init(void);
extern void gic_init(void);
extern void timer_init_1khz(void);
extern void first_user_enter(Context*);

extern void user_loop(void);
extern void user_loop2(void);

#define STK_SZ 4096

static uint8_t kstack0[STK_SZ] __attribute__((aligned(16)));
static uint8_t ustack0[STK_SZ] __attribute__((aligned(16)));
static uint8_t kstack1[STK_SZ] __attribute__((aligned(16)));
static uint8_t ustack1[STK_SZ] __attribute__((aligned(16)));

Task T0, T1;


static void task_init(Task* t, int id, const char* name,
                      void (*entry)(void), void* kstk_base, void* ustk_base, size_t sz)
{

    t->id           = id;
    t->name         = name;
    t->kernel_stack = kstk_base;
    t->user_stack   = ustk_base;

    uint8_t* ktop = (uint8_t*)kstk_base + sz;
    Context* ctx  = (Context*)(ktop - sizeof(Context));
    build_initial_context(ctx, entry, (uint8_t*)ustk_base + sz);
    t->context     = ctx;

    sched_ready_enqueue(t);
}

void kmain(void)
{
    uart_init();
    uart_puts("\r\nEL1: Kernel Starting\n");

    gic_init();
    timer_init_1khz();
    uart_puts("EL1: GIC+Timer ready\n");

    task_init(&T0, 0, "A", user_loop,  kstack0, ustack0, STK_SZ);
    task_init(&T1, 1, "B", user_loop2, kstack1, ustack1, STK_SZ);
    uart_puts("EL1: Tasks created\n");

    // Pick first runnable (T0 enqueued first)
    sched_set_current(&T0);

    uart_puts("EL1: enable IRQs\n");
    __asm__ volatile("msr DAIFClr, #2");

    uart_puts("EL1: enter first user task (EL0)\n");
    first_user_enter(T0.context);

    while (1) { __asm__ volatile("wfi"); }
}

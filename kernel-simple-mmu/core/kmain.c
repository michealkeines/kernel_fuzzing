#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "context.h"

extern void uart_init(void);
extern void uart_printf(const char* fmt, ...);
extern void timer_init(void);
extern void enable_gic_distributor(void);
extern void enable_gic_cpu_interface(void);
extern void sched_set_current(Task *t);
extern void user_loop1(void);
extern void user_loop2(void);
extern void first_user_enter(Context*);

extern void enqueue(Task *);

#define STK_SZ 4096

static uint8_t kstack0[STK_SZ] __attribute__((aligned(16)));
static uint8_t ustack0[STK_SZ] __attribute__((aligned(16)));
static uint8_t kstack1[STK_SZ] __attribute__((aligned(16)));
static uint8_t ustack1[STK_SZ] __attribute__((aligned(16)));

Task T0, T1;

static void task_init(Task *t, uint64_t id, const char *name, void (*entry)(void), void *kstk_base, void *ustk_base, size_t sz)
{
    t->task_id = id;
    t->name = name;
    t->sp_el1 = (uint64_t *)kstk_base;
    t->time_slice = 10000;
    uint8_t *ktop = (uint8_t *)kstk_base + sz;
    Context *ctx = (Context *)(ktop - sizeof(Context));
    ctx->elr_el1 = (uint64_t)entry;
    ctx->sp_el0 = (uint64_t)ustk_base;
    const uint64_t M_EL0t = 0b0000;
    const uint64_t DAIF = 0;
    uint64_t spsr = (M_EL0t | (DAIF << 6));
    ctx->spsr_el1 = spsr;
    t->context = ctx;

    enqueue(t);
}

void kmain(void)
{
    uart_init();

    uart_printf("Kernel Starting\n");
    // uart_printf("Count %d\n", 345);
    // uart_printf("full %s\n", "sdfsd");
    // uart_printf("char %c\n", 'a');
    enable_gic_distributor();
    enable_gic_cpu_interface();
    uart_printf("Enabled GIC Dis/CPU\n");

    timer_init();

    uart_printf("Timmer Inited\n");


    task_init(&T0, 0, "A", user_loop1,  kstack0, ustack0, STK_SZ);
    task_init(&T1, 1, "B", user_loop2, kstack1, ustack1, STK_SZ);
    uart_printf("EL1: Tasks created\n");
    sched_set_current(&T0);
    asm volatile("msr daifclr, #2");
    uart_printf("Enabled Interrupts\n");

    uart_printf("EL1: enter first user task (EL0)\n");
    first_user_enter(T0.context);
    while (1) {int i = 1;}
}
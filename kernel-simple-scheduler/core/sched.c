#include "sched.h"
#include <stdint.h>
#include <stddef.h>

extern void uart_puts(const char*);
extern void timer_program_next_tick(void);
extern void gic_eoi(unsigned int);
extern unsigned int gic_ack(void);
extern void set_cntv(void);

static Task *rq_head = 0;
static Task *rq_tail = 0;
static Task *current = 0;

static int RR_SLICE_TICKS = 10000;

static inline void rq_push(Task *t)
{
    // if (rq_head) {
    //     uart_puts("\nHEAD--");
    //     uart_puts(rq_head->name);
    //     uart_puts("--HEAD\n");
    // }
    // if (rq_tail) {
    //     uart_puts("\nTAIL--");
    //     uart_puts(rq_tail->name);
    //     uart_puts("--TAIL\n");
    // }
    // if (current) {
    //     uart_puts("\nCURRENT--");
    //     uart_puts(current->name);
    //     uart_puts("--CURRENT\n");
    // }
    
    if (!rq_tail) {
        rq_head = rq_tail = t;
        uart_puts("\npush to head/tail--");
        uart_puts(t->name);
        uart_puts("--push to head/tail\n");
    } 
    else {
        uart_puts("\npush to tail--");
        uart_puts(t->name);
        uart_puts("--push to tail\n");
        rq_tail->next = t;
        rq_tail = t;
    }
}

/*
a = 1
b = 2

a.n = b

b.n = a

*/

static inline Task *rq_pop(void)
{
    // uart_puts("new pop started\n");
    // for (Task *a = rq_head;; a= a->next)
    // {
    //     if (a->name) {
    //         uart_puts("\nTraverse--");
    //         uart_puts(a->name);
    //         uart_puts("--Traverse\n");
    //     }
        
    //     if (a->next == 0) break;
    // }
    // uart_puts("traversla over\n");
    // if (rq_head) {
    //     uart_puts("\nPHEAD--");
    //     uart_puts(rq_head->name);
    //     uart_puts("--PHEAD\n");
    // }
    // if (rq_tail) {
    //     uart_puts("\nPTAIL--");
    //     uart_puts(rq_tail->name);
    //     uart_puts("--PTAIL\n");
    // }
    // if (current) {
    //     uart_puts("\nPCURRENT--");
    //     uart_puts(current->name);
    //     uart_puts("--CURRENT\n");
    // }
    Task *t = rq_head;
    if (!t) {return 0;}
    rq_head = t->next;
    if (rq_head) {
        uart_puts("\nPNEXT--");
        uart_puts(rq_head->name);
        uart_puts("--NEXT\n");
    }
    if (!rq_head) rq_tail = 0;
    t->next = 0;
    Task* b = rq_head;
    return b;
}

void build_initial_context(Context *ctx, void (*entry)(void), void * user_stack)
{
    ctx->sp_el0 = (uint64_t) user_stack;
    ctx->elr_el1 = (uint64_t) entry;
    ctx->spsr_el1 = spsr_el0t_default();
}

void sched_ready_enqueue(Task *t)
{
    t->state = READY;
    t->slice_left = RR_SLICE_TICKS;
    uart_puts("\nsched_ready_enqueue--");
        uart_puts(t->name);
        uart_puts("--sched_ready_enqueue\n");
    rq_push(t);
}

void sched_set_current(Task *t)
{
    current = t;
    current->state = RUNNING;
}

Context *schedule_on_tick(Context *tf)
{
    unsigned iar = gic_ack();
    unsigned id  = iar & 0x3ffu;
    Task *next = 0;

    // check current task's time left
    if (current && current->state == RUNNING)
    {
        current->context = tf;
        // uart_puts("Done one Tick, we are in EL1\n");

        if (--current->slice_left > 0)
        {
            set_cntv();
            gic_eoi(iar);
            return current->context;
        }
        // uart_puts("Done one Tick, we are in EL1\n");
        // no time left, we push it to back if the ready queue
        current->slice_left = RR_SLICE_TICKS;
        current->state = READY;
        uart_puts("\ncurrent-tick-done--");
        uart_puts(current->name);
        uart_puts("--current-tick-done\n");
        rq_push(current);

    }

    next = rq_pop();

    if (!next)
    {
        // nothing else is ready
        if (current)
        {
            set_cntv();
            gic_eoi(iar);
            return current->context;
        }

        // if nothing is there
        set_cntv();
        gic_eoi(iar);
        return tf;
    }

    next->state = RUNNING;
    current = next;
    // uart_puts("Done one Tick, we are in EL1\n");
    uart_puts("\nset-running--");
    uart_puts(current->name);
    uart_puts("--set-running\n");
    set_cntv();
    gic_eoi(iar);
    return current->context;
}
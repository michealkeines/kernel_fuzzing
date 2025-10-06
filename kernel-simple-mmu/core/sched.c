#include "context.h"

extern void uart_printf(const char *fmt, ...);

static Task *head = 0;
static Task *tail = 0;
static Task *current = 0;

void enqueue(Task *current_task)
{
    
    if (head == 0)
    {
        // uart_printf("inside equeu %s\n", current_task->name);
        head = tail = current_task;
        current_task->next = 0;
        return;
    }

    tail->next = current_task;
    tail = current_task;
    // current_task->next = 0;
    // uart_printf("inside equeu %s\n", current_task->name);
}

Task *dequeue(void)
{
    // uart_printf("inside deq %s\n", current->name);
    if (head == 0) return 0;

    Task *task = head;
    head = head->next;
    // uart_printf("inside head deq %s\n", head->name);
    if (head == 0) tail = 0;
    current = head;
    task->next = 0;
    return task;
}


Context *schedule_next(struct Context *current_context)
{
    // uart_printf("we are inside next schedule\n");
    // if (current == 0) return current_context;

    // check current ticks
    if (current->time_slice >0)
    {
        // uart_printf("time slice not over for %s\n", current->name);
        current->time_slice--;
        // time slice not over yet
        return current_context;
    }

    current->time_slice = 10000;
    current->status = READY;
    // uart_printf("Enqueue %s\n", current->name);
    enqueue(current);
    
    
    Task *next = dequeue();
    // uart_printf("Dequeue %s\n", current->name);
    current->status = RUNNING;

    return current->context;
}

void sched_set_current(Task *t)
{
    current = t;
    current->status = RUNNING;
}
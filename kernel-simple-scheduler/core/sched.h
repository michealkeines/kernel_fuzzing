#pragma once

#include "context.h"

typedef enum { READY, RUNNING, BLOCKED } TaskState;

typedef struct Task {
    int        id;
    TaskState  state;
    Context    *context;
    void       *user_stack;
    void       *kernel_stack;
    int        slice_left;
    const char       *name;
    struct Task       *next;
} Task;

void build_initial_context(Context *ctx, void (*entry)(void), void *user_stack);

void sched_ready_enqueue(Task *t);
void sched_set_current(Task *t);

Context *schedule_on_click(Context *tf);

void first_user_enter(Context *ctx);
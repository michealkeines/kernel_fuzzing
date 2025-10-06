#include <stdint.h>

typedef __attribute__((aligned(8))) struct Context
{
    uint64_t registers[31];
    uint64_t sp_el0;
    uint64_t elr_el1;
    uint64_t spsr_el1;
} Context;

enum Status {READY, RUNNING, DONE};

typedef struct Task
{
    const char *name;
    uint64_t task_id;
    enum Status status;
    struct Context *context;
    uint64_t time_slice;
    void *sp_el1;
    struct Task *next;
} Task;
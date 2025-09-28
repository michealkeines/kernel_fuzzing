#include "sched.h"

Context* el0_sync_handler(Context* tf)
{
    return tf;
}

Context* el1_sync_handler(Context* tf)
{
    return tf;
}

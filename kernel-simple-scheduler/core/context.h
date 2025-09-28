#pragma once

#include <stdint.h>

typedef struct __attribute__((aligned(16))) Context {
    uint64_t x[31];
    uint64_t sp_el0;
    uint64_t elr_el1;
    uint64_t spsr_el1;
} Context;

static inline uint64_t spsr_el0t_default(void)
{
    const uint64_t M_EL0t = 0b0000;
    const uint64_t DAIF = 0;
    return (M_EL0t | (DAIF << 6));
}
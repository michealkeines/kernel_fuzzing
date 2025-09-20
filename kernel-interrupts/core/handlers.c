#include <stdint.h>

extern void uart_puts(const char*);

void el1_sync(void)
{
    uart_puts("EL1 sync\n");
}

void el0_sync(void)
{
    uart_puts("EL0 sync\n");
}
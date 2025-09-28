#include <stdint.h>

extern void uart_puts(const char*);

void user_loop(void)
{
    uint64_t counter = 1;

    while (1)
    {
        if (counter == 10000000000) {
            uart_puts("User Loop A\n");
            counter = 0;
        }
        counter++;
    }
}
void user_loop2(void)
{
    uint64_t counter = 1;

    while (1)
    {
        if (counter == 10000000000) {
            uart_puts("User Loop B\n");
            counter = 0;
        }
        counter++;
    }
}
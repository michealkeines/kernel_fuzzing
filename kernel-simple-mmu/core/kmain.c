#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

extern void uart_init(void);
extern void uart_printf(const char* fmt, ...);

void kmain(void)
{
    uart_init();

    uart_printf("Kernel Starting\n");
    while (1) {int i = 1;}
}
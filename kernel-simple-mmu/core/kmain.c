#include <stdint.h>
#include <stdarg.h>

extern void uart_init(void);
extern void uart_printf(const char* fmt, ...);
extern void timer_init(void);
extern void enable_gic_distributor(void);
extern void enable_gic_cpu_interface(void);

void kmain(void)
{
    uart_init();

    uart_printf("Kernel Starting\n");
    // uart_printf("Count %d\n", 345);
    // uart_printf("full %s\n", "sdfsd");
    // uart_printf("char %c\n", 'a');
    timer_init();

    uart_printf("Timmer Inited\n");

    enable_gic_distributor();
    enable_gic_cpu_interface();

    uart_printf("Enabled GIC Dis/CPU\n");

    asm volatile("msr daifset, #0x0");
    uart_printf("Enabled Interrupts\n");


    while (1) {int i = 1;}
}
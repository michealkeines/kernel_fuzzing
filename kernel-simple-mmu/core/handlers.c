
#include <stdint.h>

extern uint32_t gic_iar(void);
extern void gic_eoir(uint32_t);
extern void uart_printf(const char *fmt, ...);

extern void timer_handler(void);

void irq_dispatcher(void)
{
    uart_printf("Current irq in disptacher");
    uint32_t id = gic_iar();
    uart_printf("Current irq ID: %d\n", id);
    switch (id)
    {
        case 27: 
        timer_handler(); 
        break;
        default:
        break;
    }

    gic_eoir(id);
}
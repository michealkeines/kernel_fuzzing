#include <stdint.h>

#define GICD_BASE 0x08000000UL // this is where every device will register their interrupts

#define GICC_BASE 0x08010000UL // this where the CPU will receive its one request from GIC


static inline void w32(uint64_t a, uint32_t v)
{
    *(volatile uint32_t*)a = v;
}

static inline uint32_t r32(uint64_t a)
{
    return *(volatile uint32_t*)a;
}

#define IRQ_VTIMER 27

void gic_init(void)
{
    w32(GICD_BASE + 0x000, 1); // Enable GIC, so other devices can register their interrupts

    // enable vtimer, this is per-cpu irq, PPI27, so we need to write to ISENABLER0 (0x100)
    // this way our GIC will only listen for this interrupt, every other interrupt will be ignored by GIC
    w32(GICD_BASE + 0x100, (1u<<IRQ_VTIMER));


    // Enable CPU to listen for interrupts from GIC
    w32(GICC_BASE + 0x000, 1);


    // CPU will only process interrupts, if its priority level is atleast 0xf0, its like saying only give me interrupts that are critical
    w32(GICC_BASE + 0x004, 0xf0);
}

// Interrupt handler will use this to get the ID of the current interrupt
unsigned int gic_ack(void)
{
    // reading from register 0x00C, that is Interrupt Acknowledge Register, IAR
    return r32(GICC_BASE + 0x00C);
}

// Interrupt handler will call this after done with the request handling
void gic_eoi(unsigned int irq)
{
    // writing to register 0x010 to notify we are done, End of Interrupt Register, EOIR
    w32(GICC_BASE + 0x010, irq);
}
#include <stdint.h>

#define GIC_DBASE 0x08000000UL
#define GICD_CTLR (GIC_DBASE + 0x0000) // first is the control register

#define GICD_ISENABLER (GIC_DBASE + 0x100)

#define GIC_CBASE 0x08010000UL
#define GICC_CTLR (GIC_CBASE + 0x0000) // first is the conrol register
#define GICC_PMR (GIC_CBASE + 0x00000004) // priority

#define GICC_IAR (GIC_CBASE + 0x0000000C) // acknowledge
#define GICC_EOIR (GIC_CBASE + 0x00000010) // end

static inline uint32_t read_gicd_ctlr(void)
{
    return *(volatile uint32_t *)GICD_CTLR;
}

static inline void write_gicd_bits(void)
{
    *(volatile uint32_t*)GICD_CTLR = 0x3;
}

static inline void write_gicc_bits(void)
{
    *(volatile uint32_t*)GICC_CTLR = 0x3;
}

static inline uint32_t enable_bit(int count)
{
    uint32_t temp = (1U << count);
    return temp;
}

static inline void write_isenabler0(uint32_t input)
{
    *(volatile uint32_t*)GICD_ISENABLER = input;
}


void enable_gic_distributor(void)
{
    // enable the distributor first
    // *(volatile int32_t*) (GIC_DBASE + 0x000) = 1U;
    write_gicd_bits();
    // enable 27th bit, which is PPI 11
    uint32_t control = enable_bit(27);
    write_isenabler0(control);
}

void enable_gic_cpu_interface(void)
{
    // enable the cpu interface
    // *(volatile int32_t*) (GIC_CBASE + 0x000) = 1U;
    write_gicc_bits();
    *(volatile uint32_t*)GICC_PMR = 0xF0;
}

uint32_t gic_iar()
{
    return *(volatile uint32_t*)GICC_IAR;
}

void gic_eoir(uint32_t id)
{
    *(volatile uint32_t*)GICC_EOIR = id;
}

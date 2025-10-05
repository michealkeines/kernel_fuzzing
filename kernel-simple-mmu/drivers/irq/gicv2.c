#include <stdint.h>

#define GIC_DBASE 0x08000000
#define GICD_CTLR (GIC_DBASE + 0x0000) // first is the control register

#define GICD_ISENABLER (GIC_DBASE + 0x100)

#define GIC_CBASE 0x08010000
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
    uint32_t temp = 0;
    return temp |= (1U << count);
}

static inline void write_isenabler0(uint32_t input)
{
    *(volatile uint32_t*)GICD_ISENABLER = input;
}


void enable_gic_distributor(void)
{
    write_gicd_bits();
}

void enable_gic_cpu_interface(void)
{
    write_gicc_bits();
    // enable 27th bit, which is PPI 11
    uint32_t control = enable_bit(27);
    write_isenabler0(control);

    *(volatile uint32_t*)GICC_PMR = 0xFF;
}

uint32_t gic_iar()
{
    return *(volatile uint32_t*)GICC_IAR;
}

void gic_eoir(uint32_t id)
{
    *(volatile uint32_t*)GICC_IAR = id;
}

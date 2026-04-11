#define VIRTUAL_GICD_BASE 0xffff000081400000UL  // virtual for UART
#define VIRTUAL_GICC_BASE 0xffff000081410000UL  // virtual for UART
#define GICD_BASE 0x0000000008000000UL // this is where every device will register their interrupts

#define GICC_BASE 0x0000000008010000UL // this where the CPU will receive its one request from GIC
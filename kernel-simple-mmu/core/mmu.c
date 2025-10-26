#include <stdint.h>

void mmu_init(void)
{
    const uint64_t L1_BASE = 0x80002000UL;
    const uint64_t L2_BASE = 0x80003000UL;

    for (int i = 0; i < 512; i++) {
        ((uint64_t*)L1_BASE)[i] = 0;
        ((uint64_t*)L2_BASE)[i] = 0;
    }

    ((uint64_t*)L1_BASE)[2] = (L2_BASE | 0b11); // table descriptor

    // Map RAM (0x80000000)
    ((uint64_t*)L2_BASE)[0] =
        (0x80000000UL & 0xFFFFFFE00000UL) |
        (1 << 0) | (0 << 2) | (1 << 10) | (3 << 8);

    // Map UART (as normal memory) at VA 0x82400000 → PA 0x09000000
    ((uint64_t*)L2_BASE)[0x12] =
    (0x09000000UL & 0xFFFFFFE00000UL) |
    (1 << 0) | (0 << 2) | (1 << 10) | (0 << 8);

    uint64_t mair = (0xFF << 0) | (0x04 << 8); // Normal + Device
    uint64_t tcr  = 0x0000000000003520UL;
    uint64_t ttbr = L1_BASE;

    __asm__ volatile(
        "msr MAIR_EL1, %[mair]\n"
        "msr TCR_EL1,  %[tcr]\n"
        "msr TTBR0_EL1,%[ttbr]\n"
        "dsb ishst\n"
        "isb\n"
        "mrs x0, SCTLR_EL1\n"
        "orr x0, x0, #(1 << 0)\n"
        "orr x0, x0, #(1 << 2)\n"
        "orr x0, x0, #(1 << 12)\n"
        "msr SCTLR_EL1, x0\n"
        "dsb sy\n"
        "isb\n"
        :
        : [mair]"r"(mair), [tcr]"r"(tcr), [ttbr]"r"(ttbr)
        : "x0", "memory");
}

#include <stdint.h>

// Function to flip a specific bit
uint64_t flip_bit(uint64_t value, unsigned int index) {
    if (index >= 64) {
        return value;
    }
    return value | ((uint64_t)1 << index);
}

uint64_t flip_bit_range(uint64_t value, unsigned int start, unsigned int end) {
    if (start > end || end >= 64) {
        return value;
    }

    for (unsigned int i = start; i <= end; i++) {
        value = flip_bit(value, i);
    }
    return value;
}

#define TCR_T0SZ(x)   ((uint64_t)(x) & 0x3F)
#define TCR_TG0(x)    ((uint64_t)(x) << 6)
#define TCR_SH0(x)    ((uint64_t)(x) << 8)
#define TCR_ORGN0(x)  ((uint64_t)(x) << 10)
#define TCR_IRGN0(x)  ((uint64_t)(x) << 12)
#define TCR_IPS(x)    ((uint64_t)(x) << 32)

#define TCR_EL1_VALUE ( TCR_IPS(5)      | \
                        TCR_IRGN0(1)    | \
                        TCR_ORGN0(1)    | \
                        TCR_SH0(3)      | \
                        TCR_TG0(0)      | \
                        TCR_T0SZ(16) )


void mmu_init(void)
{
    const uint64_t L0_BASE = 0x80002000UL;
    const uint64_t L1_BASE = 0x80003000UL;
    const uint64_t L2_BASE = 0x80004000UL;

    for (int i = 0; i < 512; i++) {
        ((uint64_t*)L0_BASE)[i] = 0;
        ((uint64_t*)L1_BASE)[i] = 0;
        ((uint64_t*)L2_BASE)[i] = 0;
    }

    uint64_t table_entry = 0x0000000080003; // base address of next table, leaving out last bits

    table_entry = table_entry << 12; // shift left everything to start from 12 bit
    table_entry = table_entry | 0x3; // set as valid table descriptor
    // rest of the bits are 0 by default



    ((uint64_t*) L0_BASE)[0] = table_entry;

    uint64_t table_entry2 = 0x0000000080004; // base address of next table, leaving out last bits

    table_entry2 = table_entry2 << 12; // shift left everything to start from 12 bit
    table_entry2 = table_entry2 | 0x3; // set as valid table descriptor
    // rest of the bits are 0 by default

    ((uint64_t*) L1_BASE)[2] = table_entry2; // 3rd entry will be 0x80000000


    // uint64_t block_entry = 0x0000000080000; // Physicla map base address

    // block_entry = block_entry << 12; // shift left everything to start from 12 bit
    // block_entry = block_entry | 0x1; // set as valid block descriptor

    uint64_t block_entry = ((0x0000000080000ULL) << 12)
                     | (1UL << 10)   // AF=1
                     | (0UL << 2)    // AttrIndx=0 (Normal memory)
                     | (1UL << 0);   // Valid block

((uint64_t*)L2_BASE)[0] = block_entry;

#define UART_BASE_PA   0x09000000ULL   // adjust for your board (QEMU virt)
#define UART_BASE_VA   0x82400000UL 

uint64_t uart_block = (UART_BASE_PA & 0xFFFFFFFFF000ULL)
                    | (1UL << 10)   // AF=1
                    | (1UL << 2)    // AttrIndx=1 (Device)
                    | (1UL << 0);   // valid block
((uint64_t*) L2_BASE)[(UART_BASE_VA >> 21) & 0x1FF] = uart_block;


    uint64_t tcr = 0;

    tcr = TCR_EL1_VALUE; // we want 48 bit va, so 64 - 48 = 16, 16 should be place in T0SZ, TG0 needs to be 4k, so that 00, which is already 00 in out case
    uint64_t ttbr = L0_BASE;


    uint64_t mair = (0xFF << 0) | (0x04 << 8); // set everyrhign as normal and accessible



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

void mmu_init_old(void)
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
